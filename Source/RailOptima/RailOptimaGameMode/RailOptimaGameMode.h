// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#pragma once

#include "Algorithms/AStarPathFinder.h"
#include "Algorithms/RailCategories.h"
#include "Algorithms/RailPathValidator.h"
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Services/RailProjectTypes.h"
#include "UI/RailToastWidget.h"
#include "RailOptimaGameMode.generated.h"

class APathRenderer;
class AProceduralTerrain;
class ARailEndpoint;
class ARailOptimaPlayerController;
class ARailSpectatorPawn;
class URailCostSummaryWidget;
class URailMainMenuWidget;
class URailPauseMenuWidget;
class URailGraphicsSettingsWidget;
class URailBrushWidget;
class ARailZoneOverlayRenderer;
class URailProfileWidget;
struct FRailProfilePoint;
class UProceduralMeshComponent;

UENUM(BlueprintType)
enum class ERailZoneType : uint8
{
	ForbiddenRed   UMETA(DisplayName = "Запретная зона (Красная)"),
	PriorityGreen  UMETA(DisplayName = "Приоритетная зона (Зеленая)"),
	Eraser         UMETA(DisplayName = "Ластик")
};

DECLARE_MULTICAST_DELEGATE_FourParams(FRailPathRecomputedSignature,
	double /*Length_m*/, double /*Cost_kRub*/, double /*TimeMs*/, int32 /*Nodes*/);

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
	void ApplySettings(float InSlopePenalty, float InTurnPenalty, float InStepSize_m, bool bInColorByCost);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	float GetSlopePenalty() const { return RuntimeSettings.SlopePenalty; }

	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	float GetTurnPenalty() const { return RuntimeSettings.TurnPenalty; }

	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	float GetTargetStepSize() const { return RuntimeSettings.TargetStepSize_m; }

	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	bool GetColorPathByCost() const { return RuntimeSettings.bColorPathByCost; }

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Path")
	void ApplyViewScaleFromCamera(ARailSpectatorPawn* InPawn);

	// Выбранная категория ЖД (по умолчанию — I Категория)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Settings")
	ERailRoadCategory SelectedCategory = ERailRoadCategory::Category1;

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Settings")
	void SetSelectedCategory(ERailRoadCategory NewCategory);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	ERailRoadCategory GetSelectedCategory() const { return SelectedCategory; }

	// ─────────────────────────────────────────────────────────────────
	//  Промежуточные точки (Waypoints)
	// ─────────────────────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Waypoints")
	ARailEndpoint* AddWaypointAtLocation(const FVector& WorldLocation, int32 InsertIndex = -1);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Waypoints")
	bool TryAddWaypointNearLocation(const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Waypoints")
	bool RemoveWaypoint(ARailEndpoint* InWaypoint);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Waypoints")
	void ClearAllWaypoints();

	UFUNCTION(BlueprintPure, Category = "RailOptima|Waypoints")
	const TArray<ARailEndpoint*>& GetWaypoints() const { return Waypoints; }

	// ─────────────────────────────────────────────────────────────────
	//  Зоны и интерактивная кисть
	// ─────────────────────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Zones")
	void SetBrushModeActive(bool bActive);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Zones")
	bool IsBrushModeActive() const { return bBrushModeActive; }

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Zones")
	void ToggleBrushMode() { SetBrushModeActive(!bBrushModeActive); }

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Zones")
	void SetActiveZoneType(ERailZoneType NewType);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Zones")
	ERailZoneType GetActiveZoneType() const { return ActiveZoneType; }

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Zones")
	void SetBrushRadius_m(float InRadius_m);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Zones")
	float GetBrushRadius_m() const { return BrushRadius_m; }

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Zones")
	void PaintZoneAtLocation(const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Zones")
	void ClearAllZones();

	// ─────────────────────────────────────────────────────────────────
	//  Управление проектами и UI
	// ─────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Project")
	void OpenProject(const FRailProjectMetadata& InProject);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Project")
	void SaveCurrentProject();

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Project")
	void CloseCurrentProject();

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Project")
	void ExportCostToExcel();

	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void TogglePauseMenu();

	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void ShowGraphicsSettings();

	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void ShowToast(const FString& Message, ERailToastType Type = ERailToastType::Info, float DurationSec = 4.5f);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void ToggleProfileWidget();

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Settings")
	void ApplyVisualAndCameraSettings(float FlySpeed_cmps, float BoostMultiplier, float PathWidth_cm, float PathHeight_cm, float EndpointScale);

	void LoadAndApplyVisualSettingsFromConfig();

	UFUNCTION(BlueprintPure, Category = "RailOptima|Project")
	bool IsProjectActive() const { return bIsProjectActive; }

	UFUNCTION(BlueprintPure, Category = "RailOptima|Project")
	const FRailProjectMetadata& GetActiveProject() const { return CurrentActiveProject; }

	FRailPathRecomputedSignature OnPathRecomputed;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Settings")
	FRailRuntimeSettings RuntimeSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Grid",
		meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float RecomputeDebounceSec = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Endpoint")
	float AutoSpawnMargin_m = 500.0f;

	float ReferenceTopDownHeight_cm = 0.0f;
	float CurrentViewScale = 1.0f;
	bool  bViewScaleReferenceCaptured = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Path",
		meta = (ClampMin = "0.1", ClampMax = "20.0"))
	float MinViewScale = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Path",
		meta = (ClampMin = "0.1", ClampMax = "20.0"))
	float MaxViewScale = 6.0f;

