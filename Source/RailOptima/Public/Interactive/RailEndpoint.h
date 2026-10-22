// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль
// визуализации», ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RailEndpoint.generated.h"

class USceneComponent;
class UBillboardComponent;
class UStaticMeshComponent;
class USphereComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRailEndpointMovedSignature,
	ARailEndpoint*, Endpoint);

/**
 * Интерактивная точка старта или финиша.
 *
 * Материал маркера (M_EndpointMarker или другой) должен иметь два параметра:
 *   - Vector Parameter "BaseColor"     — основной цвет шара.
 *   - Scalar Parameter "EmissiveBoost" — множитель эмиссии для подсветки.
 *
 * C++ передаёт оба параметра динамически. Цвет — из EndpointColor, emissive —
 * 0 в покое и HighlightEmissiveBoost при наведении курсора.
 *
 * Динамический масштаб: SetViewScale меняет радиус сферы относительно
 * базового значения, зафиксированного в BeginPlay.
 */
UCLASS(Blueprintable)
class RAILOPTIMA_API ARailEndpoint : public AActor
{
	GENERATED_BODY()

public:
	ARailEndpoint();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	// ─────────────────────────────────────────────────────────────────
	//  Настройки
	// ─────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Endpoint")
	bool bIsStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Endpoint")
	FLinearColor EndpointColor = FLinearColor(0.1f, 0.9f, 0.1f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Endpoint",
		meta = (ClampMin = "500.0", ClampMax = "100000.0"))
	float SphereRadius_cm = 8000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Endpoint",
		meta = (ClampMin = "0.0", ClampMax = "100000.0"))
	float TerrainZOffset_cm = 8000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Endpoint")
	TObjectPtr<UMaterialInterface> EndpointMaterial;

	// Множитель эмиссии при подсветке. 0 — без свечения.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Endpoint",
		meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float HighlightEmissiveBoost = 5.0f;

	// ─────────────────────────────────────────────────────────────────
	//  События
	// ─────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "RailOptima|Endpoint")
	FRailEndpointMovedSignature OnEndpointMoved;

	// ─────────────────────────────────────────────────────────────────
	//  API
	// ─────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Endpoint")
	void SetEndpointFromTerrainHit(const FVector& InTerrainHitLocation,
		bool									  bBroadcastEvent = true);

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Endpoint")
	void SetHighlighted(bool bHighlighted);

	// Установить масштаб маркера. Радиус сферы = BaseSphereRadius_cm * InScale.
	// Вызывается GameMode'ом при изменении высоты TopDown-камеры.
	UFUNCTION(BlueprintCallable, Category = "RailOptima|Endpoint")
	void SetViewScale(float InScale);

	UFUNCTION(BlueprintPure, Category = "RailOptima|Endpoint")
	float GetViewScale() const { return ViewScale; }

	UFUNCTION(BlueprintPure, Category = "RailOptima|Endpoint")
	USphereComponent* GetCollisionSphere() const { return CollisionSphere; }

private:
	// ─────────────────────────────────────────────────────────────────
	//  Компоненты
	// ─────────────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "RailOptima|Endpoint",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "RailOptima|Endpoint",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBillboardComponent> Billboard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "RailOptima|Endpoint",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "RailOptima|Endpoint",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	// База радиуса, зафиксированная при BeginPlay.
	float BaseSphereRadius_cm = 0.0f;

	// Текущий масштаб вида.
	float ViewScale = 1.0f;

	static constexpr float MinViewScale = 10.0f;
	static constexpr float MaxViewScale = 100.0f;

	// ─────────────────────────────────────────────────────────────────
	//  Внутренняя логика
	// ─────────────────────────────────────────────────────────────────

	void ApplyVisuals();
	void LogState(const FString& InContext) const;
};