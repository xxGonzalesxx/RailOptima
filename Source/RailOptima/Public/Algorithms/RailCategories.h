// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект: Модуль критериев проектирования ЖД путей, ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "RailCategories.generated.h"

UENUM(BlueprintType)
enum class ERailRoadCategory : uint8
{
	HighSpeed		UMETA(DisplayName = "Скоростные"),
	Passenger		UMETA(DisplayName = "Пассажирские"),
	SuperHeavyHaul	UMETA(DisplayName = "Особогрузонапряженные"),
	Category1		UMETA(DisplayName = "I Категория"),
	Category2		UMETA(DisplayName = "II Категория"),
	Category3		UMETA(DisplayName = "III Категория"),
	Category4Lines	UMETA(DisplayName = "IV (Железнодорожные линии)"),
	Category4Access UMETA(DisplayName = "IV (Подъездные пути)")
};

USTRUCT(BlueprintType)
struct FRailCategoryNorms
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	ERailRoadCategory Category = ERailRoadCategory::Category1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	FString CategoryName;

	// Максимальный продольный уклон в промилле (‰) — по СП 119.13330 для новых участков не более 15‰
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	float MaxSlopePermille = 15.0f;

	// Допускаемый уклон отвода возвышения наружного рельса (мм/м)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	float MaxElevationRamp_mm_per_m = 0.7f;

	// Рекомендуемые радиусы кривых в плане (м)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	TArray<float> RecommendedRadii;

	// Допускаемый радиус в трудных условиях (м)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	float MinRadiusDifficult = 2000.0f;

	// Допускаемый радиус в особо трудных условиях при ТЭО (м)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	float MinRadiusVeryDifficult = 1000.0f;

	// Минимальный радиус вертикальной кривой (Rv), м (Таблица 5.2 СП 119.13330.2024)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	float MinVerticalRadius = 15000.0f;

	// Порог алгебраической разности уклонов (Δi, ‰), ниже которого кривая не устраивается
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	float MinSlopeDeltaForVerticalCurve = 2.0f;

	// Стандартный ряд радиусов по п. 4.4 СП 119.13330.2012 (м)
	static const TArray<float>& GetStandardPlanRadii()
	{
		static const TArray<float> StandardRadii = {
			4000.0f, 3000.0f, 2500.0f, 2000.0f, 1800.0f, 1500.0f,
			1200.0f, 1000.0f, 800.0f, 700.0f, 600.0f, 500.0f,
			400.0f, 350.0f, 300.0f, 250.0f, 200.0f, 180.0f
		};
		return StandardRadii;
	}

	// Проверяет, входит ли радиус в стандартный ряд СП 119.13330
	static bool IsStandardRadius(float InRadius_m, float Tolerance = 1.0f)
	{
		for (float R : GetStandardPlanRadii())
		{
			if (FMath::IsNearlyEqual(InRadius_m, R, Tolerance))
			{
				return true;
			}
		}
		return false;
	}

	// Выбрать наибольший возможный стандартный радиус, не превышающий MaxFittingRadius_m
	float PickStandardRadius(float MaxFittingRadius_m) const
	{
		const TArray<float>& StdRadii = GetStandardPlanRadii();

		// 1. Сначала ищем среди рекомендуемых для данной категории (возможно больших радиусов)
		for (float R : RecommendedRadii)
		{
			if (R <= MaxFittingRadius_m)
			{
				return R;
			}
		}

		// 2. Затем ищем в стандартном ряду не ниже MinRadiusVeryDifficult (при ТЭО)
		for (float R : StdRadii)
		{
			if (R <= MaxFittingRadius_m && R >= MinRadiusVeryDifficult)
			{
				return R;
			}
		}

		// 3. Если поворот слишком крутой — берём ближайший из стандартного ряда (до 180 м)
		for (float R : StdRadii)
		{
			if (R <= MaxFittingRadius_m)
			{
				return R;
			}
		}

		return 0.0f; // Не помещается ни один стандартный радиус (даже 180 м)
	}

	// Требуется ли технико-экономическое обоснование (ТЭО) и ограничение скорости (R < 300 м)
	static bool RequiresTEO(float InRadius_m)
	{
		return InRadius_m > 0.0f && InRadius_m < 300.0f;
	}

	// Расчёт тангенса вертикальной кривой: Tv = (Rv * Δi) / 2000
	float CalculateVerticalTangent_m(float InDeltaSlopePermille) const
	{
		const float AbsDelta = FMath::Abs(InDeltaSlopePermille);
		if (AbsDelta < MinSlopeDeltaForVerticalCurve)
		{
			return 0.0f;
		}
		return (MinVerticalRadius * AbsDelta) / 2000.0f;
	}

	static FRailCategoryNorms GetNorms(ERailRoadCategory InCategory)
	{
		FRailCategoryNorms Norms;
		Norms.Category = InCategory;
		Norms.MaxSlopePermille = 15.0f; // Норматив для новых участков: не более 15‰

		switch (InCategory)
		{
			case ERailRoadCategory::HighSpeed:
				Norms.CategoryName = TEXT("Скоростная");
				Norms.MaxSlopePermille = 15.0f;
				Norms.MaxElevationRamp_mm_per_m = 0.7f; // до 140+ км/ч: не более 0,7 мм/м
				Norms.RecommendedRadii = { 4000.0f, 3000.0f };
				Norms.MinRadiusDifficult = 2500.0f;
				Norms.MinRadiusVeryDifficult = 1200.0f; // 2000–1200 м
				Norms.MinVerticalRadius = 20000.0f;
				Norms.MinSlopeDeltaForVerticalCurve = 2.0f;
				break;

			case ERailRoadCategory::Passenger:
				Norms.CategoryName = TEXT("Пассажирская");
				Norms.MaxSlopePermille = 15.0f;
				Norms.MaxElevationRamp_mm_per_m = 0.7f; // до 140 км/ч: не более 0,7 мм/м
				Norms.RecommendedRadii = { 4000.0f, 3000.0f, 2500.0f };
				Norms.MinRadiusDifficult = 2000.0f;
				Norms.MinRadiusVeryDifficult = 1000.0f;
				Norms.MinVerticalRadius = 15000.0f;
				Norms.MinSlopeDeltaForVerticalCurve = 2.0f;
				break;

			case ERailRoadCategory::SuperHeavyHaul:
				Norms.CategoryName = TEXT("Особогрузонапряженная");
				Norms.MaxSlopePermille = 15.0f;
				Norms.MaxElevationRamp_mm_per_m = 1.2f; // до 100 км/ч: 1,2–1,4 мм/м
				Norms.RecommendedRadii = { 4000.0f, 3000.0f, 2500.0f, 2000.0f };
				Norms.MinRadiusDifficult = 1500.0f;		// 1800, 1500 м
				Norms.MinRadiusVeryDifficult = 1000.0f; // 1200, 1000 м
				Norms.MinVerticalRadius = 15000.0f;
				Norms.MinSlopeDeltaForVerticalCurve = 2.0f;
				break;

			case ERailRoadCategory::Category1:
				Norms.CategoryName = TEXT("I Категория");
				Norms.MaxSlopePermille = 15.0f;
				Norms.MaxElevationRamp_mm_per_m = 0.7f; // до 140 км/ч: не более 0,7 мм/м
				Norms.RecommendedRadii = { 4000.0f, 3000.0f, 2500.0f };
				Norms.MinRadiusDifficult = 2000.0f;
				Norms.MinRadiusVeryDifficult = 1000.0f; // 1800–1000 м
				Norms.MinVerticalRadius = 15000.0f;
				Norms.MinSlopeDeltaForVerticalCurve = 2.0f;
				break;

			case ERailRoadCategory::Category2:
				Norms.CategoryName = TEXT("II Категория");
				Norms.MaxSlopePermille = 15.0f;
				Norms.MaxElevationRamp_mm_per_m = 1.2f; // до 100 км/ч: 1,2–1,4 мм/м
				Norms.RecommendedRadii = { 4000.0f, 3000.0f, 2500.0f, 2000.0f };
				Norms.MinRadiusDifficult = 1500.0f;	   // 1800, 1500 м
				Norms.MinRadiusVeryDifficult = 800.0f; // 1200–800 м
				Norms.MinVerticalRadius = 10000.0f;
				Norms.MinSlopeDeltaForVerticalCurve = 2.0f;
				break;

			case ERailRoadCategory::Category3:
				Norms.CategoryName = TEXT("III Категория");
				Norms.MaxSlopePermille = 15.0f;
				Norms.MaxElevationRamp_mm_per_m = 1.4f; // до 100 км/ч: 1,2–1,4 мм/м
				Norms.RecommendedRadii = { 4000.0f, 3000.0f, 2500.0f, 2000.0f, 1800.0f, 1500.0f, 1200.0f };
				Norms.MinRadiusDifficult = 800.0f;	   // 1000, 800 м
				Norms.MinRadiusVeryDifficult = 600.0f; // 700, 600 м
				Norms.MinVerticalRadius = 8000.0f;
				Norms.MinSlopeDeltaForVerticalCurve = 2.0f;
				break;

			case ERailRoadCategory::Category4Lines:
				Norms.CategoryName = TEXT("IV (Железнодорожные линии)");
				Norms.MaxSlopePermille = 15.0f;
				Norms.MaxElevationRamp_mm_per_m = 2.5f; // до 60 км/ч: 2,5–2,7 мм/м
				Norms.RecommendedRadii = { 2000.0f, 1800.0f, 1500.0f, 1200.0f, 1000.0f };
				Norms.MinRadiusDifficult = 600.0f;	   // 800, 600 м
				Norms.MinRadiusVeryDifficult = 350.0f; // 500–350 м
				Norms.MinVerticalRadius = 6000.0f;
				Norms.MinSlopeDeltaForVerticalCurve = 2.0f;
				break;

			case ERailRoadCategory::Category4Access:
				Norms.CategoryName = TEXT("IV (Подъездные пути)");
				Norms.MaxSlopePermille = 15.0f;
				Norms.MaxElevationRamp_mm_per_m = 2.7f; // до 60 км/ч: 2,5–2,7 мм/м
				Norms.RecommendedRadii = { 2000.0f, 1800.0f, 1500.0f, 1200.0f, 1000.0f, 800.0f, 700.0f, 600.0f };
				Norms.MinRadiusDifficult = 500.0f;
				Norms.MinRadiusVeryDifficult = 200.0f; // 400–200 м
				Norms.MinVerticalRadius = 6000.0f;
				Norms.MinSlopeDeltaForVerticalCurve = 2.0f;
				break;
		}
		return Norms;
	}
};