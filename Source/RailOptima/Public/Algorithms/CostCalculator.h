// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CostCalculator.generated.h"

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

UENUM(BlueprintType)
enum class ERailTerrainDifficulty : uint8
{
	Flat		UMETA(DisplayName = "I — Равнинный (313–319 млн руб/км)"),
	MildHilly	UMETA(DisplayName = "II — Слабохолмистый (425–497 млн руб/км)"),
	Hilly		UMETA(DisplayName = "III — Холмистый (580–632 млн руб/км)"),
	Mountainous UMETA(DisplayName = "IV — Горный (728–833 млн руб/км)")
};

USTRUCT(BlueprintType)
struct FRailCostParameters
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Cost")
	float SlopePenalty = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Cost")
	float TurnPenalty = 0.5f;
};

/**
 * Расчёт стоимости строительства участка пути по НЦС 81-02-07-2025.
 * Все денежные значения возвращаются в ТЫСЯЧАХ РУБЛЕЙ (тыс. руб.),
 * либо в рублях через вспомогательные функции.
 */
UCLASS(BlueprintType)
class RAILOPTIMA_API UCostCalculator : public UObject
{
	GENERATED_BODY()

public:
	UCostCalculator();

	/**
	 * Стоимость ребра для A* и итоговой сметы (в тыс. руб.).
	 *
	 * @param InLength_m           Горизонтальная длина участка, метры.
	 * @param InSlopePercent       Продольный уклон в процентах (например, 1.5% = 15‰).
	 * @param InTurnAngleDeg       Угол поворота между сегментами, градусы.
	 * @param InGroundCoefficient  Коэффициент типа грунта.
	 * @return                     Стоимость участка в тыс. руб.
	 */
	UFUNCTION(BlueprintPure, Category = "RailOptima|Cost")
	float CalculateEdgeCost(
		float InLength_m,
		float InSlopePercent,
		float InTurnAngleDeg,
		float InGroundCoefficient) const;

	/**
	 * Расчёт стоимости 1 км пути (тыс. руб./км) по формуле НЦС 81-02-07-2025:
	 * Цена_1км = (База_НЦС * Коэф_грунта * Коэф_уклона * Коэф_кривой) + Надбавка_уклона
	 *
	 * @param InSlopePermille  Крутизна уклона в промилле (‰).
	 * @param InCurveRadius_m  Радиус кривой в метрах (0 или >10000 = прямая).
	 * @param InGroundType     Тип грунта.
	 */
	UFUNCTION(BlueprintPure, Category = "RailOptima|Cost")
	double CalculateRatePerKm_kRub(
		float			InSlopePermille,
		float			InCurveRadius_m,
		ERailGroundType InGroundType = ERailGroundType::Loam) const;

	/** Базовая цена 1 км по категории рельефа НЦС 81-02-07-2025 (тыс. руб./км) */
	UFUNCTION(BlueprintPure, Category = "RailOptima|Cost")
	static double GetBaseCostPerKmBySlope_kRub(float InSlopePermille);

	/** Надбавка за крутизну уклона (тыс. руб./км) */
	UFUNCTION(BlueprintPure, Category = "RailOptima|Cost")
	static double GetSlopeSurchargePerKm_kRub(float InSlopePermille);

	/** Множитель уклона (1.0 для равнинного, 1.1 для пологого и т.д.) */
	UFUNCTION(BlueprintPure, Category = "RailOptima|Cost")
	static double GetSlopeMultiplier(float InSlopePermille);

	/** Множитель кривой в плане (1.0 прямая, 1.1 при R>=600м, 1.25..1.5 при R<600м) */
	UFUNCTION(BlueprintPure, Category = "RailOptima|Cost")
	static double GetCurveMultiplier(float InCurveRadius_m);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Cost")
	float GetGroundCoefficient(ERailGroundType InGroundType) const;

	UFUNCTION(BlueprintPure, Category = "RailOptima|Cost")
	bool IsForbiddenGround(ERailGroundType InGroundType) const;

	UFUNCTION(BlueprintPure, Category = "RailOptima|Cost")
	float GetMinGroundCoefficient() const { return MinGroundCoefficient; }

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Cost")
	void SetGroundCoefficient(ERailGroundType InGroundType, float InCoefficient);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Cost")
	FRailCostParameters Parameters;

	// Минимальная удельная цена 1 км (тыс. руб.) — зелёный цвет на градиенте
	static constexpr double MinColorRate_kRub = 313000.0; // ~313 млн руб/км (I кат. рельефа)

	// Высокая удельная цена 1 км (тыс. руб.) — красный цвет на градиенте
	static constexpr double MaxColorRate_kRub = 1200000.0; // ~1.2 млрд руб/км (горный + кривые + надбавки)

private:
	void		  UpdateMinCoefficient();
	TArray<float> GroundCoefficients;
	float		  MinGroundCoefficient = 1.0f;

	static const TArray<float>& GetDefaultCoefficients();
};