// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "RailSpectatorPawn.generated.h"

class USceneComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;

UENUM(BlueprintType)
enum class ERailCameraMode : uint8
{
	FreeFly UMETA(DisplayName = "Свободный полёт"),
	TopDown UMETA(DisplayName = "Вид сверху")
};

DECLARE_MULTICAST_DELEGATE(FRailPrimaryActionSignature);

UCLASS(Blueprintable)
class RAILOPTIMA_API ARailSpectatorPawn : public APawn
{
	GENERATED_BODY()

public:
	ARailSpectatorPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Camera")
	void ToggleCameraMode();

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Camera")
	void SetCameraMode(ERailCameraMode NewMode);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Camera")
	ERailCameraMode GetCameraMode() const { return CameraMode; }

	// Текущая высота обзора в TopDown, см. Используется GameMode'ом
	// для динамического масштабирования пути и маркеров.
	UFUNCTION(BlueprintPure, Category = "RailOptima|Camera")
	float GetTopDownHeight() const { return TopDownHeight_cm; }

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Camera")
	bool CenterOnWorld(float InMapSizeX_cm, float InMapSizeY_cm, float InMapSizeZ_cm);

	FRailPrimaryActionSignature OnPrimaryPressed;
	FRailPrimaryActionSignature OnPrimaryReleased;

protected:
	// ─────────────────────────────────────────────────────────────────
	//  Enhanced Input
	// ─────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Input|Actions",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Input|Actions",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveUpAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Input|Actions",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Input|Actions",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> CameraLookAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Input|Actions",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ZoomAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Input|Actions",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ToggleCameraAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Input|Actions",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> BoostAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Input|Actions",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> PrimaryAction;

	// ─────────────────────────────────────────────────────────────────
	//  Настройки перемещения
	// ─────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Camera|FreeFly",
		meta = (ClampMin = "100.0"))
	float FreeFlySpeed_cmps = 100000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Camera",
		meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float BoostMultiplier = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Camera|TopDown",
		meta = (ClampMin = "100.0"))
	float TopDownPanSpeed_cmps = 200000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Camera",
		meta = (ClampMin = "100.0"))
	float VerticalSpeed_cmps = 100000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Camera|TopDown",
		meta = (ClampMin = "1000.0"))
	float TopDownHeight_cm = 3000000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Camera|TopDown")
	float TopDownMinHeight_cm = 50000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Camera|TopDown")
	float TopDownMaxHeight_cm = 50000000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Camera|TopDown")
	float ZoomStep = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Camera|TopDown")
	float TopDownPitch = -89.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Camera")
	ERailCameraMode InitialCameraMode = ERailCameraMode::TopDown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Camera|Look")
	bool bInvertLookX = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Camera|Look")
	bool bInvertLookY = false;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RailOptima|Camera",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RailOptima|Camera",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RailOptima|Camera",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera;

	ERailCameraMode CameraMode = ERailCameraMode::TopDown;

	FVector2D MoveInput = FVector2D::ZeroVector;
	float	  MoveUpInput = 0.0f;
	FVector2D LookInput = FVector2D::ZeroVector;
	float	  ZoomInput = 0.0f;
	bool	  bBoostHeld = false;
	bool	  bCameraLookActive = false;

	void OnMove(const FInputActionValue& Value);
	void OnMoveUp(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnZoom(const FInputActionValue& Value);
	void OnToggleCamera(const FInputActionValue& Value);
	void OnBoostStarted(const FInputActionValue& Value);
	void OnBoostCompleted(const FInputActionValue& Value);
	void OnCameraLookStarted(const FInputActionValue& Value);
	void OnCameraLookCompleted(const FInputActionValue& Value);
	void OnPrimaryStarted(const FInputActionValue& Value);
	void OnPrimaryCompleted(const FInputActionValue& Value);

	void ApplyCameraMode();
	void HandleMovement(float DeltaSeconds);
	void LogState(const FString& InContext) const;
	void SetCameraLookActive(bool bActive);
};