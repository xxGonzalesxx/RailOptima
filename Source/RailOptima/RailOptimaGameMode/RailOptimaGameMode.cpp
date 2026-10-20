// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#include "RailOptimaGameMode.h"
#include "Algorithms/AStarPathFinder.h"
#include "Algorithms/CostCalculator.h"
#include "Algorithms/RailPathValidator.h"
#include "Blueprint/UserWidget.h"
#include "Core/ProceduralTerrain.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Interactive/RailEndpoint.h"
#include "Interactive/RailOptimaPlayerController.h"
#include "Interactive/RailSpectatorPawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Visualization/PathRenderer.h"
#include "Visualization/RailCostSummaryWidget.h"

ARailOptimaGameMode::ARailOptimaGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
	PlayerControllerClass = ARailOptimaPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void ARailOptimaGameMode::BeginPlay()
{
	Super::BeginPlay();
	SetupSubsystems();
	FindTerrain();
	SetupPathRenderer();
	SetupEndpoints();
	SetupCostSummaryWidget();
	RecomputePath();
}

void ARailOptimaGameMode::SetupCostSummaryWidget()
{
	if (CostSummaryWidget)
		return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
		return;

	CostSummaryWidget = CreateWidget<URailCostSummaryWidget>(PC, URailCostSummaryWidget::StaticClass());
	if (CostSummaryWidget)
	{
		CostSummaryWidget->AddToViewport(10);
		UE_LOG(LogTemp, Log, TEXT("[RailGM] UI-виджет сметы URailCostSummaryWidget добавлен на экран."));
	}
}

void ARailOptimaGameMode::ApplySettings(float InSlopePenalty, float InTurnPenalty, float InStepSize_m, bool bInColorByCost)
{
	if (!CostCalculator)
		return;

	const bool bStepChanged = !FMath::IsNearlyEqual(RuntimeSettings.TargetStepSize_m, InStepSize_m, 1.0f);

	RuntimeSettings.SlopePenalty = FMath::Clamp(InSlopePenalty, 0.0f, 100.0f);
	RuntimeSettings.TurnPenalty = FMath::Clamp(InTurnPenalty, 0.0f, 100.0f);
	RuntimeSettings.TargetStepSize_m = FMath::Clamp(InStepSize_m, 10.0f, 1000.0f);
	RuntimeSettings.bColorPathByCost = bInColorByCost;

	ApplySelectedCategory();

	if (bStepChanged)
	{
		bGridCached = false;
		CachedGrid = FRailGridData();
	}

	RecomputePath();
}

void ARailOptimaGameMode::ApplyViewScaleFromCamera(ARailSpectatorPawn* InPawn)
{
	if (!InPawn)
		return;
	float TargetScale = 1.0f;

	if (InPawn->GetCameraMode() == ERailCameraMode::TopDown)
	{
		const float CurrentHeight = InPawn->GetTopDownHeight();
		if (!bViewScaleReferenceCaptured && CurrentHeight > 0.0f)
		{
			ReferenceTopDownHeight_cm = CurrentHeight;
			bViewScaleReferenceCaptured = true;
		}
		if (ReferenceTopDownHeight_cm > 0.0f)
			TargetScale = CurrentHeight / ReferenceTopDownHeight_cm;
	}

	TargetScale = FMath::Clamp(TargetScale, MinViewScale, MaxViewScale);
	if (FMath::IsNearlyEqual(TargetScale, CurrentViewScale, 0.02f))
		return;

	CurrentViewScale = TargetScale;

	if (StartEndpoint)
		StartEndpoint->SetViewScale(CurrentViewScale);
	if (EndEndpoint)
		EndEndpoint->SetViewScale(CurrentViewScale);

	if (PathRenderer)
	{
		PathRenderer->SetViewScale(CurrentViewScale);
	}
}

void ARailOptimaGameMode::SetSelectedCategory(ERailRoadCategory NewCategory)
{
	if (SelectedCategory == NewCategory)
		return;
	SelectedCategory = NewCategory;
	ApplySelectedCategory();
	RecomputePath();
}

