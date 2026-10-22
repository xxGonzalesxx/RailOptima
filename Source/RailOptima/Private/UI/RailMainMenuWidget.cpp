// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Реализация главного меню и дашборда проектов RailOptima

#include "UI/RailMainMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Services/RailProjectManager.h"
#include "Styling/CoreStyle.h"

namespace
{
	FButtonStyle MakeGlassButtonStyle(FLinearColor BaseColor, FLinearColor HoverColor, FLinearColor PressedColor)
	{
		FButtonStyle Style;
		Style.Normal.TintColor = FSlateColor(BaseColor);
		Style.Hovered.TintColor = FSlateColor(HoverColor);
		Style.Pressed.TintColor = FSlateColor(PressedColor);
		return Style;
	}
}

void URailProjectActionHelper::OnOpenClicked()
{
	if (MenuWidget.IsValid())
	{
		MenuWidget->OnProjectOpened.Broadcast(ProjectMeta);
	}
}

void URailProjectActionHelper::OnExploreClicked()
{
	URailProjectManager::OpenFolderInExplorer(ProjectMeta.ProjectPath);
}

void URailProjectActionHelper::OnDeleteClicked()
{
	if (MenuWidget.IsValid())
	{
		if (URailProjectManager::DeleteProject(ProjectMeta.ProjectPath))
		{
			MenuWidget->ShowToast(FString::Printf(TEXT("Проект '%s' удален."), *ProjectMeta.ProjectName), ERailToastType::Info);
			MenuWidget->RefreshProjectLibrary();
		}
		else
		{
			MenuWidget->ShowToast(TEXT("Не удалось удалить папку проекта."), ERailToastType::Error);
		}
	}
}

TSharedRef<SWidget> URailMainMenuWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MainMenuCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		// Фоновая подложка во весь экран
		UBorder* BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundBorder"));
		BackgroundBorder->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.045f, 0.96f));
		BackgroundBorder->SetPadding(FMargin(32.0f, 20.0f));

		UCanvasPanelSlot* BgSlot = RootCanvas->AddChildToCanvas(BackgroundBorder);
		BgSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BgSlot->SetOffsets(FMargin(0.0f));

		UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
		BackgroundBorder->SetContent(MainVBox);

		// Верхняя панель (шапка) и центральная область
		BuildHeader(MainVBox);
		BuildContentArea(MainVBox);

		// Модальные окна и оверлей
		BuildCreateProjectModal(RootCanvas);
		BuildAuthModal(RootCanvas);
		BuildLoadingOverlay(RootCanvas);

		// Всплывающие уведомления (тосты)
		ToastWidget = WidgetTree->ConstructWidget<URailToastWidget>(URailToastWidget::StaticClass(), TEXT("ToastWidget"));
		if (ToastWidget)
		{
			UCanvasPanelSlot* ToastSlot = RootCanvas->AddChildToCanvas(ToastWidget);
			ToastSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
			ToastSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			ToastSlot->SetPosition(FVector2D(0.0f, 20.0f));
			ToastSlot->SetAutoSize(true);
		}

		// Заполнение библиотеки проектов
		RefreshProjectLibrary();
	}

	return Super::RebuildWidget();
}

void URailMainMenuWidget::BuildHeader(UVerticalBox* MainVBox)
{
	UHorizontalBox* HeaderHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = MainVBox->AddChildToVerticalBox(HeaderHBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	// Логотип и подзаголовок
	UVerticalBox* LogoVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (UHorizontalBoxSlot* LSlot = HeaderHBox->AddChildToHorizontalBox(LogoVBox))
	{
		LSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UHorizontalBox* TitleHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	LogoVBox->AddChildToVerticalBox(TitleHBox);

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TitleText->SetText(FText::FromString(TEXT("RAILOPTIMA")));
	TitleText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 24));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	TitleHBox->AddChildToHorizontalBox(TitleText);

	UBorder* VersionBadge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	VersionBadge->SetBrushColor(FLinearColor(0.08f, 0.22f, 0.38f, 0.85f));
	VersionBadge->SetPadding(FMargin(8.0f, 4.0f));
	if (UHorizontalBoxSlot* VSlot = TitleHBox->AddChildToHorizontalBox(VersionBadge))
	{
		VSlot->SetPadding(FMargin(12.0f, 4.0f, 0.0f, 4.0f));
		VSlot->SetVerticalAlignment(VAlign_Center);
	}

	UTextBlock* VersionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	VersionText->SetText(FText::FromString(TEXT("v0.6.0")));
	VersionText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
	VersionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.85f, 1.0f, 1.0f)));
	VersionBadge->SetContent(VersionText);

	UTextBlock* SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SubtitleText->SetText(FText::FromString(TEXT("САПР железнодорожных путей • Модуль 3D-трассировки и оптимизации по СП 119.13330")));
	SubtitleText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	SubtitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.68f, 0.78f, 1.0f)));
	LogoVBox->AddChildToVerticalBox(SubtitleText);

	// Правая часть шапки: статус пользователя и облачная авторизация
	UHorizontalBox* ProfileHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UHorizontalBoxSlot* RSlot = HeaderHBox->AddChildToHorizontalBox(ProfileHBox))
	{
		RSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		RSlot->SetVerticalAlignment(VAlign_Center);
	}

	UBorder* ProfilePill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	ProfilePill->SetBrushColor(FLinearColor(0.04f, 0.08f, 0.14f, 0.85f));
	ProfilePill->SetPadding(FMargin(14.0f, 8.0f));
	ProfileHBox->AddChildToHorizontalBox(ProfilePill);

	UTextBlock* ProfileText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ProfileText->SetText(FText::FromString(TEXT("Инженер (Локальный профиль)")));
	ProfileText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 11));
	ProfileText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	ProfilePill->SetContent(ProfileText);

	USpacer* Spacer1 = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	Spacer1->SetSize(FVector2D(10.0f, 1.0f));
	ProfileHBox->AddChildToHorizontalBox(Spacer1);

	// Кнопка авторизации
	UButton* AuthBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	AuthBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.06f, 0.12f, 0.22f, 0.9f),
		FLinearColor(0.12f, 0.24f, 0.40f, 1.0f),
		FLinearColor(0.04f, 0.09f, 0.16f, 1.0f)));
	AuthBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleAuthButtonClicked);
	ProfileHBox->AddChildToHorizontalBox(AuthBtn);

	UBorder* AuthBtnInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	AuthBtnInner->SetBrushColor(FLinearColor::Transparent);
	AuthBtnInner->SetPadding(FMargin(14.0f, 8.0f));
	AuthBtn->AddChild(AuthBtnInner);

	UTextBlock* AuthBtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	AuthBtnText->SetText(FText::FromString(TEXT("Облако / Авторизация")));
	AuthBtnText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	AuthBtnText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	AuthBtnInner->SetContent(AuthBtnText);
}

