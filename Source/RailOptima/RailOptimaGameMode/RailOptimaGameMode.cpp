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
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Services/RailCrashReporter.h"
#include "Services/RailProjectManager.h"
#include "TimerManager.h"
#include "UI/RailMainMenuWidget.h"
#include "UI/RailPauseMenuWidget.h"
#include "UI/RailGraphicsSettingsWidget.h"
#include "UI/RailBrushWidget.h"
#include "UI/RailProfileWidget.h"
#include "Visualization/RailZoneOverlayRenderer.h"
#include "Services/RailSettingsManager.h"
#include "UI/RailToastWidget.h"
#include "Visualization/PathRenderer.h"
#include "Visualization/RailCostSummaryWidget.h"
#include "Visualization/RoadRenderer.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"

ARailOptimaGameMode::ARailOptimaGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
	PlayerControllerClass = ARailOptimaPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void ARailOptimaGameMode::BeginPlay()
{
	Super::BeginPlay();
	EnsureDefaultEnvironment();
	SetupSubsystems();
	FindTerrain();
	SetupPathRenderer();
	SetupZoneOverlayRenderer();
	SetupEndpoints();
	SetupCostSummaryWidget();

	// Скрываем маркеры и виджет сметы до открытия конкретного проекта пользователем
	if (StartEndpoint)
	{
		StartEndpoint->SetActorHiddenInGame(true);
	}
	if (EndEndpoint)
	{
		EndEndpoint->SetActorHiddenInGame(true);
	}
	if (CostSummaryWidget)
	{
		CostSummaryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	URailSettingsManager::InitializeSettings(/*bForceBenchmark=*/false);
	SetupGlobalToastWidget();
	SetupGraphicsSettingsWidget();
	SetupPauseMenuWidget();
	SetupProfileWidget();
	LoadAndApplyVisualSettingsFromConfig();
	SetupMainMenuWidget();
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

void ARailOptimaGameMode::SetupGlobalToastWidget()
{
	if (GlobalToastWidget)
		return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
		return;

	GlobalToastWidget = CreateWidget<URailToastWidget>(PC, URailToastWidget::StaticClass());
	if (GlobalToastWidget)
	{
		GlobalToastWidget->AddToViewport(100);
	}
}

void ARailOptimaGameMode::SetupMainMenuWidget()
{
	if (MainMenuWidget)
	{
		MainMenuWidget->SetVisibility(ESlateVisibility::Visible);
		MainMenuWidget->RefreshProjectLibrary();
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
		return;

	MainMenuWidget = CreateWidget<URailMainMenuWidget>(PC, URailMainMenuWidget::StaticClass());
	if (MainMenuWidget)
	{
		MainMenuWidget->AddToViewport(50);
		MainMenuWidget->OnProjectOpened.AddDynamic(this, &ARailOptimaGameMode::HandleProjectOpenedFromMenu);
		MainMenuWidget->OnGraphicsSettingsRequested.AddDynamic(this, &ARailOptimaGameMode::ShowGraphicsSettings);
	}
}

void ARailOptimaGameMode::SetupPauseMenuWidget()
{
	if (PauseMenuWidget)
		return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
		return;

	PauseMenuWidget = CreateWidget<URailPauseMenuWidget>(PC, URailPauseMenuWidget::StaticClass());
	if (PauseMenuWidget)
	{
		PauseMenuWidget->AddToViewport(60);
		PauseMenuWidget->SetVisibility(ESlateVisibility::Collapsed);

		PauseMenuWidget->OnResumeClicked.AddDynamic(this, &ARailOptimaGameMode::HandlePauseResume);
		PauseMenuWidget->OnGraphicsSettingsClicked.AddDynamic(this, &ARailOptimaGameMode::ShowGraphicsSettings);
		PauseMenuWidget->OnSaveProjectClicked.AddDynamic(this, &ARailOptimaGameMode::HandlePauseSave);
		PauseMenuWidget->OnExportExcelClicked.AddDynamic(this, &ARailOptimaGameMode::HandlePauseExportExcel);
		PauseMenuWidget->OnReturnToMainMenuClicked.AddDynamic(this, &ARailOptimaGameMode::HandlePauseReturnToMenu);
		PauseMenuWidget->OnVisualSettingsApplied.AddDynamic(this, &ARailOptimaGameMode::ApplyVisualAndCameraSettings);
	}
}

void ARailOptimaGameMode::SetupProfileWidget()
{
	if (ProfileWidget)
		return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
		return;

	ProfileWidget = CreateWidget<URailProfileWidget>(PC, URailProfileWidget::StaticClass());
	if (ProfileWidget)
	{
		ProfileWidget->AddToViewport(35);
		ProfileWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ARailOptimaGameMode::ToggleProfileWidget()
{
	if (!ProfileWidget)
	{
		SetupProfileWidget();
	}
	if (ProfileWidget)
	{
		ProfileWidget->ToggleVisibilityCustom();
	}
}

void ARailOptimaGameMode::LoadAndApplyVisualSettingsFromConfig()
{
	const float FlySpeed = URailSettingsManager::GetCameraFlySpeed();
	const float BoostMult = URailSettingsManager::GetCameraShiftBoost();
	const float RibbonWidth = URailSettingsManager::GetPathRibbonWidth();
	const float RibbonHeight = URailSettingsManager::GetPathRibbonHeight();
	const float EndpointScale = URailSettingsManager::GetEndpointScaleMultiplier();

	ApplyVisualAndCameraSettings(FlySpeed, BoostMult, RibbonWidth, RibbonHeight, EndpointScale);
}

void ARailOptimaGameMode::ApplyVisualAndCameraSettings(float FlySpeed_cmps, float BoostMultiplier, float PathWidth_cm, float PathHeight_cm, float EndpointScale)
{
	// 1. Параметры скорости полета камеры
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (ARailSpectatorPawn* Pawn = Cast<ARailSpectatorPawn>(PC->GetPawn()))
		{
			Pawn->SetFlightSpeeds(FlySpeed_cmps, BoostMultiplier);
		}
	}

	// 2. Параметры ленты пути
	if (PathRenderer)
	{
		PathRenderer->SetVisualParameters(PathWidth_cm, PathHeight_cm);
	}

	// 3. Масштаб станций и контрольных точек
	const float BaseRadius = 8000.0f * EndpointScale;
	if (StartEndpoint)
	{
		StartEndpoint->SetBaseSphereRadius(BaseRadius);
		StartEndpoint->SetViewScale(StartEndpoint->GetViewScale(), true);
	}
	if (EndEndpoint)
	{
		EndEndpoint->SetBaseSphereRadius(BaseRadius);
		EndEndpoint->SetViewScale(EndEndpoint->GetViewScale(), true);
	}
	for (ARailEndpoint* WP : Waypoints)
	{
		if (IsValid(WP))
		{
			WP->SetBaseSphereRadius(BaseRadius);
			WP->SetViewScale(WP->GetViewScale(), true);
		}
	}

	// Если трасса уже рассчитана, перестраиваем геометрию ленты с новыми габаритами
	if (bIsProjectActive && bGridCached && StartEndpoint && EndEndpoint)
	{
		RecomputePath();
	}
}

void ARailOptimaGameMode::ShowToast(const FString& Message, ERailToastType Type, float DurationSec)
{
	if (GlobalToastWidget)
	{
		GlobalToastWidget->ShowToast(Message, Type, DurationSec);
	}
	else if (MainMenuWidget)
	{
		MainMenuWidget->ShowToast(Message, Type, DurationSec);
	}
}

void ARailOptimaGameMode::OpenProject(const FRailProjectMetadata& InProject)
{
	bIsProjectActive = true;
	CurrentActiveProject = InProject;

	EnsureDefaultEnvironment();

	if (!Terrain)
	{
		FindTerrain();
	}
	if (!PathRenderer)
	{
		SetupPathRenderer();
	}
	if (!StartEndpoint || !EndEndpoint)
	{
		SetupEndpoints();
	}

	if (URailCrashReporter* CrashReporter = URailCrashReporter::Get(this))
	{
		CrashReporter->SetActiveProjectContext(InProject.ProjectName, InProject.CategoryName);
	}

	// Установка категории
	SelectedCategory = InProject.Category;
	ApplySelectedCategory();

	// Очищаем старую сетку, контрольные точки, зоны и геометрию пути
	ClearAllWaypoints();
	ClearAllZones();
	bGridCached = false;
	CachedGrid = FRailGridData();
	if (PathRenderer)
	{
		PathRenderer->ClearPath();
	}

	// Загружаем ландшафт
	if (Terrain)
	{
		bool bLoaded = false;
		if (!InProject.HeightmapPath.IsEmpty() && FPaths::FileExists(InProject.HeightmapPath))
		{
			bLoaded = Terrain->LoadTerrainFromFiles(InProject.HeightmapPath, InProject.MetaPath);
		}

		if (!bLoaded)
		{
			// Пробуем альтернативные пути: внутри папки проекта или в RawContent
			const FString Alt1 = InProject.ProjectPath / FPaths::GetCleanFilename(InProject.HeightmapPath);
			const FString Alt2 = FPaths::ProjectDir() / TEXT("RawContent") / FPaths::GetCleanFilename(InProject.HeightmapPath);
			if (FPaths::FileExists(Alt1))
			{
				bLoaded = Terrain->LoadTerrainFromFiles(Alt1, InProject.MetaPath);
			}
			else if (FPaths::FileExists(Alt2))
			{
				bLoaded = Terrain->LoadTerrainFromFiles(Alt2, InProject.MetaPath);
			}
		}

		if (!bLoaded)
		{
			ShowToast(FString::Printf(TEXT("Внимание: карта высот не найдена: %s"), *InProject.HeightmapPath), ERailToastType::Warning);
		}
		else if (PathRenderer)
		{
			PathRenderer->WorldZScale = Terrain->TerrainParameters.ZScale;
			const FVector Loc = Terrain->GetActorLocation();
			PathRenderer->GridWorldOrigin = FVector(Loc.X, Loc.Y, 0.0f);
		}
	}

	// Загрузка линейных слоев (дороги, реки)
	TArray<AActor*> RoadRenderers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARoadRenderer::StaticClass(), RoadRenderers);

	ARoadRenderer* RoadActor = nullptr;
	ARoadRenderer* RiverActor = nullptr;
	for (AActor* Actor : RoadRenderers)
	{
		ARoadRenderer* RR = Cast<ARoadRenderer>(Actor);
		if (!RR) continue;
		RR->ClearGeometry();
		if (RR->FeatureType == ERoadFeatureType::Road && !RoadActor)
		{
			RoadActor = RR;
		}
		else if (RR->FeatureType == ERoadFeatureType::River && !RiverActor)
		{
			RiverActor = RR;
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (InProject.bHasRoads && !InProject.RoadsPath.IsEmpty() && FPaths::FileExists(InProject.RoadsPath))
	{
		if (!RoadActor)
		{
			RoadActor = GetWorld()->SpawnActor<ARoadRenderer>(ARoadRenderer::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			if (RoadActor)
			{
				RoadActor->FeatureType = ERoadFeatureType::Road;
				RoadActor->Terrain = Terrain;
			}
		}
		if (RoadActor)
		{
			RoadActor->LoadAndBuild(InProject.RoadsPath);
		}
	}

	if (InProject.bHasRivers && !InProject.RiversPath.IsEmpty() && FPaths::FileExists(InProject.RiversPath))
	{
		if (!RiverActor)
		{
			RiverActor = GetWorld()->SpawnActor<ARoadRenderer>(ARoadRenderer::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			if (RiverActor)
			{
				RiverActor->FeatureType = ERoadFeatureType::River;
				RiverActor->RiverColor = FLinearColor(0.05f, 0.45f, 0.95f, 0.95f);
				RiverActor->Terrain = Terrain;
			}
		}
		if (RiverActor)
		{
			RiverActor->LoadAndBuild(InProject.RiversPath);
		}
	}

	// Позиционируем маркеры старта и финиша
	const float MapSize_m = Terrain ? Terrain->GetMapSizeMeters() : 30000.0f;
	const float MapSize_cm = MapSize_m * 100.0f;

	if (StartEndpoint)
	{
		StartEndpoint->SetActorHiddenInGame(false);
		if (!InProject.StartPoint.IsNearlyZero())
		{
			StartEndpoint->SetActorLocation(InProject.StartPoint);
		}
		else
		{
			const FVector2D S2D(MapSize_cm * 0.2f, MapSize_cm * 0.2f);
			float StartZ = 0.0f;
			if (Terrain) Terrain->SampleTerrainZ_cm(S2D, StartZ);
			StartEndpoint->SetActorLocation(FVector(S2D.X, S2D.Y, StartZ + 8000.0f));
		}
	}

	if (EndEndpoint)
	{
		EndEndpoint->SetActorHiddenInGame(false);
		if (!InProject.EndPoint.IsNearlyZero())
		{
			EndEndpoint->SetActorLocation(InProject.EndPoint);
		}
		else
		{
			const FVector2D E2D(MapSize_cm * 0.8f, MapSize_cm * 0.8f);
			float EndZ = 0.0f;
			if (Terrain) Terrain->SampleTerrainZ_cm(E2D, EndZ);
			EndEndpoint->SetActorLocation(FVector(E2D.X, E2D.Y, EndZ + 8000.0f));
		}
	}

	// Центрируем камеру
	if (ARailOptimaPlayerController* PC = Cast<ARailOptimaPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PC->CenterCameraOnTerrain();
	}

	// Показываем виджет сметы
	if (CostSummaryWidget)
	{
		CostSummaryWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// Пересчитываем трассировку
	RecomputePath();

	// Скрываем главное меню и паузу
	if (MainMenuWidget)
	{
		MainMenuWidget->HideLoadingOverlay();
		MainMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (PauseMenuWidget)
	{
		PauseMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	ShowToast(FString::Printf(TEXT("Проект '%s' успешно загружен"), *InProject.ProjectName), ERailToastType::Success);
}

void ARailOptimaGameMode::SaveCurrentProject()
{
	if (!bIsProjectActive)
	{
		ShowToast(TEXT("Нет активного проекта для сохранения"), ERailToastType::Warning);
		return;
	}

	if (StartEndpoint)
	{
		CurrentActiveProject.StartPoint = StartEndpoint->GetActorLocation();
		CurrentActiveProject.bHasSavedEndpoints = true;
	}
	if (EndEndpoint)
	{
		CurrentActiveProject.EndPoint = EndEndpoint->GetActorLocation();
	}

	CurrentActiveProject.TotalLength_m = LastLength_m;
	CurrentActiveProject.TotalCost_kRub = LastCost_kRub;
	CurrentActiveProject.LastModified = FDateTime::Now();

	bool bOk = URailProjectManager::SaveProject(CurrentActiveProject);
	if (bOk)
	{
		ShowToast(FString::Printf(TEXT("Проект '%s' успешно сохранён"), *CurrentActiveProject.ProjectName), ERailToastType::Success);
	}
	else
	{
		ShowToast(TEXT("Ошибка при сохранении проекта!"), ERailToastType::Error);
	}
}

void ARailOptimaGameMode::CloseCurrentProject()
{
	bIsProjectActive = false;

	if (StartEndpoint)
	{
		StartEndpoint->SetActorHiddenInGame(true);
	}
	if (EndEndpoint)
	{
		EndEndpoint->SetActorHiddenInGame(true);
	}
	ClearAllWaypoints();
	SetBrushModeActive(false);

	if (ZoneOverlayRenderer)
	{
		ZoneOverlayRenderer->ClearZones();
	}
	if (PathRenderer)
	{
		PathRenderer->ClearPath();
	}
	if (CostSummaryWidget)
	{
		CostSummaryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (PauseMenuWidget)
	{
		PauseMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	SetupMainMenuWidget();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
}

void ARailOptimaGameMode::ExportCostToExcel()
{
	if (!bIsProjectActive)
	{
		ShowToast(TEXT("Нет активного проекта для экспорта сметы"), ERailToastType::Warning);
		return;
	}

	const FRailCategoryNorms Norms = FRailCategoryNorms::GetNorms(SelectedCategory);
	FString CsvContent;
	CsvContent += FString::Printf(TEXT("СВОДНАЯ ВЕДОМОСТЬ СМЕТНОЙ СТОИМОСТИ СТРОИТЕЛЬСТВА ЖЕЛЕЗНОДОРОЖНОГО ПУТИ\n"));
	CsvContent += FString::Printf(TEXT("Проект;%s\n"), *CurrentActiveProject.ProjectName);
	CsvContent += FString::Printf(TEXT("Категория линии;%s;%s\n"), *Norms.CategoryName, TEXT("СП 119.13330.2017"));
	CsvContent += FString::Printf(TEXT("Нормативный документ ценообразования;НЦС 81-02-07-2025;Минстрой России\n"));
	CsvContent += FString::Printf(TEXT("Дата расчета;%s\n\n"), *FDateTime::Now().ToString(TEXT("%d.%m.%Y %H:%M")));

	CsvContent += TEXT("Параметр;Значение;Единица измерения;Нормативное ограничение\n");
	CsvContent += FString::Printf(TEXT("Длина трассы;%.2f;м;-\n"), LastLength_m);
	CsvContent += FString::Printf(TEXT("Длина трассы;%.3f;км;-\n"), LastLength_m / 1000.0);
	CsvContent += FString::Printf(TEXT("Максимальный продольный уклон;%.1f;‰;не более %.1f ‰\n"), LastMaxSlope_perm, Norms.MaxSlopePermille);
	CsvContent += FString::Printf(TEXT("Минимальный радиус кривой в плане;%.0f;м;не менее %.0f м\n"), LastMinRadius_m, Norms.MinRadiusDifficult);
	CsvContent += FString::Printf(TEXT("Сметная стоимость (НЦС);%.2f;млн. руб.;-\n"), (LastCost_kRub / 1000.0));
	const double AvgRatePerKm = (LastLength_m > 0.0) ? (LastCost_kRub / (LastLength_m / 1000.0)) : 0.0;
	CsvContent += FString::Printf(TEXT("Удельная стоимость;%.2f;млн. руб./км;-\n"), AvgRatePerKm / 1000.0);
	CsvContent += FString::Printf(TEXT("Время трассировки A*;%.1f;мс;-\n"), LastCalcTime_ms);

	const FString ExportFilePath = FPaths::Combine(CurrentActiveProject.ProjectPath, TEXT("estimate_export.csv"));
	bool bSaved = FFileHelper::SaveStringToFile(CsvContent, *ExportFilePath, FFileHelper::EEncodingOptions::ForceUTF8);
	if (bSaved)
	{
		ShowToast(FString::Printf(TEXT("Смета успешно экспортирована в Excel: %s"), *ExportFilePath), ERailToastType::Success, 6.0f);
	}
	else
	{
		ShowToast(TEXT("Не удалось записать файл сметы на диск!"), ERailToastType::Error);
	}
}

void ARailOptimaGameMode::TogglePauseMenu()
{
	if (!bIsProjectActive)
	{
		if (MainMenuWidget)
		{
			MainMenuWidget->CloseOpenModals();
		}
		return;
	}

	if (PauseMenuWidget)
	{
		if (PauseMenuWidget->GetVisibility() == ESlateVisibility::Visible)
		{
			PauseMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			PauseMenuWidget->SetProjectInfo(CurrentActiveProject);
			PauseMenuWidget->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void ARailOptimaGameMode::HandleProjectOpenedFromMenu(const FRailProjectMetadata& ProjectMeta)
{
	OpenProject(ProjectMeta);
}

void ARailOptimaGameMode::HandlePauseResume()
{
	if (PauseMenuWidget)
	{
		PauseMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ARailOptimaGameMode::HandlePauseSave()
{
	SaveCurrentProject();
}

void ARailOptimaGameMode::HandlePauseExportExcel()
{
	ExportCostToExcel();
}

void ARailOptimaGameMode::HandlePauseReturnToMenu()
{
	CloseCurrentProject();
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

	for (ARailEndpoint* WP : Waypoints)
	{
		if (IsValid(WP))
		{
			WP->SetViewScale(CurrentViewScale);
		}
	}

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

void ARailOptimaGameMode::EnsureDefaultEnvironment()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	// Проверяем наличие направленного света (Солнце)
	TArray<AActor*> DirLights;
	UGameplayStatics::GetAllActorsOfClass(World, ADirectionalLight::StaticClass(), DirLights);
	if (DirLights.Num() == 0)
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0.0f, 0.0f, 50000.0f), FRotator(-50.0f, -45.0f, 0.0f), SP);
		if (Sun)
		{
			if (UDirectionalLightComponent* DLC = Sun->GetComponentByClass<UDirectionalLightComponent>())
			{
				DLC->SetIntensity(10.0f);
				DLC->SetAtmosphereSunLight(true);
				DLC->SetLightColor(FLinearColor(1.0f, 0.98f, 0.92f));
			}
		}
	}

	// Проверяем наличие небесного света (SkyLight)
	TArray<AActor*> SkyLights;
	UGameplayStatics::GetAllActorsOfClass(World, ASkyLight::StaticClass(), SkyLights);
	if (SkyLights.Num() == 0)
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ASkyLight* Sky = World->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector(0.0f, 0.0f, 60000.0f), FRotator::ZeroRotator, SP);
		if (Sky && Sky->GetLightComponent())
		{
			Sky->GetLightComponent()->SetIntensity(2.0f);
			Sky->GetLightComponent()->bRealTimeCapture = true;
			Sky->GetLightComponent()->SetLightColor(FLinearColor(0.85f, 0.92f, 1.0f));
		}
	}
}

void ARailOptimaGameMode::FindTerrain()
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProceduralTerrain::StaticClass(), Found);
	if (Found.Num() > 0)
	{
		Terrain = Cast<AProceduralTerrain>(Found[0]);
	}
	else if (!Terrain)
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Terrain = GetWorld()->SpawnActor<AProceduralTerrain>(AProceduralTerrain::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SP);
		UE_LOG(LogTemp, Log, TEXT("[RailGM] AProceduralTerrain автоматически создан в мире."));
	}
}

void ARailOptimaGameMode::SetupPathRenderer()
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APathRenderer::StaticClass(), Found);
	if (Found.Num() > 0)
	{
		PathRenderer = Cast<APathRenderer>(Found[0]);
	}
	else if (!PathRenderer)
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		PathRenderer = GetWorld()->SpawnActor<APathRenderer>(APathRenderer::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SP);
		UE_LOG(LogTemp, Log, TEXT("[RailGM] APathRenderer автоматически создан в мире."));
	}

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

	// Собираем все контрольные точки маршрута: Start -> Waypoints[0..N] -> End
	TArray<ARailEndpoint*> SegPoints;
	if (StartEndpoint && !StartEndpoint->IsHidden())
	{
		SegPoints.Add(StartEndpoint);
	}
	for (ARailEndpoint* WP : Waypoints)
	{
		if (IsValid(WP) && !WP->IsHidden())
		{
			SegPoints.Add(WP);
		}
	}
	if (EndEndpoint && !EndEndpoint->IsHidden())
	{
		SegPoints.Add(EndEndpoint);
	}

	const FRailCategoryNorms Norms = FRailCategoryNorms::GetNorms(SelectedCategory);

	if (SegPoints.Num() < 2)
	{
		LastLength_m = 0.0;
		LastCost_kRub = 0.0;
		LastMaxSlope_perm = 0.0f;
		LastMinRadius_m = 0.0f;
		LastCalcTime_ms = 0.0;

		PathRenderer->ClearPath();
		if (ProfileWidget)
		{
			ProfileWidget->UpdateProfileData(TArray<FRailProfilePoint>(), 0.0f, 0.0f, 0.0f, 0.0f);
		}
		if (CostSummaryWidget)
		{
			CostSummaryWidget->UpdateRouteSummary(Norms.CategoryName, 0.0, 0.0, 0.0f, 0.0f, 0.0);
		}
		OnPathRecomputed.Broadcast(0.0, 0.0, 0.0, 0);
		return;
	}

	// 1. Построение сквозного маршрута по всем промежуточным секциям
	PathFinder->SetCategoryNorms(Norms);

	const double T0 = FPlatformTime::Seconds();
	FRailPathResult CompositeResult;
	CompositeResult.bFound = true;
	CompositeResult.TotalCost = 0.0f;
	CompositeResult.TotalLength_m = 0.0f;

	for (int32 s = 0; s < SegPoints.Num() - 1; ++s)
	{
		const int32 SegStartIdx = WorldToNodeIndex(CachedGrid, SegPoints[s]->GetActorLocation());
		const int32 SegEndIdx = WorldToNodeIndex(CachedGrid, SegPoints[s + 1]->GetActorLocation());

		if (SegStartIdx == INDEX_NONE || SegEndIdx == INDEX_NONE)
		{
			CompositeResult.bFound = false;
			break;
		}

		if (SegStartIdx == SegEndIdx)
		{
			continue;
		}

		FRailPathResult SegRes = PathFinder->FindPath(CachedGrid, SegStartIdx, SegEndIdx);

		// Если по нормативу не сошлось — пробуем relaxed fallback (горный/тоннельный допуск)
		if (!SegRes.bFound)
		{
			FRailCategoryNorms RelaxedNorms = Norms;
			RelaxedNorms.MaxSlopePermille = 150.0f;
			PathFinder->SetCategoryNorms(RelaxedNorms);
			SegRes = PathFinder->FindPath(CachedGrid, SegStartIdx, SegEndIdx);
			PathFinder->SetCategoryNorms(Norms);
		}

		if (!SegRes.bFound)
		{
			CompositeResult.bFound = false;
			break;
		}

		if (CompositeResult.NodeIndices.Num() == 0)
		{
			CompositeResult.NodeIndices = SegRes.NodeIndices;
		}
		else
		{
			// Пропускаем нулевой элемент, так как он совпадает с концом предыдущего сегмента
			for (int32 p = 1; p < SegRes.NodeIndices.Num(); ++p)
			{
				CompositeResult.NodeIndices.Add(SegRes.NodeIndices[p]);
			}
		}

		CompositeResult.TotalCost += SegRes.TotalCost;
		CompositeResult.TotalLength_m += SegRes.TotalLength_m;
		CompositeResult.EdgeCosts.Append(SegRes.EdgeCosts);
	}

	const double Ms = (FPlatformTime::Seconds() - T0) * 1000.0;
	FRailPathResult Result = MoveTemp(CompositeResult);

	if (!Result.bFound || Result.NodeIndices.Num() < 2)
	{
		LastLength_m = 0.0;
		LastCost_kRub = 0.0;
		LastMaxSlope_perm = 0.0f;
		LastMinRadius_m = 0.0f;
		LastCalcTime_ms = Ms;

		PathRenderer->ClearPath();
		if (ProfileWidget)
		{
			ProfileWidget->UpdateProfileData(TArray<FRailProfilePoint>(), 0.0f, 0.0f, 0.0f, 0.0f);
		}
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
	LastSmoothTrackPoints = SmoothPoints;

	// 4. Валидация по СП 119.13330 (проверка построенной 3D трассы)
	if (PathValidator && SmoothPoints.Num() > 1)
	{
		const float ZScaleVal = (PathRenderer && PathRenderer->WorldZScale > 0.0f) ? PathRenderer->WorldZScale : 1.0f;
		const FRailValidationResult ValRes = PathValidator->ValidateWorldTrack(SmoothPoints, Norms, ZScaleVal);
		if (!ValRes.bIsValid && ValRes.Violations.Num() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RailGM] Отступления от СП 119.13330 (требуется ТЭО/ИССО): %d. Пример: %s"),
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

		// Обновляем данные проекции высот продольного профиля (F1)
		if (SmoothPoints.Num() > 1)
		{
			LastProfilePoints.Reset();
			LastProfilePoints.Reserve(SmoothPoints.Num());

			const float ZOff_cm = PathRenderer ? PathRenderer->ZOffset_cm : 8000.0f;

			float CumulativeDist_m = 0.0f;
			float MinH = TNumericLimits<float>::Max();
			float MaxH = TNumericLimits<float>::Lowest();

			for (int32 i = 0; i < SmoothPoints.Num(); ++i)
			{
				if (i > 0)
				{
					CumulativeDist_m += FVector2D::Distance(
						FVector2D(SmoothPoints[i - 1].X, SmoothPoints[i - 1].Y),
						FVector2D(SmoothPoints[i].X, SmoothPoints[i].Y)) * 0.01f;
				}

				FRailProfilePoint Pt;
				Pt.DistanceAlongRoute_m = CumulativeDist_m;
				Pt.TrackHeight_m = (SmoothPoints[i].Z - ZOff_cm) * 0.01f / ZScale;

				float TerrainZ_cm = 0.0f;
				if (Terrain && Terrain->SampleTerrainZ_cm(FVector2D(SmoothPoints[i].X, SmoothPoints[i].Y), TerrainZ_cm))
				{
					Pt.TerrainHeight_m = TerrainZ_cm * 0.01f / ZScale;
				}
				else
				{
					Pt.TerrainHeight_m = Pt.TrackHeight_m;
				}

				MinH = FMath::Min(MinH, FMath::Min(Pt.TrackHeight_m, Pt.TerrainHeight_m));
				MaxH = FMath::Max(MaxH, FMath::Max(Pt.TrackHeight_m, Pt.TerrainHeight_m));

				LastProfilePoints.Add(Pt);
			}

			if (ProfileWidget)
			{
				ProfileWidget->UpdateProfileData(LastProfilePoints, MinH, MaxH, CumulativeDist_m, MaxSlopePermille);
			}
		}
	}
	else
	{
		PathRenderer->ClearPath();
		if (ProfileWidget)
		{
			ProfileWidget->UpdateProfileData(TArray<FRailProfilePoint>(), 0.0f, 0.0f, 0.0f, 0.0f);
		}
	}

	// 6. Обновляем UI-табличку со сметой НЦС 81-02-07-2025
	LastLength_m = Result.TotalLength_m;
	LastCost_kRub = Result.TotalCost;
	LastMaxSlope_perm = MaxSlopePermille;
	LastMinRadius_m = MinPlanRadius_m;
	LastCalcTime_ms = Ms;

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
	// Допуск подбирается пропорционально шагу сетки (30-50 м), чтобы не срезать плавные изгибы в острые углы
	const float		  RDP_Tolerance = FMath::Clamp(InGrid.StepSize_m * 45.0f, 3000.0f, 5000.0f);
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
			float TargetRadius_m = InNorms.PickStandardRadius(MaxFittingRadius_m);
			if (TargetRadius_m <= 0.0f || (TargetRadius_m * 100.0f * HalfTurnTan > MaxTangent_cm + 1.0f))
			{
				TargetRadius_m = FMath::Max(MaxFittingRadius_m, 50.0f);
			}

			const float R_cm = TargetRadius_m * 100.0f;
			const float T_cm = FMath::Min(R_cm * HalfTurnTan, MaxTangent_cm);

			if (T_cm > 100.0f && R_cm > 2000.0f)
			{
				BestMinRadius_m = FMath::Min(BestMinRadius_m, TargetRadius_m);

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
			else if (MaxTangent_cm > 100.0f)
			{
				// Плавное квадратичное скругление Безье для предотвращения любых острых углов
				const FVector2D T1 = PCurr - V1 * MaxTangent_cm;
				const FVector2D T2 = PCurr + V2 * MaxTangent_cm;

				if (FVector2D::Distance(Final2D.Last(), T1) > 10.0f)
				{
					Final2D.Add(T1);
				}

				const int32 NumBezSteps = 8;
				for (int32 b = 1; b <= NumBezSteps; ++b)
				{
					const float t = static_cast<float>(b) / static_cast<float>(NumBezSteps);
					const float omt = 1.0f - t;
					const FVector2D BezPt = omt * omt * T1 + 2.0f * omt * t * PCurr + t * t * T2;
					Final2D.Add(BezPt);
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
void ARailOptimaGameMode::SetupGraphicsSettingsWidget()
{
	if (GraphicsSettingsWidget)
		return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
		return;

	GraphicsSettingsWidget = CreateWidget<URailGraphicsSettingsWidget>(PC, URailGraphicsSettingsWidget::StaticClass());
	if (GraphicsSettingsWidget)
	{
		GraphicsSettingsWidget->AddToViewport(120);
		GraphicsSettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
		GraphicsSettingsWidget->OnSettingsApplied.AddDynamic(this, &ARailOptimaGameMode::HandleGraphicsSettingsApplied);
	}
}

void ARailOptimaGameMode::ShowGraphicsSettings()
{
	if (!GraphicsSettingsWidget)
	{
		SetupGraphicsSettingsWidget();
	}
	if (GraphicsSettingsWidget)
	{
		GraphicsSettingsWidget->OpenSettingsModal();
	}
}

void ARailOptimaGameMode::HandleGraphicsSettingsApplied()
{
	ShowToast(TEXT("Настройки графики успешно применены и сохранены"), ERailToastType::Success, 3.5f);
}

// ─────────────────────────────────────────────────────────────────
//  Промежуточные контрольные точки (Waypoints)
// ─────────────────────────────────────────────────────────────────

ARailEndpoint* ARailOptimaGameMode::AddWaypointAtLocation(const FVector& WorldLocation, int32 InsertIndex)
{
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UClass* EndpointClass = StartEndpoint ? StartEndpoint->GetClass() : ARailEndpoint::StaticClass();
	ARailEndpoint* WP = GetWorld()->SpawnActor<ARailEndpoint>(EndpointClass, WorldLocation, FRotator::ZeroRotator, SP);
	if (!WP)
	{
		return nullptr;
	}

	WP->bIsStart = false;
	WP->bIsWaypoint = true;
	if (StartEndpoint)
	{
		WP->SetActorScale3D(StartEndpoint->GetActorScale3D());
		WP->SetBaseSphereRadius(StartEndpoint->GetBaseSphereRadius());
		WP->SphereRadius_cm = StartEndpoint->SphereRadius_cm;
		WP->TerrainZOffset_cm = StartEndpoint->TerrainZOffset_cm;
		WP->SetViewScale(StartEndpoint->GetViewScale(), /*bForce=*/true);
	}
	else
	{
		WP->SetViewScale(10.0f, /*bForce=*/true);
	}
	WP->SetActorLocation(WorldLocation);
	WP->SetCustomColor(FLinearColor(1.0f, 0.72f, 0.05f, 1.0f)); // Янтарно-желтый
	WP->OnEndpointMoved.AddDynamic(this, &ARailOptimaGameMode::HandleEndpointMoved);

	if (InsertIndex >= 0 && InsertIndex <= Waypoints.Num())
	{
		Waypoints.Insert(WP, InsertIndex);
	}
	else
	{
		Waypoints.Add(WP);
	}

	ShowToast(FString::Printf(TEXT("Добавлена контрольная точка #%d (ПКМ на точку для удаления)"), Waypoints.Num()), ERailToastType::Success, 3.0f);
	RecomputePath();
	return WP;
}

bool ARailOptimaGameMode::TryAddWaypointNearLocation(const FVector& WorldLocation)
{
	if (LastSmoothTrackPoints.Num() < 2)
	{
		AddWaypointAtLocation(WorldLocation);
		return true;
	}

	float MinDistSq = MAX_flt;
	int32 BestSegmentIndex = 0;

	for (int32 i = 0; i < LastSmoothTrackPoints.Num() - 1; ++i)
	{
		const FVector P1 = LastSmoothTrackPoints[i];
		const FVector P2 = LastSmoothTrackPoints[i + 1];
		const FVector ClosestPoint = FMath::ClosestPointOnSegment(WorldLocation, P1, P2);
		const float DistSq = FVector::DistSquared(WorldLocation, ClosestPoint);
		if (DistSq < MinDistSq)
		{
			MinDistSq = DistSq;
			BestSegmentIndex = i;
		}
	}

	// Порог клика: в пределах 250 метров от полотна
	const float MaxThresholdSq = FMath::Square(25000.0f);
	if (MinDistSq < MaxThresholdSq)
	{
		AddWaypointAtLocation(WorldLocation);
		return true;
	}

	return false;
}

bool ARailOptimaGameMode::RemoveWaypoint(ARailEndpoint* InWaypoint)
{
	if (!InWaypoint || !Waypoints.Contains(InWaypoint))
	{
		return false;
	}

	InWaypoint->OnEndpointMoved.RemoveAll(this);
	Waypoints.Remove(InWaypoint);
	InWaypoint->Destroy();

	ShowToast(TEXT("Контрольная точка удалена"), ERailToastType::Info, 2.5f);
	RecomputePath();
	return true;
}

void ARailOptimaGameMode::ClearAllWaypoints()
{
	for (ARailEndpoint* WP : Waypoints)
	{
		if (IsValid(WP))
		{
			WP->OnEndpointMoved.RemoveAll(this);
			WP->Destroy();
		}
	}
	Waypoints.Empty();
}

// ─────────────────────────────────────────────────────────────────
//  Зоны и интерактивная кисть
// ─────────────────────────────────────────────────────────────────

void ARailOptimaGameMode::SetActiveZoneType(ERailZoneType NewType)
{
	ActiveZoneType = NewType;
	if (BrushWidget)
	{
		BrushWidget->UpdateZoneTypeDisplay(ActiveZoneType);
	}
}

void ARailOptimaGameMode::SetBrushRadius_m(float InRadius_m)
{
	BrushRadius_m = FMath::Clamp(InRadius_m, 25.0f, 1000.0f);
	if (BrushWidget)
	{
		BrushWidget->UpdateRadiusDisplay(BrushRadius_m);
	}
}

void ARailOptimaGameMode::SetBrushModeActive(bool bActive)
{
	bBrushModeActive = bActive;
	if (bBrushModeActive)
	{
		if (!BrushWidget)
		{
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			if (PC)
			{
				BrushWidget = CreateWidget<URailBrushWidget>(PC, URailBrushWidget::StaticClass());
				if (BrushWidget)
				{
					BrushWidget->AddToViewport(15);
				}
			}
		}

		if (BrushWidget)
		{
			BrushWidget->SetVisibility(ESlateVisibility::Visible);
			BrushWidget->UpdateRadiusDisplay(BrushRadius_m);
			BrushWidget->UpdateZoneTypeDisplay(ActiveZoneType);
		}

		const TCHAR* ZoneName = (ActiveZoneType == ERailZoneType::ForbiddenRed)
			? TEXT("Запретная зона (Красная)")
			: TEXT("Ластик");
		ShowToast(FString::Printf(TEXT("Режим кисти: %s. Рисуйте зажатием ЛКМ на карте (B для выхода)"), ZoneName), ERailToastType::Info, 3.5f);
	}
	else
	{
		if (BrushWidget)
		{
			BrushWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		ShowToast(TEXT("Режим кисти отключен"), ERailToastType::Info, 2.0f);
	}
}

void ARailOptimaGameMode::PaintZoneAtLocation(const FVector& WorldLocation)
{
	if (!bGridCached || CachedGrid.Width <= 0 || CachedGrid.Height <= 0)
	{
		return;
	}

	const FVector Origin = PathRenderer ? PathRenderer->GridWorldOrigin : FVector::ZeroVector;
	const float Step_cm = CachedGrid.StepSize_m * 100.0f;
	if (Step_cm <= 0.0f) return;

	const float CenterGridX = (WorldLocation.X - Origin.X) / Step_cm;
	const float CenterGridY = (WorldLocation.Y - Origin.Y) / Step_cm;
	const float RadiusGrid = (BrushRadius_m * 100.0f) / Step_cm;
	const float RadiusGridSq = RadiusGrid * RadiusGrid;

	const int32 MinX = FMath::Clamp(FMath::FloorToInt(CenterGridX - RadiusGrid), 0, CachedGrid.Width - 1);
	const int32 MaxX = FMath::Clamp(FMath::CeilToInt(CenterGridX + RadiusGrid), 0, CachedGrid.Width - 1);
	const int32 MinY = FMath::Clamp(FMath::FloorToInt(CenterGridY - RadiusGrid), 0, CachedGrid.Height - 1);
	const int32 MaxY = FMath::Clamp(FMath::CeilToInt(CenterGridY + RadiusGrid), 0, CachedGrid.Height - 1);

	bool bModified = false;
	for (int32 Y = MinY; Y <= MaxY; ++Y)
	{
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			const float DistSq = FMath::Square(static_cast<float>(X) - CenterGridX) + FMath::Square(static_cast<float>(Y) - CenterGridY);
			if (DistSq <= RadiusGridSq)
			{
				const int32 NodeIdx = CachedGrid.IndexOf(X, Y);
				if (CachedGrid.Nodes.IsValidIndex(NodeIdx))
				{
					FRailGridNode& Node = CachedGrid.Nodes[NodeIdx];
					if (ActiveZoneType == ERailZoneType::ForbiddenRed)
					{
						if (!Node.bIsForbidden)
						{
							Node.bIsForbidden = true;
							Node.PriorityModifier = 1.0f;
							bModified = true;
						}
					}
					else // Eraser
					{
						if (Node.bIsForbidden)
						{
							Node.bIsForbidden = false;
							Node.PriorityModifier = 1.0f;
							bModified = true;
						}
					}
				}
			}
		}
	}

	if (bModified)
	{
		UpdateZoneOverlay();
	}
}

void ARailOptimaGameMode::SetupZoneOverlayRenderer()
{
	if (ZoneOverlayRenderer)
		return;

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARailZoneOverlayRenderer::StaticClass(), Found);
	if (Found.Num() > 0)
	{
		ZoneOverlayRenderer = Cast<ARailZoneOverlayRenderer>(Found[0]);
	}
	else
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ZoneOverlayRenderer = GetWorld()->SpawnActor<ARailZoneOverlayRenderer>(
			ARailZoneOverlayRenderer::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SP);
	}
}

void ARailOptimaGameMode::ClearAllZones()
{
	if (!bGridCached) return;

	for (FRailGridNode& Node : CachedGrid.Nodes)
	{
		Node.bIsForbidden = false;
		Node.PriorityModifier = 1.0f;
	}

	if (ZoneOverlayRenderer)
	{
		ZoneOverlayRenderer->ClearZones();
	}

	UpdateZoneOverlay();
	RecomputePath();
	ShowToast(TEXT("Все запретные зоны очищены"), ERailToastType::Info, 2.5f);
}

void ARailOptimaGameMode::UpdateZoneOverlay()
{
	SetupZoneOverlayRenderer();
	if (!ZoneOverlayRenderer || !bGridCached || CachedGrid.Width <= 0 || CachedGrid.Height <= 0)
	{
		return;
	}

	const FVector Origin = PathRenderer ? PathRenderer->GridWorldOrigin : FVector::ZeroVector;
	ZoneOverlayRenderer->UpdateZones(CachedGrid, Origin, Terrain);
}
