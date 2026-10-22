// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#include "Interactive/RailSpectatorPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"


// ─────────────────────────────────────────────────────────────────
//  Жизненный цикл
// ─────────────────────────────────────────────────────────────────

ARailSpectatorPawn::ARailSpectatorPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(SceneRoot);
	CameraBoom->TargetArmLength = 0.0f;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->bEnableCameraRotationLag = false;
	CameraBoom->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	Camera->SetFieldOfView(90.0f);

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

void ARailSpectatorPawn::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("[RailSpectator] ─────────────────────────────────────────────"));
	UE_LOG(LogTemp, Log,
		TEXT("[RailSpectator] BeginPlay: pawn '%s', стартовая позиция (%.0f, %.0f, %.0f)."),
		*GetName(), GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z);

	UE_LOG(LogTemp, Log,
		TEXT("[RailSpectator] Actions: Move=%s, MoveUp=%s, Look=%s, CameraLook=%s, ")
			TEXT("Zoom=%s, Toggle=%s, Boost=%s, Primary=%s."),
		MoveAction ? *MoveAction->GetName() : TEXT("null"),
		MoveUpAction ? *MoveUpAction->GetName() : TEXT("null"),
		LookAction ? *LookAction->GetName() : TEXT("null"),
		CameraLookAction ? *CameraLookAction->GetName() : TEXT("null"),
		ZoomAction ? *ZoomAction->GetName() : TEXT("null"),
		ToggleCameraAction ? *ToggleCameraAction->GetName() : TEXT("null"),
		BoostAction ? *BoostAction->GetName() : TEXT("null"),
		PrimaryAction ? *PrimaryAction->GetName() : TEXT("null"));

	SetCameraMode(InitialCameraMode);

	UE_LOG(LogTemp, Log, TEXT("[RailSpectator] ─────────────────────────────────────────────"));
}

void ARailSpectatorPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	HandleMovement(DeltaSeconds);
}

void ARailSpectatorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[RailSpectator] SetupPlayerInputComponent: не UEnhancedInputComponent. ")
				TEXT("Project Settings → Input → Default Input Component Class = EnhancedInputComponent."));
		return;
	}

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARailSpectatorPawn::OnMove);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &ARailSpectatorPawn::OnMove);
		UE_LOG(LogTemp, Log, TEXT("[RailSpectator] BindAction: Move → OnMove."));
	}

	if (MoveUpAction)
	{
		EnhancedInput->BindAction(MoveUpAction, ETriggerEvent::Triggered, this, &ARailSpectatorPawn::OnMoveUp);
		EnhancedInput->BindAction(MoveUpAction, ETriggerEvent::Completed, this, &ARailSpectatorPawn::OnMoveUp);
		UE_LOG(LogTemp, Log, TEXT("[RailSpectator] BindAction: MoveUp → OnMoveUp."));
	}

	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ARailSpectatorPawn::OnLook);
		UE_LOG(LogTemp, Log, TEXT("[RailSpectator] BindAction: Look → OnLook."));
	}

	if (CameraLookAction)
	{
		EnhancedInput->BindAction(CameraLookAction, ETriggerEvent::Started, this, &ARailSpectatorPawn::OnCameraLookStarted);
		EnhancedInput->BindAction(CameraLookAction, ETriggerEvent::Completed, this, &ARailSpectatorPawn::OnCameraLookCompleted);
		UE_LOG(LogTemp, Log, TEXT("[RailSpectator] BindAction: CameraLook → OnCameraLookStarted/Completed."));
	}

	if (ZoomAction)
	{
		EnhancedInput->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &ARailSpectatorPawn::OnZoom);
		UE_LOG(LogTemp, Log, TEXT("[RailSpectator] BindAction: Zoom → OnZoom."));
	}

	if (ToggleCameraAction)
	{
		EnhancedInput->BindAction(ToggleCameraAction, ETriggerEvent::Started, this, &ARailSpectatorPawn::OnToggleCamera);
		UE_LOG(LogTemp, Log, TEXT("[RailSpectator] BindAction: ToggleCamera → OnToggleCamera."));
	}

	if (BoostAction)
	{
		EnhancedInput->BindAction(BoostAction, ETriggerEvent::Started, this, &ARailSpectatorPawn::OnBoostStarted);
		EnhancedInput->BindAction(BoostAction, ETriggerEvent::Completed, this, &ARailSpectatorPawn::OnBoostCompleted);
		UE_LOG(LogTemp, Log, TEXT("[RailSpectator] BindAction: Boost → OnBoostStarted/Completed."));
	}

	if (PrimaryAction)
	{
		EnhancedInput->BindAction(PrimaryAction, ETriggerEvent::Started, this, &ARailSpectatorPawn::OnPrimaryStarted);
		EnhancedInput->BindAction(PrimaryAction, ETriggerEvent::Completed, this, &ARailSpectatorPawn::OnPrimaryCompleted);
		UE_LOG(LogTemp, Log, TEXT("[RailSpectator] BindAction: Primary → OnPrimaryStarted/Completed."));
	}

	UE_LOG(LogTemp, Log, TEXT("[RailSpectator] Enhanced Input привязан."));
}

