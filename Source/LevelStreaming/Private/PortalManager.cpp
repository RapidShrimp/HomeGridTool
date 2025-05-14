// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelStreaming/Public/PortalManager.h"

#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "LevelStreaming/Public/EuclidCharacter.h"
#include "LevelStreaming/Public/Portal.h"


// Sets default values
UPortalManager::UPortalManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	if(!GetOwner()->IsA(AController::StaticClass()))
	{
		UE_LOG(LogTemp,Error,TEXT("Cannot Attach Portal Manager To a Non Controller classtype"))
		return;
	}

	UpdateDelay = 1.1f;
	PreviousScreenSizeX = 0;
	PreviousScreenSizeY = 0;
	PortalTexture = nullptr;

	SetControllerOwner(Cast<APlayerController>(GetOwner()));
	Init();
}

void UPortalManager::TeleportRequest(APortal* Portal, AActor* TeleportActor)
{
}

void UPortalManager::SetControllerOwner(APlayerController* NewOwner)
{
	ControllerOwner = NewOwner;
	CharacterOwner = Cast<AEuclidCharacter>(ControllerOwner->GetPawn());
	
}

void UPortalManager::Init()
{

	SceneCapture = NewObject<USceneCaptureComponent2D>(this,USceneCaptureComponent2D::StaticClass(),TEXT("PortalSceneCapture"));
	SceneCapture->AttachToComponent(GetOwner()->GetRootComponent(),FAttachmentTransformRules::SnapToTargetIncludingScale);
	SceneCapture->RegisterComponent();
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	SceneCapture->LODDistanceFactor = 3;
	SceneCapture->TextureTarget = nullptr;
	SceneCapture->bEnableClipPlane = true;
	SceneCapture->bUseCustomProjectionMatrix = true;
	SceneCapture->CaptureSource = SCS_SceneColorHDRNoAlpha;

	
	FPostProcessSettings CaptureSettings;
	CaptureSettings.bOverride_AmbientOcclusionQuality = true;
	CaptureSettings.bOverride_MotionBlurAmount = true;
	CaptureSettings.bOverride_SceneFringeIntensity = true;
	CaptureSettings.bOverride_FilmGrainIntensity = true;
	CaptureSettings.bOverride_ScreenSpaceReflectionQuality = true;

	CaptureSettings.AmbientOcclusionQuality = 0.0f;
	CaptureSettings.MotionBlurAmount = 0.0f;
	CaptureSettings.SceneFringeIntensity = 0.0f;
	CaptureSettings.FilmGrainIntensity = 0.0f;
	CaptureSettings.ScreenSpaceReflectionQuality = 0.0f;

	CaptureSettings.bOverride_ScreenPercentage_DEPRECATED = true;
	CaptureSettings.ScreenPercentage_DEPRECATED = 100.0f;

	SceneCapture->PostProcessSettings = CaptureSettings;

	GeneratePortalTexture();
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
		return nullptr;
	}

	APortal* ActivePortal = nullptr;

	const FVector PlayerLocation = ControllerOwner->GetPawn()->GetActorLocation();
	float Distance = 4096.0f;

	for( TActorIterator<APortal>ActorItr( GetWorld() ); ActorItr; ++ActorItr )
	{
		APortal* Portal = *ActorItr;
		FVector PortalLocation  = Portal->GetActorLocation();
		FVector PortalNormal = -1 * Portal->GetActorForwardVector();

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

	if(ControllerOwner == nullptr) {return;}

	if(!SceneCapture || !PortalTexture || !Portal) {return;}

	UCameraComponent* PlayerCamera = CharacterOwner->FindComponentByClass<UCameraComponent>();
	AActor* Target = Portal->GetTarget();

	if(Target == nullptr) {return;}


	FVector NewLoc = APortal::ConvertLocationWorldToActorLocal(PlayerCamera->GetComponentLocation(),Portal,Target);
	SceneCapture->SetWorldLocation(NewLoc);

	FTransform CameraTransform = PlayerCamera->GetComponentTransform();
	FTransform SourceTransform = Portal->GetActorTransform();
	FTransform TargetTransform = Target->GetActorTransform();

	FQuat LocalQuat = SourceTransform.GetRotation().Inverse() * CameraTransform.GetRotation();
	FQuat NewQuat = TargetTransform.GetRotation() * LocalQuat;

	SceneCapture->SetWorldRotation(NewQuat);

	SceneCapture->ClipPlaneNormal = Target->GetActorForwardVector();
	SceneCapture->ClipPlaneBase = Target->GetActorLocation() + (SceneCapture->ClipPlaneNormal * -1.5f);

	Portal->SetActive(true);
	Portal->SetRenderTargetTexture(PortalTexture);
	SceneCapture->TextureTarget = PortalTexture;


	SceneCapture->CustomProjectionMatrix;

	SceneCapture->CaptureScene();
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
		PortalTexture->TargetGamma = 2.2f;
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

// Called when the game starts or when spawned
void UPortalManager::BeginPlay()
{
	Super::BeginPlay();
	
}