void ARailOptimaGameMode::ApplySelectedCategory()
{
	if (!PathFinder || !CostCalculator)
		return;

	const FRailCategoryNorms Norms = FRailCategoryNorms::GetNorms(SelectedCategory);
	PathFinder->SetCategoryNorms(Norms);

	float CategoryTurnPenalty = RuntimeSettings.TurnPenalty;
	switch (SelectedCategory)
	{
		case ERailRoadCategory::HighSpeed:
			CategoryTurnPenalty = 3.0f;
			break;
		case ERailRoadCategory::Passenger:
			CategoryTurnPenalty = 2.5f;
			break;
		case ERailRoadCategory::SuperHeavyHaul:
			CategoryTurnPenalty = 2.5f;
			break;
		case ERailRoadCategory::Category1:
			CategoryTurnPenalty = 2.0f;
			break;
		case ERailRoadCategory::Category2:
			CategoryTurnPenalty = 1.5f;
			break;
		case ERailRoadCategory::Category3:
			CategoryTurnPenalty = 1.0f;
			break;
		case ERailRoadCategory::Category4Lines:
			CategoryTurnPenalty = 0.5f;
			break;
		case ERailRoadCategory::Category4Access:
			CategoryTurnPenalty = 0.2f;
			break;
	}

	CostCalculator->Parameters.TurnPenalty = CategoryTurnPenalty;
	CostCalculator->Parameters.SlopePenalty = RuntimeSettings.SlopePenalty;
}

void ARailOptimaGameMode::SetupSubsystems()
{
	PathFinder = NewObject<UAStarPathFinder>(this);
	CostCalculator = NewObject<UCostCalculator>(this);
	PathValidator = NewObject<URailPathValidator>(this);
	PathFinder->SetCostCalculator(CostCalculator);
	ApplySelectedCategory();
}

void ARailOptimaGameMode::FindTerrain()
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProceduralTerrain::StaticClass(), Found);
	if (Found.Num() > 0)
		Terrain = Cast<AProceduralTerrain>(Found[0]);
}

void ARailOptimaGameMode::SetupPathRenderer()
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APathRenderer::StaticClass(), Found);
	if (Found.Num() > 0)
		PathRenderer = Cast<APathRenderer>(Found[0]);

	if (PathRenderer && Terrain)
	{
		PathRenderer->WorldZScale = Terrain->TerrainParameters.ZScale;
		const FVector Loc = Terrain->GetActorLocation();
		PathRenderer->GridWorldOrigin = FVector(Loc.X, Loc.Y, 0.0f);
	}
}

void ARailOptimaGameMode::SetupEndpoints()
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARailEndpoint::StaticClass(), Found);
	for (AActor* Actor : Found)
	{
		ARailEndpoint* E = Cast<ARailEndpoint>(Actor);
		if (!E)
			continue;
		if (E->bIsStart && !StartEndpoint)
			StartEndpoint = E;
		else if (!E->bIsStart && !EndEndpoint)
			EndEndpoint = E;
	}

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (!StartEndpoint)
	{
		StartEndpoint = GetWorld()->SpawnActor<ARailEndpoint>(ARailEndpoint::StaticClass(), FVector(0, 0, 0), FRotator::ZeroRotator, SP);
		if (StartEndpoint)
			StartEndpoint->bIsStart = true;
	}
	if (!EndEndpoint)
	{
		EndEndpoint = GetWorld()->SpawnActor<ARailEndpoint>(ARailEndpoint::StaticClass(), FVector(0, 0, 0), FRotator::ZeroRotator, SP);
		if (EndEndpoint)
			EndEndpoint->bIsStart = false;
	}

	if (StartEndpoint)
		StartEndpoint->OnEndpointMoved.AddDynamic(this, &ARailOptimaGameMode::HandleEndpointMoved);
	if (EndEndpoint)
		EndEndpoint->OnEndpointMoved.AddDynamic(this, &ARailOptimaGameMode::HandleEndpointMoved);
}

