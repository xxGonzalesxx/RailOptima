// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#include "Core/MapLoader.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"


bool UMapLoader::LoadHeightMap(const FString& InFilePath, FRailHeightMapData& OutData)
{
	TArray<uint8> RawFileBytes;
	if (!FFileHelper::LoadFileToArray(RawFileBytes, *InFilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("[UMapLoader] Не удалось прочитать файл по пути: %s"), *InFilePath);
		return false;
	}

	return DecodeImage(RawFileBytes, OutData);
}

bool UMapLoader::DecodeImage(const TArray<uint8>& InRawBytes, FRailHeightMapData& OutData)
{
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));

	// Автоматически определяем формат ( PNG / JPEG и т.д. )
	EImageFormat ImageFormat = ImageWrapperModule.DetectImageFormat(InRawBytes.GetData(), InRawBytes.Num());
	if (ImageFormat == EImageFormat::Invalid)
	{
		UE_LOG(LogTemp, Error, TEXT("[UMapLoader] Не удалось определить формат изображения карты высот."));
		return false;
	}

	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
	if (!ImageWrapper.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[UMapLoader] Не удалось создать IImageWrapper для формата изображения."));
		return false;
	}

	if (!ImageWrapper->SetCompressed(InRawBytes.GetData(), InRawBytes.Num()))
	{
		UE_LOG(LogTemp, Error, TEXT("[UMapLoader] Ошибка при передаче сжатых байтов в ImageWrapper."));
		return false;
	}

	// Запрашиваем 16-битные данные (ERGBFormat::Gray, EBitDepth::UInt16)
	TArray<uint8> UncompressedBytes;
	if (!ImageWrapper->GetRaw(ERGBFormat::Gray, 16, UncompressedBytes))
	{
		UE_LOG(LogTemp, Error, TEXT("[UMapLoader] Не удалось распаковать изображение в 16-бит Grayscale."));
		return false;
	}

	OutData.Width = ImageWrapper->GetWidth();
	OutData.Height = ImageWrapper->GetHeight();

	const int32	  TotalPixels = OutData.Width * OutData.Height;
	const uint16* PixelPtr = reinterpret_cast<const uint16*>(UncompressedBytes.GetData());

	OutData.Heights.SetNumUninitialized(TotalPixels);

	// Поиск минимального и максимального значения для нормализации 16-бит (0..65535)
	uint16 MinVal = MAX_uint16;
	uint16 MaxVal = 0;

	for (int32 i = 0; i < TotalPixels; ++i)
	{
		uint16 Val = PixelPtr[i];
		if (Val < MinVal)
			MinVal = Val;
		if (Val > MaxVal)
			MaxVal = Val;
	}

	const float Range = static_cast<float>(MaxVal - MinVal);

	// Заполняем нормализованный массив высот [0..1]
	for (int32 i = 0; i < TotalPixels; ++i)
	{
		if (Range > 0.0f)
		{
			OutData.Heights[i] = static_cast<float>(PixelPtr[i] - MinVal) / Range;
		}
		else
		{
			OutData.Heights[i] = 0.0f;
		}
	}

	// Примерные метаданные высот в метрах (можно переопределить позже привязкой)
	OutData.MinElevation_m = 0.0f;
	OutData.MaxElevation_m = 1000.0f; // Условный диапазон по умолчанию

	UE_LOG(LogTemp, Log, TEXT("[UMapLoader] Успешно загружена карта высот: %dx%d пикселей."), OutData.Width, OutData.Height);
	return true;
}