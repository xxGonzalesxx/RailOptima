// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Реализация главного меню и дашборда проектов RailOptima с поддержкой Backend API и облачной синхронизации

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
#include "Services/RailApiService.h"
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

// ─────────────────────────────────────────────────────────────────
//  Вспомогательные классы действий
// ─────────────────────────────────────────────────────────────────

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

void URailProjectActionHelper::OnSyncToCloudClicked()
{
	if (MenuWidget.IsValid())
	{
		URailApiService* Api = URailApiService::Get(MenuWidget.Get());
		if (!Api || !Api->IsAuthenticated())
		{
			MenuWidget->ShowToast(TEXT("Для синхронизации необходимо войти в облако"), ERailToastType::Warning);
			MenuWidget->HandleAuthButtonClicked();
			return;
		}

		MenuWidget->ShowToast(FString::Printf(TEXT("Отправка проекта '%s' в облако..."), *ProjectMeta.ProjectName), ERailToastType::Info);
		Api->SyncLocalProjectToCloud(ProjectMeta);
	}
}

void URailCloudProjectActionHelper::OnCreateLocalCloneClicked()
{
	if (MenuWidget.IsValid())
	{
		MenuWidget->ShowToast(FString::Printf(TEXT("Облачный проект '%s' выбран. Настройте карту высот для 3D-трассировки."), *CloudProject.Name), ERailToastType::Info);
		MenuWidget->HandleNewProjectClicked();
	}
}

void URailCloudProjectActionHelper::OnDeleteFromCloudClicked()
{
	if (MenuWidget.IsValid())
	{
		URailApiService* Api = URailApiService::Get(MenuWidget.Get());
		if (Api)
		{
			MenuWidget->ShowToast(FString::Printf(TEXT("Удаление проекта '%s' из облака..."), *CloudProject.Name), ERailToastType::Info);
			Api->DeleteRemoteProject(CloudProject.Id);
		}
	}
}

// ─────────────────────────────────────────────────────────────────
//  Построение UI
// ─────────────────────────────────────────────────────────────────

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
		BuildCreateCloudProjectModal(RootCanvas);
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

		// Подписка на события API
		if (URailApiService* Api = URailApiService::Get(this))
		{
			Api->OnAuthCompleted.RemoveDynamic(this, &URailMainMenuWidget::HandleApiAuthCompleted);
			Api->OnAuthCompleted.AddDynamic(this, &URailMainMenuWidget::HandleApiAuthCompleted);

			Api->OnProjectsListReceived.RemoveDynamic(this, &URailMainMenuWidget::HandleApiProjectsReceived);
			Api->OnProjectsListReceived.AddDynamic(this, &URailMainMenuWidget::HandleApiProjectsReceived);

			Api->OnProjectActionCompleted.RemoveDynamic(this, &URailMainMenuWidget::HandleApiProjectActionCompleted);
			Api->OnProjectActionCompleted.AddDynamic(this, &URailMainMenuWidget::HandleApiProjectActionCompleted);

			Api->OnProjectDeleted.RemoveDynamic(this, &URailMainMenuWidget::HandleApiProjectDeleted);
			Api->OnProjectDeleted.AddDynamic(this, &URailMainMenuWidget::HandleApiProjectDeleted);
		}

		UpdateHeaderProfileUI();
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
	VersionText->SetText(FText::FromString(TEXT("v0.6.0 • Cloud API")));
	VersionText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
	VersionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.85f, 1.0f, 1.0f)));
	VersionBadge->SetContent(VersionText);

	UTextBlock* SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SubtitleText->SetText(FText::FromString(TEXT("САПР железнодорожных путей • Модуль 3D-трассировки и оптимизации по СП 119.13330 • Backend REST API")));
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

	ProfileStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ProfileStatusText->SetText(FText::FromString(TEXT("Инженер (Локальный профиль)")));
	ProfileStatusText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 11));
	ProfileStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	ProfilePill->SetContent(ProfileStatusText);

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

	AuthButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	AuthButtonText->SetText(FText::FromString(TEXT("Облако / Вход")));
	AuthButtonText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	AuthButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	AuthBtnInner->SetContent(AuthButtonText);
}