// ─────────────────────────────────────────────────────────────────
//  Публичный API
// ─────────────────────────────────────────────────────────────────

void ARailSpectatorPawn::ToggleCameraMode()
{
	const ERailCameraMode NewMode =
		(CameraMode == ERailCameraMode::FreeFly) ? ERailCameraMode::TopDown : ERailCameraMode::FreeFly;

	SetCameraMode(NewMode);
}

void ARailSpectatorPawn::SetCameraMode(ERailCameraMode NewMode)
{
	if (CameraMode == NewMode && CameraBoom)
		return;

	const ERailCameraMode OldMode = CameraMode;

	// 1. Запомнить позицию камеры ДО смены режима.
	const FVector CameraOldLocation = Camera ? Camera->GetComponentLocation() : GetActorLocation();

	UE_LOG(LogTemp, Log,
		TEXT("[RailSpectator] Tab: %s → %s. Камера сейчас в (%.0f, %.0f, %.0f)."),
		OldMode == ERailCameraMode::FreeFly ? TEXT("FreeFly") : TEXT("TopDown"),
		NewMode == ERailCameraMode::FreeFly ? TEXT("FreeFly") : TEXT("TopDown"),
		CameraOldLocation.X, CameraOldLocation.Y, CameraOldLocation.Z);

	// 2. Сменить режим и применить настройки boom'а.
	CameraMode = NewMode;
	ApplyCameraMode();

	// 3. Позиционирование павна.
	FVector NewPawnLocation;

	if (NewMode == ERailCameraMode::FreeFly)
	{
		// В FreeFly ставим павн туда, где сейчас камера.
		// Никакого телепорта — камера остаётся на месте.
		NewPawnLocation = CameraOldLocation;

		// Установить ControlRotation так, чтобы камера смотрела туда же,
		// куда смотрела в TopDown (вниз). Иначе она повернётся горизонтально.
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->SetControlRotation(FRotator(TopDownPitch, 0.0f, 0.0f));
		}

		SetActorRotation(FRotator::ZeroRotator);
	}
	else // TopDown
	{
		// Павн на "земле" под старой камерой (XY сохраняем, Z = 0).
		NewPawnLocation = FVector(CameraOldLocation.X, CameraOldLocation.Y, 0.0f);
		SetActorRotation(FRotator::ZeroRotator);
	}

	SetActorLocation(NewPawnLocation);

	UE_LOG(LogTemp, Log,
		TEXT("[RailSpectator] Pawn переставлен: (%.0f, %.0f, %.0f)."),
		NewPawnLocation.X, NewPawnLocation.Y, NewPawnLocation.Z);

	// 4. Сброс накопленных осей и отпускание RMB.
	if (bCameraLookActive)
	{
		SetCameraLookActive(false);
	}

	MoveInput = FVector2D::ZeroVector;
	MoveUpInput = 0.0f;
	LookInput = FVector2D::ZeroVector;
	ZoomInput = 0.0f;

	LogState(TEXT("SetCameraMode"));
}

