// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelStreaming/Public/PortalManager.h"

#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "LevelStreaming/Public/EuclidCharacter.h"
#include "LevelStreaming/Public/EuclidFunctionLibrary.h"
#include "LevelStreaming/Public/Portal.h"


// Sets default values
UPortalManager::UPortalManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	UpdateDelay = 1.1f;
	PreviousScreenSizeX = 0;
	PreviousScreenSizeY = 0;
	PortalTexture = nullptr;
}


// Called when the game starts or when spawned
void UPortalManager::BeginPlay()
{
	Super::BeginPlay();

}

void UPortalManager::Init(AEuclidCharacter* PlayerCharacter)
{

	if(!GetOwner())
	{
		UE_LOG(PortalLog,Error,TEXT("Portal Manager Has No Owner"))
		return;
	}
	if(!GetOwner()->IsA(AController::StaticClass()))
	{
		UE_LOG(PortalLog,Error,TEXT("Cannot Attach Portal Manager To a Non Controller classtype"))
		return;
	}
	SetControllerOwner(Cast<APlayerController>(GetOwner()));
	SetPlayerCharacter(PlayerCharacter);
	
	SceneCapture = NewObject<USceneCaptureComponent2D>(this,USceneCaptureComponent2D::StaticClass(),TEXT("PortalSceneCapture"));
	SceneCapture->AttachToComponent(GetOwner()->GetRootComponent(),FAttachmentTransformRules::SnapToTargetIncludingScale);
	SceneCapture->RegisterComponent();
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	//SceneCapture->LODDistanceFactor = 3;
	SceneCapture->TextureTarget = nullptr;
	SceneCapture->bEnableClipPlane = true;
	SceneCapture->bUseCustomProjectionMatrix = true;
	SceneCapture->CaptureSource = SCS_SceneColorHDRNoAlpha;

	
	FPostProcessSettings CaptureSettings;
	//TODO - Enable Lumen
	//TODO - Capture Cache Scale
	
	CaptureSettings.bOverride_AmbientOcclusionQuality = true;
	CaptureSettings.bOverride_MotionBlurAmount = true;
	CaptureSettings.bOverride_SceneFringeIntensity = true;
	CaptureSettings.bOverride_FilmGrainIntensity = true;

	CaptureSettings.AmbientOcclusionQuality = 0.0f;
	CaptureSettings.MotionBlurAmount = 0.0f;
	CaptureSettings.SceneFringeIntensity = 0.0f;
	CaptureSettings.FilmGrainIntensity = 0.0f;
	CaptureSettings.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::Lumen;
	CaptureSettings.LumenSurfaceCacheResolution = 1;
	
	SceneCapture->PostProcessSettings = CaptureSettings;

	GeneratePortalTexture();

	UE_LOG(PortalLog,Display,TEXT("Successfully Init Portal Manager"));

}

void UPortalManager::SetPlayerCharacter(AEuclidCharacter* PlayerCharacter)
{
	CharacterOwner = PlayerCharacter;
	CharacterOwner->SetPortalManger(this);
}

void UPortalManager::SetControllerOwner(APlayerController* NewOwner)
{
	ControllerOwner = NewOwner;
}

void UPortalManager::TeleportRequest(APortal* Portal, AActor* TeleportActor)
{
	if(!Portal || !TeleportActor)
	{
		UE_LOG(PortalLog,Error,TEXT("Teleport Request Failed Portal OR Teleport Actor Nullptr"));
		return;
	}

	UE_LOG(LogTemp,Display,TEXT("Teleport Requested"));

	Portal->TeleportActor(TeleportActor);

	APortal* FuturePortal = UpdatePortals();

	if(FuturePortal)
	{
		UE_LOG(PortalLog,Display,TEXT("Future Portal"));

		FuturePortal->ForceTick();
		UpdateCapture(FuturePortal);
	}
}

void UPortalManager::Update(float DeltaTime)
{
	UpdateDelay += DeltaTime;

	if(UpdateDelay > 1.0f)
	{
		UpdateDelay = 0.0f;
		GeneratePortalTexture();
	}

	APortal* Portal = UpdatePortals();

	if(Portal)
	{
		UpdateCapture(Portal);
	}
}

