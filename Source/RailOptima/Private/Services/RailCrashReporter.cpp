// Copyright (c) 2026 Николайчик А.В. Все права защищены.

#include "Services/RailCrashReporter.h"
#include "GenericPlatform/GenericPlatformCrashContext.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformOutputDevices.h"
#include "Misc/CoreDelegates.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

// ─────────────────────────────────────────────────────────────────
//  FRailLogCaptureDevice
// ─────────────────────────────────────────────────────────────────

FRailLogCaptureDevice::FRailLogCaptureDevice()
{
	RecentLines.Reserve(MaxBufferLines);
	if (GLog)
	{
		GLog->AddOutputDevice(this);
	}
}

FRailLogCaptureDevice::~FRailLogCaptureDevice()
{
	if (GLog)
	{
		GLog->RemoveOutputDevice(this);
	}
}

void FRailLogCaptureDevice::Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category)
{
	if (!V) return;

	FScopeLock Lock(&LogLock);
	const FString Line = FString::Printf(TEXT("[%s] [%s] %s"),
		*FDateTime::Now().ToString(TEXT("%H:%M:%S")),
		*Category.ToString(),
		V);

	if (RecentLines.Num() >= MaxBufferLines)
	{
		RecentLines.RemoveAt(0);
	}
	RecentLines.Add(Line);
}

TArray<FString> FRailLogCaptureDevice::GetRecentLogs() const
{
	FScopeLock Lock(&LogLock);
	return RecentLines;
}

void FRailLogCaptureDevice::FlushCrashLog(const FString& InReason, const FString& InCallstack)
{
	const FString CrashLogFilePath = URailCrashReporter::GetCrashLogPath();
	
	FString Report;
	Report += TEXT("====================================================\n");
	Report += TEXT("          ОТЧЕТ ОБ ОШИБКЕ RAILOPTIMA\n");
	Report += TEXT("====================================================\n");
	Report += FString::Printf(TEXT("Время: %s\n"), *FDateTime::Now().ToString());
	Report += FString::Printf(TEXT("Причина / Текст ошибки: %s\n\n"), *InReason);

	if (!InCallstack.IsEmpty())
	{
		Report += TEXT("--- Callstack ---\n");
		Report += InCallstack;
		Report += TEXT("\n\n");
	}

	Report += TEXT("--- Последние события приложения ---\n");
	{
		FScopeLock Lock(&LogLock);
		for (const FString& L : RecentLines)
		{
			Report += L + TEXT("\n");
		}
	}
	Report += TEXT("====================================================\n");

	FFileHelper::SaveStringToFile(Report, *CrashLogFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

// ─────────────────────────────────────────────────────────────────
//  URailCrashReporter
// ─────────────────────────────────────────────────────────────────

URailCrashReporter* URailCrashReporter::Instance = nullptr;

void URailCrashReporter::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Instance = this;

	LogDevice = MakeShared<FRailLogCaptureDevice>();

	FCoreDelegates::OnHandleSystemError.AddStatic(&URailCrashReporter::HandleSystemCrash);

	UE_LOG(LogTemp, Log, TEXT("[RailCrashReporter] Подсистема защиты и логирования запущена."));
}

void URailCrashReporter::Deinitialize()
{
	if (LogDevice.IsValid())
	{
		LogDevice.Reset();
	}
	Instance = nullptr;
	Super::Deinitialize();
}

URailCrashReporter* URailCrashReporter::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return Instance;
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<URailCrashReporter>();
		}
	}
	return Instance;
}

FString URailCrashReporter::GetCrashLogPath()
{
	return FPaths::ProjectSavedDir() / TEXT("Logs/RailOptima_CrashReport.log");
}

void URailCrashReporter::SetActiveProjectContext(const FString& InProjectName, const FString& InCategory)
{
	CurrentProjectName = InProjectName;
	CurrentCategory = InCategory;
}

void URailCrashReporter::HandleSystemCrash()
{
	if (!Instance || !Instance->LogDevice.IsValid())
	{
		return;
	}

	const FString CrashDetails = FString::Printf(TEXT("Критический сбой игрового процесса в проекте: %s (%s)"),
		*Instance->CurrentProjectName, *Instance->CurrentCategory);

	Instance->LogDevice->FlushCrashLog(CrashDetails, TEXT(""));

#if PLATFORM_WINDOWS
	const FString Msg = FString::Printf(
		TEXT("Произошла непредвиденная ошибка в RailOptima.\n\n")
		TEXT("Детальный отчет сохранен в файл:\n%s\n\n")
		TEXT("Приложение будет завершено."),
		*GetCrashLogPath());

	::MessageBoxW(nullptr, *Msg, L"RailOptima — Критическая ошибка", MB_OK | MB_ICONERROR | MB_TOPMOST);
#endif
}

void URailCrashReporter::ShowFatalErrorMessage(const FString& Title, const FString& Message)
{
	if (Instance && Instance->LogDevice.IsValid())
	{
		Instance->LogDevice->FlushCrashLog(Message, TEXT(""));
	}

#if PLATFORM_WINDOWS
	const FString Formatted = FString::Printf(
		TEXT("%s\n\n")
		TEXT("Подробный лог:\n%s"),
		*Message,
		*GetCrashLogPath());

	::MessageBoxW(nullptr, *Formatted, *Title, MB_OK | MB_ICONWARNING | MB_TOPMOST);
#else
	UE_LOG(LogTemp, Error, TEXT("[RailCrashReporter] %s: %s"), *Title, *Message);
#endif
}
