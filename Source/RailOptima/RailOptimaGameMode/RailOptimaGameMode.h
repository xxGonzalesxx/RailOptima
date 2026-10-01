// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Algorithms/AStarPathFinder.h"
#include "RailOptimaGameMode.generated.h"

class APathRenderer;
class AProceduralTerrain;

UCLASS()
class RAILOPTIMA_API ARailOptimaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ARailOptimaGameMode();

	virtual void BeginPlay() override;

	/**
	 * Прогон A* на сетке, построенной из рельефа.
	 * Если рельефа на уровне нет — fallback на синтетику.
	 *
	 * @param InTargetStepSize_m  Желаемый шаг сетки в метрах.
	 *                            Реальный шаг = PixelSize_m × round(желаемый / PixelSize_m).
	 */
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Test")
	void RunAStarBenchmark(float InTargetStepSize_m = 100.0f);

private:

	// ─────────────────────────────────────────────────────────────────
	//  Построение сетки
	// ─────────────────────────────────────────────────────────────────

	/**
	 * Построить сетку из рельефа.
	 *
	 * PNG рельефа 1009×1009 пикселей по 30 м = ~30 км по стороне.
	 * Сетка берёт каждый N-й пиксель: N = round(InTargetStepSize_m / PixelSize_m).
	 * Высоты — метры над уровнем моря (без нормализации).
	 */
	FRailGridData BuildGridFromTerrain(
		const AProceduralTerrain& InTerrain,
		float InTargetStepSize_m) const;

	// Синтетическая сетка — только если на уровне нет рельефа.
	FRailGridData GenerateSyntheticGrid(int32 InSize, float InStepSize_m) const;

	// ─────────────────────────────────────────────────────────────────
	//  Подсистемы
	// ─────────────────────────────────────────────────────────────────

	void SetupPathRenderer();

	UPROPERTY()
	TObjectPtr<UAStarPathFinder> PathFinder;

	UPROPERTY()
	TObjectPtr<UCostCalculator> CostCalculator;

	UPROPERTY()
	TObjectPtr<APathRenderer> PathRenderer;

	UPROPERTY()
	TObjectPtr<AProceduralTerrain> Terrain;
};