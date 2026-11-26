// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Реализация менеджера проектов RailOptima

#include "Services/RailProjectManager.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <commdlg.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

FString URailProjectManager::GetProjectsRootDirectory()
{
	const FString Dir = FPaths::ProjectDir() / TEXT("Projects");
	return FPaths::ConvertRelativePathToFull(Dir);
}

void URailProjectManager::EnsureProjectsDirectoryExists()
{
	const FString RootDir = GetProjectsRootDirectory();
	IFileManager& FM = IFileManager::Get();

	if (!FM.DirectoryExists(*RootDir))
	{
		FM.MakeDirectory(*RootDir, true);
	}

	// Создаем стартовый демо-проект, если список проектов пуст
	const FString DemoDir = RootDir / TEXT("Krasnodar_Demo");
	const FString DemoProjectJson = DemoDir / TEXT("project.json");

	if (!FM.FileExists(*DemoProjectJson))
	{
		FM.MakeDirectory(*DemoDir, true);

		const FString RawDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("RawContent"));
		const FString ContentRawDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("RawContent"));

		auto TryCopy = [&](const FString& FileName) {
			const FString Dst = DemoDir / FileName;
			if (FM.FileExists(*Dst))
				return;

			const FString Src1 = RawDir / FileName;
			const FString Src2 = ContentRawDir / FileName;
			if (FM.FileExists(*Src1))
			{
				FM.Copy(*Dst, *Src1);
			}
			else if (FM.FileExists(*Src2))
			{
				FM.Copy(*Dst, *Src2);
			}
		};

		TryCopy(TEXT("heightmap.png"));
		TryCopy(TEXT("region_meta.json"));
		TryCopy(TEXT("roads.geojson"));
		TryCopy(TEXT("rivers.geojson"));

		FRailProjectMetadata DemoMeta;
		DemoMeta.ProjectName = TEXT("Краснодар (Демо 30м)");
		DemoMeta.ProjectDirectoryName = TEXT("Krasnodar_Demo");
		DemoMeta.ProjectPath = DemoDir;
		DemoMeta.Category = ERailRoadCategory::Category1;
		DemoMeta.CategoryName = TEXT("I Категория");
		DemoMeta.bHasRoads = FM.FileExists(*(DemoDir / TEXT("roads.geojson")));
		DemoMeta.bHasRivers = FM.FileExists(*(DemoDir / TEXT("rivers.geojson")));
		DemoMeta.HeightmapPath = DemoDir / TEXT("heightmap.png");
		DemoMeta.MetaPath = DemoDir / TEXT("region_meta.json");
		DemoMeta.RoadsPath = DemoDir / TEXT("roads.geojson");
		DemoMeta.RiversPath = DemoDir / TEXT("rivers.geojson");

		SaveProject(DemoMeta);
		UE_LOG(LogTemp, Log, TEXT("[ProjectManager] Демо-проект Краснодара успешно инициализирован в: %s"), *DemoDir);
	}
}

TArray<FRailProjectMetadata> URailProjectManager::GetAllProjects()
{
	EnsureProjectsDirectoryExists();
	TArray<FRailProjectMetadata> Result;

	const FString RootDir = GetProjectsRootDirectory();
	IFileManager& FM = IFileManager::Get();

	TArray<FString> SubDirs;
	FM.FindFiles(SubDirs, *(RootDir / TEXT("*")), /*Files=*/false, /*Directories=*/true);

	for (const FString& DirName : SubDirs)
	{
		const FString FullSubDir = RootDir / DirName;
		const FString ProjectJson = FullSubDir / TEXT("project.json");

		if (FM.FileExists(*ProjectJson))
		{
			FRailProjectMetadata Meta;
			if (LoadProject(FullSubDir, Meta))
			{
				Result.Add(Meta);
			}
		}
	}

	// Сортировка: новые/недавно измененные проекты сверху
	Result.Sort([](const FRailProjectMetadata& A, const FRailProjectMetadata& B) {
		return A.LastModified > B.LastModified;
	});

	return Result;
}

