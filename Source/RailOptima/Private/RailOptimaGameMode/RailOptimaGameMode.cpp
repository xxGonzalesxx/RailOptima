// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#include "RailOptimaGameMode/RailOptimaGameMode.h"
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
#include "Services/RailProjectManager.h"
#include "TimerManager.h"
#include "UI/RailMainMenuWidget.h"
#include "UI/RailPauseMenuWidget.h"
#include "UI/RailGraphicsSettingsWidget.h"
#include "Services/RailSettingsManager.h"
#include "UI/RailToastWidget.h"
#include "Visualization/PathRenderer.h"
#include "Visualization/RailCostSummaryWidget.h"
#include "Visualization/RoadRenderer.h"

ARailOptimaGameMode::ARailOptimaGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
	PlayerControllerClass = ARailOptimaPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void ARailOptimaGameMode::BeginPlay()
{
	Super::BeginPlay();
	SetupSubsystems();
	FindTerrain();
	SetupPathRenderer();
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

	// Установка категории
	SelectedCategory = InProject.Category;
	ApplySelectedCategory();

	// Очищаем старую сетку и геометрию пути
	bGridCached = false;
	CachedGrid = FRailGridData();
	if (PathRenderer)
	{
		PathRenderer->ClearPath();
	}

	// Загружаем ландшафт
	if (Terrain)
	{
		bool bLoaded = Terrain->LoadTerrainFromFiles(InProject.HeightmapPath, InProject.MetaPath);
		if (!bLoaded)
		{
			ShowToast(FString::Printf(TEXT("Внимание: карта высот не найдена: %s"), *InProject.HeightmapPath), ERailToastType::Warning);
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

	if (InProject.bHasRoads && !InProject.RoadsPath.IsEmpty() && FPaths::FileExists(InProject.RoadsPath))
	{
		if (RoadActor)
		{
			RoadActor->LoadAndBuild(InProject.RoadsPath);
		}
	}

	if (InProject.bHasRivers && !InProject.RiversPath.IsEmpty() && FPaths::FileExists(InProject.RiversPath))
	{
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

void ARailOptimaGameMode::FindTerrain()
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProceduralTerrain::StaticClass(), Found);
	if (Found.Num() > 0)
		Terrain = Cast<AProceduralTerrain>(Found[0]);
}

void ARailOptimaGameMode::SetupPathRenderer()
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APathRenderer::StaticClass(), Found);
	if (Found.Num() > 0)
		PathRenderer = Cast<APathRenderer>(Found[0]);

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

		if (bGridCached)
		{
			TArray<float> Slopes;
			Slopes.Reserve(CachedGrid.Width * CachedGrid.Height * 2);
			float MinH = TNumericLimits<float>::Max();
			float MaxH = -TNumericLimits<float>::Max();
			for (int32 Y = 0; Y < CachedGrid.Height - 1; ++Y)
			{
				for (int32 X = 0; X < CachedGrid.Width - 1; ++X)
				{
					const float H = CachedGrid.Nodes[CachedGrid.IndexOf(X, Y)].Height_m;
					const float Hx = CachedGrid.Nodes[CachedGrid.IndexOf(X + 1, Y)].Height_m;
					const float Hy = CachedGrid.Nodes[CachedGrid.IndexOf(X, Y + 1)].Height_m;
					MinH = FMath::Min(MinH, H);
					MaxH = FMath::Max(MaxH, H);
					Slopes.Add(FMath::Abs(Hx - H) / CachedGrid.StepSize_m * 1000.0f);
					Slopes.Add(FMath::Abs(Hy - H) / CachedGrid.StepSize_m * 1000.0f);
				}
			}
			Slopes.Sort();
			const int32 N = Slopes.Num();
			int32 Below15 = 0;
			for (float S : Slopes)
			{
				if (S <= 15.0f) ++Below15;
			}
			UE_LOG(LogTemp, Warning,
				TEXT("[PERF] Рельеф сетки: высоты %.0f..%.0f м | уклон ребра, ‰: p10=%.1f, медиана=%.1f, p90=%.1f, макс=%.1f | рёбер <=15‰: %.1f%%"),
				MinH, MaxH, Slopes[N / 10], Slopes[N / 2], Slopes[N * 9 / 10], Slopes[N - 1],
				100.0 * Below15 / FMath::Max(N, 1));
		}
	}

	if (!bGridCached)
		return;

	const int32 StartIdx = WorldToNodeIndex(CachedGrid, StartEndpoint->GetActorLocation());
	const int32 EndIdx = WorldToNodeIndex(CachedGrid, EndEndpoint->GetActorLocation());

	const FRailCategoryNorms Norms = FRailCategoryNorms::GetNorms(SelectedCategory);

	if (StartIdx == INDEX_NONE || EndIdx == INDEX_NONE || StartIdx == EndIdx)
	{
		LastLength_m = 0.0;
		LastCost_kRub = 0.0;
		LastMaxSlope_perm = 0.0f;
		LastMinRadius_m = 0.0f;
		LastCalcTime_ms = 0.0;

		PathRenderer->ClearPath();
		if (CostSummaryWidget)
		{
			CostSummaryWidget->UpdateRouteSummary(Norms.CategoryName, 0.0, 0.0, 0.0f, 0.0f, 0.0);
		}
		OnPathRecomputed.Broadcast(0.0, 0.0, 0.0, 0);
		return;
	}

	// 1. Сначала ищем путь в строгом соответствии с нормативом уклона (15‰)
	PathFinder->SetCategoryNorms(Norms);

	const double	T0 = FPlatformTime::Seconds();
	FRailPathResult Result = PathFinder->FindPath(CachedGrid, StartIdx, EndIdx);
	const double FirstSearchMs = (FPlatformTime::Seconds() - T0) * 1000.0;
	const bool	 bFirstFound = Result.bFound;
	const int32	 FirstExpanded = Result.ExpandedNodes;

	// 2. Если на крутом горном рельефе прямой путь <= 15‰ невозможен, делаем fallback
	// с допуском горных/тоннельных участков (с высокими надбавками НЦС за мосты/тоннели)
	if (!Result.bFound)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PERF] ФОЛЛБЭК: первый поиск не нашёл путь за %.1f мс (расширено %d из %d узлов)."),
			FirstSearchMs, FirstExpanded, CachedGrid.Nodes.Num());

		FRailCategoryNorms RelaxedNorms = Norms;
		RelaxedNorms.MaxSlopePermille = 150.0f;	
		PathFinder->SetCategoryNorms(RelaxedNorms);
		Result = PathFinder->FindPath(CachedGrid, StartIdx, EndIdx);
		PathFinder->SetCategoryNorms(Norms);
	}

	const double Ms = (FPlatformTime::Seconds() - T0) * 1000.0;

	if (!Result.bFound)
	{
		LastLength_m = 0.0;
		LastCost_kRub = 0.0;
		LastMaxSlope_perm = 0.0f;
		LastMinRadius_m = 0.0f;
		LastCalcTime_ms = Ms;

		PathRenderer->ClearPath();
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
	const double	TArc0 = FPlatformTime::Seconds();
	TArray<FVector> SmoothPoints = GenerateRailwayArcs(CachedGrid, Result, Norms, MinPlanRadius_m, MaxSlopePermille);
	const double	ArcsMs = (FPlatformTime::Seconds() - TArc0) * 1000.0;
	double			RenderMs = 0.0;	

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

		const double TRen0 = FPlatformTime::Seconds();
		PathRenderer->RenderCustomPath(SmoothPoints, Colors);
		RenderMs = (FPlatformTime::Seconds() - TRen0) * 1000.0;
	}
	else
	{
		PathRenderer->ClearPath();
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


	 UE_LOG(LogTemp, Warning,
		TEXT("[PERF] сетка=%dx%d (%d узлов), шаг=%.0f м | A* №1: %.1f мс, расширено=%d, найден=%d | A* итого (с фоллбэком): %.1f мс, расширено=%d | дуги: %.1f мс | рендер: %.1f мс"),
		CachedGrid.Width, CachedGrid.Height, CachedGrid.Nodes.Num(), CachedGrid.StepSize_m,
		FirstSearchMs, FirstExpanded, bFirstFound ? 1 : 0,
		Ms, Result.ExpandedNodes, ArcsMs, RenderMs);

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
	const float		  RDP_Tolerance = 15000.0f; // 150 м
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
			const float SelectedStandardRadius_m = InNorms.PickStandardRadius(MaxFittingRadius_m);

			if (SelectedStandardRadius_m > 0.0f && SelectedStandardRadius_m <= MaxFittingRadius_m)
			{
				const float R_cm = SelectedStandardRadius_m * 100.0f;
				const float T_cm = R_cm * HalfTurnTan;

				if (T_cm <= MaxTangent_cm + 1.0f)
				{
					BestMinRadius_m = FMath::Min(BestMinRadius_m, SelectedStandardRadius_m);

					if (SelectedStandardRadius_m < 300.0f)
					{
						UE_LOG(LogTemp, Warning,
							TEXT("[ArcGen] Малый радиус R=%.0f м (< 300 м): по СП 119.13330 требуется ТЭО и ограничение скорости!"),
							SelectedStandardRadius_m);
					}

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
