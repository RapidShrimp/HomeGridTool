// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EuclidFunctionLibrary.generated.h"

class UBoxComponent;
class UPortalManager;
class AController;
/**
 * 
 */
UCLASS()
class LEVELSTREAMING_API UEuclidFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	//Transform Helpers
	static FVector ConvertLocationWorldToActorLocal(const FVector& Location, const AActor* ToLocalActor, const AActor* FromTarget);
	static FRotator ConvertRotationWorldToActorLocal(const FRotator& Rotation, const AActor* ToLocalActor, const AActor* FromTarget);
	static  FMatrix GetCameraProjectionMatrix(const APlayerController* PlayerController);

	//Component Grabbers

	UFUNCTION(BlueprintCallable)
	static UPortalManager* GetPortalManagerAttachedToController(const AController* Controller);

	UFUNCTION(BlueprintCallable)
	static bool IsLocationInBounds(FVector Location, UBoxComponent* Box);
};
