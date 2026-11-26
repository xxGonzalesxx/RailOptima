 // Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Визуализатор запретных зон поверх рельефа (аналогично дорогам и рекам)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Algorithms/AStarPathFinder.h"
#include "RailZoneOverlayRenderer.generated.h"

class USceneComponent;
class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class AProceduralTerrain;

UCLASS()
class RAILOPTIMA_API ARailZoneOverlayRenderer : public AActor
{
	GENERATED_BODY()

public:
	ARailZoneOverlayRenderer();

	void UpdateZones(const FRailGridData& InGrid, const FVector& Origin, const AProceduralTerrain* InTerrain);
	void ClearZones();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProceduralMeshComponent> OverlayMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ZoneMaterialInstance;

	UPROPERTY(EditAnywhere, Category = "RailOptima|Zones")
	float ZOffset_cm = 350.0f;

	UPROPERTY(EditAnywhere, Category = "RailOptima|Zones")
	FLinearColor ForbiddenColor = FLinearColor(0.95f, 0.06f, 0.06f, 0.90f);
};
