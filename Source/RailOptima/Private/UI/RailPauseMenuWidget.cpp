// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Меню паузы (Esc) в рабочем режиме проекта

#include "UI/RailPauseMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "RailOptimaGameMode/RailOptimaGameMode.h"
#include "Services/RailSettingsManager.h"
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

	void ApplyPauseComboBoxStyle(UComboBoxString* Combo)
	{
		if (!Combo) return;
		FComboBoxStyle WidgetStyle = Combo->GetWidgetStyle();
		WidgetStyle.ComboButtonStyle.ButtonStyle.Normal.TintColor = FSlateColor(FLinearColor(0.04f, 0.08f, 0.15f, 0.95f));
		WidgetStyle.ComboButtonStyle.ButtonStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.07f, 0.14f, 0.25f, 1.0f));
		WidgetStyle.ComboButtonStyle.ButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.02f, 0.05f, 0.10f, 1.0f));
		WidgetStyle.ComboButtonStyle.MenuBorderBrush.TintColor = FSlateColor(FLinearColor(0.02f, 0.04f, 0.08f, 0.98f));
		WidgetStyle.ComboButtonStyle.DownArrowImage.TintColor = FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f));
		Combo->SetWidgetStyle(WidgetStyle);

		FTableRowStyle ItemStyle = Combo->GetItemStyle();
		ItemStyle.TextColor = FSlateColor(FLinearColor(0.95f, 0.95f, 0.98f, 1.0f));
		ItemStyle.SelectedTextColor = FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f));
		Combo->SetItemStyle(ItemStyle);
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
		DimmerBorder->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.03f, 0.78f));

		UCanvasPanelSlot* DimmerSlot = RootCanvas->AddChildToCanvas(DimmerBorder);
		DimmerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		DimmerSlot->SetOffsets(FMargin(0.0f));

		// Горизонтальный контейнер с двумя карточками (Меню слева, Гайд справа)
		UHorizontalBox* CenterHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CenterHBox"));
		UCanvasPanelSlot* CenterSlot = RootCanvas->AddChildToCanvas(CenterHBox);
		CenterSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		CenterSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CenterSlot->SetPosition(FVector2D(0.0f, 0.0f));
		CenterSlot->SetAutoSize(true);

		// ─── ЛЕВАЯ КОЛОНКА: МЕНЮ ДЕЙСТВИЙ И КАТЕГОРИЯ ───
		USizeBox* MenuSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		MenuSizeBox->SetWidthOverride(440.0f);
		CenterHBox->AddChildToHorizontalBox(MenuSizeBox);

		UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardBorder"));
		CardBorder->SetBrushColor(FLinearColor(0.02f, 0.035f, 0.06f, 0.96f));
		CardBorder->SetPadding(FMargin(24.0f, 20.0f));
		MenuSizeBox->SetContent(CardBorder);

		UVerticalBox* CardVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CardVBox"));
		CardBorder->SetContent(CardVBox);

		// Заголовок
		UTextBlock* HeaderTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		HeaderTitle->SetText(FText::FromString(TEXT("МЕНЮ ПРОЕКТА")));
		HeaderTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 17));
		HeaderTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
		HeaderTitle->SetJustification(ETextJustify::Center);
		CardVBox->AddChildToVerticalBox(HeaderTitle);

		// Карточка сведений о проекте
		UBorder* InfoBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		InfoBox->SetBrushColor(FLinearColor(0.035f, 0.055f, 0.09f, 0.9f));
		InfoBox->SetPadding(FMargin(14.0f, 10.0f));
		if (UVerticalBoxSlot* InfoSlot = CardVBox->AddChildToVerticalBox(InfoBox))
		{
			InfoSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 12.0f));
		}

		UVerticalBox* InfoVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		InfoBox->SetContent(InfoVBox);

		ProjectNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ProjectNameText->SetText(FText::FromString(TEXT("Проект: Краснодарский край")));
		ProjectNameText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
		ProjectNameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.98f, 1.0f)));
		InfoVBox->AddChildToVerticalBox(ProjectNameText);

		DetailsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		DetailsText->SetText(FText::FromString(TEXT("Рельеф: 1081x1081 • Шаг: 90 м")));
		DetailsText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
		DetailsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.62f, 0.72f, 1.0f)));
		InfoVBox->AddChildToVerticalBox(DetailsText);

		// Выбор нормативной категории ЖД (СП 119.13330)
		UTextBlock* CatLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		CatLabel->SetText(FText::FromString(TEXT("Категория ЖД (СП 119.13330):")));
		CatLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
		CatLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.85f, 0.7f, 1.0f)));
		if (UVerticalBoxSlot* CatSlot = CardVBox->AddChildToVerticalBox(CatLabel))
		{
			CatSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 4.0f));
		}

		CategoryComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PauseCategoryComboBox"));
		ApplyPauseComboBoxStyle(CategoryComboBox);
		CategoryComboBox->AddOption(TEXT("I Категория (магистральная)"));
		CategoryComboBox->AddOption(TEXT("II Категория"));
		CategoryComboBox->AddOption(TEXT("III Категория"));
		CategoryComboBox->AddOption(TEXT("IV Категория (местная)"));
		CategoryComboBox->AddOption(TEXT("Высокоскоростная (ВСМ)"));
		CategoryComboBox->AddOption(TEXT("Скоростная пассажирская"));
		CategoryComboBox->AddOption(TEXT("Особогрузонапряженная"));
		CategoryComboBox->AddOption(TEXT("Подъездные пути"));
		CategoryComboBox->SetSelectedOption(TEXT("I Категория (магистральная)"));
		CategoryComboBox->OnSelectionChanged.AddDynamic(this, &URailPauseMenuWidget::HandleCategorySelectionChanged);

		if (UVerticalBoxSlot* ComboSlot = CardVBox->AddChildToVerticalBox(CategoryComboBox))
		{
			ComboSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}

		// Кнопки действий (все одного цвета и комфортного размера)
		const FLinearColor UnifiedBtnColor = FLinearColor(0.08f, 0.22f, 0.38f, 0.95f);

		UButton* ResumeBtn = CreateMenuButton(TEXT("Продолжить проектирование"), UnifiedBtnColor);
		if (ResumeBtn) ResumeBtn->OnClicked.AddDynamic(this, &URailPauseMenuWidget::HandleResume);

		UButton* VisualSettingsBtn = CreateMenuButton(TEXT("🛠 Параметры камеры и пути"), UnifiedBtnColor);
		if (VisualSettingsBtn) VisualSettingsBtn->OnClicked.AddDynamic(this, &URailPauseMenuWidget::HandleOpenVisualSettings);

		UButton* SettingsBtn = CreateMenuButton(TEXT("⚙ Настройки графики и качества"), UnifiedBtnColor);
		if (SettingsBtn) SettingsBtn->OnClicked.AddDynamic(this, &URailPauseMenuWidget::HandleGraphicsSettings);

		UButton* SaveBtn = CreateMenuButton(TEXT("Сохранить проект"), UnifiedBtnColor);
		if (SaveBtn) SaveBtn->OnClicked.AddDynamic(this, &URailPauseMenuWidget::HandleSave);

		UButton* ExportBtn = CreateMenuButton(TEXT("Экспорт сметы в Excel (CSV)"), UnifiedBtnColor);
		if (ExportBtn) ExportBtn->OnClicked.AddDynamic(this, &URailPauseMenuWidget::HandleExportExcel);

		UButton* MenuBtn = CreateMenuButton(TEXT("Главное меню (Библиотека проектов)"), UnifiedBtnColor);
		if (MenuBtn) MenuBtn->OnClicked.AddDynamic(this, &URailPauseMenuWidget::HandleReturnToMainMenu);

		UButton* QuitBtn = CreateMenuButton(TEXT("Выход из программы"), UnifiedBtnColor);
		if (QuitBtn) QuitBtn->OnClicked.AddDynamic(this, &URailPauseMenuWidget::HandleQuit);

		// Разделитель между колонками
		USpacer* ColSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		ColSpacer->SetSize(FVector2D(24.0f, 1.0f));
		CenterHBox->AddChildToHorizontalBox(ColSpacer);

		// ─── ПРАВАЯ КОЛОНКА: ГАЙД ПО УПРАВЛЕНИЮ ───
		USizeBox* GuideSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		GuideSizeBox->SetWidthOverride(530.0f);
		CenterHBox->AddChildToHorizontalBox(GuideSizeBox);

		UBorder* GuideBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("GuideBorder"));
		GuideBorder->SetBrushColor(FLinearColor(0.02f, 0.035f, 0.06f, 0.96f));
		GuideBorder->SetPadding(FMargin(24.0f, 20.0f));
		GuideSizeBox->SetContent(GuideBorder);

		UVerticalBox* GuideVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GuideVBox"));
		GuideBorder->SetContent(GuideVBox);

		UTextBlock* GuideTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		GuideTitle->SetText(FText::FromString(TEXT("РУКОВОДСТВО ПО УПРАВЛЕНИЮ")));
		GuideTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
		GuideTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
		GuideTitle->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* TitleSlot = GuideVBox->AddChildToVerticalBox(GuideTitle))
		{
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}

		auto AddGuideRow = [&](const FString& KeyText, const FString& DescText) {
			UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			RowBorder->SetBrushColor(FLinearColor(0.035f, 0.055f, 0.09f, 0.75f));
			RowBorder->SetPadding(FMargin(12.0f, 7.0f));
			if (UVerticalBoxSlot* Slot = GuideVBox->AddChildToVerticalBox(RowBorder))
			{
				Slot->SetPadding(FMargin(0.0f, 2.5f, 0.0f, 2.5f));
			}

			UHorizontalBox* RowHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			RowBorder->SetContent(RowHBox);

			// Левая колонка (Клавиша/действие) - фиксированная ширина для строгого выравнивания
			USizeBox* KeyBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			KeyBox->SetWidthOverride(190.0f);
			RowHBox->AddChildToHorizontalBox(KeyBox);

			UTextBlock* KeyLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			KeyLabel->SetText(FText::FromString(KeyText));
			KeyLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
			KeyLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
			KeyBox->SetContent(KeyLabel);

			// Правая колонка (Описание) - выравнивание строго влево от границы первой колонки
			UTextBlock* DescLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			DescLabel->SetText(FText::FromString(DescText));
			DescLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
			DescLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.94f, 0.98f, 1.0f)));
			DescLabel->SetAutoWrapText(true);
			if (UHorizontalBoxSlot* HSlot = RowHBox->AddChildToHorizontalBox(DescLabel))
			{
				HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				HSlot->SetHorizontalAlignment(HAlign_Left);
				HSlot->SetVerticalAlignment(VAlign_Center);
				HSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
			}
		};

		AddGuideRow(TEXT("ЛКМ (зажать маркер)"), TEXT("Перемещение станции по карте"));
		AddGuideRow(TEXT("Двойной клик ЛКМ"), TEXT("Создать промежуточную точку (Waypoint)"));
		AddGuideRow(TEXT("ПКМ (по точке)"), TEXT("Удалить промежуточную точку"));
		AddGuideRow(TEXT("ПКМ (в воздухе)"), TEXT("Свободное вращение камеры"));
		AddGuideRow(TEXT("WASD"), TEXT("Полет камеры над рельефом"));
		AddGuideRow(TEXT("Зажатый Shift"), TEXT("Ускорение полета камеры"));
		AddGuideRow(TEXT("Пробел"), TEXT("Подъем камеры по высоте"));
		AddGuideRow(TEXT("Колесо мыши"), TEXT("Плавное приближение / удаление"));
		AddGuideRow(TEXT("Клавиша B"), TEXT("Вкл/выкл кисти запретных зон"));
		AddGuideRow(TEXT("ЛКМ в режиме B"), TEXT("Рисование запретных зон (красный)"));
		AddGuideRow(TEXT("Клавиша F1"), TEXT("Продольный профиль трассы (высоты)"));
		AddGuideRow(TEXT("Клавиша ESC"), TEXT("Меню проекта / возврат к работе"));
	}

	BuildVisualSettingsModal(Cast<UCanvasPanel>(WidgetTree->RootWidget));

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

	USizeBox* BtnSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BtnSizeBox->SetMinDesiredHeight(44.0f);

	UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	FButtonStyle BtnStyle = MakeMenuButtonStyle(
		AccentColor,
		AccentColor * 1.35f,
		AccentColor * 0.75f
	);
	Btn->SetStyle(BtnStyle);

	UTextBlock* BtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BtnText->SetText(FText::FromString(Text));
	BtnText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
	BtnText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	BtnText->SetJustification(ETextJustify::Center);
	Btn->SetContent(BtnText);

	BtnSizeBox->SetContent(Btn);

	if (UVerticalBoxSlot* BtnSlot = CardVBox->AddChildToVerticalBox(BtnSizeBox))
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
	if (CategoryComboBox && !InMeta.CategoryName.IsEmpty())
	{
		CategoryComboBox->SetSelectedOption(InMeta.CategoryName);
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

void URailPauseMenuWidget::HandleGraphicsSettings()
{
	OnGraphicsSettingsClicked.Broadcast();
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

void URailPauseMenuWidget::HandleCategorySelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	ERailRoadCategory NewCat = ERailRoadCategory::Category1;
	if (SelectedItem.Contains(TEXT("ВСМ")) || SelectedItem.Contains(TEXT("Высокоскоростная")))
	{
		NewCat = ERailRoadCategory::HighSpeed;
	}
	else if (SelectedItem.Contains(TEXT("пассажирская")))
	{
		NewCat = ERailRoadCategory::Passenger;
	}
	else if (SelectedItem.Contains(TEXT("Особогрузонапряженная")))
	{
		NewCat = ERailRoadCategory::SuperHeavyHaul;
	}
	else if (SelectedItem.Contains(TEXT("I Категория")))
	{
		NewCat = ERailRoadCategory::Category1;
	}
	else if (SelectedItem.Contains(TEXT("II Категория")))
	{
		NewCat = ERailRoadCategory::Category2;
	}
	else if (SelectedItem.Contains(TEXT("III Категория")))
	{
		NewCat = ERailRoadCategory::Category3;
	}
	else if (SelectedItem.Contains(TEXT("IV Категория")))
	{
		NewCat = ERailRoadCategory::Category4Lines;
	}
	else if (SelectedItem.Contains(TEXT("Подъездные")))
	{
		NewCat = ERailRoadCategory::Category4Access;
	}

	CurrentMeta.Category = NewCat;
	CurrentMeta.CategoryName = SelectedItem;

	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->SetSelectedCategory(NewCat);
	}
}

