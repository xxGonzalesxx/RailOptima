// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/MapLoader.h"
#include "ProceduralMeshComponent.h"
#include "RoadRenderer.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class AProceduralTerrain;

UENUM(BlueprintType)
enum class ERoadFeatureType : uint8
{
	Road    UMETA(DisplayName = "Дорога"),
	River   UMETA(DisplayName = "Река")
};

/**
 * Отрисовка линейных объектов (дорог, рек) из GeoJSON поверх ландшафта.
 *
 * Оптимизации:
 *  - MinPointSpacing_m — прореживание линий.
 *  - MaxVertsPerSection — секционирование RoadMesh.
 *  - bUseAsyncCooking = false — не держим вторую копию буферов.
 *
 * Посадка на рельеф:
 *  - Основной путь: LineTraceComponent по мешу ландшафта —
 *    работает без физической коллизии, минуя BodySetup.
 *  - Fallback: LineTraceSingleByChannel.
 *  - Fallback 2: GetTerrainZFromMap (интерполяция по PNG).
 *  - Y инвертируется в GeoToWorld, чтобы совместиться с PNG.
 */
UCLASS(Blueprintable)
class RAILOPTIMA_API ARoadRenderer : public AActor
{
	GENERATED_BODY()

public:

	ARoadRenderer();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Roads")
	bool LoadAndBuild(const FString& InGeoJsonPath);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Roads")
	void ClearGeometry();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "RailOptima|Roads")
	void Rebuild();

	// ─────────────────────────────────────────────────────────────────
	//  Настройки
	// ─────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Roads")
	ERoadFeatureType FeatureType = ERoadFeatureType::Road;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Roads")
	FString GeoJsonPath = TEXT("RawContent/roads.geojson");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Roads",
		meta = (ClampMin = "10.0", UIMax = "20000.0"))
	float Width_cm = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Roads",
		meta = (ClampMin = "0.0", ClampMax = "5000.0"))
	float MinPointSpacing_m = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Roads")
	bool bAutoZOffset = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Roads",
		meta = (ClampMin = "0.01", ClampMax = "0.5"))
	float AutoZOffsetFactor = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Roads",
		meta = (ClampMin = "0.0", UIMax = "50000.0",
			EditCondition = "!bAutoZOffset"))
	float ZOffset_cm = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Roads")
	float WorldZScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Roads",
		meta = (ClampMin = "0", ClampMax = "10"))
	int32 ZSmoothingRadius = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Roads")
	bool bClampToMapBounds = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Roads",
		meta = (ClampMin = "1000", ClampMax = "65000"))
	int32 MaxVertsPerSection = 60000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Roads")
	FLinearColor RoadColor = FLinearColor(0.55f, 0.55f, 0.55f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Roads")
	FLinearColor RiverColor = FLinearColor(0.15f, 0.45f, 0.85f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Roads")
	TObjectPtr<UMaterialInterface> RoadMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Roads")
	TObjectPtr<AProceduralTerrain> Terrain;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Debug")
	bool bLogStats = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Roads",
		meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DeferredBuildDelay = 0.5f;

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RailOptima|Roads",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RailOptima|Roads",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProceduralMeshComponent> RoadMesh;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> TerrainMeshCache;

	struct FRibbonBuffer
	{
		TArray<FVector>          Vertices;
		TArray<FLinearColor>     Colors;
		TArray<FVector>          Normals;
		TArray<FVector2D>        UVs;
		TArray<FProcMeshTangent> Tangents;
		TArray<int32>            Triangles;

		void Reset()
		{
			Vertices.Reset();
			Colors.Reset();
			Normals.Reset();
			UVs.Reset();
			Tangents.Reset();
			Triangles.Reset();
		}

		int32 NumVerts() const { return Vertices.Num(); }
	};

	FRibbonBuffer SectionBuffer;
	int32 CurrentSectionIndex = 0;
	int32 TotalVertsWritten = 0;
	int32 TotalTrisWritten = 0;
	int32 SectionsCreated = 0;

	void FlushSection();

	void DeferredBuild();

	bool ParseGeoJson(const FString& InRawJson,
		TArray<TArray<FVector2D>>& OutLines) const;

	FVector2D GeoToWorld(const FVector2D& InLatLon,
		const FRailHeightMapData& MapData) const;

	bool TraceTerrainZ(const FVector2D& InWorldXY, float& OutZ_cm) const;

	float GetTerrainZFromMap(const FVector2D& InWorldXY,
		const FRailHeightMapData& MapData) const;

	static void SmoothZ(TArray<float>& InOutZ, int32 InRadius);

	static void SimplifyLine(const TArray<FVector2D>& InLine,
		float InMinSpacingCm,
		TArray<FVector2D>& OutLine);

	void AppendRibbon(const TArray<FVector>& InPoints,
		const FLinearColor& InColor);

	void ResolveTerrain();
	void CacheTerrainMesh();
	void UpdateAutoZOffset();

	static FString ResolveFilePath(const FString& InRelativePath);
};