void URailMainMenuWidget::UpdateHeaderProfileUI()
{
	URailApiService* Api = URailApiService::Get(this);
	if (Api && Api->IsAuthenticated())
	{
		if (ProfileStatusText)
		{
			ProfileStatusText->SetText(FText::FromString(FString::Printf(TEXT("👤 %s (Облако: %s)"), *Api->GetAuthData().Email, *Api->GetAuthData().Role)));
			ProfileStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.9f, 0.6f, 1.0f)));
		}
		if (AuthButtonText)
		{
			AuthButtonText->SetText(FText::FromString(TEXT("Кабинет / Выйти")));
		}
	}
	else
	{
		if (ProfileStatusText)
		{
			ProfileStatusText->SetText(FText::FromString(TEXT("Инженер (Локальный профиль)")));
			ProfileStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
		}
		if (AuthButtonText)
		{
			AuthButtonText->SetText(FText::FromString(TEXT("Облако / Вход")));
		}
	}
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
		BtnSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
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

	// Карточка интеграции с Backend и OpenTopography
	UBorder* OpenTopoCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	OpenTopoCard->SetBrushColor(FLinearColor(0.03f, 0.07f, 0.12f, 0.75f));
	OpenTopoCard->SetPadding(FMargin(14.0f, 12.0f));
	ActionVBox->AddChildToVerticalBox(OpenTopoCard);

	UVerticalBox* OpenTopoVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	OpenTopoCard->SetContent(OpenTopoVBox);

	UTextBlock* OpenTopoTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OpenTopoTitle->SetText(FText::FromString(TEXT("Синхронизация с экосистемой")));
	OpenTopoTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	OpenTopoTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	if (UVerticalBoxSlot* SubSlot = OpenTopoVBox->AddChildToVerticalBox(OpenTopoTitle))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	UTextBlock* OpenTopoDesc = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OpenTopoDesc->SetText(FText::FromString(TEXT("Клиент подключен к единому Backend API. Проекты синхронизируются между Web Platform и 3D Desktop клиентом.")));
	OpenTopoDesc->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	OpenTopoDesc->SetAutoWrapText(true);
	OpenTopoDesc->SetWrapTextAt(310.0f);
	OpenTopoDesc->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.72f, 0.82f, 1.0f)));
	OpenTopoVBox->AddChildToVerticalBox(OpenTopoDesc);

	// ─── Разделитель между колонками ───
	USpacer* ColSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	ColSpacer->SetSize(FVector2D(24.0f, 1.0f));
	ContentHBox->AddChildToHorizontalBox(ColSpacer);

	// ─── Правая колонка: Библиотека проектов (Fill) ───
	UBorder* LibraryCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	LibraryCard->SetBrushColor(FLinearColor(0.02f, 0.04f, 0.07f, 0.90f));
	LibraryCard->SetPadding(FMargin(22.0f, 18.0f));
	if (UHorizontalBoxSlot* RSlot = ContentHBox->AddChildToHorizontalBox(LibraryCard))
	{
		RSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UVerticalBox* LibVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	LibraryCard->SetContent(LibVBox);

	// Вкладки переключения: Локальные / Облачные проекты
	UHorizontalBox* TabsHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = LibVBox->AddChildToVerticalBox(TabsHBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	TabLocalBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	TabLocalBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.06f, 0.24f, 0.42f, 0.95f), FLinearColor(0.1f, 0.34f, 0.58f, 1.0f), FLinearColor(0.04f, 0.18f, 0.32f, 1.0f)));
	TabLocalBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleTabLocalClicked);
	if (UHorizontalBoxSlot* HSlot = TabsHBox->AddChildToHorizontalBox(TabLocalBtn))
	{
		HSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
	}

	UBorder* LocalTabInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	LocalTabInner->SetBrushColor(FLinearColor::Transparent);
	LocalTabInner->SetPadding(FMargin(14.0f, 8.0f));
	TabLocalBtn->AddChild(LocalTabInner);

	TabLocalText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TabLocalText->SetText(FText::FromString(TEXT("📁 Локальные проекты")));
	TabLocalText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	TabLocalText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	LocalTabInner->SetContent(TabLocalText);

	TabCloudBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	TabCloudBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.04f, 0.08f, 0.14f, 0.85f), FLinearColor(0.08f, 0.15f, 0.24f, 0.95f), FLinearColor(0.03f, 0.06f, 0.10f, 1.0f)));
	TabCloudBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleTabCloudClicked);
	TabsHBox->AddChildToHorizontalBox(TabCloudBtn);

	UBorder* CloudTabInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	CloudTabInner->SetBrushColor(FLinearColor::Transparent);
	CloudTabInner->SetPadding(FMargin(14.0f, 8.0f));
	TabCloudBtn->AddChild(CloudTabInner);

	TabCloudText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TabCloudText->SetText(FText::FromString(TEXT("☁ Облачные проекты (API)")));
	TabCloudText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	TabCloudText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.75f, 0.85f, 1.0f)));
	CloudTabInner->SetContent(TabCloudText);

	USpacer* TabSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	if (UHorizontalBoxSlot* HSlot = TabsHBox->AddChildToHorizontalBox(TabSpacer))
	{
		HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	ProjectCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ProjectCountText->SetText(FText::FromString(TEXT("Всего: 0")));
	ProjectCountText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	ProjectCountText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.62f, 0.72f, 1.0f)));
	if (UHorizontalBoxSlot* SubSlot = TabsHBox->AddChildToHorizontalBox(ProjectCountText))
	{
		SubSlot->SetVerticalAlignment(VAlign_Center);
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
	}

	// Кнопка создать в облаке (для вкладки Cloud)
	UButton* CreateCloudBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CreateCloudBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.08f, 0.28f, 0.42f, 0.9f), FLinearColor(0.12f, 0.38f, 0.55f, 1.0f), FLinearColor(0.05f, 0.20f, 0.32f, 1.0f)));
	CreateCloudBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleCreateCloudProjectClicked);
	TabsHBox->AddChildToHorizontalBox(CreateCloudBtn);

	UBorder* CloudBtnInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	CloudBtnInner->SetBrushColor(FLinearColor::Transparent);
	CloudBtnInner->SetPadding(FMargin(10.0f, 6.0f));
	CreateCloudBtn->AddChild(CloudBtnInner);

	UTextBlock* CloudBtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CloudBtnText->SetText(FText::FromString(TEXT("+ В облако")));
	CloudBtnText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 10));
	CloudBtnText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.95f, 1.0f, 1.0f)));
	CloudBtnInner->SetContent(CloudBtnText);

	USpacer* BtnSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	BtnSpacer->SetSize(FVector2D(6.0f, 1.0f));
	TabsHBox->AddChildToHorizontalBox(BtnSpacer);

	// Кнопка обновить
	UButton* RefreshBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	RefreshBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.06f, 0.12f, 0.20f, 0.85f),
		FLinearColor(0.12f, 0.22f, 0.35f, 0.95f),
		FLinearColor(0.04f, 0.08f, 0.15f, 1.0f)));
	RefreshBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleRefreshLibraryClicked);
	TabsHBox->AddChildToHorizontalBox(RefreshBtn);

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
	if (UVerticalBoxSlot* SubSlot = LibVBox->AddChildToVerticalBox(ProjectListScrollBox))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
}

void URailMainMenuWidget::PopulateProjectCard(const FRailProjectMetadata& Meta, UScrollBox* TargetScrollBox)
{
	if (!TargetScrollBox)
		return;

	UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	CardBorder->SetBrushColor(FLinearColor(0.025f, 0.045f, 0.075f, 0.85f));
	CardBorder->SetPadding(FMargin(16.0f, 14.0f));

	if (UScrollBoxSlot* CardSlot = Cast<UScrollBoxSlot>(TargetScrollBox->AddChild(CardBorder)))
	{
		CardSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	UHorizontalBox* CardHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	CardBorder->SetContent(CardHBox);

	// Левая колонка карточки: Текстовые данные
	UVerticalBox* InfoVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (UHorizontalBoxSlot* SubSlot = CardHBox->AddChildToHorizontalBox(InfoVBox))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SubSlot->SetVerticalAlignment(VAlign_Center);
	}

	UHorizontalBox* TitleLineHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	InfoVBox->AddChildToVerticalBox(TitleLineHBox);

	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameText->SetText(FText::FromString(Meta.ProjectName));
	NameText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 13));
	NameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.96f, 0.98f, 1.0f)));
	TitleLineHBox->AddChildToHorizontalBox(NameText);

	// Бейдж категории
	UBorder* CatBadge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	CatBadge->SetBrushColor(FLinearColor(0.08f, 0.24f, 0.42f, 0.85f));
	CatBadge->SetPadding(FMargin(8.0f, 2.0f));
	if (UHorizontalBoxSlot* SubSlot = TitleLineHBox->AddChildToHorizontalBox(CatBadge))
	{
		SubSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
		SubSlot->SetVerticalAlignment(VAlign_Center);
	}

	UTextBlock* CatBadgeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CatBadgeText->SetText(FText::FromString(Meta.CategoryName));
	CatBadgeText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 9));
	CatBadgeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.85f, 1.0f, 1.0f)));
	CatBadge->SetContent(CatBadgeText);

	// Статистика (протяженность, стоимость)
	FString StatsString;
	if (Meta.TotalLength_m > 0.0)
	{
		StatsString = FString::Printf(TEXT("Длина: %.0f м • Смета: %.1f млн руб."),
			Meta.TotalLength_m, Meta.TotalCost_kRub / 1000.0);
	}
	else
	{
		StatsString = TEXT("Трассировка: готова к расчету");
	}

	if (Meta.bHasRoads) StatsString += TEXT(" • Автодороги");
	if (Meta.bHasRivers) StatsString += TEXT(" • Реки");

	UTextBlock* StatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	StatsText->SetText(FText::FromString(StatsString));
	StatsText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	StatsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.85f, 0.7f, 1.0f)));
	if (UVerticalBoxSlot* SubSlot = InfoVBox->AddChildToVerticalBox(StatsText))
	{
		SubSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));
	}

	// Правая колонка карточки: Кнопки действий
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

	// Кнопка В ОБЛАКО
	UButton* CloudSyncBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CloudSyncBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.06f, 0.22f, 0.36f, 0.85f),
		FLinearColor(0.10f, 0.32f, 0.50f, 0.95f),
		FLinearColor(0.04f, 0.16f, 0.26f, 1.0f)));
	CloudSyncBtn->OnClicked.AddDynamic(ActionHelper, &URailProjectActionHelper::OnSyncToCloudClicked);
	ActionsHBox->AddChildToHorizontalBox(CloudSyncBtn);

	UBorder* CloudInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	CloudInner->SetBrushColor(FLinearColor::Transparent);
	CloudInner->SetPadding(FMargin(10.0f, 8.0f));
	CloudSyncBtn->AddChild(CloudInner);

	UTextBlock* CloudText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CloudText->SetText(FText::FromString(TEXT("☁ В облако")));
	CloudText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	CloudText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.85f, 1.0f, 1.0f)));
	CloudInner->SetContent(CloudText);

	USpacer* BtnSpacer2 = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	BtnSpacer2->SetSize(FVector2D(8.0f, 1.0f));
	ActionsHBox->AddChildToHorizontalBox(BtnSpacer2);

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

	USpacer* BtnSpacer3 = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	BtnSpacer3->SetSize(FVector2D(8.0f, 1.0f));
	ActionsHBox->AddChildToHorizontalBox(BtnSpacer3);

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
	DelText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	DelText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.45f, 0.45f, 1.0f)));
	DelInner->SetContent(DelText);
}