UWidget* URailPauseMenuWidget::HandleGenerateCategoryWidget(FString Item)
{
	UTextBlock* ItemText = WidgetTree ? WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()) : NewObject<UTextBlock>(this);
	ItemText->SetText(FText::FromString(Item));
	ItemText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	ItemText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.96f, 0.99f, 1.0f)));
	return ItemText;
}

void URailPauseMenuWidget::BuildVisualSettingsModal(UCanvasPanel* RootCanvas)
{
	if (!RootCanvas || !WidgetTree) return;

	// Полупрозрачный оверлей затемнения
	VisualSettingsModalBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("VisualSettingsModalBorder"));
	VisualSettingsModalBorder->SetBrushColor(FLinearColor(0.008f, 0.012f, 0.025f, 0.88f));
	VisualSettingsModalBorder->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* OverlaySlot = RootCanvas->AddChildToCanvas(VisualSettingsModalBorder);
	OverlaySlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	OverlaySlot->SetOffsets(FMargin(0.0f));

	// Внутренний канвас для позиционирования карточки
	UCanvasPanel* ModalInnerCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	VisualSettingsModalBorder->SetContent(ModalInnerCanvas);

	// Карточка по центру
	USizeBox* ModalCardBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ModalCardBox->SetWidthOverride(500.0f);

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Card->SetBrushColor(FLinearColor(0.02f, 0.035f, 0.065f, 0.98f));
	Card->SetPadding(FMargin(24.0f, 20.0f));
	ModalCardBox->SetContent(Card);

	UCanvasPanelSlot* InnerSlot = ModalInnerCanvas->AddChildToCanvas(ModalCardBox);
	InnerSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
	InnerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	InnerSlot->SetPosition(FVector2D::ZeroVector);
	InnerSlot->SetAutoSize(true);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Card->SetContent(VBox);

	// Заголовок
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("ПАРАМЕТРЫ КАМЕРЫ И ПУТИ")));
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 15));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	Title->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* TitleSlot = VBox->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	auto AddSettingRow = [&](const FString& LabelText, TObjectPtr<UComboBoxString>& OutCombo) {
		UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Lbl->SetText(FText::FromString(LabelText));
		Lbl->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
		Lbl->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.85f, 0.95f, 1.0f)));
		if (UVerticalBoxSlot* LblSlot = VBox->AddChildToVerticalBox(Lbl))
		{
			LblSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 2.0f));
		}

		OutCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
		ApplyPauseComboBoxStyle(OutCombo);
		if (UVerticalBoxSlot* ComboSlot = VBox->AddChildToVerticalBox(OutCombo))
		{
			ComboSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
	};

	// 1. Скорость полета камеры
	AddSettingRow(TEXT("Скорость свободного полета камеры (WASD):"), CameraSpeedCombo);
	CameraSpeedCombo->AddOption(TEXT("300 м/с (медленная)"));
	CameraSpeedCombo->AddOption(TEXT("500 м/с"));
	CameraSpeedCombo->AddOption(TEXT("1000 м/с (по умолчанию)"));
	CameraSpeedCombo->AddOption(TEXT("2000 м/с (быстрая)"));
	CameraSpeedCombo->AddOption(TEXT("3500 м/с (очень быстрая)"));
	CameraSpeedCombo->AddOption(TEXT("5000 м/с (максимальная)"));
	CameraSpeedCombo->SetSelectedOption(TEXT("1000 м/с (по умолчанию)"));

	// 2. Ускорение Shift
	AddSettingRow(TEXT("Ускорение при зажатом Shift:"), ShiftBoostCombo);
	ShiftBoostCombo->AddOption(TEXT("2.0x"));
	ShiftBoostCombo->AddOption(TEXT("3.0x"));
	ShiftBoostCombo->AddOption(TEXT("4.0x (по умолчанию)"));
	ShiftBoostCombo->AddOption(TEXT("6.0x"));
	ShiftBoostCombo->AddOption(TEXT("8.0x"));
	ShiftBoostCombo->AddOption(TEXT("10.0x"));
	ShiftBoostCombo->SetSelectedOption(TEXT("4.0x (по умолчанию)"));

	// 3. Ширина пути
	AddSettingRow(TEXT("Ширина отображаемого пути (ленты):"), PathWidthCombo);
	PathWidthCombo->AddOption(TEXT("20 м"));
	PathWidthCombo->AddOption(TEXT("40 м (по умолчанию)"));
	PathWidthCombo->AddOption(TEXT("60 м"));
	PathWidthCombo->AddOption(TEXT("80 м"));
	PathWidthCombo->AddOption(TEXT("100 м"));
	PathWidthCombo->SetSelectedOption(TEXT("40 м (по умолчанию)"));

	// 4. Высота пути
	AddSettingRow(TEXT("Высота пути над рельефом (отступ Z):"), PathHeightCombo);
	PathHeightCombo->AddOption(TEXT("20 м"));
	PathHeightCombo->AddOption(TEXT("50 м"));
	PathHeightCombo->AddOption(TEXT("80 м (по умолчанию)"));
	PathHeightCombo->AddOption(TEXT("120 м"));
	PathHeightCombo->AddOption(TEXT("200 м"));
	PathHeightCombo->SetSelectedOption(TEXT("80 м (по умолчанию)"));

	// 5. Масштаб станций и точек
	AddSettingRow(TEXT("Масштаб маркеров станций и точек:"), EndpointScaleCombo);
	EndpointScaleCombo->AddOption(TEXT("50% (компактный)"));
	EndpointScaleCombo->AddOption(TEXT("75%"));
	EndpointScaleCombo->AddOption(TEXT("100% (нормальный)"));
	EndpointScaleCombo->AddOption(TEXT("150% (увеличенный)"));
	EndpointScaleCombo->AddOption(TEXT("200% (крупный)"));
	EndpointScaleCombo->SetSelectedOption(TEXT("100% (нормальный)"));

	// Кнопки управления
	UHorizontalBox* BtnHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* BtnSlot = VBox->AddChildToVerticalBox(BtnHBox))
	{
		BtnSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
	}

	UButton* ApplyBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	ApplyBtn->SetStyle(MakeMenuButtonStyle(
		FLinearColor(0.10f, 0.42f, 0.30f, 0.95f),
		FLinearColor(0.15f, 0.58f, 0.42f, 1.0f),
		FLinearColor(0.06f, 0.28f, 0.20f, 1.0f)));
	ApplyBtn->OnClicked.AddDynamic(this, &URailPauseMenuWidget::HandleApplyVisualSettings);

	UTextBlock* ApplyTxt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ApplyTxt->SetText(FText::FromString(TEXT("Применить и сохранить")));
	ApplyTxt->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	ApplyTxt->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.98f, 0.96f, 1.0f)));
	ApplyTxt->SetJustification(ETextJustify::Center);
	ApplyBtn->AddChild(ApplyTxt);

	if (UHorizontalBoxSlot* HSlot = BtnHBox->AddChildToHorizontalBox(ApplyBtn))
	{
		HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	UButton* CancelBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CancelBtn->SetStyle(MakeMenuButtonStyle(
		FLinearColor(0.12f, 0.16f, 0.24f, 0.95f),
		FLinearColor(0.18f, 0.24f, 0.36f, 1.0f),
		FLinearColor(0.08f, 0.10f, 0.16f, 1.0f)));
	CancelBtn->OnClicked.AddDynamic(this, &URailPauseMenuWidget::HandleCloseVisualSettings);

	UTextBlock* CancelTxt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CancelTxt->SetText(FText::FromString(TEXT("Закрыть")));
	CancelTxt->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	CancelTxt->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.95f, 1.0f)));
	CancelTxt->SetJustification(ETextJustify::Center);
	CancelBtn->AddChild(CancelTxt);

	if (UHorizontalBoxSlot* HSlot = BtnHBox->AddChildToHorizontalBox(CancelBtn))
	{
		HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}
}

