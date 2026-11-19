// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Главное меню и дашборд проектов RailOptima с поддержкой Backend API и облачной синхронизации

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Services/RailProjectTypes.h"
#include "Services/RailApiService.h"
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRailGraphicsSettingsRequestedSignature);

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

	UFUNCTION()
	void OnSyncToCloudClicked();
};

UCLASS()
class RAILOPTIMA_API URailCloudProjectActionHelper : public UObject
{
	GENERATED_BODY()

public:
	FRailCloudProject CloudProject;
	TWeakObjectPtr<URailMainMenuWidget> MenuWidget;

	UFUNCTION()
	void OnCreateLocalCloneClicked();

	UFUNCTION()
	void OnDeleteFromCloudClicked();
};

UCLASS()
class RAILOPTIMA_API URailMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// Обновить список проектов в библиотеке (локальных или облачных)
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

	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void HandleNewProjectClicked();

	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void HandleAuthButtonClicked();

	// Делегаты событий меню
	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Events")
	FOnRailProjectSelectedSignature OnProjectOpened;

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Events")
	FOnRailGraphicsSettingsRequestedSignature OnGraphicsSettingsRequested;

private:
	// Построение подсекций UI
	void BuildHeader(UVerticalBox* MainVBox);
	void BuildContentArea(UVerticalBox* MainVBox);
	void BuildCreateProjectModal(UCanvasPanel* RootCanvas);
	void BuildCreateCloudProjectModal(UCanvasPanel* RootCanvas);
	void BuildAuthModal(UCanvasPanel* RootCanvas);
	void BuildLoadingOverlay(UCanvasPanel* RootCanvas);

	void UpdateHeaderProfileUI();
	void PopulateCloudProjectCard(const FRailCloudProject& CloudProj, UScrollBox* TargetScrollBox);
	void PopulateProjectCard(const FRailProjectMetadata& Meta, UScrollBox* TargetScrollBox);

	// Обработчики кнопок дашборда
	UFUNCTION()
	void HandleOpenProjectsFolderClicked();

	UFUNCTION()
	void HandleRefreshLibraryClicked();

	UFUNCTION()
	void HandleGraphicsSettingsClicked();

	// Вкладки библиотеки проектов
	UFUNCTION()
	void HandleTabLocalClicked();

	UFUNCTION()
	void HandleTabCloudClicked();

	UFUNCTION()
	void HandleCreateCloudProjectClicked();

	UFUNCTION()
	void HandleCreateCloudConfirmClicked();

	UFUNCTION()
	void HandleCreateCloudCancelClicked();

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

	UFUNCTION()
	UWidget* HandleGenerateCategoryWidget(FString Item);

	// Обработчики окна авторизации
	UFUNCTION()
	void HandleAuthModeLoginClicked();

	UFUNCTION()
	void HandleAuthModeRegisterClicked();

	UFUNCTION()
	void HandleAuthSubmitClicked();

	UFUNCTION()
	void HandleAuthLogoutClicked();

	UFUNCTION()
	void HandleAuthCloseClicked();

	// Обработчики ответов API
	UFUNCTION()
	void HandleApiAuthCompleted(bool bSuccess, const FString& Message, const FString& Email, const FString& Role);

	UFUNCTION()
	void HandleApiProjectsReceived(bool bSuccess, const TArray<FRailCloudProject>& Projects, const FString& ErrorMessage);

	UFUNCTION()
	void HandleApiProjectActionCompleted(bool bSuccess, const FRailCloudProject& Project, const FString& Message);

	UFUNCTION()
	void HandleApiProjectDeleted(bool bSuccess, const FString& Message);

	// Ссылки на элементы UI
	UPROPERTY()
	TObjectPtr<UScrollBox> ProjectListScrollBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> ProjectCountText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ProfileStatusText;

	UPROPERTY()
	TObjectPtr<UTextBlock> AuthButtonText;

	// Вкладки
	UPROPERTY()
	TObjectPtr<UButton> TabLocalBtn;

	UPROPERTY()
	TObjectPtr<UButton> TabCloudBtn;

	UPROPERTY()
	TObjectPtr<UTextBlock> TabLocalText;

	UPROPERTY()
	TObjectPtr<UTextBlock> TabCloudText;

	bool bShowingCloudProjects = false;

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

	// Модальное окно создания облачного проекта
	UPROPERTY()
	TObjectPtr<UBorder> CreateCloudModalBorder;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> CloudProjNameInput;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> CloudProjDescInput;

	// Окно авторизации
	UPROPERTY()
	TObjectPtr<UBorder> AuthModalBorder;

	UPROPERTY()
	TObjectPtr<UTextBlock> AuthTitleText;

	UPROPERTY()
	TObjectPtr<UButton> AuthTabLoginBtn;

	UPROPERTY()
	TObjectPtr<UButton> AuthTabRegisterBtn;

	UPROPERTY()
	TObjectPtr<UTextBlock> AuthTabLoginText;

	UPROPERTY()
	TObjectPtr<UTextBlock> AuthTabRegisterText;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> AuthEmailInput;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> AuthPassInput;

	UPROPERTY()
	TObjectPtr<UTextBlock> AuthSubmitText;

	UPROPERTY()
	TObjectPtr<UTextBlock> AuthStatusMessage;

	UPROPERTY()
	TObjectPtr<UButton> AuthLogoutBtn;

	bool bAuthIsRegisterMode = false;

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

	UPROPERTY()
	TArray<TObjectPtr<URailCloudProjectActionHelper>> CloudActionHelpers;
};
