// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#include "RailOptimaGameMode.h"
#include "Algorithms/AStarPathFinder.h"
#include "Algorithms/CostCalculator.h"
#include "Core/ProceduralTerrain.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Interactive/RailEndpoint.h"
#include "Interactive/RailOptimaPlayerController.h"
#include "Interactive/RailSpectatorPawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Visualization/PathRenderer.h"

// ─────────────────────────────────────────────────────────────────
//  Жизненный цикл
// ─────────────────────────────────────────────────────────────────

ARailOptimaGameMode::ARailOptimaGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
	PlayerControllerClass = ARailOptimaPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void ARailOptimaGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("[RailGM] ─────────────────────────────────────────────"));
	UE_LOG(LogTemp, Log, TEXT("[RailGM] BeginPlay."));

	SetupSubsystems();
	FindTerrain();
	SetupPathRenderer();
	SetupEndpoints();

	RecomputePath();

	UE_LOG(LogTemp, Log, TEXT("[RailGM] ─────────────────────────────────────────────"));
}

// ─────────────────────────────────────────────────────────────────
//  Публичный API
// ─────────────────────────────────────────────────────────────────

void ARailOptimaGameMode::ApplySettings(
	float InSlopePenalty, float InTurnPenalty, float InStepSize_m, bool bInColorByCost)
{
	if (!CostCalculator)
		return;

	const bool bStepChanged = !FMath::IsNearlyEqual(RuntimeSettings.TargetStepSize_m, InStepSize_m, 1.0f);

	RuntimeSettings.SlopePenalty = FMath::Clamp(InSlopePenalty, 0.0f, 100.0f);
	RuntimeSettings.TurnPenalty = FMath::Clamp(InTurnPenalty, 0.0f, 100.0f);
	RuntimeSettings.TargetStepSize_m = FMath::Clamp(InStepSize_m, 10.0f, 1000.0f);
	RuntimeSettings.bColorPathByCost = bInColorByCost;

	ApplySelectedCategory();

	if (bStepChanged)
	{
		bGridCached = false;
		CachedGrid = FRailGridData();
	}
}

void ARailOptimaGameMode::ApplyViewScaleFromCamera(ARailSpectatorPawn* InPawn)
{
	if (!InPawn)
		return;

	float TargetScale = 1.0f;

	if (InPawn->GetCameraMode() == ERailCameraMode::TopDown)
	{
		const float CurrentHeight = InPawn->GetTopDownHeight();

		if (!bViewScaleReferenceCaptured && CurrentHeight > 0.0f)
		{
			ReferenceTopDownHeight_cm = CurrentHeight;
			bViewScaleReferenceCaptured = true;
		}
		if (ReferenceTopDownHeight_cm > 0.0f)
		{
			TargetScale = CurrentHeight / ReferenceTopDownHeight_cm;
		}
	}

	TargetScale = FMath::Clamp(TargetScale, MinViewScale, MaxViewScale);
	if (FMath::IsNearlyEqual(TargetScale, CurrentViewScale, 0.02f))
		return;

	CurrentViewScale = TargetScale;

	if (StartEndpoint)
		StartEndpoint->SetViewScale(CurrentViewScale);
	if (EndEndpoint)
		EndEndpoint->SetViewScale(CurrentViewScale);

	if (PathRenderer)
	{
		PathRenderer->SetViewScale(CurrentViewScale);
		if (bGridCached && CachedGrid.Width > 1)
			RecomputePath();
	}
}

void ARailOptimaGameMode::SetSelectedCategory(ERailRoadCategory NewCategory)
{
	if (SelectedCategory == NewCategory)
		return;

	SelectedCategory = NewCategory;
	ApplySelectedCategory();
	RecomputePath();
}

