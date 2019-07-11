#include "MapPointGenerator.h"

const double AMapPointGenerator::Eps = 1e-8;

// Sets default values
AMapPointGenerator::AMapPointGenerator() {
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMapPointGenerator::BeginPlay() {
	Super::BeginPlay();

	if ((abs(MapWidth - 0.0) < Eps) || (abs(MapHeight - 0.0) < Eps)) {
		return;
	}

	Generate();
}

// Called every frame
void AMapPointGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if ((abs(PreviousMapWidth - MapWidth) > Eps) || (abs(PreviousMapHeight - MapHeight) > Eps)) {
		// The size of the map has changed. --Zachary
		Generate();
		return;
	}

	TArray<FBatchedLine> DrawingLines;
	if (IsShowSiteLines) {
		DrawingLines.Append(SiteLines);
	}

	if (IsShowVerticeLines) {
		DrawingLines.Append(VerticeLines);
	}

	GetWorld()->LineBatcher->DrawLines(DrawingLines);
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

	//UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] spawned actor for class [%s] named [%s]"), ClassName, *SpawnedActor->GetName());

	return SpawnedActor;
}

void AMapPointGenerator::Generate() {
	Reset();

	UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Debug] In generation, previous map width [%f] current map width [%f] previous map height [%f] map height [%f]"), PreviousMapWidth, MapWidth, PreviousMapHeight, MapHeight);
	auto Generator = new SobolGenerator(HowManyGenerating, 2);
	vector<double> VectorFromSobol;
	FVector SpawnLocation = FVector();
	while (Generator->GetNext(VectorFromSobol)) {
		SpawnLocation.X = VectorFromSobol[0] * MapWidth;
		SpawnLocation.Y = VectorFromSobol[1] * MapHeight;
		SpawnLocation.Z = ElementZ;

		auto VPoint = VoronoiPoint{ SpawnLocation.X, SpawnLocation.Y };
		VPoint.pointActor = SpawnActorInWorld(TEXT("Blueprint'/Game/GeneratedPointBP.GeneratedPointBP_C'"), SpawnLocation);
		SiteVoronoiPoints.push_back(VPoint);

		//UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] Spawn site (%f, %f)"), SpawnLocation.X, SpawnLocation.Y);

		VectorFromSobol.clear();
	}

	sort(SiteVoronoiPoints.begin(), SiteVoronoiPoints.end());

	Sweepline(SiteVoronoiPoints.cbegin(), SiteVoronoiPoints.cend());
	for (auto it = Sweepline.vertices_.begin(); it != Sweepline.vertices_.end(); ++it) {
		SpawnLocation.X = it->c.x;
		SpawnLocation.Y = it->c.y;
		SpawnLocation.Z = ElementZ;
		it->c.pointActor = SpawnActorInWorld(TEXT("Blueprint'/Game/GeneratedAssistPointBP.GeneratedAssistPointBP_C'"), SpawnLocation);
		//UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] Spawn voronoi diagram vertice (%f, %f)"), it->c.x, it->c.y);
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
		auto isInfBegin = it->b == Sweepline.inf;
		auto isInfEnd = it->e == Sweepline.inf;

		if (!isInfBegin && !isInfEnd) {
			auto verticeBegin = Sweepline.vertices_[it->b];
			auto verticeEnd = Sweepline.vertices_[it->e];

			VerticeLines.Add(FBatchedLine(
				FVector(verticeBegin.c.x, verticeBegin.c.y, ElementZ),
				FVector(verticeEnd.c.x, verticeEnd.c.y, ElementZ),
				VerticeLineColor,
				1000.0f,
				LineThinkness,
				SDPG_World
			));
		}
	}

	PreviousMapWidth = MapWidth;
	PreviousMapHeight = MapHeight;
}

void AMapPointGenerator::Reset() {
	for (auto it = SiteVoronoiPoints.begin(); it != SiteVoronoiPoints.end(); ++it) {
		it->pointActor->Destroy();
		it->pointActor = nullptr;
	}
	SiteVoronoiPoints.clear();

	for (auto it = Sweepline.vertices_.begin(); it != Sweepline.vertices_.end(); ++it) {
		it->c.pointActor->Destroy();
		it->c.pointActor = nullptr;
	}
	Sweepline.clear();

	SiteLines.Reset();
	VerticeLines.Reset();
}