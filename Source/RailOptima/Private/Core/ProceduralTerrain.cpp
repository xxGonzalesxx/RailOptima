// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль
// визуализации», ПривГУПС, 2026.

#include "Core/ProceduralTerrain.h"
#include "Dom/JsonObject.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// ─────────────────────────────────────────────────────────────────
//  Жизненный цикл
// ─────────────────────────────────────────────────────────────────

AProceduralTerrain::AProceduralTerrain()
{
	PrimaryActorTick.bCanEverTick = false;

	TerrainMesh =
		CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainMesh"));
	RootComponent = TerrainMesh;

	TerrainMesh->bUseAsyncCooking = false;
	TerrainMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TerrainMesh->bUseComplexAsSimpleCollision = true;
	TerrainMesh->SetCollisionObjectType(ECC_WorldStatic);
	TerrainMesh->SetCollisionResponseToAllChannels(ECR_Block);
	TerrainMesh->SetCastShadow(true);
}

void AProceduralTerrain::BeginPlay()
{
	Super::BeginPlay();

	if (HeightmapPath.IsEmpty())
	{
		UE_LOG(
			LogTemp, Warning,
			TEXT("[ProceduralTerrain] HeightmapPath пустой — рельеф не построен."));
		return;
	}

	LoadRegionMetadata();

	const FString FullPath = ResolveContentOrProjectPath(HeightmapPath);

	UE_LOG(LogTemp, Log, TEXT("[ProceduralTerrain] Загрузка карты высот из: %s"),
		*FullPath);

	if (!FPaths::FileExists(FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[ProceduralTerrain] Файл НЕ найден: %s."),
			*FullPath);
		return;
	}

	BuildFromHeightmap(FullPath);
}

// ─────────────────────────────────────────────────────────────────
//  Построение
// ─────────────────────────────────────────────────────────────────

bool AProceduralTerrain::BuildFromHeightmap(const FString& InHeightMapPath)
{
	UMapLoader*		   Loader = NewObject<UMapLoader>(this);
	FRailHeightMapData HeightMap;
	if (!Loader->LoadHeightMap(InHeightMapPath, HeightMap))
	{
		UE_LOG(LogTemp, Error, TEXT("[ProceduralTerrain] Не удалось загрузить: %s"),
			*InHeightMapPath);
		return false;
	}

	HeightMap.MinElevation_m = TerrainParameters.MinElevation_m;
	HeightMap.MaxElevation_m = TerrainParameters.MaxElevation_m;
	HeightMap.OriginLat = PendingOriginLat;
	HeightMap.OriginLon = PendingOriginLon;

	CachedHeightMap = HeightMap;

	const int32 Resolution = TerrainParameters.MeshResolution;

	TArray<FVector>		 Vertices;
	TArray<int32>		 Triangles;
	TArray<FLinearColor> Colors;
	TArray<FVector>		 Normals;

	BuildMeshData(HeightMap, Resolution, Vertices, Triangles, Colors, Normals);

	MeshVertexHeightsZ_cm.SetNumUninitialized(Vertices.Num());
	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		MeshVertexHeightsZ_cm[i] = Vertices[i].Z;
	}

	TArray<FVector2D> UVs;
	UVs.SetNumUninitialized(Vertices.Num());
	for (int32 i = 0; i < UVs.Num(); ++i)
	{
		UVs[i] = FVector2D(static_cast<float>(i % Resolution) /
				static_cast<float>(Resolution - 1),
			static_cast<float>(i / Resolution) /
				static_cast<float>(Resolution - 1));
	}

	TArray<FProcMeshTangent> Tangents;
	Tangents.Init(FProcMeshTangent(FVector::ForwardVector, false),
		Vertices.Num());

	TerrainMesh->ClearAllMeshSections();
	TerrainMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals,
		UVs, Colors, Tangents,
		/*bCreateCollision=*/true);

	if (TerrainMaterial)
	{
		TerrainMesh->SetMaterial(0, TerrainMaterial);
	}

	UE_LOG(LogTemp, Log,
		TEXT("[ProceduralTerrain] Рельеф построен: %dx%d, шаг=%.2f м."),
		Resolution, Resolution, TerrainParameters.StepSize_m);
	return true;
}

