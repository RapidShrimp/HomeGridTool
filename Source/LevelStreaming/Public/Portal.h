// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Portal.generated.h"

UCLASS()
class LEVELSTREAMING_API APortal : public AActor
{
	GENERATED_BODY()

public:
	//No Public Variables Currently

protected:

	//Components
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USceneComponent> m_PortalRoot;

	//Variables
	//N/A 
private:
	
	bool bIsActive;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<AActor> Target;

	//Previous Frame Data
	FVector LastLocation;
	bool LastInFront;
	
	
public:

	// Sets default values for this actor's properties
	APortal();
	
	virtual void BeginPlay() override;

#pragma region Getters/Setters
	//If the portal is on and Seen by player
	UFUNCTION(BlueprintPure,Category = "Portal")
	bool IsActive() const {return bIsActive;}
	
	UFUNCTION(BlueprintCallable,Category = "Portal")
	void SetActive(bool IsActive) {bIsActive = IsActive;}

	UFUNCTION(BlueprintPure,Category = "Portal")
	AActor* GetTarget() {return Target;}

	UFUNCTION(BlueprintCallable,Category = "Portal")
	void SetTarget(AActor* InTarget) {Target = InTarget;}
	
#pragma endregion
	
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintNativeEvent,Category = "Portal")
	void ForceTick();
	//Target Handling
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable,Category = "Portal")
	void ClearRenderTargetTexture();

	UFUNCTION(BlueprintNativeEvent,BlueprintCallable,Category = "Portal")
	void SetRenderTargetTexture(UTexture* RenderTexture);

	//Viewport Helpers
	UFUNCTION(BlueprintCallable,Category = "Portal")
	bool IsVisibleLocation(FVector Location, FVector PortalLoc, FVector PortalNormal);

	UFUNCTION(BlueprintCallable,Category = "Portal")
	bool IsCrossingPortal(FVector Location, FVector PortalLoc, FVector PortalNorm);

	UFUNCTION(BlueprintCallable,Category = "Portal")
	void TeleportActor(AActor* TeleportActor);

	static FVector ConvertLocationWorldToActorLocal(const FVector& Location, const AActor* ToLocalActor, const AActor* FromTarget);
	static FRotator ConvertRotationWorldToActorLocal(const FRotator& Rotation, const AActor* ToLocalActor, const AActor* FromTarget);
	
};
