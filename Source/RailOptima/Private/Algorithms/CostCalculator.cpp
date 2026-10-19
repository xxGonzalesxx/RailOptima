// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль вызуализации»,
// ПривГУПС, 2026.

#include "Algorithms/CostCalculator.h"

// ─────────────────────────────────────────────────────────────────
//  Жизненный цикл
// ─────────────────────────────────────────────────────────────────

UCostCalculator::UCostCalculator()
{
	GroundCoefficients = GetDefaultCoefficients();
	UpdateMinCoefficient();
}

// ─────────────────────────────────────────────────────────────────
//  Геттеры
// ─────────────────────────────────────────────────────────────────

float UCostCalculator::CalculateEdgeCost(
	float InLength_m,
	float InSlopeRatio,
	float InTurnAngleDeg,
	float InGroundCoefficient) const
{
	const float SlopeFactor = 1.0f + FMath::Abs(InSlopeRatio) * Parameters.SlopePenalty;
	const float TurnFactor = 1.0f + Parameters.TurnPenalty * FMath::Square(InTurnAngleDeg / 90.0f);

	return InLength_m * SlopeFactor * InGroundCoefficient * TurnFactor;
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

// ─────────────────────────────────────────────────────────────────
//  Управление состоянием
// ─────────────────────────────────────────────────────────────────

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

// ─────────────────────────────────────────────────────────────────
//  Внутренняя логика
// ─────────────────────────────────────────────────────────────────

void UCostCalculator::UpdateMinCoefficient()
{
	MinGroundCoefficient = TNumericLimits<float>::Max();
	for (const float Coefficient : GroundCoefficients)
	{
		MinGroundCoefficient = FMath::Min(MinGroundCoefficient, Coefficient);
	}
}

// ─────────────────────────────────────────────────────────────────
//  Константы
// ─────────────────────────────────────────────────────────────────

const TArray<float>& UCostCalculator::GetDefaultCoefficients()
{
	// Значения откалиброваны так, чтобы:
	//  - песок и гравий дешевле суглинка;
	//  - скала и болото — самые дорогие;
	//  - город и заповедник — фактически запретные зоны.
	static const TArray<float> Defaults = {
		1.0f,  // Loam
		0.9f,  // Sand
		3.5f,  // Rock
		5.0f,  // Water
		0.8f,  // Gravel
		1.2f,  // Clay
		1.8f,  // Forest
		10.0f, // City
		10.0f  // Reserve
	};
	return Defaults;
}