void URailMainMenuWidget::BuildContentArea(UVerticalBox* MainVBox)
{
	UHorizontalBox* ContentHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = MainVBox->AddChildToVerticalBox(ContentHBox))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	// ─── Левая колонка (фиксированная ширина 360 px): Быстрые действия ───
	USizeBox* LeftSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	LeftSizeBox->SetWidthOverride(360.0f);
	ContentHBox->AddChildToHorizontalBox(LeftSizeBox);

	UBorder* ActionCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	ActionCard->SetBrushColor(FLinearColor(0.02f, 0.04f, 0.07f, 0.90f));
	ActionCard->SetPadding(FMargin(20.0f, 18.0f));
	LeftSizeBox->SetContent(ActionCard);

	UVerticalBox* ActionVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ActionCard->SetContent(ActionVBox);

	UTextBlock* ActionsTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ActionsTitle->SetText(FText::FromString(TEXT("БЫСТРЫЕ ДЕЙСТВИЯ")));
	ActionsTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
	ActionsTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	if (UVerticalBoxSlot* TSlot = ActionVBox->AddChildToVerticalBox(ActionsTitle))
	{
		TSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	// Кнопка создания нового проекта
	UButton* NewProjectBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	NewProjectBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.05f, 0.22f, 0.38f, 0.95f),
		FLinearColor(0.08f, 0.35f, 0.58f, 1.0f),
		FLinearColor(0.03f, 0.16f, 0.28f, 1.0f)));
	NewProjectBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleNewProjectClicked);
	if (UVerticalBoxSlot* BtnSlot = ActionVBox->AddChildToVerticalBox(NewProjectBtn))
	{
		BtnSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	UBorder* NewProjectInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	NewProjectInner->SetBrushColor(FLinearColor::Transparent);
	NewProjectInner->SetPadding(FMargin(18.0f, 12.0f));
	NewProjectBtn->AddChild(NewProjectInner);

	UTextBlock* NewProjectText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NewProjectText->SetText(FText::FromString(TEXT("+  СОЗДАТЬ НОВЫЙ ПРОЕКТ")));
	NewProjectText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
	NewProjectText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	NewProjectInner->SetContent(NewProjectText);

	// Кнопка открытия папки проектов
	UButton* FolderBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	FolderBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.05f, 0.08f, 0.14f, 0.85f),
		FLinearColor(0.10f, 0.16f, 0.26f, 0.95f),
		FLinearColor(0.03f, 0.05f, 0.09f, 1.0f)));
	FolderBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleOpenProjectsFolderClicked);
	if (UVerticalBoxSlot* BtnSlot = ActionVBox->AddChildToVerticalBox(FolderBtn))
	{
		BtnSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	UBorder* FolderInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	FolderInner->SetBrushColor(FLinearColor::Transparent);
	FolderInner->SetPadding(FMargin(16.0f, 10.0f));
	FolderBtn->AddChild(FolderInner);

	UTextBlock* FolderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FolderText->SetText(FText::FromString(TEXT("Открыть папку проектов")));
	FolderText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 11));
	FolderText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f)));
	FolderInner->SetContent(FolderText);

	// Кнопка настроек графики и производительности
	UButton* SettingsBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	SettingsBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.10f, 0.18f, 0.32f, 0.90f),
		FLinearColor(0.16f, 0.28f, 0.48f, 1.0f),
		FLinearColor(0.06f, 0.12f, 0.22f, 1.0f)));
	SettingsBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleGraphicsSettingsClicked);
	if (UVerticalBoxSlot* BtnSlot = ActionVBox->AddChildToVerticalBox(SettingsBtn))
	{
		BtnSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	UBorder* SettingsInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	SettingsInner->SetBrushColor(FLinearColor::Transparent);
	SettingsInner->SetPadding(FMargin(16.0f, 10.0f));
	SettingsBtn->AddChild(SettingsInner);

	UTextBlock* SettingsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SettingsText->SetText(FText::FromString(TEXT("⚙  Настройки графики")));
	SettingsText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	SettingsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.94f, 1.0f, 1.0f)));
	SettingsInner->SetContent(SettingsText);

	// Карточка интеграции с OpenTopography
	UBorder* OpenTopoCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	OpenTopoCard->SetBrushColor(FLinearColor(0.03f, 0.07f, 0.12f, 0.75f));
	OpenTopoCard->SetPadding(FMargin(14.0f, 12.0f));
	ActionVBox->AddChildToVerticalBox(OpenTopoCard);

	UVerticalBox* OpenTopoVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	OpenTopoCard->SetContent(OpenTopoVBox);

	UTextBlock* OpenTopoTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OpenTopoTitle->SetText(FText::FromString(TEXT("Веб-сервис и OpenTopography")));
	OpenTopoTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	OpenTopoTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	if (UVerticalBoxSlot* SubSlot = OpenTopoVBox->AddChildToVerticalBox(OpenTopoTitle))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	UTextBlock* OpenTopoDesc = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OpenTopoDesc->SetText(FText::FromString(TEXT("В веб-версии интегрирован выбор зоны рельефа через спутниковый сервис OpenTopography. Зона выбирается прямо на интерактивной карте и синхронизируется с проектом.")));
	OpenTopoDesc->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	OpenTopoDesc->SetAutoWrapText(true);
	OpenTopoDesc->SetWrapTextAt(310.0f);
	OpenTopoDesc->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.72f, 0.82f, 1.0f)));
	OpenTopoVBox->AddChildToVerticalBox(OpenTopoDesc);

	// ─── Разделитель между колонками ───
	USpacer* ColSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	ColSpacer->SetSize(FVector2D(24.0f, 1.0f));
	ContentHBox->AddChildToHorizontalBox(ColSpacer);

	// ─── Правая колонка: Библиотека локальных проектов (Fill) ───
	UBorder* LibraryCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	LibraryCard->SetBrushColor(FLinearColor(0.02f, 0.04f, 0.07f, 0.90f));
	LibraryCard->SetPadding(FMargin(22.0f, 18.0f));
	if (UHorizontalBoxSlot* RSlot = ContentHBox->AddChildToHorizontalBox(LibraryCard))
	{
		RSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UVerticalBox* LibVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	LibraryCard->SetContent(LibVBox);

	// Заголовок библиотеки
	UHorizontalBox* LibHeaderHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = LibVBox->AddChildToVerticalBox(LibHeaderHBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	UTextBlock* LibTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LibTitle->SetText(FText::FromString(TEXT("БИБЛИОТЕКА ПРОЕКТОВ")));
	LibTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 13));
	LibTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	if (UHorizontalBoxSlot* SubSlot = LibHeaderHBox->AddChildToHorizontalBox(LibTitle))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SubSlot->SetVerticalAlignment(VAlign_Center);
	}

	ProjectCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ProjectCountText->SetText(FText::FromString(TEXT("Всего: 0")));
	ProjectCountText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	ProjectCountText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.62f, 0.72f, 1.0f)));
	if (UHorizontalBoxSlot* SubSlot = LibHeaderHBox->AddChildToHorizontalBox(ProjectCountText))
	{
		SubSlot->SetVerticalAlignment(VAlign_Center);
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
	}

	// Кнопка обновить
	UButton* RefreshBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	RefreshBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.06f, 0.12f, 0.20f, 0.85f),
		FLinearColor(0.12f, 0.22f, 0.35f, 0.95f),
		FLinearColor(0.04f, 0.08f, 0.15f, 1.0f)));
	RefreshBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleRefreshLibraryClicked);
	LibHeaderHBox->AddChildToHorizontalBox(RefreshBtn);

	UBorder* RefInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	RefInner->SetBrushColor(FLinearColor::Transparent);
	RefInner->SetPadding(FMargin(10.0f, 6.0f));
	RefreshBtn->AddChild(RefInner);

	UTextBlock* RefText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	RefText->SetText(FText::FromString(TEXT("Обновить")));
	RefText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 10));
	RefText->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.82f, 0.92f, 1.0f)));
	RefInner->SetContent(RefText);

	// Прокручиваемый список проектов
	ProjectListScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	ProjectListScrollBox->SetOrientation(EOrientation::Orient_Vertical);
	ProjectListScrollBox->SetScrollBarVisibility(ESlateVisibility::Visible);
	if (UVerticalBoxSlot* SubSlot = LibVBox->AddChildToVerticalBox(ProjectListScrollBox))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
}

