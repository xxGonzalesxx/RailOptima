// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Модульные автоматизированные тесты RailOptima (Unreal Engine Automation Testing)

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Algorithms/AStarPathFinder.h"
#include "Algorithms/CostCalculator.h"
#include "Algorithms/RailCategories.h"
#include "Algorithms/RailPathValidator.h"

// Вспомогательная функция генерации тестовой плоской сетки
static FRailGridData CreateTestGrid(int32 Width, int32 Height, float Step_m = 50.0f)
{
	FRailGridData Grid;
	Grid.Width = Width;
	Grid.Height = Height;
	Grid.StepSize_m = Step_m;
	Grid.Nodes.SetNum(Width * Height);

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const int32 Idx = Grid.IndexOf(X, Y);
			Grid.Nodes[Idx].X = X;
			Grid.Nodes[Idx].Y = Y;
			Grid.Nodes[Idx].Height_m = 100.0f; // Плоский рельеф
			Grid.Nodes[Idx].GroundType = ERailGroundType::Loam;
			Grid.Nodes[Idx].bIsForbidden = false;
			Grid.Nodes[Idx].PriorityModifier = 1.0f;
		}
	}
	return Grid;
}

// ─────────────────────────────────────────────────────────────────
// 1. Тест: Отсутствие острых углов в трассе A* (<= 50 градусов за шаг)
// ─────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailOptimaAStarNoAcuteAnglesTest, "RailOptima.Algorithms.AStarNoAcuteAngles", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FRailOptimaAStarNoAcuteAnglesTest::RunTest(const FString& Parameters)
{
	FRailGridData Grid = CreateTestGrid(40, 40, 50.0f);

	UAStarPathFinder* PathFinder = NewObject<UAStarPathFinder>();
	UCostCalculator* CostCalc = NewObject<UCostCalculator>();
	PathFinder->SetCostCalculator(CostCalc);

	const FRailCategoryNorms Norms = FRailCategoryNorms::GetNorms(ERailRoadCategory::Category1);
	PathFinder->SetCategoryNorms(Norms);

	// Диагональный маршрут из (2, 2) в (35, 30)
	const int32 StartIdx = Grid.IndexOf(2, 2);
	const int32 EndIdx = Grid.IndexOf(35, 30);

	const FRailPathResult Result = PathFinder->FindPath(Grid, StartIdx, EndIdx);
	TestTrue(TEXT("Путь должен быть успешно найден"), Result.bFound);
	TestTrue(TEXT("В пути должно быть более 5 узлов"), Result.NodeIndices.Num() > 5);

	// Проверяем все углы между последовательными ребрами
	bool bHasAcuteOrRightAngle = false;
	for (int32 i = 2; i < Result.NodeIndices.Num(); ++i)
	{
		const FRailGridNode& N0 = Grid.Nodes[Result.NodeIndices[i - 2]];
		const FRailGridNode& N1 = Grid.Nodes[Result.NodeIndices[i - 1]];
		const FRailGridNode& N2 = Grid.Nodes[Result.NodeIndices[i]];

		const FVector2D V1(N1.X - N0.X, N1.Y - N0.Y);
		const FVector2D V2(N2.X - N1.X, N2.Y - N1.Y);

		const float Dot = FVector2D::DotProduct(V1.GetSafeNormal(), V2.GetSafeNormal());
		const float TurnAngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));

		if (TurnAngleDeg > 51.0f)
		{
			bHasAcuteOrRightAngle = true;
			AddError(FString::Printf(TEXT("Обнаружен недопустимый угол поворота: %.1f° между узлами %d и %d"), TurnAngleDeg, i - 1, i));
		}
	}

	TestFalse(TEXT("Трасса не должна содержать резких изломов > 50° за шаг"), bHasAcuteOrRightAngle);
	return true;
}

// ─────────────────────────────────────────────────────────────────
// 2. Тест: Корректный обход запретных (красных) зон
// ─────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailOptimaAStarForbiddenZonesTest, "RailOptima.Algorithms.AStarForbiddenZones", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FRailOptimaAStarForbiddenZonesTest::RunTest(const FString& Parameters)
{
	FRailGridData Grid = CreateTestGrid(50, 50, 50.0f);

	// Создаем запретную стену посередине карты от Y=10 до Y=40 на X=25
	for (int32 Y = 10; Y <= 40; ++Y)
	{
		Grid.Nodes[Grid.IndexOf(25, Y)].bIsForbidden = true;
	}

	UAStarPathFinder* PathFinder = NewObject<UAStarPathFinder>();
	UCostCalculator* CostCalc = NewObject<UCostCalculator>();
	PathFinder->SetCostCalculator(CostCalc);

	const int32 StartIdx = Grid.IndexOf(10, 25);
	const int32 EndIdx = Grid.IndexOf(40, 25);

	const FRailPathResult Result = PathFinder->FindPath(Grid, StartIdx, EndIdx);
	TestTrue(TEXT("Путь в обход запретной зоны должен быть найден"), Result.bFound);

	// Проверяем, что ни один узел пути не лежит в запретной зоне
	bool bTraversedForbiddenNode = false;
	for (int32 Idx : Result.NodeIndices)
	{
		if (Grid.Nodes[Idx].bIsForbidden)
		{
			bTraversedForbiddenNode = true;
			AddError(FString::Printf(TEXT("Узел пути (%d, %d) находится в запретной зоне!"), Grid.Nodes[Idx].X, Grid.Nodes[Idx].Y));
		}
	}

	TestFalse(TEXT("Путь не должен проходить через запретные узлы"), bTraversedForbiddenNode);
	return true;
}

