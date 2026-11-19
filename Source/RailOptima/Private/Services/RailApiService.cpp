// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Клиентский сервис взаимодействия с REST API RailOptima (Quarkus/Spring Boot)

#include "Services/RailApiService.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/ConfigCacheIni.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void URailApiService::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadSessionFromConfig();
	UE_LOG(LogTemp, Log, TEXT("[RailAPI] URailApiService инициализирован. BaseUrl: %s, Авторизован: %s"),
		*BaseUrl, AuthData.bIsAuthenticated ? TEXT("Да") : TEXT("Нет"));
}

void URailApiService::Deinitialize()
{
	SaveSessionToConfig();
	Super::Deinitialize();
}

URailApiService* URailApiService::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<URailApiService>() : nullptr;
}

void URailApiService::SetBaseUrl(const FString& InUrl)
{
	BaseUrl = InUrl.TrimStartAndEnd();
	while (BaseUrl.EndsWith(TEXT("/")))
	{
		BaseUrl.RemoveAt(BaseUrl.Len() - 1);
	}
	SaveSessionToConfig();
}

void URailApiService::SaveSessionToConfig()
{
	GConfig->SetString(TEXT("RailOptima.Api"), TEXT("BaseUrl"), *BaseUrl, GGameIni);
	GConfig->SetString(TEXT("RailOptima.Api"), TEXT("Token"), *AuthData.Token, GGameIni);
	GConfig->SetString(TEXT("RailOptima.Api"), TEXT("UserId"), *AuthData.UserId, GGameIni);
	GConfig->SetString(TEXT("RailOptima.Api"), TEXT("Email"), *AuthData.Email, GGameIni);
	GConfig->SetString(TEXT("RailOptima.Api"), TEXT("Role"), *AuthData.Role, GGameIni);
	GConfig->SetBool(TEXT("RailOptima.Api"), TEXT("bIsAuthenticated"), AuthData.bIsAuthenticated, GGameIni);
	GConfig->Flush(false, GGameIni);
}

void URailApiService::LoadSessionFromConfig()
{
	GConfig->GetString(TEXT("RailOptima.Api"), TEXT("BaseUrl"), BaseUrl, GGameIni);
	if (BaseUrl.IsEmpty())
	{
		BaseUrl = TEXT("http://localhost:8080");
	}

	GConfig->GetString(TEXT("RailOptima.Api"), TEXT("Token"), AuthData.Token, GGameIni);
	GConfig->GetString(TEXT("RailOptima.Api"), TEXT("UserId"), AuthData.UserId, GGameIni);
	GConfig->GetString(TEXT("RailOptima.Api"), TEXT("Email"), AuthData.Email, GGameIni);
	GConfig->GetString(TEXT("RailOptima.Api"), TEXT("Role"), AuthData.Role, GGameIni);
	GConfig->GetBool(TEXT("RailOptima.Api"), TEXT("bIsAuthenticated"), AuthData.bIsAuthenticated, GGameIni);

	if (AuthData.Token.IsEmpty())
	{
		AuthData.bIsAuthenticated = false;
	}
}

TSharedRef<IHttpRequest> URailApiService::CreateAuthenticatedRequest(const FString& SubPath, const FString& Verb)
{
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	const FString CleanSubPath = SubPath.StartsWith(TEXT("/")) ? SubPath : (TEXT("/") + SubPath);
	Request->SetURL(BaseUrl + CleanSubPath);
	Request->SetVerb(Verb);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));

	if (IsAuthenticated())
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AuthData.Token));
	}
	return Request;
}

bool URailApiService::ParseJsonApiResponse(FHttpResponsePtr Response, TSharedPtr<FJsonObject>& OutData, FString& OutMessage, bool& OutSuccess)
{
	OutSuccess = false;
	OutMessage = TEXT("Неизвестный ответ сервера");

	if (!Response.IsValid())
	{
		OutMessage = TEXT("Сервер недоступен или соединение прервано");
		return false;
	}

	const int32 ResponseCode = Response->GetResponseCode();
	const FString Content = Response->GetContentAsString();

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		if (ResponseCode >= 200 && ResponseCode < 300)
		{
			OutSuccess = true;
			OutMessage = TEXT("Успешно");
			return true;
		}
		OutMessage = FString::Printf(TEXT("Ошибка сервера (HTTP %d)"), ResponseCode);
		return false;
	}

	Root->TryGetBoolField(TEXT("success"), OutSuccess);
	Root->TryGetStringField(TEXT("message"), OutMessage);

	const TSharedPtr<FJsonObject>* DataObj = nullptr;
	if (Root->TryGetObjectField(TEXT("data"), DataObj) && DataObj)
	{
		OutData = *DataObj;
	}

	// Если поле success явно не указано, ориентируемся на HTTP-код
	if (!Root->HasField(TEXT("success")))
	{
		OutSuccess = (ResponseCode >= 200 && ResponseCode < 300);
	}

	if (!OutSuccess && OutMessage.IsEmpty())
	{
		OutMessage = FString::Printf(TEXT("Ошибка запроса (HTTP %d)"), ResponseCode);
	}

	return OutSuccess;
}

