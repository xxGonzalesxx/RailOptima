// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект

#include "Algorithms/RailPathValidator.h"

FRailValidationResult URailPathValidator::ValidatePath(
	const FRailGridData&	  InGrid,
	const FRailPathResult&	  InPath,
	const FRailCategoryNorms& InNorms) const
{
	FRailValidationResult Result;

	if (!InPath.bFound || InPath.NodeIndices.Num() < 3)
	{
		return Result;
	}

	const int32 NumNodes = InPath.NodeIndices.Num();

	// Проходим по всем тройкам точек для проверки горизонтальных кривых
	for (int32 i = 1; i < NumNodes - 1; ++i)
	{
		const FRailGridNode& A = InGrid.Nodes[InPath.NodeIndices[i - 1]];
		const FRailGridNode& B = InGrid.Nodes[InPath.NodeIndices[i]];
		const FRailGridNode& C = InGrid.Nodes[InPath.NodeIndices[i + 1]];

		// Длины сторон (в метрах) - приводим к float для корректной работы FMath::Sqrt
		const float DX_AB = static_cast<float>(B.X - A.X);
		const float DY_AB = static_cast<float>(B.Y - A.Y);
		const float AB = InGrid.StepSize_m * FMath::Sqrt(DX_AB * DX_AB + DY_AB * DY_AB);

		const float DX_BC = static_cast<float>(C.X - B.X);
		const float DY_BC = static_cast<float>(C.Y - B.Y);
		const float BC = InGrid.StepSize_m * FMath::Sqrt(DX_BC * DX_BC + DY_BC * DY_BC);

		const float DX_AC = static_cast<float>(C.X - A.X);
		const float DY_AC = static_cast<float>(C.Y - A.Y);
		const float AC = InGrid.StepSize_m * FMath::Sqrt(DX_AC * DX_AC + DY_AC * DY_AC);

		// Площадь треугольника по формуле Герона
		const float P = (AB + BC + AC) * 0.5f;
		const float AreaSq = P * (P - AB) * (P - BC) * (P - AC);

		if (AreaSq > KINDA_SMALL_NUMBER)
		{
			const float Area = FMath::Sqrt(AreaSq);
			// Радиус описанной окружности R = (a * b * c) / (4 * S)
			const float R = (AB * BC * AC) / (4.0f * Area);

			if (R < InNorms.MinRadiusVeryDifficult)
			{
				FRailPathViolation V;
				V.NodeIndex = InPath.NodeIndices[i];
				V.Message = FString::Printf(TEXT("Критический радиус поворота: %.1f м (мин. %.1f м)"), R, InNorms.MinRadiusVeryDifficult);
				Result.Violations.Add(V);
				Result.bIsValid = false;
			}
			else if (R < InNorms.MinRadiusDifficult)
			{
				FRailPathViolation V;
				V.NodeIndex = InPath.NodeIndices[i];
				V.Message = FString::Printf(TEXT("Трудный радиус поворота: %.1f м (реком. %.1f м)"), R, InNorms.MinRadiusDifficult);
				Result.Violations.Add(V);
			}
		}
	}

	// Проходим по всем стыкам для проверки вертикальных кривых
	for (int32 i = 1; i < NumNodes - 1; ++i)
	{
		const int32 PrevIdx = InPath.NodeIndices[i - 1];
		const int32 CurrIdx = InPath.NodeIndices[i];
		const int32 NextIdx = InPath.NodeIndices[i + 1];

		const float Dist1 = InPath.EdgeLengths[i - 1];
		const float Dist2 = InPath.EdgeLengths[i];

		if (Dist1 <= KINDA_SMALL_NUMBER || Dist2 <= KINDA_SMALL_NUMBER)
			continue;

		// Уклон в промилле
		const float Slope1 = ((InGrid.Nodes[CurrIdx].Height_m - InGrid.Nodes[PrevIdx].Height_m) / Dist1) * 1000.0f;
		const float Slope2 = ((InGrid.Nodes[NextIdx].Height_m - InGrid.Nodes[CurrIdx].Height_m) / Dist2) * 1000.0f;

		// Алгебраическая разность уклонов
		const float DeltaI = FMath::Abs(Slope2 - Slope1);

		if (DeltaI > InNorms.MinSlopeDeltaForVerticalCurve)
		{
			// Проверяем тангенс вертикальной кривой Tв = Rв * Δi / 2000
			const float Tv = InNorms.MinVerticalRadius * DeltaI / 2000.0f;

			// Если тангенс кривой больше половины длины сегмента — кривая не влезает
			if (Tv > (Dist1 * 0.5f) || Tv > (Dist2 * 0.5f))
			{
				FRailPathViolation V;
				V.NodeIndex = CurrIdx;
				V.Message = FString::Printf(TEXT("Вертикальная кривая не влезает: Δi=%.1f‰, Tв=%.1f м (сегменты %.1f/%.1f м)"), DeltaI, Tv, Dist1, Dist2);
				Result.Violations.Add(V);
				Result.bIsValid = false;
			}
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