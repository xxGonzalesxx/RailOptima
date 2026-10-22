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
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ARailOptimaPlayerController::HandleEscapePressed);
	}
}

void ARailOptimaPlayerController::HandleEscapePressed()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->TogglePauseMenu();
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
			const float				  SideX_cm = Map.Width * static_cast<float>(Map.PixelSize_m) * 100.0f;
			const float				  SideY_cm = Map.Height * static_cast<float>(Map.PixelSize_m) * 100.0f;
			const float				  SideZ_cm = (Map.MaxElevation_m - Map.MinElevation_m) * 100.0f;

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
//  Логика ЛКМ
// ─────────────────────────────────────────────────────────────────

void ARailOptimaPlayerController::HandlePrimaryPressed()
{
	UE_LOG(LogTemp, Log, TEXT("[RailPC] Primary Pressed — ищу маркер под курсором."));

	ARailEndpoint* Marker = PickEndpointUnderCursor();
	if (!Marker)
	{
		UE_LOG(LogTemp, Log, TEXT("[RailPC] ЛКМ: маркер под курсором НЕ найден."));
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
	if (!DraggedEndpoint)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[RailPC] Primary Released, но ничего не тащим."));
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

// ─────────────────────────────────────────────────────────────────
//  Трассы
// ─────────────────────────────────────────────────────────────────

ARailEndpoint* ARailOptimaPlayerController::PickEndpointUnderCursor() const
{
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
			return Endpoint;
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

	if (const_cast<ARailOptimaPlayerController*>(this)->TraceFrameCounter % 15 == 0)
	{
		if (bHit)
		{
			UE_LOG(LogTemp, Log,
				TEXT("[RailPC] Трейс рельефа: HIT, actor='%s', impact=(%.0f, %.0f, %.0f)."),
				*GetNameSafe(Hit.GetActor()),
				Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RailPC] Трейс рельефа: MISS. Проверь complex collision у ProceduralTerrain."));
		}
	}

	if (!bHit)
		return false;

	OutHitLocation = Hit.ImpactPoint;
	return true;
}