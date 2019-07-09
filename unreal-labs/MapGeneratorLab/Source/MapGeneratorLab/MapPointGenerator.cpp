#include "MapPointGenerator.h"

struct VoronoiPoint {
	double x, y;

	bool operator < (const VoronoiPoint& p) const {
		return tie(x, y) < tie(p.x, p.y);
	}
};

// Sets default values
AMapPointGenerator::AMapPointGenerator() {
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMapPointGenerator::BeginPlay() {
	Super::BeginPlay();

	auto Generator = new SobolGenerator(HowManyGenerating, 2);
	vector<double> VectorFromSobol;
	vector<VoronoiPoint> VoronoiPoints;
	FVector SpawnLocation = FVector();
	while (Generator->GetNext(VectorFromSobol)) {
		SpawnLocation.X = VectorFromSobol[0] * Scalar;
		SpawnLocation.Y = VectorFromSobol[1] * Scalar;
		SpawnLocation.Z = 0.0;
		SpawnActorInWorld(TEXT("Blueprint'/Game/GeneratedPointBP.GeneratedPointBP_C'"), SpawnLocation);

		VoronoiPoints.push_back(VoronoiPoint{ SpawnLocation.X, SpawnLocation.Y });
		UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] Spawn location (%f, %f)"), SpawnLocation.X, SpawnLocation.Y);
		VectorFromSobol.clear();
	}

	sort(VoronoiPoints.begin(), VoronoiPoints.end());

	sweepline<vector<VoronoiPoint>::const_iterator, VoronoiPoint, double> Sweepline{ 1e-8 };
	Sweepline(VoronoiPoints.cbegin(), VoronoiPoints.cend());
	for (auto it = Sweepline.vertices_.cbegin(); it != Sweepline.vertices_.cend(); ++it) {
		UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] Voronoi diagram vertices (%f, %f)"), it->c.x, it->c.y);
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