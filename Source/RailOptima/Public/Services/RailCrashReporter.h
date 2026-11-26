// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Подсистема перехвата сбоев и детального логирования ошибок RailOptima

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Misc/OutputDevice.h"
#include "RailCrashReporter.generated.h"

class FRailLogCaptureDevice : public FOutputDevice
{
public:
	FRailLogCaptureDevice();
	virtual ~FRailLogCaptureDevice() override;

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override;

	TArray<FString> GetRecentLogs() const;
	void FlushCrashLog(const FString& InReason, const FString& InCallstack);

private:
	mutable FCriticalSection LogLock;
	TArray<FString> RecentLines;
	static constexpr int32 MaxBufferLines = 150;
};

UCLASS()
class RAILOPTIMA_API URailCrashReporter : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Статический доступ к репортеру
	UFUNCTION(BlueprintPure, Category = "RailOptima|Debug", meta = (WorldContext = "WorldContextObject"))
	static URailCrashReporter* Get(const UObject* WorldContextObject);

	// Принудительная запись и вывод ошибки пользователю
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Debug")
	static void ShowFatalErrorMessage(const FString& Title, const FString& Message);

	// Получить путь к файлу краш-дампа
	UFUNCTION(BlueprintPure, Category = "RailOptima|Debug")
	static FString GetCrashLogPath();

	// Установить имя текущего проекта для отчета
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Debug")
	void SetActiveProjectContext(const FString& InProjectName, const FString& InCategory);

private:
	static void HandleSystemCrash();

	TSharedPtr<FRailLogCaptureDevice> LogDevice;
	static URailCrashReporter* Instance;

	FString CurrentProjectName = TEXT("None");
	FString CurrentCategory = TEXT("None");
};
