// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelStreaming/Public/EuclidCharacter.h"


// Sets default values
AEuclidCharacter::AEuclidCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AEuclidCharacter::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	//TODO - Find Alternative to Update
	if(m_PortalManager != nullptr)
	{
		m_PortalManager->Update(DeltaTime);
	}
}