void URailPauseMenuWidget::HandleOpenVisualSettings()
{
	if (!VisualSettingsModalBorder) return;

	const float FlySpeed = URailSettingsManager::GetCameraFlySpeed();
	if (CameraSpeedCombo)
	{
		if (FlySpeed <= 35000.0f) CameraSpeedCombo->SetSelectedOption(TEXT("300 м/с (медленная)"));
		else if (FlySpeed <= 60000.0f) CameraSpeedCombo->SetSelectedOption(TEXT("500 м/с"));
		else if (FlySpeed <= 120000.0f) CameraSpeedCombo->SetSelectedOption(TEXT("1000 м/с (по умолчанию)"));
		else if (FlySpeed <= 250000.0f) CameraSpeedCombo->SetSelectedOption(TEXT("2000 м/с (быстрая)"));
		else if (FlySpeed <= 400000.0f) CameraSpeedCombo->SetSelectedOption(TEXT("3500 м/с (очень быстрая)"));
		else CameraSpeedCombo->SetSelectedOption(TEXT("5000 м/с (максимальная)"));
	}

	const float ShiftBoost = URailSettingsManager::GetCameraShiftBoost();
	if (ShiftBoostCombo)
	{
		if (ShiftBoost <= 2.2f) ShiftBoostCombo->SetSelectedOption(TEXT("2.0x"));
		else if (ShiftBoost <= 3.2f) ShiftBoostCombo->SetSelectedOption(TEXT("3.0x"));
		else if (ShiftBoost <= 4.5f) ShiftBoostCombo->SetSelectedOption(TEXT("4.0x (по умолчанию)"));
		else if (ShiftBoost <= 6.5f) ShiftBoostCombo->SetSelectedOption(TEXT("6.0x"));
		else if (ShiftBoost <= 8.5f) ShiftBoostCombo->SetSelectedOption(TEXT("8.0x"));
		else ShiftBoostCombo->SetSelectedOption(TEXT("10.0x"));
	}

	const float PathW = URailSettingsManager::GetPathRibbonWidth();
	if (PathWidthCombo)
	{
		if (PathW <= 2500.0f) PathWidthCombo->SetSelectedOption(TEXT("20 м"));
		else if (PathW <= 4500.0f) PathWidthCombo->SetSelectedOption(TEXT("40 м (по умолчанию)"));
		else if (PathW <= 6500.0f) PathWidthCombo->SetSelectedOption(TEXT("60 м"));
		else if (PathW <= 8500.0f) PathWidthCombo->SetSelectedOption(TEXT("80 м"));
		else PathWidthCombo->SetSelectedOption(TEXT("100 м"));
	}

	const float PathH = URailSettingsManager::GetPathRibbonHeight();
	if (PathHeightCombo)
	{
		if (PathH <= 3000.0f) PathHeightCombo->SetSelectedOption(TEXT("20 м"));
		else if (PathH <= 6000.0f) PathHeightCombo->SetSelectedOption(TEXT("50 м"));
		else if (PathH <= 9000.0f) PathHeightCombo->SetSelectedOption(TEXT("80 м (по умолчанию)"));
		else if (PathH <= 15000.0f) PathHeightCombo->SetSelectedOption(TEXT("120 м"));
		else PathHeightCombo->SetSelectedOption(TEXT("200 м"));
	}

	const float PtScale = URailSettingsManager::GetEndpointScaleMultiplier();
	if (EndpointScaleCombo)
	{
		if (PtScale <= 0.6f) EndpointScaleCombo->SetSelectedOption(TEXT("50% (компактный)"));
		else if (PtScale <= 0.85f) EndpointScaleCombo->SetSelectedOption(TEXT("75%"));
		else if (PtScale <= 1.2f) EndpointScaleCombo->SetSelectedOption(TEXT("100% (нормальный)"));
		else if (PtScale <= 1.7f) EndpointScaleCombo->SetSelectedOption(TEXT("150% (увеличенный)"));
		else EndpointScaleCombo->SetSelectedOption(TEXT("200% (крупный)"));
	}

	VisualSettingsModalBorder->SetVisibility(ESlateVisibility::Visible);
}

