// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#include "Visualization/RoadRenderer.h"
#include "CollisionQueryParams.h"
#include "Core/ProceduralTerrain.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"

// ─────────────────────────────────────────────────────────────────
//  Жизненный цикл
// ─────────────────────────────────────────────────────────────────

ARoadRenderer::ARoadRenderer()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	RoadMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RoadMesh"));
	RoadMesh->SetupAttachment(SceneRoot);
	RoadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RoadMesh->SetCastShadow(false);
	RoadMesh->bUseAsyncCooking = false;
}

void ARoadRenderer::BeginPlay()
{
	Super::BeginPlay();

	if (RoadMesh)
	{
		RoadMesh->ClearAllMeshSections();
	}

	if (UWorld* World = GetWorld())
	{
		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(
			TimerHandle, this,
			&ARoadRenderer::DeferredBuild,
			FMath::Max(DeferredBuildDelay, 0.01f),
			/*bLoop=*/false);

		UE_LOG(LogTemp, Log,
			TEXT("[RoadRenderer] BeginPlay: построение отложено на %.2f с."),
			DeferredBuildDelay);
	}
	else
	{
		DeferredBuild();
	}
}

void ARoadRenderer::DeferredBuild()
{
	ResolveTerrain();
	UpdateAutoZOffset();

	if (GeoJsonPath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoadRenderer] GeoJsonPath пустой."));
		return;
	}

	const FString FullPath = ResolveFilePath(GeoJsonPath);
	if (!FPaths::FileExists(FullPath))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[RoadRenderer] Файл НЕ найден: %s."), *FullPath);
		return;
	}

	LoadAndBuild(FullPath);
}

// ─────────────────────────────────────────────────────────────────
//  Публичный API
// ─────────────────────────────────────────────────────────────────

