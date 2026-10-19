// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль вызуализации»,
// ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MapLoader.generated.h"

/**
 * Нормализованная карта высот, полученная из 16-бит PNG.
 *
 * Хранит высоты в диапазоне [0..1] и метаданные (границы высот,
 * геопривязку, размер пикселя), необходимые для расчёта уклонов
 * и генерации Landscape.
 */
USTRUCT(BlueprintType)
struct FRailHeightMapData
{
	GENERATED_BODY()

	// Ширина карты в пикселях.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Map")
	int32 Width = 0;

	// Высота карты в пикселях.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Map")
	int32 Height = 0;

	// Нормализованные высоты [0..1], длина = Width * Height.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Map")
	TArray<float> Heights;

	// Минимальная высота на карте, метры.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Map")
	float MinElevation_m = 0.0f;

	// Максимальная высота на карте, метры.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Map")
	float MaxElevation_m = 0.0f;

	// Широта левого нижнего угла карты.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Map")
	double OriginLat = 0.0;

	// Долгота левого нижнего угла карты.
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Map")
	double OriginLon = 0.0;

	// Размер пикселя в метрах (SRTM/Copernicus ~30 м).
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|Map")
	double PixelSize_m = 30.0;
};

/**
 * Загрузчик карт высот из 16-бит grayscale PNG.
 *
 * Использует модуль ImageWrapper. На выходе даёт нормализованный
 * массив высот, пригодный для билинейной интерполяции и генерации
 * ландшафта.
 */
UCLASS()
class RAILOPTIMA_API UMapLoader : public UObject
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────────
	//  Инициализация
	// ─────────────────────────────────────────────────────────────────

	/**
	 * Загрузка 16-бит PNG с картой высот.
	 *
	 * @param InFilePath  Абсолютный путь к PNG.
	 * @param OutData     Заполненная структура с высотами и метаданными.
	 * @return            true, если файл успешно прочитан и декодирован.
	 */
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Map")
	bool LoadHeightMap(const FString& InFilePath, FRailHeightMapData& OutData);

private:
	// ─────────────────────────────────────────────────────────────────
	//  Внутренняя логика
	// ─────────────────────────────────────────────────────────────────

	/**
	 * Декодирование сырых байтов PNG в нормализованный массив высот.
	 *
	 * @param InRawBytes  Содержимое PNG-файла.
	 * @param OutData     Результат декодирования.
	 * @return            true при успехе.
	 */
	bool DecodeImage(const TArray<uint8>& InRawBytes, FRailHeightMapData& OutData);
};