// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Виджет настроек графики и масштабируемости RailOptima

#include "UI/RailGraphicsSettingsWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Services/RailSettingsManager.h"
#include "Styling/CoreStyle.h"

namespace
{
	FButtonStyle MakeSettingsButtonStyle(FLinearColor BaseColor, FLinearColor HoverColor, FLinearColor PressedColor)
	{
		FButtonStyle Style;
		Style.Normal.TintColor = FSlateColor(BaseColor);
		Style.Hovered.TintColor = FSlateColor(HoverColor);
		Style.Pressed.TintColor = FSlateColor(PressedColor);
		return Style;
	}

	void ApplySettingsComboBoxStyle(UComboBoxString* Combo)
	{
		if (!Combo) return;
		FComboBoxStyle WidgetStyle = Combo->GetWidgetStyle();
		WidgetStyle.ComboButtonStyle.ButtonStyle.Normal.TintColor = FSlateColor(FLinearColor(0.04f, 0.08f, 0.15f, 0.95f));
		WidgetStyle.ComboButtonStyle.ButtonStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.07f, 0.14f, 0.25f, 1.0f));
		WidgetStyle.ComboButtonStyle.ButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.02f, 0.05f, 0.10f, 1.0f));
		WidgetStyle.ComboButtonStyle.MenuBorderBrush.TintColor = FSlateColor(FLinearColor(0.02f, 0.04f, 0.08f, 0.98f));
		WidgetStyle.ComboButtonStyle.DownArrowImage.TintColor = FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f));
		Combo->SetWidgetStyle(WidgetStyle);
	}
}