bool ARoadRenderer::LoadAndBuild(const FString& InGeoJsonPath)
{
	if (!Terrain)
	{
		UE_LOG(LogTemp, Error, TEXT("[RoadRenderer] Terrain == nullptr."));
		return false;
	}

	FString RawJson;
	if (!FFileHelper::LoadFileToString(RawJson, *InGeoJsonPath))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[RoadRenderer] Не удалось прочитать: %s"), *InGeoJsonPath);
		return false;
	}

	TArray<TArray<FVector2D>> Lines;
	if (!ParseGeoJson(RawJson, Lines))
	{
		UE_LOG(LogTemp, Error, TEXT("[RoadRenderer] Не удалось распарсить GeoJSON."));
		return false;
	}

	if (Lines.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoadRenderer] Нет LineString."));
		return false;
	}

	const FRailHeightMapData& MapData = Terrain->GetHeightMapData();

	UE_LOG(LogTemp, Log,
		TEXT("[RoadRenderer] MapData: OriginLat=%.6f, OriginLon=%.6f, ")
			TEXT("PixelSize=%.2f, Width=%d, ZOffset=%.0f см, AutoZOffset=%s, ")
				TEXT("MinSpacing=%.0f м, MaxVertsPerSection=%d."),
		MapData.OriginLat, MapData.OriginLon,
		MapData.PixelSize_m, MapData.Width,
		ZOffset_cm,
		bAutoZOffset ? TEXT("true") : TEXT("false"),
		MinPointSpacing_m, MaxVertsPerSection);

	if (MapData.Width <= 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[RoadRenderer] Рельеф ещё не построен (Width=%d). "),
			MapData.Width);
		return false;
	}

	// Диагностика первой точки.
	if (Lines.Num() > 0 && Lines[0].Num() > 0)
	{
		const FVector2D FirstWorld = GeoToWorld(Lines[0][0], MapData);
		float			FirstZ = 0.0f;
		const bool		bHit = Terrain->SampleTerrainZ_cm(FirstWorld, FirstZ);
		UE_LOG(LogTemp, Warning,
			TEXT("[RoadRenderer] Первая точка GeoJSON: LatLon=(%.6f, %.6f) → ")
				TEXT("World=(%.0f, %.0f), Z=%.0f см (ok=%s)"),
			Lines[0][0].X, Lines[0][0].Y,
			FirstWorld.X, FirstWorld.Y,
			FirstZ, bHit ? TEXT("yes") : TEXT("no"));
	}

	const FLinearColor Color = (FeatureType == ERoadFeatureType::River)
		? RiverColor
		: RoadColor;

	if (RoadMesh)
		RoadMesh->ClearAllMeshSections();
	SectionBuffer.Reset();
	CurrentSectionIndex = 0;
	TotalVertsWritten = 0;
	TotalTrisWritten = 0;
	SectionsCreated = 0;

	const float MinSpacingCm = FMath::Max(MinPointSpacing_m, 0.0f) * 100.0f;

	int32 TotalLines = 0;
	int32 TotalPointsRaw = 0;
	int32 TotalPointsSimplified = 0;
	int32 TotalPointsAfterZ = 0;
	int32 TraceHits = 0;
	int32 TraceMisses = 0;

	for (const TArray<FVector2D>& Line : Lines)
	{
		if (Line.Num() < 2)
			continue;

		TArray<FVector2D> WorldXY;
		WorldXY.Reserve(Line.Num());
		for (const FVector2D& LatLon : Line)
		{
			WorldXY.Add(GeoToWorld(LatLon, MapData));
		}
		TotalPointsRaw += WorldXY.Num();

		TArray<FVector2D> Simplified;
		SimplifyLine(WorldXY, MinSpacingCm, Simplified);
		if (Simplified.Num() < 2)
			continue;

		TotalPointsSimplified += Simplified.Num();

		TArray<float> Zs;
		Zs.SetNumUninitialized(Simplified.Num());
		for (int32 i = 0; i < Simplified.Num(); ++i)
		{
			float Z = 0.0f;
			if (Terrain->SampleTerrainZ_cm(Simplified[i], Z))
			{
				++TraceHits;
			}
			else
			{
				++TraceMisses;
				Z = GetTerrainZFromMap(Simplified[i], MapData);
			}
			Zs[i] = Z;
		}

		SmoothZ(Zs, ZSmoothingRadius);

		TArray<FVector> WorldPoints;
		WorldPoints.Reserve(Simplified.Num());
		for (int32 i = 0; i < Simplified.Num(); ++i)
		{
			WorldPoints.Add(FVector(
				Simplified[i].X,
				Simplified[i].Y,
				Zs[i] + ZOffset_cm));
		}

		TotalPointsAfterZ += WorldPoints.Num();

		AppendRibbon(WorldPoints, Color);
		++TotalLines;
	}

	FlushSection();

	if (RoadMaterial)
	{
		RoadMesh->SetMaterial(0, RoadMaterial);
	}

	if (bLogStats)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[RoadRenderer] Построено: линий=%d, точек raw=%d → simplified=%d → ")
				TEXT("после Z=%d, вершин=%d, треугольников=%d, секций=%d, ")
					TEXT("SampleTerrain hit=%d / miss=%d."),
			TotalLines,
			TotalPointsRaw, TotalPointsSimplified, TotalPointsAfterZ,
			TotalVertsWritten, TotalTrisWritten, SectionsCreated,
			TraceHits, TraceMisses);
	}

	return true;
}

void ARoadRenderer::ClearGeometry()
{
	if (RoadMesh)
		RoadMesh->ClearAllMeshSections();
	SectionBuffer.Reset();
	CurrentSectionIndex = 0;
	TotalVertsWritten = 0;
	TotalTrisWritten = 0;
	SectionsCreated = 0;
}

void ARoadRenderer::Rebuild()
{
	ResolveTerrain();
	UpdateAutoZOffset();

	if (GeoJsonPath.IsEmpty())
		return;
	const FString FullPath = ResolveFilePath(GeoJsonPath);
	LoadAndBuild(FullPath);
}

// ─────────────────────────────────────────────────────────────────
//  Секционирование
// ─────────────────────────────────────────────────────────────────

void ARoadRenderer::FlushSection()
{
	if (!RoadMesh)
		return;
	if (SectionBuffer.NumVerts() == 0)
		return;

	RoadMesh->CreateMeshSection_LinearColor(
		CurrentSectionIndex,
		SectionBuffer.Vertices,
		SectionBuffer.Triangles,
		SectionBuffer.Normals,
		SectionBuffer.UVs,
		SectionBuffer.Colors,
		SectionBuffer.Tangents,
		/*bCreateCollision=*/false,
		/*bSRGBConversion=*/true);

	TotalVertsWritten += SectionBuffer.NumVerts();
	TotalTrisWritten += SectionBuffer.Triangles.Num() / 3;
	++SectionsCreated;

	SectionBuffer.Reset();
	++CurrentSectionIndex;
}