bool URailProjectManager::LoadProject(const FString& InProjectDir, FRailProjectMetadata& OutProject)
{
	const FString JsonPath = InProjectDir / TEXT("project.json");
	FString RawJson;
	if (!FFileHelper::LoadFileToString(RawJson, *JsonPath))
	{
		return false;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RawJson);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	OutProject.ProjectPath = InProjectDir;
	OutProject.ProjectDirectoryName = FPaths::GetCleanFilename(InProjectDir);
	Root->TryGetStringField(TEXT("name"), OutProject.ProjectName);

	FString CreatedStr, ModifiedStr;
	if (Root->TryGetStringField(TEXT("created_at"), CreatedStr))
	{
		FDateTime::ParseIso8601(*CreatedStr, OutProject.CreatedAt);
	}
	if (Root->TryGetStringField(TEXT("last_modified"), ModifiedStr))
	{
		FDateTime::ParseIso8601(*ModifiedStr, OutProject.LastModified);
	}

	int32 CatInt = 3;
	Root->TryGetNumberField(TEXT("category"), CatInt);
	OutProject.Category = static_cast<ERailRoadCategory>(FMath::Clamp(CatInt, 0, 7));
	OutProject.CategoryName = FRailCategoryNorms::GetNorms(OutProject.Category).CategoryName;

	Root->TryGetBoolField(TEXT("has_roads"), OutProject.bHasRoads);
	Root->TryGetBoolField(TEXT("has_rivers"), OutProject.bHasRivers);

	FString RelH, RelM, RelRd, RelRv;
	Root->TryGetStringField(TEXT("heightmap_path"), RelH);
	Root->TryGetStringField(TEXT("meta_path"), RelM);
	Root->TryGetStringField(TEXT("roads_path"), RelRd);
	Root->TryGetStringField(TEXT("rivers_path"), RelRv);

	OutProject.HeightmapPath = FPaths::IsRelative(RelH) ? (InProjectDir / RelH) : RelH;
	OutProject.MetaPath = FPaths::IsRelative(RelM) ? (InProjectDir / RelM) : RelM;
	OutProject.RoadsPath = FPaths::IsRelative(RelRd) ? (InProjectDir / RelRd) : RelRd;
	OutProject.RiversPath = FPaths::IsRelative(RelRv) ? (InProjectDir / RelRv) : RelRv;

	Root->TryGetBoolField(TEXT("has_saved_endpoints"), OutProject.bHasSavedEndpoints);
	const TSharedPtr<FJsonObject>* StartObj = nullptr;
	if (Root->TryGetObjectField(TEXT("start_point"), StartObj) && StartObj)
	{
		double X = 0, Y = 0, Z = 0;
		(*StartObj)->TryGetNumberField(TEXT("x"), X);
		(*StartObj)->TryGetNumberField(TEXT("y"), Y);
		(*StartObj)->TryGetNumberField(TEXT("z"), Z);
		OutProject.StartPoint = FVector(X, Y, Z);
	}

	const TSharedPtr<FJsonObject>* EndObj = nullptr;
	if (Root->TryGetObjectField(TEXT("end_point"), EndObj) && EndObj)
	{
		double X = 0, Y = 0, Z = 0;
		(*EndObj)->TryGetNumberField(TEXT("x"), X);
		(*EndObj)->TryGetNumberField(TEXT("y"), Y);
		(*EndObj)->TryGetNumberField(TEXT("z"), Z);
		OutProject.EndPoint = FVector(X, Y, Z);
	}

	Root->TryGetNumberField(TEXT("total_length_m"), OutProject.TotalLength_m);
	Root->TryGetNumberField(TEXT("total_cost_krub"), OutProject.TotalCost_kRub);

	Root->TryGetStringField(TEXT("remote_id"), OutProject.RemoteId);
	Root->TryGetStringField(TEXT("description"), OutProject.Description);
	Root->TryGetStringField(TEXT("cloud_status"), OutProject.CloudStatus);
	Root->TryGetBoolField(TEXT("is_synced"), OutProject.bIsSyncedWithCloud);

	return true;
}