void ARailOptimaGameMode::RecomputePath()
{
	if (!PathFinder || !PathRenderer || !StartEndpoint || !EndEndpoint)
		return;

	if (!CostSummaryWidget)
	{
		SetupCostSummaryWidget();
	}

	if (!bGridCached)
	{
		if (Terrain && Terrain->GetHeightMapData().Heights.Num() > 0)
			CachedGrid = BuildGridFromTerrain(*Terrain, RuntimeSettings.TargetStepSize_m);
		else
			CachedGrid = GenerateSyntheticGrid(336, RuntimeSettings.TargetStepSize_m);
		bGridCached = CachedGrid.Width > 1 && CachedGrid.Height > 1;
	}

	if (!bGridCached)
		return;

	const int32 StartIdx = WorldToNodeIndex(CachedGrid, StartEndpoint->GetActorLocation());
	const int32 EndIdx = WorldToNodeIndex(CachedGrid, EndEndpoint->GetActorLocation());

	const FRailCategoryNorms Norms = FRailCategoryNorms::GetNorms(SelectedCategory);

	if (StartIdx == INDEX_NONE || EndIdx == INDEX_NONE || StartIdx == EndIdx)
	{
		PathRenderer->ClearPath();
		if (CostSummaryWidget)
		{
			CostSummaryWidget->UpdateRouteSummary(Norms.CategoryName, 0.0, 0.0, 0.0f, 0.0f, 0.0);
		}
		OnPathRecomputed.Broadcast(0.0, 0.0, 0.0, 0);
		return;
	}

	// 1. Сначала ищем путь в строгом соответствии с нормативом уклона (15‰)
	PathFinder->SetCategoryNorms(Norms);

	const double	T0 = FPlatformTime::Seconds();
	FRailPathResult Result = PathFinder->FindPath(CachedGrid, StartIdx, EndIdx);

	// 2. Если на крутом горном рельефе прямой путь <= 15‰ невозможен, делаем fallback
	// с допуском горных/тоннельных участков (с высокими надбавками НЦС за мосты/тоннели)
	if (!Result.bFound)
	{
		FRailCategoryNorms RelaxedNorms = Norms;
		RelaxedNorms.MaxSlopePermille = 150.0f;
		PathFinder->SetCategoryNorms(RelaxedNorms);
		Result = PathFinder->FindPath(CachedGrid, StartIdx, EndIdx);
		PathFinder->SetCategoryNorms(Norms);
	}

	const double Ms = (FPlatformTime::Seconds() - T0) * 1000.0;

	if (!Result.bFound)
	{
		PathRenderer->ClearPath();
		if (CostSummaryWidget)
		{
			CostSummaryWidget->UpdateRouteSummary(Norms.CategoryName, 0.0, 0.0, 0.0f, 0.0f, Ms);
		}
		OnPathRecomputed.Broadcast(0.0, 0.0, Ms, 0);
		return;
	}

	// 3. Строим трассу с вписыванием стандартных радиусов СП 119.13330 и вертикальных кривых
	float			MinPlanRadius_m = 0.0f;
	float			MaxSlopePermille = 0.0f;
	TArray<FVector> SmoothPoints = GenerateRailwayArcs(CachedGrid, Result, Norms, MinPlanRadius_m, MaxSlopePermille);

	// 4. Валидация по СП 119.13330
	if (PathValidator)
	{
		const FRailValidationResult ValRes = PathValidator->ValidatePath(CachedGrid, Result, Norms);
		if (!ValRes.bIsValid && ValRes.Violations.Num() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RailGM] Обнаружено отступлений от норм (требуется ТЭО/ИССО): %d. Пример: %s"),
				ValRes.Violations.Num(), *ValRes.Violations[0].Message);
		}
	}

	// 5. Раскраска трассы по реальной удельной стоимости километра (НЦС 81-02-07-2025)
	if (SmoothPoints.Num() > 1)
	{
		TArray<FLinearColor> Colors;
		Colors.SetNumUninitialized(SmoothPoints.Num());

		const float ZScale = (PathRenderer && PathRenderer->WorldZScale > 0.0f) ? PathRenderer->WorldZScale : 1.0f;

		for (int32 i = 0; i < SmoothPoints.Num(); ++i)
		{
			const int32 PrevIdx = FMath::Max(0, i - 1);
			const int32 NextIdx = FMath::Min(SmoothPoints.Num() - 1, i + 1);

			const FVector& A = SmoothPoints[PrevIdx];
			const FVector& B = SmoothPoints[NextIdx];

			const float DistXY_m = FVector2D::Distance(FVector2D(A.X, A.Y), FVector2D(B.X, B.Y)) * 0.01f;
			const float Dz_m = (FMath::Abs(B.Z - A.Z) * 0.01f) / ZScale;
			const float LocalSlopePermille = (DistXY_m > 0.1f) ? ((Dz_m / DistXY_m) * 1000.0f) : 0.0f;

			if (RuntimeSettings.bColorPathByCost && CostCalculator)
			{
				const double LocalRate_kRub = CostCalculator->CalculateRatePerKm_kRub(
					LocalSlopePermille, MinPlanRadius_m, ERailGroundType::Loam);

				const float T = FMath::Clamp(
					static_cast<float>((LocalRate_kRub - UCostCalculator::MinColorRate_kRub) /
						(UCostCalculator::MaxColorRate_kRub - UCostCalculator::MinColorRate_kRub)),
					0.0f, 1.0f);

				Colors[i] = PathRenderer->EvaluateColor(T);
			}
			else
			{
				const float T = static_cast<float>(i) / static_cast<float>(SmoothPoints.Num() - 1);
				Colors[i] = PathRenderer->EvaluateColor(T);
			}
		}

		PathRenderer->RenderCustomPath(SmoothPoints, Colors);
	}
	else
	{
		PathRenderer->ClearPath();
	}

	// 6. Обновляем UI-табличку со сметой НЦС 81-02-07-2025
	if (CostSummaryWidget)
	{
		CostSummaryWidget->UpdateRouteSummary(
			Norms.CategoryName,
			Result.TotalLength_m,
			Result.TotalCost,
			MaxSlopePermille,
			MinPlanRadius_m,
			Ms);
	}

	UE_LOG(LogTemp, Log,
		TEXT("[RailGM] Путь [%s]: длина=%.0f м, смета=%.1f млн руб., макс.уклон=%.1f‰, мин.R=%.0f м, время=%.1f мс."),
		*Norms.CategoryName, Result.TotalLength_m, Result.TotalCost / 1000.0, MaxSlopePermille, MinPlanRadius_m, Ms);

	OnPathRecomputed.Broadcast(Result.TotalLength_m, Result.TotalCost, Ms, Result.NodeIndices.Num());
}