// ─────────────────────────────────────────────────────────────────
// 3. Тест: Приоритетные (зеленые) зоны коридора
// ─────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailOptimaPriorityZonesTest, "RailOptima.Algorithms.PriorityZones", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FRailOptimaPriorityZonesTest::RunTest(const FString& Parameters)
{
	FRailGridData Grid = CreateTestGrid(40, 40, 50.0f);

	// Задаем зеленый приоритетный коридор по Y=25 (сильное удешевление)
	for (int32 X = 5; X <= 35; ++X)
	{
		Grid.Nodes[Grid.IndexOf(X, 25)].PriorityModifier = 0.2f;
	}

	UAStarPathFinder* PathFinder = NewObject<UAStarPathFinder>();
	UCostCalculator* CostCalc = NewObject<UCostCalculator>();
	PathFinder->SetCostCalculator(CostCalc);

	const int32 StartIdx = Grid.IndexOf(5, 20);
	const int32 EndIdx = Grid.IndexOf(35, 20);

	const FRailPathResult Result = PathFinder->FindPath(Grid, StartIdx, EndIdx);
	TestTrue(TEXT("Путь с приоритетным коридором найден"), Result.bFound);

	// Проверяем, что трасса отклонилась в сторону зеленого коридора (Y=25)
	int32 PriorityNodesCount = 0;
	for (int32 Idx : Result.NodeIndices)
	{
		if (Grid.Nodes[Idx].Y == 25)
		{
			++PriorityNodesCount;
		}
	}

	TestTrue(TEXT("Трасса должна использовать приоритетный коридор"), PriorityNodesCount > 5);
	return true;
}

// ─────────────────────────────────────────────────────────────────
// 4. Тест: Соответствие нормативной базы СП 119.13330
// ─────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailOptimaNormsValidationTest, "RailOptima.Norms.StandardRadiiAndSlopes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FRailOptimaNormsValidationTest::RunTest(const FString& Parameters)
{
	// 1. Проверка I Категории
	const FRailCategoryNorms Cat1 = FRailCategoryNorms::GetNorms(ERailRoadCategory::Category1);
	TestEqual(TEXT("I Категория: руководящий уклон 15‰"), Cat1.MaxSlopePermille, 15.0f);
	TestTrue(TEXT("I Категория: подбор стандартного радиуса 2500 м"), Cat1.PickStandardRadius(2600.0f) == 2500.0f);
	TestTrue(TEXT("I Категория: подбор стандартного радиуса 1200 м"), Cat1.PickStandardRadius(1350.0f) == 1200.0f);

	// 2. Проверка ВСМ (Высокоскоростной)
	const FRailCategoryNorms VSM = FRailCategoryNorms::GetNorms(ERailRoadCategory::HighSpeed);
	TestTrue(TEXT("ВСМ: рекомендуемый радиус в плане >= 4000 м"), VSM.RecommendedRadii.Num() > 0 && VSM.RecommendedRadii[0] >= 4000.0f);
	TestTrue(TEXT("ВСМ: радиус в трудных условиях >= 2500 м"), VSM.MinRadiusDifficult >= 2500.0f);

	// 3. Проверка подъездных путей
	const FRailCategoryNorms Access = FRailCategoryNorms::GetNorms(ERailRoadCategory::Category4Access);
	TestTrue(TEXT("Подъездные пути: допустимый уклон 15‰"), Access.MaxSlopePermille == 15.0f);
	TestTrue(TEXT("Подъездные пути: минимальный радиус в особо трудных условиях <= 250 м"), Access.MinRadiusVeryDifficult <= 250.0f);

	return true;
}

// ─────────────────────────────────────────────────────────────────
// 5. Тест: Расчет сметной стоимости по НЦС 81-02-07-2025
// ─────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailOptimaCostCalculatorTest, "RailOptima.Cost.NCSCalculation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FRailOptimaCostCalculatorTest::RunTest(const FString& Parameters)
{
	UCostCalculator* CostCalc = NewObject<UCostCalculator>();

	// Стоимость ровного 1 км пути (уклон 0%, поворот 0°, обычный грунт)
	const float BaseCost1Km = CostCalc->CalculateEdgeCost(1000.0f, 0.0f, 0.0f, 1.0f);
	TestTrue(TEXT("Стоимость 1 км пути должна быть положительной"), BaseCost1Km > 0.0f);

	// Путь с уклоном 15‰ должен стоить дороже ровного (надбавка за насыпи/выемки)
	const float SlopedCost1Km = CostCalc->CalculateEdgeCost(1000.0f, 1.5f, 0.0f, 1.0f);
	TestTrue(TEXT("Стоимость пути с уклоном должна превышать ровный участок"), SlopedCost1Km > BaseCost1Km);

	// Скальный грунт дороже суглинка по НЦС
	const float RockCoef = CostCalc->GetGroundCoefficient(ERailGroundType::Rock);
	const float LoamCoef = CostCalc->GetGroundCoefficient(ERailGroundType::Loam);
	TestTrue(TEXT("Коэффициент скального грунта выше суглинка"), RockCoef > LoamCoef);

	return true;
}