bool AProceduralTerrain::LoadTerrainFromFiles(const FString& InHeightmapFullPath, const FString& InMetaFullPath)
{
	ClearTerrain();
	if (!InMetaFullPath.IsEmpty() && FPaths::FileExists(InMetaFullPath))
	{
		LoadRegionMetadataFromFile(InMetaFullPath);
	}
	if (!InHeightmapFullPath.IsEmpty() && FPaths::FileExists(InHeightmapFullPath))
	{
		return BuildFromHeightmap(InHeightmapFullPath);
	}
	return false;
}

void AProceduralTerrain::ClearTerrain()
{
	if (TerrainMesh)
	{
		TerrainMesh->ClearAllMeshSections();
	}
	CachedHeightMap = FRailHeightMapData();
	MeshVertexHeightsZ_cm.Reset();
}

void AProceduralTerrain::RebuildTerrain()
{
	if (HeightmapPath.IsEmpty())
		return;
	LoadRegionMetadata();
	const FString FullPath = ResolveContentOrProjectPath(HeightmapPath);
	BuildFromHeightmap(FullPath);
}

// ─────────────────────────────────────────────────────────────────
//  SampleTerrainZ_cm — билинейная интерполяция по сетке меша
// ─────────────────────────────────────────────────────────────────

bool AProceduralTerrain::SampleTerrainZ_cm(const FVector2D& InWorldXY,
	float&													OutZ_cm) const
{
	const int32 Res = TerrainParameters.MeshResolution;
	const float Step_cm = TerrainParameters.StepSize_m * 100.0f;

	if (Res < 2 || Step_cm <= 0.0f)
		return false;
	if (MeshVertexHeightsZ_cm.Num() != Res * Res)
		return false;

	const FVector ActorLoc = GetActorLocation();
	const float FX = (InWorldXY.X - ActorLoc.X) / Step_cm;
	const float FY = (InWorldXY.Y - ActorLoc.Y) / Step_cm;

	// Расширяем допуск до Res - 1 + 0.5f, чтобы точки на границе меша не давали
	// miss.
	const float MaxIdx = static_cast<float>(Res - 1);
	if (FX < -0.5f || FY < -0.5f || FX > MaxIdx + 0.5f || FY > MaxIdx + 0.5f)
		return false;

	const int32 X0 = FMath::Clamp(FMath::FloorToInt(FX), 0, Res - 1);
	const int32 Y0 = FMath::Clamp(FMath::FloorToInt(FY), 0, Res - 1);
	const int32 X1 = FMath::Min(X0 + 1, Res - 1);
	const int32 Y1 = FMath::Min(Y0 + 1, Res - 1);

	const float FracX = FMath::Clamp(FX - X0, 0.0f, 1.0f);
	const float FracY = FMath::Clamp(FY - Y0, 0.0f, 1.0f);

	const float H00 = MeshVertexHeightsZ_cm[Y0 * Res + X0];
	const float H10 = MeshVertexHeightsZ_cm[Y0 * Res + X1];
	const float H01 = MeshVertexHeightsZ_cm[Y1 * Res + X0];
	const float H11 = MeshVertexHeightsZ_cm[Y1 * Res + X1];

	const float H0 = FMath::Lerp(H00, H10, FracX);
	const float H1 = FMath::Lerp(H01, H11, FracX);

	OutZ_cm = FMath::Lerp(H0, H1, FracY);
	return true;
}

// ─────────────────────────────────────────────────────────────────
//  Загрузка region_meta.json
// ─────────────────────────────────────────────────────────────────