TArray<FVector> ARailOptimaGameMode::GenerateRailwayArcs(
	const FRailGridData&	  InGrid,
	const FRailPathResult&	  InPath,
	const FRailCategoryNorms& InNorms,
	float&					  OutMinPlanRadius_m,
	float&					  OutMaxSlopePermille) const
{
	TArray<FVector> OutPoints;
	OutMinPlanRadius_m = 0.0f;
	OutMaxSlopePermille = 0.0f;

	if (!InPath.bFound || InPath.NodeIndices.Num() < 2)
		return OutPoints;

	const FVector Origin = PathRenderer ? PathRenderer->GridWorldOrigin : FVector::ZeroVector;

	// 1. Сбор сырых точек в мировых координатах (см)
	TArray<FVector2D> RawPoints;
	RawPoints.Reserve(InPath.NodeIndices.Num());
	for (int32 Idx : InPath.NodeIndices)
	{
		const auto& N = InGrid.Nodes[Idx];
		RawPoints.Add(FVector2D(
			Origin.X + N.X * InGrid.StepSize_m * 100.0f,
			Origin.Y + N.Y * InGrid.StepSize_m * 100.0f));
	}

	// 2. Упрощение ломаной (алгоритм Рамера — Дугласа — Пекера)
	const float		  RDP_Tolerance = 15000.0f; // 150 м
	TArray<FVector2D> Simplified;
	Simplified.Add(RawPoints[0]);

	TFunction<void(int32, int32)> RDP = [&](int32 start, int32 end) {
		float	  maxDist = 0.0f;
		int32	  index = -1;
		FVector2D startPt = RawPoints[start];
		FVector2D endPt = RawPoints[end];
		for (int32 i = start + 1; i < end; ++i)
		{
			FVector2D pt = RawPoints[i];
			float	  d = FMath::Abs((endPt.X - startPt.X) * (startPt.Y - pt.Y) - (startPt.X - pt.X) * (endPt.Y - startPt.Y));
			float	  len = FVector2D::Distance(startPt, endPt);
			if (len > 0.0f)
				d /= len;
			if (d > maxDist)
			{
				maxDist = d;
				index = i;
			}
		}
		if (maxDist > RDP_Tolerance && index != -1)
		{
			RDP(start, index);
			RDP(index, end);
		}
		else
		{
			if (!Simplified.Last().Equals(endPt, 1.0f))
				Simplified.Add(endPt);
		}
	};
	RDP(0, RawPoints.Num() - 1);

	// 3. Вписывание круговых кривых в плане по стандартному ряду СП 119.13330
	TArray<FVector2D> Final2D;
	Final2D.Add(Simplified[0]);

	float BestMinRadius_m = 999999.0f;

	for (int32 i = 1; i < Simplified.Num() - 1; ++i)
	{
		const FVector2D PPrev = Simplified[i - 1];
		const FVector2D PCurr = Simplified[i];
		const FVector2D PNext = Simplified[i + 1];

		const FVector2D V1 = (PCurr - PPrev).GetSafeNormal();
		const FVector2D V2 = (PNext - PCurr).GetSafeNormal();
		const float		Dot = FVector2D::DotProduct(V1, V2);

		bool bArcAdded = false;

		if (Dot < 0.999f) // Есть угол поворота в плане
		{
			const float TurnAngleRad = FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f));
			const float HalfTurnTan = FMath::Max(FMath::Tan(TurnAngleRad * 0.5f), 0.001f);

			const float Dist1_cm = FVector2D::Distance(PPrev, PCurr);
			const float Dist2_cm = FVector2D::Distance(PCurr, PNext);
			const float MaxTangent_cm = FMath::Min(Dist1_cm, Dist2_cm) * 0.48f;

			// Максимально помещающийся геометрический радиус (в метрах)
			const float MaxFittingRadius_m = (MaxTangent_cm / HalfTurnTan) * 0.01f;

			// Выбираем радиус из стандартного ряда СП 119.13330 (4000, 3000, 2500 ... 180 м)
			const float SelectedStandardRadius_m = InNorms.PickStandardRadius(MaxFittingRadius_m);
			const float R_cm = FMath::Min(SelectedStandardRadius_m, MaxFittingRadius_m) * 100.0f;
			const float T_cm = R_cm * HalfTurnTan;

			if (R_cm >= 15000.0f && T_cm <= MaxTangent_cm + 1.0f) // от 150 м и выше
			{
				BestMinRadius_m = FMath::Min(BestMinRadius_m, R_cm * 0.01f);

				if (SelectedStandardRadius_m < 300.0f)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[ArcGen] Малый радиус R=%.0f м (< 300 м): по СП 119.13330 требуется ТЭО и ограничение скорости!"),
						SelectedStandardRadius_m);
				}

				const FVector2D T1 = PCurr - V1 * T_cm;
				const FVector2D T2 = PCurr + V2 * T_cm;

				if (FVector2D::Distance(Final2D.Last(), T1) > 10.0f)
				{
					Final2D.Add(T1);
				}

				// Находим центр дуги через нормаль в сторону поворота
				const float		CrossZ = V1.X * V2.Y - V1.Y * V2.X;
				const float		Sign = (CrossZ >= 0.0f) ? 1.0f : -1.0f;
				const FVector2D PerpV1(-V1.Y * Sign, V1.X * Sign);
				const FVector2D Center = T1 + PerpV1 * R_cm;

				const float StartAngle = FMath::Atan2(T1.Y - Center.Y, T1.X - Center.X);
				const float EndAngle = FMath::Atan2(T2.Y - Center.Y, T2.X - Center.X);
				float		DeltaAngle = EndAngle - StartAngle;

				while (DeltaAngle > PI)
					DeltaAngle -= 2.0f * PI;
				while (DeltaAngle < -PI)
					DeltaAngle += 2.0f * PI;

				const int32 NumSteps = FMath::Clamp(FMath::CeilToInt(FMath::Abs(DeltaAngle) * R_cm / 2000.0f), 6, 48);
				for (int32 s = 1; s <= NumSteps; ++s)
				{
					const float a = StartAngle + DeltaAngle * (static_cast<float>(s) / static_cast<float>(NumSteps));
					Final2D.Add(Center + FVector2D(FMath::Cos(a), FMath::Sin(a)) * R_cm);
				}
				bArcAdded = true;
			}
		}

		if (!bArcAdded && FVector2D::Distance(Final2D.Last(), PCurr) > 1.0f)
		{
			Final2D.Add(PCurr);
		}
	}

	if (FVector2D::Distance(Final2D.Last(), Simplified.Last()) > 1.0f)
	{
		Final2D.Add(Simplified.Last());
	}

	OutMinPlanRadius_m = (BestMinRadius_m < 900000.0f) ? BestMinRadius_m : 0.0f;

	// 4. Построение продольного профиля Z и сглаживание вертикальными кривыми (Rv, Tv)
	const float	  ZOff_cm = PathRenderer ? PathRenderer->ZOffset_cm : 8000.0f;
	const float	  ZScale = (PathRenderer && PathRenderer->WorldZScale > 0.0f) ? PathRenderer->WorldZScale : 1.0f;
	TArray<float> ProfileZ_cm;
	ProfileZ_cm.SetNumUninitialized(Final2D.Num());

	for (int32 i = 0; i < Final2D.Num(); ++i)
	{
		float Z = 0.0f;
		if (Terrain)
		{
			Terrain->SampleTerrainZ_cm(Final2D[i], Z);
		}
		ProfileZ_cm[i] = Z;
	}

	// Ограничение продольного уклона проектной линии (земляное полотно: насыпи/выемки до MaxSlopePermille)
	// и сглаживание переломов профиля вертикальными кривыми радиуса MinVerticalRadius
	if (ProfileZ_cm.Num() >= 3)
	{
		TArray<float> SmoothedZ = ProfileZ_cm;
		const int32	  NumPasses = 3;
		for (int32 Pass = 0; Pass < NumPasses; ++Pass)
		{
			for (int32 i = 1; i < ProfileZ_cm.Num() - 1; ++i)
			{
				const float DistPrev_m = FVector2D::Distance(Final2D[i - 1], Final2D[i]) * 0.01f;
				const float DistNext_m = FVector2D::Distance(Final2D[i], Final2D[i + 1]) * 0.01f;
				if (DistPrev_m < 1.0f || DistNext_m < 1.0f)
					continue;

				const float Slope1_perm = (((SmoothedZ[i] - SmoothedZ[i - 1]) * 0.01f / ZScale) / DistPrev_m) * 1000.0f;
				const float Slope2_perm = (((SmoothedZ[i + 1] - SmoothedZ[i]) * 0.01f / ZScale) / DistNext_m) * 1000.0f;
				const float DeltaI = FMath::Abs(Slope2_perm - Slope1_perm);

				// Если разность уклонов Δi >= порога (2.0‰), вписываем вертикальную кривую
				if (DeltaI >= InNorms.MinSlopeDeltaForVerticalCurve)
				{
					SmoothedZ[i] = 0.25f * SmoothedZ[i - 1] + 0.5f * SmoothedZ[i] + 0.25f * SmoothedZ[i + 1];
				}
			}
		}
		ProfileZ_cm = MoveTemp(SmoothedZ);
	}

	// Подсчет максимального фактического уклона на построенном профиле
	for (int32 i = 1; i < Final2D.Num(); ++i)
	{
		const float Dist_m = FVector2D::Distance(Final2D[i - 1], Final2D[i]) * 0.01f;
		if (Dist_m > 1.0f)
		{
			const float Dz_m = (FMath::Abs(ProfileZ_cm[i] - ProfileZ_cm[i - 1]) * 0.01f) / ZScale;
			const float SlopePermille = (Dz_m / Dist_m) * 1000.0f;
			OutMaxSlopePermille = FMath::Max(OutMaxSlopePermille, SlopePermille);
		}
	}

	OutPoints.Reserve(Final2D.Num());
	for (int32 i = 0; i < Final2D.Num(); ++i)
	{
		OutPoints.Add(FVector(Final2D[i].X, Final2D[i].Y, ProfileZ_cm[i] + ZOff_cm));
	}

	return OutPoints;
}

