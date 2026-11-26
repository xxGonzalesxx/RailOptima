// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#include "Algorithms/AStarPathFinder.h"

// ─────────────────────────────────────────────────────────────────
//  Статические константы
// ─────────────────────────────────────────────────────────────────

const int32 UAStarPathFinder::DirectionX[UAStarPathFinder::NumDirections] = { 0, 1, 0, -1, 1, 1, -1, -1 };
const int32 UAStarPathFinder::DirectionY[UAStarPathFinder::NumDirections] = { -1, 0, 1, 0, -1, 1, 1, -1 };

// ─────────────────────────────────────────────────────────────────
//  Бинарный heap
// ─────────────────────────────────────────────────────────────────

namespace
{
	void HeapSwap(TArray<int32>& InHeap, TArray<int32>& InPositions, int32 InA, int32 InB)
	{
		const int32 NodeA = InHeap[InA];
		const int32 NodeB = InHeap[InB];
		InHeap[InA] = NodeB;
		InHeap[InB] = NodeA;
		InPositions[NodeA] = InB;
		InPositions[NodeB] = InA;
	}

	void HeapSiftUp(TArray<int32>& InHeap, TArray<int32>& InPositions, int32 InIndex, const TArray<float>& InFScores)
	{
		while (InIndex > 0)
		{
			const int32 Parent = (InIndex - 1) / 2;
			if (InFScores[InHeap[InIndex]] >= InFScores[InHeap[Parent]])
				break;
			HeapSwap(InHeap, InPositions, InIndex, Parent);
			InIndex = Parent;
		}
	}

	void HeapSiftDown(TArray<int32>& InHeap, TArray<int32>& InPositions, int32 InIndex, const TArray<float>& InFScores)
	{
		const int32 Count = InHeap.Num();
		while (true)
		{
			const int32 Left = InIndex * 2 + 1;
			const int32 Right = Left + 1;
			int32		Smallest = InIndex;

			if (Left < Count && InFScores[InHeap[Left]] < InFScores[InHeap[Smallest]])
				Smallest = Left;
			if (Right < Count && InFScores[InHeap[Right]] < InFScores[InHeap[Smallest]])
				Smallest = Right;
			if (Smallest == InIndex)
				break;

			HeapSwap(InHeap, InPositions, InIndex, Smallest);
			InIndex = Smallest;
		}
	}

	void HeapPush(TArray<int32>& InHeap, TArray<int32>& InPositions, int32 InNode, const TArray<float>& InFScores)
	{
		InPositions[InNode] = InHeap.Num();
		InHeap.Add(InNode);
		HeapSiftUp(InHeap, InPositions, InHeap.Num() - 1, InFScores);
	}

	int32 HeapPop(TArray<int32>& InHeap, TArray<int32>& InPositions, const TArray<float>& InFScores)
	{
		const int32 Top = InHeap[0];
		InPositions[Top] = INDEX_NONE;

		const int32 LastIndex = InHeap.Num() - 1;
		if (LastIndex > 0)
		{
			InHeap[0] = InHeap[LastIndex];
			InPositions[InHeap[0]] = 0;
		}
		InHeap.Pop(EAllowShrinking::No);

		if (InHeap.Num() > 0)
		{
			HeapSiftDown(InHeap, InPositions, 0, InFScores);
		}
		return Top;
	}

	void HeapUpdate(TArray<int32>& InHeap, TArray<int32>& InPositions, int32 InNode, const TArray<float>& InFScores)
	{
		const int32 Index = InPositions[InNode];
		if (Index == INDEX_NONE)
			return;
		HeapSiftUp(InHeap, InPositions, Index, InFScores);
		HeapSiftDown(InHeap, InPositions, InPositions[InNode], InFScores);
	}
} // namespace

// ─────────────────────────────────────────────────────────────────
//  Поиск пути
// ─────────────────────────────────────────────────────────────────

