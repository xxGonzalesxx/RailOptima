// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "RailDesignCriteria.generated.h"

// Категории железнодорожных линий согласно нормам
UENUM(BlueprintType)
enum class ERailLineCategory : uint8
{
	HighSpeed	UMETA(DisplayName = "Скоростная"),
	HeavyHaul	UMETA(DisplayName = "Особогрузонапряженная"),
	CategoryI	UMETA(DisplayName = "Категория I"),
	CategoryII	UMETA(DisplayName = "Категория II (Критерий 1 по умолчанию)"),
	CategoryIII UMETA(DisplayName = "Категория III"),
	CategoryIV	UMETA(DisplayName = "Категория IV"),
	Industrial	UMETA(DisplayName = "Подъездной путь")
};

// Структура нормативных критериев проектирования
USTRUCT(BlueprintType)
struct FRailDesignCriteria
{
	GENERATED_BODY()

	// Категория линии
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	ERailLineCategory Category = ERailLineCategory::CategoryII;

	// Максимальный продольный уклон в промилле (‰). По умолчанию для Критерия 1 = 15.0‰
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	float MaxSlopePermille = 15.0f;

	// Допускаемый уклон отвода возвышения (мм/м) в зависимости от скорости
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	float MaxElevationRailDropRate = 0.7f;

	// Минимальный радиус вертикальной кривой (Rv), м (для Категории II по умолчанию 10000м)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	float MinVerticalCurveRadius = 10000.0f;

	// Порог разности уклонов (Δi, ‰), ниже которого вертикальная кривая не устраивается (по умолчанию 2.0‰)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	float MinDeltaSlopeForVerticalCurve = 2.0f;

	// Стандартный ряд радиусов кривых в плане (м)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Norms")
	TArray<float> StandardPlanRadii = { 4000, 3000, 2500, 2000, 1800, 1500, 1200, 1000, 800, 700, 600, 500, 400, 350, 300, 250, 200, 180 };

	/**
	 * Проверка и подгонка радиуса в плане под стандартный ряд
	 */
	float FindClosestStandardRadius(float InDesiredRadius) const
	{
		if (StandardPlanRadii.Num() == 0)
			return InDesiredRadius;

		float BestRadius = StandardPlanRadii[0];
		float MinDiff = FMath::Abs(InDesiredRadius - BestRadius);

		for (float Rad : StandardPlanRadii)
		{
			float Diff = FMath::Abs(InDesiredRadius - Rad);
			if (Diff < MinDiff)
			{
				MinDiff = Diff;
				BestRadius = Rad;
			}
		}
		return BestRadius;
	}

	/**
	 * Расчет тангенса вертикальной кривой (Tv) по формуле: Tv = (Rv * Δi) / 2000
	 * @param DeltaSlopePermille Алгебраическая разность уклонов в ‰
	 */
	float CalculateVerticalCurveTangent(float DeltaSlopePermille) const
	{
		if (FMath::Abs(DeltaSlopePermille) < MinDeltaSlopeForVerticalCurve)
		{
			return 0.0f; // Кривая не нужна
		}
		return (MinVerticalCurveRadius * FMath::Abs(DeltaSlopePermille)) / 2000.0f;
	}
};