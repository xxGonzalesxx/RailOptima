// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Модуль проверки пути на соответствие СП 119.13330.

#pragma once

#include "Algorithms/AStarPathFinder.h"
#include "Algorithms/RailCategories.h"
#include "CoreMinimal.h"
#include "RailPathValidator.generated.h"


USTRUCT(BlueprintType)
struct FRailPathViolation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Validation")
	FString Message;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Validation")
	int32 NodeIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Validation")
	bool bIsWarning = false;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Validation")
	bool bRequiresTEO = false;
};

USTRUCT(BlueprintType)
struct FRailValidationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Validation")
	bool bIsValid = true;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Validation")
	TArray<FRailPathViolation> Violations;
};

UCLASS()
class RAILOPTIMA_API URailPathValidator : public UObject
{
	GENERATED_BODY()

public:
	// Инженерная проверка готовой трассы в мировых координатах (СП 119.13330)
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Validation")
	FRailValidationResult ValidateWorldTrack(
		const TArray<FVector>&	  InWorldPoints_cm,
		const FRailCategoryNorms& InNorms,
		float					  InWorldZScale = 1.0f) const;

	// Полная проверка пути по нормативам на сетке A*
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Validation")
	FRailValidationResult ValidatePath(
		const FRailGridData&	  InGrid,
		const FRailPathResult&	  InPath,
		const FRailCategoryNorms& InNorms) const;

	// Пост-обработка: сглаживает острые углы (возвращает новые точки для отрисовки)
	// ВНИМАНИЕ: Это упрощенный алгоритм, вписывающий дуги в вершины.
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Validation")
	void SmoothPathToNorms(
		const FRailGridData&	  InGrid,
		const FRailPathResult&	  InPath,
		const FRailCategoryNorms& InNorms,
		TArray<FVector>&		  OutSmoothWorldPoints) const;
};