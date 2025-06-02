// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



#include "LevelInstance/LevelInstanceTypes.h" // for FLevelNameObjectReference
#include "HomeGridFunctionLibraray.generated.h"


/**
 * 
 */
UCLASS()
class LEVELSTREAMING_API UHomeGridFunctionLibraray : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * @param AssetClass - the asset class that we want to create in the folder structure 
	 * @param DefualtPath - the default open save dialogue location
	 * @param DefaultAssetName - the default asset placeholder name
	 */
	UFUNCTION(BlueprintCallable, Category = "Editor|File Dialog", meta = (DevelopmentOnly))
	static UObject* ShowEditorAssetSaveDialog(const FString& DefaultPath = "/Game", const FString& DefaultAssetName = "NewAsset", TSubclassOf<UObject> AssetClass = nullptr);

	UFUNCTION(BlueprintCallable)
	LevelNameObjectReference UHomeGridFunctionLibraray::MakeSoftLevelReference(UObject* LevelAssetObject)


	
};