void ARailOptimaGameMode::ApplySelectedCategory()
{
	if (!PathFinder || !CostCalculator)
		return;

	const FRailCategoryNorms Norms = FRailCategoryNorms::GetNorms(SelectedCategory);
	PathFinder->SetCategoryNorms(Norms);

	float CategoryTurnPenalty = RuntimeSettings.TurnPenalty;
	switch (SelectedCategory)
	{
		case ERailRoadCategory::HighSpeed:
			CategoryTurnPenalty = 3.0f;
			break;
		case ERailRoadCategory::SuperHeavyHaul:
			CategoryTurnPenalty = 2.5f;
			break;
		case ERailRoadCategory::Category1:
			CategoryTurnPenalty = 2.0f;
			break;
		case ERailRoadCategory::Category2:
			CategoryTurnPenalty = 1.5f;
			break;
		case ERailRoadCategory::Category3:
			CategoryTurnPenalty = 1.0f;
			break;
		case ERailRoadCategory::Category4Lines:
			CategoryTurnPenalty = 0.5f;
			break;
		case ERailRoadCategory::Category4Access:
			CategoryTurnPenalty = 0.2f;
			break;
	}

	CostCalculator->Parameters.TurnPenalty = CategoryTurnPenalty;
	CostCalculator->Parameters.SlopePenalty = RuntimeSettings.SlopePenalty;

	UE_LOG(LogTemp, Log, TEXT("[RailGM] Нормы: MaxSlope=%.1f‰, TurnPenalty=%.2f, Rв_min=%.0f м."),
		Norms.MaxSlopePermille, CategoryTurnPenalty, Norms.MinVerticalRadius);
}

// ─────────────────────────────────────────────────────────────────
//  Настройка подсистем
// ─────────────────────────────────────────────────────────────────

void ARailOptimaGameMode::SetupSubsystems()
{
	PathFinder = NewObject<UAStarPathFinder>(this);
	CostCalculator = NewObject<UCostCalculator>(this);
	PathFinder->SetCostCalculator(CostCalculator);
	ApplySelectedCategory();
}

void ARailOptimaGameMode::FindTerrain()
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProceduralTerrain::StaticClass(), Found);

	if (Found.Num() > 0)
	{
		Terrain = Cast<AProceduralTerrain>(Found[0]);
		UE_LOG(LogTemp, Log, TEXT("[RailGM] Рельеф: '%s'."), *Terrain->GetName());
	}
	else
	{
		Terrain = nullptr;
	}
}

void ARailOptimaGameMode::SetupPathRenderer()
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APathRenderer::StaticClass(), Found);

	if (Found.Num() > 0)
	{
		PathRenderer = Cast<APathRenderer>(Found[0]);
	}
	else
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		PathRenderer = GetWorld()->SpawnActor<APathRenderer>(APathRenderer::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	}

	if (PathRenderer && Terrain)
	{
		PathRenderer->WorldZScale = Terrain->TerrainParameters.ZScale;
		const FVector Loc = Terrain->GetActorLocation();
		PathRenderer->GridWorldOrigin = FVector(Loc.X, Loc.Y, 0.0f);
	}
}

void ARailOptimaGameMode::SetupEndpoints()
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARailEndpoint::StaticClass(), Found);

	for (AActor* Actor : Found)
	{
		ARailEndpoint* E = Cast<ARailEndpoint>(Actor);
		if (!E)
			continue;
		if (E->bIsStart && !StartEndpoint)
			StartEndpoint = E;
		else if (!E->bIsStart && !EndEndpoint)
			EndEndpoint = E;
	}

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (!StartEndpoint)
	{
		StartEndpoint = GetWorld()->SpawnActor<ARailEndpoint>(ARailEndpoint::StaticClass(), FVector(0, 0, 0), FRotator::ZeroRotator, SP);
		if (StartEndpoint)
			StartEndpoint->bIsStart = true;
	}
	if (!EndEndpoint)
	{
		EndEndpoint = GetWorld()->SpawnActor<ARailEndpoint>(ARailEndpoint::StaticClass(), FVector(0, 0, 0), FRotator::ZeroRotator, SP);
		if (EndEndpoint)
			EndEndpoint->bIsStart = false;
	}

	if (StartEndpoint)
		StartEndpoint->OnEndpointMoved.AddDynamic(this, &ARailOptimaGameMode::HandleEndpointMoved);
	if (EndEndpoint)
		EndEndpoint->OnEndpointMoved.AddDynamic(this, &ARailOptimaGameMode::HandleEndpointMoved);
}

// ─────────────────────────────────────────────────────────────────
//  Пересчёт пути
// ─────────────────────────────────────────────────────────────────