void URailMainMenuWidget::PopulateCloudProjectCard(const FRailCloudProject& CloudProj, UScrollBox* TargetScrollBox)
{
	if (!TargetScrollBox)
		return;

	UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	CardBorder->SetBrushColor(FLinearColor(0.02f, 0.045f, 0.08f, 0.90f));
	CardBorder->SetPadding(FMargin(16.0f, 14.0f));

	if (UScrollBoxSlot* CardSlot = Cast<UScrollBoxSlot>(TargetScrollBox->AddChild(CardBorder)))
	{
		CardSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	UHorizontalBox* CardHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	CardBorder->SetContent(CardHBox);

	UVerticalBox* InfoVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (UHorizontalBoxSlot* SubSlot = CardHBox->AddChildToHorizontalBox(InfoVBox))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SubSlot->SetVerticalAlignment(VAlign_Center);
	}

	UHorizontalBox* TitleLineHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	InfoVBox->AddChildToVerticalBox(TitleLineHBox);

	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameText->SetText(FText::FromString(CloudProj.Name));
	NameText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 13));
	NameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.96f, 0.98f, 1.0f)));
	TitleLineHBox->AddChildToHorizontalBox(NameText);

	// Бейдж статуса проекта в облаке
	UBorder* StatusBadge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	FLinearColor BadgeBg = FLinearColor(0.08f, 0.22f, 0.38f, 0.85f);
	FLinearColor BadgeFg = FLinearColor(0.35f, 0.78f, 1.0f, 1.0f);

	if (CloudProj.Status == ERailCloudProjectStatus::InProgress)
	{
		BadgeBg = FLinearColor(0.42f, 0.24f, 0.05f, 0.85f);
		BadgeFg = FLinearColor(1.0f, 0.75f, 0.2f, 1.0f);
	}
	else if (CloudProj.Status == ERailCloudProjectStatus::Completed)
	{
		BadgeBg = FLinearColor(0.05f, 0.32f, 0.15f, 0.85f);
		BadgeFg = FLinearColor(0.3f, 0.95f, 0.5f, 1.0f);
	}

	StatusBadge->SetBrushColor(BadgeBg);
	StatusBadge->SetPadding(FMargin(8.0f, 2.0f));
	if (UHorizontalBoxSlot* SubSlot = TitleLineHBox->AddChildToHorizontalBox(StatusBadge))
	{
		SubSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
		SubSlot->SetVerticalAlignment(VAlign_Center);
	}

	UTextBlock* StatusBadgeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	StatusBadgeText->SetText(FText::FromString(CloudProj.StatusString));
	StatusBadgeText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 9));
	StatusBadgeText->SetColorAndOpacity(FSlateColor(BadgeFg));
	StatusBadge->SetContent(StatusBadgeText);

	// Описание и дата
	FString Subtitle = CloudProj.Description.IsEmpty() ? TEXT("Нет описания") : CloudProj.Description;
	if (!CloudProj.CreatedAt.IsEmpty())
	{
		Subtitle += FString::Printf(TEXT(" • Создан: %s"), *CloudProj.CreatedAt.Left(10));
	}

	UTextBlock* DescText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	DescText->SetText(FText::FromString(Subtitle));
	DescText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	DescText->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.72f, 0.82f, 1.0f)));
	if (UVerticalBoxSlot* SubSlot = InfoVBox->AddChildToVerticalBox(DescText))
	{
		SubSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));
	}

	// Правая колонка: кнопки действий облака
	UHorizontalBox* ActionsHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UHorizontalBoxSlot* SubSlot = CardHBox->AddChildToHorizontalBox(ActionsHBox))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		SubSlot->SetVerticalAlignment(VAlign_Center);
	}

	URailCloudProjectActionHelper* Helper = NewObject<URailCloudProjectActionHelper>(this);
	Helper->CloudProject = CloudProj;
	Helper->MenuWidget = this;
	CloudActionHelpers.Add(Helper);

	// Кнопка создания локальной копии
	UButton* CloneBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CloneBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.08f, 0.32f, 0.22f, 0.95f),
		FLinearColor(0.12f, 0.48f, 0.32f, 1.0f),
		FLinearColor(0.05f, 0.22f, 0.15f, 1.0f)));
	CloneBtn->OnClicked.AddDynamic(Helper, &URailCloudProjectActionHelper::OnCreateLocalCloneClicked);
	ActionsHBox->AddChildToHorizontalBox(CloneBtn);

	UBorder* CloneInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	CloneInner->SetBrushColor(FLinearColor::Transparent);
	CloneInner->SetPadding(FMargin(14.0f, 8.0f));
	CloneBtn->AddChild(CloneInner);

	UTextBlock* CloneText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CloneText->SetText(FText::FromString(TEXT("ВЫБРАТЬ ДЛЯ 3D")));
	CloneText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
	CloneText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.95f, 0.4f, 1.0f)));
	CloneInner->SetContent(CloneText);

	USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	Spacer->SetSize(FVector2D(8.0f, 1.0f));
	ActionsHBox->AddChildToHorizontalBox(Spacer);

	// Кнопка удаления из облака
	UButton* DelBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	DelBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.18f, 0.05f, 0.05f, 0.85f),
		FLinearColor(0.32f, 0.08f, 0.08f, 0.95f),
		FLinearColor(0.12f, 0.03f, 0.03f, 1.0f)));
	DelBtn->OnClicked.AddDynamic(Helper, &URailCloudProjectActionHelper::OnDeleteFromCloudClicked);
	ActionsHBox->AddChildToHorizontalBox(DelBtn);

	UBorder* DelInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	DelInner->SetBrushColor(FLinearColor::Transparent);
	DelInner->SetPadding(FMargin(10.0f, 8.0f));
	DelBtn->AddChild(DelInner);

	UTextBlock* DelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	DelText->SetText(FText::FromString(TEXT("Удалить")));
	DelText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	DelText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.45f, 0.45f, 1.0f)));
	DelInner->SetContent(DelText);
}

