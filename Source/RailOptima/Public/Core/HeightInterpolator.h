// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект 
// «Разработка программы проектирования железнодорожных путей: модуль вызуализации», 
// ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/MapLoader.h"
#include "HeightInterpolator.generated.h"

/**
 * Билинейная интерполяция высот по карте рельефа.
 *
 * Работает в трёх системах координат:
 *  - нормализованные [0..1];
 *  - пиксельные [0..Width-1] × [0..Height-1];
 *  - мировые UE (сантиметры), привязанные к Landscape через SetWorldMapping.
 *
 * Возвращает высоты в метрах (с учётом MinElevation_m / MaxElevation_m),
 * а не нормализованные [0..1]. Это нужно для корректного расчёта уклонов
 * и стоимости строительства.
 */
UCLASS()
class RAILOPTIMA_API UHeightInterpolator : public UObject
{
	GENERATED_BODY()

public:

	// ─────────────────────────────────────────────────────────────────
	//  Инициализация
	// ─────────────────────────────────────────────────────────────────

	/**
	 * Привязка интерполятора к загруженной карте высот.
	 *
	 * @param InMapData  Данные из UMapLoader::LoadHeightMap.
	 */
	void Initialize(const FRailHeightMapData& InMapData);

	/**
	 * Привязка мировой системы координат UE к пиксельной сетке карты.
	 *
	 * @param InWorldOrigin    Мировая позиция пикселя (0, 0) в UE-единицах.
	 * @param InWorldSizeXY    Размер всей карты по осям X и Y в UE-единицах.
	 * @param InWorldSizeZ     Полный размах высот в UE-единицах.
	 */
	void SetWorldMapping(const FVector2D& InWorldOrigin, const FVector2D& InWorldSizeXY, float InWorldSizeZ);

	// ─────────────────────────────────────────────────────────────────
	//  Геттеры
	// ─────────────────────────────────────────────────────────────────

	/**
	 * Высота в метрах по нормализованным координатам.
	 *
	 * @param InNormX  Координата X в диапазоне [0..1].
	 * @param InNormY  Координата Y в диапазоне [0..1].
	 * @return         Высота в метрах над уровнем моря.
	 */
	UFUNCTION(BlueprintPure, Category = "RailOptima|Interpolation")
	float GetHeightNormalized(float InNormX, float InNormY) const;

	/**
	 * Высота в метрах по пиксельным координатам (дробным).
	 *
	 * @param InPixelX  Координата X в пикселях [0..Width-1].
	 * @param InPixelY  Координата Y в пикселях [0..Height-1].
	 * @return          Высота в метрах.
	 */
	UFUNCTION(BlueprintPure, Category = "RailOptima|Interpolation")
	float GetHeightPixel(float InPixelX, float InPixelY) const;

	/**
	 * Высота в метрах по мировой позиции UE.
	 *
	 * @param InWorldLocation  Позиция в UE-единицах (сантиметрах).
	 * @return                 Высота в метрах.
	 */
	UFUNCTION(BlueprintPure, Category = "RailOptima|Interpolation")
	float GetHeightWorld(const FVector& InWorldLocation) const;

	/**
	 * Уклон между двумя точками как отношение превышения к заложению.
	 *
	 * Возвращает tan(угла наклона). Например, 0.02 = 2% = 20‰.
	 *
	 * @param InFrom  Начальная точка в UE-единицах.
	 * @param InTo    Конечная точка в UE-единицах.
	 * @return        Отношение превышения к горизонтальному расстоянию.
	 */
	UFUNCTION(BlueprintPure, Category = "RailOptima|Interpolation")
	float GetSlopeRatio(const FVector& InFrom, const FVector& InTo) const;

	/**
	 * Уклон между двумя точками в градусах.
	 *
	 * @param InFrom  Начальная точка в UE-единицах.
	 * @param InTo    Конечная точка в UE-единицах.
	 * @return        Угол наклона в градусах [−90..90].
	 */
	UFUNCTION(BlueprintPure, Category = "RailOptima|Interpolation")
	float GetSlopeDegrees(const FVector& InFrom, const FVector& InTo) const;

	// ─────────────────────────────────────────────────────────────────
	//  Управление состоянием
	// ─────────────────────────────────────────────────────────────────

	// Готова ли карта к запросам.
	UFUNCTION(BlueprintPure, Category = "RailOptima|Interpolation")
	bool IsInitialized() const { return bInitialized; }

private:

	// ─────────────────────────────────────────────────────────────────
	//  Внутренняя логика
	// ─────────────────────────────────────────────────────────────────

	/**
	 * Ядро билинейной интерполяции по пиксельным координатам.
	 *
	 * @param InPixelX  Дробная координата X.
	 * @param InPixelY  Дробная координата Y.
	 * @return          Высота в метрах.
	 */
	float SampleBilinear(float InPixelX, float InPixelY) const;

	/**
	 * Нормализованная высота пикселя с клампом индексов по границам.
	 *
	 * @param InX  Индекс X.
	 * @param InY  Индекс Y.
	 * @return     Нормализованная высота [0..1].
	 */
	float GetPixelClamped(int32 InX, int32 InY) const;

	/**
	 * Перевод мировой позиции в дробные пиксельные координаты.
	 *
	 * @param InWorldLocation  Позиция в UE-единицах.
	 * @return                 Координаты в пикселях.
	 */
	FVector2D WorldToPixel(const FVector& InWorldLocation) const;

	// ─────────────────────────────────────────────────────────────────
	//  Состояние
	// ─────────────────────────────────────────────────────────────────

	// Копия карты высот.
	FRailHeightMapData MapData;

	// Инициализирован ли интерполятор.
	bool bInitialized = false;

	// Мировая позиция пикселя (0, 0), UE-единицы.
	FVector2D WorldOrigin = FVector2D::ZeroVector;

	// Размер карты по X и Y в UE-единицах.
	FVector2D WorldSizeXY = FVector2D(1.0f, 1.0f);

	// Размах высот в UE-единицах.
	float WorldSizeZ = 1.0f;

	// Размах высот в метрах (MaxElevation_m − MinElevation_m).
	float ElevationRange_m = 0.0f;
};