void ARailOptimaGameMode::RecomputePath()
{
	if (!PathFinder || !PathRenderer || !StartEndpoint || !EndEndpoint)
		return;

	if (!bGridCached)
	{
		if (Terrain && Terrain->GetHeightMapData().Heights.Num() > 0)
			CachedGrid = BuildGridFromTerrain(*Terrain, RuntimeSettings.TargetStepSize_m);
		else
			CachedGrid = GenerateSyntheticGrid(336, RuntimeSettings.TargetStepSize_m);

		bGridCached = CachedGrid.Width > 1 && CachedGrid.Height > 1;
	}

	if (!bGridCached)
		return;

	const int32 StartIdx = WorldToNodeIndex(CachedGrid, StartEndpoint->GetActorLocation());
	const int32 EndIdx = WorldToNodeIndex(CachedGrid, EndEndpoint->GetActorLocation());

	if (StartIdx == INDEX_NONE || EndIdx == INDEX_NONE || StartIdx == EndIdx)
	{
		PathRenderer->ClearPath();
		OnPathRecomputed.Broadcast(0.0, 0.0, 0.0, 0);
		return;
	}

	const double		  T0 = FPlatformTime::Seconds();
	const FRailPathResult Result = PathFinder->FindPath(CachedGrid, StartIdx, EndIdx);
	const double		  Ms = (FPlatformTime::Seconds() - T0) * 1000.0;

	if (!Result.bFound)
	{
		PathRenderer->ClearPath();
		OnPathRecomputed.Broadcast(0.0, 0.0, Ms, 0);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[RailGM] Путь: длина=%.0f м, стоимость=%.1f, время=%.1f мс."), Result.TotalLength_m, Result.TotalCost, Ms);

	const FRailCategoryNorms Norms = FRailCategoryNorms::GetNorms(SelectedCategory);
	TArray<FVector>			 SmoothPoints = GenerateRailwayArcs(CachedGrid, Result, Norms);

	if (SmoothPoints.Num() > 1)
	{
		TArray<FLinearColor> Colors;
		Colors.SetNumUninitialized(SmoothPoints.Num());

		for (int32 i = 0; i < SmoothPoints.Num(); ++i)
		{
			float T = (float)i / (float)(SmoothPoints.Num() - 1);
			Colors[i] = FMath::Lerp(PathRenderer->CheapColor, PathRenderer->ExpensiveColor, T);
		}
		PathRenderer->RenderCustomPath(SmoothPoints, Colors);
	}
	else
	{
		PathRenderer->ClearPath();
	}

	OnPathRecomputed.Broadcast(Result.TotalLength_m, Result.TotalCost, Ms, Result.NodeIndices.Num());
}

TArray<FVector> ARailOptimaGameMode::GenerateRailwayArcs(const FRailGridData& InGrid, const FRailPathResult& InPath, const FRailCategoryNorms& InNorms) const
{
	TArray<FVector> OutPoints;
	if (!InPath.bFound || InPath.NodeIndices.Num() < 2)
		return OutPoints;

	UE_LOG(LogTemp, Log, TEXT("[ArcGen] Начало: A* узлов = %d"), InPath.NodeIndices.Num());

	TArray<FVector2D> RawPoints;
	for (int32 Idx : InPath.NodeIndices)
	{
		const auto& N = InGrid.Nodes[Idx];
		RawPoints.Add(FVector2D(N.X * InGrid.StepSize_m * 100.0f, N.Y * InGrid.StepSize_m * 100.0f));
	}

	// RDP с допуском 150 метров (15000 см)
	const float		  RDP_Tolerance = 15000.0f;
	TArray<FVector2D> Simplified;
	Simplified.Add(RawPoints[0]);

	TFunction<void(int32, int32)> RDP = [&](int32 start, int32 end) {
		float	  maxDist = 0.0f;
		int32	  index = -1;
		FVector2D startPt = RawPoints[start];
		FVector2D endPt = RawPoints[end];

		for (int32 i = start + 1; i < end; ++i)
		{
			FVector2D pt = RawPoints[i];
			float	  d = FMath::Abs((endPt.X - startPt.X) * (startPt.Y - pt.Y) - (startPt.X - pt.X) * (endPt.Y - startPt.Y));
			float	  len = FVector2D::Distance(startPt, endPt);
			if (len > 0)
				d /= len;
			if (d > maxDist)
			{
				maxDist = d;
				index = i;
			}
		}

		if (maxDist > RDP_Tolerance && index != -1)
		{
			RDP(start, index);
			RDP(index, end);
		}
		else
		{
			if (!Simplified.Last().Equals(endPt, 1.0f))
				Simplified.Add(endPt);
		}
	};
	RDP(0, RawPoints.Num() - 1);

	UE_LOG(LogTemp, Log, TEXT("[ArcGen] После RDP: узлов = %d"), Simplified.Num());

	float TargetRadius = (InNorms.RecommendedRadii.Num() > 0) ? InNorms.RecommendedRadii[0] : InNorms.MinRadiusDifficult;
	TargetRadius *= 100.0f;

	UE_LOG(LogTemp, Log, TEXT("[ArcGen] Целевой радиус: %.0f м"), TargetRadius / 100.0f);

	TArray<FVector2D> Final2D;
	Final2D.Add(Simplified[0]);

	for (int32 i = 1; i < Simplified.Num() - 1; ++i)
	{
		FVector2D P0 = Final2D.Last();
		FVector2D P1 = Simplified[i];
		FVector2D P2 = Simplified[i + 1];

		FVector2D V1 = (P1 - P0).GetSafeNormal();
		FVector2D V2 = (P2 - P1).GetSafeNormal();
		float	  Dot = FVector2D::DotProduct(V1, V2);

		if (Dot > 0.999f)
			continue;

		float Angle = FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f));
		float T = TargetRadius * FMath::Tan(Angle / 2.0f);

		float Dist1 = FVector2D::Distance(P0, P1);
		float Dist2 = FVector2D::Distance(P1, P2);

		if (T > Dist1 * 0.5f)
			T = Dist1 * 0.5f;
		if (T > Dist2 * 0.5f)
			T = Dist2 * 0.5f;

		float R = T / FMath::Tan(Angle / 2.0f);
		if (R < 1000.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ArcGen] Радиус слишком мал: %.1f м (сегменты %.0f/%.0f м)"), R / 100.0f, Dist1 / 100.0f, Dist2 / 100.0f);
			continue;
		}

		FVector2D T1 = P1 - V1 * T;
		FVector2D T2 = P1 + V2 * T;

		if (FVector2D::Distance(Final2D.Last(), T1) > 1.0f)
			Final2D.Add(T1);

		FVector2D Bisector = (V2 - V1).GetSafeNormal();
		float	  CenterDist = R / FMath::Cos(Angle / 2.0f);
		FVector2D Center = P1 + Bisector * CenterDist;

		float StartAngle = FMath::Atan2(T1.Y - Center.Y, T1.X - Center.X);
		float EndAngle = FMath::Atan2(T2.Y - Center.Y, T2.X - Center.X);
		float DeltaAngle = EndAngle - StartAngle;

		if (DeltaAngle > PI)
			DeltaAngle -= 2 * PI;
		if (DeltaAngle < -PI)
			DeltaAngle += 2 * PI;

		int32 NumSteps = FMath::Max(5, FMath::CeilToInt(FMath::Abs(DeltaAngle) * R / 200.0f));
		for (int32 s = 1; s <= NumSteps; ++s)
		{
			float a = StartAngle + DeltaAngle * ((float)s / NumSteps);
			Final2D.Add(Center + FVector2D(FMath::Cos(a), FMath::Sin(a)) * R);
		}
	}
	Final2D.Add(Simplified.Last());

	UE_LOG(LogTemp, Log, TEXT("[ArcGen] Итог: сгенерировано %d точек дуг"), Final2D.Num());

	float ZOff = PathRenderer ? PathRenderer->ZOffset_cm : 1237.0f;
	for (const FVector2D& Pt : Final2D)
	{
		float Z = 0.0f;
		if (Terrain)
			Terrain->SampleTerrainZ_cm(Pt, Z);
		OutPoints.Add(FVector(Pt.X, Pt.Y, Z + ZOff));
	}

	return OutPoints;
}

