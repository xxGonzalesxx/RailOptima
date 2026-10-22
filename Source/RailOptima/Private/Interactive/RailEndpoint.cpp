// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль
// визуализации», ПривГУПС, 2026.

#include "Interactive/RailEndpoint.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

// ─────────────────────────────────────────────────────────────────
//  Жизненный цикл
// ─────────────────────────────────────────────────────────────────

ARailEndpoint::ARailEndpoint()
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

	CollisionSphere =
		CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetupAttachment(SceneRoot);
	CollisionSphere->InitSphereRadius(SphereRadius_cm);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereFinder.Succeeded())
	{
		VisualMesh->SetStaticMesh(SphereFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatFinder(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (MatFinder.Succeeded())
	{
		EndpointMaterial = MatFinder.Object;
	}
}

void ARailEndpoint::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyVisuals();

	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		LogState(TEXT("OnConstruction"));
	}
}

void ARailEndpoint::BeginPlay()
{
	Super::BeginPlay();

	BaseSphereRadius_cm = SphereRadius_cm;

	ApplyVisuals();
	LogState(TEXT("BeginPlay"));

	// УДАЛЕН отладочный сброс в (10, 10, 0), теперь маркеры остаются на своих
	// местах.

	UE_LOG(LogTemp, Log,
		TEXT("[RailEndpoint|%s] База радиуса зафиксирована: %.0f см."),
		bIsStart ? TEXT("Start") : TEXT("End"), BaseSphereRadius_cm);
}

// ─────────────────────────────────────────────────────────────────
//  API
// ─────────────────────────────────────────────────────────────────

void ARailEndpoint::SetEndpointFromTerrainHit(
	const FVector& InTerrainHitLocation, bool bBroadcastEvent)
{
	const FVector OldLocation = GetActorLocation();

	const float NewZ = InTerrainHitLocation.Z + TerrainZOffset_cm;

	const FVector NewLocation(InTerrainHitLocation.X, InTerrainHitLocation.Y,
		NewZ);

	if (OldLocation.Equals(NewLocation, 1.0f))
	{
		return;
	}

	SetActorLocation(NewLocation);

	UE_LOG(
		LogTemp, Log,
		TEXT("[RailEndpoint|%s] Move: (%.0f, %.0f, %.0f) → (%.0f, %.0f, %.0f). ")
			TEXT("TerrainZ=%.0f, Offset=%.0f."),
		bIsStart ? TEXT("Start") : TEXT("End"), OldLocation.X, OldLocation.Y,
		OldLocation.Z, NewLocation.X, NewLocation.Y, NewLocation.Z,
		InTerrainHitLocation.Z, TerrainZOffset_cm);

	if (bBroadcastEvent)
	{
		OnEndpointMoved.Broadcast(this);
	}
}

void ARailEndpoint::SetHighlighted(bool bHighlighted)
{
	if (!DynamicMaterial)
	{
		ApplyVisuals();
	}

	if (!DynamicMaterial)
		return;

	const float Emissive = bHighlighted ? HighlightEmissiveBoost : 0.0f;
	DynamicMaterial->SetScalarParameterValue(TEXT("EmissiveBoost"), Emissive);
}

void ARailEndpoint::SetViewScale(float InScale)
{
	const float NewScale = FMath::Clamp(InScale, MinViewScale, MaxViewScale);

	if (FMath::IsNearlyEqual(NewScale, ViewScale, 0.01f))
	{
		return;
	}

	ViewScale = NewScale;
	SphereRadius_cm = BaseSphereRadius_cm * ViewScale;
	ApplyVisuals();
}

// ─────────────────────────────────────────────────────────────────
//  Внутренняя логика
// ─────────────────────────────────────────────────────────────────

void ARailEndpoint::ApplyVisuals()
{
	if (CollisionSphere)
	{
		CollisionSphere->SetSphereRadius(SphereRadius_cm,
			/*bUpdateOverlaps=*/false);
	}

	if (VisualMesh)
	{
		const float Scale = SphereRadius_cm / 50.0f;
		VisualMesh->SetRelativeScale3D(FVector(Scale));
	}

	if (VisualMesh && EndpointMaterial)
	{
		if (!DynamicMaterial || DynamicMaterial->Parent != EndpointMaterial)
		{
			DynamicMaterial =
				UMaterialInstanceDynamic::Create(EndpointMaterial, this);
		}

		if (DynamicMaterial)
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"),
				EndpointColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), EndpointColor);
			DynamicMaterial->SetScalarParameterValue(TEXT("EmissiveBoost"), 0.0f);

			VisualMesh->SetMaterial(0, DynamicMaterial);
		}
	}
}

void ARailEndpoint::LogState(const FString& InContext) const
{
	const FVector Loc = GetActorLocation();

	UE_LOG(LogTemp, Log,
		TEXT("[RailEndpoint|%s] %s: role=%s, pos=(%.0f, %.0f, %.0f) см, "
			 "radius=%.0f см."),
		bIsStart ? TEXT("Start") : TEXT("End"), *InContext,
		bIsStart ? TEXT("START") : TEXT("END"), Loc.X, Loc.Y, Loc.Z,
		SphereRadius_cm);
}