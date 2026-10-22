// Copyright (c) 2026 ÐÐ¸ÐºÐ¾Ð»Ð°Ð¹Ñ‡Ð¸Ðº Ð.Ð’. Ð’ÑÐµ Ð¿Ñ€Ð°Ð²Ð° Ð·Ð°Ñ‰Ð¸Ñ‰ÐµÐ½Ñ‹.
// Ð ÐµÐ°Ð»Ð¸Ð·Ð°Ñ†Ð¸Ñ Ð³Ð»Ð°Ð²Ð½Ð¾Ð³Ð¾ Ð¼ÐµÐ½ÑŽ Ð¸ Ð´Ð°ÑˆÐ±Ð¾Ñ€Ð´Ð° Ð¿Ñ€Ð¾ÐµÐºÑ‚Ð¾Ð²

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
			MenuWidget->ShowToast(FString::Printf(TEXT("Проект '%s' удалён."), *ProjectMeta.ProjectName), ERailToastType::Info);
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

		// â”€â”€â”€ Ð“Ð»Ð°Ð²Ð½Ñ‹Ð¹ Ð¿Ð¾Ð»Ð½Ð¾ÑÐºÑ€Ð°Ð½Ð½Ñ‹Ð¹ ÐºÐ¾Ð½Ñ‚ÐµÐ¹Ð½ÐµÑ€ â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
		UBorder* BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundBorder"));
		BackgroundBorder->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.045f, 0.96f));
		BackgroundBorder->SetPadding(FMargin(40.0f, 30.0f));

		UCanvasPanelSlot* BgSlot = RootCanvas->AddChildToCanvas(BackgroundBorder);
		BgSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BgSlot->SetOffsets(FMargin(0.0f));

		UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
		BackgroundBorder->SetContent(MainVBox);

		// â”€â”€â”€ Ð¨Ð°Ð¿ÐºÐ° Ð¸ Ð”Ð°ÑˆÐ±Ð¾Ñ€Ð´ â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
		BuildHeader(MainVBox);
		BuildContentArea(MainVBox);

		// â”€â”€â”€ ÐœÐ¾Ð´Ð°Ð»ÑŒÐ½Ñ‹Ðµ Ð¾ÐºÐ½Ð° Ð¸ Ð¾Ð²ÐµÑ€Ð»ÐµÐ¸ â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
		BuildCreateProjectModal(RootCanvas);
		BuildAuthModal(RootCanvas);
		BuildLoadingOverlay(RootCanvas);

		// â”€â”€â”€ Ð¢Ð¾ÑÑ‚Ñ‹ â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
		ToastWidget = WidgetTree->ConstructWidget<URailToastWidget>(URailToastWidget::StaticClass(), TEXT("ToastWidget"));
		if (ToastWidget)
		{
			UCanvasPanelSlot* ToastSlot = RootCanvas->AddChildToCanvas(ToastWidget);
			ToastSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
			ToastSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			ToastSlot->SetPosition(FVector2D(0.0f, 20.0f));
			ToastSlot->SetAutoSize(true);
		}

		// Ð—Ð°Ð¿Ð¾Ð»Ð½ÑÐµÐ¼ Ð±Ð¸Ð±Ð»Ð¸Ð¾Ñ‚ÐµÐºÑƒ Ð¿Ñ€Ð¾ÐµÐºÑ‚Ð¾Ð²
		RefreshProjectLibrary();
	}

	return Super::RebuildWidget();
}

