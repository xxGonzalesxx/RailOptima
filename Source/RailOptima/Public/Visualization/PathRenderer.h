// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект

#pragma once

#include "Algorithms/AStarPathFinder.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PathRenderer.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class UBillboardComponent;

UENUM(BlueprintType)
enum class ERailPathColorMode : uint8
{
	Height UMETA(DisplayName = "По высоте"),
	Cost   UMETA(DisplayName = "По стоимости")
};

UCLASS(Blueprintable)
class RAILOPTIMA_API APathRenderer : public AActor
{
	GENERATED_BODY()

public:
	APathRenderer();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Path")
	void RenderPath(const FRailGridData& InGrid, const FRailPathResult& InPath);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Path")
	void RenderColoredPath(
		const FRailGridData&   InGrid,
		const FRailPathResult& InPath,
		const TArray<float>&   InNodeValues,
		float				   InLowValue,
		float				   InHighValue);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Path")
	void RenderCustomPath(const TArray<FVector>& InPoints, const TArray<FLinearColor>& InColors);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Path")
	void ClearPath();

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Path")
	void SetViewScale(float InScale);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Path")
	float GetViewScale() const { return ViewScale; }

	UFUNCTION(BlueprintPure, Category = "RailOptima|Path")
	FLinearColor EvaluateColor(float InT) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Geometry",
		meta = (ClampMin = "10.0", UIMin = "100.0", UIMax = "20000.0"))
	float Width_cm = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Geometry",
		meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "50000.0"))
	float Height_cm = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Geometry",
		meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "50000.0"))
	float ZOffset_cm = 8000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Geometry",
		meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float WorldZScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Geometry")
	FVector GridWorldOrigin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Smoothing")
	bool bSmoothPath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Smoothing",
		meta = (ClampMin = "1", ClampMax = "16", EditCondition = "bSmoothPath"))
	int32 SamplesPerSegment = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Appearance")
	TObjectPtr<UMaterialInterface> PathMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Appearance|Color")
	FLinearColor CheapColor = FLinearColor(0.05f, 0.9f, 0.1f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Appearance|Color")
	FLinearColor MidColor = FLinearColor(1.0f, 0.55f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Appearance|Color")
	FLinearColor ExpensiveColor = FLinearColor(0.9f, 0.05f, 0.05f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Debug")
	bool bLogVertexColors = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Debug")
	bool bLogLayout = true;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RailOptima|Path", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RailOptima|Path", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBillboardComponent> Billboard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RailOptima|Path", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProceduralMeshComponent> PathMesh;

	float BaseWidth_cm = 0.0f;
	float BaseHeight_cm = 0.0f;
	float ViewScale = 1.0f;

	TArray<FVector>		 CachedRibbonPoints;
	TArray<FLinearColor> CachedRibbonColors;

	static constexpr float MinViewScale = 0.3f;
	static constexpr float MaxViewScale = 6.0f;

	FVector GridNodeToWorld(const FRailGridNode& InNode, float InStepSize_m) const;

	static void SmoothPolylineCatmullRom(
		const TArray<FVector>&		InPoints,
		const TArray<FLinearColor>& InColors,
		int32						InSamplesPerSegment,
		TArray<FVector>&			OutPoints,
		TArray<FLinearColor>&		OutColors);

	void BuildRibbon(
		const TArray<FVector>&		InPoints,
		const TArray<FLinearColor>& InColors,
		float						InWidth_cm,
		float						InHeight_cm);
};