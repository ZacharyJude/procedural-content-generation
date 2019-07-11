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
	static const double Eps;

	// Sets default values for this actor's properties
	AMapPointGenerator();

	UPROPERTY(Category = Map, EditAnywhere, BlueprintReadWrite)
	float MapWidth = 0.0;

	UPROPERTY(Category = Map, EditAnywhere, BlueprintReadWrite)
	float MapHeight = 0.0;

	UPROPERTY(Category = Effects, EditAnywhere)
	bool IsShowSiteLines = false;

	UPROPERTY(Category = Effects, EditAnywhere)
	bool IsShowVerticeLines = false;

	UPROPERTY(Category = Effects, EditAnywhere)
	unsigned int HowManyGenerating = 10;

	UPROPERTY(Category = Effects, EditAnywhere)
	float ElementZ = 20.0;

	UPROPERTY(Category = Effects, EditAnywhere)
	float LineThinkness = 10.0;

	UPROPERTY(Category = Effects, EditAnywhere)
	FLinearColor SiteLineColor = FLinearColor::Blue;

	UPROPERTY(Category = Effects, EditAnywhere)
	FLinearColor VerticeLineColor = FLinearColor::Green;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	float PreviousMapWidth = 0.0, PreviousMapHeight = 0.0;

	AActor* SpawnActorInWorld(const TCHAR* ClassName, const FVector& SpawnLocation);
	sweepline<vector<VoronoiPoint>::const_iterator, VoronoiPoint, double> Sweepline{ 1e-8 };
	TArray<FBatchedLine> SiteLines, VerticeLines;
	TArray<AActor> SpawnedActors;
	vector<VoronoiPoint> SiteVoronoiPoints;
	void Reset();
	void Generate();
};