bool URailProjectManager::SaveProject(const FRailProjectMetadata& InProject)
{
	const FString JsonPath = InProject.ProjectPath / TEXT("project.json");

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("name"), InProject.ProjectName);
	Root->SetStringField(TEXT("created_at"), InProject.CreatedAt.ToIso8601());
	Root->SetStringField(TEXT("last_modified"), FDateTime::Now().ToIso8601());
	Root->SetNumberField(TEXT("category"), static_cast<int32>(InProject.Category));
	Root->SetStringField(TEXT("category_name"), InProject.CategoryName);
	Root->SetBoolField(TEXT("has_roads"), InProject.bHasRoads);
	Root->SetBoolField(TEXT("has_rivers"), InProject.bHasRivers);

	Root->SetStringField(TEXT("heightmap_path"), FPaths::GetCleanFilename(InProject.HeightmapPath));
	Root->SetStringField(TEXT("meta_path"), FPaths::GetCleanFilename(InProject.MetaPath));
	Root->SetStringField(TEXT("roads_path"), FPaths::GetCleanFilename(InProject.RoadsPath));
	Root->SetStringField(TEXT("rivers_path"), FPaths::GetCleanFilename(InProject.RiversPath));

	Root->SetBoolField(TEXT("has_saved_endpoints"), InProject.bHasSavedEndpoints);

	TSharedPtr<FJsonObject> StartObj = MakeShared<FJsonObject>();
	StartObj->SetNumberField(TEXT("x"), InProject.StartPoint.X);
	StartObj->SetNumberField(TEXT("y"), InProject.StartPoint.Y);
	StartObj->SetNumberField(TEXT("z"), InProject.StartPoint.Z);
	Root->SetObjectField(TEXT("start_point"), StartObj);

	TSharedPtr<FJsonObject> EndObj = MakeShared<FJsonObject>();
	EndObj->SetNumberField(TEXT("x"), InProject.EndPoint.X);
	EndObj->SetNumberField(TEXT("y"), InProject.EndPoint.Y);
	EndObj->SetNumberField(TEXT("z"), InProject.EndPoint.Z);
	Root->SetObjectField(TEXT("end_point"), EndObj);

	Root->SetNumberField(TEXT("total_length_m"), InProject.TotalLength_m);
	Root->SetNumberField(TEXT("total_cost_krub"), InProject.TotalCost_kRub);

	Root->SetStringField(TEXT("remote_id"), InProject.RemoteId);
	Root->SetStringField(TEXT("description"), InProject.Description);
	Root->SetStringField(TEXT("cloud_status"), InProject.CloudStatus);
	Root->SetBoolField(TEXT("is_synced"), InProject.bIsSyncedWithCloud);

	FString OutputString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	if (FJsonSerializer::Serialize(Root.ToSharedRef(), Writer))
	{
		return FFileHelper::SaveStringToFile(OutputString, *JsonPath);
	}
	return false;
}

bool URailProjectManager::DeleteProject(const FString& InProjectDir)
{
	IFileManager& FM = IFileManager::Get();
	if (FM.DirectoryExists(*InProjectDir))
	{
		return FM.DeleteDirectory(*InProjectDir, false, true);
	}
	return false;
}

void URailProjectManager::OpenFolderInExplorer(const FString& InFolderFullPath)
{
	FPlatformProcess::ExploreFolder(*InFolderFullPath);
}