void URailMainMenuWidget::PopulateProjectCard(const FRailProjectMetadata& Meta, UScrollBox* TargetScrollBox)
{
	if (!TargetScrollBox)
		return;

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Card->SetBrushColor(FLinearColor(0.03f, 0.06f, 0.10f, 0.90f));
	Card->SetPadding(FMargin(16.0f, 12.0f));
	if (UScrollBoxSlot* SubSlot = Cast<UScrollBoxSlot>(TargetScrollBox->AddChild(Card)))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	UHorizontalBox* CardHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Card->SetContent(CardHBox);

	// Левая часть карточки: описание проекта
	UVerticalBox* InfoVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (UHorizontalBoxSlot* SubSlot = CardHBox->AddChildToHorizontalBox(InfoVBox))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SubSlot->SetVerticalAlignment(VAlign_Center);
	}

	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameText->SetText(FText::FromString(Meta.ProjectName));
	NameText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 13));
	NameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.96f, 0.98f, 1.0f)));
	InfoVBox->AddChildToVerticalBox(NameText);

	UHorizontalBox* MetaHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = InfoVBox->AddChildToVerticalBox(MetaHBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
	}

	UTextBlock* CatText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CatText->SetText(FText::FromString(Meta.CategoryName));
	CatText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 10));
	CatText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.85f, 0.7f, 1.0f)));
	if (UHorizontalBoxSlot* SubSlot = MetaHBox->AddChildToHorizontalBox(CatText))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
	}

	UTextBlock* DateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	DateText->SetText(FText::FromString(Meta.CreatedAt.ToString(TEXT("%d.%m.%Y %H:%M"))));
	DateText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	DateText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.62f, 0.72f, 1.0f)));
	if (UHorizontalBoxSlot* SubSlot = MetaHBox->AddChildToHorizontalBox(DateText))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
	}

	if (Meta.bHasRoads)
	{
		UTextBlock* RoadsBadge = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		RoadsBadge->SetText(FText::FromString(TEXT("Дороги OSM")));
		RoadsBadge->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 9));
		RoadsBadge->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.75f, 1.0f, 0.9f)));
		if (UHorizontalBoxSlot* SubSlot = MetaHBox->AddChildToHorizontalBox(RoadsBadge))
		{
			SubSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		}
	}

	if (Meta.bHasRivers)
	{
		UTextBlock* RiversBadge = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		RiversBadge->SetText(FText::FromString(TEXT("Реки OSM")));
		RiversBadge->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 9));
		RiversBadge->SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.85f, 0.85f, 0.9f)));
		MetaHBox->AddChildToHorizontalBox(RiversBadge);
	}

	// Правая часть карточки: кнопки действий
	UHorizontalBox* ActionsHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UHorizontalBoxSlot* SubSlot = CardHBox->AddChildToHorizontalBox(ActionsHBox))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		SubSlot->SetVerticalAlignment(VAlign_Center);
	}

	URailProjectActionHelper* ActionHelper = NewObject<URailProjectActionHelper>(this);
	ActionHelper->ProjectMeta = Meta;
	ActionHelper->MenuWidget = this;
	ActionHelpers.Add(ActionHelper);

	// Кнопка ОТКРЫТЬ
	UButton* OpenBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	OpenBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.08f, 0.32f, 0.22f, 0.95f),
		FLinearColor(0.12f, 0.48f, 0.32f, 1.0f),
		FLinearColor(0.05f, 0.22f, 0.15f, 1.0f)));
	OpenBtn->OnClicked.AddDynamic(ActionHelper, &URailProjectActionHelper::OnOpenClicked);
	ActionsHBox->AddChildToHorizontalBox(OpenBtn);

	UBorder* OpenInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	OpenInner->SetBrushColor(FLinearColor::Transparent);
	OpenInner->SetPadding(FMargin(16.0f, 8.0f));
	OpenBtn->AddChild(OpenInner);

	UTextBlock* OpenText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OpenText->SetText(FText::FromString(TEXT("ОТКРЫТЬ")));
	OpenText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	OpenText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.95f, 0.4f, 1.0f)));
	OpenInner->SetContent(OpenText);

	USpacer* BtnSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	BtnSpacer->SetSize(FVector2D(8.0f, 1.0f));
	ActionsHBox->AddChildToHorizontalBox(BtnSpacer);

	// Кнопка В ПАПКЕ
	UButton* ExploreBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	ExploreBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.05f, 0.08f, 0.14f, 0.85f),
		FLinearColor(0.10f, 0.16f, 0.26f, 0.95f),
		FLinearColor(0.03f, 0.05f, 0.09f, 1.0f)));
	ExploreBtn->OnClicked.AddDynamic(ActionHelper, &URailProjectActionHelper::OnExploreClicked);
	ActionsHBox->AddChildToHorizontalBox(ExploreBtn);

	UBorder* ExpInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	ExpInner->SetBrushColor(FLinearColor::Transparent);
	ExpInner->SetPadding(FMargin(10.0f, 8.0f));
	ExploreBtn->AddChild(ExpInner);

	UTextBlock* ExpText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ExpText->SetText(FText::FromString(TEXT("Папка")));
	ExpText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	ExpInner->SetContent(ExpText);

	USpacer* BtnSpacer2 = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	BtnSpacer2->SetSize(FVector2D(8.0f, 1.0f));
	ActionsHBox->AddChildToHorizontalBox(BtnSpacer2);

	// Кнопка УДАЛИТЬ
	UButton* DelBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	DelBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.18f, 0.05f, 0.05f, 0.85f),
		FLinearColor(0.32f, 0.08f, 0.08f, 0.95f),
		FLinearColor(0.12f, 0.03f, 0.03f, 1.0f)));
	DelBtn->OnClicked.AddDynamic(ActionHelper, &URailProjectActionHelper::OnDeleteClicked);
	ActionsHBox->AddChildToHorizontalBox(DelBtn);

	UBorder* DelInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	DelInner->SetBrushColor(FLinearColor::Transparent);
	DelInner->SetPadding(FMargin(10.0f, 8.0f));
	DelBtn->AddChild(DelInner);

	UTextBlock* DelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	DelText->SetText(FText::FromString(TEXT("Удалить")));
	DelText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
	DelText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.4f, 0.4f, 1.0f)));
	DelInner->SetContent(DelText);
}

