// Fill out your copyright notice in the Description page of Project Settings.

#include "HomeGridFunctionLibraray.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "EditorLevelUtils.h"
#include "FileHelpers.h"
#include "Selection.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Factories/WorldFactory.h"
#include "LevelInstance/ILevelInstanceEditorModule.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

bool UHomeGridFunctionLibraray::CreateLevelInstanceFromSelected(AActor* ObjectOrigin ,const FString& DefaultPath, const FString& DefaultAssetName)
{
    
    if(ObjectOrigin)
    {
        return true;
    }
    //This function is removed for now, just a temporary setback
#if WITH_EDITOR


    
    // 1. Get editor world
    UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!EditorWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("Editor world not found."));
        return false;
    }

    // 2. Get selected actors
    TArray<AActor*> SelectedActors;
    GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(SelectedActors);
    if (SelectedActors.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No actors selected."));
        return false;
    }

    // 3. Show save dialog to get new asset path and name
    FSaveAssetDialogConfig SaveConfig;
    SaveConfig.DefaultPath = DefaultPath;
    SaveConfig.DefaultAssetName = DefaultAssetName;
    SaveConfig.DialogTitleOverride = FText::FromString(TEXT("Save Level Asset"));
    SaveConfig.AssetClassNames.Add(UWorld::StaticClass()->GetClassPathName());
    SaveConfig.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;

    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    FString ObjectPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(SaveConfig);

    if (ObjectPath.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("User cancelled save asset dialog."));
        return false;
    }

    // 4. Parse package name and asset name
    FString PackageName, AssetName;
    if (!ObjectPath.Split(TEXT("."), &PackageName, &AssetName))
    {
        int32 LastSlashIndex;
        if (!ObjectPath.FindLastChar('/', LastSlashIndex))
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid asset path format: %s"), *ObjectPath);
            return false;
        }
        PackageName = ObjectPath.Left(LastSlashIndex);
        AssetName = ObjectPath.Mid(LastSlashIndex + 1);
    }

    // 5. Create package for the new level asset
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *PackageName);
        return false;
    }

    // 6. Create new UWorld asset (level)

    UWorldFactory* Factory = NewObject<UWorldFactory>();
    EObjectFlags Flags = RF_Public | RF_Standalone | RF_MarkAsRootSet;

    UObject* NewLevelAsset = Factory->FactoryCreateNew(
        UWorld::StaticClass(),
        Package,
        *AssetName,
        Flags,
        nullptr,
        GWarn
    );
    
    UWorld* NewWorld = Cast<UWorld>(NewLevelAsset);
    if (!NewWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create new UWorld asset via factory: %s"), *AssetName);
        return false;
    }
    
    // 7. Notify asset registry
    FAssetRegistryModule::AssetCreated(NewWorld);

    // 8. Mark package dirty
    Package->MarkPackageDirty();

    // 9. Move selected actors to new level's persistent level
    UEditorLevelUtils::MoveActorsToLevel(SelectedActors, NewWorld->PersistentLevel);

    // 10. Prepare for saving: add to root to avoid GC during save
    NewWorld->AddToRoot();

    // Ensure directory exists
    FString FilePath = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetMapPackageExtension());
    FString Directory = FPaths::GetPath(FilePath);
    if (!IFileManager::Get().DirectoryExists(*Directory))
    {
        IFileManager::Get().MakeDirectory(*Directory, true);
    }

    // 11. Save the new level asset as a map file
    bool bSaved = FEditorFileUtils::SaveLevel(NewWorld->PersistentLevel, *FilePath);
    if (!bSaved)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save new level asset to: %s"), *FilePath);
        NewWorld->RemoveFromRoot();
        return false;
    }

    // 12. Remove root after save
    NewWorld->RemoveFromRoot();

    // 13. Determine spawn transform relative to ReferenceActor or fallback to origin
    FVector SpawnLocation = FVector::ZeroVector;
    FRotator SpawnRotation = FRotator::ZeroRotator;

    if (ObjectOrigin)
    {
        SpawnLocation = ObjectOrigin->GetActorLocation();
        SpawnRotation = ObjectOrigin->GetActorRotation();
    }

    // 14. Create level instance in the editor world at the reference actor's location
    FActorSpawnParameters SpawnParams;
    SpawnParams.OverrideLevel = EditorWorld->PersistentLevel;

    ALevelInstance* LevelInstanceActor = EditorWorld->SpawnActor<ALevelInstance>(
        ALevelInstance::StaticClass(),
        SpawnLocation,
        SpawnRotation,
        SpawnParams
    );
    
    if (!LevelInstanceActor)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn LevelInstance actor."));
        return false;
    }

    
    // 15. Set the level asset and force load it
    FString FullAssetPath = FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName);
    TSoftObjectPtr<UWorld> LevelAsset(FullAssetPath);
    LevelInstanceActor->SetWorldAsset(LevelAsset);

    UFunction* LoadInstanceFunc = LevelInstanceActor->FindFunction(FName("LoadInstance"));
    if (LoadInstanceFunc)
    {
        LevelInstanceActor->ProcessEvent(LoadInstanceFunc, nullptr);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find LoadInstance() function on LevelInstanceActor"));
    }

    if (ULevelInstanceSubsystem* Subsystem = EditorWorld->GetSubsystem<ULevelInstanceSubsystem>())
    {
        Subsystem->RegisterLevelInstance(LevelInstanceActor);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Level instance created successfully from selected actors."));
    return true;

#else
    UE_LOG(LogTemp, Warning, TEXT("This function can only be run in editor builds."));
    return false;
#endif
}

