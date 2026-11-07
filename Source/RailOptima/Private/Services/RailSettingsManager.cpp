// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Менеджер настроек графики и оптимизации для слабого железа

#include "Services/RailSettingsManager.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "Misc/ConfigCacheIni.h"

void URailSettingsManager::InitializeSettings(bool bForceBenchmark)
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings)
	{
		return;
	}

	Settings->LoadSettings();

	UE_LOG(LogTemp, Log, TEXT("[RailSettings] Принудительная активация режима для слабого железа (Low-End)."));

	Settings->SetOverallScalabilityLevel(0);
	Settings->SetResolutionScaleNormalized(0.70f);
	Settings->SetFrameRateLimit(60.0f);
	Settings->SetVSyncEnabled(false);

	ApplyLowEndOptimizations(true);

	Settings->ApplySettings(false);
	Settings->SaveSettings();

	GConfig->SetBool(TEXT("RailOptima"), TEXT("bHardwareBenchmarkCompleted"), true, GGameIni);
	GConfig->Flush(false, GGameIni);
}

void URailSettingsManager::ApplyQualityPreset(int32 PresetLevel)
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings)
	{
		return;
	}

	// Пока жестко фиксируем пресет 0 (Низкое / Слабый ПК)
	const int32 Level = 0;
	Settings->SetOverallScalabilityLevel(Level);
	Settings->SetResolutionScaleNormalized(0.70f);
	Settings->SetFrameRateLimit(60.0f);
	Settings->SetVSyncEnabled(false);
	ApplyLowEndOptimizations(true);

	Settings->ApplySettings(false);
	Settings->SaveSettings();
	UE_LOG(LogTemp, Log, TEXT("[RailSettings] Применён принудительный пресет для слабого ПК: %d"), Level);
}

int32 URailSettingsManager::GetCurrentQualityPreset()
{
	return 0;
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
	return 70.0f;
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
	return 60.0f;
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
		Settings->SetShadowQuality(0);
	}
}

int32 URailSettingsManager::GetShadowQuality()
{
	return 0;
}

void URailSettingsManager::SetAntiAliasingQuality(int32 Quality)
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetAntiAliasingQuality(0);
	}
}

int32 URailSettingsManager::GetAntiAliasingQuality()
{
	return 0;
}

void URailSettingsManager::SetViewDistanceQuality(int32 Quality)
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetViewDistanceQuality(0);
	}
}

int32 URailSettingsManager::GetViewDistanceQuality()
{
	return 0;
}

void URailSettingsManager::ApplyLowEndOptimizations(bool bEnableLowEnd)
{
	if (!GEngine)
	{
		return;
	}

	// Всегда включаем оптимизации для слабого оборудования
	GEngine->Exec(nullptr, TEXT("r.Shadow.Virtual.Enable 0"));
	GEngine->Exec(nullptr, TEXT("r.Lumen.DiffuseIndirect.Allow 0"));
	GEngine->Exec(nullptr, TEXT("r.Lumen.Reflections.Allow 0"));
	GEngine->Exec(nullptr, TEXT("r.DistanceFieldShadowing 0"));
	GEngine->Exec(nullptr, TEXT("r.VolumetricFog 0"));
	GEngine->Exec(nullptr, TEXT("r.Streaming.PoolSize 1500"));
	UE_LOG(LogTemp, Log, TEXT("[RailSettings] Активированы жесткие оптимизации для слабого оборудования (GTX 1650 / 4GB VRAM)"));
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
