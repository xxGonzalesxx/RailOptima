// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Менеджер настроек графики и оптимизации для слабого железа

#include "Services/RailSettingsManager.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void URailSettingsManager::InitializeSettings(bool bForceBenchmark)
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings)
	{
		return;
	}

	Settings->LoadSettings();

	bool bBenchmarkDone = false;
	GConfig->GetBool(TEXT("RailOptima"), TEXT("bHardwareBenchmarkCompleted"), bBenchmarkDone, GGameIni);

	if (!bBenchmarkDone || bForceBenchmark)
	{
		UE_LOG(LogTemp, Log, TEXT("[RailSettings] Автонастройка: запуск бенчмарка оборудования..."));
				if (PLATFORM_LINUX || FParse::Param(FCommandLine::Get(), TEXT("NoAutoBench")))
		{
			// GPU-бенчмарк на Linux/Vulkan вешает драйвер NVIDIA (Xid 109) — пропускаем, берём Low.
			UE_LOG(LogTemp, Warning, TEXT("[RailSettings] Бенчмарк пропущен, применяется пресет Low"));
			Settings->SetOverallScalabilityLevel(0);
		}
		else
		{
			Settings->RunHardwareBenchmark(10, 1.0f, 1.0f);
			Settings->ApplyHardwareBenchmarkResults();
		}

		const int32 Overall = Settings->GetOverallScalabilityLevel();
		UE_LOG(LogTemp, Log, TEXT("[RailSettings] Результат бенчмарка: уровень масштабирования = %d"), Overall);

		if (Overall <= 0)
		{
			// Слабое железо: применяем щадящие лимиты и консольные оптимизации
			Settings->SetFrameRateLimit(60.0f);
			Settings->SetResolutionScaleNormalized(0.70f);
			ApplyLowEndOptimizations(true);
		}
		else if (Overall == 1)
		{
			Settings->SetFrameRateLimit(60.0f);
			Settings->SetResolutionScaleNormalized(0.85f);
			ApplyLowEndOptimizations(false);
		}
		else
		{
			Settings->SetFrameRateLimit(0.0f);
			Settings->SetResolutionScaleNormalized(1.0f);
			ApplyLowEndOptimizations(false);
		}

		Settings->ApplySettings(false);
		Settings->SaveSettings();

		GConfig->SetBool(TEXT("RailOptima"), TEXT("bHardwareBenchmarkCompleted"), true, GGameIni);
		GConfig->Flush(false, GGameIni);
	}
	else
	{
		if (Settings->GetOverallScalabilityLevel() <= 0)
		{
			ApplyLowEndOptimizations(true);
		}
		Settings->ApplySettings(false);
	}
}

void URailSettingsManager::ApplyQualityPreset(int32 PresetLevel)
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings)
	{
		return;
	}

	const int32 Level = FMath::Clamp(PresetLevel, 0, 3);
	Settings->SetOverallScalabilityLevel(Level);

	switch (Level)
	{
	case 0: // Низкое (Слабый ПК)
		Settings->SetResolutionScaleNormalized(0.70f);
		Settings->SetFrameRateLimit(60.0f);
		Settings->SetVSyncEnabled(false);
		ApplyLowEndOptimizations(true);
		break;

	case 1: // Среднее
		Settings->SetResolutionScaleNormalized(0.85f);
		Settings->SetFrameRateLimit(60.0f);
		Settings->SetVSyncEnabled(false);
		ApplyLowEndOptimizations(false);
		break;

	case 2: // Высокое
		Settings->SetResolutionScaleNormalized(1.0f);
		Settings->SetFrameRateLimit(0.0f);
		ApplyLowEndOptimizations(false);
		break;

	case 3: // Ультра
	default:
		Settings->SetResolutionScaleNormalized(1.0f);
		Settings->SetFrameRateLimit(0.0f);
		ApplyLowEndOptimizations(false);
		break;
	}

	Settings->ApplySettings(false);
	Settings->SaveSettings();
	UE_LOG(LogTemp, Log, TEXT("[RailSettings] Применён пресет графики: %d"), Level);
}

int32 URailSettingsManager::GetCurrentQualityPreset()
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		return Settings->GetOverallScalabilityLevel();
	}
	return 2;
}

void URailSettingsManager::RunAutoBenchmark()
{
	InitializeSettings(/*bForceBenchmark=*/true);
}

void URailSettingsManager::SetResolutionScale(float ScalePercent)
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		const float Normalized = FMath::Clamp(ScalePercent, 50.0f, 100.0f) / 100.0f;
		Settings->SetResolutionScaleNormalized(Normalized);
	}
}

float URailSettingsManager::GetResolutionScale()
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		return Settings->GetResolutionScaleNormalized() * 100.0f;
	}
	return 100.0f;
}

void URailSettingsManager::SetFrameRateLimit(float Limit)
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetFrameRateLimit(Limit);
	}
}

float URailSettingsManager::GetFrameRateLimit()
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		return Settings->GetFrameRateLimit();
	}
	return 0.0f;
}

void URailSettingsManager::SetVSyncEnabled(bool bEnabled)
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetVSyncEnabled(bEnabled);
	}
}

bool URailSettingsManager::IsVSyncEnabled()
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		return Settings->IsVSyncEnabled();
	}
	return false;
}

