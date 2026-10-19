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
	{
		UE_LOG(LogTemp, Warning, TEXT("[RailGM] ApplySettings: CostCalculator == nullptr."));
		return;
	}

	const bool bStepChanged =
		!FMath::IsNearlyEqual(RuntimeSettings.TargetStepSize_m, InStepSize_m, 1.0f);

	RuntimeSettings.SlopePenalty = FMath::Clamp(InSlopePenalty, 0.0f, 100.0f);
	RuntimeSettings.TurnPenalty = FMath::Clamp(InTurnPenalty, 0.0f, 100.0f);
	RuntimeSettings.TargetStepSize_m = FMath::Clamp(InStepSize_m, 10.0f, 1000.0f);
	RuntimeSettings.bColorPathByCost = bInColorByCost;

	CostCalculator->Parameters.SlopePenalty = RuntimeSettings.SlopePenalty;
	CostCalculator->Parameters.TurnPenalty = RuntimeSettings.TurnPenalty;

	if (bStepChanged)
	{
		bGridCached = false;
		CachedGrid = FRailGridData();
		UE_LOG(LogTemp, Log, TEXT("[RailGM] ApplySettings: шаг сетки изменён, кэш сброшен."));
	}

	UE_LOG(LogTemp, Log,
		TEXT("[RailGM] ApplySettings: Slope=%.3f, Turn=%.3f, Step=%.1f, ColorByCost=%s."),
		RuntimeSettings.SlopePenalty, RuntimeSettings.TurnPenalty,
		RuntimeSettings.TargetStepSize_m,
		RuntimeSettings.bColorPathByCost ? TEXT("true") : TEXT("false"));
}

void ARailOptimaGameMode::ApplyViewScaleFromCamera(ARailSpectatorPawn* InPawn)
{
	if (!InPawn)
	{
		return;
	}

	float TargetScale = 1.0f;

	if (InPawn->GetCameraMode() == ERailCameraMode::TopDown)
	{
		const float CurrentHeight = InPawn->GetTopDownHeight();

		if (!bViewScaleReferenceCaptured && CurrentHeight > 0.0f)
		{
			ReferenceTopDownHeight_cm = CurrentHeight;
			bViewScaleReferenceCaptured = true;

			UE_LOG(LogTemp, Log,
				TEXT("[RailGM] Захвачена референсная высота TopDown: %.0f см."),
				ReferenceTopDownHeight_cm);
		}

		if (ReferenceTopDownHeight_cm > 0.0f)
		{
			TargetScale = CurrentHeight / ReferenceTopDownHeight_cm;
		}
	}

	TargetScale = FMath::Clamp(TargetScale, MinViewScale, MaxViewScale);

	if (FMath::IsNearlyEqual(TargetScale, CurrentViewScale, 0.02f))
	{
		return;
	}

	CurrentViewScale = TargetScale;

	UE_LOG(LogTemp, Log,
		TEXT("[RailGM] ViewScale = %.2f (camera mode=%s)."),
		CurrentViewScale,
		(InPawn->GetCameraMode() == ERailCameraMode::FreeFly) ? TEXT("FreeFly") : TEXT("TopDown"));

	if (StartEndpoint)
		StartEndpoint->SetViewScale(CurrentViewScale);
	if (EndEndpoint)
		EndEndpoint->SetViewScale(CurrentViewScale);

	if (PathRenderer)
	{
		PathRenderer->SetViewScale(CurrentViewScale);

		if (bGridCached && CachedGrid.Width > 1)
		{
			RecomputePath();
		}
	}
}

// ─────────────────────────────────────────────────────────────────
//  Настройка подсистем
// ─────────────────────────────────────────────────────────────────

void ARailOptimaGameMode::SetupSubsystems()
{
	PathFinder = NewObject<UAStarPathFinder>(this);
	CostCalculator = NewObject<UCostCalculator>(this);
	PathFinder->SetCostCalculator(CostCalculator);

	CostCalculator->Parameters.SlopePenalty = RuntimeSettings.SlopePenalty;
	CostCalculator->Parameters.TurnPenalty = RuntimeSettings.TurnPenalty;

	UE_LOG(LogTemp, Log,
		TEXT("[RailGM] Подсистемы: Slope=%.3f, Turn=%.3f, Step=%.1f, ColorByCost=%s."),
		RuntimeSettings.SlopePenalty, RuntimeSettings.TurnPenalty,
		RuntimeSettings.TargetStepSize_m,
		RuntimeSettings.bColorPathByCost ? TEXT("true") : TEXT("false"));
}