FRailGridData ARailOptimaGameMode::BuildGridFromTerrain(const AProceduralTerrain& InTerrain, float InTargetStep_m) const
{
	const FRailHeightMapData& MapData = InTerrain.GetHeightMapData();
	FRailGridData			  Grid;
	if (MapData.Width <= 1 || MapData.Height <= 1 || MapData.Heights.Num() == 0)
		return Grid;

	const int32 GridW = FMath::Max(2, FMath::RoundToInt(InTerrain.GetMapSizeMeters() / FMath::Max(InTargetStep_m, 1.0f)));
	const float ActualStep_m = InTerrain.GetMapSizeMeters() / static_cast<float>(GridW - 1);
	const float ElevRange = FMath::Max(MapData.MaxElevation_m - MapData.MinElevation_m, 1.0f);

	Grid.Width = GridW;
	Grid.Height = GridW;
	Grid.StepSize_m = ActualStep_m;
	Grid.Nodes.SetNum(GridW * GridW);

	const float ScaleX = static_cast<float>(MapData.Width - 1) / FMath::Max(GridW - 1, 1);
	const float ScaleY = static_cast<float>(MapData.Height - 1) / FMath::Max(GridW - 1, 1);

	for (int32 Y = 0; Y < GridW; ++Y)
	{
		for (int32 X = 0; X < GridW; ++X)
		{
			const int32	   SrcX = FMath::Clamp(FMath::RoundToInt(X * ScaleX), 0, MapData.Width - 1);
			const int32	   SrcY = FMath::Clamp(FMath::RoundToInt(Y * ScaleY), 0, MapData.Height - 1);
			FRailGridNode& Node = Grid.Nodes[Grid.IndexOf(X, Y)];
			Node.X = X;
			Node.Y = Y;
			Node.Height_m = MapData.MinElevation_m + MapData.Heights[SrcY * MapData.Width + SrcX] * ElevRange;
			Node.GroundType = ERailGroundType::Loam;
			Node.bIsForbidden = false;
		}
	}
	return Grid;
}