void URailMainMenuWidget::RefreshProjectLibrary()
{
	ActionHelpers.Empty();

	if (!ProjectListScrollBox)
		return;

	ProjectListScrollBox->ClearChildren();
	TArray<FRailProjectMetadata> Projects = URailProjectManager::GetAllProjects();

	if (ProjectCountText)
	{
		ProjectCountText->SetText(FText::FromString(FString::Printf(TEXT("Всего: %d"), Projects.Num())));
	}

	if (Projects.Num() == 0)
	{
		UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		EmptyText->SetText(FText::FromString(TEXT("В библиотеке пока нет проектов. Нажмите '+ СОЗДАТЬ НОВЫЙ ПРОЕКТ' для начала.")));
		EmptyText->SetFont(FCoreStyle::GetDefaultFontStyle("Italic", 12));
		EmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.62f, 0.72f, 1.0f)));
		ProjectListScrollBox->AddChild(EmptyText);
		return;
	}

	for (const FRailProjectMetadata& Meta : Projects)
	{
		PopulateProjectCard(Meta, ProjectListScrollBox);
	}
}

void URailMainMenuWidget::BuildCreateProjectModal(UCanvasPanel* RootCanvas)
{
	CreateProjectModalBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CreateProjectModalBorder"));
	CreateProjectModalBorder->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.03f, 0.85f));
	CreateProjectModalBorder->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* ModalSlot = RootCanvas->AddChildToCanvas(CreateProjectModalBorder);
	ModalSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	ModalSlot->SetOffsets(FMargin(0.0f));

	// Карточка по центру экрана с ограничением ширины и высоты (адаптивность под любой экран)
	USizeBox* ModalSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ModalSizeBox->SetWidthOverride(540.0f);
	ModalSizeBox->SetMaxDesiredHeight(680.0f);

	UBorder* DialogCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	DialogCard->SetBrushColor(FLinearColor(0.02f, 0.035f, 0.065f, 0.98f));
	DialogCard->SetPadding(FMargin(28.0f, 20.0f));
	ModalSizeBox->SetContent(DialogCard);

	// Центрирование через CanvasPanel внутри полноэкранного затемнения
	UCanvasPanel* ModalCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	CreateProjectModalBorder->SetContent(ModalCanvas);

	UCanvasPanelSlot* CardSlot = ModalCanvas->AddChildToCanvas(ModalSizeBox);
	CardSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
	CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CardSlot->SetPosition(FVector2D(0.0f, 0.0f));
	CardSlot->SetAutoSize(true);

	UScrollBox* ModalScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	DialogCard->SetContent(ModalScroll);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ModalScroll->AddChild(VBox);

	// Заголовок
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("СОЗДАНИЕ НОВОГО ПРОЕКТА")));
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(Title))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	// Поле 1: Название проекта
	UTextBlock* NameLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameLabel->SetText(FText::FromString(TEXT("Название проекта:")));
	NameLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	NameLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	VBox->AddChildToVerticalBox(NameLabel);

	ProjectNameInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	ProjectNameInput->SetText(FText::FromString(TEXT("Участок трассы 1")));
	ProjectNameInput->SetHintText(FText::FromString(TEXT("Введите название проекта...")));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(ProjectNameInput))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 12.0f));
	}

	// Поле 2: Категория дороги
	UTextBlock* CatLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CatLabel->SetText(FText::FromString(TEXT("Категория железнодорожной линии (СП 119.13330):")));
	CatLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	CatLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	VBox->AddChildToVerticalBox(CatLabel);

	CategoryComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());

	// Стилизация выпадающего списка: темный фон, белый читаемый текст пунктов
	FTableRowStyle ItemStyle = CategoryComboBox->GetItemStyle();
	ItemStyle.TextColor = FSlateColor(FLinearColor(0.92f, 0.95f, 0.98f, 1.0f));
	ItemStyle.SelectedTextColor = FSlateColor(FLinearColor::White);
	ItemStyle.EvenRowBackgroundBrush.TintColor = FSlateColor(FLinearColor(0.04f, 0.07f, 0.12f, 0.98f));
	ItemStyle.OddRowBackgroundBrush.TintColor = FSlateColor(FLinearColor(0.03f, 0.05f, 0.09f, 0.98f));
	ItemStyle.ActiveBrush.TintColor = FSlateColor(FLinearColor(0.08f, 0.38f, 0.68f, 1.0f));
	ItemStyle.ActiveHoveredBrush.TintColor = FSlateColor(FLinearColor(0.12f, 0.48f, 0.82f, 1.0f));
	ItemStyle.EvenRowBackgroundHoveredBrush.TintColor = FSlateColor(FLinearColor(0.08f, 0.16f, 0.28f, 1.0f));
	ItemStyle.OddRowBackgroundHoveredBrush.TintColor = FSlateColor(FLinearColor(0.08f, 0.16f, 0.28f, 1.0f));
	CategoryComboBox->SetItemStyle(ItemStyle);

	FComboBoxStyle WidgetStyle = CategoryComboBox->GetWidgetStyle();
	WidgetStyle.ComboButtonStyle.ButtonStyle.Normal.TintColor = FSlateColor(FLinearColor(0.04f, 0.08f, 0.15f, 0.95f));
	WidgetStyle.ComboButtonStyle.ButtonStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.07f, 0.14f, 0.25f, 1.0f));
	WidgetStyle.ComboButtonStyle.ButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.02f, 0.05f, 0.10f, 1.0f));
	WidgetStyle.ComboButtonStyle.MenuBorderBrush.TintColor = FSlateColor(FLinearColor(0.02f, 0.04f, 0.08f, 0.98f));
	WidgetStyle.ComboButtonStyle.DownArrowImage.TintColor = FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f));
	CategoryComboBox->SetWidgetStyle(WidgetStyle);

	CategoryComboBox->OnGenerateWidgetEvent.BindDynamic(this, &URailMainMenuWidget::HandleGenerateCategoryWidget);

	CategoryComboBox->AddOption(TEXT("I Категория"));
	CategoryComboBox->AddOption(TEXT("II Категория"));
	CategoryComboBox->AddOption(TEXT("III Категория"));
	CategoryComboBox->AddOption(TEXT("IV (Железнодорожные линии)"));
	CategoryComboBox->AddOption(TEXT("IV (Подъездные пути)"));
	CategoryComboBox->AddOption(TEXT("Скоростные"));
	CategoryComboBox->AddOption(TEXT("Пассажирские"));
	CategoryComboBox->AddOption(TEXT("Особогрузонапряженные"));
	CategoryComboBox->SetSelectedOption(TEXT("I Категория"));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(CategoryComboBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 12.0f));
	}

	// Поле 3: Тип источника рельефа
	UTextBlock* TypeLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TypeLabel->SetText(FText::FromString(TEXT("Формат карты высот:")));
	TypeLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	TypeLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	VBox->AddChildToVerticalBox(TypeLabel);

	UHorizontalBox* TypeHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(TypeHBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 10.0f));
	}

	PngTypeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	PngTypeButton->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.08f, 0.28f, 0.45f, 0.95f),
		FLinearColor(0.12f, 0.38f, 0.60f, 1.0f),
		FLinearColor(0.05f, 0.20f, 0.35f, 1.0f)));
	PngTypeButton->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleSourceTypeToggledPng);
	TypeHBox->AddChildToHorizontalBox(PngTypeButton);

	PngTypeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	PngTypeText->SetText(FText::FromString(TEXT("●  16-бит PNG (готовый)")));
	PngTypeText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	PngTypeText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	UBorder* PngInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	PngInner->SetBrushColor(FLinearColor::Transparent);
	PngInner->SetPadding(FMargin(14.0f, 8.0f));
	PngInner->SetContent(PngTypeText);
	PngTypeButton->AddChild(PngInner);

	USpacer* TypeSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	TypeSpacer->SetSize(FVector2D(12.0f, 1.0f));
	TypeHBox->AddChildToHorizontalBox(TypeSpacer);

	TifTypeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	TifTypeButton->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.04f, 0.07f, 0.12f, 0.85f),
		FLinearColor(0.08f, 0.15f, 0.25f, 0.95f),
		FLinearColor(0.02f, 0.05f, 0.09f, 1.0f)));
	TifTypeButton->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleSourceTypeToggledTif);
	TypeHBox->AddChildToHorizontalBox(TifTypeButton);

	TifTypeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TifTypeText->SetText(FText::FromString(TEXT("○  GeoTIFF DEM (.tif)")));
	TifTypeText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	TifTypeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.72f, 0.82f, 1.0f)));
	UBorder* TifInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	TifInner->SetBrushColor(FLinearColor::Transparent);
	TifInner->SetPadding(FMargin(14.0f, 8.0f));
	TifInner->SetContent(TifTypeText);
	TifTypeButton->AddChild(TifInner);

	// Поле 4: Путь к файлу
	UTextBlock* FileLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FileLabel->SetText(FText::FromString(TEXT("Путь к исходному файлу рельефа:")));
	FileLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	FileLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	VBox->AddChildToVerticalBox(FileLabel);

	UHorizontalBox* FileHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(FileHBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 12.0f));
	}

	FilePathInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	FilePathInput->SetHintText(FText::FromString(TEXT("Выберите файл карты высот (.png или .tif)...")));
	if (UHorizontalBoxSlot* SubSlot = FileHBox->AddChildToHorizontalBox(FilePathInput))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	USpacer* FileSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	FileSpacer->SetSize(FVector2D(8.0f, 1.0f));
	FileHBox->AddChildToHorizontalBox(FileSpacer);

	UButton* BrowseBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	BrowseBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.06f, 0.14f, 0.24f, 0.9f),
		FLinearColor(0.10f, 0.24f, 0.40f, 1.0f),
		FLinearColor(0.04f, 0.10f, 0.18f, 1.0f)));
	BrowseBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleBrowseFileClicked);
	FileHBox->AddChildToHorizontalBox(BrowseBtn);

	UBorder* BrowseInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	BrowseInner->SetBrushColor(FLinearColor::Transparent);
	BrowseInner->SetPadding(FMargin(14.0f, 8.0f));
	BrowseBtn->AddChild(BrowseInner);

	UTextBlock* BrowseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BrowseText->SetText(FText::FromString(TEXT("Обзор...")));
	BrowseText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	BrowseText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	BrowseInner->SetContent(BrowseText);

	// Поле 5: Плашки загрузки векторных слоев
	UHorizontalBox* CheckHBox1 = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	RoadsCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
	RoadsCheckBox->SetIsChecked(true);
	CheckHBox1->AddChildToHorizontalBox(RoadsCheckBox);

	UTextBlock* RoadsLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	RoadsLabel->SetText(FText::FromString(TEXT("  Загрузить дороги (автотрассы, развязки)")));
	RoadsLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	RoadsLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	CheckHBox1->AddChildToHorizontalBox(RoadsLabel);
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(CheckHBox1))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 4.0f));
	}

	UHorizontalBox* CheckHBox2 = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	RiversCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
	RiversCheckBox->SetIsChecked(true);
	CheckHBox2->AddChildToHorizontalBox(RiversCheckBox);

	UTextBlock* RiversLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	RiversLabel->SetText(FText::FromString(TEXT("  Загрузить реки (гидрографию)")));
	RiversLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	RiversLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	CheckHBox2->AddChildToHorizontalBox(RiversLabel);
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(CheckHBox2))
	{
		SubSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 18.0f));
	}

	// Кнопки подтверждения и отмены
	UHorizontalBox* BtnHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	VBox->AddChildToVerticalBox(BtnHBox);

	UButton* ConfirmBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	ConfirmBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.06f, 0.32f, 0.20f, 0.95f),
		FLinearColor(0.10f, 0.48f, 0.30f, 1.0f),
		FLinearColor(0.04f, 0.22f, 0.14f, 1.0f)));
	ConfirmBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleCreateProjectConfirmClicked);
	if (UHorizontalBoxSlot* SubSlot = BtnHBox->AddChildToHorizontalBox(ConfirmBtn))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UBorder* ConfInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	ConfInner->SetBrushColor(FLinearColor::Transparent);
	ConfInner->SetPadding(FMargin(16.0f, 10.0f));
	ConfirmBtn->AddChild(ConfInner);

	UTextBlock* ConfText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ConfText->SetText(FText::FromString(TEXT("СОЗДАТЬ И ОТКРЫТЬ")));
	ConfText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
	ConfText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ConfText->SetJustification(ETextJustify::Center);
	ConfInner->SetContent(ConfText);

	USpacer* ModalBtnSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	ModalBtnSpacer->SetSize(FVector2D(12.0f, 1.0f));
	BtnHBox->AddChildToHorizontalBox(ModalBtnSpacer);

	UButton* CancelBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CancelBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.06f, 0.08f, 0.12f, 0.85f),
		FLinearColor(0.12f, 0.16f, 0.22f, 0.95f),
		FLinearColor(0.03f, 0.04f, 0.08f, 1.0f)));
	CancelBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleCreateProjectCancelClicked);
	if (UHorizontalBoxSlot* SubSlot = BtnHBox->AddChildToHorizontalBox(CancelBtn))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UBorder* CancInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	CancInner->SetBrushColor(FLinearColor::Transparent);
	CancInner->SetPadding(FMargin(16.0f, 10.0f));
	CancelBtn->AddChild(CancInner);

	UTextBlock* CancText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CancText->SetText(FText::FromString(TEXT("ОТМЕНА")));
	CancText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
	CancText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.75f, 0.85f, 1.0f)));
	CancText->SetJustification(ETextJustify::Center);
	CancInner->SetContent(CancText);
}