// ─────────────────────────────────────────────────────────────────
//  Сетка
// ─────────────────────────────────────────────────────────────────

FRailGridData ARailOptimaGameMode::BuildGridFromTerrain(const AProceduralTerrain& InTerrain, float InTargetStep_m) const
{
	const FRailHeightMapData& MapData = InTerrain.GetHeightMapData();
	FRailGridData			  Grid;
	if (MapData.Width <= 1 || MapData.Height <= 1 || MapData.Heights.Num() == 0)
		return Grid;

	const int32 TerrainRes = InTerrain.TerrainParameters.MeshResolution;
	const float TerrainStep = InTerrain.TerrainParameters.StepSize_m;
	const float MapSizeMeters = InTerrain.GetMapSizeMeters();

	if (TerrainRes <= 1 || TerrainStep <= 0.0f || MapSizeMeters <= 0.0f)
		return Grid;

	const int32 GridW = FMath::Max(2, FMath::RoundToInt(MapSizeMeters / FMath::Max(InTargetStep_m, 1.0f)));
	const int32 GridH = GridW;
	const float ActualStep_m = MapSizeMeters / static_cast<float>(GridW - 1);

	const float ElevRange = FMath::Max(MapData.MaxElevation_m - MapData.MinElevation_m, 1.0f);

	Grid.Width = GridW;
	Grid.Height = GridH;
	Grid.StepSize_m = ActualStep_m;
	Grid.Nodes.SetNum(GridW * GridH);

	const float ScaleX = static_cast<float>(MapData.Width - 1) / FMath::Max(GridW - 1, 1);
	const float ScaleY = static_cast<float>(MapData.Height - 1) / FMath::Max(GridH - 1, 1);

	for (int32 Y = 0; Y < GridH; ++Y)
	{
		for (int32 X = 0; X < GridW; ++X)
		{
			const int32 SrcX = FMath::Clamp(FMath::RoundToInt(X * ScaleX), 0, MapData.Width - 1);
			const int32 SrcY = FMath::Clamp(FMath::RoundToInt(Y * ScaleY), 0, MapData.Height - 1);

			const float NormH = MapData.Heights[SrcY * MapData.Width + SrcX];
			const float Height_m = MapData.MinElevation_m + NormH * ElevRange;

			FRailGridNode& Node = Grid.Nodes[Grid.IndexOf(X, Y)];
			Node.X = X;
			Node.Y = Y;
			Node.Height_m = Height_m;
			Node.GroundType = ERailGroundType::Loam;
			Node.bIsForbidden = false;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[RailGM] Сетка A*: %dx%d, шаг=%.1f м."), GridW, GridH, ActualStep_m);

	return Grid;
}

FRailGridData ARailOptimaGameMode::GenerateSyntheticGrid(int32 InSize, float InStepSize_m) const
{
	FRailGridData Grid;
	Grid.Width = InSize;
	Grid.Height = InSize;
	Grid.StepSize_m = InStepSize_m;
	Grid.Nodes.SetNum(InSize * InSize);

	FRandomStream Random(1337);
	for (int32 Y = 0; Y < InSize; ++Y)
	{
		for (int32 X = 0; X < InSize; ++X)
		{
			FRailGridNode& Node = Grid.Nodes[Grid.IndexOf(X, Y)];
			Node.X = X;
			Node.Y = Y;
			const float FX = static_cast<float>(X) / InSize;
			const float FY = static_cast<float>(Y) / InSize;
			Node.Height_m = 200.0f + FMath::Sin(FX * 4.0f * PI) * 80.0f + FMath::Cos(FY * 3.0f * PI) * 60.0f + Random.FRandRange(-5.0f, 5.0f);
		}
	}
	return Grid;
}

// ─────────────────────────────────────────────────────────────────
//  World ↔ Node
// ─────────────────────────────────────────────────────────────────

int32 ARailOptimaGameMode::WorldToNodeIndex(const FRailGridData& InGrid, const FVector& InWorldPos) const
{
	if (!PathRenderer || InGrid.Width < 2 || InGrid.Height < 2)
		return INDEX_NONE;

	const FVector Origin = PathRenderer->GridWorldOrigin;
	const float	  Step_cm = InGrid.StepSize_m * 100.0f;

	const int32 X = FMath::Clamp(FMath::RoundToInt((InWorldPos.X - Origin.X) / Step_cm), 0, InGrid.Width - 1);
	const int32 Y = FMath::Clamp(FMath::RoundToInt((InWorldPos.Y - Origin.Y) / Step_cm), 0, InGrid.Height - 1);

	return InGrid.IndexOf(X, Y);
}

// ─────────────────────────────────────────────────────────────────
//  Реакция на маркеры
// ─────────────────────────────────────────────────────────────────

void ARailOptimaGameMode::HandleEndpointMoved(ARailEndpoint* /*Endpoint*/)
{
	if (RecomputeDebounceSec <= 0.0f)
	{
		RecomputePath();
		return;
	}

	GetWorldTimerManager().ClearTimer(RecomputeTimer);
	GetWorldTimerManager().SetTimer(RecomputeTimer, this, &ARailOptimaGameMode::HandleDeferredRecompute, RecomputeDebounceSec, false);
}

void ARailOptimaGameMode::HandleDeferredRecompute()
{
	RecomputePath();
}