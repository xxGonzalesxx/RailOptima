// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#include "Algorithms/CostCalculator.h"

UCostCalculator::UCostCalculator()
{
	GroundCoefficients = GetDefaultCoefficients();
	UpdateMinCoefficient();
}

double UCostCalculator::GetBaseCostPerKmBySlope_kRub(float InSlopePermille)
{
	const float AbsSlope = FMath::Abs(InSlopePermille);

	// По таблицам 07-01-005 и 07-01-006 НЦС 81-02-07-2025 (однопутная, тыс. руб./км):
	if (AbsSlope < 5.0f)
	{
		return 316236.0; // I (равнинный): среднее между 313 075 и 319 397 тыс. руб.
	}
	if (AbsSlope <= 15.0f)
	{
		return 461371.5; // II (слабохолмистый): 425 117 – 497 626 тыс. руб.
	}
	if (AbsSlope <= 25.0f)
	{
		return 580611.0; // III (холмистый): 580 611 – 632 593 тыс. руб.
	}
	return 780948.5; // IV (горный): 728 609 – 833 288 тыс. руб.
}

double UCostCalculator::GetSlopeSurchargePerKm_kRub(float InSlopePermille)
{
	const float AbsSlope = FMath::Abs(InSlopePermille);

	// Надбавки за крутизну продольного уклона и искусственные сооружения (в тыс. руб. на 1 км):
	if (AbsSlope < 5.0f)
		return 0.0; // Равнинный (до 5‰): 0 руб.
	if (AbsSlope <= 15.0f)
		return 45000.0; // Пологий (5–15‰): +45 млн руб./км
	if (AbsSlope <= 25.0f)
		return 90000.0; // Холмистый (15–25‰): +90 млн руб./км
	if (AbsSlope <= 40.0f)
		return 157500.0; // Горный (25–40‰): +157.5 млн руб./км
	if (AbsSlope <= 60.0f)
		return 450000.0; // Экстремальный (40–60‰): +450 млн руб./км

	return 1800000.0; // Критический (>60‰): +1.8 млрд руб./км (тоннели/эстакады)
}

double UCostCalculator::GetSlopeMultiplier(float InSlopePermille)
{
	const float AbsSlope = FMath::Abs(InSlopePermille);

	if (AbsSlope < 5.0f)
		return 1.0; // Равнинный (<5‰)
	if (AbsSlope <= 15.0f)
		return 1.1; // Пологий уклон (5–15‰) — коэф. 1.1 по примеру из ТЗ
	if (AbsSlope <= 25.0f)
		return 1.25; // Холмистый (15–25‰) — коэф. 1.25
	if (AbsSlope <= 40.0f)
		return 1.45; // Горный (25–40‰) — коэф. 1.2–1.5
	if (AbsSlope <= 60.0f)
		return 2.0; // Экстремальный (40–60‰) — коэф. 2.0+

	return 3.0; // Критический (>60‰)
}

double UCostCalculator::GetCurveMultiplier(float InCurveRadius_m)
{
	// Прямой участок (радиус 0 или >= 4000 м)
	if (InCurveRadius_m <= 1.0f || InCurveRadius_m >= 4000.0f)
	{
		return 1.0;
	}
	// Кривые малого радиуса (< 600 м): коэф. 1.2–1.5
	if (InCurveRadius_m < 300.0f)
	{
		return 1.5; // Особо малые радиусы (<300 м, требуют ТЭО)
	}
	if (InCurveRadius_m < 600.0f)
	{
		return 1.2; // Радиусы 300–599 м (вкл. R=400 м из примера: коэф. 1.2)
	}
	// Кривые радиусом > 600 м: коэф. 1.05–1.15
	if (InCurveRadius_m <= 1500.0f)
	{
		return 1.12;
	}
	return 1.05; // Радиусы 1500–3999 м
}

