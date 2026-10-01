// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#include "RailOptimaGameMode.h"
#include "Algorithms/AStarPathFinder.h"
#include "Algorithms/CostCalculator.h"
#include "Core/ProceduralTerrain.h"
#include "Visualization/PathRenderer.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// ─────────────────────────────────────────────────────────────────
//  Жизненный цикл
// ─────────────────────────────────────────────────────────────────

ARailOptimaGameMode::ARailOptimaGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARailOptimaGameMode::BeginPlay()
{
	Super::BeginPlay();

	PathFinder = NewObject<UAStarPathFinder>(this);
	CostCalculator = NewObject<UCostCalculator>(this);
	PathFinder->SetCostCalculator(CostCalculator);

	// ─────────────────────────────────────────────────────────────────
	//  Рельеф
	// ─────────────────────────────────────────────────────────────────

	TArray<AActor*> FoundTerrains;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProceduralTerrain::StaticClass(), FoundTerrains);

	if (FoundTerrains.Num() > 0)
	{
		Terrain = Cast<AProceduralTerrain>(FoundTerrains[0]);
		UE_LOG(LogTemp, Log, TEXT("[RailOptimaGameMode] Найден рельеф на уровне."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[RailOptimaGameMode] Рельеф не найден — синтетическая сетка."));
	}

	// ─────────────────────────────────────────────────────────────────
	//  Рендерер
	// ─────────────────────────────────────────────────────────────────

	SetupPathRenderer();

	// ─────────────────────────────────────────────────────────────────
	//  Бенчмарк и отрисовка
	// ─────────────────────────────────────────────────────────────────

	RunAStarBenchmark(100.0f);
}

// ─────────────────────────────────────────────────────────────────
//  Команды навигации
// ─────────────────────────────────────────────────────────────────

void ARailOptimaGameMode::RunAStarBenchmark(float InTargetStepSize_m)
{
	if (!PathFinder)
	{
		UE_LOG(LogTemp, Error, TEXT("[RailOptimaGameMode] PathFinder не инициализирован."));
		return;
	}

	// 1. Сетка — из рельефа, если он есть.
	FRailGridData Grid;
	if (Terrain && Terrain->GetHeightMapData().Heights.Num() > 0)
	{
		Grid = BuildGridFromTerrain(*Terrain, InTargetStepSize_m);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[RailOptimaGameMode] Fallback: синтетическая сетка."));
		Grid = GenerateSyntheticGrid(512, InTargetStepSize_m);
	}

	if (Grid.Width < 2 || Grid.Height < 2)
	{
		UE_LOG(LogTemp, Error, TEXT("[RailOptimaGameMode] Сетка пуста."));
		return;
	}

	// 2. Старт/финиш — противоположные углы.
	const int32 StartIndex = Grid.IndexOf(0, 0);
	const int32 EndIndex = Grid.IndexOf(Grid.Width - 1, Grid.Height - 1);

	UE_LOG(LogTemp, Log, TEXT("[RailOptimaGameMode] Старт A*: %dx%d, шаг %.1f м."),
		Grid.Width, Grid.Height, Grid.StepSize_m);

	// 3. Поиск.
	const double StartTime = FPlatformTime::Seconds();
	const FRailPathResult Result = PathFinder->FindPath(Grid, StartIndex, EndIndex);
	const double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	if (!Result.bFound)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RailOptimaGameMode] Путь не найден. Время: %.2f мс, расширений: %d."),
			ElapsedMs, Result.ExpandedNodes);

		if (PathRenderer)
		{
			PathRenderer->ClearPath();
		}
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[RailOptimaGameMode] Путь найден:"));
	UE_LOG(LogTemp, Log, TEXT("  Длина: %.1f м."), Result.TotalLength_m);
	UE_LOG(LogTemp, Log, TEXT("  Стоимость: %.2f ед."), Result.TotalCost);
	UE_LOG(LogTemp, Log, TEXT("  Узлов в пути: %d."), Result.NodeIndices.Num());
	UE_LOG(LogTemp, Log, TEXT("  Расширений: %d."), Result.ExpandedNodes);
	UE_LOG(LogTemp, Log, TEXT("  Время: %.2f мс."), ElapsedMs);

	if (ElapsedMs < 1000.0)
	{
		UE_LOG(LogTemp, Log, TEXT("[RailOptimaGameMode] Критерий успеха: < 1 сек."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[RailOptimaGameMode] Превышен лимит 1 сек: %.2f мс."), ElapsedMs);
	}

	// 4. Отрисовка.
	if (PathRenderer)
	{
		PathRenderer->RenderPath(Grid, Result);
	}
}

// ─────────────────────────────────────────────────────────────────
//  Построение сетки из рельефа
// ─────────────────────────────────────────────────────────────────

FRailGridData ARailOptimaGameMode::BuildGridFromTerrain(
	const AProceduralTerrain& InTerrain,
	float InTargetStepSize_m) const
{
	const FRailHeightMapData& MapData = InTerrain.GetHeightMapData();
	FRailGridData Grid;

	if (MapData.Width <= 1 || MapData.Height <= 1 || MapData.Heights.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[Grid] Данные рельефа пусты."));
		return Grid;
	}

	// Шаг сетки округляем к кратному размеру пикселя карты.
	const float PixelSize_m = FMath::Max(static_cast<float>(MapData.PixelSize_m), 1.0f);
	const int32 Subsample = FMath::Max(1,
		FMath::RoundToInt(InTargetStepSize_m / PixelSize_m));
	const float ActualStep_m = PixelSize_m * static_cast<float>(Subsample);

	const int32 GridW = FMath::Max(2, MapData.Width / Subsample);
	const int32 GridH = FMath::Max(2, MapData.Height / Subsample);

	const float ElevRange = FMath::Max(MapData.MaxElevation_m - MapData.MinElevation_m, 1.0f);

	Grid.Width = GridW;
	Grid.Height = GridH;
	Grid.StepSize_m = ActualStep_m;
	Grid.Nodes.SetNum(GridW * GridH);

	for (int32 Y = 0; Y < GridH; ++Y)
	{
		for (int32 X = 0; X < GridW; ++X)
		{
			// Пиксель PNG, соответствующий узлу.
			const int32 SrcX = FMath::Clamp(X * Subsample, 0, MapData.Width - 1);
			const int32 SrcY = FMath::Clamp(Y * Subsample, 0, MapData.Height - 1);

			const float NormH = MapData.Heights[SrcY * MapData.Width + SrcX];
			const float Height_m = MapData.MinElevation_m + NormH * ElevRange;

			FRailGridNode& Node = Grid.Nodes[Grid.IndexOf(X, Y)];
			Node.X = X;
			Node.Y = Y;
			Node.Height_m = Height_m;
			Node.GroundType = ERailGroundType::Loam;  // карты грунтов пока нет
			Node.bIsForbidden = false;
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("[Grid] Построена сетка %dx%d из рельефа %dx%d "
			"(субдискретизация ×%d, шаг %.1f м, высоты %.0f..%.0f м)."),
		GridW, GridH, MapData.Width, MapData.Height,
		Subsample, ActualStep_m,
		MapData.MinElevation_m, MapData.MaxElevation_m);

	return Grid;
}

// ─────────────────────────────────────────────────────────────────
//  Вспомогательное
// ─────────────────────────────────────────────────────────────────

void ARailOptimaGameMode::SetupPathRenderer()
{
	// 1. Ищем на уровне.
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APathRenderer::StaticClass(), Found);

	if (Found.Num() > 0)
	{
		PathRenderer = Cast<APathRenderer>(Found[0]);
		UE_LOG(LogTemp, Log, TEXT("[RailOptimaGameMode] Использую PathRenderer с уровня."));
	}
	else
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		PathRenderer = GetWorld()->SpawnActor<APathRenderer>(
			APathRenderer::StaticClass(),
			FVector::ZeroVector, FRotator::ZeroRotator, Params);

		UE_LOG(LogTemp, Warning,
			TEXT("[RailOptimaGameMode] PathRenderer заспавнен программно. ")
			TEXT("Лучше разместить BP_PathRenderer на карте."));
	}

	if (!PathRenderer)
	{
		return;
	}

	// 2. Актёр рендерера ставим строго в начало координат.
	//    Вершины ленты считаются в мировых координатах (см. GridNodeToWorld),
	//    поэтому трансформ актёра НЕ должен их смещать.
	PathRenderer->SetActorLocation(FVector::ZeroVector);
	PathRenderer->SetActorRotation(FRotator::ZeroRotator);

	// 3. Синхронизация с рельефом.
	if (Terrain)
	{
		PathRenderer->WorldZScale = Terrain->TerrainParameters.ZScale;

		// Смещение по XY: если рельеф размещён не в (0,0), узлы сетки
		// (0,0) находятся не в мировом нуле. По Z смещение НЕ переносим —
		// высота уже заложена в Height_m узла.
		const FVector Loc = Terrain->GetActorLocation();
		PathRenderer->GridWorldOrigin = FVector(Loc.X, Loc.Y, 0.0f);
	}
}