void ARailOptimaGameMode::FindTerrain()
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProceduralTerrain::StaticClass(), Found);

	if (Found.Num() > 0)
	{
		Terrain = Cast<AProceduralTerrain>(Found[0]);
		const FRailHeightMapData& Map = Terrain->GetHeightMapData();

		UE_LOG(LogTemp, Log, TEXT("[RailGM] Рельеф: '%s', %dx%d px, pixel=%.1f м."),
			*Terrain->GetName(), Map.Width, Map.Height, Map.PixelSize_m);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[RailGM] Рельеф не найден. Синтетическая сетка."));
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
		UE_LOG(LogTemp, Log, TEXT("[RailGM] PathRenderer: '%s'."), *PathRenderer->GetName());
	}
	else
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		PathRenderer = GetWorld()->SpawnActor<APathRenderer>(
			APathRenderer::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
		UE_LOG(LogTemp, Warning, TEXT("[RailGM] PathRenderer заспавнен программно."));
	}

	if (!PathRenderer)
		return;

	PathRenderer->SetActorLocation(FVector::ZeroVector);
	PathRenderer->SetActorRotation(FRotator::ZeroRotator);

	if (Terrain)
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

	FVector DefaultStartHit = FVector(0, 0, 0);
	FVector DefaultEndHit = FVector(0, 0, 0);

	if (Terrain)
	{
		const FRailHeightMapData& Map = Terrain->GetHeightMapData();
		const float				  SideX_cm = Map.Width * static_cast<float>(Map.PixelSize_m) * 100.0f;
		const float				  SideY_cm = Map.Height * static_cast<float>(Map.PixelSize_m) * 100.0f;
		const FVector			  TLoc = Terrain->GetActorLocation();
		const float				  M = AutoSpawnMargin_m * 100.0f;
		DefaultStartHit = FVector(TLoc.X + M, TLoc.Y + M, 0.0f);
		DefaultEndHit = FVector(TLoc.X + SideX_cm - M, TLoc.Y + SideY_cm - M, 0.0f);
	}

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (!StartEndpoint)
	{
		StartEndpoint = GetWorld()->SpawnActor<ARailEndpoint>(
			ARailEndpoint::StaticClass(), FVector(0, 0, 0), FRotator::ZeroRotator, SP);
		if (StartEndpoint)
		{
			StartEndpoint->bIsStart = true;
			StartEndpoint->EndpointColor = FLinearColor(0.1f, 0.9f, 0.1f, 1.0f);
			StartEndpoint->SetEndpointFromTerrainHit(DefaultStartHit, false);
			UE_LOG(LogTemp, Log, TEXT("[RailGM] StartEndpoint создан программно."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[RailGM] StartEndpoint с уровня: '%s'."), *StartEndpoint->GetName());
	}

	if (!EndEndpoint)
	{
		EndEndpoint = GetWorld()->SpawnActor<ARailEndpoint>(
			ARailEndpoint::StaticClass(), FVector(0, 0, 0), FRotator::ZeroRotator, SP);
		if (EndEndpoint)
		{
			EndEndpoint->bIsStart = false;
			EndEndpoint->EndpointColor = FLinearColor(0.9f, 0.1f, 0.1f, 1.0f);
			EndEndpoint->SetEndpointFromTerrainHit(DefaultEndHit, false);
			UE_LOG(LogTemp, Log, TEXT("[RailGM] EndEndpoint создан программно."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[RailGM] EndEndpoint с уровня: '%s'."), *EndEndpoint->GetName());
	}

	if (StartEndpoint)
		StartEndpoint->OnEndpointMoved.AddDynamic(this, &ARailOptimaGameMode::HandleEndpointMoved);
	if (EndEndpoint)
		EndEndpoint->OnEndpointMoved.AddDynamic(this, &ARailOptimaGameMode::HandleEndpointMoved);

	UE_LOG(LogTemp, Log, TEXT("[RailGM] Маркеры готовы."));
}

// ─────────────────────────────────────────────────────────────────
//  Пересчёт
// ─────────────────────────────────────────────────────────────────

void ARailOptimaGameMode::RecomputePath()
{
	if (!PathFinder || !PathRenderer || !StartEndpoint || !EndEndpoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RailGM] RecomputePath: не готова связка."));
		return;
	}

	if (!bGridCached)
	{
		if (Terrain && Terrain->GetHeightMapData().Heights.Num() > 0)
		{
			CachedGrid = BuildGridFromTerrain(*Terrain, RuntimeSettings.TargetStepSize_m);
		}
		else
		{
			CachedGrid = GenerateSyntheticGrid(336, RuntimeSettings.TargetStepSize_m);
		}
		bGridCached = CachedGrid.Width > 1 && CachedGrid.Height > 1;
	}

	if (!bGridCached)
	{
		UE_LOG(LogTemp, Error, TEXT("[RailGM] Сетка не построена."));
		return;
	}

	const int32 StartIdx = WorldToNodeIndex(CachedGrid, StartEndpoint->GetActorLocation());
	const int32 EndIdx = WorldToNodeIndex(CachedGrid, EndEndpoint->GetActorLocation());

	if (StartIdx == INDEX_NONE || EndIdx == INDEX_NONE || StartIdx == EndIdx)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RailGM] Некорректные узлы."));
		PathRenderer->ClearPath();
		OnPathRecomputed.Broadcast(0.0, 0.0, 0.0, 0);
		return;
	}

	const double		  T0 = FPlatformTime::Seconds();
	const FRailPathResult Result = PathFinder->FindPath(CachedGrid, StartIdx, EndIdx);
	const double		  Ms = (FPlatformTime::Seconds() - T0) * 1000.0;

	if (!Result.bFound)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RailGM] Путь не найден. %.1f мс, расширений=%d."),
			Ms, Result.ExpandedNodes);
		PathRenderer->ClearPath();
		OnPathRecomputed.Broadcast(0.0, 0.0, Ms, 0);
		return;
	}

	UE_LOG(LogTemp, Log,
		TEXT("[RailGM] Путь: длина=%.0f м, стоимость=%.1f ед., узлов=%d, рёбер=%d, время=%.1f мс."),
		Result.TotalLength_m, Result.TotalCost,
		Result.NodeIndices.Num(), Result.EdgeCosts.Num(), Ms);

	if (!RuntimeSettings.bColorPathByCost)
	{
		PathRenderer->RenderPath(CachedGrid, Result);
	}
	else
	{
		const int32	  NumNodes = Result.NodeIndices.Num();
		TArray<float> NodeUnitCosts;
		NodeUnitCosts.Reserve(NumNodes);

		float MinU = TNumericLimits<float>::Max();
		float MaxU = TNumericLimits<float>::Lowest();

		for (int32 i = 0; i < NumNodes; ++i)
		{
			float Unit = 0.0f;
			if (i < Result.EdgeCosts.Num() && Result.EdgeLengths[i] > KINDA_SMALL_NUMBER)
			{
				Unit = Result.EdgeCosts[i] / Result.EdgeLengths[i];
			}
			else if (Result.EdgeCosts.Num() > 0 && Result.EdgeLengths.Last() > KINDA_SMALL_NUMBER)
			{
				Unit = Result.EdgeCosts.Last() / Result.EdgeLengths.Last();
			}
			NodeUnitCosts.Add(Unit);
			MinU = FMath::Min(MinU, Unit);
			MaxU = FMath::Max(MaxU, Unit);
		}

		if (FMath::IsNearlyEqual(MinU, MaxU))
			MaxU = MinU + 1.0f;

		UE_LOG(LogTemp, Log,
			TEXT("[RailGM] Раскраска по стоимости: unit-cost min=%.3f, max=%.3f."),
			MinU, MaxU);

		PathRenderer->RenderColoredPath(CachedGrid, Result, NodeUnitCosts, MinU, MaxU);
	}

	OnPathRecomputed.Broadcast(Result.TotalLength_m, Result.TotalCost, Ms, Result.NodeIndices.Num());
}

