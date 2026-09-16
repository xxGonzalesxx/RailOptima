// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#include "RailOptimaGameMode.h"

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

	// Автоматический прогон бенчмарка при старте.
	RunAStarBenchmark(512, 50.0f);
}

// ─────────────────────────────────────────────────────────────────
//  Команды навигации
// ─────────────────────────────────────────────────────────────────

void ARailOptimaGameMode::RunAStarBenchmark(int32 InSize, float InStepSize_m)
{
	if (!PathFinder)
	{
		UE_LOG(LogTemp, Error, TEXT("[RailOptimaGameMode] PathFinder не инициализирован."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[RailOptimaGameMode] Старт бенчмарка: %dx%d, шаг %.1f м."),
		InSize, InSize, InStepSize_m);

	const FRailGridData Grid = GenerateSyntheticGrid(InSize, InStepSize_m);

	const int32 StartIndex = Grid.IndexOf(0, 0);
	const int32 EndIndex = Grid.IndexOf(InSize - 1, InSize - 1);

	const double StartTime = FPlatformTime::Seconds();
	const FRailPathResult Result = PathFinder->FindPath(Grid, StartIndex, EndIndex);
	const double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	if (!Result.bFound)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RailOptimaGameMode] Путь не найден. Время: %.2f мс, расширений: %d."),
			ElapsedMs, Result.ExpandedNodes);
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
		UE_LOG(LogTemp, Log, TEXT("[RailOptimaGameMode] Критерий успеха выполнен: < 1 сек."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[RailOptimaGameMode] Превышен лимит 1 сек: %.2f мс."), ElapsedMs);
	}
}

// ─────────────────────────────────────────────────────────────────
//  Внутренняя логика
// ─────────────────────────────────────────────────────────────────

FRailGridData ARailOptimaGameMode::GenerateSyntheticGrid(int32 InSize, float InStepSize_m) const
{
	FRailGridData Grid;
	Grid.Width = InSize;
	Grid.Height = InSize;
	Grid.StepSize_m = InStepSize_m;
	Grid.Nodes.SetNum(InSize * InSize);

	// Псевдослучайный генератор с фиксированным seed — для воспроизводимости.
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

			// Синусоидальный рельеф + небольшой шум.
			const float FX = static_cast<float>(X) / InSize;
			const float FY = static_cast<float>(Y) / InSize;
			const float Base = FMath::Sin(FX * 4.0f * PI) * 80.0f
				+ FMath::Cos(FY * 3.0f * PI) * 60.0f;
			Node.Height_m = 200.0f + Base + Random.FRandRange(-5.0f, 5.0f);

			// Распределение грунтов: в основном суглинок, местами — остальное.
			const float Roll = Random.FRand();
			if (Roll < 0.50f)
			{
				Node.GroundType = ERailGroundType::Loam;
			}
			else if (Roll < 0.70f)
			{
				Node.GroundType = ERailGroundType::Sand;
			}
			else if (Roll < 0.80f)
			{
				Node.GroundType = ERailGroundType::Gravel;
			}
			else if (Roll < 0.88f)
			{
				Node.GroundType = ERailGroundType::Forest;
			}
			else if (Roll < 0.94f)
			{
				Node.GroundType = ERailGroundType::Rock;
			}
			else
			{
				Node.GroundType = ERailGroundType::Water;
			}

			// Запретная зона — круг в центре карты.
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