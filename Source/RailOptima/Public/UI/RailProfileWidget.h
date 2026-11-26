// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Виджет продольного профиля трассы (проекция перепада высот)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Widgets/SCompoundWidget.h"
#include "RailProfileWidget.generated.h"

USTRUCT(BlueprintType)
struct FRailProfilePoint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Profile")
	float DistanceAlongRoute_m = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Profile")
	float TerrainHeight_m = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Profile")
	float TrackHeight_m = 0.0f;
};

class SRailProfileSlateCanvas : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRailProfileSlateCanvas) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	void SetProfileData(
		const TArray<FRailProfilePoint>& InPoints,
		float InMinHeight_m,
		float InMaxHeight_m,
		float InTotalLength_m,
		float InMaxSlopePermille);

private:
	TArray<FRailProfilePoint> ProfilePoints;
	float MinHeight_m = 0.0f;
	float MaxHeight_m = 100.0f;
	float TotalLength_m = 0.0f;
	float MaxSlopePermille = 0.0f;
};

UCLASS()
class RAILOPTIMA_API URailProfileCanvasWidget : public UWidget
{
	GENERATED_BODY()

public:
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	void SetProfileData(
		const TArray<FRailProfilePoint>& InPoints,
		float InMinHeight_m,
		float InMaxHeight_m,
		float InTotalLength_m,
		float InMaxSlopePermille);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SRailProfileSlateCanvas> SlateCanvas;
};

class UBorder;
class UButton;
class UTextBlock;

UCLASS()
class RAILOPTIMA_API URailProfileWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Profile")
	void UpdateProfileData(
		const TArray<FRailProfilePoint>& InPoints,
		float InMinHeight_m,
		float InMaxHeight_m,
		float InTotalLength_m,
		float InMaxSlopePermille);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Profile")
	void ToggleVisibilityCustom();

private:
	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY()
	TObjectPtr<URailProfileCanvasWidget> ProfileCanvas;

	UPROPERTY()
	TObjectPtr<UTextBlock> SummaryText;
};