TSharedRef<SWidget> URailGraphicsSettingsWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SettingsRootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		// Полупрозрачный фон
		RootDimmerBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsDimmer"));
		RootDimmerBorder->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.03f, 0.82f));
		UCanvasPanelSlot* DimmerSlot = RootCanvas->AddChildToCanvas(RootDimmerBorder);
		DimmerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		DimmerSlot->SetOffsets(FMargin(0.0f));

		// Центрированная карточка с ограничением размера
		USizeBox* ModalSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ModalSizeBox"));
		ModalSizeBox->SetWidthOverride(560.0f);
		ModalSizeBox->SetMaxDesiredHeight(680.0f);

		UBorder* ModalBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModalCardBorder"));
		ModalBorder->SetBrushColor(FLinearColor(0.02f, 0.035f, 0.065f, 0.98f));
		ModalBorder->SetPadding(FMargin(26.0f, 22.0f));
		ModalSizeBox->SetContent(ModalBorder);

		UCanvasPanelSlot* CardSlot = RootCanvas->AddChildToCanvas(ModalSizeBox);
		CardSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CardSlot->SetPosition(FVector2D(0.0f, 0.0f));
		CardSlot->SetAutoSize(true);

		// Вертикальный контейнер карточки
		UVerticalBox* CardVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ModalCardVBox"));
		ModalBorder->SetContent(CardVBox);

		// Заголовок
		UTextBlock* HeaderTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		HeaderTitle->SetText(FText::FromString(TEXT("НАСТРОЙКИ ГРАФИКИ И ПРОИЗВОДИТЕЛЬНОСТИ")));
		HeaderTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
		HeaderTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
		CardVBox->AddChildToVerticalBox(HeaderTitle);

		UTextBlock* HeaderSub = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		HeaderSub->SetText(FText::FromString(TEXT("Масштабирование для плавного рендеринга на любом железе")));
		HeaderSub->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
		HeaderSub->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.68f, 0.78f, 0.9f)));
		if (UVerticalBoxSlot* SubSlot = CardVBox->AddChildToVerticalBox(HeaderSub))
		{
			SubSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 14.0f));
		}

		// Скролл-бокс для опций, чтобы помещалось на экранах 720p/1366x768
		UScrollBox* ContentScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ContentScrollBox"));
		if (UVerticalBoxSlot* ScrollSlot = CardVBox->AddChildToVerticalBox(ContentScrollBox))
		{
			ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UVerticalBox* OptionsVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		ContentScrollBox->AddChild(OptionsVBox);

		// Быстрые пресеты качества
		UTextBlock* PresetsLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		PresetsLabel->SetText(FText::FromString(TEXT("БЫСТРЫЕ ПРЕСЕТЫ:")));
		PresetsLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
		PresetsLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.82f, 0.9f, 1.0f)));
		OptionsVBox->AddChildToVerticalBox(PresetsLabel);

		UHorizontalBox* PresetsHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (UVerticalBoxSlot* PresetsSlot = OptionsVBox->AddChildToVerticalBox(PresetsHBox))
		{
			PresetsSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 12.0f));
		}

		UButton* BtnLow = CreatePresetButton(TEXT("Низкое (Слабый ПК)"), FLinearColor(0.12f, 0.42f, 0.35f, 0.95f));
		if (BtnLow)
		{
			BtnLow->OnClicked.AddDynamic(this, &URailGraphicsSettingsWidget::HandlePresetLowClicked);
			if (UHorizontalBoxSlot* HSlot = PresetsHBox->AddChildToHorizontalBox(BtnLow))
			{
				HSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
				HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}

		UButton* BtnMed = CreatePresetButton(TEXT("Среднее"), FLinearColor(0.15f, 0.35f, 0.65f, 0.95f));
		if (BtnMed)
		{
			BtnMed->OnClicked.AddDynamic(this, &URailGraphicsSettingsWidget::HandlePresetMediumClicked);
			if (UHorizontalBoxSlot* HSlot = PresetsHBox->AddChildToHorizontalBox(BtnMed))
			{
				HSlot->SetPadding(FMargin(2.0f, 0.0f, 2.0f, 0.0f));
				HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}

		UButton* BtnHigh = CreatePresetButton(TEXT("Высокое"), FLinearColor(0.35f, 0.25f, 0.65f, 0.95f));
		if (BtnHigh)
		{
			BtnHigh->OnClicked.AddDynamic(this, &URailGraphicsSettingsWidget::HandlePresetHighClicked);
			if (UHorizontalBoxSlot* HSlot = PresetsHBox->AddChildToHorizontalBox(BtnHigh))
			{
				HSlot->SetPadding(FMargin(2.0f, 0.0f, 2.0f, 0.0f));
				HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}

		UButton* BtnUltra = CreatePresetButton(TEXT("Ультра"), FLinearColor(0.55f, 0.2f, 0.5f, 0.95f));
		if (BtnUltra)
		{
			BtnUltra->OnClicked.AddDynamic(this, &URailGraphicsSettingsWidget::HandlePresetUltraClicked);
			if (UHorizontalBoxSlot* HSlot = PresetsHBox->AddChildToHorizontalBox(BtnUltra))
			{
				HSlot->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));
				HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}

		// Кнопка автоопределения железа
		UButton* BtnAuto = CreatePresetButton(TEXT("⚡ Автоопределение оборудования"), FLinearColor(0.18f, 0.48f, 0.78f, 0.95f));
		if (BtnAuto)
		{
			BtnAuto->OnClicked.AddDynamic(this, &URailGraphicsSettingsWidget::HandleAutoBenchmarkClicked);
			if (UVerticalBoxSlot* AutoSlot = OptionsVBox->AddChildToVerticalBox(BtnAuto))
			{
				AutoSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
			}
		}

		// Функция-помощник для создания комбобоксов с меткой
		auto AddSettingCombo = [&](const FString& LabelText, const FString& HintText, const TArray<FString>& Options) -> UComboBoxString* {
			UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Lbl->SetText(FText::FromString(LabelText));
			Lbl->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
			Lbl->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.92f, 1.0f)));
			OptionsVBox->AddChildToVerticalBox(Lbl);

			if (!HintText.IsEmpty())
			{
				UTextBlock* Sub = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
				Sub->SetText(FText::FromString(HintText));
				Sub->SetFont(FCoreStyle::GetDefaultFontStyle("Italic", 9));
				Sub->SetColorAndOpacity(FSlateColor(FLinearColor(0.52f, 0.58f, 0.68f, 0.85f)));
				if (UVerticalBoxSlot* HintSlot = OptionsVBox->AddChildToVerticalBox(Sub))
				{
					HintSlot->SetPadding(FMargin(0.0f, 1.0f, 0.0f, 3.0f));
				}
			}

			UComboBoxString* Combo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
			ApplySettingsComboBoxStyle(Combo);
			for (const FString& Opt : Options)
			{
				Combo->AddOption(Opt);
			}
			Combo->OnGenerateWidgetEvent.BindDynamic(this, &URailGraphicsSettingsWidget::HandleGenerateComboWidget);

			if (UVerticalBoxSlot* CSlot = OptionsVBox->AddChildToVerticalBox(Combo))
			{
				CSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 10.0f));
			}
			return Combo;
		};

		// 1. Масштаб разрешения 3D
		ResolutionComboBox = AddSettingCombo(
			TEXT("Масштаб 3D-разрешения:"),
			TEXT("Понижение масштаба до 65-75% даёт максимальный прирост FPS на встроенных видеокартах"),
			{ TEXT("50% (Макс. FPS)"), TEXT("65% (Слабый ПК)"), TEXT("75% (Баланс)"), TEXT("85% (Высокое)"), TEXT("100% (Нативное)") }
		);

		// 2. Лимит частоты кадров
		FpsLimitComboBox = AddSettingCombo(
			TEXT("Ограничение частоты кадров (FPS):"),
			TEXT("Лимит 60 или 30 FPS предотвращает перегрев процессора и троттлинг на ноутбуках"),
			{ TEXT("30 FPS (Ноутбук / Энергосбережение)"), TEXT("60 FPS (Рекомендуется)"), TEXT("120 FPS"), TEXT("Без ограничений") }
		);

		// 3. Тени
		ShadowComboBox = AddSettingCombo(
			TEXT("Качество теней:"),
			TEXT("Низкие тени отключают ресурсоёмкие виртуальные карты теней (Virtual Shadow Maps)"),
			{ TEXT("Низкое / Выкл (Макс. FPS)"), TEXT("Среднее"), TEXT("Высокое"), TEXT("Ультра") }
		);

		// 4. Сглаживание
		AntiAliasingComboBox = AddSettingCombo(
			TEXT("Сглаживание (Anti-Aliasing):"),
			TEXT("Низкое качество использует быстрый FXAA вместо тяжёлого темпорального апскейлинга"),
			{ TEXT("Низкое (FXAA)"), TEXT("Среднее"), TEXT("Высокое"), TEXT("Ультра") }
		);

		// 5. Дальность видимости
		ViewDistanceComboBox = AddSettingCombo(
			TEXT("Дальность прорисовки:"),
			TEXT("Регулирует радиус отображения дорог и рек вокруг камеры"),
			{ TEXT("Низкая"), TEXT("Средняя"), TEXT("Высокая"), TEXT("Ультра") }
		);

		// 6. VSync
		VSyncCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
		UHorizontalBox* VSyncBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		VSyncBox->AddChildToHorizontalBox(VSyncCheckBox);

		UTextBlock* VSyncText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		VSyncText->SetText(FText::FromString(TEXT(" Вертикальная синхронизация (V-Sync)")));
		VSyncText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
		VSyncText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.92f, 1.0f)));
		VSyncBox->AddChildToHorizontalBox(VSyncText);

		if (UVerticalBoxSlot* VSSlot = OptionsVBox->AddChildToVerticalBox(VSyncBox))
		{
			VSSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 16.0f));
		}

		// Нижние кнопки управления (Применить / Закрыть)
		UHorizontalBox* ActionsHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (UVerticalBoxSlot* ActSlot = CardVBox->AddChildToVerticalBox(ActionsHBox))
		{
			ActSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
		}

		UButton* ApplyBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		ApplyBtn->SetStyle(MakeSettingsButtonStyle(
			FLinearColor(0.0f, 0.45f, 0.75f, 0.95f),
			FLinearColor(0.1f, 0.55f, 0.85f, 1.0f),
			FLinearColor(0.0f, 0.35f, 0.65f, 1.0f)));
		ApplyBtn->OnClicked.AddDynamic(this, &URailGraphicsSettingsWidget::HandleApplyClicked);

		UTextBlock* ApplyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ApplyText->SetText(FText::FromString(TEXT("Применить и сохранить")));
		ApplyText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
		ApplyText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		ApplyText->SetJustification(ETextJustify::Center);
		ApplyBtn->SetContent(ApplyText);

		if (UHorizontalBoxSlot* HSlot = ActionsHBox->AddChildToHorizontalBox(ApplyBtn))
		{
			HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			HSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		}

		UButton* CloseBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		CloseBtn->SetStyle(MakeSettingsButtonStyle(
			FLinearColor(0.18f, 0.22f, 0.28f, 0.9f),
			FLinearColor(0.25f, 0.30f, 0.38f, 1.0f),
			FLinearColor(0.12f, 0.15f, 0.20f, 1.0f)));
		CloseBtn->OnClicked.AddDynamic(this, &URailGraphicsSettingsWidget::HandleCloseClicked);

		UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		CloseText->SetText(FText::FromString(TEXT("Закрыть")));
		CloseText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
		CloseText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.92f, 1.0f)));
		CloseText->SetJustification(ETextJustify::Center);
		CloseBtn->SetContent(CloseText);

		if (UHorizontalBoxSlot* HSlot = ActionsHBox->AddChildToHorizontalBox(CloseBtn))
		{
			HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	SetVisibility(ESlateVisibility::Collapsed);
	return Super::RebuildWidget();
}

