// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект: UI-виджет сводной таблицы стоимости строительства пути, ПривГУПС, 2026.

#include "Visualization/RailCostSummaryWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "RailOptimaGameMode/RailOptimaGameMode.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> URailCostSummaryWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		// Тёмная полупрозрачная карточка в правом верхнем углу
		UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardBorder"));
		CardBorder->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.06f, 0.88f));
		CardBorder->SetPadding(FMargin(20.0f, 16.0f));

		UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(CardBorder);
		CanvasSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
		CanvasSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		CanvasSlot->SetPosition(FVector2D(-28.0f, 28.0f));
		CanvasSlot->SetAutoSize(true);

		UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
		CardBorder->SetContent(MainVBox);

		// Заголовок таблицы
		UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		TitleText->SetText(FText::FromString(TEXT("СМЕТА СТРОИТЕЛЬСТВА ПУТИ (НЦС 81-02-07-2025)")));
		TitleText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 13));
		TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
		if (UVerticalBoxSlot* TitleSlot = MainVBox->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}

		// Вспомогательная лямбда для создания строки таблицы (принимает TObjectPtr<UTextBlock>&)
		auto AddRow = [&](const FString& LabelStr, TObjectPtr<UTextBlock>& OutValueBlock, FLinearColor ValueColor, int32 FontSize, bool bBold) {
			UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			if (UVerticalBoxSlot* VSlot = MainVBox->AddChildToVerticalBox(RowBox))
			{
				VSlot->SetPadding(FMargin(0.0f, 4.0f));
			}

			UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Lbl->SetText(FText::FromString(LabelStr));
			Lbl->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
			Lbl->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.76f, 0.82f, 1.0f)));
			if (UHorizontalBoxSlot* LSlot = RowBox->AddChildToHorizontalBox(Lbl))
			{
				LSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				LSlot->SetHorizontalAlignment(HAlign_Left);
				LSlot->SetVerticalAlignment(VAlign_Center);
			}

			USpacer* RowSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
			RowSpacer->SetSize(FVector2D(32.0f, 1.0f));
			RowBox->AddChildToHorizontalBox(RowSpacer);

			OutValueBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			OutValueBlock->SetText(FText::FromString(TEXT("—")));
			OutValueBlock->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", FontSize));
			OutValueBlock->SetColorAndOpacity(FSlateColor(ValueColor));
			if (UHorizontalBoxSlot* RSlot = RowBox->AddChildToHorizontalBox(OutValueBlock))
			{
				RSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				RSlot->SetHorizontalAlignment(HAlign_Right);
				RSlot->SetVerticalAlignment(VAlign_Center);
			}
		};

		AddRow(TEXT("Категория дороги:"), CategoryValueText, FLinearColor::White, 11, true);
		AddRow(TEXT("Протяжённость трассы:"), LengthValueText, FLinearColor::White, 11, false);
		AddRow(TEXT("Средняя цена 1 км:"), AvgRateValueText, FLinearColor(0.9f, 0.9f, 0.9f, 1.0f), 11, false);
		AddRow(TEXT("Макс. уклон / Мин. радиус:"), ProfileValueText, FLinearColor(1.0f, 0.85f, 0.35f, 1.0f), 11, false);
		AddRow(TEXT("ИТОГОВАЯ СТОИМОСТЬ:"), TotalCostValueText, FLinearColor(0.2f, 0.95f, 0.4f, 1.0f), 15, true);
		AddRow(TEXT("Время расчёта:"), CalcTimeValueText, FLinearColor(0.55f, 0.6f, 0.65f, 1.0f), 10, false);

		// Разделитель
		USpacer* Sep1 = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		Sep1->SetSize(FVector2D(1.0f, 10.0f));
		MainVBox->AddChildToVerticalBox(Sep1);

		// Кнопка-тагл видимости станций (H)
		EndpointsToggleBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EndpointsToggleBtn"));
		FButtonStyle BtnStyle;
		BtnStyle.Normal.TintColor = FSlateColor(FLinearColor(0.08f, 0.22f, 0.38f, 0.9f));
		BtnStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.12f, 0.35f, 0.58f, 1.0f));
		BtnStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.05f, 0.16f, 0.28f, 1.0f));
		EndpointsToggleBtn->SetStyle(BtnStyle);
		EndpointsToggleBtn->OnClicked.AddDynamic(this, &URailCostSummaryWidget::HandleToggleEndpointsClicked);

		if (UVerticalBoxSlot* BtnSlot = MainVBox->AddChildToVerticalBox(EndpointsToggleBtn))
		{
			BtnSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 8.0f));
		}

		EndpointsToggleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EndpointsToggleText"));
		EndpointsToggleText->SetText(FText::FromString(TEXT("👁  Скрыть станции (H)")));
		EndpointsToggleText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
		EndpointsToggleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.92f, 1.0f, 1.0f)));
		EndpointsToggleText->SetJustification(ETextJustify::Center);
		EndpointsToggleBtn->SetContent(EndpointsToggleText);

		// Разделитель
		USpacer* Sep2 = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		Sep2->SetSize(FVector2D(1.0f, 6.0f));
		MainVBox->AddChildToVerticalBox(Sep2);

		// Заголовок подсказки по управлению
		UTextBlock* HelpHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		HelpHeader->SetText(FText::FromString(TEXT("УПРАВЛЕНИЕ И ГОРЯЧИЕ КЛАВИШИ:")));
		HelpHeader->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
		HelpHeader->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
		if (UVerticalBoxSlot* HdrSlot = MainVBox->AddChildToVerticalBox(HelpHeader))
		{
			HdrSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 4.0f));
		}

		auto AddHelpRow = [&](const FString& KeyStr, const FString& ActionStr) {
			UHorizontalBox* HelpBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			if (UVerticalBoxSlot* HVSlot = MainVBox->AddChildToVerticalBox(HelpBox))
			{
				HVSlot->SetPadding(FMargin(0.0f, 1.5f));
			}

			UTextBlock* KeyBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			KeyBlock->SetText(FText::FromString(KeyStr));
			KeyBlock->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 9));
			KeyBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.85f, 0.45f, 1.0f)));
			if (UHorizontalBoxSlot* KSlot = HelpBox->AddChildToHorizontalBox(KeyBlock))
			{
				KSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				KSlot->SetVerticalAlignment(VAlign_Center);
			}

			USpacer* Spc = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
			Spc->SetSize(FVector2D(12.0f, 1.0f));
			HelpBox->AddChildToHorizontalBox(Spc);

			UTextBlock* ActBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			ActBlock->SetText(FText::FromString(ActionStr));
			ActBlock->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 9));
			ActBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.80f, 0.88f, 1.0f)));
			if (UHorizontalBoxSlot* ASlot = HelpBox->AddChildToHorizontalBox(ActBlock))
			{
				ASlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				ASlot->SetHorizontalAlignment(HAlign_Right);
				ASlot->SetVerticalAlignment(VAlign_Center);
			}
		};

		AddHelpRow(TEXT("WASD / Shift"), TEXT("Полет / Ускорение камеры"));
		AddHelpRow(TEXT("ПКМ (зажать)"), TEXT("Свободный обзор"));
		AddHelpRow(TEXT("Space / Ctrl"), TEXT("Подъём / Спуск камеры"));
		AddHelpRow(TEXT("Колесо мыши"), TEXT("Зум / Высота Top-Down"));
		AddHelpRow(TEXT("ЛКМ (зажать)"), TEXT("Перемещение станции (400м)"));
		AddHelpRow(TEXT("ЛКМ (x2 на пути)"), TEXT("Добавить контрольную точку"));
		AddHelpRow(TEXT("ПКМ (по точке)"), TEXT("Удалить контрольную точку"));
		AddHelpRow(TEXT("H"), TEXT("Скрыть / Показать станции"));
		AddHelpRow(TEXT("F1"), TEXT("Продольный профиль высот"));
		AddHelpRow(TEXT("B / [ / ]"), TEXT("Кисть зон / Радиус"));
		AddHelpRow(TEXT("Esc"), TEXT("Меню настроек и смета"));
	}

	return Super::RebuildWidget();
}

