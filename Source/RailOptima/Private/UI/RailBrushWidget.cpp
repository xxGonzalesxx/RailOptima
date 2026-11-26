 // Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Плавающий виджет управления кистью запретных зон

#include "UI/RailBrushWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "RailOptimaGameMode/RailOptimaGameMode.h"
#include "Styling/CoreStyle.h"

namespace
{
	FButtonStyle MakeBrushButtonStyle(const FLinearColor& NormalColor, const FLinearColor& HoveredColor, const FLinearColor& PressedColor)
	{
		FButtonStyle Style;
		Style.Normal.DrawAs = ESlateBrushDrawType::Box;
		Style.Normal.Margin = FMargin(4.0f / 32.0f);
		Style.Normal.TintColor = FSlateColor(NormalColor);

		Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
		Style.Hovered.Margin = FMargin(4.0f / 32.0f);
		Style.Hovered.TintColor = FSlateColor(HoveredColor);

		Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
		Style.Pressed.Margin = FMargin(4.0f / 32.0f);
		Style.Pressed.TintColor = FSlateColor(PressedColor);

		return Style;
	}
}

TSharedRef<SWidget> URailBrushWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BrushRootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		USizeBox* PanelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BrushPanelSizeBox"));
		PanelSizeBox->SetWidthOverride(380.0f);

		UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(PanelSizeBox);
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		CanvasSlot->SetPosition(FVector2D(28.0f, 28.0f));
		CanvasSlot->SetAutoSize(true);

		UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BrushCardBorder"));
		CardBorder->SetBrushColor(FLinearColor(0.02f, 0.035f, 0.06f, 0.94f));
		CardBorder->SetPadding(FMargin(18.0f, 14.0f));
		PanelSizeBox->SetContent(CardBorder);

		UVerticalBox* CardVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BrushCardVBox"));
		CardBorder->SetContent(CardVBox);

		// Заголовок
		UTextBlock* HeaderTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		HeaderTitle->SetText(FText::FromString(TEXT("🖌 РЕЖИМ КИСТИ: ЗОНЫ")));
		HeaderTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
		HeaderTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
		HeaderTitle->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* HSlot = CardVBox->AddChildToVerticalBox(HeaderTitle))
		{
			HSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}

		// Тип кисти: Запретная зона (Красный) / Ластик
		UTextBlock* ModeLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ModeLabel->SetText(FText::FromString(TEXT("Тип кисти:")));
		ModeLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
		ModeLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.76f, 0.82f, 1.0f)));
		CardVBox->AddChildToVerticalBox(ModeLabel);

		UHorizontalBox* ModeHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (UVerticalBoxSlot* MSlot = CardVBox->AddChildToVerticalBox(ModeHBox))
		{
			MSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 10.0f));
		}

		const FLinearColor RedAccent = FLinearColor(0.75f, 0.12f, 0.12f, 0.95f);
		const FLinearColor NavyAccent = FLinearColor(0.08f, 0.22f, 0.38f, 0.95f);

		ForbiddenBtn = CreateToolButton(TEXT("⛔ Запретная (Красная)"), RedAccent, 34.0f);
		if (ForbiddenBtn)
		{
			ForbiddenBtn->OnClicked.AddDynamic(this, &URailBrushWidget::HandleForbiddenClicked);
			if (UHorizontalBoxSlot* BtnSlot = ModeHBox->AddChildToHorizontalBox(ForbiddenBtn))
			{
				BtnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				BtnSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
			}
		}

		EraserBtn = CreateToolButton(TEXT("🧹 Ластик (Стереть)"), NavyAccent, 34.0f);
		if (EraserBtn)
		{
			EraserBtn->OnClicked.AddDynamic(this, &URailBrushWidget::HandleEraserClicked);
			if (UHorizontalBoxSlot* BtnSlot = ModeHBox->AddChildToHorizontalBox(EraserBtn))
			{
				BtnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				BtnSlot->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));
			}
		}

		// Радиус кисти
		UHorizontalBox* RadiusHeaderBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		CardVBox->AddChildToVerticalBox(RadiusHeaderBox);

		UTextBlock* RadiusTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		RadiusTitle->SetText(FText::FromString(TEXT("Радиус кисти: ")));
		RadiusTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
		RadiusTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.76f, 0.82f, 1.0f)));
		RadiusHeaderBox->AddChildToHorizontalBox(RadiusTitle);

		RadiusValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		RadiusValueText->SetText(FText::FromString(TEXT("150 м")));
		RadiusValueText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
		RadiusValueText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.82f, 0.15f, 1.0f)));
		RadiusHeaderBox->AddChildToHorizontalBox(RadiusValueText);

		// Кнопки быстрой настройки радиуса
		UHorizontalBox* RadiusBtnsBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (UVerticalBoxSlot* RSlot = CardVBox->AddChildToVerticalBox(RadiusBtnsBox))
		{
			RSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 10.0f));
		}

		auto SetupPresetSlot = [&](UButton* Btn) {
			if (Btn)
			{
				if (UHorizontalBoxSlot* Slot = RadiusBtnsBox->AddChildToHorizontalBox(Btn))
				{
					Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
					Slot->SetPadding(FMargin(2.0f, 0.0f, 2.0f, 0.0f));
				}
			}
		};

		UButton* MinusBtn = CreateToolButton(TEXT("–25м"), NavyAccent, 30.0f);
		if (MinusBtn) { MinusBtn->OnClicked.AddDynamic(this, &URailBrushWidget::HandleRadiusMinusClicked); SetupPresetSlot(MinusBtn); }

		UButton* PlusBtn = CreateToolButton(TEXT("+25м"), NavyAccent, 30.0f);
		if (PlusBtn) { PlusBtn->OnClicked.AddDynamic(this, &URailBrushWidget::HandleRadiusPlusClicked); SetupPresetSlot(PlusBtn); }

		UButton* P50Btn = CreateToolButton(TEXT("50м"), NavyAccent, 30.0f);
		if (P50Btn) { P50Btn->OnClicked.AddDynamic(this, &URailBrushWidget::HandlePreset50Clicked); SetupPresetSlot(P50Btn); }

		UButton* P150Btn = CreateToolButton(TEXT("150м"), NavyAccent, 30.0f);
		if (P150Btn) { P150Btn->OnClicked.AddDynamic(this, &URailBrushWidget::HandlePreset150Clicked); SetupPresetSlot(P150Btn); }

		UButton* P300Btn = CreateToolButton(TEXT("300м"), NavyAccent, 30.0f);
		if (P300Btn) { P300Btn->OnClicked.AddDynamic(this, &URailBrushWidget::HandlePreset300Clicked); SetupPresetSlot(P300Btn); }

		UButton* P500Btn = CreateToolButton(TEXT("500м"), NavyAccent, 30.0f);
		if (P500Btn) { P500Btn->OnClicked.AddDynamic(this, &URailBrushWidget::HandlePreset500Clicked); SetupPresetSlot(P500Btn); }

		// Действия: Очистить все зоны и Закрыть кисть
		UButton* ClearBtn = CreateToolButton(TEXT("Очистить все зоны"), FLinearColor(0.55f, 0.10f, 0.10f, 0.95f), 34.0f);
		if (ClearBtn)
		{
			ClearBtn->OnClicked.AddDynamic(this, &URailBrushWidget::HandleClearAllClicked);
			if (UVerticalBoxSlot* CSlot = CardVBox->AddChildToVerticalBox(ClearBtn))
			{
				CSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
			}
		}

		UButton* CloseBtn = CreateToolButton(TEXT("Закрыть кисть (клавиша B)"), NavyAccent, 34.0f);
		if (CloseBtn)
		{
			CloseBtn->OnClicked.AddDynamic(this, &URailBrushWidget::HandleCloseClicked);
			if (UVerticalBoxSlot* CSlot = CardVBox->AddChildToVerticalBox(CloseBtn))
			{
				CSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
			}
		}

		// Текст подсказки
		UTextBlock* HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		HintText->SetText(FText::FromString(TEXT("ЛКМ на карте — рисовать\n[ и ] — изменить радиус\nB — выход из кисти")));
		HintText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 9));
		HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.50f, 0.58f, 0.68f, 1.0f)));
		HintText->SetJustification(ETextJustify::Center);
		CardVBox->AddChildToVerticalBox(HintText);
	}

	return Super::RebuildWidget();
}