void URailMainMenuWidget::BuildAuthModal(UCanvasPanel* RootCanvas)
{
	AuthModalBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AuthModalBorder"));
	AuthModalBorder->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.03f, 0.85f));
	AuthModalBorder->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* ModalSlot = RootCanvas->AddChildToCanvas(AuthModalBorder);
	ModalSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	ModalSlot->SetOffsets(FMargin(0.0f));

	USizeBox* ModalSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ModalSizeBox->SetWidthOverride(460.0f);

	UBorder* DialogCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	DialogCard->SetBrushColor(FLinearColor(0.02f, 0.035f, 0.065f, 0.98f));
	DialogCard->SetPadding(FMargin(28.0f, 22.0f));
	ModalSizeBox->SetContent(DialogCard);

	UCanvasPanel* ModalCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	AuthModalBorder->SetContent(ModalCanvas);

	UCanvasPanelSlot* CardSlot = ModalCanvas->AddChildToCanvas(ModalSizeBox);
	CardSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
	CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CardSlot->SetPosition(FVector2D(0.0f, 0.0f));
	CardSlot->SetAutoSize(true);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	DialogCard->SetContent(VBox);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("ОБЛАЧНАЯ АВТОРИЗАЦИЯ")));
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 15));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(Title))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UBorder* InfoCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	InfoCard->SetBrushColor(FLinearColor(0.03f, 0.06f, 0.10f, 0.8f));
	InfoCard->SetPadding(FMargin(12.0f, 10.0f));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(InfoCard))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	UTextBlock* InfoText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	InfoText->SetText(FText::FromString(TEXT("Авторизуйтесь для синхронизации проектов с облаком и быстрого импорта зон рельефа OpenTopography из личного кабинета.")));
	InfoText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	InfoText->SetAutoWrapText(true);
	InfoText->SetWrapTextAt(400.0f);
	InfoText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.78f, 0.88f, 1.0f)));
	InfoCard->SetContent(InfoText);

	UTextBlock* EmailLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	EmailLabel->SetText(FText::FromString(TEXT("Email пользователя:")));
	EmailLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	EmailLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	VBox->AddChildToVerticalBox(EmailLabel);

	AuthEmailInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	AuthEmailInput->SetText(FText::FromString(TEXT("engineer@railoptima.ru")));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(AuthEmailInput))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 12.0f));
	}

	UTextBlock* PassLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	PassLabel->SetText(FText::FromString(TEXT("Пароль:")));
	PassLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	PassLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	VBox->AddChildToVerticalBox(PassLabel);

	AuthPassInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	AuthPassInput->SetIsPassword(true);
	AuthPassInput->SetText(FText::FromString(TEXT("••••••••••••")));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(AuthPassInput))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 16.0f));
	}

	UHorizontalBox* BtnHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	VBox->AddChildToVerticalBox(BtnHBox);

	UButton* LoginBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	LoginBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.06f, 0.28f, 0.45f, 0.95f),
		FLinearColor(0.10f, 0.38f, 0.60f, 1.0f),
		FLinearColor(0.04f, 0.20f, 0.35f, 1.0f)));
	LoginBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleAuthSubmitClicked);
	if (UHorizontalBoxSlot* SubSlot = BtnHBox->AddChildToHorizontalBox(LoginBtn))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UBorder* LogInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	LogInner->SetBrushColor(FLinearColor::Transparent);
	LogInner->SetPadding(FMargin(16.0f, 10.0f));
	LoginBtn->AddChild(LogInner);

	UTextBlock* LogText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LogText->SetText(FText::FromString(TEXT("ВОЙТИ В ОБЛАКО")));
	LogText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
	LogText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	LogText->SetJustification(ETextJustify::Center);
	LogInner->SetContent(LogText);

	USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	Spacer->SetSize(FVector2D(12.0f, 1.0f));
	BtnHBox->AddChildToHorizontalBox(Spacer);

	UButton* CloseBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CloseBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.06f, 0.08f, 0.12f, 0.85f),
		FLinearColor(0.12f, 0.16f, 0.22f, 0.95f),
		FLinearColor(0.03f, 0.04f, 0.08f, 1.0f)));
	CloseBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleAuthCloseClicked);
	if (UHorizontalBoxSlot* SubSlot = BtnHBox->AddChildToHorizontalBox(CloseBtn))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UBorder* CloseInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	CloseInner->SetBrushColor(FLinearColor::Transparent);
	CloseInner->SetPadding(FMargin(16.0f, 10.0f));
	CloseBtn->AddChild(CloseInner);

	UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CloseText->SetText(FText::FromString(TEXT("ЗАКРЫТЬ")));
	CloseText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
	CloseText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.75f, 0.85f, 1.0f)));
	CloseText->SetJustification(ETextJustify::Center);
	CloseInner->SetContent(CloseText);
}