FRailCloudProject URailApiService::ParseProjectJsonObject(const TSharedPtr<FJsonObject>& JsonObj)
{
	FRailCloudProject Proj;
	if (!JsonObj.IsValid())
	{
		return Proj;
	}

	JsonObj->TryGetStringField(TEXT("id"), Proj.Id);
	JsonObj->TryGetStringField(TEXT("name"), Proj.Name);
	JsonObj->TryGetStringField(TEXT("description"), Proj.Description);
	JsonObj->TryGetStringField(TEXT("status"), Proj.StatusString);
	JsonObj->TryGetStringField(TEXT("ownerId"), Proj.OwnerId);
	JsonObj->TryGetStringField(TEXT("createdAt"), Proj.CreatedAt);
	JsonObj->TryGetStringField(TEXT("updatedAt"), Proj.UpdatedAt);

	if (Proj.StatusString == TEXT("COMPLETED"))
	{
		Proj.Status = ERailCloudProjectStatus::Completed;
	}
	else if (Proj.StatusString == TEXT("IN_PROGRESS"))
	{
		Proj.Status = ERailCloudProjectStatus::InProgress;
	}
	else
	{
		Proj.Status = ERailCloudProjectStatus::Created;
	}

	return Proj;
}

// ─────────────────────────────────────────────────────────────────
//  Аутентификация
// ─────────────────────────────────────────────────────────────────

void URailApiService::Register(const FString& Email, const FString& Password)
{
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BaseUrl + TEXT("/api/v1/auth/register"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetStringField(TEXT("email"), Email);
	JsonObj->SetStringField(TEXT("password"), Password);

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);
	Request->SetContentAsString(Body);

	Request->OnProcessRequestComplete().BindUObject(this, &URailApiService::HandleRegisterResponse);
	Request->ProcessRequest();

	UE_LOG(LogTemp, Log, TEXT("[RailAPI] Отправка запроса регистрации: %s"), *Email);
}

void URailApiService::HandleRegisterResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		OnAuthCompleted.Broadcast(false, TEXT("Не удалось подключиться к серверу API (проверьте URL и запуск Backend)"), TEXT(""), TEXT(""));
		return;
	}

	TSharedPtr<FJsonObject> DataObj;
	FString Message;
	bool bSuccess = false;
	ParseJsonApiResponse(Response, DataObj, Message, bSuccess);

	if (bSuccess && DataObj.IsValid())
	{
		DataObj->TryGetStringField(TEXT("token"), AuthData.Token);
		DataObj->TryGetStringField(TEXT("userId"), AuthData.UserId);
		DataObj->TryGetStringField(TEXT("email"), AuthData.Email);
		DataObj->TryGetStringField(TEXT("role"), AuthData.Role);
		AuthData.bIsAuthenticated = true;
		SaveSessionToConfig();

		OnAuthCompleted.Broadcast(true, TEXT("Регистрация успешна!"), AuthData.Email, AuthData.Role);
	}
	else
	{
		OnAuthCompleted.Broadcast(false, Message, TEXT(""), TEXT(""));
	}
}

void URailApiService::Login(const FString& Email, const FString& Password)
{
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BaseUrl + TEXT("/api/v1/auth/login"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetStringField(TEXT("email"), Email);
	JsonObj->SetStringField(TEXT("password"), Password);

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);
	Request->SetContentAsString(Body);

	Request->OnProcessRequestComplete().BindUObject(this, &URailApiService::HandleLoginResponse);
	Request->ProcessRequest();

	UE_LOG(LogTemp, Log, TEXT("[RailAPI] Отправка запроса входа: %s"), *Email);
}