void URailMainMenuWidget::RefreshProjectLibrary()
{
	if (!ProjectListScrollBox)
	{
		return;
	}

	ProjectListScrollBox->ClearChildren();
	ActionHelpers.Empty();
	CloudActionHelpers.Empty();

	if (bShowingCloudProjects)
	{
		URailApiService* Api = URailApiService::Get(this);
		if (!Api || !Api->IsAuthenticated())
		{
			UBorder* EmptyBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			EmptyBorder->SetBrushColor(FLinearColor(0.03f, 0.06f, 0.10f, 0.8f));
			EmptyBorder->SetPadding(FMargin(24.0f, 20.0f));
			ProjectListScrollBox->AddChild(EmptyBorder);

			UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			EmptyText->SetText(FText::FromString(TEXT("Для просмотра облачных проектов войдите в учетную запись (кнопка 'Облако / Вход' вверху).")));
			EmptyText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 11));
			EmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.78f, 0.88f, 1.0f)));
			EmptyBorder->SetContent(EmptyText);

			if (ProjectCountText) ProjectCountText->SetText(FText::FromString(TEXT("Не авторизован")));
			return;
		}

		if (Api->CachedCloudProjects.Num() == 0)
		{
			Api->FetchProjects(0, 50);
		}
		else
		{
			for (const FRailCloudProject& CloudProj : Api->CachedCloudProjects)
			{
				PopulateCloudProjectCard(CloudProj, ProjectListScrollBox);
			}
			if (ProjectCountText)
			{
				ProjectCountText->SetText(FText::FromString(FString::Printf(TEXT("В облаке: %d"), Api->CachedCloudProjects.Num())));
			}
		}
	}
	else
	{
		URailProjectManager::EnsureProjectsDirectoryExists();
		TArray<FRailProjectMetadata> Projects = URailProjectManager::GetAllProjects();

		for (const FRailProjectMetadata& Proj : Projects)
		{
			PopulateProjectCard(Proj, ProjectListScrollBox);
		}

		if (ProjectCountText)
		{
			ProjectCountText->SetText(FText::FromString(FString::Printf(TEXT("Всего: %d"), Projects.Num())));
		}
	}
}

// ─────────────────────────────────────────────────────────────────
//  Модальные окна
// ─────────────────────────────────────────────────────────────────

void URailMainMenuWidget::BuildCreateProjectModal(UCanvasPanel* RootCanvas)
{
	CreateProjectModalBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CreateProjectModalBorder"));
	CreateProjectModalBorder->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.03f, 0.85f));
	CreateProjectModalBorder->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* ModalSlot = RootCanvas->AddChildToCanvas(CreateProjectModalBorder);
	ModalSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	ModalSlot->SetOffsets(FMargin(0.0f));

	USizeBox* ModalSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ModalSizeBox->SetWidthOverride(540.0f);
	ModalSizeBox->SetMaxDesiredHeight(680.0f);

	UBorder* DialogCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	DialogCard->SetBrushColor(FLinearColor(0.02f, 0.035f, 0.065f, 0.98f));
	DialogCard->SetPadding(FMargin(28.0f, 20.0f));
	ModalSizeBox->SetContent(DialogCard);

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
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(ProjectNameInput))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 12.0f));
	}

	// Поле 2: Категория железной дороги
	UTextBlock* CatLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CatLabel->SetText(FText::FromString(TEXT("Категория дороги (СП 119.13330):")));
	CatLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	CatLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	VBox->AddChildToVerticalBox(CatLabel);

	CategoryComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
	CategoryComboBox->AddOption(TEXT("Скоростная (до 250 км/ч)"));
	CategoryComboBox->AddOption(TEXT("Особогрузонапряженная"));
	CategoryComboBox->AddOption(TEXT("I Категория (Магистральная)"));
	CategoryComboBox->AddOption(TEXT("II Категория"));
	CategoryComboBox->AddOption(TEXT("III Категория"));
	CategoryComboBox->AddOption(TEXT("IV Категория"));
	CategoryComboBox->SetSelectedIndex(2);
	CategoryComboBox->OnGenerateWidgetEvent.BindDynamic(this, &URailMainMenuWidget::HandleGenerateCategoryWidget);

	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(CategoryComboBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 14.0f));
	}

	// Переключатель типа источника данных
	UTextBlock* SourceTypeLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SourceTypeLabel->SetText(FText::FromString(TEXT("Тип карты высот рельефа:")));
	SourceTypeLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	SourceTypeLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	VBox->AddChildToVerticalBox(SourceTypeLabel);

	UHorizontalBox* TypeHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(TypeHBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 10.0f));
	}

	PngTypeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	PngTypeButton->SetStyle(MakeGlassButtonStyle(FLinearColor(0.08f, 0.32f, 0.52f, 0.95f), FLinearColor(0.12f, 0.42f, 0.65f, 1.0f), FLinearColor(0.06f, 0.24f, 0.40f, 1.0f)));
	PngTypeButton->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleSourceTypeToggledPng);
	if (UHorizontalBoxSlot* HSlot = TypeHBox->AddChildToHorizontalBox(PngTypeButton))
	{
		HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
	}

	PngTypeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	PngTypeText->SetText(FText::FromString(TEXT("16-битный PNG (готовая карта)")));
	PngTypeText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
	PngTypeText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	PngTypeText->SetJustification(ETextJustify::Center);
	PngTypeButton->SetContent(PngTypeText);

	TifTypeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	TifTypeButton->SetStyle(MakeGlassButtonStyle(FLinearColor(0.05f, 0.08f, 0.14f, 0.85f), FLinearColor(0.09f, 0.15f, 0.24f, 0.95f), FLinearColor(0.03f, 0.06f, 0.10f, 1.0f)));
	TifTypeButton->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleSourceTypeToggledTif);
	if (UHorizontalBoxSlot* HSlot = TypeHBox->AddChildToHorizontalBox(TifTypeButton))
	{
		HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HSlot->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));
	}

	TifTypeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TifTypeText->SetText(FText::FromString(TEXT("GeoTIFF (.tif спутниковый)")));
	TifTypeText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	TifTypeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.68f, 0.78f, 1.0f)));
	TifTypeText->SetJustification(ETextJustify::Center);
	TifTypeButton->SetContent(TifTypeText);

	// Путь к файлу с кнопкой Обзор
	UHorizontalBox* FileHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(FileHBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	FilePathInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	FilePathInput->SetHintText(FText::FromString(TEXT("Выберите файл карты высот...")));
	if (UHorizontalBoxSlot* HSlot = FileHBox->AddChildToHorizontalBox(FilePathInput))
	{
		HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	UButton* BrowseBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	BrowseBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.12f, 0.18f, 0.28f, 0.9f), FLinearColor(0.18f, 0.26f, 0.38f, 1.0f), FLinearColor(0.08f, 0.12f, 0.20f, 1.0f)));
	BrowseBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleBrowseFileClicked);
	FileHBox->AddChildToHorizontalBox(BrowseBtn);

	UTextBlock* BrowseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BrowseText->SetText(FText::FromString(TEXT("Обзор...")));
	BrowseText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
	BrowseText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.98f, 1.0f)));
	BrowseBtn->SetContent(BrowseText);

	// Чекбоксы слоев
	RoadsCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
	RoadsCheckBox->SetIsChecked(true);

	UHorizontalBox* RoadsHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	RoadsHBox->AddChildToHorizontalBox(RoadsCheckBox);

	UTextBlock* RoadsLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	RoadsLabel->SetText(FText::FromString(TEXT(" Загрузить автодорожную сеть (OSM)")));
	RoadsLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	RoadsLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f)));
	RoadsHBox->AddChildToHorizontalBox(RoadsLabel);
	VBox->AddChildToVerticalBox(RoadsHBox);

	RiversCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
	RiversCheckBox->SetIsChecked(true);

	UHorizontalBox* RiversHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	RiversHBox->AddChildToHorizontalBox(RiversCheckBox);

	UTextBlock* RiversLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	RiversLabel->SetText(FText::FromString(TEXT(" Загрузить реки и водоемы (OSM)")));
	RiversLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	RiversLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f)));
	RiversHBox->AddChildToHorizontalBox(RiversLabel);
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(RiversHBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 16.0f));
	}

	// Кнопки Создать / Отмена
	UHorizontalBox* ModalBtnsHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	VBox->AddChildToVerticalBox(ModalBtnsHBox);

	UButton* ConfirmBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	ConfirmBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.08f, 0.38f, 0.22f, 0.95f), FLinearColor(0.12f, 0.50f, 0.30f, 1.0f), FLinearColor(0.05f, 0.26f, 0.15f, 1.0f)));
	ConfirmBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleCreateProjectConfirmClicked);
	if (UHorizontalBoxSlot* HSlot = ModalBtnsHBox->AddChildToHorizontalBox(ConfirmBtn))
	{
		HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	UTextBlock* ConfirmText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ConfirmText->SetText(FText::FromString(TEXT("СОЗДАТЬ И ЗАПУСТИТЬ")));
	ConfirmText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	ConfirmText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ConfirmText->SetJustification(ETextJustify::Center);
	ConfirmBtn->SetContent(ConfirmText);

	UButton* CancelBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CancelBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.18f, 0.22f, 0.28f, 0.9f), FLinearColor(0.25f, 0.30f, 0.38f, 1.0f), FLinearColor(0.12f, 0.15f, 0.20f, 1.0f)));
	CancelBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleCreateProjectCancelClicked);
	ModalBtnsHBox->AddChildToHorizontalBox(CancelBtn);

	UTextBlock* CancelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CancelText->SetText(FText::FromString(TEXT("Отмена")));
	CancelText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	CancelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f)));
	CancelBtn->SetContent(CancelText);
}

