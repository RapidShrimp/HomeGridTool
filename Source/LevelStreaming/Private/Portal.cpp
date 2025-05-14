// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelStreaming/Public/Portal.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"


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

// Called when the game starts or when spawned
void APortal::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APortal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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
	bool IsCrossingPortal = false;
	const FPlane PortalPlane = FPlane(PortalLoc, PortalNorm);
	const float PortalDot = PortalPlane.PlaneDot(Location);
	const bool Infront = (PortalDot >= 0);
	const bool Intersected = FMath::SegmentPlaneIntersection(
		LastLocation,
		Location,
		PortalPlane,
		Intersect );


	//Set the previous frame values
	LastInFront = Infront;
	LastLocation = Location;

	//Prevent entering portal from the rear
	if(Intersected && !Infront && LastInFront)
		{ IsCrossingPortal = true;	}

	return IsCrossingPortal;
}

void APortal::TeleportActor(AActor* TeleportActor)
{
	//Exit Early Null Checks
	if(!TeleportActor || !Target) { return;	}

	//Location Change
	FHitResult Hit;
	const FVector NewLocation = ConvertLocationWorldToActorLocal(TeleportActor->GetActorLocation(),this,Target);
	TeleportActor->SetActorLocation(NewLocation,false,&Hit,ETeleportType::TeleportPhysics);
	

	//Rotation Change
	FRotator ExitRotation = ConvertRotationWorldToActorLocal(TeleportActor->GetActorRotation(),this,Target);
	TeleportActor->SetActorRotation(ExitRotation);
	

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
				ExitRotation = ConvertRotationWorldToActorLocal(PlayerController->GetControlRotation(),this,Target);
				PlayerController->SetControlRotation(ExitRotation);
			}
			PlayerCharacter->GetCharacterMovement()->Velocity = ExitVelocity;
		}
	}
	LastLocation = NewLocation;
}

FVector APortal::ConvertLocationWorldToActorLocal(const FVector& Location, const AActor* ToLocalActor, const AActor* FromTarget)
{
	if(!ToLocalActor || !FromTarget) {return FVector::ZeroVector;}

	const FVector Direction = Location - ToLocalActor->GetActorLocation();
	const FVector TargetLocation = FromTarget->GetActorLocation();

	FVector Dots;
	Dots.X  = FVector::DotProduct( Direction, ToLocalActor->GetActorForwardVector() );
	Dots.Y  = FVector::DotProduct( Direction, ToLocalActor->GetActorRightVector() );
	Dots.Z  = FVector::DotProduct( Direction, ToLocalActor->GetActorUpVector() );

	const FVector NewDirection =
		Dots.X * FromTarget->GetActorForwardVector()+
		Dots.Y * FromTarget->GetActorRightVector()+
		Dots.Z * FromTarget->GetActorUpVector();

	return TargetLocation + NewDirection;
}

FRotator APortal::ConvertRotationWorldToActorLocal(const FRotator& Rotation, const AActor* ToLocalActor, const AActor* FromTarget)
{
	if( ToLocalActor == nullptr || FromTarget == nullptr )
	{
		return FRotator::ZeroRotator;
	}

	const FTransform SourceTransform = ToLocalActor->GetActorTransform();
	const FTransform TargetTransform = FromTarget->GetActorTransform();

	const FQuat LocalQuaternion = SourceTransform.GetRotation().Inverse() * FQuat( Rotation );
	const FQuat NewQuaternion = TargetTransform.GetRotation() * LocalQuaternion;

	return NewQuaternion.Rotator();
}