bool AProceduralTerrain::LoadRegionMetadata()
{
	if (RegionMetaPath.IsEmpty())
		return false;
	const FString FullPath = ResolveContentOrProjectPath(RegionMetaPath);
	if (!FPaths::FileExists(FullPath))
		return false;
	return LoadRegionMetadataFromFile(FullPath);
}

bool AProceduralTerrain::LoadRegionMetadataFromFile(const FString& InFullPath)
{
	FString RawJson;
	if (!FFileHelper::LoadFileToString(RawJson, *InFullPath))
		return false;

	TSharedPtr<FJsonObject>			Root;
	const TSharedRef<TJsonReader<>> Reader =
		TJsonReaderFactory<>::Create(RawJson);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		return false;

	double MinElev = TerrainParameters.MinElevation_m;
	double MaxElev = TerrainParameters.MaxElevation_m;
	Root->TryGetNumberField(TEXT("min_elevation_m"), MinElev);
	Root->TryGetNumberField(TEXT("max_elevation_m"), MaxElev);

	TerrainParameters.MinElevation_m = static_cast<float>(MinElev);
	TerrainParameters.MaxElevation_m = static_cast<float>(MaxElev);

	double WidthPxD = 0.0;
	double PixelSizeD = 30.0;
	Root->TryGetNumberField(TEXT("width_px"), WidthPxD);
	Root->TryGetNumberField(TEXT("pixel_size_m"), PixelSizeD);

	SourceWidthPx = static_cast<int32>(WidthPxD);
	SourcePixelSize_m = static_cast<float>(PixelSizeD);

	if (SourceWidthPx > 0)
	{
		const int32 MaxRes = FMath::Max(TerrainParameters.MaxMeshResolution, 64);
		int32		TargetRes = SourceWidthPx;

		if (TargetRes > MaxRes)
		{
			TargetRes = MaxRes;
			for (int32 Div = 2; Div <= 32; ++Div)
			{
				if (SourceWidthPx % Div == 0)
				{
					const int32 Candidate = SourceWidthPx / Div;
					if (Candidate <= MaxRes)
					{
						TargetRes = Candidate;
						break;
					}
				}
			}
		}

		TerrainParameters.MeshResolution = TargetRes;
		TerrainParameters.StepSize_m =
			(SourceWidthPx * SourcePixelSize_m) / TargetRes;
	}

	const TSharedPtr<FJsonObject>* BboxObj = nullptr;
	if (Root->TryGetObjectField(TEXT("bbox"), BboxObj) && BboxObj)
	{
		double West = 0.0, South = 0.0;
		(*BboxObj)->TryGetNumberField(TEXT("west"), West);
		(*BboxObj)->TryGetNumberField(TEXT("south"), South);
		PendingOriginLat = South;
		PendingOriginLon = West;
	}

	return true;
}

FString
AProceduralTerrain::ResolveContentOrProjectPath(const FString& InRelativePath)
{
	if (!FPaths::IsRelative(InRelativePath))
		return InRelativePath;

	const FString ViaContent = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectContentDir() / InRelativePath);
	if (FPaths::FileExists(ViaContent))
		return ViaContent;

	const FString ViaRoot =
		FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / InRelativePath);
	if (FPaths::FileExists(ViaRoot))
		return ViaRoot;

	return ViaContent;
}

FLinearColor
AProceduralTerrain::GetColorByHeight(float InNormalizedHeight) const
{
	if (InNormalizedHeight < 0.5f)
	{
		const float T = InNormalizedHeight * 2.0f;
		return FMath::Lerp(TerrainParameters.LowColor, TerrainParameters.MidColor,
			T);
	}

	const float T = (InNormalizedHeight - 0.5f) * 2.0f;
	return FMath::Lerp(TerrainParameters.MidColor, TerrainParameters.HighColor,
		T);
}