void URailMainMenuWidget::BuildLoadingOverlay(UCanvasPanel* RootCanvas)
{
	LoadingOverlayBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LoadingOverlayBorder"));
	LoadingOverlayBorder->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.03f, 0.88f));
	LoadingOverlayBorder->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* LoadSlot = RootCanvas->AddChildToCanvas(LoadingOverlayBorder);
	LoadSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	LoadSlot->SetOffsets(FMargin(0.0f));

	USizeBox* CenterSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CenterSizeBox->SetWidthOverride(420.0f);

	UBorder* CenterCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	CenterCard->SetBrushColor(FLinearColor(0.02f, 0.035f, 0.065f, 0.98f));
	CenterCard->SetPadding(FMargin(32.0f, 24.0f));
	CenterSizeBox->SetContent(CenterCard);

	UCanvasPanel* SubPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	LoadingOverlayBorder->SetContent(SubPanel);

	UCanvasPanelSlot* CenterSlot = SubPanel->AddChildToCanvas(CenterSizeBox);
	CenterSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
	CenterSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CenterSlot->SetPosition(FVector2D(0.0f, 0.0f));
	CenterSlot->SetAutoSize(true);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	CenterCard->SetContent(VBox);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("Загрузка проекта...")));
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	Title->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(Title))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	LoadingStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LoadingStatusText->SetText(FText::FromString(TEXT("Подготовка рельефа и генерация геометрии...")));
	LoadingStatusText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	LoadingStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.78f, 0.88f, 1.0f)));
	LoadingStatusText->SetJustification(ETextJustify::Center);
	LoadingStatusText->SetAutoWrapText(true);
	VBox->AddChildToVerticalBox(LoadingStatusText);
}

