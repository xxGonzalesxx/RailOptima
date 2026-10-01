// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Algorithms/CostCalculator.h"
#include "AStarPathFinder.generated.h"

USTRUCT(BlueprintType)
struct FRailGridNode
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	int32 X = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	int32 Y = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	float Height_m = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	ERailGroundType GroundType = ERailGroundType::Loam;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	bool bIsForbidden = false;
};

USTRUCT(BlueprintType)
struct FRailGridData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	int32 Width = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	int32 Height = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	float StepSize_m = 50.0f;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Grid")
	TArray<FRailGridNode> Nodes;

	FORCEINLINE int32 IndexOf(int32 InX, int32 InY) const { return InY * Width + InX; }

	FORCEINLINE bool IsValid(int32 InX, int32 InY) const
	{
		return InX >= 0 && InX < Width && InY >= 0 && InY < Height;
	}
};

/**
 * Результат поиска пути A*.
 *
 * Помимо узлов и итогов, хранит по каждому ребру его стоимость
 * и длину — это нужно для раскраски и отчёта.
 */
USTRUCT(BlueprintType)
struct FRailPathResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Path")
	bool bFound = false;

	// Последовательность индексов узлов от старта до финиша.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Path")
	TArray<int32> NodeIndices;

	// Стоимость каждого ребра, длина = NodeIndices.Num() - 1.
	// Ребро i соединяет узлы NodeIndices[i] и NodeIndices[i+1].
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Path")
	TArray<float> EdgeCosts;

	// Длина каждого ребра в метрах, та же длина массива.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Path")
	TArray<float> EdgeLengths;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Path")
	double TotalLength_m = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Path")
	double TotalCost = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Path")
	int32 ExpandedNodes = 0;
};

UCLASS()
class RAILOPTIMA_API UAStarPathFinder : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Path")
	FRailPathResult FindPath(
		const FRailGridData& InGrid,
		int32 InStartIndex,
		int32 InEndIndex,
		int32 InMaxExpansions = 500000);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Path")
	void SetCostCalculator(UCostCalculator* InCalculator);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Path")
	UCostCalculator* GetCostCalculator() const { return CostCalculator; }

private:

	void CollectNeighbors(
		const FRailGridData& InGrid,
		int32 InNodeIndex,
		TArray<int32, TInlineAllocator<8>>& OutNeighbors) const;

	void ReconstructPath(
		const FRailGridData& InGrid,
		const TArray<int32>& InCameFrom,
		int32 InStartIndex,
		int32 InEndIndex,
		FRailPathResult& OutResult) const;

	float EvaluateEdge(
		const FRailGridData& InGrid,
		int32 InFromIndex,
		int32 InToIndex,
		const TArray<int32>& InCameFrom) const;

	float Heuristic(const FRailGridData& InGrid, int32 InFromIndex, int32 InToIndex) const;

	UPROPERTY()
	TObjectPtr<UCostCalculator> CostCalculator;

	static constexpr int32 NumDirections = 8;
	static const int32 DirectionX[NumDirections];
	static const int32 DirectionY[NumDirections];
};