// ─────────────────────────────────────────────────────────────────
//  Сетка
// ─────────────────────────────────────────────────────────────────

FRailGridData ARailOptimaGameMode::BuildGridFromTerrain(
	const AProceduralTerrain& InTerrain, float InTargetStep_m) const
{
	const FRailHeightMapData& MapData = InTerrain.GetHeightMapData();
	FRailGridData			  Grid;
	if (MapData.Width <= 1 || MapData.Height <= 1 || MapData.Heights.Num() == 0)
		return Grid;

	// Работаем в системе координат ЛАНДШАФТА, а не PNG.
	// Размер карты в метрах = MeshResolution × StepSize_m.
	const int32 TerrainRes = InTerrain.TerrainParameters.MeshResolution;
	const float TerrainStep = InTerrain.TerrainParameters.StepSize_m;
	const float MapSizeMeters = InTerrain.GetMapSizeMeters();

	if (TerrainRes <= 1 || TerrainStep <= 0.0f || MapSizeMeters <= 0.0f)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RailGM] Некорректные параметры ландшафта: Resolution=%d, Step=%.2f."),
			TerrainRes, TerrainStep);
		return Grid;
	}

	// Сколько узлов сетки A* укладывается в размер ландшафта.
	// Шаг сетки A* округляем к размеру ландшафта.
	const int32 GridW = FMath::Max(2, FMath::RoundToInt(MapSizeMeters / FMath::Max(InTargetStep_m, 1.0f)));
	const int32 GridH = GridW;
	const float ActualStep_m = MapSizeMeters / static_cast<float>(GridW - 1);

	const float ElevRange = FMath::Max(MapData.MaxElevation_m - MapData.MinElevation_m, 1.0f);

	Grid.Width = GridW;
	Grid.Height = GridH;
	Grid.StepSize_m = ActualStep_m;
	Grid.Nodes.SetNum(GridW * GridH);

	// Соотношение между сеткой A* и PNG (для сэмплирования высот).
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

	UE_LOG(LogTemp, Log,
		TEXT("[RailGM] Сетка A*: %dx%d, шаг=%.1f м, размер=%.1f км (совпадает с ландшафтом)."),
		GridW, GridH, ActualStep_m, MapSizeMeters / 1000.0f);

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

	UE_LOG(LogTemp, Warning, TEXT("[RailGM] Синтетическая сетка %dx%d."), InSize, InSize);
	return Grid;
}