void URailMainMenuWidget::BuildCreateCloudProjectModal(UCanvasPanel* RootCanvas)
{
	CreateCloudModalBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CreateCloudModalBorder"));
	CreateCloudModalBorder->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.03f, 0.85f));
	CreateCloudModalBorder->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* ModalSlot = RootCanvas->AddChildToCanvas(CreateCloudModalBorder);
	ModalSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	ModalSlot->SetOffsets(FMargin(0.0f));

	USizeBox* ModalSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ModalSizeBox->SetWidthOverride(480.0f);

	UBorder* DialogCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	DialogCard->SetBrushColor(FLinearColor(0.02f, 0.035f, 0.065f, 0.98f));
	DialogCard->SetPadding(FMargin(26.0f, 20.0f));
	ModalSizeBox->SetContent(DialogCard);

	UCanvasPanel* ModalCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	CreateCloudModalBorder->SetContent(ModalCanvas);

	UCanvasPanelSlot* CardSlot = ModalCanvas->AddChildToCanvas(ModalSizeBox);
	CardSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
	CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CardSlot->SetPosition(FVector2D(0.0f, 0.0f));
	CardSlot->SetAutoSize(true);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	DialogCard->SetContent(VBox);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("СОЗДАНИЕ ПРОЕКТА В ОБЛАКЕ (REST API)")));
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(Title))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	UTextBlock* NameLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameLabel->SetText(FText::FromString(TEXT("Название проекта в облаке:")));
	NameLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	NameLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	VBox->AddChildToVerticalBox(NameLabel);

	CloudProjNameInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	CloudProjNameInput->SetText(FText::FromString(TEXT("Северный железнодорожный коридор")));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(CloudProjNameInput))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 12.0f));
	}

	UTextBlock* DescLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	DescLabel->SetText(FText::FromString(TEXT("Описание проекта (до 500 символов):")));
	DescLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	DescLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	VBox->AddChildToVerticalBox(DescLabel);

	CloudProjDescInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	CloudProjDescInput->SetText(FText::FromString(TEXT("Предварительное технико-экономическое обоснование трассы")));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(CloudProjDescInput))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 18.0f));
	}

	UHorizontalBox* BtnsHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	VBox->AddChildToVerticalBox(BtnsHBox);

	UButton* ConfirmBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	ConfirmBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.08f, 0.38f, 0.22f, 0.95f), FLinearColor(0.12f, 0.50f, 0.30f, 1.0f), FLinearColor(0.05f, 0.26f, 0.15f, 1.0f)));
	ConfirmBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleCreateCloudConfirmClicked);
	if (UHorizontalBoxSlot* HSlot = BtnsHBox->AddChildToHorizontalBox(ConfirmBtn))
	{
		HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	UTextBlock* ConfirmText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ConfirmText->SetText(FText::FromString(TEXT("Создать в базе API")));
	ConfirmText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	ConfirmText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ConfirmText->SetJustification(ETextJustify::Center);
	ConfirmBtn->SetContent(ConfirmText);

	UButton* CancelBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CancelBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.18f, 0.22f, 0.28f, 0.9f), FLinearColor(0.25f, 0.30f, 0.38f, 1.0f), FLinearColor(0.12f, 0.15f, 0.20f, 1.0f)));
	CancelBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleCreateCloudCancelClicked);
	BtnsHBox->AddChildToHorizontalBox(CancelBtn);

	UTextBlock* CancelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CancelText->SetText(FText::FromString(TEXT("Отмена")));
	CancelText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	CancelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f)));
	CancelBtn->SetContent(CancelText);
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
	ModalSizeBox->SetWidthOverride(480.0f);
	ModalSizeBox->SetMaxDesiredHeight(680.0f);

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

	UScrollBox* ModalScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	DialogCard->SetContent(ModalScroll);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ModalScroll->AddChild(VBox);

	AuthTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	AuthTitleText->SetText(FText::FromString(TEXT("ОБЛАЧНАЯ АВТОРИЗАЦИЯ (JWT)")));
	AuthTitleText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 15));
	AuthTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(AuthTitleText))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	// Переключатель режима: Вход / Регистрация
	UHorizontalBox* ModeHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(ModeHBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	AuthTabLoginBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	AuthTabLoginBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.08f, 0.32f, 0.52f, 0.95f), FLinearColor(0.12f, 0.42f, 0.65f, 1.0f), FLinearColor(0.06f, 0.24f, 0.40f, 1.0f)));
	AuthTabLoginBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleAuthModeLoginClicked);
	if (UHorizontalBoxSlot* HSlot = ModeHBox->AddChildToHorizontalBox(AuthTabLoginBtn))
	{
		HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
	}

	AuthTabLoginText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	AuthTabLoginText->SetText(FText::FromString(TEXT("Вход в аккаунт")));
	AuthTabLoginText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	AuthTabLoginText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	AuthTabLoginText->SetJustification(ETextJustify::Center);
	AuthTabLoginBtn->SetContent(AuthTabLoginText);

	AuthTabRegisterBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	AuthTabRegisterBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.05f, 0.08f, 0.14f, 0.85f), FLinearColor(0.09f, 0.15f, 0.24f, 0.95f), FLinearColor(0.03f, 0.06f, 0.10f, 1.0f)));
	AuthTabRegisterBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleAuthModeRegisterClicked);
	if (UHorizontalBoxSlot* HSlot = ModeHBox->AddChildToHorizontalBox(AuthTabRegisterBtn))
	{
		HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HSlot->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));
	}

	AuthTabRegisterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	AuthTabRegisterText->SetText(FText::FromString(TEXT("Регистрация")));
	AuthTabRegisterText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	AuthTabRegisterText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.75f, 0.85f, 1.0f)));
	AuthTabRegisterText->SetJustification(ETextJustify::Center);
	AuthTabRegisterBtn->SetContent(AuthTabRegisterText);

	URailApiService* Api = URailApiService::Get(this);

	// Поле Email
	UTextBlock* EmailLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	EmailLabel->SetText(FText::FromString(TEXT("Email пользователя:")));
	EmailLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
	EmailLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	VBox->AddChildToVerticalBox(EmailLabel);

	AuthEmailInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	AuthEmailInput->SetText(FText::FromString(Api && Api->IsAuthenticated() ? Api->GetAuthData().Email : TEXT("user@example.com")));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(AuthEmailInput))
	{
		SubSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 10.0f));
	}

	// Поле пароля
	UTextBlock* PassLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	PassLabel->SetText(FText::FromString(TEXT("Пароль:")));
	PassLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
	PassLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	VBox->AddChildToVerticalBox(PassLabel);

	AuthPassInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	AuthPassInput->SetIsPassword(true);
	AuthPassInput->SetText(FText::FromString(TEXT("secure-password")));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(AuthPassInput))
	{
		SubSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 12.0f));
	}

	// Сообщение о статусе
	AuthStatusMessage = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	AuthStatusMessage->SetText(FText::GetEmpty());
	AuthStatusMessage->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	AuthStatusMessage->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.7f, 0.2f, 1.0f)));
	AuthStatusMessage->SetAutoWrapText(true);
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(AuthStatusMessage))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	// Кнопки отправки / закрытия
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
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	AuthSubmitText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	AuthSubmitText->SetText(FText::FromString(TEXT("ВОЙТИ В ОБЛАКО")));
	AuthSubmitText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	AuthSubmitText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	AuthSubmitText->SetJustification(ETextJustify::Center);
	LoginBtn->SetContent(AuthSubmitText);

	UButton* CloseBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CloseBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.12f, 0.16f, 0.22f, 0.85f),
		FLinearColor(0.18f, 0.22f, 0.30f, 0.95f),
		FLinearColor(0.08f, 0.10f, 0.15f, 1.0f)));
	CloseBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleAuthCloseClicked);
	BtnHBox->AddChildToHorizontalBox(CloseBtn);

	UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CloseText->SetText(FText::FromString(TEXT("Закрыть")));
	CloseText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	CloseText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.92f, 1.0f)));
	CloseBtn->SetContent(CloseText);

	// Кнопка Выйти из аккаунта (если авторизован)
	AuthLogoutBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	AuthLogoutBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.25f, 0.08f, 0.08f, 0.85f),
		FLinearColor(0.38f, 0.12f, 0.12f, 0.95f),
		FLinearColor(0.15f, 0.05f, 0.05f, 1.0f)));
	AuthLogoutBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleAuthLogoutClicked);
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(AuthLogoutBtn))
	{
		SubSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
	}

	UTextBlock* LogoutText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LogoutText->SetText(FText::FromString(TEXT("Выйти из учетной записи")));
	LogoutText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
	LogoutText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.5f, 0.5f, 1.0f)));
	LogoutText->SetJustification(ETextJustify::Center);
	AuthLogoutBtn->SetContent(LogoutText);
}

