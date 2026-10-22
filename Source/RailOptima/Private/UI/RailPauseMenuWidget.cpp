// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Меню паузы (Esc) в рабочем режиме проекта

#include "UI/RailPauseMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"

namespace
{
	FButtonStyle MakeMenuButtonStyle(FLinearColor BaseColor, FLinearColor HoverColor, FLinearColor PressedColor)
	{
		FButtonStyle Style;
		Style.Normal.TintColor = FSlateColor(BaseColor);
		Style.Hovered.TintColor = FSlateColor(HoverColor);
		Style.Pressed.TintColor = FSlateColor(PressedColor);
		return Style;
	}
}

TSharedRef<SWidget> URailPauseMenuWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PauseRootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		// Полупрозрачный фон с затемнением
		UBorder* DimmerBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DimmerBorder"));
		DimmerBorder->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.03f, 0.75f));

		UCanvasPanelSlot* DimmerSlot = RootCanvas->AddChildToCanvas(DimmerBorder);
		DimmerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		DimmerSlot->SetOffsets(FMargin(0.0f));

		// Центральная карточка меню
		UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardBorder"));
		CardBorder->SetBrushColor(FLinearColor(0.02f, 0.035f, 0.06f, 0.96f));
		CardBorder->SetPadding(FMargin(28.0f, 24.0f));

		UCanvasPanelSlot* CardSlot = RootCanvas->AddChildToCanvas(CardBorder);
		CardSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CardSlot->SetPosition(FVector2D(0.0f, 0.0f));
		CardSlot->SetSize(FVector2D(460.0f, 0.0f));
		CardSlot->SetAutoSize(true);

		UVerticalBox* CardVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CardVBox"));
		CardBorder->SetContent(CardVBox);

		// Заголовок
		UTextBlock* HeaderTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		HeaderTitle->SetText(FText::FromString(TEXT("МЕНЮ ПРОЕКТА")));
		HeaderTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 18));
		HeaderTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
		HeaderTitle->SetJustification(ETextJustify::Center);
		CardVBox->AddChildToVerticalBox(HeaderTitle);

		// Карточка сведений о проекте
		UBorder* InfoBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		InfoBox->SetBrushColor(FLinearColor(0.035f, 0.055f, 0.09f, 0.9f));
		InfoBox->SetPadding(FMargin(14.0f, 10.0f));
		if (UVerticalBoxSlot* InfoSlot = CardVBox->AddChildToVerticalBox(InfoBox))
		{
			InfoSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 16.0f));
		}

		UVerticalBox* InfoVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		InfoBox->SetContent(InfoVBox);

		ProjectNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ProjectNameText->SetText(FText::FromString(TEXT("Проект: Краснодарский край")));
		ProjectNameText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 13));
		ProjectNameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.98f, 1.0f)));
		InfoVBox->AddChildToVerticalBox(ProjectNameText);

		CategoryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		CategoryText->SetText(FText::FromString(TEXT("Категория: II Категория (СП 119.13330)")));
		CategoryText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
		CategoryText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.85f, 0.7f, 1.0f)));
		InfoVBox->AddChildToVerticalBox(CategoryText);

		DetailsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		DetailsText->SetText(FText::FromString(TEXT("Рельеф: 1081x1081 • Шаг: 90 м")));
		DetailsText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
		DetailsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.62f, 0.72f, 1.0f)));
		InfoVBox->AddChildToVerticalBox(DetailsText);

		// Кнопки действий
		UButton* ResumeBtn = CreateMenuButton(TEXT("Продолжить проектирование"), FLinearColor(0.0f, 0.45f, 0.75f, 0.9f));
		if (ResumeBtn) ResumeBtn->OnClicked.AddDynamic(this, &URailPauseMenuWidget::HandleResume);

		UButton* SaveBtn = CreateMenuButton(TEXT("Сохранить проект"), FLinearColor(0.05f, 0.5f, 0.35f, 0.9f));
		if (SaveBtn) SaveBtn->OnClicked.AddDynamic(this, &URailPauseMenuWidget::HandleSave);

		UButton* ExportBtn = CreateMenuButton(TEXT("Экспорт сметы в Excel (CSV)"), FLinearColor(0.18f, 0.38f, 0.75f, 0.9f));
		if (ExportBtn) ExportBtn->OnClicked.AddDynamic(this, &URailPauseMenuWidget::HandleExportExcel);

		UButton* MenuBtn = CreateMenuButton(TEXT("Главное меню (Библиотека проектов)"), FLinearColor(0.65f, 0.45f, 0.12f, 0.9f));
		if (MenuBtn) MenuBtn->OnClicked.AddDynamic(this, &URailPauseMenuWidget::HandleReturnToMainMenu);

		UButton* QuitBtn = CreateMenuButton(TEXT("Выход из программы"), FLinearColor(0.65f, 0.18f, 0.18f, 0.9f));
		if (QuitBtn) QuitBtn->OnClicked.AddDynamic(this, &URailPauseMenuWidget::HandleQuit);

		// Подсказка снизу
		UTextBlock* FooterHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FooterHint->SetText(FText::FromString(TEXT("Нажмите ESC для быстрого возврата к проектированию")));
		FooterHint->SetFont(FCoreStyle::GetDefaultFontStyle("Italic", 9));
		FooterHint->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.48f, 0.58f, 0.85f)));
		FooterHint->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* HintSlot = CardVBox->AddChildToVerticalBox(FooterHint))
		{
			HintSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
		}
	}

	return Super::RebuildWidget();
}

