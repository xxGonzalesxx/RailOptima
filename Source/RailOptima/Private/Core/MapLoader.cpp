// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект 
// «Разработка программы проектирования железнодорожных путей: модуль вызуализации», 
// ПривГУПС, 2026.

#include "Core/MapLoader.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/FileHelper.h"

// ─────────────────────────────────────────────────────────────────
//  Инициализация
// ─────────────────────────────────────────────────────────────────

bool UMapLoader::LoadHeightMap(const FString& InFilePath, FRailHeightMapData& OutData)
{
	TArray<uint8> RawBytes;
	if (!FFileHelper::LoadFileToArray(RawBytes, *InFilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("[MapLoader] Не удалось прочитать файл: %s"), *InFilePath);
		return false;
	}

	return DecodeImage(RawBytes, OutData);
}

// ─────────────────────────────────────────────────────────────────
//  Внутренняя логика
// ─────────────────────────────────────────────────────────────────

bool UMapLoader::DecodeImage(const TArray<uint8>& InRawBytes, FRailHeightMapData& OutData)
{
	IImageWrapperModule& ImageWrapperModule =
		FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));

	TSharedPtr<IImageWrapper> Wrapper =
		ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	if (!Wrapper.IsValid() || !Wrapper->SetCompressed(InRawBytes.GetData(), InRawBytes.Num()))
	{
		UE_LOG(LogTemp, Error, TEXT("[MapLoader] PNG не распознан."));
		return false;
	}

	OutData.Width = Wrapper->GetWidth();
	OutData.Height = Wrapper->GetHeight();

	TArray64<uint8> RawData;
	if (!Wrapper->GetRaw(ERGBFormat::Gray, 16, RawData))
	{
		UE_LOG(LogTemp, Error, TEXT("[MapLoader] Не удалось получить 16-бит raw-данные."));
		return false;
	}

	const int32 NumPixels = OutData.Width * OutData.Height;
	OutData.Heights.SetNumUninitialized(NumPixels);

	const uint16* Ptr16 = reinterpret_cast<const uint16*>(RawData.GetData());
	for (int32 i = 0; i < NumPixels; ++i)
	{
		OutData.Heights[i] = static_cast<float>(Ptr16[i]) / 65535.0f;
	}

	UE_LOG(LogTemp, Log, TEXT("[MapLoader] Загружено %dx%d (16-bit)."), OutData.Width, OutData.Height);
	return true;
}