void URailMainMenuWidget::ShowLoadingOverlay(const FString& InStatusText)
{
	if (LoadingStatusText)
	{
		LoadingStatusText->SetText(FText::FromString(InStatusText));
	}
	if (LoadingOverlayBorder)
	{
		LoadingOverlayBorder->SetVisibility(ESlateVisibility::Visible);
	}
}

void URailMainMenuWidget::HideLoadingOverlay()
{
	if (LoadingOverlayBorder)
	{
		LoadingOverlayBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool URailMainMenuWidget::CloseOpenModals()
{
	bool bClosedAny = false;
	if (CreateProjectModalBorder && CreateProjectModalBorder->GetVisibility() == ESlateVisibility::Visible)
	{
		CreateProjectModalBorder->SetVisibility(ESlateVisibility::Collapsed);
		bClosedAny = true;
	}
	if (AuthModalBorder && AuthModalBorder->GetVisibility() == ESlateVisibility::Visible)
	{
		AuthModalBorder->SetVisibility(ESlateVisibility::Collapsed);
		bClosedAny = true;
	}
	return bClosedAny;
}

void URailMainMenuWidget::ShowToast(const FString& Message, ERailToastType Type, float DurationSec)
{
	if (ToastWidget)
	{
		ToastWidget->ShowToast(Message, Type, DurationSec);
	}
}

void URailMainMenuWidget::HandleNewProjectClicked()
{
	if (CreateProjectModalBorder)
	{
		CreateProjectModalBorder->SetVisibility(ESlateVisibility::Visible);
	}
}

void URailMainMenuWidget::HandleOpenProjectsFolderClicked()
{
	FString RootDir = URailProjectManager::GetProjectsRootDirectory();
	URailProjectManager::OpenFolderInExplorer(RootDir);
}

void URailMainMenuWidget::HandleAuthButtonClicked()
{
	if (AuthModalBorder)
	{
		AuthModalBorder->SetVisibility(ESlateVisibility::Visible);
	}
}

void URailMainMenuWidget::HandleRefreshLibraryClicked()
{
	RefreshProjectLibrary();
	ShowToast(TEXT("Список локальных проектов обновлен"), ERailToastType::Info, 2.5f);
}

void URailMainMenuWidget::HandleGraphicsSettingsClicked()
{
	OnGraphicsSettingsRequested.Broadcast();
}

void URailMainMenuWidget::HandleSourceTypeToggledPng()
{
	bSourceIsGeoTiff = false;
	if (PngTypeText)
	{
		PngTypeText->SetText(FText::FromString(TEXT("●  16-бит PNG (готовый)")));
		PngTypeText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
	if (TifTypeText)
	{
		TifTypeText->SetText(FText::FromString(TEXT("○  GeoTIFF DEM (.tif)")));
		TifTypeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.72f, 0.82f, 1.0f)));
	}
}

