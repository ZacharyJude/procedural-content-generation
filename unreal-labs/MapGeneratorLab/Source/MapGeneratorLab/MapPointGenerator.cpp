// Fill out your copyright notice in the Description page of Project Settings.

#include "MapPointGenerator.h"

// Sets default values
AMapPointGenerator::AMapPointGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMapPointGenerator::BeginPlay()
{
	Super::BeginPlay();

	auto Generator = new SobolGenerator(HowManyGenerating, 2);
	vector<double> VectorFromSobol;
	FVector SpawnLocation = FVector();
	while (Generator->GetNext(VectorFromSobol)) {
		SpawnLocation.X = VectorFromSobol[0] * Scalar;
		SpawnLocation.Y = VectorFromSobol[1] * Scalar;
		SpawnLocation.Z = 0.0;
		SpawnActorInWorld(TEXT("Blueprint'/Game/GeneratedPointBP.GeneratedPointBP_C'"), SpawnLocation);
		VectorFromSobol.clear();
	}
}

// Called every frame
void AMapPointGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMapPointGenerator::SpawnActorInWorld(const TCHAR* ClassName, const FVector& SpawnLocation) {
	auto World = this->GetWorld();
	if (!World) {
		UE_LOG(LogTemp, Fatal, TEXT("[AMapPointGenerator.Fatal] world is null"));
		return;
	}

	auto SpawningClass = LoadClass<AActor>(nullptr, ClassName, nullptr, LOAD_None, nullptr);
	if (nullptr == SpawningClass) {
		UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] spawning class [%s] can not be found"), ClassName);
		return;
	}

	FActorSpawnParameters spawnParameters;
	spawnParameters.Owner = this;

	FRotator Rotator = this->GetActorRotation();

	auto SpawnedActor = World->SpawnActor<AActor>(SpawningClass, SpawnLocation, Rotator, spawnParameters);
	if (nullptr == SpawnedActor) {
		UE_LOG(LogTemp, Warning, TEXT("[AMapPointGenerator.Warning] spawned actor for class [%s] is null"), ClassName);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] spawned actor for class [%s] named [%s]"), ClassName, *SpawnedActor->GetName());
}