void URailApiService::HandleLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		OnAuthCompleted.Broadcast(false, TEXT("Не удалось связаться с сервером API (проверьте URL и запуск Backend)"), TEXT(""), TEXT(""));
		return;
	}

	TSharedPtr<FJsonObject> DataObj;
	FString Message;
	bool bSuccess = false;
	ParseJsonApiResponse(Response, DataObj, Message, bSuccess);

	if (bSuccess && DataObj.IsValid())
	{
		DataObj->TryGetStringField(TEXT("token"), AuthData.Token);
		DataObj->TryGetStringField(TEXT("userId"), AuthData.UserId);
		DataObj->TryGetStringField(TEXT("email"), AuthData.Email);
		DataObj->TryGetStringField(TEXT("role"), AuthData.Role);
		AuthData.bIsAuthenticated = true;
		SaveSessionToConfig();

		OnAuthCompleted.Broadcast(true, TEXT("Вход выполнен успешно!"), AuthData.Email, AuthData.Role);

		// Автоматически запрашиваем список облачных проектов
		FetchProjects(0, 50);
	}
	else
	{
		OnAuthCompleted.Broadcast(false, Message, TEXT(""), TEXT(""));
	}
}

void URailApiService::Logout()
{
	AuthData = FRailAuthData();
	CachedCloudProjects.Empty();
	SaveSessionToConfig();
	OnAuthCompleted.Broadcast(false, TEXT("Вы вышли из системы"), TEXT(""), TEXT(""));
	UE_LOG(LogTemp, Log, TEXT("[RailAPI] Пользователь вышел из системы"));
}

// ─────────────────────────────────────────────────────────────────
//  Проекты (REST /api/v1/projects)
// ─────────────────────────────────────────────────────────────────

void URailApiService::FetchProjects(int32 Page, int32 Size)
{
	if (!IsAuthenticated())
	{
		OnProjectsListReceived.Broadcast(false, CachedCloudProjects, TEXT("Требуется авторизация в облаке"));
		return;
	}

	const FString Path = FString::Printf(TEXT("/api/v1/projects?page=%d&size=%d"), Page, Size);
	TSharedRef<IHttpRequest> Request = CreateAuthenticatedRequest(Path, TEXT("GET"));
	Request->OnProcessRequestComplete().BindUObject(this, &URailApiService::HandleGetProjectsResponse);
	Request->ProcessRequest();
}

void URailApiService::HandleGetProjectsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		OnProjectsListReceived.Broadcast(false, CachedCloudProjects, TEXT("Ошибка соединения с сервером"));
		return;
	}

	const FString Content = Response->GetContentAsString();
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OnProjectsListReceived.Broadcast(false, CachedCloudProjects, TEXT("Некорректный JSON ответа проектов"));
		return;
	}

	bool bSuccess = false;
	FString Message;
	Root->TryGetBoolField(TEXT("success"), bSuccess);
	Root->TryGetStringField(TEXT("message"), Message);

	TArray<FRailCloudProject> LoadedProjects;

	const TSharedPtr<FJsonObject>* DataObj = nullptr;
	if (Root->TryGetObjectField(TEXT("data"), DataObj) && DataObj)
	{
		const TArray<TSharedPtr<FJsonValue>>* ContentArray = nullptr;
		if ((*DataObj)->TryGetArrayField(TEXT("content"), ContentArray) && ContentArray)
		{
			for (const TSharedPtr<FJsonValue>& Val : *ContentArray)
			{
				if (Val.IsValid())
				{
					TSharedPtr<FJsonObject> ItemObj = Val->AsObject();
					if (ItemObj.IsValid())
					{
						LoadedProjects.Add(ParseProjectJsonObject(ItemObj));
					}
				}
			}
		}
	}

	CachedCloudProjects = LoadedProjects;
	OnProjectsListReceived.Broadcast(true, CachedCloudProjects, TEXT("Список проектов получен"));
	UE_LOG(LogTemp, Log, TEXT("[RailAPI] Получено проектов из облака: %d"), CachedCloudProjects.Num());
}

void URailApiService::CreateRemoteProject(const FString& Name, const FString& Description)
{
	if (!IsAuthenticated())
	{
		OnProjectActionCompleted.Broadcast(false, FRailCloudProject(), TEXT("Требуется авторизация"));
		return;
	}

	TSharedRef<IHttpRequest> Request = CreateAuthenticatedRequest(TEXT("/api/v1/projects"), TEXT("POST"));

	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetStringField(TEXT("name"), Name);
	JsonObj->SetStringField(TEXT("description"), Description);

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);
	Request->SetContentAsString(Body);

	Request->OnProcessRequestComplete().BindUObject(this, &URailApiService::HandleCreateProjectResponse);
	Request->ProcessRequest();
}

void URailApiService::HandleCreateProjectResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> DataObj;
	FString Message;
	bool bSuccess = false;
	ParseJsonApiResponse(Response, DataObj, Message, bSuccess);

	if (bSuccess && DataObj.IsValid())
	{
		FRailCloudProject NewProj = ParseProjectJsonObject(DataObj);
		CachedCloudProjects.Insert(NewProj, 0);
		OnProjectActionCompleted.Broadcast(true, NewProj, TEXT("Проект успешно создан в облаке"));
	}
	else
	{
		OnProjectActionCompleted.Broadcast(false, FRailCloudProject(), Message);
	}
}