UButton* URailBrushWidget::CreateToolButton(const FString& Text, const FLinearColor& AccentColor, float Height)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SizeBox->SetMinDesiredHeight(Height);

	UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	FButtonStyle BtnStyle = MakeBrushButtonStyle(
		AccentColor,
		AccentColor * 1.35f,
		AccentColor * 0.75f
	);
	Btn->SetStyle(BtnStyle);

	UTextBlock* BtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BtnText->SetText(FText::FromString(Text));
	BtnText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
	BtnText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.96f, 0.98f, 1.0f)));
	BtnText->SetJustification(ETextJustify::Center);

	Btn->SetContent(BtnText);
	SizeBox->SetContent(Btn);

	return Btn;
}

void URailBrushWidget::UpdateRadiusDisplay(float NewRadius_m)
{
	if (RadiusValueText)
	{
		RadiusValueText->SetText(FText::FromString(FString::Printf(TEXT("%.0f м"), NewRadius_m)));
	}
}

void URailBrushWidget::UpdateZoneTypeDisplay(ERailZoneType NewType)
{
	const FLinearColor RedActive = FLinearColor(0.85f, 0.12f, 0.12f, 0.95f);
	const FLinearColor GrayActive = FLinearColor(0.40f, 0.45f, 0.52f, 0.95f);
	const FLinearColor NavyInactive = FLinearColor(0.06f, 0.14f, 0.25f, 0.85f);

	if (ForbiddenBtn)
	{
		const bool bIsActive = (NewType == ERailZoneType::ForbiddenRed);
		ForbiddenBtn->SetStyle(MakeBrushButtonStyle(
			bIsActive ? RedActive : NavyInactive,
			(bIsActive ? RedActive : NavyInactive) * 1.35f,
			(bIsActive ? RedActive : NavyInactive) * 0.75f
		));
	}

	if (EraserBtn)
	{
		const bool bIsActive = (NewType == ERailZoneType::Eraser);
		EraserBtn->SetStyle(MakeBrushButtonStyle(
			bIsActive ? GrayActive : NavyInactive,
			(bIsActive ? GrayActive : NavyInactive) * 1.35f,
			(bIsActive ? GrayActive : NavyInactive) * 0.75f
		));
	}
}