void URailMainMenuWidget::BuildHeader(UVerticalBox* MainVBox)
{
	UHorizontalBox* HeaderHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = MainVBox->AddChildToVerticalBox(HeaderHBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
	}

	// Ð›Ð¾Ð³Ð¾Ñ‚Ð¸Ð¿ Ð¸ Ð½Ð°Ð·Ð²Ð°Ð½Ð¸Ðµ
	UVerticalBox* LogoVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (UHorizontalBoxSlot* LSlot = HeaderHBox->AddChildToHorizontalBox(LogoVBox))
	{
		LSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TitleText->SetText(FText::FromString(TEXT("RAILOPTIMA")));
	TitleText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 24));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	LogoVBox->AddChildToVerticalBox(TitleText);

	UTextBlock* SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SubtitleText->SetText(FText::FromString(TEXT("Ð¡ÐÐŸÐ  Ð¶ÐµÐ»ÐµÐ·Ð½Ð¾Ð´Ð¾Ñ€Ð¾Ð¶Ð½Ñ‹Ñ… Ð¿ÑƒÑ‚ÐµÐ¹ â€¢ ÐœÐ¾Ð´ÑƒÐ»ÑŒ 3D-Ñ‚Ñ€Ð°ÑÑÐ¸Ñ€Ð¾Ð²ÐºÐ¸ Ð¸ Ð¾Ð¿Ñ‚Ð¸Ð¼Ð¸Ð·Ð°Ñ†Ð¸Ð¸ Ð¿Ð¾ Ð¡ÐŸ 119.13330")));
	SubtitleText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	SubtitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.68f, 0.78f, 1.0f)));
	LogoVBox->AddChildToVerticalBox(SubtitleText);

	// ÐŸÑ€Ð°Ð²Ð°Ñ Ð¿Ð°Ð½ÐµÐ»ÑŒ: ÑÑ‚Ð°Ñ‚ÑƒÑ Ð¿Ñ€Ð¾Ñ„Ð¸Ð»Ñ Ð¸ Ð¾Ð±Ð»Ð°ÐºÐ°
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
	ProfileText->SetText(FText::FromString(TEXT("ðŸ‘¤ Ð˜Ð½Ð¶ÐµÐ½ÐµÑ€ (Ð›Ð¾ÐºÐ°Ð»ÑŒÐ½Ñ‹Ð¹ Ð¿Ñ€Ð¾Ñ„Ð¸Ð»ÑŒ)")));
	ProfileText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 11));
	ProfileText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.96f, 1.0f)));
	ProfilePill->SetContent(ProfileText);

	USpacer* Spacer1 = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	Spacer1->SetSize(FVector2D(10.0f, 1.0f));
	ProfileHBox->AddChildToHorizontalBox(Spacer1);

	// ÐšÐ½Ð¾Ð¿ÐºÐ° Ð°Ð²Ñ‚Ð¾Ñ€Ð¸Ð·Ð°Ñ†Ð¸Ð¸
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
	AuthBtnText->SetText(FText::FromString(TEXT("â˜ ÐžÐ±Ð»Ð°ÐºÐ¾ / ÐŸÑ€Ð¾Ñ„Ð¸Ð»ÑŒ")));
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

	// â”€â”€â”€ Ð›ÐµÐ²Ð°Ñ ÐºÐ¾Ð»Ð¾Ð½ÐºÐ°: Ð”ÐµÐ¹ÑÑ‚Ð²Ð¸Ñ Ð¸ Ð¸Ð½Ñ„Ð¾Ñ€Ð¼Ð°Ñ†Ð¸Ñ â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	UVerticalBox* LeftCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (UHorizontalBoxSlot* LSlot = ContentHBox->AddChildToHorizontalBox(LeftCol))
	{
		LSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	UBorder* ActionCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	ActionCard->SetBrushColor(FLinearColor(0.02f, 0.04f, 0.07f, 0.90f));
	ActionCard->SetPadding(FMargin(22.0f, 20.0f));
	LeftCol->AddChildToVerticalBox(ActionCard);

	UVerticalBox* ActionVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ActionCard->SetContent(ActionVBox);

	UTextBlock* ActionsTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ActionsTitle->SetText(FText::FromString(TEXT("Ð‘Ð«Ð¡Ð¢Ð Ð«Ð• Ð”Ð•Ð™Ð¡Ð¢Ð’Ð˜Ð¯")));
	ActionsTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
	ActionsTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	if (UVerticalBoxSlot* TSlot = ActionVBox->AddChildToVerticalBox(ActionsTitle))
	{
		TSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	// ÐšÐ½Ð¾Ð¿ÐºÐ° ÑÐ¾Ð·Ð´Ð°Ð½Ð¸Ñ Ð½Ð¾Ð²Ð¾Ð³Ð¾ Ð¿Ñ€Ð¾ÐµÐºÑ‚Ð°
	UButton* NewProjectBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	NewProjectBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.05f, 0.22f, 0.38f, 0.95f),
		FLinearColor(0.08f, 0.35f, 0.58f, 1.0f),
		FLinearColor(0.03f, 0.16f, 0.28f, 1.0f)));
	NewProjectBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleNewProjectClicked);
	if (UVerticalBoxSlot* BtnSlot = ActionVBox->AddChildToVerticalBox(NewProjectBtn))
	{
		BtnSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UBorder* NewProjectInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	NewProjectInner->SetBrushColor(FLinearColor::Transparent);
	NewProjectInner->SetPadding(FMargin(20.0f, 14.0f));
	NewProjectBtn->AddChild(NewProjectInner);

	UTextBlock* NewProjectText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NewProjectText->SetText(FText::FromString(TEXT("+  Ð¡ÐžÐ—Ð”ÐÐ¢Ð¬ ÐÐžÐ’Ð«Ð™ ÐŸÐ ÐžÐ•ÐšÐ¢")));
	NewProjectText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 13));
	NewProjectText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	NewProjectInner->SetContent(NewProjectText);

	// ÐšÐ½Ð¾Ð¿ÐºÐ° Ð¾Ñ‚ÐºÑ€Ñ‹Ñ‚Ð¸Ñ Ð¿Ð°Ð¿ÐºÐ¸ Ð¿Ñ€Ð¾ÐµÐºÑ‚Ð¾Ð²
	UButton* FolderBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	FolderBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.05f, 0.08f, 0.14f, 0.85f),
		FLinearColor(0.10f, 0.16f, 0.26f, 0.95f),
		FLinearColor(0.03f, 0.05f, 0.09f, 1.0f)));
	FolderBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleOpenProjectsFolderClicked);
	if (UVerticalBoxSlot* BtnSlot = ActionVBox->AddChildToVerticalBox(FolderBtn))
	{
		BtnSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
	}

	UBorder* FolderInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	FolderInner->SetBrushColor(FLinearColor::Transparent);
	FolderInner->SetPadding(FMargin(18.0f, 10.0f));
	FolderBtn->AddChild(FolderInner);

	UTextBlock* FolderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FolderText->SetText(FText::FromString(TEXT("ðŸ“  ÐžÑ‚ÐºÑ€Ñ‹Ñ‚ÑŒ Ð¿Ð°Ð¿ÐºÑƒ Ð¿Ñ€Ð¾ÐµÐºÑ‚Ð¾Ð²")));
	FolderText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 11));
	FolderText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f)));
	FolderInner->SetContent(FolderText);

	// ÐšÐ°Ñ€Ñ‚Ð¾Ñ‡ÐºÐ° Ð¸Ð½Ñ‚ÐµÐ³Ñ€Ð°Ñ†Ð¸Ð¸ Ñ OpenTopography
	UBorder* OpenTopoCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	OpenTopoCard->SetBrushColor(FLinearColor(0.03f, 0.07f, 0.12f, 0.75f));
	OpenTopoCard->SetPadding(FMargin(16.0f, 14.0f));
	ActionVBox->AddChildToVerticalBox(OpenTopoCard);

	UVerticalBox* OpenTopoVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	OpenTopoCard->SetContent(OpenTopoVBox);

	UTextBlock* OpenTopoTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OpenTopoTitle->SetText(FText::FromString(TEXT("ðŸŒ Ð’ÐµÐ±-ÑÐµÑ€Ð²Ð¸Ñ Ð¸ OpenTopography")));
	OpenTopoTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	OpenTopoTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	if (UVerticalBoxSlot* SubSlot = OpenTopoVBox->AddChildToVerticalBox(OpenTopoTitle))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	UTextBlock* OpenTopoDesc = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OpenTopoDesc->SetText(FText::FromString(TEXT("Ð’ Ñ€Ð°Ð·Ñ€Ð°Ð±Ð°Ñ‚Ñ‹Ð²Ð°ÐµÐ¼Ð¾Ð¹ Ð²ÐµÐ±-Ð²ÐµÑ€ÑÐ¸Ð¸ Ð¸Ð½Ñ‚ÐµÐ³Ñ€Ð¸Ñ€Ð¾Ð²Ð°Ð½ ÑÐ¿ÑƒÑ‚Ð½Ð¸ÐºÐ¾Ð²Ñ‹Ð¹ Ð²Ñ‹Ð±Ð¾Ñ€ Ð·Ð¾Ð½Ñ‹ OpenTopography. Ð—Ð¾Ð½Ð° Ð²Ñ‹Ð±Ð¸Ñ€Ð°ÐµÑ‚ÑÑ Ð¿Ñ€ÑÐ¼Ð¾ Ð½Ð° Ð¸Ð½Ñ‚ÐµÑ€Ð°ÐºÑ‚Ð¸Ð²Ð½Ð¾Ð¹ ÐºÐ°Ñ€Ñ‚Ðµ Ð² Ð±Ñ€Ð°ÑƒÐ·ÐµÑ€Ðµ Ð¸ Ð°Ð²Ñ‚Ð¾Ð¼Ð°Ñ‚Ð¸Ñ‡ÐµÑÐºÐ¸ Ð¾Ñ‚Ð¿Ñ€Ð°Ð²Ð»ÑÐµÑ‚ÑÑ Ð² 3D-Ð¼Ð¾Ð´ÑƒÐ»ÑŒ.")));
	OpenTopoDesc->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	OpenTopoDesc->SetAutoWrapText(true);
	OpenTopoDesc->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.72f, 0.82f, 1.0f)));
	OpenTopoVBox->AddChildToVerticalBox(OpenTopoDesc);

	// â”€â”€â”€ Ð Ð°Ð·Ð´ÐµÐ»Ð¸Ñ‚ÐµÐ»ÑŒ â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	USpacer* ColSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	ColSpacer->SetSize(FVector2D(28.0f, 1.0f));
	ContentHBox->AddChildToHorizontalBox(ColSpacer);

	// â”€â”€â”€ ÐŸÑ€Ð°Ð²Ð°Ñ ÐºÐ¾Ð»Ð¾Ð½ÐºÐ°: Ð‘Ð¸Ð±Ð»Ð¸Ð¾Ñ‚ÐµÐºÐ° Ð»Ð¾ÐºÐ°Ð»ÑŒÐ½Ñ‹Ñ… Ð¿Ñ€Ð¾ÐµÐºÑ‚Ð¾Ð² â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	UBorder* LibraryCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	LibraryCard->SetBrushColor(FLinearColor(0.02f, 0.04f, 0.07f, 0.90f));
	LibraryCard->SetPadding(FMargin(24.0f, 20.0f));
	if (UHorizontalBoxSlot* RSlot = ContentHBox->AddChildToHorizontalBox(LibraryCard))
	{
		RSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UVerticalBox* LibVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	LibraryCard->SetContent(LibVBox);

	// Ð—Ð°Ð³Ð¾Ð»Ð¾Ð²Ð¾Ðº Ð±Ð¸Ð±Ð»Ð¸Ð¾Ñ‚ÐµÐºÐ¸ Ð¸ ÐºÐ½Ð¾Ð¿ÐºÐ° Ð¾Ð±Ð½Ð¾Ð²Ð»ÐµÐ½Ð¸Ñ
	UHorizontalBox* LibHeaderHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = LibVBox->AddChildToVerticalBox(LibHeaderHBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	UTextBlock* LibTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LibTitle->SetText(FText::FromString(TEXT("Ð‘Ð˜Ð‘Ð›Ð˜ÐžÐ¢Ð•ÐšÐ Ð›ÐžÐšÐÐ›Ð¬ÐÐ«Ð¥ ÐŸÐ ÐžÐ•ÐšÐ¢ÐžÐ’")));
	LibTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 13));
	LibTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	if (UHorizontalBoxSlot* SubSlot = LibHeaderHBox->AddChildToHorizontalBox(LibTitle))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SubSlot->SetVerticalAlignment(VAlign_Center);
	}

	ProjectCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ProjectCountText->SetText(FText::FromString(TEXT("Ð’ÑÐµÐ³Ð¾: 0")));
	ProjectCountText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 11));
	ProjectCountText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.68f, 0.78f, 1.0f)));
	if (UHorizontalBoxSlot* SubSlot = LibHeaderHBox->AddChildToHorizontalBox(ProjectCountText))
	{
		SubSlot->SetVerticalAlignment(VAlign_Center);
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));
	}

	UButton* RefreshBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	RefreshBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.06f, 0.10f, 0.16f, 0.85f),
		FLinearColor(0.12f, 0.20f, 0.32f, 0.95f),
		FLinearColor(0.04f, 0.08f, 0.12f, 1.0f)));
	RefreshBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleRefreshLibraryClicked);
	LibHeaderHBox->AddChildToHorizontalBox(RefreshBtn);

	UBorder* RefreshInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	RefreshInner->SetBrushColor(FLinearColor::Transparent);
	RefreshInner->SetPadding(FMargin(12.0f, 6.0f));
	RefreshBtn->AddChild(RefreshInner);

	UTextBlock* RefreshText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	RefreshText->SetText(FText::FromString(TEXT("â†» ÐžÐ±Ð½Ð¾Ð²Ð¸Ñ‚ÑŒ")));
	RefreshText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 10));
	RefreshText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f)));
	RefreshInner->SetContent(RefreshText);

	// Ð¡Ð¿Ð¸ÑÐ¾Ðº Ð¿Ñ€Ð¾ÐµÐºÑ‚Ð¾Ð² (ScrollBox)
	ProjectListScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ProjectListScrollBox"));
	if (UVerticalBoxSlot* SubSlot = LibVBox->AddChildToVerticalBox(ProjectListScrollBox))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
}