void URailApiService::GetRemoteProject(const FString& ProjectId)
{
	if (!IsAuthenticated())
	{
		OnProjectActionCompleted.Broadcast(false, FRailCloudProject(), TEXT("Требуется авторизация"));
		return;
	}

	const FString Path = FString::Printf(TEXT("/api/v1/projects/%s"), *ProjectId);
	TSharedRef<IHttpRequest> Request = CreateAuthenticatedRequest(Path, TEXT("GET"));
	Request->OnProcessRequestComplete().BindUObject(this, &URailApiService::HandleCreateProjectResponse); // Формат идентичен
	Request->ProcessRequest();
}

void URailApiService::UpdateRemoteProject(const FString& ProjectId, const FString& Name, const FString& Description, ERailCloudProjectStatus Status)
{
	if (!IsAuthenticated())
	{
		OnProjectActionCompleted.Broadcast(false, FRailCloudProject(), TEXT("Требуется авторизация"));
		return;
	}

	const FString Path = FString::Printf(TEXT("/api/v1/projects/%s"), *ProjectId);
	TSharedRef<IHttpRequest> Request = CreateAuthenticatedRequest(Path, TEXT("PUT"));

	FString StatusStr = TEXT("CREATED");
	if (Status == ERailCloudProjectStatus::InProgress) StatusStr = TEXT("IN_PROGRESS");
	else if (Status == ERailCloudProjectStatus::Completed) StatusStr = TEXT("COMPLETED");

	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetStringField(TEXT("name"), Name);
	JsonObj->SetStringField(TEXT("description"), Description);
	JsonObj->SetStringField(TEXT("status"), StatusStr);

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);
	Request->SetContentAsString(Body);

	Request->OnProcessRequestComplete().BindUObject(this, &URailApiService::HandleUpdateProjectResponse);
	Request->ProcessRequest();
}

void URailApiService::HandleUpdateProjectResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> DataObj;
	FString Message;
	bool bSuccess = false;
	ParseJsonApiResponse(Response, DataObj, Message, bSuccess);

	if (bSuccess && DataObj.IsValid())
	{
		FRailCloudProject UpdatedProj = ParseProjectJsonObject(DataObj);
		for (FRailCloudProject& P : CachedCloudProjects)
		{
			if (P.Id == UpdatedProj.Id)
			{
				P = UpdatedProj;
				break;
			}
		}
		OnProjectActionCompleted.Broadcast(true, UpdatedProj, TEXT("Проект успешно обновлен в облаке"));
	}
	else
	{
		OnProjectActionCompleted.Broadcast(false, FRailCloudProject(), Message);
	}
}

void URailApiService::DeleteRemoteProject(const FString& ProjectId)
{
	if (!IsAuthenticated())
	{
		OnProjectDeleted.Broadcast(false, TEXT("Требуется авторизация"));
		return;
	}

	const FString Path = FString::Printf(TEXT("/api/v1/projects/%s"), *ProjectId);
	TSharedRef<IHttpRequest> Request = CreateAuthenticatedRequest(Path, TEXT("DELETE"));
	Request->OnProcessRequestComplete().BindUObject(this, &URailApiService::HandleDeleteProjectResponse, ProjectId);
	Request->ProcessRequest();
}

void URailApiService::HandleDeleteProjectResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, FString ProjectId)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		OnProjectDeleted.Broadcast(false, TEXT("Ошибка связи с сервером при удалении проекта"));
		return;
	}

	const int32 Code = Response->GetResponseCode();
	if (Code == 204 || Code == 200)
	{
		CachedCloudProjects.RemoveAll([&](const FRailCloudProject& P) { return P.Id == ProjectId; });
		OnProjectDeleted.Broadcast(true, TEXT("Проект успешно удален из облака"));
	}
	else
	{
		OnProjectDeleted.Broadcast(false, FString::Printf(TEXT("Не удалось удалить проект (код %d)"), Code));
	}
}

void URailApiService::SyncLocalProjectToCloud(const FRailProjectMetadata& LocalProject)
{
	FString Desc = FString::Printf(TEXT("Категория: %s. Длина: %.0f м. Смета: %.1f тыс. руб."),
		*LocalProject.CategoryName, LocalProject.TotalLength_m, LocalProject.TotalCost_kRub);
	CreateRemoteProject(LocalProject.ProjectName, Desc);
}

// ─────────────────────────────────────────────────────────────────
