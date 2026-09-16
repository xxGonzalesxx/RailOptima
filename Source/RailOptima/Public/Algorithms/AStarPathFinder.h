// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект 
// «Разработка программы проектирования железнодорожных путей: модуль вызуализации», 
// ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Algorithms/CostCalculator.h"
#include "AStarPathFinder.generated.h"

/**
 * Узел сетки поиска пути.
 *
 * Сетка строится поверх ландшафта с шагом 50–100 м.
 * Индекс = Y × Width + X.
 */
USTRUCT(BlueprintType)
struct FRailGridNode
{
	GENERATED_BODY()

	// Координата X в сетке.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	int32 X = 0;

	// Координата Y в сетке.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	int32 Y = 0;

	// Высота над уровнем моря, метры.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	float Height_m = 0.0f;

	// Тип грунта под узлом.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	ERailGroundType GroundType = ERailGroundType::Loam;

	// Запретная зона: строить нельзя.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	bool bIsForbidden = false;
};

/**
 * Сетка узлов для A*.
 *
 * Хранится линейно, доступ по индексу Y × Width + X.
 */
USTRUCT(BlueprintType)
struct FRailGridData
{
	GENERATED_BODY()

	// Ширина сетки в узлах.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	int32 Width = 0;

	// Высота сетки в узлах.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	int32 Height = 0;

	// Шаг сетки, метры.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	float StepSize_m = 50.0f;

	// Узлы сетки.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	TArray<FRailGridNode> Nodes;

	// Индекс узла по координатам.
	FORCEINLINE int32 IndexOf(int32 InX, int32 InY) const { return InY * Width + InX; }

	// Проверка выхода за границы.
	FORCEINLINE bool IsValid(int32 InX, int32 InY) const
	{
		return InX >= 0 && InX < Width && InY >= 0 && InY < Height;
	}
};

/**
 * Результат поиска пути A*.
 */
USTRUCT(BlueprintType)
struct FRailPathResult
{
	GENERATED_BODY()

	// Найден ли путь.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Path")
	bool bFound = false;

	// Последовательность индексов узлов от старта до финиша.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Path")
	TArray<int32> NodeIndices;

	// Суммарная длина пути, метры.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Path")
	double TotalLength_m = 0.0;

	// Суммарная стоимость строительства.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Path")
	double TotalCost = 0.0;

	// Число расширенных узлов (для отладки производительности).
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Path")
	int32 ExpandedNodes = 0;
};

/**
 * Поиск оптимального маршрута алгоритмом A*.
 *
 * Вес ребра — стоимость строительства, а не длина. Используется
 * собственный бинарный heap с поддержкой UpdatePriority: TArray
 * в UE не умеет обновлять приоритет за O(log n).
 */
UCLASS()
class RAILOPTIMA_API UAStarPathFinder : public UObject
{
	GENERATED_BODY()

public:

	// ─────────────────────────────────────────────────────────────────
	//  Команды навигации
	// ─────────────────────────────────────────────────────────────────

	/**
	 * Найти путь между двумя узлами сетки.
	 *
	 * @param InGrid           Сетка поиска.
	 * @param InStartIndex     Индекс стартового узла.
	 * @param InEndIndex       Индекс финишного узла.
	 * @param InMaxExpansions  Лимит расширений (защита от бесконечного поиска).
	 * @return                 Результат поиска.
	 */
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Path")
	FRailPathResult FindPath(
		const FRailGridData& InGrid,
		int32 InStartIndex,
		int32 InEndIndex,
		int32 InMaxExpansions = 500000);

	// ─────────────────────────────────────────────────────────────────
	//  Управление состоянием
	// ─────────────────────────────────────────────────────────────────

	/**
	 * Задать калькулятор стоимости.
	 *
	 * @param InCalculator  Указатель на калькулятор.
	 */
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Path")
	void SetCostCalculator(UCostCalculator* InCalculator);

	// ─────────────────────────────────────────────────────────────────
	//  Геттеры
	// ─────────────────────────────────────────────────────────────────

	// Текущий калькулятор стоимости.
	UFUNCTION(BlueprintPure, Category = "RailOptima|Path")
	UCostCalculator* GetCostCalculator() const { return CostCalculator; }

private:

	// ─────────────────────────────────────────────────────────────────
	//  Внутренняя логика
	// ─────────────────────────────────────────────────────────────────

	// Заполнить массив соседей узла, допустимых для прохода.
	void CollectNeighbors(
		const FRailGridData& InGrid,
		int32 InNodeIndex,
		TArray<int32, TInlineAllocator<8>>& OutNeighbors) const;

	// Восстановить путь по массиву CameFrom.
	void ReconstructPath(
		const FRailGridData& InGrid,
		const TArray<int32>& InCameFrom,
		int32 InStartIndex,
		int32 InEndIndex,
		FRailPathResult& OutResult) const;

	// Оценить стоимость одного ребра между соседними узлами.
	float EvaluateEdge(
		const FRailGridData& InGrid,
		int32 InFromIndex,
		int32 InToIndex,
		const TArray<int32>& InCameFrom) const;

	// Эвристика: нижняя оценка стоимости до финиша.
	float Heuristic(const FRailGridData& InGrid, int32 InFromIndex, int32 InToIndex) const;

	// ─────────────────────────────────────────────────────────────────
	//  Состояние
	// ─────────────────────────────────────────────────────────────────

	// Калькулятор стоимости.
	UPROPERTY()
	TObjectPtr<UCostCalculator> CostCalculator;

	// ─────────────────────────────────────────────────────────────────
	//  Константы
	// ─────────────────────────────────────────────────────────────────

	// 8 направлений обхода: 4 ортогональных + 4 диагональных.
	static constexpr int32 NumDirections = 8;

	// Смещения по X для каждого направления.
	static const int32 DirectionX[NumDirections];

	// Смещения по Y для каждого направления.
	static const int32 DirectionY[NumDirections];
};