UButton* URailGraphicsSettingsWidget::CreatePresetButton(const FString& Text, const FLinearColor& Color)
{
	UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Btn->SetStyle(MakeSettingsButtonStyle(Color, Color * 1.2f, Color * 0.8f));

	UTextBlock* BtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BtnText->SetText(FText::FromString(Text));
	BtnText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 9));
	BtnText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	BtnText->SetJustification(ETextJustify::Center);
	Btn->SetContent(BtnText);

	return Btn;
}

UWidget* URailGraphicsSettingsWidget::HandleGenerateComboWidget(FString Item)
{
	UTextBlock* ItemText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ItemText->SetText(FText::FromString(Item));
	ItemText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	ItemText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.95f, 0.98f, 1.0f)));
	return ItemText;
}

void URailGraphicsSettingsWidget::OpenSettingsModal()
{
	SyncUiFromSettings();
	SetVisibility(ESlateVisibility::Visible);
}

void URailGraphicsSettingsWidget::CloseSettingsModal()
{
	SetVisibility(ESlateVisibility::Collapsed);
	OnSettingsClosed.Broadcast();
}

void URailGraphicsSettingsWidget::SyncUiFromSettings()
{
	// 1. Resolution
	const float Res = URailSettingsManager::GetResolutionScale();
	if (ResolutionComboBox)
	{
		if (Res <= 55.0f) ResolutionComboBox->SetSelectedIndex(0);
		else if (Res <= 68.0f) ResolutionComboBox->SetSelectedIndex(1);
		else if (Res <= 78.0f) ResolutionComboBox->SetSelectedIndex(2);
		else if (Res <= 90.0f) ResolutionComboBox->SetSelectedIndex(3);
		else ResolutionComboBox->SetSelectedIndex(4);
	}

	// 2. FPS Limit
	const float Fps = URailSettingsManager::GetFrameRateLimit();
	if (FpsLimitComboBox)
	{
		if (FMath::IsNearlyEqual(Fps, 30.0f, 1.0f)) FpsLimitComboBox->SetSelectedIndex(0);
		else if (FMath::IsNearlyEqual(Fps, 60.0f, 1.0f)) FpsLimitComboBox->SetSelectedIndex(1);
		else if (FMath::IsNearlyEqual(Fps, 120.0f, 1.0f)) FpsLimitComboBox->SetSelectedIndex(2);
		else FpsLimitComboBox->SetSelectedIndex(3);
	}

	// 3. Shadow
	if (ShadowComboBox)
	{
		ShadowComboBox->SetSelectedIndex(FMath::Clamp(URailSettingsManager::GetShadowQuality(), 0, 3));
	}

	// 4. AA
	if (AntiAliasingComboBox)
	{
		AntiAliasingComboBox->SetSelectedIndex(FMath::Clamp(URailSettingsManager::GetAntiAliasingQuality(), 0, 3));
	}

	// 5. View Distance
	if (ViewDistanceComboBox)
	{
		ViewDistanceComboBox->SetSelectedIndex(FMath::Clamp(URailSettingsManager::GetViewDistanceQuality(), 0, 3));
	}

	// 6. VSync
	if (VSyncCheckBox)
	{
		VSyncCheckBox->SetIsChecked(URailSettingsManager::IsVSyncEnabled());
	}
}