void AProceduralTerrain::BuildMeshData(const FRailHeightMapData& InHeightMap,
	int32														 InResolution,
	TArray<FVector>&											 OutVertices,
	TArray<int32>&												 OutTriangles,
	TArray<FLinearColor>&										 OutColors,
	TArray<FVector>&											 OutNormals) const
{
	const int32 NumVertices = InResolution * InResolution;
	const float StepSize = TerrainParameters.StepSize_m;
	const float ZScale = TerrainParameters.ZScale;
	const float ElevRange =
		FMath::Max(InHeightMap.MaxElevation_m - InHeightMap.MinElevation_m, 1.0f);

	OutVertices.Reserve(NumVertices);
	OutColors.Reserve(NumVertices);
	OutNormals.Reserve(NumVertices);
	OutTriangles.Reserve((InResolution - 1) * (InResolution - 1) * 6);

	const float ScaleX = static_cast<float>(InHeightMap.Width - 1) /
		FMath::Max(InResolution - 1, 1);
	const float ScaleY = static_cast<float>(InHeightMap.Height - 1) /
		FMath::Max(InResolution - 1, 1);

	for (int32 Y = 0; Y < InResolution; ++Y)
	{
		for (int32 X = 0; X < InResolution; ++X)
		{
			const float SrcX = X * ScaleX;
			const float SrcY = Y * ScaleY;

			const int32 X0 =
				FMath::Clamp(FMath::FloorToInt(SrcX), 0, InHeightMap.Width - 1);
			const int32 Y0 =
				FMath::Clamp(FMath::FloorToInt(SrcY), 0, InHeightMap.Height - 1);
			const int32 X1 = FMath::Clamp(X0 + 1, 0, InHeightMap.Width - 1);
			const int32 Y1 = FMath::Clamp(Y0 + 1, 0, InHeightMap.Height - 1);

			const float FracX = SrcX - X0;
			const float FracY = SrcY - Y0;

			const float H00 = InHeightMap.Heights[Y0 * InHeightMap.Width + X0];
			const float H10 = InHeightMap.Heights[Y0 * InHeightMap.Width + X1];
			const float H01 = InHeightMap.Heights[Y1 * InHeightMap.Width + X0];
			const float H11 = InHeightMap.Heights[Y1 * InHeightMap.Width + X1];

			const float H0 = FMath::Lerp(H00, H10, FracX);
			const float H1 = FMath::Lerp(H01, H11, FracX);
			const float NormHeight = FMath::Lerp(H0, H1, FracY);

			const float Height_m =
				InHeightMap.MinElevation_m + NormHeight * ElevRange;
			const float Z_cm = Height_m * 100.0f * ZScale;

			OutVertices.Add(
				FVector(X * StepSize * 100.0f, Y * StepSize * 100.0f, Z_cm));
			OutColors.Add(GetColorByHeight(NormHeight));
			OutNormals.Add(FVector::UpVector);
		}
	}

	for (int32 Y = 0; Y < InResolution - 1; ++Y)
	{
		for (int32 X = 0; X < InResolution - 1; ++X)
		{
			const int32 I0 = Y * InResolution + X;
			const int32 I1 = I0 + 1;
			const int32 I2 = I0 + InResolution;
			const int32 I3 = I2 + 1;

			OutTriangles.Add(I0);
			OutTriangles.Add(I2);
			OutTriangles.Add(I1);

			OutTriangles.Add(I1);
			OutTriangles.Add(I2);
			OutTriangles.Add(I3);
		}
	}

	for (int32 i = 0; i < OutNormals.Num(); ++i)
	{
		OutNormals[i] = FVector::ZeroVector;
	}

	for (int32 t = 0; t < OutTriangles.Num(); t += 3)
	{
		const int32 A = OutTriangles[t];
		const int32 B = OutTriangles[t + 1];
		const int32 C = OutTriangles[t + 2];

		const FVector Normal =
			FVector::CrossProduct(OutVertices[B] - OutVertices[A],
				OutVertices[C] - OutVertices[A])
				.GetSafeNormal();

		OutNormals[A] += Normal;
		OutNormals[B] += Normal;
		OutNormals[C] += Normal;
	}

	for (FVector& N : OutNormals)
	{
		N = N.GetSafeNormal();
	}
}
