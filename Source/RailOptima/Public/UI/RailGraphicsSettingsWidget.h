// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Виджет настроек графики и масштабируемости RailOptima

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "RailGraphicsSettingsWidget.generated.h"

class UBorder;
class UButton;
class UCheckBox;
class UComboBoxString;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRailSettingsClosedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRailSettingsAppliedSignature);

UCLASS()
class RAILOPTIMA_API URailGraphicsSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// Открыть модальное окно настроек и синхронизировать с текущими параметрами системы
	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void OpenSettingsModal();

	// Закрыть окно настроек
	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void CloseSettingsModal();

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Events")
	FOnRailSettingsClosedSignature OnSettingsClosed;

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Events")
	FOnRailSettingsAppliedSignature OnSettingsApplied;

private:
	void SyncUiFromSettings();

	UFUNCTION()
	void HandlePresetLowClicked();

	UFUNCTION()
	void HandlePresetMediumClicked();

	UFUNCTION()
	void HandlePresetHighClicked();

	UFUNCTION()
	void HandlePresetUltraClicked();

	UFUNCTION()
	void HandleAutoBenchmarkClicked();

	UFUNCTION()
	void HandleApplyClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	UWidget* HandleGenerateComboWidget(FString Item);

	UButton* CreatePresetButton(const FString& Text, const FLinearColor& Color);

	UPROPERTY()
	TObjectPtr<UBorder> RootDimmerBorder;

	UPROPERTY()
	TObjectPtr<UComboBoxString> ResolutionComboBox;

	UPROPERTY()
	TObjectPtr<UComboBoxString> FpsLimitComboBox;

	UPROPERTY()
	TObjectPtr<UComboBoxString> ShadowComboBox;

	UPROPERTY()
	TObjectPtr<UComboBoxString> AntiAliasingComboBox;

	UPROPERTY()
	TObjectPtr<UComboBoxString> ViewDistanceComboBox;

	UPROPERTY()
	TObjectPtr<UCheckBox> VSyncCheckBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> CurrentPresetBadge;
};
