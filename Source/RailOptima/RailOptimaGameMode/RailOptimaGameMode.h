// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Algorithms/AStarPathFinder.h"
#include "RailOptimaGameMode.generated.h"

/**
 * Игровой режим RailOptima.
 *
 * Содержит точку входа для тестового прогона A* на синтетической
 * сетке и в дальнейшем — управление основными подсистемами.
 */
UCLASS()
class RAILOPTIMA_API ARailOptimaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ARailOptimaGameMode();

	// ─────────────────────────────────────────────────────────────────
	//  Жизненный цикл
	// ─────────────────────────────────────────────────────────────────

	virtual void BeginPlay() override;

	// ─────────────────────────────────────────────────────────────────
	//  Команды навигации
	// ─────────────────────────────────────────────────────────────────

	/**
	 * Тестовый прогон A* на синтетической сетке.
	 *
	 * Замеряет время поиска пути и выводит результат в лог.
	 *
	 * @param InSize         Сторона квадратной сетки (512 → 512×512).
	 * @param InStepSize_m   Шаг сетки, метры.
	 */
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Test")
	void RunAStarBenchmark(int32 InSize = 512, float InStepSize_m = 50.0f);

private:

	// ─────────────────────────────────────────────────────────────────
	//  Внутренняя логика
	// ─────────────────────────────────────────────────────────────────

	/**
	 * Сгенерировать синтетическую сетку.
	 *
	 * Высоты — синусоидальный рельеф, грунты — по псевдослучайному
	 * распределению. В центре — «город» (запретная зона) для проверки
	 * обхода.
	 *
	 * @param InSize         Сторона сетки.
	 * @param InStepSize_m   Шаг сетки, метры.
	 * @return               Заполненная сетка.
	 */
	FRailGridData GenerateSyntheticGrid(int32 InSize, float InStepSize_m) const;

	// ─────────────────────────────────────────────────────────────────
	//  Состояние
	// ─────────────────────────────────────────────────────────────────

	// Поисковик пути, создаётся в BeginPlay.
	UPROPERTY()
	TObjectPtr<UAStarPathFinder> PathFinder;

	// Калькулятор стоимости.
	UPROPERTY()
	TObjectPtr<UCostCalculator> CostCalculator;
};