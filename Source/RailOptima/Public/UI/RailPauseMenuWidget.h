// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Меню паузы (Esc) в рабочем режиме проекта

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Services/RailProjectTypes.h"
#include "RailPauseMenuWidget.generated.h"

class UBorder;
class UButton;
class UComboBoxString;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRailPauseActionSignature);

UCLASS()
class RAILOPTIMA_API URailPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// Обновить отображаемую информацию о текущем проекте
	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void SetProjectInfo(const FRailProjectMetadata& InMeta);

	// События кнопок меню
	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Events")
	FOnRailPauseActionSignature OnResumeClicked;

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Events")
	FOnRailPauseActionSignature OnGraphicsSettingsClicked;

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Events")
	FOnRailPauseActionSignature OnSaveProjectClicked;

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Events")
	FOnRailPauseActionSignature OnExportExcelClicked;

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Events")
	FOnRailPauseActionSignature OnReturnToMainMenuClicked;

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Events")
	FOnRailPauseActionSignature OnQuitClicked;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnRailVisualSettingsAppliedSignature,
		float, FlySpeed_cmps, float, BoostMult, float, PathWidth_cm, float, PathHeight_cm, float, EndpointScale);

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Events")
	FOnRailVisualSettingsAppliedSignature OnVisualSettingsApplied;

private:
	UFUNCTION()
	void HandleResume();

	UFUNCTION()
	void HandleGraphicsSettings();

	UFUNCTION()
	void HandleOpenVisualSettings();

	UFUNCTION()
	void HandleApplyVisualSettings();

	UFUNCTION()
	void HandleCloseVisualSettings();

	UFUNCTION()
	void HandleSave();

	UFUNCTION()
	void HandleExportExcel();

	UFUNCTION()
	void HandleReturnToMainMenu();

	UFUNCTION()
	void HandleQuit();

	UFUNCTION()
	void HandleCategorySelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	UWidget* HandleGenerateCategoryWidget(FString Item);

	UButton* CreateMenuButton(const FString& Text, const FLinearColor& AccentColor);

	void BuildVisualSettingsModal(class UCanvasPanel* RootCanvas);

	UPROPERTY()
	TObjectPtr<UBorder> VisualSettingsModalBorder;

	UPROPERTY()
	TObjectPtr<UComboBoxString> CameraSpeedCombo;

	UPROPERTY()
	TObjectPtr<UComboBoxString> ShiftBoostCombo;

	UPROPERTY()
	TObjectPtr<UComboBoxString> PathWidthCombo;

	UPROPERTY()
	TObjectPtr<UComboBoxString> PathHeightCombo;

	UPROPERTY()
	TObjectPtr<UComboBoxString> EndpointScaleCombo;

	UPROPERTY()
	TObjectPtr<UComboBoxString> CategoryComboBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> ProjectNameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> CategoryText;

	UPROPERTY()
	TObjectPtr<UTextBlock> DetailsText;

	FRailProjectMetadata CurrentMeta;
};
