// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <tuple>
#include <vector>
#include <algorithm>

#include "Engine.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SobolGenerator.h"
#include "VoronoiDiagram/Fortune/Tomilov/sweepline.hpp"
#include "MapPointGenerator.generated.h"

using namespace std;

struct VoronoiPoint {
	double x, y;
	AActor *pointActor;
	bool operator < (const VoronoiPoint& p) const {
		return tie(x, y) < tie(p.x, p.y);
	}
};

UCLASS()
class MAPGENERATORLAB_API AMapPointGenerator : public AActor {
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMapPointGenerator();

	UPROPERTY(Category = Effects, EditAnywhere)
	class UParticleSystem *SiteParticleTemplate;

	UPROPERTY(Category = Effects, EditAnywhere)
	class UParticleSystem *VerticeParticleTemplate;

	UPROPERTY(Category = Effects, EditAnywhere)
	double Scalar = 10.0;

	UPROPERTY(Category = Effects, EditAnywhere)
	unsigned int HowManyGenerating = 10;

	UPROPERTY(Category = Effects, EditAnywhere)
	double ElementZ = 20.0;

	UPROPERTY(Category = Effects, EditAnywhere)
	double LineThinkness = 10.0;

	UPROPERTY(Category = Effects, EditAnywhere)
	FLinearColor SiteLineColor = FLinearColor::Blue;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	AActor* SpawnActorInWorld(const TCHAR* ClassName, const FVector& SpawnLocation);
	sweepline<vector<VoronoiPoint>::const_iterator, VoronoiPoint, double> Sweepline{ 1e-8 };
	TArray<FBatchedLine> SiteLines;
};