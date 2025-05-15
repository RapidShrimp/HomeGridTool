// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PortalManager.h"
#include "GameFramework/Character.h"
#include "EuclidCharacter.generated.h"

class UCameraComponent;

UCLASS()
class LEVELSTREAMING_API AEuclidCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEuclidCharacter();

	UFUNCTION(BlueprintCallable, Category = "Portal")
	void SetPortalManger(UPortalManager* Manager) {m_PortalManager = Manager;}

	UFUNCTION(BlueprintPure, Category = "Portal")
	UCameraComponent* GetCameraComponent() {return m_Camera;}

	UFUNCTION(BlueprintCallable, Category = "Portal")
	void SetCameraComponent(UCameraComponent* InCamera) {m_Camera = InCamera;}
	
protected:
	void TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPortalManager> m_PortalManager;

	UPROPERTY()
	TObjectPtr<UCameraComponent> m_Camera;
};