FRailGridData ARailOptimaGameMode::GenerateSyntheticGrid(int32 InSize, float InStepSize_m) const
{
	FRailGridData Grid;
	const int32	  ClampedSize = FMath::Max(2, InSize);
	Grid.Width = ClampedSize;
	Grid.Height = ClampedSize;
	Grid.StepSize_m = FMath::Max(1.0f, InStepSize_m);
	Grid.Nodes.SetNum(ClampedSize * ClampedSize);

	for (int32 Y = 0; Y < ClampedSize; ++Y)
	{
		for (int32 X = 0; X < ClampedSize; ++X)
		{
			FRailGridNode& Node = Grid.Nodes[Grid.IndexOf(X, Y)];
			Node.X = X;
			Node.Y = Y;
			Node.Height_m = 100.0f;
			Node.GroundType = ERailGroundType::Loam;
			Node.bIsForbidden = false;
		}
	}
	return Grid;
}

int32 ARailOptimaGameMode::WorldToNodeIndex(const FRailGridData& InGrid, const FVector& InWorldPos) const
{
	if (!PathRenderer || InGrid.Width < 2)
		return INDEX_NONE;
	const FVector Origin = PathRenderer->GridWorldOrigin;
	const float	  Step_cm = InGrid.StepSize_m * 100.0f;
	const int32	  X = FMath::Clamp(FMath::RoundToInt((InWorldPos.X - Origin.X) / Step_cm), 0, InGrid.Width - 1);
	const int32	  Y = FMath::Clamp(FMath::RoundToInt((InWorldPos.Y - Origin.Y) / Step_cm), 0, InGrid.Height - 1);
	return InGrid.IndexOf(X, Y);
}

FVector ARailOptimaGameMode::NodeIndexToWorldXY(const FRailGridData& InGrid, int32 InNodeIndex) const
{
	if (!InGrid.Nodes.IsValidIndex(InNodeIndex))
		return FVector::ZeroVector;

	const FVector		 Origin = PathRenderer ? PathRenderer->GridWorldOrigin : FVector::ZeroVector;
	const FRailGridNode& Node = InGrid.Nodes[InNodeIndex];
	return FVector(
		Origin.X + Node.X * InGrid.StepSize_m * 100.0f,
		Origin.Y + Node.Y * InGrid.StepSize_m * 100.0f,
		Node.Height_m * 100.0f);
}

void ARailOptimaGameMode::HandleEndpointMoved(ARailEndpoint* /*Endpoint*/)
{
	if (RecomputeDebounceSec <= 0.0f)
	{
		RecomputePath();
		return;
	}
	GetWorldTimerManager().ClearTimer(RecomputeTimer);
	GetWorldTimerManager().SetTimer(RecomputeTimer, this, &ARailOptimaGameMode::HandleDeferredRecompute, RecomputeDebounceSec, false);
}

void ARailOptimaGameMode::HandleDeferredRecompute()
{
	RecomputePath();
}