// Copyright (c) 2026 Николайчик А.В. Все права защищены.
// Дипломный проект
// «Разработка программы проектирования железнодорожных путей: модуль визуализации»,
// ПривГУПС, 2026.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RailOptimaPlayerController.generated.h"

class ARailEndpoint;
class ARailSpectatorPawn;
class UInputMappingContext;

/**
 * PlayerController для RailOptima.
 *
 * Сам не ловит клавиши. Enhanced Input-привязки живут в ARailSpectatorPawn.
 * Контроллер:
 *  - добавляет IMC в BeginPlay;
 *  - подписывается на делегаты павна OnPrimaryPressed/OnPrimaryReleased;
 *  - выполняет логику ЛКМ: пик маркера, захват, перетаскивание;
 *  - центрирует павн на карте.
 */
UCLASS()
class RAILOPTIMA_API ARailOptimaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARailOptimaPlayerController();

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintCallable, Category = "RailOptima|Camera")
	void CenterCameraOnTerrain();

protected:
	// ─────────────────────────────────────────────────────────────────
	//  Enhanced Input
	// ─────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Input|Contexts",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Input|Contexts",
		meta = (AllowPrivateAccess = "true"))
	int32 MappingContextPriority = 0;

	// ─────────────────────────────────────────────────────────────────
	//  Пик маркеров
	// ─────────────────────────────────────────────────────────────────

	// Радиус сферы, которой «ощупывается» курсор при поиске маркера. 1000 см = 10 м.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RailOptima|Input",
		meta = (AllowPrivateAccess = "true", ClampMin = "10.0", ClampMax = "100000.0"))
	float EndpointPickRadius_cm = 1000.0f;

private:
	// ─────────────────────────────────────────────────────────────────
	//  Логика ЛКМ (вызывается из делегатов павна)
	// ─────────────────────────────────────────────────────────────────

	void HandlePrimaryPressed();
	void HandlePrimaryReleased();
	void HandleEscapePressed();

	// ─────────────────────────────────────────────────────────────────
	//  Трассы
	// ─────────────────────────────────────────────────────────────────

	ARailEndpoint* PickEndpointUnderCursor() const;
	bool		   TraceTerrainUnderCursor(FVector& OutHitLocation) const;

	// ─────────────────────────────────────────────────────────────────
	//  Состояние
	// ─────────────────────────────────────────────────────────────────

	UPROPERTY()
	TObjectPtr<ARailEndpoint> HoveredEndpoint;

	UPROPERTY()
	TObjectPtr<ARailEndpoint> DraggedEndpoint;

	// Павн, на который подписались. Храним, чтобы корректно отписаться при
	// перевладении (Possess). Обычно не нужно, но на всякий.
	UPROPERTY()
	TObjectPtr<ARailSpectatorPawn> SubscribedPawn;

	int32 TraceFrameCounter = 0;
};