void URailMainMenuWidget::PopulateProjectCard(const FRailProjectMetadata& Meta, UScrollBox* TargetScrollBox)
{
	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Card->SetBrushColor(FLinearColor(0.035f, 0.065f, 0.11f, 0.9f));
	Card->SetPadding(FMargin(18.0f, 14.0f));
	if (UScrollBoxSlot* SubSlot = Cast<UScrollBoxSlot>(TargetScrollBox->AddChild(Card)))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UHorizontalBox* CardHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Card->SetContent(CardHBox);

	// Ð˜Ð½Ñ„Ð¾Ñ€Ð¼Ð°Ñ†Ð¸Ñ Ð¾ Ð¿Ñ€Ð¾ÐµÐºÑ‚Ðµ
	UVerticalBox* InfoVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (UHorizontalBoxSlot* SubSlot = CardHBox->AddChildToHorizontalBox(InfoVBox))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameText->SetText(FText::FromString(Meta.ProjectName));
	NameText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
	NameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	InfoVBox->AddChildToVerticalBox(NameText);

	UHorizontalBox* MetaHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = InfoVBox->AddChildToVerticalBox(MetaHBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
	}

	// ÐšÐ°Ñ‚ÐµÐ³Ð¾Ñ€Ð¸Ñ Ð´Ð¾Ñ€Ð¾Ð³Ð¸
	UTextBlock* CatText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CatText->SetText(FText::FromString(Meta.CategoryName));
	CatText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 11));
	CatText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.85f, 0.35f, 1.0f)));
	if (UHorizontalBoxSlot* SubSlot = MetaHBox->AddChildToHorizontalBox(CatText))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));
	}

	// Ð”Ð°Ñ‚Ð° Ð¸Ð·Ð¼ÐµÐ½ÐµÐ½Ð¸Ñ
	UTextBlock* DateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	DateText->SetText(FText::FromString(Meta.LastModified.ToString(TEXT("%d.%m.%Y %H:%M"))));
	DateText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	DateText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.62f, 0.72f, 1.0f)));
	if (UHorizontalBoxSlot* SubSlot = MetaHBox->AddChildToHorizontalBox(DateText))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));
	}

	// Ð‘ÐµÐ¹Ð´Ð¶Ð¸ ÑÐ»Ð¾ÐµÐ²
	if (Meta.bHasRoads)
	{
		UTextBlock* RoadsBadge = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		RoadsBadge->SetText(FText::FromString(TEXT("â€¢ Ð”Ð¾Ñ€Ð¾Ð³Ð¸")));
		RoadsBadge->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
		RoadsBadge->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
		if (UHorizontalBoxSlot* SubSlot = MetaHBox->AddChildToHorizontalBox(RoadsBadge))
		{
			SubSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		}
	}

	if (Meta.bHasRivers)
	{
		UTextBlock* RiversBadge = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		RiversBadge->SetText(FText::FromString(TEXT("â€¢ Ð ÐµÐºÐ¸")));
		RiversBadge->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
		RiversBadge->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.75f, 1.0f, 1.0f)));
		MetaHBox->AddChildToHorizontalBox(RiversBadge);
	}

	// ÐšÐ½Ð¾Ð¿ÐºÐ¸ Ð´ÐµÐ¹ÑÑ‚Ð²Ð¸Ð¹ ÐºÐ°Ñ€Ñ‚Ð¾Ñ‡ÐºÐ¸
	UHorizontalBox* ActionsHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UHorizontalBoxSlot* SubSlot = CardHBox->AddChildToHorizontalBox(ActionsHBox))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		SubSlot->SetVerticalAlignment(VAlign_Center);
	}

	// ÐšÐ½Ð¾Ð¿ÐºÐ° ÐžÐ¢ÐšÐ Ð«Ð¢Ð¬
	UButton* OpenBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	OpenBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.08f, 0.32f, 0.22f, 0.95f),
		FLinearColor(0.12f, 0.48f, 0.32f, 1.0f),
		FLinearColor(0.05f, 0.22f, 0.15f, 1.0f)));

	URailProjectActionHelper* ActionHelper = NewObject<URailProjectActionHelper>(this);
	ActionHelper->ProjectMeta = Meta;
	ActionHelper->MenuWidget = this;
	ActionHelpers.Add(ActionHelper);

	OpenBtn->OnClicked.AddDynamic(ActionHelper, &URailProjectActionHelper::OnOpenClicked);
	ActionsHBox->AddChildToHorizontalBox(OpenBtn);

	UBorder* OpenInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	OpenInner->SetBrushColor(FLinearColor::Transparent);
	OpenInner->SetPadding(FMargin(18.0f, 8.0f));
	OpenBtn->AddChild(OpenInner);

	UTextBlock* OpenText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OpenText->SetText(FText::FromString(TEXT("ÐžÐ¢ÐšÐ Ð«Ð¢Ð¬")));
	OpenText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	OpenText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.95f, 0.4f, 1.0f)));
	OpenInner->SetContent(OpenText);

	USpacer* BtnSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	BtnSpacer->SetSize(FVector2D(8.0f, 1.0f));
	ActionsHBox->AddChildToHorizontalBox(BtnSpacer);

	// ÐšÐ½Ð¾Ð¿ÐºÐ° Ð’ ÐŸÐÐŸÐšÐ•
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
	ExpText->SetText(FText::FromString(TEXT("ðŸ“")));
	ExpText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	ExpInner->SetContent(ExpText);

	USpacer* BtnSpacer2 = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	BtnSpacer2->SetSize(FVector2D(8.0f, 1.0f));
	ActionsHBox->AddChildToHorizontalBox(BtnSpacer2);

	// ÐšÐ½Ð¾Ð¿ÐºÐ° Ð£Ð”ÐÐ›Ð˜Ð¢Ð¬
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
	DelText->SetText(FText::FromString(TEXT("âœ•")));
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
		ProjectCountText->SetText(FText::FromString(FString::Printf(TEXT("Ð’ÑÐµÐ³Ð¾: %d"), Projects.Num())));
	}

	if (Projects.Num() == 0)
	{
		UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		EmptyText->SetText(FText::FromString(TEXT("Ð’ Ð±Ð¸Ð±Ð»Ð¸Ð¾Ñ‚ÐµÐºÐµ Ð½ÐµÑ‚ Ð¿Ñ€Ð¾ÐµÐºÑ‚Ð¾Ð². ÐÐ°Ð¶Ð¼Ð¸Ñ‚Ðµ '+ Ð¡ÐžÐ—Ð”ÐÐ¢Ð¬ ÐÐžÐ’Ð«Ð™ ÐŸÐ ÐžÐ•ÐšÐ¢' Ð´Ð»Ñ Ð½Ð°Ñ‡Ð°Ð»Ð°.")));
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

// â”€â”€â”€ ÐœÐ¾Ð´Ð°Ð»ÑŒÐ½Ð¾Ðµ Ð¾ÐºÐ½Ð¾ ÑÐ¾Ð·Ð´Ð°Ð½Ð¸Ñ Ð¿Ñ€Ð¾ÐµÐºÑ‚Ð° â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void URailMainMenuWidget::BuildCreateProjectModal(UCanvasPanel* RootCanvas)
{
	CreateProjectModalBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CreateProjectModalBorder"));
	CreateProjectModalBorder->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.03f, 0.98f));
	CreateProjectModalBorder->SetPadding(FMargin(32.0f, 26.0f));

	UCanvasPanelSlot* ModalSlot = RootCanvas->AddChildToCanvas(CreateProjectModalBorder);
	ModalSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
	ModalSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	ModalSlot->SetPosition(FVector2D(0.0f, 0.0f));
	ModalSlot->SetAutoSize(true);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	CreateProjectModalBorder->SetContent(VBox);

	// Ð—Ð°Ð³Ð¾Ð»Ð¾Ð²Ð¾Ðº Ð¼Ð¾Ð´Ð°Ð»ÐºÐ¸
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("Ð¡ÐžÐ—Ð”ÐÐÐ˜Ð• ÐÐžÐ’ÐžÐ“Ðž ÐŸÐ ÐžÐ•ÐšÐ¢Ð")));
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(Title))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	// 1. ÐÐ°Ð·Ð²Ð°Ð½Ð¸Ðµ Ð¿Ñ€Ð¾ÐµÐºÑ‚Ð°
	UTextBlock* NameLbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameLbl->SetText(FText::FromString(TEXT("ÐÐ°Ð·Ð²Ð°Ð½Ð¸Ðµ Ð¿Ñ€Ð¾ÐµÐºÑ‚Ð°:")));
	NameLbl->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 11));
	NameLbl->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f)));
	VBox->AddChildToVerticalBox(NameLbl);

	ProjectNameInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	ProjectNameInput->SetText(FText::FromString(FString::Printf(TEXT("Ð£Ñ‡Ð°ÑÑ‚Ð¾Ðº_%s"), *FDateTime::Now().ToString(TEXT("%d%m%Y")))));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(ProjectNameInput))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 14.0f));
	}

	// 2. ÐšÐ°Ñ‚ÐµÐ³Ð¾Ñ€Ð¸Ñ Ð´Ð¾Ñ€Ð¾Ð³Ð¸
	UTextBlock* CatLbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CatLbl->SetText(FText::FromString(TEXT("ÐšÐ°Ñ‚ÐµÐ³Ð¾Ñ€Ð¸Ñ Ð»Ð¸Ð½Ð¸Ð¸ (Ð¡ÐŸ 119.13330):")));
	CatLbl->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 11));
	CatLbl->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f)));
	VBox->AddChildToVerticalBox(CatLbl);

	CategoryComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
	CategoryComboBox->AddOption(TEXT("Ð¡ÐºÐ¾Ñ€Ð¾ÑÑ‚Ð½Ð°Ñ"));
	CategoryComboBox->AddOption(TEXT("ÐŸÐ°ÑÑÐ°Ð¶Ð¸Ñ€ÑÐºÐ°Ñ"));
	CategoryComboBox->AddOption(TEXT("ÐžÑÐ¾Ð±Ð¾Ð³Ñ€ÑƒÐ·Ð¾Ð½Ð°Ð¿Ñ€ÑÐ¶ÐµÐ½Ð½Ð°Ñ"));
	CategoryComboBox->AddOption(TEXT("I ÐšÐ°Ñ‚ÐµÐ³Ð¾Ñ€Ð¸Ñ"));
	CategoryComboBox->AddOption(TEXT("II ÐšÐ°Ñ‚ÐµÐ³Ð¾Ñ€Ð¸Ñ"));
	CategoryComboBox->AddOption(TEXT("III ÐšÐ°Ñ‚ÐµÐ³Ð¾Ñ€Ð¸Ñ"));
	CategoryComboBox->AddOption(TEXT("IV (Ð–ÐµÐ»ÐµÐ·Ð½Ð¾Ð´Ð¾Ñ€Ð¾Ð¶Ð½Ñ‹Ðµ Ð»Ð¸Ð½Ð¸Ð¸)"));
	CategoryComboBox->AddOption(TEXT("IV (ÐŸÐ¾Ð´ÑŠÐµÐ·Ð´Ð½Ñ‹Ðµ Ð¿ÑƒÑ‚Ð¸)"));
	CategoryComboBox->SetSelectedOption(TEXT("I ÐšÐ°Ñ‚ÐµÐ³Ð¾Ñ€Ð¸Ñ"));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(CategoryComboBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 14.0f));
	}

	// 3. Ð¢Ð¸Ð¿ Ð¸ÑÑ…Ð¾Ð´Ð½Ñ‹Ñ… Ð´Ð°Ð½Ð½Ñ‹Ñ…
	UTextBlock* TypeLbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TypeLbl->SetText(FText::FromString(TEXT("Ð¤Ð¾Ñ€Ð¼Ð°Ñ‚ Ð´Ð°Ð½Ð½Ñ‹Ñ… Ñ€ÐµÐ»ÑŒÐµÑ„Ð°:")));
	TypeLbl->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 11));
	TypeLbl->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f)));
	VBox->AddChildToVerticalBox(TypeLbl);

	UHorizontalBox* TypeHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(TypeHBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 12.0f));
	}

	PngTypeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	PngTypeButton->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleSourceTypeToggledPng);
	TypeHBox->AddChildToHorizontalBox(PngTypeButton);

	UBorder* PngInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	PngInner->SetBrushColor(FLinearColor::Transparent);
	PngInner->SetPadding(FMargin(14.0f, 6.0f));
	PngTypeButton->AddChild(PngInner);

	PngTypeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	PngTypeText->SetText(FText::FromString(TEXT("â—  16-Ð±Ð¸Ñ‚Ð½Ñ‹Ð¹ PNG")));
	PngTypeText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
	PngInner->SetContent(PngTypeText);

	USpacer* SpType = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	SpType->SetSize(FVector2D(10.0f, 1.0f));
	TypeHBox->AddChildToHorizontalBox(SpType);

	TifTypeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	TifTypeButton->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleSourceTypeToggledTif);
	TypeHBox->AddChildToHorizontalBox(TifTypeButton);

	UBorder* TifInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	TifInner->SetBrushColor(FLinearColor::Transparent);
	TifInner->SetPadding(FMargin(14.0f, 6.0f));
	TifTypeButton->AddChild(TifInner);

	TifTypeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TifTypeText->SetText(FText::FromString(TEXT("â—‹  GeoTIFF (.tif)")));
	TifTypeText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	TifInner->SetContent(TifTypeText);

	HandleSourceTypeToggledPng(); // ÐŸÐ¾ ÑƒÐ¼Ð¾Ð»Ñ‡Ð°Ð½Ð¸ÑŽ Ð²Ñ‹Ð±Ñ€Ð°Ð½ PNG

	// 4. ÐŸÐ¾Ð»Ðµ Ñ„Ð°Ð¹Ð»Ð° + ÐšÐ½Ð¾Ð¿ÐºÐ° ÐžÐ±Ð·Ð¾Ñ€
	UTextBlock* FileLbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FileLbl->SetText(FText::FromString(TEXT("Ð¤Ð°Ð¹Ð» ÐºÐ°Ñ€Ñ‚Ñ‹ Ð²Ñ‹ÑÐ¾Ñ‚:")));
	FileLbl->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 11));
	FileLbl->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f)));
	VBox->AddChildToVerticalBox(FileLbl);

	UHorizontalBox* FileHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(FileHBox))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 14.0f));
	}

	FilePathInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	const FString DefaultPng = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("RawContent/heightmap.png"));
	FilePathInput->SetText(FText::FromString(DefaultPng));
	if (UHorizontalBoxSlot* SubSlot = FileHBox->AddChildToHorizontalBox(FilePathInput))
	{
		SubSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	USpacer* SpFile = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	SpFile->SetSize(FVector2D(8.0f, 1.0f));
	FileHBox->AddChildToHorizontalBox(SpFile);

	UButton* BrowseBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	BrowseBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.08f, 0.16f, 0.28f, 0.9f),
		FLinearColor(0.14f, 0.26f, 0.44f, 1.0f),
		FLinearColor(0.05f, 0.10f, 0.18f, 1.0f)));
	BrowseBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleBrowseFileClicked);
	FileHBox->AddChildToHorizontalBox(BrowseBtn);

	UBorder* BrowseInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	BrowseInner->SetBrushColor(FLinearColor::Transparent);
	BrowseInner->SetPadding(FMargin(14.0f, 6.0f));
	BrowseBtn->AddChild(BrowseInner);

	UTextBlock* BrowseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BrowseText->SetText(FText::FromString(TEXT("ÐžÐ±Ð·Ð¾Ñ€...")));
	BrowseText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
	BrowseText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	BrowseInner->SetContent(BrowseText);

	// 5. Ð§ÐµÐºÐ±Ð¾ÐºÑÑ‹ ÑÐ»Ð¾ÐµÐ² Ð¸Ð½Ñ„Ñ€Ð°ÑÑ‚Ñ€ÑƒÐºÑ‚ÑƒÑ€Ñ‹
	UTextBlock* LayersLbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LayersLbl->SetText(FText::FromString(TEXT("Ð¡Ð»Ð¾Ð¸ Ð¸Ð½Ñ„Ñ€Ð°ÑÑ‚Ñ€ÑƒÐºÑ‚ÑƒÑ€Ñ‹ OpenStreetMap:")));
	LayersLbl->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 11));
	LayersLbl->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f)));
	VBox->AddChildToVerticalBox(LayersLbl);

	UHorizontalBox* CheckHBox1 = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(CheckHBox1))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 4.0f));
	}

	RoadsCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
	RoadsCheckBox->SetIsChecked(true);
	CheckHBox1->AddChildToHorizontalBox(RoadsCheckBox);

	UTextBlock* RoadsLbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	RoadsLbl->SetText(FText::FromString(TEXT("  Ð—Ð°Ð³Ñ€ÑƒÐ·Ð¸Ñ‚ÑŒ Ð´Ð¾Ñ€Ð¾Ð¶Ð½ÑƒÑŽ ÑÐµÑ‚ÑŒ (OSM roads)")));
	RoadsLbl->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	RoadsLbl->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CheckHBox1->AddChildToHorizontalBox(RoadsLbl);

	UHorizontalBox* CheckHBox2 = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(CheckHBox2))
	{
		SubSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 20.0f));
	}

	RiversCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
	RiversCheckBox->SetIsChecked(true);
	CheckHBox2->AddChildToHorizontalBox(RiversCheckBox);

	UTextBlock* RiversLbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	RiversLbl->SetText(FText::FromString(TEXT("  Ð—Ð°Ð³Ñ€ÑƒÐ·Ð¸Ñ‚ÑŒ Ð³Ð¸Ð´Ñ€Ð¾Ð³Ñ€Ð°Ñ„Ð¸ÑŽ Ð¸ Ñ€ÐµÐºÐ¸ (OSM rivers)")));
	RiversLbl->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	RiversLbl->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CheckHBox2->AddChildToHorizontalBox(RiversLbl);

	// 6. ÐšÐ½Ð¾Ð¿ÐºÐ¸ Â«Ð¡Ð¾Ð·Ð´Ð°Ñ‚ÑŒÂ» Ð¸ Â«ÐžÑ‚Ð¼ÐµÐ½Ð°Â»
	UHorizontalBox* ButtonsHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	VBox->AddChildToVerticalBox(ButtonsHBox);

	UButton* ConfirmBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	ConfirmBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.05f, 0.28f, 0.48f, 0.95f),
		FLinearColor(0.08f, 0.40f, 0.68f, 1.0f),
		FLinearColor(0.03f, 0.20f, 0.35f, 1.0f)));
	ConfirmBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleCreateProjectConfirmClicked);
	ButtonsHBox->AddChildToHorizontalBox(ConfirmBtn);

	UBorder* ConfInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	ConfInner->SetBrushColor(FLinearColor::Transparent);
	ConfInner->SetPadding(FMargin(22.0f, 10.0f));
	ConfirmBtn->AddChild(ConfInner);

	UTextBlock* ConfText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ConfText->SetText(FText::FromString(TEXT("Ð¡ÐžÐ—Ð”ÐÐ¢Ð¬ Ð˜ ÐžÐ¢ÐšÐ Ð«Ð¢Ð¬")));
	ConfText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
	ConfText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ConfInner->SetContent(ConfText);

	USpacer* SpModal = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	SpModal->SetSize(FVector2D(12.0f, 1.0f));
	ButtonsHBox->AddChildToHorizontalBox(SpModal);

	UButton* CancelBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CancelBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.08f, 0.10f, 0.14f, 0.85f),
		FLinearColor(0.14f, 0.18f, 0.24f, 0.95f),
		FLinearColor(0.05f, 0.07f, 0.10f, 1.0f)));
	CancelBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleCreateProjectCancelClicked);
	ButtonsHBox->AddChildToHorizontalBox(CancelBtn);

	UBorder* CancInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	CancInner->SetBrushColor(FLinearColor::Transparent);
	CancInner->SetPadding(FMargin(18.0f, 10.0f));
	CancelBtn->AddChild(CancInner);

	UTextBlock* CancText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CancText->SetText(FText::FromString(TEXT("ÐžÑ‚Ð¼ÐµÐ½Ð°")));
	CancText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 11));
	CancText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.75f, 0.85f, 1.0f)));
	CancInner->SetContent(CancText);

	CreateProjectModalBorder->SetVisibility(ESlateVisibility::Collapsed);
}

