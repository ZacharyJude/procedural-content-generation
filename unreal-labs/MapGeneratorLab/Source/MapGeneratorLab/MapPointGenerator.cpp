#include "MapPointGenerator.h"

// Sets default values
AMapPointGenerator::AMapPointGenerator() {
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SiteParticleTemplate = CreateDefaultSubobject<UParticleSystem>(TEXT("SiteParticleTemplate"));
	VerticeParticleTemplate = CreateDefaultSubobject<UParticleSystem>(TEXT("VerticeParticleTemplate"));
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
		SpawnLocation.Z = ElementZ;

		auto VPoint = VoronoiPoint{ SpawnLocation.X, SpawnLocation.Y };
		VPoint.pointActor = SpawnActorInWorld(TEXT("Blueprint'/Game/GeneratedPointBP.GeneratedPointBP_C'"), SpawnLocation);
		VoronoiPoints.push_back(VPoint);

		UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] Spawn site (%f, %f)"), SpawnLocation.X, SpawnLocation.Y);

		VectorFromSobol.clear();
	}

	sort(VoronoiPoints.begin(), VoronoiPoints.end());

	Sweepline(VoronoiPoints.cbegin(), VoronoiPoints.cend());
	for (auto it = Sweepline.vertices_.begin(); it != Sweepline.vertices_.end(); ++it) {
		SpawnLocation.X = it->c.x;
		SpawnLocation.Y = it->c.y;
		SpawnLocation.Z = ElementZ;
		it->c.pointActor = SpawnActorInWorld(TEXT("Blueprint'/Game/GeneratedAssistPointBP.GeneratedAssistPointBP_C'"), SpawnLocation);
		UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] Spawn voronoi diagram vertice (%f, %f)"), it->c.x, it->c.y);
	}

	for (auto it = Sweepline.edges_.cbegin(); it != Sweepline.edges_.cend(); ++it) {
		SiteLines.Add(FBatchedLine(
			FVector(it->l->x, it->l->y, ElementZ),
			FVector(it->r->x, it->r->y, ElementZ),
			SiteLineColor,
			1000.0f,
			LineThinkness,
			SDPG_World
		));
		//auto beamEmitter = UGameplayStatics::SpawnEmitterAttached(SiteParticleTemplate, it->l->pointActor->GetRootComponent(), NAME_None, FVector(it->l->x, it->l->y, ElementZ), FRotator::ZeroRotator, EAttachLocation::KeepWorldPosition, false);
		////auto beamEmitter = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SiteParticleTemplate, FVector(it->l->x, it->l->y, ElementZ), FRotator::ZeroRotator, false);
		//beamEmitter->SetBeamSourcePoint(0, FVector(it->l->x, it->l->y, ElementZ), 0);
		//beamEmitter->SetBeamTargetPoint(0, FVector(it->r->x, it->r->y, ElementZ), 0);
	}
}

// Called every frame
void AMapPointGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	GetWorld()->LineBatcher->DrawLines(SiteLines);
}

AActor* AMapPointGenerator::SpawnActorInWorld(const TCHAR* ClassName, const FVector& SpawnLocation) {
	auto World = this->GetWorld();
	if (!World) {
		UE_LOG(LogTemp, Fatal, TEXT("[AMapPointGenerator.Fatal] world is null"));
		return nullptr;
	}

	auto SpawningClass = LoadClass<AActor>(nullptr, ClassName, nullptr, LOAD_None, nullptr);
	if (nullptr == SpawningClass) {
		UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] spawning class [%s] can not be found"), ClassName);
		return nullptr;
	}

	FActorSpawnParameters spawnParameters;
	spawnParameters.Owner = this;

	FRotator Rotator = this->GetActorRotation();

	auto SpawnedActor = World->SpawnActor<AActor>(SpawningClass, SpawnLocation, Rotator, spawnParameters);
	if (nullptr == SpawnedActor) {
		UE_LOG(LogTemp, Warning, TEXT("[AMapPointGenerator.Warning] spawned actor for class [%s] is null"), ClassName);
		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] spawned actor for class [%s] named [%s]"), ClassName, *SpawnedActor->GetName());

	return SpawnedActor;
}