// ─────────────────────────────────────────────────────────────────
//  World ↔ Node
// ─────────────────────────────────────────────────────────────────

int32 ARailOptimaGameMode::WorldToNodeIndex(
	const FRailGridData& InGrid, const FVector& InWorldPos) const
{
	if (!PathRenderer || InGrid.Width < 2 || InGrid.Height < 2)
		return INDEX_NONE;

	const FVector Origin = PathRenderer->GridWorldOrigin;
	const float	  Step_cm = InGrid.StepSize_m * 100.0f;

	const int32 X = FMath::Clamp(
		FMath::RoundToInt((InWorldPos.X - Origin.X) / Step_cm), 0, InGrid.Width - 1);
	const int32 Y = FMath::Clamp(
		FMath::RoundToInt((InWorldPos.Y - Origin.Y) / Step_cm), 0, InGrid.Height - 1);

	return InGrid.IndexOf(X, Y);
}

FVector ARailOptimaGameMode::NodeIndexToWorldXY(
	const FRailGridData& InGrid, int32 InNodeIndex) const
{
	if (!InGrid.Nodes.IsValidIndex(InNodeIndex) || !PathRenderer)
		return FVector::ZeroVector;

	const FRailGridNode& Node = InGrid.Nodes[InNodeIndex];
	const FVector		 Origin = PathRenderer->GridWorldOrigin;
	const float			 Step_cm = InGrid.StepSize_m * 100.0f;

	return FVector(Origin.X + Node.X * Step_cm, Origin.Y + Node.Y * Step_cm, 0.0f);
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
	GetWorldTimerManager().SetTimer(RecomputeTimer, this,
		&ARailOptimaGameMode::HandleDeferredRecompute, RecomputeDebounceSec, false);
}

void ARailOptimaGameMode::HandleDeferredRecompute()
{
	RecomputePath();
}