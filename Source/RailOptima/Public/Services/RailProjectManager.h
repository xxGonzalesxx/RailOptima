// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Менеджер проектов RailOptima (создание, хранение, сканирование, запуск)

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Services/RailProjectTypes.h"
#include "RailProjectManager.generated.h"

UCLASS()
class RAILOPTIMA_API URailProjectManager : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Путь к корневой директории всех локальных проектов (ProjectDir/Projects)
	UFUNCTION(BlueprintPure, Category = "RailOptima|Project")
	static FString GetProjectsRootDirectory();

	// Гарантирует наличие папки Projects и создает демо-проект при первом запуске
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Project")
	static void EnsureProjectsDirectoryExists();

	// Сканирует и возвращает список всех локальных проектов
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Project")
	static TArray<FRailProjectMetadata> GetAllProjects();

	// Загружает метаданные конкретного проекта из его папки
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Project")
	static bool LoadProject(const FString& InProjectDir, FRailProjectMetadata& OutProject);

	// Сохраняет метаданные проекта в project.json
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Project")
	static bool SaveProject(const FRailProjectMetadata& InProject);

	// Удаляет папку проекта
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Project")
	static bool DeleteProject(const FString& InProjectDir);

	// Открывает проводник Windows по указанному пути
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Project")
	static void OpenFolderInExplorer(const FString& InFolderFullPath);

	// Создает проект из готового 16-битного PNG файла карты высот
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Project")
	static bool CreateProjectFromPng(
		const FString&		 InProjectName,
		ERailRoadCategory	 InCategory,
		const FString&		 InSourcePngPath,
		bool				 bLoadRoads,
		bool				 bLoadRivers,
		FRailProjectMetadata& OutProject,
		FString&			 OutErrorMessage);

	// Создает проект из файла GeoTIFF (.tif / .tiff) с конвертацией через GDAL
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Project")
	static bool CreateProjectFromGeoTiff(
		const FString&		 InProjectName,
		ERailRoadCategory	 InCategory,
		const FString&		 InSourceTifPath,
		bool				 bLoadRoads,
		bool				 bLoadRivers,
		FRailProjectMetadata& OutProject,
		FString&			 OutErrorMessage);

	// Системный диалог открытия файлов Windows (выбор *.png или *.tif)
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Project")
	static bool OpenFileDialogWindows(
		const FString& InTitle,
		const FString& InFilter,
		FString&	   OutSelectedPath);

	// Преобразует название проекта в безопасное имя папки
	static FString MakeSafeFolderName(const FString& InName);
};
