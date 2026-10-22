// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Модуль проверки пути на соответствие СП 119.13330 и НЦС 81-02-07-2025.

#include "Algorithms/RailPathValidator.h"

FRailValidationResult URailPathValidator::ValidateWorldTrack(
	const TArray<FVector>&	  InWorldPoints_cm,
	const FRailCategoryNorms& InNorms,
	float					  InWorldZScale) const
{
	FRailValidationResult Result;
	const int32			  NumPoints = InWorldPoints_cm.Num();

	if (NumPoints < 2)
	{
		return Result;
	}

	const float ZScale = (InWorldZScale > 0.0f) ? InWorldZScale : 1.0f;

	// ─────────────────────────────────────────────────────────────────
	// 1. Проверка продольных уклонов каждого сегмента (СП 119.13330 <= 15‰)
	// ─────────────────────────────────────────────────────────────────
	for (int32 i = 0; i < NumPoints - 1; ++i)
	{
		const FVector& P1 = InWorldPoints_cm[i];
		const FVector& P2 = InWorldPoints_cm[i + 1];

		const float DistXY_m = FVector2D::Distance(FVector2D(P1.X, P1.Y), FVector2D(P2.X, P2.Y)) * 0.01f;
		if (DistXY_m < 0.1f)
			continue;

		const float Dz_m = (FMath::Abs(P2.Z - P1.Z) * 0.01f) / ZScale;
		const float SlopePermille = (Dz_m / DistXY_m) * 1000.0f;

		if (SlopePermille > InNorms.MaxSlopePermille + 0.5f)
		{
			FRailPathViolation V;
			V.NodeIndex = i;
			V.bIsWarning = false;
			V.Message = FString::Printf(
				TEXT("Превышен максимальный продольный уклон: %.1f‰ (норматив: %.1f‰ для %s)"),
				SlopePermille, InNorms.MaxSlopePermille, *InNorms.CategoryName);
			Result.Violations.Add(V);
			Result.bIsValid = false;
		}
	}

	// ─────────────────────────────────────────────────────────────────
	// 2. Проверка кривых в плане (Стандартный ряд СП 119.13330, R < 300 м ТЭО)
	// ─────────────────────────────────────────────────────────────────
	if (NumPoints >= 3)
	{
		// Шаг выборки хорды вдоль трассы (~40–80 м) для устойчивой оценки радиуса
		const int32 ChordStep = 3;

		for (int32 i = ChordStep; i < NumPoints - ChordStep; i += ChordStep)
		{
			const FVector2D A(InWorldPoints_cm[i - ChordStep].X * 0.01f, InWorldPoints_cm[i - ChordStep].Y * 0.01f);
			const FVector2D B(InWorldPoints_cm[i].X * 0.01f, InWorldPoints_cm[i].Y * 0.01f);
			const FVector2D C(InWorldPoints_cm[i + ChordStep].X * 0.01f, InWorldPoints_cm[i + ChordStep].Y * 0.01f);

			const float AB = FVector2D::Distance(A, B);
			const float BC = FVector2D::Distance(B, C);
			const float AC = FVector2D::Distance(A, C);

			if (AB < 1.0f || BC < 1.0f)
				continue;

			// Угол излома
			const FVector2D V1 = (B - A).GetSafeNormal();
			const FVector2D V2 = (C - B).GetSafeNormal();
			const float		Dot = FVector2D::DotProduct(V1, V2);

			if (Dot < 0.999f && Dot > -0.99f)
			{
				const float P = (AB + BC + AC) * 0.5f;
				const float AreaSq = P * (P - AB) * (P - BC) * (P - AC);

				if (AreaSq > 0.001f)
				{
					const float Area = FMath::Sqrt(AreaSq);
					const float R = (AB * BC * AC) / (4.0f * Area);

					// Проверка на малый радиус < 300 м (требуется ТЭО)
					if (R < 300.0f)
					{
						FRailPathViolation V;
						V.NodeIndex = i;
						V.bIsWarning = true;
						V.bRequiresTEO = true;
						V.Message = FString::Printf(
							TEXT("Малый радиус кривой R=%.0f м (< 300 м): по СП 119.13330 требуется ТЭО и ограничение скорости!"),
							R);
						Result.Violations.Add(V);
					}

					// Проверка на допустимость радиуса для категории
					if (R < InNorms.MinRadiusVeryDifficult - 5.0f)
					{
						FRailPathViolation V;
						V.NodeIndex = i;
						V.bIsWarning = false;
						V.Message = FString::Printf(
							TEXT("Критический радиус кривой R=%.0f м ниже допустимого для %s (мин. %.0f м)"),
							R, *InNorms.CategoryName, InNorms.MinRadiusVeryDifficult);
						Result.Violations.Add(V);
						Result.bIsValid = false;
					}
					else if (R < InNorms.MinRadiusDifficult - 5.0f)
					{
						FRailPathViolation V;
						V.NodeIndex = i;
						V.bIsWarning = true;
						V.Message = FString::Printf(
							TEXT("Радиус кривой в трудных условиях R=%.0f м (рекомендуется от %.0f м)"),
							R, InNorms.MinRadiusDifficult);
						Result.Violations.Add(V);
					}
				}
			}
		}
	}

	// ─────────────────────────────────────────────────────────────────
	// 3. Проверка вертикальных кривых (Rв, порог Δi >= 2‰, тангенс Tв)
	// ─────────────────────────────────────────────────────────────────
	if (NumPoints >= 3)
	{
		for (int32 i = 1; i < NumPoints - 1; ++i)
		{
			const FVector& PPrev = InWorldPoints_cm[i - 1];
			const FVector& PCurr = InWorldPoints_cm[i];
			const FVector& PNext = InWorldPoints_cm[i + 1];

			const float Dist1 = FVector2D::Distance(FVector2D(PPrev.X, PPrev.Y), FVector2D(PCurr.X, PCurr.Y)) * 0.01f;
			const float Dist2 = FVector2D::Distance(FVector2D(PCurr.X, PCurr.Y), FVector2D(PNext.X, PNext.Y)) * 0.01f;

			if (Dist1 < 1.0f || Dist2 < 1.0f)
				continue;

			const float Slope1 = (((PCurr.Z - PPrev.Z) * 0.01f / ZScale) / Dist1) * 1000.0f;
			const float Slope2 = (((PNext.Z - PCurr.Z) * 0.01f / ZScale) / Dist2) * 1000.0f;
			const float DeltaI = FMath::Abs(Slope2 - Slope1);

			if (DeltaI >= InNorms.MinSlopeDeltaForVerticalCurve)
			{
				// Tв = Rв * Δi / 2000
				const float Tv = (InNorms.MinVerticalRadius * DeltaI) / 2000.0f;

				if (Tv > (Dist1 + Dist2) * 0.5f)
				{
					FRailPathViolation V;
					V.NodeIndex = i;
					V.bIsWarning = true;
					V.Message = FString::Printf(
						TEXT("Вертикальная кривая (Rв=%.0f м, Δi=%.1f‰, Tв=%.1f м) перекрывает смежные элементы профиля (%.0f / %.0f м)"),
						InNorms.MinVerticalRadius, DeltaI, Tv, Dist1, Dist2);
					Result.Violations.Add(V);
				}
			}
		}
	}

	return Result;
}