UObject* UHomeGridFunctionLibraray::ShowEditorAssetSaveDialog(const FString& DefaultPath, const FString& DefaultAssetName, TSubclassOf<UObject> AssetClass, bool OpenAfterCreate)
{
#if WITH_EDITOR

    if (!AssetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("No asset class provided."));
        return nullptr;
    }

    // Configure save dialog
    FSaveAssetDialogConfig SaveConfig;
    SaveConfig.DefaultPath = DefaultPath;
    SaveConfig.DefaultAssetName = DefaultAssetName;
    SaveConfig.DialogTitleOverride = FText::FromString(TEXT("Save Asset"));
    SaveConfig.AssetClassNames.Add(AssetClass->GetClassPathName());
    SaveConfig.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;

    // Show dialog
    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    FString ObjectPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(SaveConfig);

    if (ObjectPath.IsEmpty())
    {
        return nullptr; // User cancelled
    }

    // Remove trailing slash
    FString CleanObjectPath = ObjectPath;
    if (CleanObjectPath.EndsWith("/"))
    {
        CleanObjectPath = CleanObjectPath.LeftChop(1);
    }

    // Extract package name and asset name
    FString PackageName;
    FString AssetName;

    if (!CleanObjectPath.Split(TEXT("."), &PackageName, &AssetName))
    {
        int32 LastSlashIndex;
        if (!CleanObjectPath.FindLastChar('/', LastSlashIndex))
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid ObjectPath format: %s"), *CleanObjectPath);
            return nullptr;
        }
        PackageName = CleanObjectPath.Left(LastSlashIndex);
        AssetName = CleanObjectPath.Mid(LastSlashIndex + 1);
    }

    // Create package
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create package: %s"), *PackageName);
        return nullptr;
    }

    // Create asset
    UObject* NewAsset = NewObject<UObject>(Package, AssetClass.Get(), *AssetName, RF_Public | RF_Standalone);
    if (!NewAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create asset: %s"), *AssetName);
        return nullptr;
    }

    // Notify Asset Registry about the new asset
    FAssetRegistryModule::AssetCreated(NewAsset);

    // Load to avoid partial load issues
    Package->FullyLoad();
    NewAsset->ConditionalPostLoad();

    // Mark package dirty so editor and Git knows it changed
    Package->MarkPackageDirty();

    //Force Save package
    FString FilePath = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

    bool bSuccess = UPackage::SavePackage(
        Package,
        NewAsset,
        EObjectFlags::RF_Public | RF_Standalone,
        *FilePath,
        GError,
        nullptr,
        true,  // bForceByteSwapping
        true,  // bWarnOfLongFilename
        SAVE_NoError
    );

    if (!bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to save asset to disk at: %s"), *FilePath);
        return nullptr;
    }

    if(!GEditor)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot Open Asset, No Editor Found"));
    }
    // Open in editor
    if (GEditor && OpenAfterCreate)
    {
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(NewAsset);
        UE_LOG(LogTemp, Display, TEXT("Opening Assset"));

    }
    return NewAsset;

#else
    return nullptr;
#endif
}

FSoftWorldReference UHomeGridFunctionLibraray::MakeSoftLevelReference(UObject* LevelAssetObject)
{
    if (LevelAssetObject && LevelAssetObject->IsA<UWorld>())
    {
        return FSoftWorldReference(Cast<UWorld>(LevelAssetObject));
    }

    UE_LOG(LogTemp, Warning, TEXT("Provided asset is not a UWorld."));
    return FSoftWorldReference();
}