FString URailProjectManager::MakeSafeFolderName(const FString& InName)
{
	FString Safe = InName.TrimStartAndEnd();
	Safe.ReplaceInline(TEXT(" "), TEXT("_"));
	Safe.ReplaceInline(TEXT("/"), TEXT("_"));
	Safe.ReplaceInline(TEXT("\\"), TEXT("_"));
	Safe.ReplaceInline(TEXT(":"), TEXT("_"));
	Safe.ReplaceInline(TEXT("*"), TEXT("_"));
	Safe.ReplaceInline(TEXT("?"), TEXT("_"));
	Safe.ReplaceInline(TEXT("\""), TEXT("_"));
	Safe.ReplaceInline(TEXT("<"), TEXT("_"));
	Safe.ReplaceInline(TEXT(">"), TEXT("_"));
	Safe.ReplaceInline(TEXT("|"), TEXT("_"));

	if (Safe.IsEmpty())
	{
		Safe = FString::Printf(TEXT("Project_%s"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	}
	return Safe;
}

bool URailProjectManager::CreateProjectFromPng(
	const FString&		 InProjectName,
	ERailRoadCategory	 InCategory,
	const FString&		 InSourcePngPath,
	bool				 bLoadRoads,
	bool				 bLoadRivers,
	FRailProjectMetadata& OutProject,
	FString&			 OutErrorMessage)
{
	IFileManager& FM = IFileManager::Get();
	if (!FM.FileExists(*InSourcePngPath))
	{
		OutErrorMessage = FString::Printf(TEXT("Файл карты высот PNG не найден: %s"), *InSourcePngPath);
		return false;
	}

	EnsureProjectsDirectoryExists();
	const FString SafeName = MakeSafeFolderName(InProjectName);
	const FString ProjectDir = GetProjectsRootDirectory() / SafeName;

	if (!FM.DirectoryExists(*ProjectDir))
	{
		FM.MakeDirectory(*ProjectDir, true);
	}

	FString FullSourcePath = FPaths::ConvertRelativePathToFull(InSourcePngPath);
	FPaths::NormalizeFilename(FullSourcePath);

	FString DestPng = ProjectDir / TEXT("heightmap.png");
	FPaths::NormalizeFilename(DestPng);

	if (FullSourcePath != DestPng)
	{
		const uint32 CopyResult = FM.Copy(*DestPng, *FullSourcePath, /*Replace=*/true, /*EvenIfReadOnly=*/true);
		if (CopyResult != 0)
		{
			OutErrorMessage = FString::Printf(TEXT("Не удалось скопировать файл карты высот в папку проекта (код %u): %s"), CopyResult, *FullSourcePath);
			return false;
		}
	}

	// Читаем размеры PNG из заголовка (байты 16-23: Width и Height в Big-Endian)
	int32 PngWidth = 3600;
	int32 PngHeight = 3600;
	TArray<uint8> PngHeader;
	if (FFileHelper::LoadFileToArray(PngHeader, *DestPng, FILEREAD_Silent))
	{
		if (PngHeader.Num() >= 24 && PngHeader[0] == 0x89 && PngHeader[1] == 'P' && PngHeader[2] == 'N' && PngHeader[3] == 'G')
		{
			PngWidth = (PngHeader[16] << 24) | (PngHeader[17] << 16) | (PngHeader[18] << 8) | PngHeader[19];
			PngHeight = (PngHeader[20] << 24) | (PngHeader[21] << 16) | (PngHeader[22] << 8) | PngHeader[23];
		}
	}

	// Копируем или создаем region_meta.json
	const FString DestMeta = ProjectDir / TEXT("region_meta.json");
	const FString SourceMetaNearby = FPaths::GetPath(FullSourcePath) / TEXT("region_meta.json");

	if (FM.FileExists(*SourceMetaNearby))
	{
		FM.Copy(*DestMeta, *SourceMetaNearby, true, true);
	}
	else
	{
		FString DefaultMeta = FString::Printf(
			TEXT("{\n  \"region_name\": \"%s\",\n  \"min_elevation_m\": 0.0,\n  \"max_elevation_m\": 1500.0,\n  \"pixel_size_m\": 30.0,\n  \"width_px\": %d,\n  \"height_px\": %d,\n  \"bbox\": {\n    \"west\": 38.85,\n    \"south\": 44.95,\n    \"east\": 39.25,\n    \"north\": 45.35\n  }\n}"),
			*InProjectName, PngWidth, PngHeight);
		FFileHelper::SaveStringToFile(DefaultMeta, *DestMeta, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	// Копируем дороги и реки, если они есть рядом или берем дефолтные из RawContent
	const FString DestRoads = ProjectDir / TEXT("roads.geojson");
	if (bLoadRoads)
	{
		const FString SourceRoadsNearby = FPaths::GetPath(FullSourcePath) / TEXT("roads.geojson");
		if (FM.FileExists(*SourceRoadsNearby))
		{
			FM.Copy(*DestRoads, *SourceRoadsNearby, true, true);
		}
		else
		{
			const FString FallbackRoads = FPaths::ProjectDir() / TEXT("RawContent/roads.geojson");
			if (FM.FileExists(*FallbackRoads))
			{
				FM.Copy(*DestRoads, *FallbackRoads, true, true);
			}
		}

		if (!FM.FileExists(*DestRoads))
		{
			// Автоматическая генерация опорной дорожной сети региона
			const FString DefaultRoadsJson = TEXT("{\n  \"type\": \"FeatureCollection\",\n  \"features\": [\n    {\n      \"type\": \"Feature\",\n      \"properties\": {\"name\": \"Highway M-4 West-East\"},\n      \"geometry\": {\n        \"type\": \"LineString\",\n        \"coordinates\": [[38.86, 45.03], [38.92, 45.035], [38.98, 45.04], [39.05, 45.042], [39.12, 45.045], [39.24, 45.048]]\n      }\n    },\n    {\n      \"type\": \"Feature\",\n      \"properties\": {\"name\": \"North Bypass\"},\n      \"geometry\": {\n        \"type\": \"LineString\",\n        \"coordinates\": [[38.88, 45.22], [38.96, 45.25], [39.08, 45.24], [39.22, 45.19]]\n      }\n    },\n    {\n      \"type\": \"Feature\",\n      \"properties\": {\"name\": \"North-South Route\"},\n      \"geometry\": {\n        \"type\": \"LineString\",\n        \"coordinates\": [[39.02, 44.96], [39.015, 45.04], [39.02, 45.16], [39.03, 45.33]]\n      }\n    }\n  ]\n}");
			FFileHelper::SaveStringToFile(DefaultRoadsJson, *DestRoads, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		}
	}

	const FString DestRivers = ProjectDir / TEXT("rivers.geojson");
	if (bLoadRivers)
	{
		const FString SourceRiversNearby = FPaths::GetPath(FullSourcePath) / TEXT("rivers.geojson");
		if (FM.FileExists(*SourceRiversNearby))
		{
			FM.Copy(*DestRivers, *SourceRiversNearby, true, true);
		}
		else
		{
			const FString FallbackRivers = FPaths::ProjectDir() / TEXT("RawContent/rivers.geojson");
			if (FM.FileExists(*FallbackRivers))
			{
				FM.Copy(*DestRivers, *FallbackRivers, true, true);
			}
		}

		if (!FM.FileExists(*DestRivers))
		{
			// Автоматическая генерация русла реки с притоком
			const FString DefaultRiversJson = TEXT("{\n  \"type\": \"FeatureCollection\",\n  \"features\": [\n    {\n      \"type\": \"Feature\",\n      \"properties\": {\"name\": \"Kuban River Main\"},\n      \"geometry\": {\n        \"type\": \"LineString\",\n        \"coordinates\": [[38.85, 45.01], [38.92, 45.012], [39.01, 45.022], [39.09, 45.028], [39.19, 45.035], [39.25, 45.03]]\n      }\n    },\n    {\n      \"type\": \"Feature\",\n      \"properties\": {\"name\": \"North Stream Tributary\"},\n      \"geometry\": {\n        \"type\": \"LineString\",\n        \"coordinates\": [[39.06, 45.28], [39.05, 45.21], [39.065, 45.14], [39.05, 45.015]]\n      }\n    }\n  ]\n}");
			FFileHelper::SaveStringToFile(DefaultRiversJson, *DestRivers, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		}
	}

	OutProject.ProjectName = InProjectName;
	OutProject.ProjectDirectoryName = SafeName;
	OutProject.ProjectPath = ProjectDir;
	OutProject.CreatedAt = FDateTime::Now();
	OutProject.LastModified = FDateTime::Now();
	OutProject.Category = InCategory;
	OutProject.CategoryName = FRailCategoryNorms::GetNorms(InCategory).CategoryName;
	OutProject.bHasRoads = bLoadRoads && FM.FileExists(*DestRoads);
	OutProject.bHasRivers = bLoadRivers && FM.FileExists(*DestRivers);
	OutProject.HeightmapPath = DestPng;
	OutProject.MetaPath = DestMeta;
	OutProject.RoadsPath = DestRoads;
	OutProject.RiversPath = DestRivers;

	if (!SaveProject(OutProject))
	{
		OutErrorMessage = TEXT("Не удалось сохранить project.json.");
		return false;
	}

	return true;
}

bool URailProjectManager::CreateProjectFromGeoTiff(
	const FString&		 InProjectName,
	ERailRoadCategory	 InCategory,
	const FString&		 InSourceTifPath,
	bool				 bLoadRoads,
	bool				 bLoadRivers,
	FRailProjectMetadata& OutProject,
	FString&			 OutErrorMessage)
{
	IFileManager& FM = IFileManager::Get();
	if (!FM.FileExists(*InSourceTifPath))
	{
		OutErrorMessage = FString::Printf(TEXT("Файл GeoTIFF не найден: %s"), *InSourceTifPath);
		return false;
	}

	EnsureProjectsDirectoryExists();
	const FString SafeName = MakeSafeFolderName(InProjectName);
	const FString ProjectDir = GetProjectsRootDirectory() / SafeName;
	FM.MakeDirectory(*ProjectDir, true);

	const FString ScriptPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Scripts/process_project.py"));
	if (!FM.FileExists(*ScriptPath))
	{
		OutErrorMessage = FString::Printf(TEXT("Скрипт обработки не найден: %s"), *ScriptPath);
		return false;
	}

	// Формируем аргументы запуска python
	FString Args = FString::Printf(TEXT("\"%s\" --dem \"%s\" --output \"%s\" --name \"%s\" --category %d"),
		*ScriptPath, *InSourceTifPath, *ProjectDir, *InProjectName, static_cast<int32>(InCategory));

	if (bLoadRoads)
		Args += TEXT(" --roads");
	if (bLoadRivers)
		Args += TEXT(" --rivers");

	UE_LOG(LogTemp, Log, TEXT("[ProjectManager] Запуск обработки GeoTIFF: python %s"), *Args);

	int32 ReturnCode = 0;
	FString StdOut, StdErr;
	bool bProcSuccess = FPlatformProcess::ExecProcess(TEXT("python"), *Args, &ReturnCode, &StdOut, &StdErr);

	if (!bProcSuccess || ReturnCode != 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[ProjectManager] Ошибка ExecProcess: code=%d\nSTDOUT: %s\nSTDERR: %s"),
			ReturnCode, *StdOut, *StdErr);
		OutErrorMessage = FString::Printf(TEXT("Ошибка обработки GeoTIFF (код %d): %s"), ReturnCode, *StdErr.Left(300));
		return false;
	}

	// Загружаем созданный проект
	if (!LoadProject(ProjectDir, OutProject))
	{
		OutErrorMessage = TEXT("Файлы проекта были сгенерированы, но не удалось прочитать project.json.");
		return false;
	}

	return true;
}

bool URailProjectManager::OpenFileDialogWindows(
	const FString& InTitle,
	const FString& InFilter,
	FString&	   OutSelectedPath)
{
#if PLATFORM_WINDOWS
	OPENFILENAMEW ofn;
	wchar_t szFile[MAX_PATH] = { 0 };

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);

	// Преобразуем фильтр (например, "Карты высот (*.png;*.tif;*.tiff)|*.png;*.tif;*.tiff|Все файлы (*.*)|*.*")
	// в формат Win32: двойной нуль-терминированный буфер
	FString FormattedFilter = InFilter;
	if (FormattedFilter.IsEmpty())
	{
		FormattedFilter = TEXT("Поддерживаемые файлы (*.png;*.tif;*.tiff)|*.png;*.tif;*.tiff|Все файлы (*.*)|*.*");
	}

	TArray<wchar_t> FilterBuffer;
	FilterBuffer.Reserve(FormattedFilter.Len() + 4);
	for (TCHAR Ch : FormattedFilter)
	{
		if (Ch == TEXT('|'))
		{
			FilterBuffer.Add(L'\0');
		}
		else
		{
			FilterBuffer.Add(static_cast<wchar_t>(Ch));
		}
	}
	FilterBuffer.Add(L'\0');
	FilterBuffer.Add(L'\0');

	ofn.lpstrFilter = FilterBuffer.GetData();
	ofn.nFilterIndex = 1;

	const FString SafeTitle = InTitle.IsEmpty() ? TEXT("Выберите файл") : InTitle;
	ofn.lpstrTitle = *SafeTitle;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetOpenFileNameW(&ofn))
	{
		OutSelectedPath = FString(szFile);
		OutSelectedPath.ReplaceInline(TEXT("\\"), TEXT("/"));
		return true;
	}
#endif
	return false;
}