FRailValidationResult URailPathValidator::ValidatePath(
	const FRailGridData&	  InGrid,
	const FRailPathResult&	  InPath,
	const FRailCategoryNorms& InNorms) const
{
	FRailValidationResult Result;

	if (!InPath.bFound || InPath.NodeIndices.Num() < 2)
	{
		return Result;
	}

	// Проверка уклонов на ребрах сырой сетки
	for (int32 i = 0; i < InPath.EdgeLengths.Num(); ++i)
	{
		const float EdgeLen = InPath.EdgeLengths[i];
		if (EdgeLen <= 0.1f)
			continue;

		const int32 FromIdx = InPath.NodeIndices[i];
		const int32 ToIdx = InPath.NodeIndices[i + 1];

		const float HeightDiff = FMath::Abs(InGrid.Nodes[ToIdx].Height_m - InGrid.Nodes[FromIdx].Height_m);
		const float SlopePermille = (HeightDiff / EdgeLen) * 1000.0f;

		if (SlopePermille > InNorms.MaxSlopePermille + 0.1f)
		{
			FRailPathViolation V;
			V.NodeIndex = FromIdx;
			V.Message = FString::Printf(TEXT("Уклон ребра сетки %.1f‰ превышает норматив %.1f‰"), SlopePermille, InNorms.MaxSlopePermille);
			Result.Violations.Add(V);
			Result.bIsValid = false;
		}
	}

	return Result;
}

void URailPathValidator::SmoothPathToNorms(
	const FRailGridData&	  InGrid,
	const FRailPathResult&	  InPath,
	const FRailCategoryNorms& InNorms,
	TArray<FVector>&		  OutSmoothWorldPoints) const
{
	OutSmoothWorldPoints.Reset();

	for (int32 i = 0; i < InPath.NodeIndices.Num(); ++i)
	{
		const FRailGridNode& N = InGrid.Nodes[InPath.NodeIndices[i]];
		OutSmoothWorldPoints.Add(FVector(
			N.X * InGrid.StepSize_m * 100.0f,
			N.Y * InGrid.StepSize_m * 100.0f,
			N.Height_m * 100.0f));
	}
}
