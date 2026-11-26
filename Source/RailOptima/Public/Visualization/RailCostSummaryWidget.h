// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект: UI-виджет сводной таблицы стоимости строительства пути, ПривГУПС, 2026.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "RailCostSummaryWidget.generated.h"

class UTextBlock;

/**
 * Красивая информационная табличка со сметой пути по НЦС 81-02-07-2025.
 * Автоматически создаёт иерархию UMG-компонентов в C++, не требуя ручной верстки в Blueprint.
 */
UCLASS()
class RAILOPTIMA_API URailCostSummaryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/**
	 * Обновить данные в таблице при пересчёте пути.
	 *
	 * @param InCategoryName   Название текущей категории ЖД.
	 * @param InLength_m       Общая длина пути в метрах.
	 * @param InTotalCost_kRub Общая стоимость в тыс. руб.
	 * @param InMaxSlope_perm  Максимальный уклон на маршруте (‰).
	 * @param InMinRadius_m    Минимальный радиус кривой в плане (м).
	 * @param InTime_ms        Время расчёта (мс).
	 */
	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void UpdateRouteSummary(
		const FString& InCategoryName,
		double		   InLength_m,
		double		   InTotalCost_kRub,
		float		   InMaxSlope_perm,
		float		   InMinRadius_m,
		double		   InTime_ms);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|UI")
	void UpdateEndpointsToggleState(bool bVisible);

private:
	UFUNCTION()
	void HandleToggleEndpointsClicked();

	UPROPERTY()
	TObjectPtr<class UButton> EndpointsToggleBtn;

	UPROPERTY()
	TObjectPtr<UTextBlock> EndpointsToggleText;
	UPROPERTY()
	TObjectPtr<UTextBlock> CategoryValueText;

	UPROPERTY()
	TObjectPtr<UTextBlock> LengthValueText;

	UPROPERTY()
	TObjectPtr<UTextBlock> AvgRateValueText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ProfileValueText;

	UPROPERTY()
	TObjectPtr<UTextBlock> TotalCostValueText;

	UPROPERTY()
	TObjectPtr<UTextBlock> CalcTimeValueText;

	static FString FormatMoneyRubles(double InCost_kRub);
};