private:
	void SetupSubsystems();
	void SetupCostSummaryWidget();
	void EnsureDefaultEnvironment();
	void FindTerrain();
	void SetupPathRenderer();
	void SetupEndpoints();
	void ApplySelectedCategory();

	TArray<FVector> GenerateRailwayArcs(
		const FRailGridData&	  InGrid,
		const FRailPathResult&	  InPath,
		const FRailCategoryNorms& InNorms,
		float&					  OutMinPlanRadius_m,
		float&					  OutMaxSlopePermille) const;

	FRailGridData BuildGridFromTerrain(const AProceduralTerrain& InTerrain, float InTargetStep_m) const;
	FRailGridData GenerateSyntheticGrid(int32 InSize, float InStepSize_m) const;

	int32	WorldToNodeIndex(const FRailGridData& InGrid, const FVector& InWorldPos) const;
	FVector NodeIndexToWorldXY(const FRailGridData& InGrid, int32 InNodeIndex) const;

	UFUNCTION()
	void HandleEndpointMoved(ARailEndpoint* Endpoint);

	void HandleDeferredRecompute();

	// Настройка меню и тостов
	void SetupMainMenuWidget();
	void SetupPauseMenuWidget();
	void SetupGlobalToastWidget();
	void SetupGraphicsSettingsWidget();

	UFUNCTION()
	void HandleGraphicsSettingsApplied();

	UFUNCTION()
	void HandleProjectOpenedFromMenu(const FRailProjectMetadata& ProjectMeta);

	UFUNCTION()
	void HandlePauseResume();

	UFUNCTION()
	void HandlePauseSave();

	UFUNCTION()
	void HandlePauseExportExcel();

	UFUNCTION()
	void HandlePauseReturnToMenu();

	UPROPERTY()
	TObjectPtr<URailMainMenuWidget> MainMenuWidget;

	UPROPERTY()
	TObjectPtr<URailPauseMenuWidget> PauseMenuWidget;

	UPROPERTY()
	TObjectPtr<URailToastWidget> GlobalToastWidget;

	UPROPERTY()
	TObjectPtr<URailGraphicsSettingsWidget> GraphicsSettingsWidget;

	bool bIsProjectActive = false;
	FRailProjectMetadata CurrentActiveProject;

	// Кэш последнего расчёта для сметы
	double LastLength_m = 0.0;
	double LastCost_kRub = 0.0;
	float  LastMaxSlope_perm = 0.0f;
	float  LastMinRadius_m = 0.0f;
	double LastCalcTime_ms = 0.0;

	UPROPERTY()
	TObjectPtr<UAStarPathFinder> PathFinder;

	UPROPERTY()
	TObjectPtr<UCostCalculator> CostCalculator;

	UPROPERTY()
	TObjectPtr<URailPathValidator> PathValidator;

	UPROPERTY()
	TObjectPtr<URailCostSummaryWidget> CostSummaryWidget;

	UPROPERTY()
	TObjectPtr<APathRenderer> PathRenderer;

	UPROPERTY()
	TObjectPtr<AProceduralTerrain> Terrain;

	UPROPERTY()
	TObjectPtr<ARailEndpoint> StartEndpoint;

	UPROPERTY()
	TObjectPtr<ARailEndpoint> EndEndpoint;

	UPROPERTY()
	TArray<TObjectPtr<ARailEndpoint>> Waypoints;

	bool bBrushModeActive = false;
	ERailZoneType ActiveZoneType = ERailZoneType::ForbiddenRed;
	float BrushRadius_m = 150.0f;

	TArray<FVector> LastSmoothTrackPoints;

	void SetupZoneOverlayRenderer();
	void UpdateZoneOverlay();

	UPROPERTY()
	TObjectPtr<ARailZoneOverlayRenderer> ZoneOverlayRenderer;

	UPROPERTY()
	TObjectPtr<URailBrushWidget> BrushWidget;

	void SetupProfileWidget();

	UPROPERTY()
	TObjectPtr<URailProfileWidget> ProfileWidget;

	TArray<FRailProfilePoint> LastProfilePoints;

	FRailGridData CachedGrid;
	bool		  bGridCached = false;

	FTimerHandle RecomputeTimer;
};