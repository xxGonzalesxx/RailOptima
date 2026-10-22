// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Меню паузы (Esc) в рабочем режиме проекта

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Services/RailProjectTypes.h"
#include "RailPauseMenuWidget.generated.h"

class UBorder;
class UButton;
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
	FOnRailPauseActionSignature OnSaveProjectClicked;

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Events")
	FOnRailPauseActionSignature OnExportExcelClicked;

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Events")
	FOnRailPauseActionSignature OnReturnToMainMenuClicked;

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Events")
	FOnRailPauseActionSignature OnQuitClicked;

private:
	UFUNCTION()
	void HandleResume();

	UFUNCTION()
	void HandleSave();

	UFUNCTION()
	void HandleExportExcel();

	UFUNCTION()
	void HandleReturnToMainMenu();

	UFUNCTION()
	void HandleQuit();

	UButton* CreateMenuButton(const FString& Text, const FLinearColor& AccentColor);

	UPROPERTY()
	TObjectPtr<UTextBlock> ProjectNameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> CategoryText;

	UPROPERTY()
	TObjectPtr<UTextBlock> DetailsText;

	FRailProjectMetadata CurrentMeta;
};
