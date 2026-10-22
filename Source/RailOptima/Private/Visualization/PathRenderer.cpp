// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект

#include "Visualization/PathRenderer.h"
#include "Components/BillboardComponent.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

APathRenderer::APathRenderer()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	Billboard->SetupAttachment(SceneRoot);
#if WITH_EDITOR
	Billboard->bVisualizeComponent = true;
#endif
	Billboard->SetHiddenInGame(true);

	PathMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("PathMesh"));
	PathMesh->SetupAttachment(SceneRoot);
	PathMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PathMesh->SetCastShadow(false);
	PathMesh->bUseAsyncCooking = true;
}

void APathRenderer::BeginPlay()
{
	Super::BeginPlay();
	if (PathMesh)
		PathMesh->ClearAllMeshSections();
	BaseWidth_cm = Width_cm;
	BaseHeight_cm = Height_cm;
}

void APathRenderer::RenderPath(const FRailGridData& InGrid, const FRailPathResult& InPath)
{
	if (!InPath.bFound || InPath.NodeIndices.Num() < 2)
	{
		ClearPath();
		return;
	}

	TArray<float> Heights;
	Heights.Reserve(InPath.NodeIndices.Num());
	float MinH = TNumericLimits<float>::Max();
	float MaxH = TNumericLimits<float>::Lowest();

	for (const int32 NodeIndex : InPath.NodeIndices)
	{
		if (!InGrid.Nodes.IsValidIndex(NodeIndex))
			continue;
		const float H = InGrid.Nodes[NodeIndex].Height_m;
		Heights.Add(H);
		MinH = FMath::Min(MinH, H);
		MaxH = FMath::Max(MaxH, H);
	}

	if (FMath::IsNearlyEqual(MinH, MaxH))
		MaxH = MinH + 1.0f;
	RenderColoredPath(InGrid, InPath, Heights, MinH, MaxH);
}

void APathRenderer::RenderColoredPath(const FRailGridData& InGrid, const FRailPathResult& InPath, const TArray<float>& InNodeValues, float InLowValue, float InHighValue)
{
	if (!InPath.bFound || InPath.NodeIndices.Num() < 2)
	{
		ClearPath();
		return;
	}
	if (InNodeValues.Num() != InPath.NodeIndices.Num())
		return;

	const float			 Range = FMath::Max(InHighValue - InLowValue, KINDA_SMALL_NUMBER);
	TArray<FVector>		 RawPoints;
	TArray<FLinearColor> RawColors;
	RawPoints.Reserve(InPath.NodeIndices.Num());
	RawColors.Reserve(InPath.NodeIndices.Num());

	for (int32 i = 0; i < InPath.NodeIndices.Num(); ++i)
	{
		const int32 NodeIndex = InPath.NodeIndices[i];
		if (!InGrid.Nodes.IsValidIndex(NodeIndex))
			continue;
		const FRailGridNode& Node = InGrid.Nodes[NodeIndex];
		RawPoints.Add(GridNodeToWorld(Node, InGrid.StepSize_m));
		const float T = FMath::Clamp((InNodeValues[i] - InLowValue) / Range, 0.0f, 1.0f);
		RawColors.Add(EvaluateColor(T));
	}

	if (RawPoints.Num() < 2)
	{
		ClearPath();
		return;
	}

	TArray<FVector>		 FinalPoints;
	TArray<FLinearColor> FinalColors;

	if (bSmoothPath && SamplesPerSegment > 1)
	{
		SmoothPolylineCatmullRom(RawPoints, RawColors, SamplesPerSegment, FinalPoints, FinalColors);
	}
	else
	{
		FinalPoints = RawPoints;
		FinalColors = RawColors;
	}

	RenderCustomPath(FinalPoints, FinalColors);
}

void APathRenderer::RenderCustomPath(const TArray<FVector>& InPoints, const TArray<FLinearColor>& InColors)
{
	if (InPoints.Num() < 2 || InPoints.Num() != InColors.Num())
	{
		ClearPath();
		return;
	}

	CachedRibbonPoints = InPoints;
	CachedRibbonColors = InColors;

	BuildRibbon(CachedRibbonPoints, CachedRibbonColors, Width_cm, Height_cm);

	if (PathMaterial)
		PathMesh->SetMaterial(0, PathMaterial);
}

void APathRenderer::ClearPath()
{
	CachedRibbonPoints.Reset();
	CachedRibbonColors.Reset();
	if (PathMesh)
		PathMesh->ClearAllMeshSections();
}

