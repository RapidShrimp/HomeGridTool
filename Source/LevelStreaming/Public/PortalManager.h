// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalManager.generated.h"

class AEuclidCharacter;
class APlayerController;
class APortal;
class UTextureRenderTarget2D;



/***
 *This component is created to be attached to a controller
 *If attached to anything else, it will be removed
 *Update needs to be called manually
 */
UCLASS()
class LEVELSTREAMING_API UPortalManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UPortalManager();

	UFUNCTION(BlueprintCallable,Category = "Portal")
	void TeleportRequest(APortal* Portal, AActor* TeleportActor);

	void SetControllerOwner(APlayerController* NewOwner);

	void Init();

	UFUNCTION(BlueprintCallable)
	void Update (float DeltaTime);

	APortal* UpdatePortals();

	void UpdateCapture(APortal* Portal);

private:
	void GeneratePortalTexture();

	UPROPERTY()
	TObjectPtr<USceneCaptureComponent2D> SceneCapture;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> PortalTexture;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<APlayerController> ControllerOwner;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<AEuclidCharacter> CharacterOwner;
	
	int32 PreviousScreenSizeX;
	int32 PreviousScreenSizeY;

	float UpdateDelay;

	
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

};