// â”€â”€â”€ ÐœÐ¾Ð´Ð°Ð»ÑŒÐ½Ð¾Ðµ Ð¾ÐºÐ½Ð¾ Ð°Ð²Ñ‚Ð¾Ñ€Ð¸Ð·Ð°Ñ†Ð¸Ð¸ Ð¸ Ð¾Ð±Ð»Ð°ÐºÐ° â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void URailMainMenuWidget::BuildAuthModal(UCanvasPanel* RootCanvas)
{
	AuthModalBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AuthModalBorder"));
	AuthModalBorder->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.035f, 0.98f));
	AuthModalBorder->SetPadding(FMargin(32.0f, 26.0f));

	UCanvasPanelSlot* ModalSlot = RootCanvas->AddChildToCanvas(AuthModalBorder);
	ModalSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
	ModalSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	ModalSlot->SetPosition(FVector2D(0.0f, 0.0f));
	ModalSlot->SetAutoSize(true);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	AuthModalBorder->SetContent(VBox);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("ÐžÐ‘Ð›ÐÐ§ÐÐÐ¯ ÐŸÐ›ÐÐ¢Ð¤ÐžÐ ÐœÐ Ð˜ ÐÐ’Ð¢ÐžÐ Ð˜Ð—ÐÐ¦Ð˜Ð¯")));
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 15));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(Title))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	UBorder* InfoCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	InfoCard->SetBrushColor(FLinearColor(0.04f, 0.08f, 0.14f, 0.8f));
	InfoCard->SetPadding(FMargin(16.0f, 12.0f));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(InfoCard))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	UTextBlock* InfoText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	InfoText->SetText(FText::FromString(TEXT("Ð’ Ð°Ñ€Ñ…Ð¸Ñ‚ÐµÐºÑ‚ÑƒÑ€Ðµ RailOptima Ð¿Ñ€ÐµÐ´ÑƒÑÐ¼Ð¾Ñ‚Ñ€ÐµÐ½Ð° Ð²ÐµÐ±-Ð¿Ð»Ð°Ñ‚Ñ„Ð¾Ñ€Ð¼Ð° Ñ Ð»Ð¸Ñ‡Ð½Ñ‹Ð¼ ÐºÐ°Ð±Ð¸Ð½ÐµÑ‚Ð¾Ð¼ Ð¸Ð½Ð¶ÐµÐ½ÐµÑ€Ð°. ÐÐ° ÑÐ°Ð¹Ñ‚Ðµ Ð¼Ð¾Ð¶Ð½Ð¾ Ð²Ñ‹Ð±Ñ€Ð°Ñ‚ÑŒ Ð¿Ð¾Ð»Ð¸Ð³Ð¾Ð½ Ð² OpenTopography Ð½Ð° Ð¸Ð½Ñ‚ÐµÑ€Ð°ÐºÑ‚Ð¸Ð²Ð½Ð¾Ð¹ ÑÐ¿ÑƒÑ‚Ð½Ð¸ÐºÐ¾Ð²Ð¾Ð¹ ÐºÐ°Ñ€Ñ‚Ðµ, ÑÐ¾Ñ…Ñ€Ð°Ð½Ð¸Ñ‚ÑŒ Ð² Ð¾Ð±Ð»Ð°ÐºÐ¾ Ð¸ Ð±ÐµÑÑˆÐ¾Ð²Ð½Ð¾ Ð¾Ñ‚ÐºÑ€Ñ‹Ñ‚ÑŒ Ð¿Ñ€Ð¾ÐµÐºÑ‚ Ð² 3D-Ð¿Ñ€Ð¸Ð»Ð¾Ð¶ÐµÐ½Ð¸Ð¸.")));
	InfoText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	InfoText->SetAutoWrapText(true);
	InfoText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.80f, 0.90f, 1.0f)));
	InfoCard->SetContent(InfoText);

	UTextBlock* EmailLbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	EmailLbl->SetText(FText::FromString(TEXT("Ð›Ð¾Ð³Ð¸Ð½ / Email Ð¸Ð½Ð¶ÐµÐ½ÐµÑ€Ð°:")));
	EmailLbl->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 10));
	EmailLbl->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f)));
	VBox->AddChildToVerticalBox(EmailLbl);

	AuthEmailInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	AuthEmailInput->SetText(FText::FromString(TEXT("engineer@railoptima.ru")));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(AuthEmailInput))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 12.0f));
	}

	UTextBlock* PassLbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	PassLbl->SetText(FText::FromString(TEXT("ÐŸÐ°Ñ€Ð¾Ð»ÑŒ Ð´Ð¾ÑÑ‚ÑƒÐ¿Ð°:")));
	PassLbl->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 10));
	PassLbl->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f)));
	VBox->AddChildToVerticalBox(PassLbl);

	AuthPassInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	AuthPassInput->SetIsPassword(true);
	AuthPassInput->SetText(FText::FromString(TEXT("â€¢â€¢â€¢â€¢â€¢â€¢â€¢â€¢")));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(AuthPassInput))
	{
		SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 18.0f));
	}

	UHorizontalBox* BtnHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	VBox->AddChildToVerticalBox(BtnHBox);

	UButton* LoginBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	LoginBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.08f, 0.32f, 0.22f, 0.95f),
		FLinearColor(0.12f, 0.48f, 0.32f, 1.0f),
		FLinearColor(0.05f, 0.22f, 0.15f, 1.0f)));
	LoginBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleAuthSubmitClicked);
	BtnHBox->AddChildToHorizontalBox(LoginBtn);

	UBorder* LogInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	LogInner->SetBrushColor(FLinearColor::Transparent);
	LogInner->SetPadding(FMargin(20.0f, 8.0f));
	LoginBtn->AddChild(LogInner);

	UTextBlock* LogText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LogText->SetText(FText::FromString(TEXT("Ð’Ð¾Ð¹Ñ‚Ð¸ Ð² Ð°ÐºÐºÐ°ÑƒÐ½Ñ‚")));
	LogText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
	LogText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.95f, 0.4f, 1.0f)));
	LogInner->SetContent(LogText);

	USpacer* SpAuth = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	SpAuth->SetSize(FVector2D(10.0f, 1.0f));
	BtnHBox->AddChildToHorizontalBox(SpAuth);

	UButton* CloseBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CloseBtn->SetStyle(MakeGlassButtonStyle(
		FLinearColor(0.08f, 0.10f, 0.14f, 0.85f),
		FLinearColor(0.14f, 0.18f, 0.24f, 0.95f),
		FLinearColor(0.05f, 0.07f, 0.10f, 1.0f)));
	CloseBtn->OnClicked.AddDynamic(this, &URailMainMenuWidget::HandleAuthCloseClicked);
	BtnHBox->AddChildToHorizontalBox(CloseBtn);

	UBorder* ClInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	ClInner->SetBrushColor(FLinearColor::Transparent);
	ClInner->SetPadding(FMargin(18.0f, 8.0f));
	CloseBtn->AddChild(ClInner);

	UTextBlock* ClText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ClText->SetText(FText::FromString(TEXT("Ð—Ð°ÐºÑ€Ñ‹Ñ‚ÑŒ")));
	ClText->SetFont(FCoreStyle::GetDefaultFontStyle("Medium", 10));
	ClText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.75f, 0.85f, 1.0f)));
	ClInner->SetContent(ClText);

	AuthModalBorder->SetVisibility(ESlateVisibility::Collapsed);
}