FRailPathResult UAStarPathFinder::FindPath(
	const FRailGridData& InGrid,
	int32				 InStartIndex,
	int32				 InEndIndex,
	int32				 InMaxExpansions)
{
	FRailPathResult Result;

	if (!CostCalculator)
	{
		CostCalculator = NewObject<UCostCalculator>(this);
	}

	const int32 NumNodes = InGrid.Nodes.Num();
	if (NumNodes == 0 || !InGrid.Nodes.IsValidIndex(InStartIndex) || !InGrid.Nodes.IsValidIndex(InEndIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("[AStar] Некорректные индексы: %d -> %d (всего %d)."),
			InStartIndex, InEndIndex, NumNodes);
		return Result;
	}

	const FRailGridNode& StartNode = InGrid.Nodes[InStartIndex];
	const FRailGridNode& EndNode = InGrid.Nodes[InEndIndex];

	if (StartNode.bIsForbidden || EndNode.bIsForbidden)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AStar] Старт или финиш в запретной зоне."));
		return Result;
	}

	if (InStartIndex == InEndIndex)
	{
		Result.bFound = true;
		Result.NodeIndices.Add(InStartIndex);
		return Result;
	}

	if (CachedGScore.Num() != NumNodes)
	{
		CachedGScore.SetNumUninitialized(NumNodes);
		CachedFScore.SetNumUninitialized(NumNodes);
		CachedCameFrom.SetNumUninitialized(NumNodes);
		CachedNodeState.SetNumUninitialized(NumNodes);
		CachedHeapPositions.SetNumUninitialized(NumNodes);
	}

	for (int32 i = 0; i < NumNodes; ++i)
	{
		CachedGScore[i] = TNumericLimits<float>::Max();
		CachedFScore[i] = TNumericLimits<float>::Max();
		CachedCameFrom[i] = INDEX_NONE;
		CachedNodeState[i] = 0;
		CachedHeapPositions[i] = INDEX_NONE;
	}

	CachedOpenHeap.Reset();
	CachedOpenHeap.Reserve(4096);

	TArray<float>& GScore = CachedGScore;
	TArray<float>& FScore = CachedFScore;
	TArray<int32>& CameFrom = CachedCameFrom;
	TArray<int8>&  NodeState = CachedNodeState;
	TArray<int32>& OpenHeap = CachedOpenHeap;
	TArray<int32>& HeapPositions = CachedHeapPositions;

	GScore[InStartIndex] = 0.0f;
	FScore[InStartIndex] = Heuristic(InGrid, InStartIndex, InEndIndex);
	HeapPush(OpenHeap, HeapPositions, InStartIndex, FScore);
	NodeState[InStartIndex] = 1;

	TArray<int32, TInlineAllocator<8>> Neighbors;
	int32							   Expansions = 0;

	while (OpenHeap.Num() > 0)
	{
		if (++Expansions > InMaxExpansions)
		{
			UE_LOG(LogTemp, Warning, TEXT("[AStar] Превышен лимит расширений (%d)."), InMaxExpansions);
			break;
		}

		const int32 CurrentIndex = HeapPop(OpenHeap, HeapPositions, FScore);

		if (CurrentIndex == InEndIndex)
		{
			Result.bFound = true;
			Result.ExpandedNodes = Expansions;
			ReconstructPath(InGrid, CameFrom, InStartIndex, InEndIndex, Result);
			return Result;
		}

		NodeState[CurrentIndex] = 2;

		Neighbors.Reset();
		CollectNeighbors(InGrid, CurrentIndex, Neighbors);

		for (const int32 NeighborIndex : Neighbors)
		{
			if (NodeState[NeighborIndex] == 2)
				continue;

			// Оцениваем ребро. Передаем историю пути, чтобы заблокировать острые углы
			const float EdgeCost = EvaluateEdge(InGrid, CurrentIndex, NeighborIndex, CameFrom);

			// ЖЕСТКОЕ ОГРАНИЧЕНИЕ: Если функция вернула < 0, значит маневр невозможен
			if (EdgeCost < 0.0f)
			{
				continue;
			}

			const float TentativeG = GScore[CurrentIndex] + EdgeCost;

			if (TentativeG < GScore[NeighborIndex])
			{
				CameFrom[NeighborIndex] = CurrentIndex;
				GScore[NeighborIndex] = TentativeG;
				FScore[NeighborIndex] = TentativeG + Heuristic(InGrid, NeighborIndex, InEndIndex);

				if (NodeState[NeighborIndex] == 1)
				{
					HeapUpdate(OpenHeap, HeapPositions, NeighborIndex, FScore);
				}
				else
				{
					HeapPush(OpenHeap, HeapPositions, NeighborIndex, FScore);
					NodeState[NeighborIndex] = 1;
				}
			}
		}
	}

	Result.ExpandedNodes = Expansions;
	UE_LOG(LogTemp, Warning, TEXT("[AStar] Путь не найден. Расширений: %d."), Expansions);
	return Result;
}