void APathRenderer::SetViewScale(float InScale)
{
	const float NewScale = FMath::Clamp(InScale, MinViewScale, MaxViewScale);
	if (FMath::IsNearlyEqual(NewScale, ViewScale, 0.01f))
		return;

	ViewScale = NewScale;
	Width_cm = BaseWidth_cm * ViewScale;
	Height_cm = BaseHeight_cm * ViewScale;

	if (CachedRibbonPoints.Num() >= 2 && CachedRibbonPoints.Num() == CachedRibbonColors.Num())
	{
		BuildRibbon(CachedRibbonPoints, CachedRibbonColors, Width_cm, Height_cm);
	}
}

FLinearColor APathRenderer::EvaluateColor(float InT) const
{
	const float ClampedT = FMath::Clamp(InT, 0.0f, 1.0f);
	if (ClampedT < 0.5f)
		return FMath::Lerp(CheapColor, MidColor, ClampedT * 2.0f);
	return FMath::Lerp(MidColor, ExpensiveColor, (ClampedT - 0.5f) * 2.0f);
}

FVector APathRenderer::GridNodeToWorld(const FRailGridNode& InNode, float InStepSize_m) const
{
	return GridWorldOrigin + FVector(static_cast<float>(InNode.X) * InStepSize_m * 100.0f, static_cast<float>(InNode.Y) * InStepSize_m * 100.0f, InNode.Height_m * 100.0f * WorldZScale + ZOffset_cm);
}

void APathRenderer::SmoothPolylineCatmullRom(const TArray<FVector>& InPoints, const TArray<FLinearColor>& InColors, int32 InSamplesPerSegment, TArray<FVector>& OutPoints, TArray<FLinearColor>& OutColors)
{
	const int32 N = InPoints.Num();
	if (N < 2 || InSamplesPerSegment < 1)
	{
		OutPoints = InPoints;
		OutColors = InColors;
		return;
	}

	OutPoints.Reset();
	OutColors.Reset();
	OutPoints.Reserve((N - 1) * InSamplesPerSegment + 1);
	OutColors.Reserve((N - 1) * InSamplesPerSegment + 1);

	auto GetPoint = [&](int32 Index) -> FVector { return InPoints[FMath::Clamp(Index, 0, N - 1)]; };
	auto GetColor = [&](int32 Index) -> FLinearColor { return InColors[FMath::Clamp(Index, 0, N - 1)]; };

	for (int32 i = 0; i < N - 1; ++i)
	{
		const FVector	   P0 = GetPoint(i - 1);
		const FVector	   P1 = GetPoint(i);
		const FVector	   P2 = GetPoint(i + 1);
		const FVector	   P3 = GetPoint(i + 2);
		const FLinearColor C1 = GetColor(i);
		const FLinearColor C2 = GetColor(i + 1);

		for (int32 s = 0; s < InSamplesPerSegment; ++s)
		{
			const float	  t = static_cast<float>(s) / static_cast<float>(InSamplesPerSegment);
			const float	  t2 = t * t;
			const float	  t3 = t2 * t;
			const FVector P = 0.5f * ((2.0f * P1) + (-P0 + P2) * t + (2.0f * P0 - 5.0f * P1 + 4.0f * P2 - P3) * t2 + (-P0 + 3.0f * P1 - 3.0f * P2 + P3) * t3);
			OutPoints.Add(P);
			OutColors.Add(FMath::Lerp(C1, C2, t));
		}
	}
	OutPoints.Add(InPoints.Last());
	OutColors.Add(InColors.Last());
}