void URailMainMenuWidget::BuildLoadingOverlay(UCanvasPanel* RootCanvas)
{
	LoadingOverlayBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LoadingOverlayBorder"));
	LoadingOverlayBorder->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.025f, 0.92f));
	LoadingOverlayBorder->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* OverlaySlot = RootCanvas->AddChildToCanvas(LoadingOverlayBorder);
	OverlaySlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	OverlaySlot->SetOffsets(FMargin(0.0f));

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	LoadingOverlayBorder->SetContent(VBox);

	UTextBlock* SpinText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SpinText->SetText(FText::FromString(TEXT("⏳")));
	SpinText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 36));
	SpinText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	SpinText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(SpinText))
	{
		SubSlot->SetHorizontalAlignment(HAlign_Center);
		SubSlot->SetPadding(FMargin(0.0f, 240.0f, 0.0f, 12.0f));
	}

	LoadingStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LoadingStatusText->SetText(FText::FromString(TEXT("Загрузка и подготовка рельефа...")));
	LoadingStatusText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
	LoadingStatusText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	LoadingStatusText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(LoadingStatusText))
	{
		SubSlot->SetHorizontalAlignment(HAlign_Center);
	}
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
	if (CreateCloudModalBorder && CreateCloudModalBorder->GetVisibility() == ESlateVisibility::Visible)
	{
		CreateCloudModalBorder->SetVisibility(ESlateVisibility::Collapsed);
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

// ─────────────────────────────────────────────────────────────────
//  Обработчики событий
// ─────────────────────────────────────────────────────────────────

void URailMainMenuWidget::HandleNewProjectClicked()
{
	if (CreateProjectModalBorder)
	{
		CreateProjectModalBorder->SetVisibility(ESlateVisibility::Visible);
	}
}

void URailMainMenuWidget::HandleOpenProjectsFolderClicked()
{
	URailProjectManager::OpenFolderInExplorer(URailProjectManager::GetProjectsRootDirectory());
}

void URailMainMenuWidget::HandleAuthButtonClicked()
{
	if (AuthModalBorder)
	{
		URailApiService* Api = URailApiService::Get(this);
		if (Api)
		{
			if (AuthLogoutBtn)
			{
				AuthLogoutBtn->SetVisibility(Api->IsAuthenticated() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			}
			if (AuthStatusMessage)
			{
				if (Api->IsAuthenticated())
				{
					AuthStatusMessage->SetText(FText::FromString(FString::Printf(TEXT("Вы авторизованы как %s (роль: %s)"), *Api->GetAuthData().Email, *Api->GetAuthData().Role)));
					AuthStatusMessage->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.9f, 0.6f)));
				}
				else
				{
					AuthStatusMessage->SetText(FText::GetEmpty());
				}
			}
		}
		AuthModalBorder->SetVisibility(ESlateVisibility::Visible);
	}
}

