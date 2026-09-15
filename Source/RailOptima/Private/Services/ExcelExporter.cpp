#include "Services/ExcelExporter.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

// libxlsxwriter — C-библиотека, поэтому отключаем строгие правила UE на время include.
THIRD_PARTY_INCLUDES_START
#include "xlsxwriter.h"
THIRD_PARTY_INCLUDES_END

// ─────────────────────────────────────────────────────────────────
//  Команды навигации
// ─────────────────────────────────────────────────────────────────

bool UExcelExporter::ExportReport(const FRailRouteReport& InReport, const FString& InFilePath)
{
	const FString FullPath = NormalizeFilePath(InFilePath);
	const FString Directory = FPaths::GetPath(FullPath);

	if (!IFileManager::Get().DirectoryExists(*Directory))
	{
		IFileManager::Get().MakeDirectory(*Directory, true);
	}

	// Путь для libxlsxwriter — в UTF-8.
	const FTCHARToUTF8 PathUtf8(*FullPath);
	lxw_workbook* Workbook = workbook_new(PathUtf8.Get());

	if (!Workbook)
	{
		UE_LOG(LogTemp, Error, TEXT("[ExcelExporter] Не удалось создать файл: %s"), *FullPath);
		return false;
	}

	lxw_worksheet* SummarySheet = workbook_add_worksheet(Workbook, "Сводка");
	lxw_worksheet* SegmentsSheet = workbook_add_worksheet(Workbook, "Участки");

	if (!SummarySheet || !SegmentsSheet)
	{
		UE_LOG(LogTemp, Error, TEXT("[ExcelExporter] Не удалось создать листы."));
		workbook_close(Workbook);
		return false;
	}

	WriteSummarySheet(InReport, Workbook, SummarySheet);
	WriteSegmentsSheet(InReport, Workbook, SegmentsSheet);

	const lxw_error Result = workbook_close(Workbook);
	if (Result != LXW_NO_ERROR)
	{
		UE_LOG(LogTemp, Error, TEXT("[ExcelExporter] Ошибка закрытия книги: %d"), static_cast<int32>(Result));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[ExcelExporter] Отчёт сохранён: %s"), *FullPath);
	return true;
}

// ─────────────────────────────────────────────────────────────────
//  Внутренняя логика
// ─────────────────────────────────────────────────────────────────

void UExcelExporter::WriteSummarySheet(const FRailRouteReport& InReport, lxw_workbook* InWorkbook, lxw_worksheet* InSheet)
{
	lxw_format* HeaderFormat = workbook_add_format(InWorkbook);
	format_set_bold(HeaderFormat);
	format_set_font_size(HeaderFormat, 12);
	format_set_bg_color(HeaderFormat, LXW_COLOR_YELLOW);
	format_set_border(HeaderFormat, LXW_BORDER_THIN);

	lxw_format* ValueFormat = workbook_add_format(InWorkbook);
	format_set_border(ValueFormat, LXW_BORDER_THIN);

	worksheet_set_column(InSheet, 0, 0, 30, nullptr);
	worksheet_set_column(InSheet, 1, 1, 40, nullptr);

	worksheet_write_string(InSheet, 0, 0, "Показатель", HeaderFormat);
	worksheet_write_string(InSheet, 0, 1, "Значение", HeaderFormat);

	worksheet_write_string(InSheet, 1, 0, "Проект", ValueFormat);
	worksheet_write_string(InSheet, 1, 1, TCHAR_TO_UTF8(*InReport.ProjectName), ValueFormat);

	worksheet_write_string(InSheet, 2, 0, "Дата расчёта", ValueFormat);
	worksheet_write_string(InSheet, 2, 1, TCHAR_TO_UTF8(*InReport.CalculatedAt.ToString(TEXT("%Y-%m-%d %H:%M:%S"))), ValueFormat);

	worksheet_write_string(InSheet, 3, 0, "Общая длина, м", ValueFormat);
	worksheet_write_number(InSheet, 3, 1, InReport.TotalLength_m, ValueFormat);

	worksheet_write_string(InSheet, 4, 0, "Общая стоимость, руб.", ValueFormat);
	worksheet_write_number(InSheet, 4, 1, InReport.TotalCost, ValueFormat);

	worksheet_write_string(InSheet, 5, 0, "Количество участков", ValueFormat);
	worksheet_write_number(InSheet, 5, 1, InReport.Segments.Num(), ValueFormat);
}

void UExcelExporter::WriteSegmentsSheet(const FRailRouteReport& InReport, lxw_workbook* InWorkbook, lxw_worksheet* InSheet)
{
	lxw_format* HeaderFormat = workbook_add_format(InWorkbook);
	format_set_bold(HeaderFormat);
	format_set_bg_color(HeaderFormat, LXW_COLOR_GRAY);
	format_set_font_color(HeaderFormat, LXW_COLOR_WHITE);
	format_set_border(HeaderFormat, LXW_BORDER_THIN);
	format_set_align(HeaderFormat, LXW_ALIGN_CENTER);

	lxw_format* CellFormat = workbook_add_format(InWorkbook);
	format_set_border(CellFormat, LXW_BORDER_THIN);

	worksheet_set_column(InSheet, 0, 0, 8, nullptr);
	worksheet_set_column(InSheet, 1, 1, 12, nullptr);
	worksheet_set_column(InSheet, 2, 2, 12, nullptr);
	worksheet_set_column(InSheet, 3, 3, 14, nullptr);
	worksheet_set_column(InSheet, 4, 4, 14, nullptr);
	worksheet_set_column(InSheet, 5, 5, 14, nullptr);
	worksheet_set_column(InSheet, 6, 6, 18, nullptr);

	worksheet_write_string(InSheet, 0, 0, "№", HeaderFormat);
	worksheet_write_string(InSheet, 0, 1, "От узла", HeaderFormat);
	worksheet_write_string(InSheet, 0, 2, "К узлу", HeaderFormat);
	worksheet_write_string(InSheet, 0, 3, "Длина, м", HeaderFormat);
	worksheet_write_string(InSheet, 0, 4, "Уклон", HeaderFormat);
	worksheet_write_string(InSheet, 0, 5, "Коэф. грунта", HeaderFormat);
	worksheet_write_string(InSheet, 0, 6, "Стоимость, руб.", HeaderFormat);

	for (int32 i = 0; i < InReport.Segments.Num(); ++i)
	{
		const FRailRouteSegment& Seg = InReport.Segments[i];
		const uint32 Row = static_cast<uint32>(i + 1);

		worksheet_write_number(InSheet, Row, 0, i + 1, CellFormat);
		worksheet_write_number(InSheet, Row, 1, Seg.FromNodeIndex, CellFormat);
		worksheet_write_number(InSheet, Row, 2, Seg.ToNodeIndex, CellFormat);
		worksheet_write_number(InSheet, Row, 3, Seg.Length_m, CellFormat);
		worksheet_write_number(InSheet, Row, 4, Seg.SlopeRatio, CellFormat);
		worksheet_write_number(InSheet, Row, 5, Seg.GroundCoefficient, CellFormat);
		worksheet_write_number(InSheet, Row, 6, Seg.Cost, CellFormat);
	}
}

FString UExcelExporter::NormalizeFilePath(const FString& InFilePath) const
{
	FString Path = InFilePath;
	Path.ReplaceInline(TEXT("\\"), TEXT("/"));

	if (!Path.EndsWith(TEXT(".xlsx"), ESearchCase::IgnoreCase))
	{
		Path += TEXT(".xlsx");
	}

	return FPaths::ConvertRelativePathToFull(Path);
}