void URailGraphicsSettingsWidget::HandlePresetLowClicked()
{
	URailSettingsManager::ApplyQualityPreset(0);
	SyncUiFromSettings();
}

void URailGraphicsSettingsWidget::HandlePresetMediumClicked()
{
	URailSettingsManager::ApplyQualityPreset(1);
	SyncUiFromSettings();
}

void URailGraphicsSettingsWidget::HandlePresetHighClicked()
{
	URailSettingsManager::ApplyQualityPreset(2);
	SyncUiFromSettings();
}

void URailGraphicsSettingsWidget::HandlePresetUltraClicked()
{
	URailSettingsManager::ApplyQualityPreset(3);
	SyncUiFromSettings();
}

void URailGraphicsSettingsWidget::HandleAutoBenchmarkClicked()
{
	URailSettingsManager::RunAutoBenchmark();
	SyncUiFromSettings();
}

void URailGraphicsSettingsWidget::HandleApplyClicked()
{
	// 1. Resolution
	if (ResolutionComboBox)
	{
		const int32 Idx = ResolutionComboBox->GetSelectedIndex();
		const float Scales[] = { 50.0f, 65.0f, 75.0f, 85.0f, 100.0f };
		if (Idx >= 0 && Idx < 5)
		{
			URailSettingsManager::SetResolutionScale(Scales[Idx]);
		}
	}

	// 2. FPS Limit
	if (FpsLimitComboBox)
	{
		const int32 Idx = FpsLimitComboBox->GetSelectedIndex();
		const float Limits[] = { 30.0f, 60.0f, 120.0f, 0.0f };
		if (Idx >= 0 && Idx < 4)
		{
			URailSettingsManager::SetFrameRateLimit(Limits[Idx]);
		}
	}

	// 3. Shadow
	if (ShadowComboBox)
	{
		const int32 Idx = ShadowComboBox->GetSelectedIndex();
		if (Idx >= 0)
		{
			URailSettingsManager::SetShadowQuality(Idx);
			URailSettingsManager::ApplyLowEndOptimizations(Idx == 0);
		}
	}

	// 4. AA
	if (AntiAliasingComboBox)
	{
		const int32 Idx = AntiAliasingComboBox->GetSelectedIndex();
		if (Idx >= 0)
		{
			URailSettingsManager::SetAntiAliasingQuality(Idx);
		}
	}

	// 5. View Distance
	if (ViewDistanceComboBox)
	{
		const int32 Idx = ViewDistanceComboBox->GetSelectedIndex();
		if (Idx >= 0)
		{
			URailSettingsManager::SetViewDistanceQuality(Idx);
		}
	}

	// 6. VSync
	if (VSyncCheckBox)
	{
		URailSettingsManager::SetVSyncEnabled(VSyncCheckBox->IsChecked());
	}

	URailSettingsManager::SaveSettings();
	OnSettingsApplied.Broadcast();
	CloseSettingsModal();
}

void URailGraphicsSettingsWidget::HandleCloseClicked()
{
	CloseSettingsModal();
}