void URailSettingsManager::SetShadowQuality(int32 Quality)
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetShadowQuality(FMath::Clamp(Quality, 0, 3));
	}
}

int32 URailSettingsManager::GetShadowQuality()
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		return Settings->GetShadowQuality();
	}
	return 2;
}

void URailSettingsManager::SetAntiAliasingQuality(int32 Quality)
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetAntiAliasingQuality(FMath::Clamp(Quality, 0, 3));
	}
}

int32 URailSettingsManager::GetAntiAliasingQuality()
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		return Settings->GetAntiAliasingQuality();
	}
	return 2;
}

void URailSettingsManager::SetViewDistanceQuality(int32 Quality)
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetViewDistanceQuality(FMath::Clamp(Quality, 0, 3));
	}
}

int32 URailSettingsManager::GetViewDistanceQuality()
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		return Settings->GetViewDistanceQuality();
	}
	return 2;
}

void URailSettingsManager::ApplyLowEndOptimizations(bool bEnableLowEnd)
{
	if (!GEngine)
	{
		return;
	}

	if (bEnableLowEnd)
	{
		GEngine->Exec(nullptr, TEXT("r.Shadow.Virtual.Enable 0"));
		GEngine->Exec(nullptr, TEXT("r.Lumen.DiffuseIndirect.Allow 0"));
		GEngine->Exec(nullptr, TEXT("r.Lumen.Reflections.Allow 0"));
		GEngine->Exec(nullptr, TEXT("r.DistanceFieldShadowing 0"));
		GEngine->Exec(nullptr, TEXT("r.VolumetricFog 0"));
		UE_LOG(LogTemp, Log, TEXT("[RailSettings] Активированы оптимизации для слабого оборудования"));
	}
	else
	{
		GEngine->Exec(nullptr, TEXT("r.Shadow.Virtual.Enable 1"));
		GEngine->Exec(nullptr, TEXT("r.Lumen.DiffuseIndirect.Allow 1"));
		GEngine->Exec(nullptr, TEXT("r.Lumen.Reflections.Allow 1"));
		GEngine->Exec(nullptr, TEXT("r.VolumetricFog 1"));
	}
}

void URailSettingsManager::SaveSettings()
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->ApplySettings(false);
		Settings->SaveSettings();
		UE_LOG(LogTemp, Log, TEXT("[RailSettings] Настройки успешно применены и сохранены в GameUserSettings.ini"));
	}
}

static const TCHAR* GVisualSettingsSection = TEXT("RailOptima.VisualSettings");

void URailSettingsManager::SetCameraFlySpeed(float Value)
{
	if (GConfig)
	{
		GConfig->SetFloat(GVisualSettingsSection, TEXT("CameraFlySpeed"), Value, GGameIni);
		GConfig->Flush(false, GGameIni);
	}
}

float URailSettingsManager::GetCameraFlySpeed()
{
	float Value = 100000.0f; // 1000 м/с
	if (GConfig)
	{
		GConfig->GetFloat(GVisualSettingsSection, TEXT("CameraFlySpeed"), Value, GGameIni);
	}
	return Value;
}

void URailSettingsManager::SetCameraShiftBoost(float Value)
{
	if (GConfig)
	{
		GConfig->SetFloat(GVisualSettingsSection, TEXT("CameraShiftBoost"), Value, GGameIni);
		GConfig->Flush(false, GGameIni);
	}
}

float URailSettingsManager::GetCameraShiftBoost()
{
	float Value = 4.0f;
	if (GConfig)
	{
		GConfig->GetFloat(GVisualSettingsSection, TEXT("CameraShiftBoost"), Value, GGameIni);
	}
	return Value;
}

void URailSettingsManager::SetPathRibbonWidth(float Value)
{
	if (GConfig)
	{
		GConfig->SetFloat(GVisualSettingsSection, TEXT("PathRibbonWidth"), Value, GGameIni);
		GConfig->Flush(false, GGameIni);
	}
}

float URailSettingsManager::GetPathRibbonWidth()
{
	float Value = 4000.0f; // 40 м
	if (GConfig)
	{
		GConfig->GetFloat(GVisualSettingsSection, TEXT("PathRibbonWidth"), Value, GGameIni);
	}
	return Value;
}

void URailSettingsManager::SetPathRibbonHeight(float Value)
{
	if (GConfig)
	{
		GConfig->SetFloat(GVisualSettingsSection, TEXT("PathRibbonHeight"), Value, GGameIni);
		GConfig->Flush(false, GGameIni);
	}
}

float URailSettingsManager::GetPathRibbonHeight()
{
	float Value = 8000.0f; // 80 м
	if (GConfig)
	{
		GConfig->GetFloat(GVisualSettingsSection, TEXT("PathRibbonHeight"), Value, GGameIni);
	}
	return Value;
}

void URailSettingsManager::SetEndpointScaleMultiplier(float Value)
{
	if (GConfig)
	{
		GConfig->SetFloat(GVisualSettingsSection, TEXT("EndpointScaleMultiplier"), Value, GGameIni);
		GConfig->Flush(false, GGameIni);
	}
}

float URailSettingsManager::GetEndpointScaleMultiplier()
{
	float Value = 1.0f;
	if (GConfig)
	{
		GConfig->GetFloat(GVisualSettingsSection, TEXT("EndpointScaleMultiplier"), Value, GGameIni);
	}
	return Value;
}