void URailPauseMenuWidget::HandleApplyVisualSettings()
{
	float FlySpeed = 100000.0f;
	if (CameraSpeedCombo)
	{
		const FString Opt = CameraSpeedCombo->GetSelectedOption();
		if (Opt.StartsWith(TEXT("300"))) FlySpeed = 30000.0f;
		else if (Opt.StartsWith(TEXT("500"))) FlySpeed = 50000.0f;
		else if (Opt.StartsWith(TEXT("1000"))) FlySpeed = 100000.0f;
		else if (Opt.StartsWith(TEXT("2000"))) FlySpeed = 200000.0f;
		else if (Opt.StartsWith(TEXT("3500"))) FlySpeed = 350000.0f;
		else if (Opt.StartsWith(TEXT("5000"))) FlySpeed = 500000.0f;
	}

	float ShiftBoost = 4.0f;
	if (ShiftBoostCombo)
	{
		const FString Opt = ShiftBoostCombo->GetSelectedOption();
		if (Opt.StartsWith(TEXT("2.0"))) ShiftBoost = 2.0f;
		else if (Opt.StartsWith(TEXT("3.0"))) ShiftBoost = 3.0f;
		else if (Opt.StartsWith(TEXT("4.0"))) ShiftBoost = 4.0f;
		else if (Opt.StartsWith(TEXT("6.0"))) ShiftBoost = 6.0f;
		else if (Opt.StartsWith(TEXT("8.0"))) ShiftBoost = 8.0f;
		else if (Opt.StartsWith(TEXT("10.0"))) ShiftBoost = 10.0f;
	}

	float PathW = 4000.0f;
	if (PathWidthCombo)
	{
		const FString Opt = PathWidthCombo->GetSelectedOption();
		if (Opt.StartsWith(TEXT("20"))) PathW = 2000.0f;
		else if (Opt.StartsWith(TEXT("40"))) PathW = 4000.0f;
		else if (Opt.StartsWith(TEXT("60"))) PathW = 6000.0f;
		else if (Opt.StartsWith(TEXT("80"))) PathW = 8000.0f;
		else if (Opt.StartsWith(TEXT("100"))) PathW = 10000.0f;
	}

	float PathH = 8000.0f;
	if (PathHeightCombo)
	{
		const FString Opt = PathHeightCombo->GetSelectedOption();
		if (Opt.StartsWith(TEXT("20"))) PathH = 2000.0f;
		else if (Opt.StartsWith(TEXT("50"))) PathH = 5000.0f;
		else if (Opt.StartsWith(TEXT("80"))) PathH = 8000.0f;
		else if (Opt.StartsWith(TEXT("120"))) PathH = 12000.0f;
		else if (Opt.StartsWith(TEXT("200"))) PathH = 20000.0f;
	}

	float PtScale = 1.0f;
	if (EndpointScaleCombo)
	{
		const FString Opt = EndpointScaleCombo->GetSelectedOption();
		if (Opt.StartsWith(TEXT("50"))) PtScale = 0.5f;
		else if (Opt.StartsWith(TEXT("75"))) PtScale = 0.75f;
		else if (Opt.StartsWith(TEXT("100"))) PtScale = 1.0f;
		else if (Opt.StartsWith(TEXT("150"))) PtScale = 1.5f;
		else if (Opt.StartsWith(TEXT("200"))) PtScale = 2.0f;
	}

	URailSettingsManager::SetCameraFlySpeed(FlySpeed);
	URailSettingsManager::SetCameraShiftBoost(ShiftBoost);
	URailSettingsManager::SetPathRibbonWidth(PathW);
	URailSettingsManager::SetPathRibbonHeight(PathH);
	URailSettingsManager::SetEndpointScaleMultiplier(PtScale);

	if (ARailOptimaGameMode* GM = Cast<ARailOptimaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->ApplyVisualAndCameraSettings(FlySpeed, ShiftBoost, PathW, PathH, PtScale);
		GM->ShowToast(TEXT("Параметры камеры и отображения успешно применены"), ERailToastType::Success);
	}

	OnVisualSettingsApplied.Broadcast(FlySpeed, ShiftBoost, PathW, PathH, PtScale);
	HandleCloseVisualSettings();
}

void URailPauseMenuWidget::HandleCloseVisualSettings()
{
	if (VisualSettingsModalBorder)
	{
		VisualSettingsModalBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