// â”€â”€â”€ ÐžÐ²ÐµÑ€Ð»ÐµÐ¹ Ð·Ð°Ð³Ñ€ÑƒÐ·ÐºÐ¸ â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void URailMainMenuWidget::BuildLoadingOverlay(UCanvasPanel* RootCanvas)
{
	LoadingOverlayBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LoadingOverlayBorder"));
	LoadingOverlayBorder->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.025f, 0.96f));
	LoadingOverlayBorder->SetPadding(FMargin(40.0f));

	UCanvasPanelSlot* LoadSlot = RootCanvas->AddChildToCanvas(LoadingOverlayBorder);
	LoadSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	LoadSlot->SetOffsets(FMargin(0.0f));

	UBorder* CenterCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	CenterCard->SetBrushColor(FLinearColor(0.03f, 0.06f, 0.10f, 0.95f));
	CenterCard->SetPadding(FMargin(36.0f, 28.0f));

	UCanvasPanel* SubPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	LoadingOverlayBorder->SetContent(SubPanel);

	UCanvasPanelSlot* CenterSlot = SubPanel->AddChildToCanvas(CenterCard);
	CenterSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
	CenterSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CenterSlot->SetPosition(FVector2D(0.0f, 0.0f));
	CenterSlot->SetAutoSize(true);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	CenterCard->SetContent(VBox);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("Ð—ÐÐ“Ð Ð£Ð—ÐšÐ Ð˜ ÐŸÐžÐ”Ð“ÐžÐ¢ÐžÐ’ÐšÐ ÐŸÐ ÐžÐ•ÐšÐ¢Ð...")));
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	if (UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(Title))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	LoadingStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LoadingStatusText->SetText(FText::FromString(TEXT("ÐŸÐ¾Ð¶Ð°Ð»ÑƒÐ¹ÑÑ‚Ð°, Ð¿Ð¾Ð´Ð¾Ð¶Ð´Ð¸Ñ‚Ðµ. Ð˜Ð´Ñ‘Ñ‚ Ð¿Ð¾ÑÑ‚Ñ€Ð¾ÐµÐ½Ð¸Ðµ Ð³ÐµÐ¾Ð¼ÐµÑ‚Ñ€Ð¸Ð¸ Ñ€ÐµÐ»ÑŒÐµÑ„Ð° Ð¸ Ñ€Ð°ÑÑ‡Ñ‘Ñ‚Ð½Ð¾Ð¹ Ð¼Ð¾Ð´ÐµÐ»Ð¸ A*.")));
	LoadingStatusText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	LoadingStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f)));
	VBox->AddChildToVerticalBox(LoadingStatusText);

	LoadingOverlayBorder->SetVisibility(ESlateVisibility::Collapsed);
}

