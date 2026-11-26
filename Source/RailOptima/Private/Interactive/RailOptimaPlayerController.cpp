// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#include "Interactive/RailOptimaPlayerController.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Core/ProceduralTerrain.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Interactive/RailEndpoint.h"
#include "Interactive/RailSpectatorPawn.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "RailOptimaGameMode/RailOptimaGameMode.h" // ← ИСПРАВЛЕНО


// ─────────────────────────────────────────────────────────────────
//  Жизненный цикл
// ─────────────────────────────────────────────────────────────────

ARailOptimaPlayerController::ARailOptimaPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ARailOptimaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
		{
			if (DefaultMappingContext)
			{
				Subsystem->ClearAllMappings();
				Subsystem->AddMappingContext(DefaultMappingContext, MappingContextPriority);

				UE_LOG(LogTemp, Log,
					TEXT("[RailPC] IMC добавлен: '%s' (priority=%d)."),
					*DefaultMappingContext->GetName(), MappingContextPriority);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[RailPC] DefaultMappingContext не назначен."));
			}
		}
	}

	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);

	UE_LOG(LogTemp, Log, TEXT("[RailPC] BeginPlay: IMC добавлен, курсор показан."));

	SubscribedPawn = Cast<ARailSpectatorPawn>(GetPawn());
	if (SubscribedPawn)
	{
		SubscribedPawn->OnPrimaryPressed.AddUObject(this, &ARailOptimaPlayerController::HandlePrimaryPressed);
		SubscribedPawn->OnPrimaryReleased.AddUObject(this, &ARailOptimaPlayerController::HandlePrimaryReleased);

		UE_LOG(LogTemp, Log,
			TEXT("[RailPC] Подписка на делегаты павна '%s': OnPrimaryPressed/Released."),
			*SubscribedPawn->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[RailPC] Pawn не ARailSpectatorPawn."));
		return;
	}

	CenterCameraOnTerrain();
}

void ARailOptimaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent)
	{
		// Прямая привязка ЛКМ и ПКМ для гарантированной обработки кликов и драга маркеров
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ARailOptimaPlayerController::HandlePrimaryPressed);
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &ARailOptimaPlayerController::HandlePrimaryReleased);
		InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ARailOptimaPlayerController::HandleSecondaryPressed);

		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ARailOptimaPlayerController::HandleEscapePressed);
		InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &ARailOptimaPlayerController::HandleProfileToggle);
		InputComponent->BindKey(EKeys::B, IE_Pressed, this, &ARailOptimaPlayerController::HandleBrushModeToggle);
		InputComponent->BindKey(EKeys::LeftBracket, IE_Pressed, this, &ARailOptimaPlayerController::HandleBrushRadiusDecrease);
		InputComponent->BindKey(EKeys::RightBracket, IE_Pressed, this, &ARailOptimaPlayerController::HandleBrushRadiusIncrease);
	}
}

void ARailOptimaPlayerController::HandleEscapePressed()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->TogglePauseMenu();
	}
}

void ARailOptimaPlayerController::HandleProfileToggle()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->ToggleProfileWidget();
	}
}

void ARailOptimaPlayerController::CenterCameraOnTerrain()
{
	if (!SubscribedPawn)
	{
		SubscribedPawn = Cast<ARailSpectatorPawn>(GetPawn());
	}
	if (!SubscribedPawn)
	{
		return;
	}

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProceduralTerrain::StaticClass(), Found);

	if (Found.Num() > 0)
	{
		const AProceduralTerrain* Terrain = Cast<AProceduralTerrain>(Found[0]);
		if (Terrain)
		{
			const FRailHeightMapData& Map = Terrain->GetHeightMapData();
			float SideX_cm = Map.Width * static_cast<float>(Map.PixelSize_m) * 100.0f;
			float SideY_cm = Map.Height * static_cast<float>(Map.PixelSize_m) * 100.0f;
			float SideZ_cm = (Map.MaxElevation_m - Map.MinElevation_m) * 100.0f;

			if (SideX_cm <= 0.0f || SideY_cm <= 0.0f)
			{
				const float MapMeters = Terrain->GetMapSizeMeters();
				SideX_cm = (MapMeters > 0.0f ? MapMeters : 15360.0f) * 100.0f;
				SideY_cm = (MapMeters > 0.0f ? MapMeters : 15360.0f) * 100.0f;
				SideZ_cm = 50000.0f;
			}

			SubscribedPawn->CenterOnWorld(SideX_cm, SideY_cm, SideZ_cm);

			UE_LOG(LogTemp, Log,
				TEXT("[RailPC] Pawn отцентрован: (%.0f, %.0f, %.0f) см."),
				SideX_cm, SideY_cm, SideZ_cm);
		}
	}
	else
	{
		SubscribedPawn->CenterOnWorld(3000000.0f, 3000000.0f, 100000.0f);
		UE_LOG(LogTemp, Warning, TEXT("[RailPC] Рельеф не найден, павн по дефолту."));
	}
}

void ARailOptimaPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	++TraceFrameCounter;

	// ── Динамическое масштабирование пути и маркеров ─────────────
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->ApplyViewScaleFromCamera(SubscribedPawn);
	}

	// ── Режим кисти зон: динамический индикатор радиуса и непрерывное рисование ──
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (GM->IsBrushModeActive())
		{
			FVector HitLocation;
			if (TraceTerrainUnderCursor(HitLocation))
			{
				const float Radius_cm = GM->GetBrushRadius_m() * 100.0f;
				const FColor CircleColor = (GM->GetActiveZoneType() == ERailZoneType::Eraser)
					? FColor(240, 240, 240)
					: FColor(255, 30, 30);

				DrawDebugCircle(GetWorld(), HitLocation + FVector(0.0f, 0.0f, 30.0f), Radius_cm, 64, CircleColor, false, -1.0f, 0, 160.0f, FVector(1, 0, 0), FVector(0, 1, 0));
				DrawDebugPoint(GetWorld(), HitLocation + FVector(0.0f, 0.0f, 30.0f), 14.0f, CircleColor, false, -1.0f);

				if (bIsPaintingBrush)
				{
					GM->PaintZoneAtLocation(HitLocation);
				}
			}
			return;
		}
	}

	// ── Тащим маркер ─────────────────────────────────────────────
	if (DraggedEndpoint)
	{
		FVector HitLocation;
		if (TraceTerrainUnderCursor(HitLocation))
		{
			DraggedEndpoint->SetEndpointFromTerrainHit(HitLocation, /*bBroadcastEvent=*/true);
		}
		else if (TraceFrameCounter % 15 == 0)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RailPC] Тащим маркер, но трейс рельефа не нашёл поверхность."));
		}
		return;
	}

	// ── Подсветка ────────────────────────────────────────────────
	ARailEndpoint* UnderCursor = PickEndpointUnderCursor();

	if (HoveredEndpoint != UnderCursor)
	{
		if (HoveredEndpoint)
			HoveredEndpoint->SetHighlighted(false);
		HoveredEndpoint = UnderCursor;

		if (HoveredEndpoint)
		{
			HoveredEndpoint->SetHighlighted(true);
			UE_LOG(LogTemp, Log, TEXT("[RailPC] Наведение: %s."),
				HoveredEndpoint->bIsStart ? TEXT("START") : TEXT("END"));
		}
	}
}

// ─────────────────────────────────────────────────────────────────
//  Логика ЛКМ и ПКМ
// ─────────────────────────────────────────────────────────────────

void ARailOptimaPlayerController::HandlePrimaryPressed()
{
	ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this));

	// Если включен режим кисти — начинаем непрерывное рисование зоны
	if (GM && GM->IsBrushModeActive())
	{
		bIsPaintingBrush = true;
		FVector HitLocation;
		if (TraceTerrainUnderCursor(HitLocation))
		{
			GM->PaintZoneAtLocation(HitLocation);
		}
		return;
	}

	ARailEndpoint* Marker = PickEndpointUnderCursor();
	if (!Marker)
	{
		// Проверка двойного клика ЛКМ на полотне трассы для создания Waypoint
		const double Now = FPlatformTime::Seconds();
		FVector2D MousePos;
		GetMousePosition(MousePos.X, MousePos.Y);

		if ((Now - LastLmbClickTime) < 0.38 && FVector2D::Distance(MousePos, LastLmbClickPos) < 25.0f)
		{
			FVector HitLocation;
			if (TraceTerrainUnderCursor(HitLocation) && GM)
			{
				GM->TryAddWaypointNearLocation(HitLocation);
			}
			LastLmbClickTime = 0.0;
			return;
		}

		LastLmbClickTime = Now;
		LastLmbClickPos = MousePos;
		return;
	}

	DraggedEndpoint = Marker;
	DraggedEndpoint->SetHighlighted(true);

	UE_LOG(LogTemp, Log,
		TEXT("[RailPC] Захват: %s, стартовая позиция (%.0f, %.0f, %.0f)."),
		DraggedEndpoint->bIsStart ? TEXT("START") : TEXT("END"),
		DraggedEndpoint->GetActorLocation().X,
		DraggedEndpoint->GetActorLocation().Y,
		DraggedEndpoint->GetActorLocation().Z);
}

void ARailOptimaPlayerController::HandlePrimaryReleased()
{
	if (bIsPaintingBrush)
	{
		bIsPaintingBrush = false;
		if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GM->RecomputePath();
		}
		return;
	}

	if (!DraggedEndpoint)
	{
		return;
	}

	UE_LOG(LogTemp, Log,
		TEXT("[RailPC] Отпускание: %s, финальная позиция (%.0f, %.0f, %.0f)."),
		DraggedEndpoint->bIsStart ? TEXT("START") : TEXT("END"),
		DraggedEndpoint->GetActorLocation().X,
		DraggedEndpoint->GetActorLocation().Y,
		DraggedEndpoint->GetActorLocation().Z);

	DraggedEndpoint->SetHighlighted(false);
	DraggedEndpoint = nullptr;
	HoveredEndpoint = nullptr;
}

