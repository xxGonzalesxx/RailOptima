// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Главное меню и дашборд проектов RailOptima

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Services/RailProjectTypes.h"
#include "UI/RailToastWidget.h"
#include "RailMainMenuWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UCheckBox;
class UComboBoxString;
class UEditableTextBox;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class URailMainMenuWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRailProjectSelectedSignature, const FRailProjectMetadata&, ProjectMeta);

UCLASS()
class RAILOPTIMA_API URailProjectActionHelper : public UObject
{
	GENERATED_BODY()

public:
	FRailProjectMetadata ProjectMeta;
	TWeakObjectPtr<URailMainMenuWidget> MenuWidget;

	UFUNCTION()
	void OnOpenClicked();

	UFUNCTION()
	void OnExploreClicked();

	UFUNCTION()
	void OnDeleteClicked();
};

UCLASS()
class RAILOPTIMA_API URailMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// Обновить список локальных проектов в библиотеке
	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void RefreshProjectLibrary();

	// Показать всплывающее уведомление
	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void ShowToast(const FString& Message, ERailToastType Type = ERailToastType::Info, float DurationSec = 4.5f);

	// Управление оверлеем загрузки
	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void ShowLoadingOverlay(const FString& InStatusText);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void HideLoadingOverlay();

	// Закрыть открытые модальные окна (при Esc)
	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	bool CloseOpenModals();

	// Делегат: проект выбран пользователем для открытия
	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Events")
	FOnRailProjectSelectedSignature OnProjectOpened;

private:
	// Построение подсекций UI
	void BuildHeader(UVerticalBox* MainVBox);
	void BuildContentArea(UVerticalBox* MainVBox);
	void BuildCreateProjectModal(UCanvasPanel* RootCanvas);
	void BuildAuthModal(UCanvasPanel* RootCanvas);
	void BuildLoadingOverlay(UCanvasPanel* RootCanvas);

	// Обработчики кнопок дашборда
	UFUNCTION()
	void HandleNewProjectClicked();

	UFUNCTION()
	void HandleOpenProjectsFolderClicked();

	UFUNCTION()
	void HandleAuthButtonClicked();

	UFUNCTION()
	void HandleRefreshLibraryClicked();

	// Обработчики модального окна создания проекта
	UFUNCTION()
	void HandleBrowseFileClicked();

	UFUNCTION()
	void HandleCreateProjectConfirmClicked();

	UFUNCTION()
	void HandleCreateProjectCancelClicked();

	UFUNCTION()
	void HandleSourceTypeToggledPng();

	UFUNCTION()
	void HandleSourceTypeToggledTif();

	// Обработчики окна авторизации
	UFUNCTION()
	void HandleAuthSubmitClicked();

	UFUNCTION()
	void HandleAuthCloseClicked();

	// Виджеты карточек библиотеки проектов
	void PopulateProjectCard(const FRailProjectMetadata& Meta, UScrollBox* TargetScrollBox);

	// Ссылки на элементы UI
	UPROPERTY()
	TObjectPtr<UScrollBox> ProjectListScrollBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> ProjectCountText;

	// Модальное окно создания проекта
	UPROPERTY()
	TObjectPtr<UBorder> CreateProjectModalBorder;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> ProjectNameInput;

	UPROPERTY()
	TObjectPtr<UComboBoxString> CategoryComboBox;

	UPROPERTY()
	TObjectPtr<UButton> PngTypeButton;

	UPROPERTY()
	TObjectPtr<UButton> TifTypeButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> PngTypeText;

	UPROPERTY()
	TObjectPtr<UTextBlock> TifTypeText;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> FilePathInput;

	UPROPERTY()
	TObjectPtr<UCheckBox> RoadsCheckBox;

	UPROPERTY()
	TObjectPtr<UCheckBox> RiversCheckBox;

	bool bSourceIsGeoTiff = false;

	// Окно авторизации
	UPROPERTY()
	TObjectPtr<UBorder> AuthModalBorder;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> AuthEmailInput;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> AuthPassInput;

	// Оверлей загрузки
	UPROPERTY()
	TObjectPtr<UBorder> LoadingOverlayBorder;

	UPROPERTY()
	TObjectPtr<UTextBlock> LoadingStatusText;

	// Тосты
	UPROPERTY()
	TObjectPtr<URailToastWidget> ToastWidget;

	UPROPERTY()
	TArray<TObjectPtr<URailProjectActionHelper>> ActionHelpers;
};
