// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Модель данных проекта RailOptima

#pragma once

#include "Algorithms/RailCategories.h"
#include "CoreMinimal.h"
#include "RailProjectTypes.generated.h"

USTRUCT(BlueprintType)
struct FRailProjectMetadata
{
	GENERATED_BODY()

	// Название проекта, заданное пользователем
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	FString ProjectName = TEXT("Новый проект");

	// Имя папки внутри Projects/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	FString ProjectDirectoryName;

	// Полный путь к папке проекта
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	FString ProjectPath;

	// Дата и время создания
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	FDateTime CreatedAt = FDateTime::Now();

	// Дата и время последнего изменения
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	FDateTime LastModified = FDateTime::Now();

	// Категория железнодорожной линии (СП 119.13330)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	ERailRoadCategory Category = ERailRoadCategory::Category1;

	// Отображаемое имя категории
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	FString CategoryName = TEXT("I Категория");

	// Включена ли дорожная сеть
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	bool bHasRoads = false;

	// Включена ли гидрография
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	bool bHasRivers = false;

	// Полные или относительные пути к файлам данных
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	FString HeightmapPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	FString MetaPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	FString RoadsPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	FString RiversPath;

	// Сохраненные координаты начальной и конечной станций
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	bool bHasSavedEndpoints = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	FVector StartPoint = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	FVector EndPoint = FVector::ZeroVector;

	// Последняя рассчитанная протяженность (м) и смета (тыс. руб.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	double TotalLength_m = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Project")
	double TotalCost_kRub = 0.0;
};
