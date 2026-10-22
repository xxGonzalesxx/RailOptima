// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль
// визуализации», ПривГУПС, 2026.

#pragma once

#include "Core/MapLoader.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralTerrain.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct FRailTerrainVisualParameters
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Terrain",
		meta = (ClampMin = "32", ClampMax = "4096", UIMin = "64",
			UIMax = "2048"))
	int32 MeshResolution = 512;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Terrain",
		meta = (ClampMin = "256", ClampMax = "4096"))
	int32 MaxMeshResolution = 512;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Terrain")
	float StepSize_m = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Terrain")
	float MinElevation_m = 505.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Terrain")
	float MaxElevation_m = 2982.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Terrain",
		meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float ZScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "RailOptima|Terrain|Color")
	FLinearColor LowColor = FLinearColor(0.15f, 0.35f, 0.15f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "RailOptima|Terrain|Color")
	FLinearColor MidColor = FLinearColor(0.5f, 0.45f, 0.35f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "RailOptima|Terrain|Color")
	FLinearColor HighColor = FLinearColor(0.95f, 0.95f, 0.98f, 1.0f);
};

/**
 * Процедурный рельеф из 16-бит PNG.
 *
 * Хранит кэш Z-координат вершин меша (MeshVertexHeightsZ_cm).
 * RoadRenderer использует его для посадки дорог по геометрии меша —
 * это точнее трейсов и не зависит от collision / async cooking.
 */
UCLASS()
class RAILOPTIMA_API AProceduralTerrain : public AActor
{
	GENERATED_BODY()

public:
	AProceduralTerrain();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Terrain")
	bool BuildFromHeightmap(const FString& InHeightMapPath);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Terrain")
	bool LoadTerrainFromFiles(const FString& InHeightmapFullPath, const FString& InMetaFullPath);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Terrain")
	void ClearTerrain();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "RailOptima|Terrain")
	void RebuildTerrain();

	UFUNCTION(BlueprintPure, Category = "RailOptima|Terrain")
	const FRailHeightMapData& GetHeightMapData() const { return CachedHeightMap; }

	UFUNCTION(BlueprintPure, Category = "RailOptima|Terrain")
	float GetMapSizeMeters() const
	{
		const int32 Segments = FMath::Max(TerrainParameters.MeshResolution - 1, 1);
		return Segments * TerrainParameters.StepSize_m;
	}

	// Меш ландшафта — нужен ARoadRenderer для трейсов и посадки дорог.
	UFUNCTION(BlueprintPure, Category = "RailOptima|Terrain")
	UProceduralMeshComponent* GetTerrainMesh() const { return TerrainMesh; }

	// ─────────────────────────────────────────────────────────────
	//  Посадка объектов на рельеф без трейсов
	// ─────────────────────────────────────────────────────────────

	// Кэш Z-координат вершин меша (в см), длина = MeshResolution².
	// Строка 0 = первая в массиве, как в BuildMeshData (Y_png=0..N-1).
	UFUNCTION(BlueprintPure, Category = "RailOptima|Terrain")
	const TArray<float>& GetMeshVertexHeightsZ_cm() const
	{
		return MeshVertexHeightsZ_cm;
	}

	UFUNCTION(BlueprintPure, Category = "RailOptima|Terrain")
	int32 GetMeshVertexResolution() const
	{
		return TerrainParameters.MeshResolution;
	}

	// Высота рельефа (Z, см) в мировой точке (X, Y). Билинейная интерполяция
	// по сетке меша. Возвращает true, если точка внутри меша.
	UFUNCTION(BlueprintPure, Category = "RailOptima|Terrain")
	bool SampleTerrainZ_cm(const FVector2D& InWorldXY, float& OutZ_cm) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Terrain")
	FString HeightmapPath = TEXT("RawContent/heightmap.png");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Terrain")
	FString RegionMetaPath = TEXT("RawContent/region_meta.json");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Terrain")
	FRailTerrainVisualParameters TerrainParameters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Terrain")
	TObjectPtr<UMaterialInterface> TerrainMaterial;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RailOptima|Terrain",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProceduralMeshComponent> TerrainMesh;

	FRailHeightMapData CachedHeightMap;

	// Z-координаты вершин меша (см). Заполняется в BuildFromHeightmap.
	TArray<float> MeshVertexHeightsZ_cm;

	int32 SourceWidthPx = 0;
	float SourcePixelSize_m = 30.0f;

	double PendingOriginLat = 0.0;
	double PendingOriginLon = 0.0;

	FLinearColor GetColorByHeight(float InNormalizedHeight) const;

	void BuildMeshData(const FRailHeightMapData& InHeightMap, int32 InResolution,
		TArray<FVector>& OutVertices, TArray<int32>& OutTriangles,
		TArray<FLinearColor>& OutColors,
		TArray<FVector>&	  OutNormals) const;

	bool LoadRegionMetadata();
	bool LoadRegionMetadataFromFile(const FString& InFullPath);

	static FString ResolveContentOrProjectPath(const FString& InRelativePath);
};