// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект

#pragma once

#include "Algorithms/AStarPathFinder.h"
#include "Algorithms/RailCategories.h"
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RailOptimaGameMode.generated.h"

class APathRenderer;
class AProceduralTerrain;
class ARailEndpoint;
class ARailOptimaPlayerController;
class ARailSpectatorPawn;

DECLARE_MULTICAST_DELEGATE_FourParams(FRailPathRecomputedSignature, double, double, double, int32);

USTRUCT(BlueprintType)
struct FRailRuntimeSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Settings")
	float SlopePenalty = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Settings")
	float TurnPenalty = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Settings")
	float TargetStepSize_m = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Settings")
	bool bColorPathByCost = true;
};

UCLASS()
class RAILOPTIMA_API ARailOptimaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARailOptimaGameMode();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Path")
	void RecomputePath();

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Settings")
	void ApplySettings(float InSlopePenalty, float InTurnPenalty, float InStepSize_m, bool bInColorByCost);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Path")
	void ApplyViewScaleFromCamera(ARailSpectatorPawn* InPawn);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Settings")
	ERailRoadCategory SelectedCategory = ERailRoadCategory::Category1;

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Settings")
	void SetSelectedCategory(ERailRoadCategory NewCategory);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	ERailRoadCategory GetSelectedCategory() const { return SelectedCategory; }

	FRailPathRecomputedSignature OnPathRecomputed;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Settings")
	FRailRuntimeSettings RuntimeSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Grid")
	float RecomputeDebounceSec = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Endpoint")
	float AutoSpawnMargin_m = 500.0f;

	float ReferenceTopDownHeight_cm = 0.0f;
	float CurrentViewScale = 1.0f;
	bool  bViewScaleReferenceCaptured = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Path")
	float MinViewScale = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Path")
	float MaxViewScale = 6.0f;

private:
	void SetupSubsystems();
	void FindTerrain();
	void SetupPathRenderer();
	void SetupEndpoints();
	void ApplySelectedCategory();

	FRailGridData BuildGridFromTerrain(const AProceduralTerrain& InTerrain, float InTargetStep_m) const;
	FRailGridData GenerateSyntheticGrid(int32 InSize, float InStepSize_m) const;

	int32 WorldToNodeIndex(const FRailGridData& InGrid, const FVector& InWorldPos) const;

	UFUNCTION()
	void HandleEndpointMoved(ARailEndpoint* Endpoint);

	void HandleDeferredRecompute();

	// Генератор дуг (Tangent-Curve-Tangent)
	TArray<FVector> GenerateRailwayArcs(const FRailGridData& InGrid, const FRailPathResult& InPath, const FRailCategoryNorms& InNorms) const;

	UPROPERTY()
	TObjectPtr<UAStarPathFinder> PathFinder;
	UPROPERTY()
	TObjectPtr<UCostCalculator> CostCalculator;
	UPROPERTY()
	TObjectPtr<APathRenderer> PathRenderer;
	UPROPERTY()
	TObjectPtr<AProceduralTerrain> Terrain;
	UPROPERTY()
	TObjectPtr<ARailEndpoint> StartEndpoint;
	UPROPERTY()
	TObjectPtr<ARailEndpoint> EndEndpoint;

	FRailGridData CachedGrid;
	bool		  bGridCached = false;

	FTimerHandle RecomputeTimer;
};