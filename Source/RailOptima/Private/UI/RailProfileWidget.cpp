// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Виджет продольного профиля трассы (проекция перепада высот)

#include "UI/RailProfileWidget.h"
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
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

// ─────────────────────────────────────────────────────────────────
//  SRailProfileSlateCanvas
// ─────────────────────────────────────────────────────────────────

void SRailProfileSlateCanvas::Construct(const FArguments& InArgs)
{
	SetCanTick(false);
}

void SRailProfileSlateCanvas::SetProfileData(
	const TArray<FRailProfilePoint>& InPoints,
	float InMinHeight_m,
	float InMaxHeight_m,
	float InTotalLength_m,
	float InMaxSlopePermille)
{
	ProfilePoints = InPoints;
	MinHeight_m = InMinHeight_m;
	MaxHeight_m = InMaxHeight_m;
	TotalLength_m = InTotalLength_m;
	MaxSlopePermille = InMaxSlopePermille;
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 SRailProfileSlateCanvas::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
	const FSlateFontInfo FontSmall = FCoreStyle::GetDefaultFontStyle("Regular", 9);

	// 1. Темный глубокий фон графика
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2D::ZeroVector)),
		WhiteBrush,
		ESlateDrawEffect::None,
		FLinearColor(0.012f, 0.018f, 0.032f, 0.98f));
	LayerId++;

	if (ProfilePoints.Num() < 2 || TotalLength_m < 1.0f)
	{
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2D(Size.X * 0.15f, Size.Y * 0.42f))),
			TEXT("Маршрут не рассчитан. Переместите контрольные точки на карте."),
			FontSmall,
			ESlateDrawEffect::None,
			FLinearColor(0.55f, 0.65f, 0.78f, 0.9f));
		return LayerId + 1;
	}

	const float MarginL = 52.0f;
	const float MarginR = 18.0f;
	const float MarginT = 14.0f;
	const float MarginB = 26.0f;

	const float PlotX = MarginL;
	const float PlotY = MarginT;
	const float PlotW = FMath::Max(10.0f, Size.X - MarginL - MarginR);
	const float PlotH = FMath::Max(10.0f, Size.Y - MarginT - MarginB);

	const float RawRangeH = FMath::Max(MaxHeight_m - MinHeight_m, 10.0f);
	const float DispMinH = MinHeight_m - RawRangeH * 0.08f;
	const float DispMaxH = MaxHeight_m + RawRangeH * 0.08f;
	const float DispRangeH = FMath::Max(DispMaxH - DispMinH, 1.0f);

	// 2. Горизонтальная сетка по оси высот Y
	const int32 NumHorizLines = 4;
	for (int32 i = 0; i <= NumHorizLines; ++i)
	{
		const float Alpha = static_cast<float>(i) / static_cast<float>(NumHorizLines);
		const float HeightVal = FMath::Lerp(DispMinH, DispMaxH, Alpha);
		const float ScreenY = PlotY + (1.0f - Alpha) * PlotH;

		TArray<FVector2D> GridLine;
		GridLine.Add(FVector2D(PlotX, ScreenY));
		GridLine.Add(FVector2D(PlotX + PlotW, ScreenY));

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			GridLine,
			ESlateDrawEffect::None,
			FLinearColor(0.10f, 0.16f, 0.26f, 0.45f),
			true,
			1.0f);

		const FString LabelH = FString::Printf(TEXT("%.0f м"), HeightVal);
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2D(6.0f, ScreenY - 6.0f))),
			LabelH,
			FontSmall,
			ESlateDrawEffect::None,
			FLinearColor(0.52f, 0.65f, 0.80f, 0.9f));
	}

	// 3. Вертикальная сетка по дистанции X
	const float TotalKm = TotalLength_m * 0.001f;
	const int32 NumVertLines = FMath::Clamp(FMath::RoundToInt(TotalKm / 2.0f), 2, 7);
	for (int32 i = 0; i <= NumVertLines; ++i)
	{
		const float Alpha = static_cast<float>(i) / static_cast<float>(NumVertLines);
		const float ScreenX = PlotX + Alpha * PlotW;

		TArray<FVector2D> GridLine;
		GridLine.Add(FVector2D(ScreenX, PlotY));
		GridLine.Add(FVector2D(ScreenX, PlotY + PlotH));

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			GridLine,
			ESlateDrawEffect::None,
			FLinearColor(0.10f, 0.16f, 0.26f, 0.35f),
			true,
			1.0f);

		const FString LabelX = FString::Printf(TEXT("%.1f км"), Alpha * TotalKm);
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2D(ScreenX - 12.0f, PlotY + PlotH + 6.0f))),
			LabelX,
			FontSmall,
			ESlateDrawEffect::None,
			FLinearColor(0.45f, 0.58f, 0.72f, 0.85f));
	}
	LayerId++;

	// 4. Профиль поверхности земли (Terrain)
	TArray<FVector2D> TerrainScreenPoints;
	TerrainScreenPoints.Reserve(ProfilePoints.Num());
	for (const FRailProfilePoint& Pt : ProfilePoints)
	{
		const float NormX = FMath::Clamp(Pt.DistanceAlongRoute_m / TotalLength_m, 0.0f, 1.0f);
		const float NormY = FMath::Clamp((Pt.TerrainHeight_m - DispMinH) / DispRangeH, 0.0f, 1.0f);
		TerrainScreenPoints.Add(FVector2D(PlotX + NormX * PlotW, PlotY + (1.0f - NormY) * PlotH));
	}

	if (TerrainScreenPoints.Num() >= 2)
	{
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			TerrainScreenPoints,
			ESlateDrawEffect::None,
			FLinearColor(0.30f, 0.50f, 0.32f, 0.65f),
			true,
			1.5f);
	}

	// 5. Проектный профиль пути (Track)
	TArray<FVector2D> TrackScreenPoints;
	TrackScreenPoints.Reserve(ProfilePoints.Num());
	for (const FRailProfilePoint& Pt : ProfilePoints)
	{
		const float NormX = FMath::Clamp(Pt.DistanceAlongRoute_m / TotalLength_m, 0.0f, 1.0f);
		const float NormY = FMath::Clamp((Pt.TrackHeight_m - DispMinH) / DispRangeH, 0.0f, 1.0f);
		TrackScreenPoints.Add(FVector2D(PlotX + NormX * PlotW, PlotY + (1.0f - NormY) * PlotH));
	}

	if (TrackScreenPoints.Num() >= 2)
	{
		LayerId++;
		// Вертикальная штриховка земляного полотна (насыпи/выемки)
		const int32 StepHatch = FMath::Max(1, TrackScreenPoints.Num() / 55);
		for (int32 i = 0; i < TrackScreenPoints.Num(); i += StepHatch)
		{
			TArray<FVector2D> HatchLine;
			HatchLine.Add(TrackScreenPoints[i]);
			HatchLine.Add(FVector2D(TrackScreenPoints[i].X, PlotY + PlotH));
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				HatchLine,
				ESlateDrawEffect::None,
				FLinearColor(0.08f, 0.32f, 0.50f, 0.25f),
				true,
				1.0f);
		}

		LayerId++;
		// Основная проектная линия головки рельса (яркий циановый цвет)
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			TrackScreenPoints,
			ESlateDrawEffect::None,
			FLinearColor(0.20f, 0.85f, 1.0f, 1.0f),
			true,
			2.5f);

		// Небольшие аккуратные круглые маркеры старта (зеленый) и финиша (красный)
		const float DotRadius = 4.0f;
		TArray<FVector2D> StartRing;
		TArray<FVector2D> EndRing;
		const int32 NumRingSteps = 10;
		for (int32 s = 0; s <= NumRingSteps; ++s)
		{
			const float Angle = (static_cast<float>(s) / static_cast<float>(NumRingSteps)) * 6.2831853f;
			const FVector2D Offset(FMath::Cos(Angle) * DotRadius, FMath::Sin(Angle) * DotRadius);
			StartRing.Add(TrackScreenPoints[0] + Offset);
			EndRing.Add(TrackScreenPoints.Last() + Offset);
		}

		LayerId++;
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			StartRing,
			ESlateDrawEffect::None,
			FLinearColor(0.1f, 0.95f, 0.2f, 1.0f),
			true,
			2.0f);

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			EndRing,
			ESlateDrawEffect::None,
			FLinearColor(0.95f, 0.15f, 0.15f, 1.0f),
			true,
			2.0f);
	}

	return LayerId + 1;
}

