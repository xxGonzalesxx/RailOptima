// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#include "Core/HeightInterpolator.h"

// ─────────────────────────────────────────────────────────────────
//  Инициализация
// ─────────────────────────────────────────────────────────────────

void UHeightInterpolator::Initialize(const FRailHeightMapData& InMapData)
{
	MapData = InMapData;
	ElevationRange_m = MapData.MaxElevation_m - MapData.MinElevation_m;

	const int32 ExpectedSize = MapData.Width * MapData.Height;
	if (MapData.Width <= 0 || MapData.Height <= 0 || MapData.Heights.Num() != ExpectedSize)
	{
		UE_LOG(LogTemp, Error, TEXT("[HeightInterpolator] Некорректные данные карты: %dx%d, пикселей %d."),
			MapData.Width, MapData.Height, MapData.Heights.Num());
		bInitialized = false;
		return;
	}

	bInitialized = true;
	UE_LOG(LogTemp, Log, TEXT("[HeightInterpolator] Инициализация %dx%d, размах %.1f м."),
		MapData.Width, MapData.Height, ElevationRange_m);
}

void UHeightInterpolator::SetWorldMapping(const FVector2D& InWorldOrigin, const FVector2D& InWorldSizeXY, float InWorldSizeZ)
{
	WorldOrigin = InWorldOrigin;
	WorldSizeXY = (InWorldSizeXY.X > KINDA_SMALL_NUMBER && InWorldSizeXY.Y > KINDA_SMALL_NUMBER)
		? InWorldSizeXY
		: FVector2D(1.0f, 1.0f);
	WorldSizeZ = FMath::Max(InWorldSizeZ, KINDA_SMALL_NUMBER);
}

// ─────────────────────────────────────────────────────────────────
//  Геттеры
// ─────────────────────────────────────────────────────────────────

float UHeightInterpolator::GetHeightNormalized(float InNormX, float InNormY) const
{
	if (!bInitialized)
	{
		return 0.0f;
	}

	const float PixelX = FMath::Clamp(InNormX, 0.0f, 1.0f) * static_cast<float>(MapData.Width - 1);
	const float PixelY = FMath::Clamp(InNormY, 0.0f, 1.0f) * static_cast<float>(MapData.Height - 1);

	return SampleBilinear(PixelX, PixelY);
}

float UHeightInterpolator::GetHeightPixel(float InPixelX, float InPixelY) const
{
	if (!bInitialized)
	{
		return 0.0f;
	}

	const float PixelX = FMath::Clamp(InPixelX, 0.0f, static_cast<float>(MapData.Width - 1));
	const float PixelY = FMath::Clamp(InPixelY, 0.0f, static_cast<float>(MapData.Height - 1));

	return SampleBilinear(PixelX, PixelY);
}

float UHeightInterpolator::GetHeightWorld(const FVector& InWorldLocation) const
{
	if (!bInitialized)
	{
		return 0.0f;
	}

	const FVector2D Pixel = WorldToPixel(InWorldLocation);
	return SampleBilinear(Pixel.X, Pixel.Y);
}

float UHeightInterpolator::GetSlopeRatio(const FVector& InFrom, const FVector& InTo) const
{
	const float HeightFrom = GetHeightWorld(InFrom);
	const float HeightTo = GetHeightWorld(InTo);

	const FVector2D DeltaXY(InTo.X - InFrom.X, InTo.Y - InFrom.Y);
	const float		DistanceXY = DeltaXY.Size();

	if (DistanceXY < KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	// Расстояние UE-единицы (см) → метры.
	const float DistanceXY_m = DistanceXY * 0.01f;
	return (HeightTo - HeightFrom) / DistanceXY_m;
}

float UHeightInterpolator::GetSlopeDegrees(const FVector& InFrom, const FVector& InTo) const
{
	const float Ratio = GetSlopeRatio(InFrom, InTo);
	return FMath::RadiansToDegrees(FMath::Atan(Ratio));
}

// ─────────────────────────────────────────────────────────────────
//  Внутренняя логика
// ─────────────────────────────────────────────────────────────────

float UHeightInterpolator::SampleBilinear(float InPixelX, float InPixelY) const
{
	const int32 X0 = FMath::FloorToInt(InPixelX);
	const int32 Y0 = FMath::FloorToInt(InPixelY);
	const int32 X1 = X0 + 1;
	const int32 Y1 = Y0 + 1;

	const float FracX = InPixelX - static_cast<float>(X0);
	const float FracY = InPixelY - static_cast<float>(Y0);

	const float H00 = GetPixelClamped(X0, Y0);
	const float H10 = GetPixelClamped(X1, Y0);
	const float H01 = GetPixelClamped(X0, Y1);
	const float H11 = GetPixelClamped(X1, Y1);

	const float H0 = FMath::Lerp(H00, H10, FracX);
	const float H1 = FMath::Lerp(H01, H11, FracX);
	const float NormHeight = FMath::Lerp(H0, H1, FracY);

	return MapData.MinElevation_m + NormHeight * ElevationRange_m;
}

float UHeightInterpolator::GetPixelClamped(int32 InX, int32 InY) const
{
	const int32 ClampedX = FMath::Clamp(InX, 0, MapData.Width - 1);
	const int32 ClampedY = FMath::Clamp(InY, 0, MapData.Height - 1);
	return MapData.Heights[ClampedY * MapData.Width + ClampedX];
}

FVector2D UHeightInterpolator::WorldToPixel(const FVector& InWorldLocation) const
{
	const float NormX = (InWorldLocation.X - WorldOrigin.X) / WorldSizeXY.X;
	const float NormY = (InWorldLocation.Y - WorldOrigin.Y) / WorldSizeXY.Y;

	return FVector2D(
		FMath::Clamp(NormX, 0.0f, 1.0f) * static_cast<float>(MapData.Width - 1),
		FMath::Clamp(NormY, 0.0f, 1.0f) * static_cast<float>(MapData.Height - 1));
}