bool ARailSpectatorPawn::CenterOnWorld(float InMapSizeX_cm, float InMapSizeY_cm, float InMapSizeZ_cm)
{
	if (!CameraBoom)
		return false;

	const FVector Center(InMapSizeX_cm * 0.5f, InMapSizeY_cm * 0.5f, 0.0f);
	SetActorLocation(Center);

	const float HalfMax = FMath::Max(InMapSizeX_cm, InMapSizeY_cm) * 0.5f;
	TopDownHeight_cm = FMath::Clamp(HalfMax * 1.2f, TopDownMinHeight_cm, TopDownMaxHeight_cm);

	ApplyCameraMode();

	UE_LOG(LogTemp, Log,
		TEXT("[RailSpectator] CenterOnWorld: pawn в (%.0f, %.0f, %.0f), TopDownHeight=%.0f см."),
		Center.X, Center.Y, Center.Z, TopDownHeight_cm);

	return true;
}

// ─────────────────────────────────────────────────────────────────
//  Enhanced Input handlers
// ─────────────────────────────────────────────────────────────────

void ARailSpectatorPawn::OnMove(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
}

void ARailSpectatorPawn::OnMoveUp(const FInputActionValue& Value)
{
	MoveUpInput = Value.Get<float>();
}

void ARailSpectatorPawn::OnLook(const FInputActionValue& Value)
{
	if (CameraMode != ERailCameraMode::FreeFly)
		return;
	if (!bCameraLookActive)
		return;

	LookInput = Value.Get<FVector2D>();

	const float Yaw = bInvertLookX ? -LookInput.X : LookInput.X;
	const float Pitch = bInvertLookY ? -LookInput.Y : LookInput.Y;

	AddControllerYawInput(Yaw);
	AddControllerPitchInput(Pitch);
}

void ARailSpectatorPawn::OnZoom(const FInputActionValue& Value)
{
	if (CameraMode != ERailCameraMode::TopDown)
		return;

	const float Raw = Value.Get<float>();
	if (FMath::IsNearlyZero(Raw))
		return;

	const float OldHeight = TopDownHeight_cm;
	TopDownHeight_cm *= (Raw > 0.0f) ? (1.0f / ZoomStep) : ZoomStep;
	TopDownHeight_cm = FMath::Clamp(TopDownHeight_cm, TopDownMinHeight_cm, TopDownMaxHeight_cm);

	if (!FMath::IsNearlyEqual(OldHeight, TopDownHeight_cm, 1.0f))
	{
		CameraBoom->TargetArmLength = TopDownHeight_cm;
		UE_LOG(LogTemp, Log, TEXT("[RailSpectator] Zoom: height %.0f → %.0f см."),
			OldHeight, TopDownHeight_cm);
	}
}

void ARailSpectatorPawn::OnToggleCamera(const FInputActionValue& /*Value*/)
{
	ToggleCameraMode();
}

void ARailSpectatorPawn::OnBoostStarted(const FInputActionValue& /*Value*/)
{
	bBoostHeld = true;
}

void ARailSpectatorPawn::OnBoostCompleted(const FInputActionValue& /*Value*/)
{
	bBoostHeld = false;
}

void ARailSpectatorPawn::OnCameraLookStarted(const FInputActionValue& /*Value*/)
{
	if (CameraMode != ERailCameraMode::FreeFly)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[RailSpectator] RMB нажат, но режим %s — вращение отключено."),
			(CameraMode == ERailCameraMode::FreeFly) ? TEXT("FreeFly") : TEXT("TopDown"));
		return;
	}
	SetCameraLookActive(true);
}

void ARailSpectatorPawn::OnCameraLookCompleted(const FInputActionValue& /*Value*/)
{
	SetCameraLookActive(false);
}

void ARailSpectatorPawn::OnPrimaryStarted(const FInputActionValue& /*Value*/)
{
	UE_LOG(LogTemp, Verbose, TEXT("[RailSpectator] Primary Started → OnPrimaryPressed.Broadcast."));
	OnPrimaryPressed.Broadcast();
}

void ARailSpectatorPawn::OnPrimaryCompleted(const FInputActionValue& /*Value*/)
{
	UE_LOG(LogTemp, Verbose, TEXT("[RailSpectator] Primary Completed → OnPrimaryReleased.Broadcast."));
	OnPrimaryReleased.Broadcast();
}

// ─────────────────────────────────────────────────────────────────
//  Внутреннее
// ─────────────────────────────────────────────────────────────────

