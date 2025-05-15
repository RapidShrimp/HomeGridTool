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
UCLASS(meta=(BlueprintSpawnableComponent))
class LEVELSTREAMING_API UPortalManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UPortalManager();
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	UFUNCTION(BlueprintCallable)
	void Init(AEuclidCharacter* PlayerCharacter);

	void SetPlayerCharacter(AEuclidCharacter* PlayerCharacter);
	void SetControllerOwner(APlayerController* NewOwner);
	
	UFUNCTION(BlueprintCallable,Category = "Portal")
	void TeleportRequest(APortal* Portal, AActor* TeleportActor);



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


};