void URailMainMenuWidget::HandleRefreshLibraryClicked()
{
	RefreshProjectLibrary();
	ShowToast(bShowingCloudProjects ? TEXT("Обновление облачных проектов...") : TEXT("Список локальных проектов обновлен"), ERailToastType::Info, 2.5f);
}

void URailMainMenuWidget::HandleGraphicsSettingsClicked()
{
	OnGraphicsSettingsRequested.Broadcast();
}

void URailMainMenuWidget::HandleTabLocalClicked()
{
	bShowingCloudProjects = false;
	if (TabLocalBtn && TabCloudBtn)
	{
		TabLocalBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.06f, 0.24f, 0.42f, 0.95f), FLinearColor(0.1f, 0.34f, 0.58f, 1.0f), FLinearColor(0.04f, 0.18f, 0.32f, 1.0f)));
		TabCloudBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.04f, 0.08f, 0.14f, 0.85f), FLinearColor(0.08f, 0.15f, 0.24f, 0.95f), FLinearColor(0.03f, 0.06f, 0.10f, 1.0f)));
		TabLocalText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
		TabLocalText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		TabCloudText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
		TabCloudText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.75f, 0.85f, 1.0f)));
	}
	RefreshProjectLibrary();
}

void URailMainMenuWidget::HandleTabCloudClicked()
{
	bShowingCloudProjects = true;
	if (TabLocalBtn && TabCloudBtn)
	{
		TabCloudBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.06f, 0.24f, 0.42f, 0.95f), FLinearColor(0.1f, 0.34f, 0.58f, 1.0f), FLinearColor(0.04f, 0.18f, 0.32f, 1.0f)));
		TabLocalBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.04f, 0.08f, 0.14f, 0.85f), FLinearColor(0.08f, 0.15f, 0.24f, 0.95f), FLinearColor(0.03f, 0.06f, 0.10f, 1.0f)));
		TabCloudText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
		TabCloudText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		TabLocalText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
		TabLocalText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.75f, 0.85f, 1.0f)));
	}
	RefreshProjectLibrary();
}

void URailMainMenuWidget::HandleCreateCloudProjectClicked()
{
	URailApiService* Api = URailApiService::Get(this);
	if (!Api || !Api->IsAuthenticated())
	{
		ShowToast(TEXT("Для создания проектов в облаке войдите в учетную запись"), ERailToastType::Warning);
		HandleAuthButtonClicked();
		return;
	}

	if (CreateCloudModalBorder)
	{
		CreateCloudModalBorder->SetVisibility(ESlateVisibility::Visible);
	}
}

void URailMainMenuWidget::HandleCreateCloudConfirmClicked()
{
	if (!CloudProjNameInput)
		return;

	const FString Name = CloudProjNameInput->GetText().ToString().TrimStartAndEnd();
	const FString Desc = CloudProjDescInput ? CloudProjDescInput->GetText().ToString().TrimStartAndEnd() : TEXT("");

	if (Name.IsEmpty())
	{
		ShowToast(TEXT("Название проекта не может быть пустым"), ERailToastType::Error);
		return;
	}

	if (URailApiService* Api = URailApiService::Get(this))
	{
		ShowToast(TEXT("Отправка запроса создания в API..."), ERailToastType::Info);
		Api->CreateRemoteProject(Name, Desc);
	}

	if (CreateCloudModalBorder)
	{
		CreateCloudModalBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URailMainMenuWidget::HandleCreateCloudCancelClicked()
{
	if (CreateCloudModalBorder)
	{
		CreateCloudModalBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URailMainMenuWidget::HandleSourceTypeToggledPng()
{
	bSourceIsGeoTiff = false;
	PngTypeButton->SetStyle(MakeGlassButtonStyle(FLinearColor(0.08f, 0.32f, 0.52f, 0.95f), FLinearColor(0.12f, 0.42f, 0.65f, 1.0f), FLinearColor(0.06f, 0.24f, 0.40f, 1.0f)));
	TifTypeButton->SetStyle(MakeGlassButtonStyle(FLinearColor(0.05f, 0.08f, 0.14f, 0.85f), FLinearColor(0.09f, 0.15f, 0.24f, 0.95f), FLinearColor(0.03f, 0.06f, 0.10f, 1.0f)));
	PngTypeText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
	PngTypeText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TifTypeText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	TifTypeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.68f, 0.78f, 1.0f)));
}

void URailMainMenuWidget::HandleSourceTypeToggledTif()
{
	bSourceIsGeoTiff = true;
	TifTypeButton->SetStyle(MakeGlassButtonStyle(FLinearColor(0.08f, 0.32f, 0.52f, 0.95f), FLinearColor(0.12f, 0.42f, 0.65f, 1.0f), FLinearColor(0.06f, 0.24f, 0.40f, 1.0f)));
	PngTypeButton->SetStyle(MakeGlassButtonStyle(FLinearColor(0.05f, 0.08f, 0.14f, 0.85f), FLinearColor(0.09f, 0.15f, 0.24f, 0.95f), FLinearColor(0.03f, 0.06f, 0.10f, 1.0f)));
	TifTypeText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
	TifTypeText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	PngTypeText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	PngTypeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.68f, 0.78f, 1.0f)));
}

void URailMainMenuWidget::HandleBrowseFileClicked()
{
	FString SelectedPath;
	const FString Title = bSourceIsGeoTiff ? TEXT("Выберите GeoTIFF карту высот") : TEXT("Выберите 16-битный PNG файл высот");
	const FString Filter = bSourceIsGeoTiff ? TEXT("GeoTIFF Images (*.tif;*.tiff)|*.tif;*.tiff") : TEXT("PNG Heightmap (*.png)|*.png");

	if (URailProjectManager::OpenFileDialogWindows(Title, Filter, SelectedPath))
	{
		if (FilePathInput)
		{
			FilePathInput->SetText(FText::FromString(SelectedPath));
		}
	}
}