FRailGridData ARailOptimaGameMode::GenerateSyntheticGrid(int32 InSize, float InStepSize_m) const
{
	FRailGridData Grid;
	Grid.Width = InSize;
	Grid.Height = InSize;
	Grid.StepSize_m = InStepSize_m;
	Grid.Nodes.SetNum(InSize * InSize);

	FRandomStream Random(1337);
	const float CenterX = InSize * 0.5f;
	const float CenterY = InSize * 0.5f;

	for (int32 Y = 0; Y < InSize; ++Y)
	{
		for (int32 X = 0; X < InSize; ++X)
		{
			FRailGridNode& Node = Grid.Nodes[Grid.IndexOf(X, Y)];
			Node.X = X;
			Node.Y = Y;

			const float FX = static_cast<float>(X) / InSize;
			const float FY = static_cast<float>(Y) / InSize;
			const float Base = FMath::Sin(FX * 4.0f * PI) * 80.0f
				+ FMath::Cos(FY * 3.0f * PI) * 60.0f;
			Node.Height_m = 200.0f + Base + Random.FRandRange(-5.0f, 5.0f);

			const float Roll = Random.FRand();
			if (Roll < 0.50f)      Node.GroundType = ERailGroundType::Loam;
			else if (Roll < 0.70f) Node.GroundType = ERailGroundType::Sand;
			else if (Roll < 0.80f) Node.GroundType = ERailGroundType::Gravel;
			else if (Roll < 0.88f) Node.GroundType = ERailGroundType::Forest;
			else if (Roll < 0.94f) Node.GroundType = ERailGroundType::Rock;
			else                   Node.GroundType = ERailGroundType::Water;

			const float DX = static_cast<float>(X) - CenterX;
			const float DY = static_cast<float>(Y) - CenterY;
			const float Radius = InSize * 0.1f;
			if (DX * DX + DY * DY < Radius * Radius)
			{
				Node.bIsForbidden = true;
				Node.GroundType = ERailGroundType::City;
			}
		}
	}

	return Grid;
}