// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект: Модуль критериев проектирования ЖД путей, ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "RailCategories.generated.h"

// Категории железнодорожных линий согласно нормативным документам
UENUM(BlueprintType)
enum class ERailRoadCategory : uint8
{
	HighSpeed		UMETA(DisplayName = "Скоростные"),
	SuperHeavyHaul	UMETA(DisplayName = "Особогрузонапряженные"),
	Category1		UMETA(DisplayName = "I Категория"),
	Category2		UMETA(DisplayName = "II Категория"),
	Category3		UMETA(DisplayName = "III Категория"),
	Category4Lines	UMETA(DisplayName = "IV (Железнодорожные линии)"),
	Category4Access UMETA(DisplayName = "IV (Подъездные пути)")
};

// Структура нормативных ограничений для выбранной категории
USTRUCT(BlueprintType)
struct FRailCategoryNorms
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	ERailRoadCategory Category = ERailRoadCategory::Category1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	FString CategoryName;

	// Максимальный уклон в промилле (‰), по умолчанию для новых участков 15‰
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	float MaxSlopePermille = 15.0f;

	// Рекомендуемые радиусы кривых в плане (м)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	TArray<float> RecommendedRadii;

	// Допускаемые радиусы в трудных / особо трудных условиях (м)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	float MinRadiusDifficult = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	float MinRadiusVeryDifficult = 500.0f;

	// Минимальный радиус вертикальной кривой (Rv), м (Таблица 5.2 СП 119.13330)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	float MinVerticalRadius = 15000.0f;

	// Порог алгебраической разности уклонов для обязательного введения вертикальной кривой (‰)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	float MinSlopeDeltaForVerticalCurve = 2.0f;

	static FRailCategoryNorms GetNorms(ERailRoadCategory InCategory)
	{
		FRailCategoryNorms Norms;
		Norms.Category = InCategory;

		switch (InCategory)
		{
			case ERailRoadCategory::HighSpeed:
				Norms.CategoryName = TEXT("Скоростные");
				Norms.RecommendedRadii = { 4000.0f, 3000.0f };
				Norms.MinRadiusDifficult = 2500.0f;
				Norms.MinRadiusVeryDifficult = 1200.0f;
				Norms.MinVerticalRadius = 20000.0f;
				Norms.MinSlopeDeltaForVerticalCurve = 2.0f;
				break;

			case ERailRoadCategory::SuperHeavyHaul:
				Norms.CategoryName = TEXT("Особогрузонапряженные");
				Norms.RecommendedRadii = { 4000.0f, 3000.0f, 2500.0f, 2000.0f };
				Norms.MinRadiusDifficult = 1500.0f;
				Norms.MinRadiusVeryDifficult = 1000.0f;
				Norms.MinVerticalRadius = 15000.0f;
				break;

			case ERailRoadCategory::Category1:
				Norms.CategoryName = TEXT("I Категория");
				Norms.RecommendedRadii = { 4000.0f, 3000.0f, 2500.0f };
				Norms.MinRadiusDifficult = 2000.0f;
				Norms.MinRadiusVeryDifficult = 1000.0f;
				Norms.MinVerticalRadius = 15000.0f;
				break;

			case ERailRoadCategory::Category2:
				Norms.CategoryName = TEXT("II Категория");
				Norms.RecommendedRadii = { 4000.0f, 3000.0f, 2500.0f, 2000.0f };
				Norms.MinRadiusDifficult = 1500.0f;
				Norms.MinRadiusVeryDifficult = 800.0f;
				Norms.MinVerticalRadius = 10000.0f;
				break;

			case ERailRoadCategory::Category3:
				Norms.CategoryName = TEXT("III Категория");
				Norms.RecommendedRadii = { 4000.0f, 3000.0f, 2500.0f, 2000.0f, 1800.0f, 1500.0f, 1200.0f };
				Norms.MinRadiusDifficult = 800.0f;
				Norms.MinRadiusVeryDifficult = 600.0f;
				Norms.MinVerticalRadius = 8000.0f;
				break;

			case ERailRoadCategory::Category4Lines:
				Norms.CategoryName = TEXT("IV (Железнодорожные линии)");
				Norms.RecommendedRadii = { 2000.0f, 1800.0f, 1500.0f, 1200.0f, 1000.0f };
				Norms.MinRadiusDifficult = 600.0f;
				Norms.MinRadiusVeryDifficult = 350.0f;
				Norms.MinVerticalRadius = 6000.0f;
				break;

			case ERailRoadCategory::Category4Access:
				Norms.CategoryName = TEXT("IV (Подъездные пути)");
				Norms.RecommendedRadii = { 2000.0f, 1800.0f, 1500.0f, 1200.0f, 1000.0f, 800.0f, 700.0f, 600.0f };
				Norms.MinRadiusDifficult = 500.0f;
				Norms.MinRadiusVeryDifficult = 200.0f;
				Norms.MinVerticalRadius = 6000.0f;
				break;
		}
		return Norms;
	}
};