void URailMainMenuWidget::HandleCreateProjectConfirmClicked()
{
	const FString ProjName = ProjectNameInput ? ProjectNameInput->GetText().ToString().TrimStartAndEnd() : TEXT("");
	const FString SrcPath = FilePathInput ? FilePathInput->GetText().ToString().TrimStartAndEnd() : TEXT("");

	if (ProjName.IsEmpty())
	{
		ShowToast(TEXT("Укажите название проекта!"), ERailToastType::Error);
		return;
	}
	if (SrcPath.IsEmpty() || !FPaths::FileExists(SrcPath))
	{
		ShowToast(TEXT("Указанный файл рельефа не найден!"), ERailToastType::Error);
		return;
	}

	const int32 CatIdx = CategoryComboBox ? CategoryComboBox->GetSelectedIndex() : 2;
	const ERailRoadCategory Category = static_cast<ERailRoadCategory>(FMath::Clamp(CatIdx, 0, 7));
	const bool bRoads = RoadsCheckBox ? RoadsCheckBox->IsChecked() : false;
	const bool bRivers = RiversCheckBox ? RiversCheckBox->IsChecked() : false;

	ShowLoadingOverlay(TEXT("Создание проекта и подготовка рельефа..."));

	FRailProjectMetadata CreatedProject;
	FString ErrorMsg;
	bool bSuccess = false;

	if (bSourceIsGeoTiff)
	{
		bSuccess = URailProjectManager::CreateProjectFromGeoTiff(ProjName, Category, SrcPath, bRoads, bRivers, CreatedProject, ErrorMsg);
	}
	else
	{
		bSuccess = URailProjectManager::CreateProjectFromPng(ProjName, Category, SrcPath, bRoads, bRivers, CreatedProject, ErrorMsg);
	}

	HideLoadingOverlay();

	if (bSuccess)
	{
		if (CreateProjectModalBorder)
		{
			CreateProjectModalBorder->SetVisibility(ESlateVisibility::Collapsed);
		}
		ShowToast(TEXT("Проект успешно создан!"), ERailToastType::Success);

		// Если пользователь авторизован в облаке, автоматически создаем проект в API
		if (URailApiService* Api = URailApiService::Get(this))
		{
			if (Api->IsAuthenticated())
			{
				Api->SyncLocalProjectToCloud(CreatedProject);
			}
		}

		RefreshProjectLibrary();
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

void URailMainMenuWidget::HandleAuthModeLoginClicked()
{
	bAuthIsRegisterMode = false;
	AuthTabLoginBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.08f, 0.32f, 0.52f, 0.95f), FLinearColor(0.12f, 0.42f, 0.65f, 1.0f), FLinearColor(0.06f, 0.24f, 0.40f, 1.0f)));
	AuthTabRegisterBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.05f, 0.08f, 0.14f, 0.85f), FLinearColor(0.09f, 0.15f, 0.24f, 0.95f), FLinearColor(0.03f, 0.06f, 0.10f, 1.0f)));
	AuthTabLoginText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	AuthTabLoginText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	AuthTabRegisterText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	AuthTabRegisterText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.75f, 0.85f, 1.0f)));

	AuthTitleText->SetText(FText::FromString(TEXT("ВХОД В ОБЛАКО (JWT / API)")));
	AuthSubmitText->SetText(FText::FromString(TEXT("ВОЙТИ В СИСТЕМУ")));
}

void URailMainMenuWidget::HandleAuthModeRegisterClicked()
{
	bAuthIsRegisterMode = true;
	AuthTabRegisterBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.08f, 0.32f, 0.52f, 0.95f), FLinearColor(0.12f, 0.42f, 0.65f, 1.0f), FLinearColor(0.06f, 0.24f, 0.40f, 1.0f)));
	AuthTabLoginBtn->SetStyle(MakeGlassButtonStyle(FLinearColor(0.05f, 0.08f, 0.14f, 0.85f), FLinearColor(0.09f, 0.15f, 0.24f, 0.95f), FLinearColor(0.03f, 0.06f, 0.10f, 1.0f)));
	AuthTabRegisterText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	AuthTabRegisterText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	AuthTabLoginText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	AuthTabLoginText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.75f, 0.85f, 1.0f)));

	AuthTitleText->SetText(FText::FromString(TEXT("РЕГИСТРАЦИЯ В СИСТЕМЕ (API)")));
	AuthSubmitText->SetText(FText::FromString(TEXT("ЗАРЕГИСТРИРОВАТЬСЯ")));
}

void URailMainMenuWidget::HandleAuthSubmitClicked()
{
	URailApiService* Api = URailApiService::Get(this);
	if (!Api)
	{
		ShowToast(TEXT("Сервис API недоступен"), ERailToastType::Error);
		return;
	}

	const FString Email = AuthEmailInput ? AuthEmailInput->GetText().ToString().TrimStartAndEnd() : TEXT("");
	const FString Password = AuthPassInput ? AuthPassInput->GetText().ToString() : TEXT("");

	if (Email.IsEmpty() || Password.IsEmpty())
	{
		if (AuthStatusMessage)
		{
			AuthStatusMessage->SetText(FText::FromString(TEXT("Заполните Email и пароль")));
			AuthStatusMessage->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.4f, 0.4f)));
		}
		return;
	}

	if (AuthStatusMessage)
	{
		AuthStatusMessage->SetText(FText::FromString(TEXT("Отправка запроса на сервер...")));
		AuthStatusMessage->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.85f, 1.0f)));
	}

	if (bAuthIsRegisterMode)
	{
		Api->Register(Email, Password);
	}
	else
	{
		Api->Login(Email, Password);
	}
}

void URailMainMenuWidget::HandleAuthLogoutClicked()
{
	if (URailApiService* Api = URailApiService::Get(this))
	{
		Api->Logout();
	}
	if (AuthModalBorder)
	{
		AuthModalBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URailMainMenuWidget::HandleAuthCloseClicked()
{
	if (AuthModalBorder)
	{
		AuthModalBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URailMainMenuWidget::HandleApiAuthCompleted(bool bSuccess, const FString& Message, const FString& Email, const FString& Role)
{
	UpdateHeaderProfileUI();

	if (AuthStatusMessage)
	{
		AuthStatusMessage->SetText(FText::FromString(Message));
		AuthStatusMessage->SetColorAndOpacity(FSlateColor(bSuccess ? FLinearColor(0.2f, 0.9f, 0.6f) : FLinearColor(1.0f, 0.4f, 0.4f)));
	}

	if (bSuccess)
	{
		ShowToast(FString::Printf(TEXT("Вход выполнен: %s (%s)"), *Email, *Role), ERailToastType::Success);
		if (AuthModalBorder)
		{
			AuthModalBorder->SetVisibility(ESlateVisibility::Collapsed);
		}
		RefreshProjectLibrary();
	}
	else
	{
		ShowToast(Message, ERailToastType::Error, 5.0f);
	}
}

void URailMainMenuWidget::HandleApiProjectsReceived(bool bSuccess, const TArray<FRailCloudProject>& Projects, const FString& ErrorMessage)
{
	if (bShowingCloudProjects)
	{
		RefreshProjectLibrary();
	}
}

void URailMainMenuWidget::HandleApiProjectActionCompleted(bool bSuccess, const FRailCloudProject& Project, const FString& Message)
{
	ShowToast(Message, bSuccess ? ERailToastType::Success : ERailToastType::Error);
	if (bShowingCloudProjects)
	{
		RefreshProjectLibrary();
	}
}

void URailMainMenuWidget::HandleApiProjectDeleted(bool bSuccess, const FString& Message)
{
	ShowToast(Message, bSuccess ? ERailToastType::Info : ERailToastType::Error);
	if (bShowingCloudProjects)
	{
		RefreshProjectLibrary();
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