void URailMainMenuWidget::ShowLoadingOverlay(const FString& InStatusText)
{
	if (LoadingStatusText && !InStatusText.IsEmpty())
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

// â”€â”€â”€ ÐžÐ±Ñ€Ð°Ð±Ð¾Ñ‚Ñ‡Ð¸ÐºÐ¸ Ð´ÐµÐ¹ÑÑ‚Ð²Ð¸Ð¹ Ð´Ð°ÑˆÐ±Ð¾Ñ€Ð´Ð° â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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
		AuthModalBorder->SetVisibility(ESlateVisibility::Visible);
	}
}

void URailMainMenuWidget::HandleRefreshLibraryClicked()
{
	RefreshProjectLibrary();
	ShowToast(TEXT("Ð¡Ð¿Ð¸ÑÐ¾Ðº Ð»Ð¾ÐºÐ°Ð»ÑŒÐ½Ñ‹Ñ… Ð¿Ñ€Ð¾ÐµÐºÑ‚Ð¾Ð² Ð¾Ð±Ð½Ð¾Ð²Ð»Ñ‘Ð½."), ERailToastType::Info);
}

void URailMainMenuWidget::HandleSourceTypeToggledPng()
{
	bSourceIsGeoTiff = false;
	if (PngTypeText && TifTypeText)
	{
		PngTypeText->SetText(FText::FromString(TEXT("â—  16-Ð±Ð¸Ñ‚Ð½Ñ‹Ð¹ PNG")));
		PngTypeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
		TifTypeText->SetText(FText::FromString(TEXT("â—‹  GeoTIFF (.tif)")));
		TifTypeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.65f, 0.75f, 1.0f)));
	}
}