void UAStarPathFinder::SetCostCalculator(UCostCalculator* InCalculator)
{
	CostCalculator = InCalculator;
}

// ─────────────────────────────────────────────────────────────────
//  Внутренняя логика
// ─────────────────────────────────────────────────────────────────

void UAStarPathFinder::CollectNeighbors(
	const FRailGridData&				InGrid,
	int32								InNodeIndex,
	TArray<int32, TInlineAllocator<8>>& OutNeighbors) const
{
	const FRailGridNode& Node = InGrid.Nodes[InNodeIndex];
	const float			 MaxSlopePermille = CurrentNorms.MaxSlopePermille;

	for (int32 i = 0; i < NumDirections; ++i)
	{
		const int32 NX = Node.X + DirectionX[i];
		const int32 NY = Node.Y + DirectionY[i];

		if (!InGrid.IsValid(NX, NY))
			continue;

		const int32 NeighborIndex = InGrid.IndexOf(NX, NY);
		if (InGrid.Nodes[NeighborIndex].bIsForbidden)
			continue;

		// Блокируем диагональный срез, если соседние прямые узлы запрещены
		if (DirectionX[i] != 0 && DirectionY[i] != 0)
		{
			const int32 SideA = InGrid.IndexOf(Node.X + DirectionX[i], Node.Y);
			const int32 SideB = InGrid.IndexOf(Node.X, Node.Y + DirectionY[i]);

			if (InGrid.Nodes[SideA].bIsForbidden || InGrid.Nodes[SideB].bIsForbidden)
				continue;
		}

		// ── Проверка уклона по нормативам категории ─────────────
		const bool	bDiagonal = (DirectionX[i] != 0 && DirectionY[i] != 0);
		const float EdgeLength_m = bDiagonal
			? InGrid.StepSize_m * 1.41421356f
			: InGrid.StepSize_m;

		const FRailGridNode& NeighborNode = InGrid.Nodes[NeighborIndex];
		const float			 HeightDiff_m = FMath::Abs(NeighborNode.Height_m - Node.Height_m);
		const float			 SlopePermille = (HeightDiff_m / EdgeLength_m) * 1000.0f;

		if (SlopePermille > MaxSlopePermille)
		{
			continue; // Уклон превышает норматив — ребро запрещено.
		}

		OutNeighbors.Add(NeighborIndex);
	}
}