double UCostCalculator::CalculateRatePerKm_kRub(
	float			InSlopePermille,
	float			InCurveRadius_m,
	ERailGroundType InGroundType) const
{
	const double BaseCost_kRub = GetBaseCostPerKmBySlope_kRub(InSlopePermille);
	const double SlopeMult = GetSlopeMultiplier(InSlopePermille);
	const double CurveMult = GetCurveMultiplier(InCurveRadius_m);
	const double GroundCoef = static_cast<double>(GetGroundCoefficient(InGroundType));
	const double Surcharge_kRub = GetSlopeSurchargePerKm_kRub(InSlopePermille);

	// Формула из документа:
	// Стоимость 1 км = (Базовая цена 1 км × Коэф. грунта × Коэф. уклона × Коэф. кривой) + Надбавки
	return (BaseCost_kRub * GroundCoef * SlopeMult * CurveMult) + Surcharge_kRub;
}

float UCostCalculator::CalculateEdgeCost(
	float InLength_m,
	float InSlopePercent,
	float InTurnAngleDeg,
	float InGroundCoefficient) const
{
	// Переводим проценты в промилле (1.0% = 10‰)
	const float SlopePermille = FMath::Abs(InSlopePercent) * 10.0f;

	// Оцениваем радиус вписываемой кривой на сетке A* по углу поворота
	float EstimatedRadius_m = 0.0f;
	if (InTurnAngleDeg > 1.0f)
	{
		const float HalfAngleRad = FMath::DegreesToRadians(FMath::Clamp(InTurnAngleDeg, 1.0f, 170.0f) * 0.5f);
		EstimatedRadius_m = (InLength_m * 0.5f) / FMath::Max(FMath::Tan(HalfAngleRad), 0.01f);
	}

	const double BaseCost_kRub = GetBaseCostPerKmBySlope_kRub(SlopePermille);
	const double SlopeMult = GetSlopeMultiplier(SlopePermille) * (1.0 + FMath::Abs(InSlopePercent) * Parameters.SlopePenalty * 0.1);
	const double CurveMult = GetCurveMultiplier(EstimatedRadius_m) * (1.0 + Parameters.TurnPenalty * FMath::Square(InTurnAngleDeg / 90.0f));
	const double Surcharge_kRub = GetSlopeSurchargePerKm_kRub(SlopePermille);

	const double RatePerKm_kRub = (BaseCost_kRub * InGroundCoefficient * SlopeMult * CurveMult) + Surcharge_kRub;
	const double Length_km = static_cast<double>(InLength_m) / 1000.0;

	return static_cast<float>(RatePerKm_kRub * Length_km);
}

float UCostCalculator::GetGroundCoefficient(ERailGroundType InGroundType) const
{
	const int32 Index = static_cast<int32>(InGroundType);
	if (!GroundCoefficients.IsValidIndex(Index))
	{
		return 1.0f;
	}
	return GroundCoefficients[Index];
}

bool UCostCalculator::IsForbiddenGround(ERailGroundType InGroundType) const
{
	return InGroundType == ERailGroundType::City || InGroundType == ERailGroundType::Reserve;
}

void UCostCalculator::SetGroundCoefficient(ERailGroundType InGroundType, float InCoefficient)
{
	const int32 Index = static_cast<int32>(InGroundType);
	if (!GroundCoefficients.IsValidIndex(Index))
	{
		return;
	}

	GroundCoefficients[Index] = FMath::Max(InCoefficient, 0.01f);
	UpdateMinCoefficient();
}

void UCostCalculator::UpdateMinCoefficient()
{
	MinGroundCoefficient = TNumericLimits<float>::Max();
	for (const float Coefficient : GroundCoefficients)
	{
		MinGroundCoefficient = FMath::Min(MinGroundCoefficient, Coefficient);
	}
}

const TArray<float>& UCostCalculator::GetDefaultCoefficients()
{
	static const TArray<float> Defaults = {
		1.0f,  // Loam (Суглинок — база)
		0.95f, // Sand (Песок)
		1.35f, // Rock (Скала)
		1.60f, // Water (Болото/Вода)
		0.90f, // Gravel (Гравий)
		1.10f, // Clay (Глина)
		1.15f, // Forest (Лес)
		10.0f, // City (Город — запретная зона)
		10.0f  // Reserve (Заповедник — запретная зона)
	};
	return Defaults;
}