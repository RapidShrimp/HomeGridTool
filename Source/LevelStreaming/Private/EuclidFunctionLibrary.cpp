// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelStreaming/Public/EuclidFunctionLibrary.h"

#include "Components/BoxComponent.h"
#include "LevelStreaming/Public/PortalManager.h"

FVector UEuclidFunctionLibrary::ConvertLocationWorldToActorLocal(const FVector& Location, const AActor* ToLocalActor,
                                                                 const AActor* FromTarget)
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

FRotator UEuclidFunctionLibrary::ConvertRotationWorldToActorLocal(const FRotator& Rotation, const AActor* ToLocalActor,
	const AActor* FromTarget)
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

FMatrix UEuclidFunctionLibrary::GetCameraProjectionMatrix(const APlayerController* PlayerController)
{
	if(PlayerController == nullptr) {return FMatrix();}

	FSceneViewProjectionData PlayerProjectionData;
	PlayerController->GetLocalPlayer()->GetProjectionData(PlayerController->GetLocalPlayer()->ViewportClient->Viewport,PlayerProjectionData);
	return PlayerProjectionData.ProjectionMatrix;

	
}

UPortalManager* UEuclidFunctionLibrary::GetPortalManagerAttachedToController(const AController* Controller)
{
	if(!Controller) {return nullptr;}

	return Controller->FindComponentByClass<UPortalManager>();
}

bool UEuclidFunctionLibrary::IsLocationInBounds(FVector Location, UBoxComponent* Box)
{
	if(Box == nullptr)
	{
		UE_LOG(LogTemp,Error,TEXT("No Box Component Found"));
		return false;
	}

	FVector Center = Box->GetComponentLocation();
	FVector HalfBound = Box->GetScaledBoxExtent();
	FVector X = Box->GetForwardVector();
	FVector Y = Box->GetRightVector();
	FVector Z = Box->GetUpVector();

	FVector Direction = Location - Center;

	const bool Inside =
			FMath::Abs(FVector::DotProduct(Direction,X)) <= HalfBound.X &&
			FMath::Abs(FVector::DotProduct(Direction,Y)) <= HalfBound.Y &&
			FMath::Abs(FVector::DotProduct(Direction,Z)) <= HalfBound.Z;

	return Inside;
}
