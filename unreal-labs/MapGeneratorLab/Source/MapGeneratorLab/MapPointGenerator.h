// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SobolGenerator.h"
#include "MapPointGenerator.generated.h"

UCLASS()
class MAPGENERATORLAB_API AMapPointGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMapPointGenerator();

	UPROPERTY(Category = Generator, EditAnywhere)
	double Scalar = 10.0;

	UPROPERTY(Category = Generator, EditAnywhere)
	unsigned int HowManyGenerating = 10;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void SpawnActorInWorld(const TCHAR* ClassName, const FVector& SpawnLocation);
};