void URailMainMenuWidget::HandleSourceTypeToggledTif()
{
	bSourceIsGeoTiff = true;
	if (PngTypeText && TifTypeText)
	{
		PngTypeText->SetText(FText::FromString(TEXT("â—‹  16-Ð±Ð¸Ñ‚Ð½Ñ‹Ð¹ PNG")));
		PngTypeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.65f, 0.75f, 1.0f)));
		TifTypeText->SetText(FText::FromString(TEXT("â—  GeoTIFF (.tif)")));
		TifTypeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
	}
}

void URailMainMenuWidget::HandleBrowseFileClicked()
{
	const FString Title = bSourceIsGeoTiff ? TEXT("Ð’Ñ‹Ð±ÐµÑ€Ð¸Ñ‚Ðµ GeoTIFF ÐºÐ°Ñ€Ñ‚Ñƒ Ð²Ñ‹ÑÐ¾Ñ‚") : TEXT("Ð’Ñ‹Ð±ÐµÑ€Ð¸Ñ‚Ðµ 16-Ð±Ð¸Ñ‚Ð½ÑƒÑŽ PNG ÐºÐ°Ñ€Ñ‚Ñƒ Ð²Ñ‹ÑÐ¾Ñ‚");
	const FString Filter = bSourceIsGeoTiff
		? TEXT("GeoTIFF (*.tif;*.tiff)|*.tif;*.tiff|Ð’ÑÐµ Ñ„Ð°Ð¹Ð»Ñ‹ (*.*)|*.*")
		: TEXT("PNG ÐšÐ°Ñ€Ñ‚Ñ‹ Ð²Ñ‹ÑÐ¾Ñ‚ (*.png)|*.png|Ð’ÑÐµ Ñ„Ð°Ð¹Ð»Ñ‹ (*.*)|*.*");

	FString Selected;
	if (URailProjectManager::OpenFileDialogWindows(Title, Filter, Selected))
	{
		if (FilePathInput)
		{
			FilePathInput->SetText(FText::FromString(Selected));
		}
	}
}