void UAStarPathFinder::ReconstructPath(
	const FRailGridData& InGrid,
	const TArray<int32>& InCameFrom,
	int32				 InStartIndex,
	int32				 InEndIndex,
	FRailPathResult&	 OutResult) const
{
	int32 Current = InEndIndex;
	while (Current != INDEX_NONE)
	{
		OutResult.NodeIndices.Add(Current);
		if (Current == InStartIndex)
			break;
		Current = InCameFrom[Current];
	}
	Algo::Reverse(OutResult.NodeIndices);

	const int32 NumEdges = FMath::Max(0, OutResult.NodeIndices.Num() - 1);
	OutResult.EdgeCosts.Reserve(NumEdges);
	OutResult.EdgeLengths.Reserve(NumEdges);

	for (int32 i = 1; i < OutResult.NodeIndices.Num(); ++i)
	{
		const int32 PrevIndex = OutResult.NodeIndices[i - 1];
		const int32 CurrIndex = OutResult.NodeIndices[i];

		const FRailGridNode& PrevNode = InGrid.Nodes[PrevIndex];
		const FRailGridNode& CurrNode = InGrid.Nodes[CurrIndex];

		const int32 DX = FMath::Abs(CurrNode.X - PrevNode.X);
		const int32 DY = FMath::Abs(CurrNode.Y - PrevNode.Y);
		const bool	bDiagonal = (DX != 0 && DY != 0);

		const float EdgeLength_m = bDiagonal
			? InGrid.StepSize_m * FMath::Sqrt(2.0f)
			: InGrid.StepSize_m;

		const float SlopeRatio = FMath::Abs(CurrNode.Height_m - PrevNode.Height_m) / EdgeLength_m;

		const float TurnAngleDeg = (i >= 2)
			? [&]() -> float {
			const FRailGridNode& BeforeNode = InGrid.Nodes[OutResult.NodeIndices[i - 2]];
			const float			 PrevDirX = PrevNode.X - BeforeNode.X;
			const float			 PrevDirY = PrevNode.Y - BeforeNode.Y;
			const float			 CurrDirX = CurrNode.X - PrevNode.X;
			const float			 CurrDirY = CurrNode.Y - PrevNode.Y;

			const float Dot = PrevDirX * CurrDirX + PrevDirY * CurrDirY;
			const float LenA = FMath::Sqrt(PrevDirX * PrevDirX + PrevDirY * PrevDirY);
			const float LenB = FMath::Sqrt(CurrDirX * CurrDirX + CurrDirY * CurrDirY);
			if (LenA < KINDA_SMALL_NUMBER || LenB < KINDA_SMALL_NUMBER)
				return 0.0f;

			const float CosAngle = FMath::Clamp(Dot / (LenA * LenB), -1.0f, 1.0f);
			return FMath::RadiansToDegrees(FMath::Acos(CosAngle));
		}()
			: 0.0f;

		const float GroundCoef = CostCalculator->GetGroundCoefficient(CurrNode.GroundType);
		const float SlopePercent = SlopeRatio * 100.0f;

		const float EdgeCost = CostCalculator->CalculateEdgeCost(
			EdgeLength_m, SlopePercent, TurnAngleDeg, GroundCoef);

		OutResult.EdgeCosts.Add(EdgeCost);
		OutResult.EdgeLengths.Add(EdgeLength_m);
		OutResult.TotalLength_m += EdgeLength_m;
		OutResult.TotalCost += EdgeCost;
	}

	UE_LOG(LogTemp, Verbose,
		TEXT("[AStar] Reconstruct: узлов=%d, рёбер=%d, длина=%.1f м, стоимость=%.1f ед."),
		OutResult.NodeIndices.Num(), OutResult.EdgeCosts.Num(),
		OutResult.TotalLength_m, OutResult.TotalCost);
}