void URailBrushWidget::HandleForbiddenClicked()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->SetActiveZoneType(ERailZoneType::ForbiddenRed);
		UpdateZoneTypeDisplay(ERailZoneType::ForbiddenRed);
	}
}

void URailBrushWidget::HandleEraserClicked()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->SetActiveZoneType(ERailZoneType::Eraser);
		UpdateZoneTypeDisplay(ERailZoneType::Eraser);
	}
}

void URailBrushWidget::HandleRadiusMinusClicked()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->SetBrushRadius_m(GM->GetBrushRadius_m() - 25.0f);
	}
}

void URailBrushWidget::HandleRadiusPlusClicked()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->SetBrushRadius_m(GM->GetBrushRadius_m() + 25.0f);
	}
}

void URailBrushWidget::HandlePreset50Clicked()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->SetBrushRadius_m(50.0f);
	}
}

void URailBrushWidget::HandlePreset150Clicked()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->SetBrushRadius_m(150.0f);
	}
}

void URailBrushWidget::HandlePreset300Clicked()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->SetBrushRadius_m(300.0f);
	}
}

void URailBrushWidget::HandlePreset500Clicked()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->SetBrushRadius_m(500.0f);
	}
}

void URailBrushWidget::HandleClearAllClicked()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->ClearAllZones();
	}
}

void URailBrushWidget::HandleCloseClicked()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->SetBrushModeActive(false);
	}
	OnBrushClosed.Broadcast();
}
