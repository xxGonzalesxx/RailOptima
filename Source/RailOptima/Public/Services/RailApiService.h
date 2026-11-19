// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Клиентский сервис взаимодействия с REST API RailOptima (Quarkus/Spring Boot)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Services/RailProjectTypes.h"
#include "RailApiService.generated.h"

UENUM(BlueprintType)
enum class ERailCloudProjectStatus : uint8
{
	Created UMETA(DisplayName = "Создан (CREATED)"),
	InProgress UMETA(DisplayName = "В работе (IN_PROGRESS)"),
	Completed UMETA(DisplayName = "Завершен (COMPLETED)")
};

USTRUCT(BlueprintType)
struct FRailCloudProject
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|API")
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|API")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|API")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|API")
	ERailCloudProjectStatus Status = ERailCloudProjectStatus::Created;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|API")
	FString StatusString = TEXT("CREATED");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|API")
	FString OwnerId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|API")
	FString CreatedAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|API")
	FString UpdatedAt;
};

USTRUCT(BlueprintType)
struct FRailAuthData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|API")
	FString Token;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|API")
	FString UserId;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|API")
	FString Email;

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|API")
	FString Role = TEXT("USER");

	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|API")
	bool bIsAuthenticated = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnRailAuthResponseSignature, bool, bSuccess, const FString&, Message, const FString&, Email, const FString&, Role);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRailProjectsListResponseSignature, bool, bSuccess, const TArray<FRailCloudProject>&, Projects, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRailProjectActionResponseSignature, bool, bSuccess, const FRailCloudProject&, Project, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRailProjectDeleteResponseSignature, bool, bSuccess, const FString&, Message);

UCLASS()
class RAILOPTIMA_API URailApiService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Статический доступ к подсистеме из любого контекста мира
	UFUNCTION(BlueprintPure, Category = "RailOptima|API", meta = (WorldContext = "WorldContextObject"))
	static URailApiService* Get(const UObject* WorldContextObject);

	// ─── Конфигурация сервера ─────────────────────────────────────
	// Базовый адрес API (по умолчанию localhost, может быть изменен в коде)
	static constexpr const TCHAR* DefaultApiBaseUrl = TEXT("http://localhost:8080");

	UFUNCTION(BlueprintPure, Category = "RailOptima|API")
	FString GetBaseUrl() const { return BaseUrl; }

	UFUNCTION(BlueprintCallable, Category = "RailOptima|API")
	void SetBaseUrl(const FString& InUrl);

	// ─── Аутентификация и сессия ──────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "RailOptima|API")
	void Register(const FString& Email, const FString& Password);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|API")
	void Login(const FString& Email, const FString& Password);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|API")
	void Logout();

	UFUNCTION(BlueprintPure, Category = "RailOptima|API")
	bool IsAuthenticated() const { return AuthData.bIsAuthenticated && !AuthData.Token.IsEmpty(); }

	UFUNCTION(BlueprintPure, Category = "RailOptima|API")
	const FRailAuthData& GetAuthData() const { return AuthData; }

	UFUNCTION(BlueprintPure, Category = "RailOptima|API")
	FString GetAuthToken() const { return AuthData.Token; }

	// ─── Проекты (REST /api/v1/projects) ──────────────────────────
	UFUNCTION(BlueprintCallable, Category = "RailOptima|API")
	void FetchProjects(int32 Page = 0, int32 Size = 20);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|API")
	void CreateRemoteProject(const FString& Name, const FString& Description);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|API")
	void GetRemoteProject(const FString& ProjectId);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|API")
	void UpdateRemoteProject(const FString& ProjectId, const FString& Name, const FString& Description, ERailCloudProjectStatus Status);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|API")
	void DeleteRemoteProject(const FString& ProjectId);

	// Синхронизация локального проекта в облако
	UFUNCTION(BlueprintCallable, Category = "RailOptima|API")
	void SyncLocalProjectToCloud(const FRailProjectMetadata& LocalProject);

	// ─── Делегаты событий ─────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category = "RailOptima|API|Events")
	FOnRailAuthResponseSignature OnAuthCompleted;

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|API|Events")
	FOnRailProjectsListResponseSignature OnProjectsListReceived;

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|API|Events")
	FOnRailProjectActionResponseSignature OnProjectActionCompleted;

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|API|Events")
	FOnRailProjectDeleteResponseSignature OnProjectDeleted;

	// Кэш последнего полученного списка облачных проектов
	UPROPERTY(BlueprintReadOnly, Category = "RailOptima|API")
	TArray<FRailCloudProject> CachedCloudProjects;

private:
	void SaveSessionToConfig();
	void LoadSessionFromConfig();

	TSharedRef<IHttpRequest> CreateAuthenticatedRequest(const FString& SubPath, const FString& Verb);

	// Обработчики ответов HTTP
	void HandleRegisterResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void HandleLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void HandleGetProjectsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void HandleCreateProjectResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void HandleUpdateProjectResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void HandleDeleteProjectResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, FString ProjectId);

	static bool ParseJsonApiResponse(FHttpResponsePtr Response, TSharedPtr<FJsonObject>& OutData, FString& OutMessage, bool& OutSuccess);
	static FRailCloudProject ParseProjectJsonObject(const TSharedPtr<FJsonObject>& JsonObj);

	UPROPERTY()
	FString BaseUrl = TEXT("http://localhost:8080");

	UPROPERTY()
	FRailAuthData AuthData;
};