void ARailSpectatorPawn::SetCameraLookActive(bool bActive)
{
	if (bCameraLookActive == bActive)
		return;
	bCameraLookActive = bActive;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
		return;

	if (bActive)
	{
		PC->SetShowMouseCursor(false);
		FInputModeGameOnly Mode;
		PC->SetInputMode(Mode);
		UE_LOG(LogTemp, Log, TEXT("[RailSpectator] RMB активирован: курсор скрыт."));
	}
	else
	{
		PC->SetShowMouseCursor(true);
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
		UE_LOG(LogTemp, Log, TEXT("[RailSpectator] RMB деактивирован: курсор показан."));
	}
}

void ARailSpectatorPawn::ApplyCameraMode()
{
	if (!CameraBoom)
		return;

	if (CameraMode == ERailCameraMode::FreeFly)
	{
		CameraBoom->TargetArmLength = 0.0f;
		CameraBoom->SetRelativeRotation(FRotator::ZeroRotator);
		CameraBoom->bUsePawnControlRotation = true;

		bUseControllerRotationPitch = true;
		bUseControllerRotationYaw = true;
		bUseControllerRotationRoll = false;

		UE_LOG(LogTemp, Log, TEXT("[RailSpectator] Режим FreeFly: TargetArmLength=0."));
	}
	else
	{
		CameraBoom->TargetArmLength = TopDownHeight_cm;
		CameraBoom->SetRelativeRotation(FRotator(TopDownPitch, 0.0f, 0.0f));
		CameraBoom->bUsePawnControlRotation = false;

		bUseControllerRotationPitch = false;
		bUseControllerRotationYaw = false;
		bUseControllerRotationRoll = false;

		UE_LOG(LogTemp, Log, TEXT("[RailSpectator] Режим TopDown: TargetArmLength=%.0f см, Pitch=%.1f°."),
			TopDownHeight_cm, TopDownPitch);
	}
}

void ARailSpectatorPawn::HandleMovement(float DeltaSeconds)
{
	const bool bHasMove2D = !MoveInput.IsNearlyZero();
	const bool bHasMoveUp = !FMath::IsNearlyZero(MoveUpInput);
	if (!bHasMove2D && !bHasMoveUp)
		return;

	// Boost работает в ОБОИХ режимах.
	const float Boost = bBoostHeld ? BoostMultiplier : 1.0f;

	if (CameraMode == ERailCameraMode::FreeFly)
	{
		const FRotator ControlRot = GetControlRotation();
		const FVector  Forward = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::X);
		const FVector  Right = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::Y);

		const float Speed = FreeFlySpeed_cmps * Boost;

		FVector Delta = (Forward * MoveInput.X + Right * MoveInput.Y) * Speed * DeltaSeconds;
		Delta.Z += MoveUpInput * VerticalSpeed_cmps * Boost * DeltaSeconds;

		AddActorWorldOffset(Delta, false);
	}
	else
	{
		// TopDown: панорамирование по мировой XY (W → +X, D → +Y),
		// Space/Ctrl по мировой Z. Boost ускоряет и то, и то.
		const FVector Delta(
			MoveInput.X * TopDownPanSpeed_cmps * Boost * DeltaSeconds,
			MoveInput.Y * TopDownPanSpeed_cmps * Boost * DeltaSeconds,
			MoveUpInput * VerticalSpeed_cmps * Boost * DeltaSeconds);

		AddActorWorldOffset(Delta, false);
	}
}

void ARailSpectatorPawn::LogState(const FString& InContext) const
{
	const FVector  Loc = GetActorLocation();
	const FRotator ControlRot = GetControlRotation();

	UE_LOG(LogTemp, Log,
		TEXT("[RailSpectator] %s: mode=%s, pos=(%.0f, %.0f, %.0f), boomLength=%.0f см, ")
			TEXT("boomRot=(P=%.1f, Y=%.1f), ctrlRot=(P=%.1f, Y=%.1f)."),
		*InContext,
		(CameraMode == ERailCameraMode::FreeFly) ? TEXT("FreeFly") : TEXT("TopDown"),
		Loc.X, Loc.Y, Loc.Z,
		CameraBoom ? CameraBoom->TargetArmLength : -1.0f,
		CameraBoom ? CameraBoom->GetRelativeRotation().Pitch : 0.0f,
		CameraBoom ? CameraBoom->GetRelativeRotation().Yaw : 0.0f,
		ControlRot.Pitch, ControlRot.Yaw);
}