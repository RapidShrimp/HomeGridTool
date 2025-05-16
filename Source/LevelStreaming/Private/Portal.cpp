// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelStreaming/Public/Portal.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LevelStreaming/Public/EuclidFunctionLibrary.h"
DEFINE_LOG_CATEGORY(PortalLog);

// Sets default values
APortal::APortal()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bIsActive = false;
	LastInFront = false;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent->SetMobility(EComponentMobility::Static);

	m_PortalRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PortalRootComponent"));
	m_PortalRoot->SetupAttachment((GetRootComponent()));
	m_PortalRoot->SetRelativeLocation(FVector::ZeroVector);
	m_PortalRoot->SetRelativeRotation(FRotator::ZeroRotator);
	m_PortalRoot->SetMobility(EComponentMobility::Movable);

	Target = nullptr;
}

void APortal::ForceTick_Implementation() {}
void APortal::SetRenderTargetTexture_Implementation(UTexture* RenderTexture) {}
void APortal::ClearRenderTargetTexture_Implementation() {}

bool APortal::IsVisibleLocation(const FVector Location, const FVector PortalLoc, const FVector PortalNormal)
{
	const FPlane PortalPlane = FPlane(PortalLoc,PortalNormal);
	const float PortalDot = PortalPlane.PlaneDot(Location);
	return PortalDot >= 0;
	// <0 = behind the plane 
}

bool APortal::IsCrossingPortal(FVector Location, FVector PortalLoc, FVector PortalNorm)
{
	FVector Intersect;
	const FPlane PortalPlane = FPlane(PortalLoc, PortalNorm);
	const float PortalDot = PortalPlane.PlaneDot(Location);
	const bool InFront = (PortalDot >= 0);
	const bool Intersected = FMath::SegmentPlaneIntersection(
		LastLocation,
		Location,
		PortalPlane,
		Intersect );


	UE_LOG(PortalLog,Error,TEXT("Should TP %d"),Intersected && !InFront && LastInFront);



	//Prevent entering portal from the rear
	if(Intersected && !InFront && LastInFront)
	{
		UE_LOG(PortalLog,Error,TEXT("Should Teleport"));
		return true;
	}

	//Set the previous frame values
	LastInFront = InFront;
	LastLocation = Location;
	
	return false;
}

void APortal::TeleportActor(AActor* TeleportActor)
{

	//Exit Early Null Checks
	if(!TeleportActor || !Target)
	{
		UE_LOG(PortalLog,Error,TEXT("No Target or Teleport Actor"));
		return;
	}

	//Location Change
	FHitResult Hit;
	const FVector NewLocation = UEuclidFunctionLibrary::ConvertLocationWorldToActorLocal(TeleportActor->GetActorLocation(),this,Target);
	TeleportActor->SetActorLocation(NewLocation,false,&Hit,ETeleportType::TeleportPhysics);

	//Rotation Change
	FRotator ExitRotation = UEuclidFunctionLibrary::ConvertRotationWorldToActorLocal(TeleportActor->GetActorRotation(),this,Target);
	TeleportActor->SetActorRotation(ExitRotation);
	

	TeleportActor->TeleportTo(NewLocation,ExitRotation,false,false);
	//Preserve Velocity
	FVector CurrentVelocity = TeleportActor->GetVelocity();
	FVector Dots;
	Dots.X = FVector::DotProduct(CurrentVelocity,GetActorForwardVector());
	Dots.Y = FVector::DotProduct(CurrentVelocity,GetActorRightVector());
	Dots.Z = FVector::DotProduct(CurrentVelocity,GetActorUpVector());

	const FVector ExitVelocity =
		Dots.X * Target->GetActorForwardVector() + 
		Dots.Y * Target->GetActorRightVector() +
		Dots.Z * Target->GetActorUpVector();
	
	TeleportActor->GetRootComponent()->ComponentVelocity = ExitVelocity;
	
	UE_LOG(PortalLog,Display,TEXT("Try Teleport Actor"));

	//Exit Rotation
	if( TeleportActor->IsA(ACharacter::StaticClass()))
	{
		const ACharacter* PlayerCharacter = Cast<ACharacter>(TeleportActor);
		CurrentVelocity = PlayerCharacter->GetCharacterMovement()->Velocity;

		if(PlayerCharacter != nullptr)
		{
			AController* PlayerController = PlayerCharacter->GetController();  
			if(PlayerCharacter != nullptr)
			{
				ExitRotation = UEuclidFunctionLibrary::ConvertRotationWorldToActorLocal(PlayerController->GetControlRotation(),this,Target);
				PlayerController->SetControlRotation(ExitRotation);
			}
			PlayerCharacter->GetCharacterMovement()->Velocity = ExitVelocity;
		}
	}
	LastLocation = NewLocation;
}


