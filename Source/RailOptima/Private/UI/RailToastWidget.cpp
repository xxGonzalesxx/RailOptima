// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Реализация всплывающего уведомления

#include "UI/RailToastWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"

TSharedRef<SWidget> URailToastWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ToastCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		ToastCardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ToastCardBorder"));
		ToastCardBorder->SetBrushColor(FLinearColor(0.03f, 0.05f, 0.09f, 0.95f));
		ToastCardBorder->SetPadding(FMargin(18.0f, 14.0f));

		UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(ToastCardBorder);
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		CanvasSlot->SetPosition(FVector2D(0.0f, 32.0f));
		CanvasSlot->SetAutoSize(true);

		UHorizontalBox* HBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		ToastCardBorder->SetContent(HBox);

		IconText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		IconText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
		IconText->SetText(FText::FromString(TEXT("ℹ")));
		IconText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
		if (UHorizontalBoxSlot* IconSlot = HBox->AddChildToHorizontalBox(IconText))
		{
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}

		USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		Spacer->SetSize(FVector2D(12.0f, 1.0f));
		HBox->AddChildToHorizontalBox(Spacer);

		MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		MessageText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 12));
		MessageText->SetText(FText::FromString(TEXT("")));
		MessageText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		if (UHorizontalBoxSlot* MsgSlot = HBox->AddChildToHorizontalBox(MessageText))
		{
			MsgSlot->SetVerticalAlignment(VAlign_Center);
		}

		ToastCardBorder->SetVisibility(ESlateVisibility::Collapsed);
	}

	return Super::RebuildWidget();
}

void URailToastWidget::ShowToast(const FString& InMessage, ERailToastType InType, float InDurationSeconds)
{
	if (!ToastCardBorder || !MessageText)
	{
		return;
	}

	FLinearColor AccentColor;
	FString IconStr;

	switch (InType)
	{
		case ERailToastType::Success:
			AccentColor = FLinearColor(0.2f, 0.95f, 0.4f, 1.0f);
			IconStr = TEXT("✓ ");
			break;
		case ERailToastType::Warning:
			AccentColor = FLinearColor(1.0f, 0.75f, 0.2f, 1.0f);
			IconStr = TEXT("⚠ ");
			break;
		case ERailToastType::Error:
			AccentColor = FLinearColor(1.0f, 0.25f, 0.25f, 1.0f);
			IconStr = TEXT("✕ ");
			break;
		case ERailToastType::Info:
		default:
			AccentColor = FLinearColor(0.35f, 0.78f, 1.0f, 1.0f);
			IconStr = TEXT("ℹ ");
			break;
	}

	IconText->SetText(FText::FromString(IconStr));
	IconText->SetColorAndOpacity(FSlateColor(AccentColor));

	MessageText->SetText(FText::FromString(InMessage));
	MessageText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.96f, 0.98f, 1.0f)));

	ToastCardBorder->SetVisibility(ESlateVisibility::Visible);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
		if (InDurationSeconds > 0.0f)
		{
			World->GetTimerManager().SetTimer(HideTimerHandle, this, &URailToastWidget::HideToast, InDurationSeconds, false);
		}
	}
}

void URailToastWidget::HideToast()
{
	if (ToastCardBorder)
	{
		ToastCardBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}