void URailCostSummaryWidget::HandleToggleEndpointsClicked()
{
	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->ToggleEndpointsVisibility();
	}
}

void URailCostSummaryWidget::UpdateEndpointsToggleState(bool bVisible)
{
	if (EndpointsToggleText)
	{
		EndpointsToggleText->SetText(FText::FromString(bVisible ? TEXT("👁  Скрыть станции (H)") : TEXT("👁  Показать станции (H)")));
	}
}

FString URailCostSummaryWidget::FormatMoneyRubles(double InCost_kRub)
{
	if (InCost_kRub <= 0.0)
	{
		return TEXT("0 руб.");
	}

	const double Millions = InCost_kRub / 1000.0;
	const double Billions = InCost_kRub / 1000000.0;

	if (Billions >= 1.0)
	{
		return FString::Printf(TEXT("%.2f млрд руб. (%.0f млн руб.)"), Billions, Millions);
	}
	return FString::Printf(TEXT("%.1f млн руб."), Millions);
}

void URailCostSummaryWidget::UpdateRouteSummary(
	const FString& InCategoryName,
	double		   InLength_m,
	double		   InTotalCost_kRub,
	float		   InMaxSlope_perm,
	float		   InMinRadius_m,
	double		   InTime_ms)
{
	if (CategoryValueText)
	{
		CategoryValueText->SetText(FText::FromString(InCategoryName));
	}

	const double Length_km = InLength_m / 1000.0;
	if (LengthValueText)
	{
		LengthValueText->SetText(FText::FromString(
			FString::Printf(TEXT("%.2f км (%.0f м)"), Length_km, InLength_m)));
	}

	if (AvgRateValueText)
	{
		const double AvgRate_kRub = (Length_km > 0.001) ? (InTotalCost_kRub / Length_km) : 0.0;
		AvgRateValueText->SetText(FText::FromString(
			FString::Printf(TEXT("%.1f млн руб./км"), AvgRate_kRub / 1000.0)));
	}

	if (ProfileValueText)
	{
		const FString RadiusStr = (InMinRadius_m > 0.0f && InMinRadius_m < 99999.0f)
			? FString::Printf(TEXT("%.0f м"), InMinRadius_m)
			: TEXT("Прямая");

		ProfileValueText->SetText(FText::FromString(
			FString::Printf(TEXT("%.1f ‰  |  R = %s"), InMaxSlope_perm, *RadiusStr)));
	}

	if (TotalCostValueText)
	{
		TotalCostValueText->SetText(FText::FromString(FormatMoneyRubles(InTotalCost_kRub)));

		// Подсвечиваем итоговую цену цветом в зависимости от дороговизны 1 км
		const double AvgRate_kRub = (Length_km > 0.001) ? (InTotalCost_kRub / Length_km) : 0.0;
		FLinearColor PriceColor = FLinearColor(0.2f, 0.95f, 0.4f, 1.0f); // Зелёный
		if (AvgRate_kRub > 850000.0)
		{
			PriceColor = FLinearColor(1.0f, 0.25f, 0.25f, 1.0f); // Красный
		}
		else if (AvgRate_kRub > 550000.0)
		{
			PriceColor = FLinearColor(1.0f, 0.75f, 0.2f, 1.0f); // Оранжево-жёлтый
		}
		TotalCostValueText->SetColorAndOpacity(FSlateColor(PriceColor));
	}

	if (CalcTimeValueText)
	{
		CalcTimeValueText->SetText(FText::FromString(FString::Printf(TEXT("%.1f мс"), InTime_ms)));
	}
}