void APathRenderer::BuildRibbon(const TArray<FVector>& InPoints, const TArray<FLinearColor>& InColors, float InWidth_cm, float InHeight_cm)
{
	const int32 NumPoints = InPoints.Num();
	if (NumPoints < 2)
		return;
	const float HalfWidth = InWidth_cm * 0.5f;

	TArray<FVector2D> Directions;
	Directions.SetNumUninitialized(NumPoints);

	auto SegmentDir2D = [](const FVector& A, const FVector& B) -> FVector2D {
		FVector2D	D(B.X - A.X, B.Y - A.Y);
		const float Len = D.Size();
		return (Len > KINDA_SMALL_NUMBER) ? (D / Len) : FVector2D(1.0f, 0.0f);
	};

	Directions[0] = SegmentDir2D(InPoints[0], InPoints[1]);
	Directions[NumPoints - 1] = SegmentDir2D(InPoints[NumPoints - 2], InPoints[NumPoints - 1]);

	for (int32 i = 1; i < NumPoints - 1; ++i)
	{
		const FVector2D Prev = SegmentDir2D(InPoints[i - 1], InPoints[i]);
		const FVector2D Next = SegmentDir2D(InPoints[i], InPoints[i + 1]);
		FVector2D		Avg = Prev + Next;
		const float		Len = Avg.Size();
		Directions[i] = (Len > KINDA_SMALL_NUMBER) ? (Avg / Len) : Next;
	}

	TArray<FVector>			 Vertices;
	TArray<FLinearColor>	 VertColors;
	TArray<FVector>			 Normals;
	TArray<FVector2D>		 UVs;
	TArray<FProcMeshTangent> Tangents;

	const int32 NumVertices = NumPoints * 4;
	Vertices.Reserve(NumVertices);
	VertColors.Reserve(NumVertices);
	Normals.Reserve(NumVertices);
	UVs.Reserve(NumVertices);
	Tangents.Reserve(NumVertices);

	for (int32 i = 0; i < NumPoints; ++i)
	{
		const FVector&	 P = InPoints[i];
		const FVector2D& Dir = Directions[i];
		const FVector2D	 Perp(-Dir.Y, Dir.X);
		const FVector	 Lateral(Perp.X * HalfWidth, Perp.Y * HalfWidth, 0.0f);

		const FVector		   L = P - Lateral;
		const FVector		   R = P + Lateral;
		const FVector		   LTop(L.X, L.Y, L.Z + InHeight_cm);
		const FVector		   RTop(R.X, R.Y, R.Z + InHeight_cm);
		const FProcMeshTangent Tangent(FVector(Dir.X, Dir.Y, 0.0f), false);

		Vertices.Add(LTop);
		VertColors.Add(InColors[i]);
		Normals.Add(FVector::UpVector);
		UVs.Add(FVector2D(0.0f, static_cast<float>(i)));
		Tangents.Add(Tangent);

		Vertices.Add(RTop);
		VertColors.Add(InColors[i]);
		Normals.Add(FVector::UpVector);
		UVs.Add(FVector2D(1.0f, static_cast<float>(i)));
		Tangents.Add(Tangent);

		Vertices.Add(L);
		VertColors.Add(InColors[i]);
		Normals.Add(FVector(-Perp.X, -Perp.Y, 0.0f));
		UVs.Add(FVector2D(0.0f, static_cast<float>(i)));
		Tangents.Add(Tangent);

		Vertices.Add(R);
		VertColors.Add(InColors[i]);
		Normals.Add(FVector(Perp.X, Perp.Y, 0.0f));
		UVs.Add(FVector2D(1.0f, static_cast<float>(i)));
		Tangents.Add(Tangent);
	}

	TArray<int32> Triangles;
	Triangles.Reserve((NumPoints - 1) * 18);

	for (int32 i = 0; i < NumPoints - 1; ++i)
	{
		const int32 LTop0 = i * 4 + 0;
		const int32 RTop0 = i * 4 + 1;
		const int32 LBot0 = i * 4 + 2;
		const int32 RBot0 = i * 4 + 3;
		const int32 LTop1 = (i + 1) * 4 + 0;
		const int32 RTop1 = (i + 1) * 4 + 1;
		const int32 LBot1 = (i + 1) * 4 + 2;
		const int32 RBot1 = (i + 1) * 4 + 3;

		Triangles.Add(LTop0);
		Triangles.Add(RTop0);
		Triangles.Add(LTop1);
		Triangles.Add(RTop0);
		Triangles.Add(RTop1);
		Triangles.Add(LTop1);

		Triangles.Add(LTop0);
		Triangles.Add(LBot0);
		Triangles.Add(LTop1);
		Triangles.Add(LBot0);
		Triangles.Add(LBot1);
		Triangles.Add(LTop1);

		Triangles.Add(RTop0);
		Triangles.Add(RTop1);
		Triangles.Add(RBot0);
		Triangles.Add(RBot0);
		Triangles.Add(RTop1);
		Triangles.Add(RBot1);
	}

	PathMesh->ClearAllMeshSections();
	PathMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertColors, Tangents, false, true);
	if (PathMaterial)
		PathMesh->SetMaterial(0, PathMaterial);
}