float UAStarPathFinder::EvaluateEdge(
	const FRailGridData& InGrid,
	int32				 InFromIndex,
	int32				 InToIndex,
	const TArray<int32>& InCameFrom) const
{
	const FRailGridNode& FromNode = InGrid.Nodes[InFromIndex];
	const FRailGridNode& ToNode = InGrid.Nodes[InToIndex];

	const int32 DX = FMath::Abs(ToNode.X - FromNode.X);
	const int32 DY = FMath::Abs(ToNode.Y - FromNode.Y);
	const bool	bDiagonal = (DX != 0 && DY != 0);

	const float EdgeLength_m = bDiagonal
		? InGrid.StepSize_m * FMath::Sqrt(2.0f)
		: InGrid.StepSize_m;

	const float SlopeRatio = FMath::Abs(ToNode.Height_m - FromNode.Height_m) / EdgeLength_m;

	float		TurnAngleDeg = 0.0f;
	const int32 PrevIndex = InCameFrom[InFromIndex];
	if (PrevIndex != INDEX_NONE)
	{
		const FRailGridNode& PrevNode = InGrid.Nodes[PrevIndex];
		const float			 PrevDirX = FromNode.X - PrevNode.X;
		const float			 PrevDirY = FromNode.Y - PrevNode.Y;
		const float			 CurrDirX = ToNode.X - FromNode.X;
		const float			 CurrDirY = ToNode.Y - FromNode.Y;

		const float Dot = PrevDirX * CurrDirX + PrevDirY * CurrDirY;
		const float LenA = FMath::Sqrt(PrevDirX * PrevDirX + PrevDirY * PrevDirY);
		const float LenB = FMath::Sqrt(CurrDirX * CurrDirX + CurrDirY * CurrDirY);

		if (LenA > KINDA_SMALL_NUMBER && LenB > KINDA_SMALL_NUMBER)
		{
			const float CosAngle = FMath::Clamp(Dot / (LenA * LenB), -1.0f, 1.0f);
			TurnAngleDeg = FMath::RadiansToDegrees(FMath::Acos(CosAngle));
		}
	}

	// Жесткий запрет: поезда не могут делать резкие изломы и развороты назад.
	// На 8-связной сетке допустимы шаг вперед (0°) и плавный поворот на 45° на соседнюю диагональ/ортогональ.
	// Изломы >= 90° (прямые, тупые и острые углы) категорически запрещены за один шаг сетки,
	// так как они приводят к недопустимым радиусам кривизны (< 50 м). Поворот на 90° должен выполняться
	// плавно минимум за 2 последовательных шага сетки (0° -> 45° -> 90°), обеспечивая R >= 300 м по СП 119.13330.
	if (TurnAngleDeg > 50.0f)
	{
		return -1.0f; // Острый или прямой излом за один шаг запрещен
	}

	const float GroundCoef = CostCalculator->GetGroundCoefficient(ToNode.GroundType);
	const float SlopePercent = SlopeRatio * 100.0f;

	float EdgeCost = CostCalculator->CalculateEdgeCost(
		EdgeLength_m, SlopePercent, TurnAngleDeg, GroundCoef);

	// Учет зон приоритета: зеленая зона снижает стоимость прохождения, направляя трассу в рекомендуемый коридор
	if (ToNode.PriorityModifier > 0.0f && !FMath::IsNearlyEqual(ToNode.PriorityModifier, 1.0f))
	{
		EdgeCost *= FMath::Max(ToNode.PriorityModifier, 0.05f);
	}

	return EdgeCost;
}

float UAStarPathFinder::Heuristic(
	const FRailGridData& InGrid,
	int32				 InFromIndex,
	int32				 InToIndex) const
{
	const FRailGridNode& From = InGrid.Nodes[InFromIndex];
	const FRailGridNode& To = InGrid.Nodes[InToIndex];

	const float DX = static_cast<float>(To.X - From.X) * InGrid.StepSize_m;
	const float DY = static_cast<float>(To.Y - From.Y) * InGrid.StepSize_m;

	// Дистанция в километрах (так как стоимость у нас в руб/км)
	const float Distance_km = FMath::Sqrt(DX * DX + DY * DY) / 1000.0f;

	const float MinCoefficient = CostCalculator ? CostCalculator->GetMinGroundCoefficient() : 0.9f;

	// Берем минимальную стоимость 1 км (для идеальной равнины) по НЦС 81-02-07-2025,
	// чтобы эвристика была в тех же величинах (тыс. руб.), что и стоимость пути
	const float MinBaseCostPerKm_kRub = 313075.0f;

	return Distance_km * MinBaseCostPerKm_kRub * MinCoefficient;
}