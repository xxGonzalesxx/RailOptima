 // Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Визуализатор запретных зон поверх рельефа (аналогично дорогам и рекам)

#include "Visualization/RailZoneOverlayRenderer.h"
#include "Core/ProceduralTerrain.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ARailZoneOverlayRenderer::ARailZoneOverlayRenderer()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	OverlayMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("OverlayMesh"));
	OverlayMesh->SetupAttachment(SceneRoot);
	OverlayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OverlayMesh->SetCastShadow(false);
	OverlayMesh->bUseAsyncCooking = false;
}

void ARailZoneOverlayRenderer::BeginPlay()
{
	Super::BeginPlay();

	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RailOptima/Materials/M_EndpointMarker.M_EndpointMarker"));
	if (!BaseMat)
	{
		BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RailOptima/Materials/M_RoadLine.M_RoadLine"));
	}
	if (!BaseMat)
	{
		BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial"));
	}

	if (BaseMat)
	{
		ZoneMaterialInstance = UMaterialInstanceDynamic::Create(BaseMat, this);
		if (ZoneMaterialInstance)
		{
			ZoneMaterialInstance->SetVectorParameterValue(TEXT("BaseColor"), ForbiddenColor);
			ZoneMaterialInstance->SetVectorParameterValue(TEXT("Color"), ForbiddenColor);
			ZoneMaterialInstance->SetScalarParameterValue(TEXT("EmissiveBoost"), 3.0f);
		}
	}
}

void ARailZoneOverlayRenderer::ClearZones()
{
	if (OverlayMesh)
	{
		OverlayMesh->ClearAllMeshSections();
	}
}

void ARailZoneOverlayRenderer::UpdateZones(const FRailGridData& InGrid, const FVector& Origin, const AProceduralTerrain* InTerrain)
{
	if (!OverlayMesh)
	{
		return;
	}

	ClearZones();

	if (InGrid.Width <= 0 || InGrid.Height <= 0 || InGrid.Nodes.Num() == 0)
	{
		return;
	}

	const AProceduralTerrain* TerrainPtr = InTerrain;
	if (!TerrainPtr)
	{
		TerrainPtr = Cast<AProceduralTerrain>(UGameplayStatics::GetActorOfClass(GetWorld(), AProceduralTerrain::StaticClass()));
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	const float Step_cm = InGrid.StepSize_m * 100.0f;
	const float HalfCell = Step_cm * 0.52f; // Легкий 4% нахлест для устранения микро-зазоров на стыках ячеек при уклонах рельефа

	for (int32 i = 0; i < InGrid.Nodes.Num(); ++i)
	{
		const FRailGridNode& Node = InGrid.Nodes[i];
		if (!Node.bIsForbidden)
		{
			continue;
		}

		const float CenterX = Origin.X + Node.X * Step_cm;
		const float CenterY = Origin.Y + Node.Y * Step_cm;

		// 4 угла ячейки
		const FVector2D P0(CenterX - HalfCell, CenterY - HalfCell);
		const FVector2D P1(CenterX + HalfCell, CenterY - HalfCell);
		const FVector2D P2(CenterX + HalfCell, CenterY + HalfCell);
		const FVector2D P3(CenterX - HalfCell, CenterY + HalfCell);

		float Z0 = 0.0f, Z1 = 0.0f, Z2 = 0.0f, Z3 = 0.0f;

		if (TerrainPtr)
		{
			if (!TerrainPtr->SampleTerrainZ_cm(P0, Z0)) Z0 = Node.Height_m * 100.0f;
			if (!TerrainPtr->SampleTerrainZ_cm(P1, Z1)) Z1 = Node.Height_m * 100.0f;
			if (!TerrainPtr->SampleTerrainZ_cm(P2, Z2)) Z2 = Node.Height_m * 100.0f;
			if (!TerrainPtr->SampleTerrainZ_cm(P3, Z3)) Z3 = Node.Height_m * 100.0f;
		}
		else
		{
			Z0 = Z1 = Z2 = Z3 = Node.Height_m * 100.0f;
		}

		Z0 += ZOffset_cm;
		Z1 += ZOffset_cm;
		Z2 += ZOffset_cm;
		Z3 += ZOffset_cm;

		const int32 BaseVert = Vertices.Num();
		Vertices.Add(FVector(P0.X, P0.Y, Z0));
		Vertices.Add(FVector(P1.X, P1.Y, Z1));
		Vertices.Add(FVector(P2.X, P2.Y, Z2));
		Vertices.Add(FVector(P3.X, P3.Y, Z3));

		Normals.Add(FVector::UpVector);
		Normals.Add(FVector::UpVector);
		Normals.Add(FVector::UpVector);
		Normals.Add(FVector::UpVector);

		Colors.Add(ForbiddenColor);
		Colors.Add(ForbiddenColor);
		Colors.Add(ForbiddenColor);
		Colors.Add(ForbiddenColor);

		UVs.Add(FVector2D(0.0f, 0.0f));
		UVs.Add(FVector2D(1.0f, 0.0f));
		UVs.Add(FVector2D(1.0f, 1.0f));
		UVs.Add(FVector2D(0.0f, 1.0f));

		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));

		// Лицевая сторона (вид сверху)
		Triangles.Add(BaseVert + 0);
		Triangles.Add(BaseVert + 1);
		Triangles.Add(BaseVert + 2);

		Triangles.Add(BaseVert + 0);
		Triangles.Add(BaseVert + 2);
		Triangles.Add(BaseVert + 3);

		// Обратная сторона (вид под острым углом/сбоку)
		Triangles.Add(BaseVert + 0);
		Triangles.Add(BaseVert + 2);
		Triangles.Add(BaseVert + 1);

		Triangles.Add(BaseVert + 0);
		Triangles.Add(BaseVert + 3);
		Triangles.Add(BaseVert + 2);
	}

	if (Vertices.Num() > 0)
	{
		OverlayMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false, true);

		if (!ZoneMaterialInstance)
		{
			UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RailOptima/Materials/M_EndpointMarker.M_EndpointMarker"));
			if (!BaseMat)
			{
				BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RailOptima/Materials/M_RoadLine.M_RoadLine"));
			}
			if (!BaseMat)
			{
				BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial"));
			}
			if (BaseMat)
			{
				ZoneMaterialInstance = UMaterialInstanceDynamic::Create(BaseMat, this);
			}
		}

		if (ZoneMaterialInstance)
		{
			ZoneMaterialInstance->SetVectorParameterValue(TEXT("BaseColor"), ForbiddenColor);
			ZoneMaterialInstance->SetVectorParameterValue(TEXT("Color"), ForbiddenColor);
			ZoneMaterialInstance->SetScalarParameterValue(TEXT("EmissiveBoost"), 3.0f);
			OverlayMesh->SetMaterial(0, ZoneMaterialInstance);
		}
		else
		{
			UMaterialInterface* VertexMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial"));
			if (VertexMat)
			{
				OverlayMesh->SetMaterial(0, VertexMat);
			}
		}
	}
}
