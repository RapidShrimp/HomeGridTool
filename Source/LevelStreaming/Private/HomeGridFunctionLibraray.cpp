// Fill out your copyright notice in the Description page of Project Settings.

#include "HomeGridFunctionLibraray.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
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