// ─────────────────────────────────────────────────────────────────
//  Резолвинг рельефа
// ─────────────────────────────────────────────────────────────────

void ARoadRenderer::ResolveTerrain()
{
	if (Terrain)
		return;

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),
		AProceduralTerrain::StaticClass(), Found);

	if (Found.Num() > 0)
	{
		Terrain = Cast<AProceduralTerrain>(Found[0]);
		UE_LOG(LogTemp, Log,
			TEXT("[RoadRenderer] Terrain найден: '%s'."), *Terrain->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RoadRenderer] AProceduralTerrain не найден на уровне."));
	}
}

void ARoadRenderer::UpdateAutoZOffset()
{
	if (!bAutoZOffset)
		return;

	if (!Terrain)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RoadRenderer] AutoZOffset: Terrain == nullptr, ")
				TEXT("оставляю ZOffset_cm = %.0f."),
			ZOffset_cm);
		return;
	}

	const float Step_m = Terrain->TerrainParameters.StepSize_m;
	const float Step_cm = Step_m * 100.0f;
	ZOffset_cm = Step_cm * AutoZOffsetFactor;

	UE_LOG(LogTemp, Log,
		TEXT("[RoadRenderer] AutoZOffset: шаг меша=%.2f м → ZOffset=%.0f см."),
		Step_m, ZOffset_cm);
}

// ─────────────────────────────────────────────────────────────────
//  Парсинг GeoJSON
// ─────────────────────────────────────────────────────────────────

