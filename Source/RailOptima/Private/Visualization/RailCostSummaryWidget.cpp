// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект: UI-виджет сводной таблицы стоимости строительства пути, ПривГУПС, 2026.

#include "Visualization/RailCostSummaryWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
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
	}

	return Super::RebuildWidget();
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