// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль вызуализации»,
// ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CostCalculator.generated.h"

/**
 * Тип грунта под участком пути.
 *
 * Коэффициент стоимости берётся из UCostCalculator по этому значению.
 */
UENUM(BlueprintType)
enum class ERailGroundType : uint8
{
	Loam	UMETA(DisplayName = "Суглинок"),
	Sand	UMETA(DisplayName = "Песок"),
	Rock	UMETA(DisplayName = "Скала"),
	Water	UMETA(DisplayName = "Вода / болото"),
	Gravel	UMETA(DisplayName = "Гравий"),
	Clay	UMETA(DisplayName = "Глина"),
	Forest	UMETA(DisplayName = "Лес"),
	City	UMETA(DisplayName = "Город"),
	Reserve UMETA(DisplayName = "Заповедник")
};

/**
 * Параметры формулы стоимости ребра.
 *
 * Значения по умолчанию подобраны по нормам железнодорожного
 * строительства и могут быть переопределены в редакторе.
 */
USTRUCT(BlueprintType)
struct FRailCostParameters
{
	GENERATED_BODY()

	// Штраф за уклон: множитель 1.0 + |Уклон| × SlopePenalty.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Cost")
	float SlopePenalty = 0.1f;

	// Штраф за поворот: множитель 1.0 + TurnPenalty × (Угол/90)².
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Cost")
	float TurnPenalty = 0.5f;
};

/**
 * Расчёт стоимости строительства участка пути.
 *
 * Формула из ТЗ:
 *   Стоимость = Длина × (1 + |Уклон| × 0.1) × Коэф_грунта
 *             × (1 + 0.5 × (Угол_поворота / 90)²).
 *
 * Уклон задаётся отношением превышения к заложению (tan угла),
 * а не в градусах — так удобнее для норм ЖД (‰).
 */
UCLASS(BlueprintType)
class RAILOPTIMA_API UCostCalculator : public UObject
{
	GENERATED_BODY()

public:
	UCostCalculator();

	// ─────────────────────────────────────────────────────────────────
	//  Геттеры
	// ─────────────────────────────────────────────────────────────────

	/**
	 * Стоимость одного ребра маршрута.
	 *
	 * @param InLength_m           Горизонтальная длина ребра, метры.
	 * @param InSlopeRatio         Уклон как отношение превышения к заложению.
	 * @param InTurnAngleDeg       Угол поворота между рёбрами, градусы.
	 * @param InGroundCoefficient  Коэффициент типа грунта.
	 * @return                     Стоимость ребра в условных единицах.
	 */
	UFUNCTION(BlueprintPure, Category = "RailOptima|Cost")
	float CalculateEdgeCost(
		float InLength_m,
		float InSlopeRatio,
		float InTurnAngleDeg,
		float InGroundCoefficient) const;

	/**
	 * Коэффициент стоимости для типа грунта.
	 *
	 * @param InGroundType  Тип грунта.
	 * @return              Коэффициент (1.0 — базовый суглинок).
	 */
	UFUNCTION(BlueprintPure, Category = "RailOptima|Cost")
	float GetGroundCoefficient(ERailGroundType InGroundType) const;

	/**
	 * Проверка запретной зоны.
	 *
	 * @param InGroundType  Тип грунта.
	 * @return              true, если строить нельзя (город, заповедник).
	 */
	UFUNCTION(BlueprintPure, Category = "RailOptima|Cost")
	bool IsForbiddenGround(ERailGroundType InGroundType) const;

	/**
	 * Минимальный коэффициент грунта.
	 *
	 * Нужен для admissible эвристики A*: чтобы оценка не переоценивала
	 * реальную стоимость пути.
	 *
	 * @return  Наименьший коэффициент из всех типов грунтов.
	 */
	UFUNCTION(BlueprintPure, Category = "RailOptima|Cost")
	float GetMinGroundCoefficient() const { return MinGroundCoefficient; }

	// ─────────────────────────────────────────────────────────────────
	//  Управление состоянием
	// ─────────────────────────────────────────────────────────────────

	/**
	 * Переопределить коэффициент для типа грунта.
	 *
	 * @param InGroundType   Тип грунта.
	 * @param InCoefficient  Новый коэффициент.
	 */
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Cost")
	void SetGroundCoefficient(ERailGroundType InGroundType, float InCoefficient);

	// ─────────────────────────────────────────────────────────────────
	//  Настройки
	// ─────────────────────────────────────────────────────────────────

	// Параметры формулы.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Cost")
	FRailCostParameters Parameters;

private:
	// ─────────────────────────────────────────────────────────────────
	//  Внутренняя логика
	// ─────────────────────────────────────────────────────────────────

	// Пересчитать кэш минимального коэффициента.
	void UpdateMinCoefficient();

	// ─────────────────────────────────────────────────────────────────
	//  Состояние
	// ─────────────────────────────────────────────────────────────────

	// Коэффициенты по типам грунта, индекс = ERailGroundType.
	TArray<float> GroundCoefficients;

	// Кэш минимума для эвристики A*.
	float MinGroundCoefficient = 1.0f;

	// ─────────────────────────────────────────────────────────────────
	//  Константы
	// ─────────────────────────────────────────────────────────────────

	// Коэффициенты по умолчанию.
	static const TArray<float>& GetDefaultCoefficients();
};