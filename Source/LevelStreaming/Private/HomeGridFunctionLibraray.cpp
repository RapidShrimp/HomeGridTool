// Fill out your copyright notice in the Description page of Project Settings.

#include "HomeGridFunctionLibraray.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

UObject* UHomeGridFunctionLibraray::ShowEditorAssetSaveDialog(const FString& DefaultPath,const FString& DefaultAssetName, TSubclassOf<UObject> AssetClass)
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

    // Extract package path and asset name
    FString AssetName;
    FString PackagePath;

    if (!ObjectPath.Split(TEXT("/"), &PackagePath, &AssetName, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to split object path: %s"), *ObjectPath);
        return nullptr;
    }

    PackagePath = ObjectPath.Left(ObjectPath.Len() - (AssetName.Len() + 1)); // Remove '/AssetName'

    // Create package
    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create package: %s"), *PackagePath);
        return nullptr;
    }

    // Create asset
    UObject* NewAsset = NewObject<UObject>(Package, AssetClass, *AssetName, RF_Public | RF_Standalone);
    if (!NewAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create asset: %s"), *AssetName);
        return nullptr;
    }

    Package->MarkPackageDirty();

    // Save package to disk
    FString FilePath = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    bool bSuccess = UPackage::SavePackage(Package, NewAsset, EObjectFlags::RF_Public | RF_Standalone, *FilePath);

    if (!bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to save asset to disk at: %s"), *FilePath);
        return nullptr;
    }

    return NewAsset;

#else
    return nullptr;
#endif
}

FLevelObjectRef UHomeGridFunctionLibraray::MakeSoftLevelReference(UObject* LevelAssetObject)
{
    FLevelObjectRef LevelRef;

    if (LevelAsset && LevelAsset->IsA<UWorld>())
    {
        // Get the level package name (full path)
        FString PackageName = LevelAsset->GetOutermost()->GetName();

        // Assign package name to FLevelObjectRef
        LevelRef.LevelName = FName(*PackageName);
    }
    else
    {
        LevelRef.LevelName = NAME_None;
    }

    return LevelRef;
}

