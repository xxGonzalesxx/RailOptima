 // Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Плавающий виджет управления кистью запретных зон

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "RailBrushWidget.generated.h"

enum class ERailZoneType : uint8;
class UBorder;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRailBrushClosedSignature);

UCLASS()
class RAILOPTIMA_API URailBrushWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	void UpdateRadiusDisplay(float NewRadius_m);
	void UpdateZoneTypeDisplay(ERailZoneType NewType);

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Events")
	FOnRailBrushClosedSignature OnBrushClosed;

private:
	UFUNCTION()
	void HandleForbiddenClicked();

	UFUNCTION()
	void HandleEraserClicked();

	UFUNCTION()
	void HandleRadiusMinusClicked();

	UFUNCTION()
	void HandleRadiusPlusClicked();

	UFUNCTION()
	void HandlePreset50Clicked();

	UFUNCTION()
	void HandlePreset150Clicked();

	UFUNCTION()
	void HandlePreset300Clicked();

	UFUNCTION()
	void HandlePreset500Clicked();

	UFUNCTION()
	void HandleClearAllClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UButton* CreateToolButton(const FString& Text, const FLinearColor& AccentColor, float Height = 34.0f);

	UPROPERTY()
	TObjectPtr<UTextBlock> RadiusValueText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ModeStatusText;

	UPROPERTY()
	TObjectPtr<UButton> ForbiddenBtn;

	UPROPERTY()
	TObjectPtr<UButton> EraserBtn;
};