// ─────────────────────────────────────────────────────────────────
//  URailProfileCanvasWidget
// ─────────────────────────────────────────────────────────────────

void URailProfileCanvasWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateCanvas.Reset();
}

void URailProfileCanvasWidget::SetProfileData(
	const TArray<FRailProfilePoint>& InPoints,
	float InMinHeight_m,
	float InMaxHeight_m,
	float InTotalLength_m,
	float InMaxSlopePermille)
{
	if (SlateCanvas.IsValid())
	{
		SlateCanvas->SetProfileData(InPoints, InMinHeight_m, InMaxHeight_m, InTotalLength_m, InMaxSlopePermille);
	}
}

TSharedRef<SWidget> URailProfileCanvasWidget::RebuildWidget()
{
	SlateCanvas = SNew(SRailProfileSlateCanvas);
	return SlateCanvas.ToSharedRef();
}

// ─────────────────────────────────────────────────────────────────
//  URailProfileWidget
// ─────────────────────────────────────────────────────────────────

TSharedRef<SWidget> URailProfileWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ProfileRootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		// Карточка в нижнем левом углу
		UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ProfileCardBorder"));
		CardBorder->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.045f, 0.94f));
		CardBorder->SetPadding(FMargin(14.0f, 12.0f));

		UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(CardBorder);
		CanvasSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		CanvasSlot->SetPosition(FVector2D(24.0f, -24.0f));
		CanvasSlot->SetAutoSize(true);

		USizeBox* ContainerSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ContainerSizeBox"));
		ContainerSizeBox->SetWidthOverride(620.0f);
		ContainerSizeBox->SetHeightOverride(260.0f);
		CardBorder->SetContent(ContainerSizeBox);

		UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
		ContainerSizeBox->SetContent(MainVBox);

		// Верхняя строка: заголовок и кнопка закрытия
		UHorizontalBox* TopHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopHBox"));
		if (UVerticalBoxSlot* TopSlot = MainVBox->AddChildToVerticalBox(TopHBox))
		{
			TopSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
		}

		UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		TitleText->SetText(FText::FromString(TEXT("ПРОДОЛЬНЫЙ ПРОФИЛЬ ТРАССЫ (F1)")));
		TitleText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
		TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.78f, 1.0f, 1.0f)));
		if (UHorizontalBoxSlot* TitleSlot = TopHBox->AddChildToHorizontalBox(TitleText))
		{
			TitleSlot->SetVerticalAlignment(VAlign_Center);
			TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		// Кнопка закрытия
		UButton* CloseBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseBtn"));
		FButtonStyle CloseBtnStyle;
		CloseBtnStyle.Normal.TintColor = FSlateColor(FLinearColor(0.08f, 0.14f, 0.22f, 0.9f));
		CloseBtnStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.18f, 0.30f, 0.45f, 1.0f));
		CloseBtnStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.04f, 0.08f, 0.12f, 1.0f));
		CloseBtn->SetStyle(CloseBtnStyle);
		CloseBtn->OnClicked.AddDynamic(this, &URailProfileWidget::HandleCloseClicked);

		UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseText"));
		CloseText->SetText(FText::FromString(TEXT(" ✕ ")));
		CloseText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
		CloseText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.95f, 1.0f)));
		CloseBtn->AddChild(CloseText);

		if (UHorizontalBoxSlot* CloseSlot = TopHBox->AddChildToHorizontalBox(CloseBtn))
		{
			CloseSlot->SetVerticalAlignment(VAlign_Center);
		}

		// Строка сводки параметров
		SummaryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SummaryText"));
		SummaryText->SetText(FText::FromString(TEXT("Длина: 0.0 км • Отметки: 0 - 0 м • Макс. уклон: 0.0‰")));
		SummaryText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
		SummaryText->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.75f, 0.88f, 1.0f)));
		if (UVerticalBoxSlot* SumSlot = MainVBox->AddChildToVerticalBox(SummaryText))
		{
			SumSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}

		// Холст графика
		ProfileCanvas = WidgetTree->ConstructWidget<URailProfileCanvasWidget>(URailProfileCanvasWidget::StaticClass(), TEXT("ProfileCanvas"));
		if (UVerticalBoxSlot* CanvasSlotBox = MainVBox->AddChildToVerticalBox(ProfileCanvas))
		{
			CanvasSlotBox->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	}

	return Super::RebuildWidget();
}

void URailProfileWidget::UpdateProfileData(
	const TArray<FRailProfilePoint>& InPoints,
	float InMinHeight_m,
	float InMaxHeight_m,
	float InTotalLength_m,
	float InMaxSlopePermille)
{
	if (ProfileCanvas)
	{
		ProfileCanvas->SetProfileData(InPoints, InMinHeight_m, InMaxHeight_m, InTotalLength_m, InMaxSlopePermille);
	}

	if (SummaryText)
	{
		const float DeltaH = InMaxHeight_m - InMinHeight_m;
		const FString InfoStr = FString::Printf(
			TEXT("Длина: %.2f км • Мин: %.0f м • Макс: %.0f м (Δ %.0f м) • Макс. уклон: %.1f‰"),
			InTotalLength_m * 0.001f, InMinHeight_m, InMaxHeight_m, DeltaH, InMaxSlopePermille);
		SummaryText->SetText(FText::FromString(InfoStr));
	}
}

void URailProfileWidget::ToggleVisibilityCustom()
{
	const ESlateVisibility Curr = GetVisibility();
	if (Curr == ESlateVisibility::Visible || Curr == ESlateVisibility::SelfHitTestInvisible)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void URailProfileWidget::HandleCloseClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
