// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Менеджер настроек графики и оптимизации для слабого железа

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RailSettingsManager.generated.h"

UCLASS()
class RAILOPTIMA_API URailSettingsManager : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Инициализация при старте (автоматический бенчмарк при первом запуске)
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Settings")
	static void InitializeSettings(bool bForceBenchmark = false);

	// Применить общий уровень качества: 0 - Низкое (Слабый ПК), 1 - Среднее, 2 - Высокое, 3 - Ультра
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Settings")
	static void ApplyQualityPreset(int32 PresetLevel);

	// Получить текущий уровень пресета (0..3, или -1 если кастомный)
	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	static int32 GetCurrentQualityPreset();

	// Запуск встроенного теста производительности Unreal Engine и автонастройка
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Settings")
	static void RunAutoBenchmark();

	// Масштаб 3D-разрешения (50.0 .. 100.0 %)
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Settings")
	static void SetResolutionScale(float ScalePercent);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	static float GetResolutionScale();

	// Лимит FPS: 30, 60, 120, 0 (без ограничений)
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Settings")
	static void SetFrameRateLimit(float Limit);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	static float GetFrameRateLimit();

	// Вертикальная синхронизация (VSync)
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Settings")
	static void SetVSyncEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	static bool IsVSyncEnabled();

	// Качество теней (0 - Низкое/Выкл .. 3 - Ультра)
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Settings")
	static void SetShadowQuality(int32 Quality);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	static int32 GetShadowQuality();

	// Сглаживание (0 - Низкое .. 3 - Ультра)
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Settings")
	static void SetAntiAliasingQuality(int32 Quality);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	static int32 GetAntiAliasingQuality();

	// Дальность прорисовки (0 - Низкая .. 3 - Ультра)
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Settings")
	static void SetViewDistanceQuality(int32 Quality);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Settings")
	static int32 GetViewDistanceQuality();

	// Применить оптимизации консольных переменных для слабого железа
	static void ApplyLowEndOptimizations(bool bEnableLowEnd);

	// Сохранить текущие настройки в GameUserSettings.ini
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Settings")
	static void SaveSettings();
};