APortal* UPortalManager::UpdatePortals()
{
	if(ControllerOwner == nullptr)
	{
		UE_LOG(PortalLog,Error,TEXT("Cannot update portal, controller nullptr"));
		return nullptr;
	}

	APortal* ActivePortal = nullptr;

	const FVector PlayerLocation = ControllerOwner->GetPawn()->GetActorLocation();
	float Distance = 4096.0f;

	for( TActorIterator<APortal>ActorItr( GetWorld() ); ActorItr; ++ActorItr )
	{
		APortal* Portal = *ActorItr;
		FVector PortalLocation  = Portal->GetActorLocation();

		// Reset Portal
		Portal->ClearRenderTargetTexture();
		Portal->SetActive( false );

		// Find the closest Portal when the player is Standing in front of
		float NewDistance = FMath::Abs( FVector::Dist( PlayerLocation, PortalLocation ) );

		if( NewDistance < Distance )
		{
			Distance = NewDistance;
			ActivePortal = Portal;
		}
	}
	return ActivePortal;
}

void UPortalManager::UpdateCapture(APortal* Portal)
{

	if(ControllerOwner == nullptr)
	{
		UE_LOG(PortalLog,Error,TEXT("Cannot Update Capture, No Controller"));
		return;
	}

	if(!SceneCapture || !PortalTexture || !Portal)
	{
		UE_LOG(PortalLog,Error,TEXT("Cannot Update Capture nullptr found"));
		return;
	}

	UCameraComponent* PlayerCamera = CharacterOwner->FindComponentByClass<UCameraComponent>();
	AActor* Target = Portal->GetTarget();

	if(Target == nullptr)
	{
		UE_LOG(PortalLog,Error,TEXT("Target is nullptr"));
		return;
	}


	FVector NewLoc = UEuclidFunctionLibrary::ConvertLocationWorldToActorLocal(PlayerCamera->GetComponentLocation(),Portal,Target);
	SceneCapture->SetWorldLocation(NewLoc);

	FTransform CameraTransform = PlayerCamera->GetComponentTransform();
	FTransform SourceTransform = Portal->GetActorTransform();
	FTransform TargetTransform = Target->GetActorTransform();

	FQuat LocalQuat = SourceTransform.GetRotation().Inverse() * CameraTransform.GetRotation();
	FQuat NewQuat = TargetTransform.GetRotation() * LocalQuat;

	SceneCapture->SetWorldRotation(NewQuat);

	SceneCapture->ClipPlaneNormal = Target->GetActorForwardVector();
	SceneCapture->ClipPlaneBase = Target->GetActorLocation() + (SceneCapture->ClipPlaneNormal * 1.5f);

	Portal->SetActive(true);
	Portal->SetRenderTargetTexture(PortalTexture);
	SceneCapture->TextureTarget = PortalTexture;
	SceneCapture->CustomProjectionMatrix = UEuclidFunctionLibrary::GetCameraProjectionMatrix(ControllerOwner);

	SceneCapture->CaptureScene();

	//UE_LOG(PortalLog,Display,TEXT("Capture Updated"));
}

void UPortalManager::GeneratePortalTexture()
{
	int32 CurrentSizeX = 1920;
	int32 CurrentSizeY = 1080;

	if(ControllerOwner)
	{
		ControllerOwner->GetViewportSize(CurrentSizeX,CurrentSizeY);
	}

	CurrentSizeX = FMath::Clamp( int(CurrentSizeX/1.7),128,1920);
	CurrentSizeY = FMath::Clamp( int(CurrentSizeY/1.7),128,1080);

	if(CurrentSizeX == PreviousScreenSizeX
		&& CurrentSizeY == PreviousScreenSizeY)
	{
		return;
	}

	UE_LOG(PortalLog,Warning,TEXT("Size Changed"));
	if(PortalTexture == nullptr)
	{
		PortalTexture = NewObject<UTextureRenderTarget2D>(this,UTextureRenderTarget2D::StaticClass(),TEXT("Portal Render Target"));
		check(PortalTexture);


		//Set as 16 bits so it can generate bloom
		PortalTexture->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
		PortalTexture->Filter = TF_Bilinear;
		PortalTexture->SizeX = CurrentSizeX;
		PortalTexture->SizeY = CurrentSizeY;
		PortalTexture->ClearColor = FLinearColor::Black;
		PortalTexture->TargetGamma = 2.8f;
		PortalTexture->bNeedsTwoCopies = false;
		PortalTexture->AddressX = TA_Clamp;
		PortalTexture->AddressY = TA_Clamp;
		PortalTexture->bAutoGenerateMips = false;
		PortalTexture->UpdateResource();
	}
	else
	{
		PortalTexture->ResizeTarget(CurrentSizeX,CurrentSizeY);
	}
}