void URailMainMenuWidget::HandleCreateProjectConfirmClicked()
{
	const FString ProjectName = ProjectNameInput ? ProjectNameInput->GetText().ToString().TrimStartAndEnd() : TEXT("");
	const FString FilePath = FilePathInput ? FilePathInput->GetText().ToString().TrimStartAndEnd() : TEXT("");

	if (ProjectName.IsEmpty())
	{
		ShowToast(TEXT("Ð’Ð²ÐµÐ´Ð¸Ñ‚Ðµ Ð½Ð°Ð·Ð²Ð°Ð½Ð¸Ðµ Ð¿Ñ€Ð¾ÐµÐºÑ‚Ð°!"), ERailToastType::Warning);
		return;
	}

	if (FilePath.IsEmpty() || !IFileManager::Get().FileExists(*FilePath))
	{
		ShowToast(TEXT("Ð£ÐºÐ°Ð·Ð°Ð½Ð½Ñ‹Ð¹ Ñ„Ð°Ð¹Ð» ÐºÐ°Ñ€Ñ‚Ñ‹ Ð²Ñ‹ÑÐ¾Ñ‚ Ð½Ðµ Ð½Ð°Ð¹Ð´ÐµÐ½ Ð½Ð° Ð´Ð¸ÑÐºÐµ!"), ERailToastType::Error);
		return;
	}

	const bool bRoads = RoadsCheckBox ? RoadsCheckBox->IsChecked() : false;
	const bool bRivers = RiversCheckBox ? RiversCheckBox->IsChecked() : false;

	ERailRoadCategory SelectedCat = ERailRoadCategory::Category1;
	if (CategoryComboBox)
	{
		const FString CatStr = CategoryComboBox->GetSelectedOption();
		if (CatStr.Contains(TEXT("Ð¡ÐºÐ¾Ñ€Ð¾ÑÑ‚Ð½Ð°Ñ")))
			SelectedCat = ERailRoadCategory::HighSpeed;
		else if (CatStr.Contains(TEXT("ÐŸÐ°ÑÑÐ°Ð¶Ð¸Ñ€ÑÐºÐ°Ñ")))
			SelectedCat = ERailRoadCategory::Passenger;
		else if (CatStr.Contains(TEXT("ÐžÑÐ¾Ð±Ð¾Ð³Ñ€ÑƒÐ·")))
			SelectedCat = ERailRoadCategory::SuperHeavyHaul;
		else if (CatStr.Contains(TEXT("I ÐšÐ°Ñ‚ÐµÐ³Ð¾Ñ€Ð¸Ñ")))
			SelectedCat = ERailRoadCategory::Category1;
		else if (CatStr.Contains(TEXT("II ÐšÐ°Ñ‚ÐµÐ³Ð¾Ñ€Ð¸Ñ")))
			SelectedCat = ERailRoadCategory::Category2;
		else if (CatStr.Contains(TEXT("III ÐšÐ°Ñ‚ÐµÐ³Ð¾Ñ€Ð¸Ñ")))
			SelectedCat = ERailRoadCategory::Category3;
		else if (CatStr.Contains(TEXT("Ð›Ð¸Ð½Ð¸Ð¸")))
			SelectedCat = ERailRoadCategory::Category4Lines;
		else if (CatStr.Contains(TEXT("ÐŸÐ¾Ð´ÑŠÐµÐ·Ð´Ð½Ñ‹Ðµ")))
			SelectedCat = ERailRoadCategory::Category4Access;
	}

	ShowLoadingOverlay(TEXT("ÐŸÐ¾Ð´Ð³Ð¾Ñ‚Ð¾Ð²ÐºÐ° Ð¿Ñ€Ð¾ÐµÐºÑ‚Ð° Ð¸ Ð¾Ð±Ñ€Ð°Ð±Ð¾Ñ‚ÐºÐ° Ñ€ÐµÐ»ÑŒÐµÑ„Ð°..."));

	FRailProjectMetadata CreatedProject;
	FString ErrorMsg;
	bool bOk = false;

	if (bSourceIsGeoTiff)
	{
		bOk = URailProjectManager::CreateProjectFromGeoTiff(
			ProjectName, SelectedCat, FilePath, bRoads, bRivers, CreatedProject, ErrorMsg);
	}
	else
	{
		bOk = URailProjectManager::CreateProjectFromPng(
			ProjectName, SelectedCat, FilePath, bRoads, bRivers, CreatedProject, ErrorMsg);
	}

	HideLoadingOverlay();

	if (bOk)
	{
		CreateProjectModalBorder->SetVisibility(ESlateVisibility::Collapsed);
		RefreshProjectLibrary();
		ShowToast(FString::Printf(TEXT("ÐŸÑ€Ð¾ÐµÐºÑ‚ '%s' ÑƒÑÐ¿ÐµÑˆÐ½Ð¾ ÑÐ¾Ð·Ð´Ð°Ð½!"), *ProjectName), ERailToastType::Success);
		OnProjectOpened.Broadcast(CreatedProject);
	}
	else
	{
		ShowToast(ErrorMsg.IsEmpty() ? TEXT("ÐžÑˆÐ¸Ð±ÐºÐ° Ð¿Ñ€Ð¸ ÑÐ¾Ð·Ð´Ð°Ð½Ð¸Ð¸ Ð¿Ñ€Ð¾ÐµÐºÑ‚Ð°.") : ErrorMsg, ERailToastType::Error);
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
	ShowToast(TEXT("ÐÐ²Ñ‚Ð¾Ñ€Ð¸Ð·Ð°Ñ†Ð¸Ñ ÑƒÑÐ¿ÐµÑˆÐ½Ð°. ÐžÐ±Ð»Ð°Ñ‡Ð½Ð°Ñ ÑÐ¸Ð½Ñ…Ñ€Ð¾Ð½Ð¸Ð·Ð°Ñ†Ð¸Ñ Ð°ÐºÑ‚Ð¸Ð²Ð¸Ñ€Ð¾Ð²Ð°Ð½Ð° (Ñ€ÐµÐ¶Ð¸Ð¼ Ð¿Ñ€ÐµÐ´Ð¿Ñ€Ð¾ÑÐ¼Ð¾Ñ‚Ñ€Ð°)."), ERailToastType::Success);
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