UButton* URailPauseMenuWidget::CreateMenuButton(const FString& Text, const FLinearColor& AccentColor)
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		return nullptr;
	}

	UBorder* CardBorder = Cast<UBorder>(WidgetTree->FindWidget(TEXT("CardBorder")));
	if (!CardBorder)
	{
		return nullptr;
	}

	UVerticalBox* CardVBox = Cast<UVerticalBox>(CardBorder->GetContentSlot()->Content);
	if (!CardVBox)
	{
		return nullptr;
	}

	UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	FButtonStyle BtnStyle = MakeMenuButtonStyle(
		AccentColor,
		AccentColor * 1.25f,
		AccentColor * 0.8f
	);
	Btn->SetStyle(BtnStyle);

	UTextBlock* BtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BtnText->SetText(FText::FromString(Text));
	BtnText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
	BtnText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	BtnText->SetJustification(ETextJustify::Center);
	Btn->SetContent(BtnText);

	if (UVerticalBoxSlot* BtnSlot = CardVBox->AddChildToVerticalBox(Btn))
	{
		BtnSlot->SetPadding(FMargin(0.0f, 4.0f));
	}

	return Btn;
}

void URailPauseMenuWidget::SetProjectInfo(const FRailProjectMetadata& InMeta)
{
	CurrentMeta = InMeta;

	if (ProjectNameText)
	{
		ProjectNameText->SetText(FText::FromString(FString::Printf(TEXT("Проект: %s"), *InMeta.ProjectName)));
	}
	if (CategoryText)
	{
		CategoryText->SetText(FText::FromString(FString::Printf(TEXT("Категория: %s"), *InMeta.CategoryName)));
	}
	if (DetailsText)
	{
		FString Details;
		if (InMeta.TotalLength_m > 0.0)
		{
			Details = FString::Printf(TEXT("Длина: %.0f м • Смета: %.1f млн руб."), InMeta.TotalLength_m, InMeta.TotalCost_kRub / 1000.0);
		}
		else
		{
			Details = TEXT("Трассировка: готова к расчету");
		}
		if (InMeta.bHasRoads)
		{
			Details += TEXT(" • Дороги (OSM)");
		}
		if (InMeta.bHasRivers)
		{
			Details += TEXT(" • Реки (OSM)");
		}
		DetailsText->SetText(FText::FromString(Details));
	}
}

void URailPauseMenuWidget::HandleResume()
{
	SetVisibility(ESlateVisibility::Collapsed);
	OnResumeClicked.Broadcast();
}

void URailPauseMenuWidget::HandleSave()
{
	OnSaveProjectClicked.Broadcast();
}

void URailPauseMenuWidget::HandleExportExcel()
{
	OnExportExcelClicked.Broadcast();
}

void URailPauseMenuWidget::HandleReturnToMainMenu()
{
	SetVisibility(ESlateVisibility::Collapsed);
	OnReturnToMainMenuClicked.Broadcast();
}

void URailPauseMenuWidget::HandleQuit()
{
	OnQuitClicked.Broadcast();
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}
