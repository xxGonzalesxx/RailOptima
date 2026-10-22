// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Всплывающее уведомление (Toast / Alert) в стиле RailOptima

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "RailToastWidget.generated.h"

class UBorder;
class UTextBlock;

UENUM(BlueprintType)
enum class ERailToastType : uint8
{
	Info    UMETA(DisplayName = "Информация"),
	Success UMETA(DisplayName = "Успех"),
	Warning UMETA(DisplayName = "Предупреждение"),
	Error   UMETA(DisplayName = "Ошибка")
};

UCLASS()
class RAILOPTIMA_API URailToastWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// Показать уведомление
	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void ShowToast(const FString& InMessage, ERailToastType InType = ERailToastType::Info, float InDurationSeconds = 4.5f);

	// Скрыть уведомление
	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void HideToast();

private:
	UPROPERTY()
	TObjectPtr<UBorder> ToastCardBorder;

	UPROPERTY()
	TObjectPtr<UTextBlock> IconText;

	UPROPERTY()
	TObjectPtr<UTextBlock> MessageText;

	FTimerHandle HideTimerHandle;
};