void ARailOptimaPlayerController::HandleSecondaryPressed()
{
	ARailEndpoint* Marker = PickEndpointUnderCursor();
	if (Marker && Marker->bIsWaypoint)
	{
		if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GM->RemoveWaypoint(Marker);
		}
	}
}

void ARailOptimaPlayerController::HandleBrushModeToggle()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->ToggleBrushMode();
	}
}

void ARailOptimaPlayerController::HandleBrushRadiusDecrease()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (GM->IsBrushModeActive())
		{
			GM->SetBrushRadius_m(GM->GetBrushRadius_m() - 25.0f);
		}
	}
}

void ARailOptimaPlayerController::HandleBrushRadiusIncrease()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (GM->IsBrushModeActive())
		{
			GM->SetBrushRadius_m(GM->GetBrushRadius_m() + 25.0f);
		}
	}
}

// ─────────────────────────────────────────────────────────────────
//  Трассы
// ─────────────────────────────────────────────────────────────────

ARailEndpoint* ARailOptimaPlayerController::PickEndpointUnderCursor() const
{
	// 1. Приоритетный экранный хит-тест (Screen-space pick):
	// Проецирует координаты всех маркеров в экранные пиксели.
	// Работает со 100% надёжностью при любом удалении камеры и масштабе.
	FVector2D MousePos;
	if (GetMousePosition(MousePos.X, MousePos.Y))
	{
		TArray<AActor*> FoundEndpoints;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARailEndpoint::StaticClass(), FoundEndpoints);

		ARailEndpoint* BestMarker = nullptr;
		float BestDistSq = FMath::Square(50.0f); // 50 px радиус захвата на экране

		for (AActor* Actor : FoundEndpoints)
		{
			ARailEndpoint* Endpoint = Cast<ARailEndpoint>(Actor);
			if (!Endpoint || Endpoint->IsHidden())
			{
				continue;
			}

			FVector2D ScreenPos;
			if (ProjectWorldLocationToScreen(Endpoint->GetActorLocation(), ScreenPos))
			{
				const float DistSq = FVector2D::DistSquared(MousePos, ScreenPos);
				if (DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					BestMarker = Endpoint;
				}
			}
		}

		if (BestMarker)
		{
			return BestMarker;
		}
	}

	// 2. Fallback: 3D луч и sweep сферы
	FVector WorldOrigin, WorldDir;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDir))
		return nullptr;

	const FVector End = WorldOrigin + WorldDir * 100'000'000.0f;

	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RailPickEndpoint), /*bTraceComplex=*/false);
	QueryParams.AddIgnoredActor(GetPawn());

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(
		Hits, WorldOrigin, End, FQuat::Identity,
		ObjParams, FCollisionShape::MakeSphere(EndpointPickRadius_cm), QueryParams);

	for (const FHitResult& Hit : Hits)
	{
		if (ARailEndpoint* Endpoint = Cast<ARailEndpoint>(Hit.GetActor()))
		{
			if (!Endpoint->IsHidden())
			{
				return Endpoint;
			}
		}
	}
	return nullptr;
}

bool ARailOptimaPlayerController::TraceTerrainUnderCursor(FVector& OutHitLocation) const
{
	FVector WorldOrigin, WorldDir;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDir))
		return false;

	const FVector End = WorldOrigin + WorldDir * 100'000'000.0f;

	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RailTerrainCursorTrace), /*bTraceComplex=*/true);
	QueryParams.AddIgnoredActor(GetPawn());

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByObjectType(
		Hit, WorldOrigin, End, ObjParams, QueryParams);

	// Диагностический лог переведен в Verbose, чтобы не вызывать спама в консоль и на диск при движении
	if (const_cast<ARailOptimaPlayerController*>(this)->TraceFrameCounter % 60 == 0)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[RailPC] Трейс рельефа: %s, actor='%s'"),
			bHit ? TEXT("HIT") : TEXT("MISS"),
			*GetNameSafe(Hit.GetActor()));
	}

	if (bHit)
	{
		OutHitLocation = Hit.ImpactPoint;
		return true;
	}

	// Fallback при промахе по коллизии меша: математическое пересечение луча камеры с картой высот
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (AProceduralTerrain* Terrain = GM->GetTerrain())
		{
			if (FMath::Abs(WorldDir.Z) > 1e-4f)
			{
				const float T = -WorldOrigin.Z / WorldDir.Z;
				if (T > 0.0f)
				{
					const FVector IntersectPoint = WorldOrigin + WorldDir * T;
					float SampledZ = 0.0f;
					if (Terrain->SampleTerrainZ_cm(FVector2D(IntersectPoint.X, IntersectPoint.Y), SampledZ))
					{
						OutHitLocation = FVector(IntersectPoint.X, IntersectPoint.Y, SampledZ);
						return true;
					}
				}
			}
		}
	}

	return false;
}