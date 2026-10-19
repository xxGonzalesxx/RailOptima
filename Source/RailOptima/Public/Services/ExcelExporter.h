// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль вызуализации»,
// ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "Misc/DateTime.h"
#include "UObject/NoExportTypes.h"
#include "ExcelExporter.generated.h"


// Forward-объявления C-структур libxlsxwriter.
struct lxw_workbook;
struct lxw_worksheet;

/**
 * Одна запись отчёта: участок маршрута между двумя узлами.
 */
USTRUCT(BlueprintType)
struct FRailRouteSegment
{
	GENERATED_BODY()

	// Индекс начального узла сетки.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Export")
	int32 FromNodeIndex = INDEX_NONE;

	// Индекс конечного узла сетки.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Export")
	int32 ToNodeIndex = INDEX_NONE;

	// Длина участка, метры.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Export")
	double Length_m = 0.0;

	// Уклон участка как отношение превышения к заложению.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Export")
	double SlopeRatio = 0.0;

	// Коэффициент грунта на участке.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Export")
	double GroundCoefficient = 1.0;

	// Стоимость строительства участка, рубли.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Export")
	double Cost = 0.0;
};

/**
 * Сводка по рассчитанному маршруту.
 *
 * Содержит всё, что нужно для отчёта: длину, суммарную стоимость,
 * список участков и метаданные (имя проекта, дата расчёта).
 */
USTRUCT(BlueprintType)
struct FRailRouteReport
{
	GENERATED_BODY()

	// Название проекта.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Export")
	FString ProjectName = TEXT("RailOptima");

	// Дата и время расчёта.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Export")
	FDateTime CalculatedAt;

	// Общая длина маршрута, метры.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Export")
	double TotalLength_m = 0.0;

	// Общая стоимость строительства, рубли.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Export")
	double TotalCost = 0.0;

	// Участки маршрута.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Export")
	TArray<FRailRouteSegment> Segments;
};

/**
 * Экспорт отчёта о маршруте в файл Excel (.xlsx).
 *
 * Обёртка над libxlsxwriter. Формирует файл с двумя листами:
 *  - «Сводка» — итоговые показатели;
 *  - «Участки» — построчная таблица участков с форматированием.
 */
UCLASS()
class RAILOPTIMA_API UExcelExporter : public UObject
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────────
	//  Команды навигации
	// ─────────────────────────────────────────────────────────────────

	/**
	 * Сохранить отчёт в .xlsx.
	 *
	 * @param InReport      Данные отчёта.
	 * @param InFilePath    Абсолютный путь к файлу (без .xlsx или с ним).
	 * @return              true при успешном сохранении.
	 */
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Export")
	bool ExportReport(const FRailRouteReport& InReport, const FString& InFilePath);

private:
	// ─────────────────────────────────────────────────────────────────
	//  Внутренняя логика
	// ─────────────────────────────────────────────────────────────────

	/**
	 * Заполнить лист «Сводка».
	 *
	 * @param InReport    Данные отчёта.
	 * @param InWorkbook  Указатель на книгу libxlsxwriter.
	 * @param InSheet     Указатель на лист libxlsxwriter.
	 */
	void WriteSummarySheet(const FRailRouteReport& InReport, lxw_workbook* InWorkbook, lxw_worksheet* InSheet);

	/**
	 * Заполнить лист «Участки».
	 *
	 * @param InReport    Данные отчёта.
	 * @param InWorkbook  Указатель на книгу libxlsxwriter.
	 * @param InSheet     Указатель на лист libxlsxwriter.
	 */
	void WriteSegmentsSheet(const FRailRouteReport& InReport, lxw_workbook* InWorkbook, lxw_worksheet* InSheet);

	/**
	 * Нормализовать путь: убрать лишние слэши, добавить .xlsx при необходимости.
	 *
	 * @param InFilePath  Исходный путь.
	 * @return            Готовый путь к файлу.
	 */
	FString NormalizeFilePath(const FString& InFilePath) const;
};