bool ARoadRenderer::ParseGeoJson(const FString& InRawJson,
	TArray<TArray<FVector2D>>&					OutLines) const
{
	OutLines.Reset();

	TSharedPtr<FJsonObject>			Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InRawJson);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		return false;

	const TArray<TSharedPtr<FJsonValue>>* FeaturesArray = nullptr;
	if (!Root->TryGetArrayField(TEXT("features"), FeaturesArray) || !FeaturesArray)
		return false;

	for (const TSharedPtr<FJsonValue>& FeatureValue : *FeaturesArray)
	{
		const TSharedPtr<FJsonObject>* FeatureObj = nullptr;
		if (!FeatureValue->TryGetObject(FeatureObj) || !FeatureObj)
			continue;

		const TSharedPtr<FJsonObject>* GeometryObj = nullptr;
		if (!(*FeatureObj)->TryGetObjectField(TEXT("geometry"), GeometryObj) || !GeometryObj)
			continue;

		FString GeomType;
		if (!(*GeometryObj)->TryGetStringField(TEXT("type"), GeomType))
			continue;

		if (GeomType == TEXT("LineString"))
		{
			TArray<FVector2D>					  Line;
			const TArray<TSharedPtr<FJsonValue>>* CoordsArray = nullptr;
			if ((*GeometryObj)->TryGetArrayField(TEXT("coordinates"), CoordsArray) && CoordsArray)
			{
				for (const TSharedPtr<FJsonValue>& CoordValue : *CoordsArray)
				{
					const TArray<TSharedPtr<FJsonValue>>* Pair = nullptr;
					if (!CoordValue->TryGetArray(Pair) || !Pair || Pair->Num() < 2)
						continue;
					const double Lon = (*Pair)[0]->AsNumber();
					const double Lat = (*Pair)[1]->AsNumber();
					Line.Add(FVector2D(Lat, Lon));
				}
			}
			if (Line.Num() >= 2)
				OutLines.Add(MoveTemp(Line));
		}
		else if (GeomType == TEXT("MultiLineString"))
		{
			const TArray<TSharedPtr<FJsonValue>>* MultiCoords = nullptr;
			if (!(*GeometryObj)->TryGetArrayField(TEXT("coordinates"), MultiCoords) || !MultiCoords)
				continue;

			for (const TSharedPtr<FJsonValue>& LineValue : *MultiCoords)
			{
				const TArray<TSharedPtr<FJsonValue>>* CoordsArray = nullptr;
				if (!LineValue->TryGetArray(CoordsArray) || !CoordsArray)
					continue;

				TArray<FVector2D> Line;
				for (const TSharedPtr<FJsonValue>& CoordValue : *CoordsArray)
				{
					const TArray<TSharedPtr<FJsonValue>>* Pair = nullptr;
					if (!CoordValue->TryGetArray(Pair) || !Pair || Pair->Num() < 2)
						continue;
					const double Lon = (*Pair)[0]->AsNumber();
					const double Lat = (*Pair)[1]->AsNumber();
					Line.Add(FVector2D(Lat, Lon));
				}
				if (Line.Num() >= 2)
					OutLines.Add(MoveTemp(Line));
			}
		}
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────
//  Прореживание
// ─────────────────────────────────────────────────────────────────

void ARoadRenderer::SimplifyLine(const TArray<FVector2D>& InLine,
	float												  InMinSpacingCm,
	TArray<FVector2D>&									  OutLine)
{
	OutLine.Reset();
	if (InLine.Num() == 0)
		return;

	OutLine.Add(InLine[0]);

	if (InMinSpacingCm <= KINDA_SMALL_NUMBER)
	{
		for (int32 i = 1; i < InLine.Num(); ++i)
		{
			OutLine.Add(InLine[i]);
		}
		return;
	}

	const float SpacingSq = InMinSpacingCm * InMinSpacingCm;

	for (int32 i = 1; i < InLine.Num(); ++i)
	{
		const FVector2D& Last = OutLine.Last();
		const FVector2D& Curr = InLine[i];
		const float		 DistSq = FVector2D::DistSquared(Last, Curr);

		if (DistSq >= SpacingSq)
		{
			OutLine.Add(Curr);
		}
	}

	const FVector2D& LastOrig = InLine.Last();
	if (!OutLine.Last().Equals(LastOrig, 1.0f))
	{
		OutLine.Add(LastOrig);
	}
}

// ─────────────────────────────────────────────────────────────────
//  Координаты — с инверсией Y
// ─────────────────────────────────────────────────────────────────

FVector2D ARoadRenderer::GeoToWorld(const FVector2D& InLatLon,
	const FRailHeightMapData&						 MapData) const
{
	const double LatMetersPerDegree = 111320.0;
	const double CenterLat = MapData.OriginLat + 0.5;
	const double LonMetersPerDegree =
		111320.0 * FMath::Cos(FMath::DegreesToRadians(CenterLat));

	double MapSizeMeters = 0.0;
	if (Terrain)
	{
		MapSizeMeters = Terrain->GetMapSizeMeters();
	}
	else
	{
		MapSizeMeters = MapData.Width * MapData.PixelSize_m;
	}

	const double LatSpanDeg = MapSizeMeters / LatMetersPerDegree;
	const double LonSpanDeg = MapSizeMeters / LonMetersPerDegree;

	double NormX = (InLatLon.Y - MapData.OriginLon) / LonSpanDeg;

	// Y: инверсия. PNG строка 0 = север, Lat растёт к северу.
	double NormY = 1.0 - (InLatLon.X - MapData.OriginLat) / LatSpanDeg;

	if (bClampToMapBounds)
	{
		NormX = FMath::Clamp(NormX, 0.0, 1.0);
		NormY = FMath::Clamp(NormY, 0.0, 1.0);
	}

	return FVector2D(NormX * MapSizeMeters * 100.0,
		NormY * MapSizeMeters * 100.0);
}

// ─────────────────────────────────────────────────────────────────
//  Fallback — интерполяция по PNG
// ─────────────────────────────────────────────────────────────────

float ARoadRenderer::GetTerrainZFromMap(const FVector2D& InWorldXY,
	const FRailHeightMapData&							 MapData) const
{
	if (!Terrain)
		return 0.0f;

	const double MapSizeCm = Terrain->GetMapSizeMeters() * 100.0;
	if (MapSizeCm <= 0.0)
		return 0.0f;

	const double NormX = FMath::Clamp(InWorldXY.X / MapSizeCm, 0.0, 1.0);
	const double NormY = FMath::Clamp(InWorldXY.Y / MapSizeCm, 0.0, 1.0);

	const double PixelX = NormX * (MapData.Width - 1);
	const double PixelY = NormY * (MapData.Height - 1);

	const int32 X0 = FMath::Clamp(FMath::FloorToInt(PixelX), 0, MapData.Width - 1);
	const int32 Y0 = FMath::Clamp(FMath::FloorToInt(PixelY), 0, MapData.Height - 1);
	const int32 X1 = FMath::Clamp(X0 + 1, 0, MapData.Width - 1);
	const int32 Y1 = FMath::Clamp(Y0 + 1, 0, MapData.Height - 1);

	const float FracX = static_cast<float>(PixelX - X0);
	const float FracY = static_cast<float>(PixelY - Y0);

	auto Sample = [&](int32 X, int32 Y) -> float {
		return MapData.Heights[Y * MapData.Width + X];
	};

	const float H00 = Sample(X0, Y0);
	const float H10 = Sample(X1, Y0);
	const float H01 = Sample(X0, Y1);
	const float H11 = Sample(X1, Y1);

	const float H0 = FMath::Lerp(H00, H10, FracX);
	const float H1 = FMath::Lerp(H01, H11, FracX);
	const float NormH = FMath::Lerp(H0, H1, FracY);

	const float ElevRange = MapData.MaxElevation_m - MapData.MinElevation_m;
	const float Height_m = MapData.MinElevation_m + NormH * ElevRange;

	return Height_m * 100.0f * WorldZScale;
}

// ─────────────────────────────────────────────────────────────────
//  Сглаживание Z
// ─────────────────────────────────────────────────────────────────

void ARoadRenderer::SmoothZ(TArray<float>& InOutZ, int32 InRadius)
{
	const int32 N = InOutZ.Num();
	if (InRadius <= 0 || N < 3)
		return;

	TArray<float> Source = InOutZ;

	for (int32 i = 0; i < N; ++i)
	{
		const int32 Lo = FMath::Max(0, i - InRadius);
		const int32 Hi = FMath::Min(N - 1, i + InRadius);

		float Sum = 0.0f;
		int32 Count = 0;
		for (int32 j = Lo; j <= Hi; ++j)
		{
			Sum += Source[j];
			++Count;
		}
		InOutZ[i] = (Count > 0) ? (Sum / Count) : Source[i];
	}
}

// ─────────────────────────────────────────────────────────────────
//  Лента
// ─────────────────────────────────────────────────────────────────

void ARoadRenderer::AppendRibbon(const TArray<FVector>& InPoints,
	const FLinearColor&									InColor)
{
	const int32 N = InPoints.Num();
	if (N < 2)
		return;

	const float HalfWidth = Width_cm * 0.5f;

	TArray<FVector2D> Directions;
	Directions.SetNumUninitialized(N);

	auto Seg2D = [](const FVector& A, const FVector& B) -> FVector2D {
		FVector2D	D(B.X - A.X, B.Y - A.Y);
		const float L = D.Size();
		return (L > KINDA_SMALL_NUMBER) ? (D / L) : FVector2D(1.0f, 0.0f);
	};

	Directions[0] = Seg2D(InPoints[0], InPoints[1]);
	Directions[N - 1] = Seg2D(InPoints[N - 2], InPoints[N - 1]);

	for (int32 i = 1; i < N - 1; ++i)
	{
		const FVector2D P = Seg2D(InPoints[i - 1], InPoints[i]);
		const FVector2D Q = Seg2D(InPoints[i], InPoints[i + 1]);
		const FVector2D Avg = P + Q;
		const float		L = Avg.Size();
		Directions[i] = (L > KINDA_SMALL_NUMBER) ? (Avg / L) : Q;
	}

	const int32 VertsNeeded = (N - 1) * 2;

	if (VertsNeeded > MaxVertsPerSection)
	{
		const int32 MaxPointsPerChunk = (MaxVertsPerSection / 2) + 1;

		for (int32 Start = 0; Start < N - 1; Start += MaxPointsPerChunk - 1)
		{
			const int32 End = FMath::Min(Start + MaxPointsPerChunk, N);
			const int32 ChunkSize = End - Start;
			if (ChunkSize < 2)
				break;

			if (SectionBuffer.NumVerts() + (ChunkSize - 1) * 2 > MaxVertsPerSection)
			{
				FlushSection();
			}

			TArray<FVector>	  ChunkPoints;
			TArray<FVector2D> ChunkDirs;
			ChunkPoints.Reserve(ChunkSize);
			ChunkDirs.Reserve(ChunkSize);
			for (int32 k = 0; k < ChunkSize; ++k)
			{
				ChunkPoints.Add(InPoints[Start + k]);
				ChunkDirs.Add(Directions[Start + k]);
			}

			const int32 BaseIndex = SectionBuffer.Vertices.Num();

			for (int32 i = 0; i < ChunkSize; ++i)
			{
				const FVector&	 P = ChunkPoints[i];
				const FVector2D& D = ChunkDirs[i];
				const FVector2D	 Perp(-D.Y, D.X);
				const FVector	 Lateral(Perp.X * HalfWidth, Perp.Y * HalfWidth, 0.0f);

				SectionBuffer.Vertices.Add(P - Lateral);
				SectionBuffer.Colors.Add(InColor);
				SectionBuffer.Normals.Add(FVector::UpVector);
				SectionBuffer.UVs.Add(FVector2D(0.0f, static_cast<float>(i)));
				SectionBuffer.Tangents.Add(FProcMeshTangent(FVector(D.X, D.Y, 0.0f), false));

				SectionBuffer.Vertices.Add(P + Lateral);
				SectionBuffer.Colors.Add(InColor);
				SectionBuffer.Normals.Add(FVector::UpVector);
				SectionBuffer.UVs.Add(FVector2D(1.0f, static_cast<float>(i)));
				SectionBuffer.Tangents.Add(FProcMeshTangent(FVector(D.X, D.Y, 0.0f), false));
			}

			for (int32 i = 0; i < ChunkSize - 1; ++i)
			{
				const int32 L0 = BaseIndex + i * 2 + 0;
				const int32 R0 = BaseIndex + i * 2 + 1;
				const int32 L1 = BaseIndex + (i + 1) * 2 + 0;
				const int32 R1 = BaseIndex + (i + 1) * 2 + 1;

				SectionBuffer.Triangles.Add(L0);
				SectionBuffer.Triangles.Add(R0);
				SectionBuffer.Triangles.Add(L1);
				SectionBuffer.Triangles.Add(R0);
				SectionBuffer.Triangles.Add(R1);
				SectionBuffer.Triangles.Add(L1);
			}
		}
		return;
	}

	if (SectionBuffer.NumVerts() + VertsNeeded > MaxVertsPerSection)
	{
		FlushSection();
	}

	const int32 BaseIndex = SectionBuffer.Vertices.Num();

	for (int32 i = 0; i < N; ++i)
	{
		const FVector&	 P = InPoints[i];
		const FVector2D& D = Directions[i];
		const FVector2D	 Perp(-D.Y, D.X);
		const FVector	 Lateral(Perp.X * HalfWidth, Perp.Y * HalfWidth, 0.0f);

		SectionBuffer.Vertices.Add(P - Lateral);
		SectionBuffer.Colors.Add(InColor);
		SectionBuffer.Normals.Add(FVector::UpVector);
		SectionBuffer.UVs.Add(FVector2D(0.0f, static_cast<float>(i)));
		SectionBuffer.Tangents.Add(FProcMeshTangent(FVector(D.X, D.Y, 0.0f), false));

		SectionBuffer.Vertices.Add(P + Lateral);
		SectionBuffer.Colors.Add(InColor);
		SectionBuffer.Normals.Add(FVector::UpVector);
		SectionBuffer.UVs.Add(FVector2D(1.0f, static_cast<float>(i)));
		SectionBuffer.Tangents.Add(FProcMeshTangent(FVector(D.X, D.Y, 0.0f), false));
	}

	for (int32 i = 0; i < N - 1; ++i)
	{
		const int32 L0 = BaseIndex + i * 2 + 0;
		const int32 R0 = BaseIndex + i * 2 + 1;
		const int32 L1 = BaseIndex + (i + 1) * 2 + 0;
		const int32 R1 = BaseIndex + (i + 1) * 2 + 1;

		SectionBuffer.Triangles.Add(L0);
		SectionBuffer.Triangles.Add(R0);
		SectionBuffer.Triangles.Add(L1);
		SectionBuffer.Triangles.Add(R0);
		SectionBuffer.Triangles.Add(R1);
		SectionBuffer.Triangles.Add(L1);
	}
}

// ─────────────────────────────────────────────────────────────────
//  Разрешение пути
// ─────────────────────────────────────────────────────────────────

FString ARoadRenderer::ResolveFilePath(const FString& InRelativePath)
{
	if (!FPaths::IsRelative(InRelativePath))
		return InRelativePath;

	const FString ViaContent = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectContentDir() / InRelativePath);
	if (FPaths::FileExists(ViaContent))
		return ViaContent;

	const FString ViaRoot = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectDir() / InRelativePath);
	if (FPaths::FileExists(ViaRoot))
		return ViaRoot;

	return ViaContent;
}