void URailMainMenuWidget::HandleSourceTypeToggledTif()
{
	bSourceIsGeoTiff = true;
	if (PngTypeText)
	{
		PngTypeText->SetText(FText::FromString(TEXT("○  16-бит PNG (готовый)")));
		PngTypeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.72f, 0.82f, 1.0f)));
	}
	if (TifTypeText)
	{
		TifTypeText->SetText(FText::FromString(TEXT("●  GeoTIFF DEM (.tif)")));
		TifTypeText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
}

void URailMainMenuWidget::HandleBrowseFileClicked()
{
	FString SelectedFile;
	bool bSuccess = false;

	if (bSourceIsGeoTiff)
	{
		bSuccess = URailProjectManager::OpenFileDialogWindows(
			TEXT("Выберите файл цифровой модели рельефа GeoTIFF"),
			TEXT("Файлы GeoTIFF (*.tif;*.tiff) *.tif;*.tiff Все файлы (*.*) *.* "),
			SelectedFile);
	}
	else
	{
		bSuccess = URailProjectManager::OpenFileDialogWindows(
			TEXT("Выберите 16-битную карту высот PNG"),
			TEXT("Файлы PNG (*.png) *.png Все файлы (*.*) *.* "),
			SelectedFile);
	}

	if (bSuccess && !SelectedFile.IsEmpty())
	{
		if (FilePathInput)
		{
			FilePathInput->SetText(FText::FromString(SelectedFile));
		}
	}
}

void URailMainMenuWidget::HandleCreateProjectConfirmClicked()
{
	const FString ProjectName = ProjectNameInput ? ProjectNameInput->GetText().ToString().TrimStartAndEnd() : TEXT("");
	if (ProjectName.IsEmpty())
	{
		ShowToast(TEXT("Пожалуйста, укажите название проекта!"), ERailToastType::Warning);
		return;
	}

	const FString SourcePath = FilePathInput ? FilePathInput->GetText().ToString().TrimStartAndEnd() : TEXT("");
	if (SourcePath.IsEmpty() || !FPaths::FileExists(SourcePath))
	{
		ShowToast(TEXT("Указанный файл рельефа не существует на диске!"), ERailToastType::Error);
		return;
	}

	const FString SelectedCatStr = CategoryComboBox ? CategoryComboBox->GetSelectedOption() : TEXT("I Категория");
	ERailRoadCategory Cat = ERailRoadCategory::Category1;
	if (SelectedCatStr.Contains(TEXT("Скоростные")))
	{
		Cat = ERailRoadCategory::HighSpeed;
	}
	else if (SelectedCatStr.Contains(TEXT("Пассажирские")))
	{
		Cat = ERailRoadCategory::Passenger;
	}
	else if (SelectedCatStr.Contains(TEXT("Особогрузонапряженные")))
	{
		Cat = ERailRoadCategory::SuperHeavyHaul;
	}
	else if (SelectedCatStr.Contains(TEXT("II Категория")))
	{
		Cat = ERailRoadCategory::Category2;
	}
	else if (SelectedCatStr.Contains(TEXT("III Категория")))
	{
		Cat = ERailRoadCategory::Category3;
	}
	else if (SelectedCatStr.Contains(TEXT("подъездные")))
	{
		Cat = ERailRoadCategory::Category4Access;
	}
	else if (SelectedCatStr.Contains(TEXT("линии")))
	{
		Cat = ERailRoadCategory::Category4Lines;
	}

	const bool bRoads = RoadsCheckBox ? RoadsCheckBox->IsChecked() : false;
	const bool bRivers = RiversCheckBox ? RiversCheckBox->IsChecked() : false;

	ShowLoadingOverlay(TEXT("Обработка файлов рельефа и слоев проекта..."));

	FRailProjectMetadata CreatedProject;
	FString ErrorMsg;
	bool bOk = false;

	if (bSourceIsGeoTiff)
	{
		bOk = URailProjectManager::CreateProjectFromGeoTiff(
			ProjectName, Cat, SourcePath, bRoads, bRivers, CreatedProject, ErrorMsg);
	}
	else
	{
		bOk = URailProjectManager::CreateProjectFromPng(
			ProjectName, Cat, SourcePath, bRoads, bRivers, CreatedProject, ErrorMsg);
	}

	HideLoadingOverlay();

	if (bOk)
	{
		if (CreateProjectModalBorder)
		{
			CreateProjectModalBorder->SetVisibility(ESlateVisibility::Collapsed);
		}
		RefreshProjectLibrary();
		ShowToast(FString::Printf(TEXT("Проект '%s' успешно создан!"), *ProjectName), ERailToastType::Success);
		OnProjectOpened.Broadcast(CreatedProject);
	}
	else
	{
		ShowToast(FString::Printf(TEXT("Ошибка создания проекта: %s"), *ErrorMsg), ERailToastType::Error, 6.0f);
	}
}

void URailMainMenuWidget::HandleCreateProjectCancelClicked()
{
	if (CreateProjectModalBorder)
	{
		CreateProjectModalBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URailMainMenuWidget::HandleAuthSubmitClicked()
{
	if (AuthModalBorder)
	{
		AuthModalBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
	ShowToast(TEXT("Авторизация успешна! Облачные проекты синхронизированы."), ERailToastType::Success);
}

void URailMainMenuWidget::HandleAuthCloseClicked()
{
	if (AuthModalBorder)
	{
		AuthModalBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UWidget* URailMainMenuWidget::HandleGenerateCategoryWidget(FString Item)
{
	UTextBlock* ItemText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ItemText->SetText(FText::FromString(Item));
	ItemText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 11));
	ItemText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.95f, 0.98f, 1.0f)));
	return ItemText;
}
