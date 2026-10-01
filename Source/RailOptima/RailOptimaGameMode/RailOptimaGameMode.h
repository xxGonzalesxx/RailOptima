// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Algorithms/AStarPathFinder.h"
#include "RailOptimaGameMode.generated.h"

class APathRenderer;
class AProceduralTerrain;
class ARailEndpoint;
class ARailOptimaPlayerController;
class ARailSpectatorPawn;

DECLARE_MULTICAST_DELEGATE_FourParams(FRailPathRecomputedSignature,
	double /*Length_m*/, double /*Cost*/, double /*TimeMs*/, int32 /*Nodes*/);

USTRUCT(BlueprintType)
struct FRailRuntimeSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Settings",
		meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float SlopePenalty = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Settings",
		meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float TurnPenalty = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Settings",
		meta = (ClampMin = "10.0", ClampMax = "1000.0"))
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
	void ApplySettings(float InSlopePenalty, float InTurnPenalty,
		float InStepSize_m, bool bInColorByCost);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	float GetSlopePenalty() const { return RuntimeSettings.SlopePenalty; }

	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	float GetTurnPenalty() const { return RuntimeSettings.TurnPenalty; }

	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	float GetTargetStepSize() const { return RuntimeSettings.TargetStepSize_m; }

	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	bool GetColorPathByCost() const { return RuntimeSettings.bColorPathByCost; }

	/**
	 * Обновить масштаб пути и маркеров в зависимости от высоты камеры.
	 * В TopDown — масштаб пропорционален текущей высоте.
	 * В FreeFly — базовый масштаб 1.0.
	 * Троттлинг: пересчёт только при изменении масштаба > 2%.
	 */
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Path")
	void ApplyViewScaleFromCamera(ARailSpectatorPawn* InPawn);

	FRailPathRecomputedSignature OnPathRecomputed;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Settings")
	FRailRuntimeSettings RuntimeSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Grid",
		meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float RecomputeDebounceSec = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Endpoint")
	float AutoSpawnMargin_m = 500.0f;

	// Референсная высота TopDown, захватывается при первом вызове
	// ApplyViewScaleFromCamera.
	float ReferenceTopDownHeight_cm = 0.0f;

	// Текущий применённый масштаб.
	float CurrentViewScale = 1.0f;

	bool bViewScaleReferenceCaptured = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Path",
		meta = (ClampMin = "0.1", ClampMax = "20.0"))
	float MinViewScale = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Path",
		meta = (ClampMin = "0.1", ClampMax = "20.0"))
	float MaxViewScale = 6.0f;

private:

	void SetupSubsystems();
	void FindTerrain();
	void SetupPathRenderer();
	void SetupEndpoints();

	FRailGridData BuildGridFromTerrain(const AProceduralTerrain& InTerrain, float InTargetStep_m) const;
	FRailGridData GenerateSyntheticGrid(int32 InSize, float InStepSize_m) const;

	int32 WorldToNodeIndex(const FRailGridData& InGrid, const FVector& InWorldPos) const;
	FVector NodeIndexToWorldXY(const FRailGridData& InGrid, int32 InNodeIndex) const;

	UFUNCTION()
	void HandleEndpointMoved(ARailEndpoint* Endpoint);

	void HandleDeferredRecompute();

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
	bool bGridCached = false;

	FTimerHandle RecomputeTimer;
};