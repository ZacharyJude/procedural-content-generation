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
void AMapPointGenerator::Tick(float DeltaTime) {
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

	if (IsShowVertexLines) {
		DrawingLines.Append(VertexLines);
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

	MapPointGeneratorSweepline Sweepline{ Eps };
	Sweepline(SiteVoronoiPoints.cbegin(), SiteVoronoiPoints.cend());

	for (auto it = Sweepline.edges_.begin(); it != Sweepline.edges_.end(); ++it) {
		ProcessSweeplineEdge(Sweepline, *it);
	}

	for (auto it = VoronoiVertices.begin(); it != VoronoiVertices.end(); ++it) {
		SpawnLocation.X = it->X;
		SpawnLocation.Y = it->Y;
		SpawnLocation.Z = ElementZ;
		it->PointActor = SpawnActorInWorld(TEXT("Blueprint'/Game/GeneratedAssistPointBP.GeneratedAssistPointBP_C'"), SpawnLocation);
		// UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] Spawn voronoi diagram vertex (%f, %f)"), it->c.x, it->c.y);
	}

	UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] Generated [%d] edges for vonronoi diagram"), VoronoiEdges.size());
	for (auto it = VoronoiEdges.begin(); it != VoronoiEdges.end(); ++it) {
		UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] Generated site line from (%f, %f) to (%f, %f) for vonronoi diagram"), it->PSiteLeft->x, it->PSiteLeft->y, it->PSiteRight->x, it->PSiteRight->y);
		SiteLines.Add(FBatchedLine(
			FVector(it->PSiteLeft->x, it->PSiteLeft->y, ElementZ),
			FVector(it->PSiteRight->x, it->PSiteRight->y, ElementZ),
			SiteLineColor,
			LineInLifeTime,
			LineThinkness,
			SDPG_World
		));

		UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] Generated vertex line from (%f, %f) to (%f, %f) for vonronoi diagram"), VoronoiVertices[it->PVertexBegin].X, VoronoiVertices[it->PVertexBegin].Y, VoronoiVertices[it->PVertexEnd].X, VoronoiVertices[it->PVertexEnd].Y);
		VertexLines.Add(FBatchedLine(
			FVector(VoronoiVertices[it->PVertexBegin].X, VoronoiVertices[it->PVertexBegin].Y, ElementZ),
			FVector(VoronoiVertices[it->PVertexEnd].X, VoronoiVertices[it->PVertexEnd].Y, ElementZ),
			VertexLineColor,
			LineInLifeTime,
			LineThinkness,
			SDPG_World
		));
	}

	PreviousMapWidth = MapWidth;
	PreviousMapHeight = MapHeight;
}

void AMapPointGenerator::ProcessSweeplineEdge(MapPointGeneratorSweepline& Sweepline, MapPointGeneratorSweepline::edge& Edge) {
	VoronoiEdge CreatingEdge;
	CreatingEdge.PSiteLeft = Edge.l;
	CreatingEdge.PSiteRight = Edge.r;

	LineArgs Line;
	CreateLineArgs(Sweepline, Edge, Line);

	if (!Line.IsInfBegin) {
		CreatingEdge.PVertexBegin = CreateTruncatedFiniteVertex(Sweepline.vertices_[Edge.b], Line);
		if (Line.IsInfEnd) {
			CreatingEdge.PVertexEnd = CreateTruncatedInfiniteVertex(Sweepline.vertices_[Edge.b], Line);
		}
	}

	if (!Line.IsInfEnd) {
		CreatingEdge.PVertexEnd = CreateTruncatedFiniteVertex(Sweepline.vertices_[Edge.e], Line);
		if (Line.IsInfBegin) {
			CreatingEdge.PVertexBegin = CreateTruncatedInfiniteVertex(Sweepline.vertices_[Edge.e], Line);
		}
	}

	VoronoiEdges.push_back(CreatingEdge);
}

void AMapPointGenerator::CreateLineArgs(MapPointGeneratorSweepline& Sweepline, MapPointGeneratorSweepline::edge& Edge, LineArgs& Args) {
	Args.IsInfBegin = Edge.b == Sweepline.inf;
	Args.IsInfEnd = Edge.e == Sweepline.inf;
	const auto deltaX = Edge.l->x - Edge.r->x;
	const auto deltaY = Edge.l->y - Edge.r->y;
	Args.Slope = -deltaX / deltaY;

	if (!Args.IsInfBegin) {
		auto &verticeBegin = Sweepline.vertices_[Edge.b];
		Args.Intercept = verticeBegin.c.y - Args.Slope * verticeBegin.c.x;
	}
	else {
		auto &verticeEnd = Sweepline.vertices_[Edge.e];
		Args.Intercept = verticeEnd.c.y - Args.Slope * verticeEnd.c.x;
	}

	Args.IntersectXWithZeroY = -Args.Intercept / Args.Slope;
	Args.IntersectXWithMaxY = (MapHeight - Args.Intercept) / Args.Slope;
	Args.IntersectYWithZeroX = Args.Intercept;
	Args.IntersectYWithMaxX = Args.Slope * MapWidth + Args.Intercept;
}

vector<VoronoiVertex>::size_type AMapPointGenerator::CreateTruncatedFiniteVertex(const MapPointGeneratorSweepline::vertex& Vertex, const LineArgs& Args) {
	vector<VoronoiVertex>::size_type PtrVoronoiVertex;
	if (IsInsideMap(Vertex.c.x, Vertex.c.y)) {
		PtrVoronoiVertex = VoronoiVertices.size();
		VoronoiVertices.push_back(VoronoiVertex{ Vertex.c.x, Vertex.c.y, nullptr, false });
	}
	else {
		VoronoiVertex TruncatedVertex;
		TruncatedVertex.IsTruncated = true;
		TruncatedVertex.PointActor = nullptr;

		if (0.0 < Args.IntersectXWithZeroY && Args.IntersectXWithZeroY < MapWidth) {
			if (Vertex.c.y < 0.0) {
				TruncatedVertex.X = Args.IntersectXWithZeroY;
				TruncatedVertex.Y = 0.0;
			}
		}

		if (0.0 < Args.IntersectXWithMaxY && Args.IntersectXWithMaxY < MapWidth) {
			if (Vertex.c.y > MapHeight) {
				TruncatedVertex.X = Args.IntersectXWithMaxY;
				TruncatedVertex.Y = MapHeight;
			}
		}

		if (0.0 < Args.IntersectYWithZeroX && Args.IntersectYWithZeroX < MapHeight) {
			if (Vertex.c.x < 0.0) {
				TruncatedVertex.X = 0.0;
				TruncatedVertex.Y = Args.IntersectYWithZeroX;
			}
		}

		if (0.0 < Args.IntersectYWithMaxX && Args.IntersectYWithMaxX < MapHeight) {
			if (Vertex.c.x > MapWidth) {
				TruncatedVertex.X = MapWidth;
				TruncatedVertex.Y = Args.IntersectYWithMaxX;
			}
		}

		PtrVoronoiVertex = VoronoiVertices.size();
		VoronoiVertices.push_back(TruncatedVertex);
	}

	return PtrVoronoiVertex;
}

vector<VoronoiVertex>::size_type AMapPointGenerator::CreateTruncatedInfiniteVertex(const MapPointGeneratorSweepline::vertex& AnotherEndpointVertex, const LineArgs& Args) {
	vector<VoronoiVertex>::size_type PtrVoronoiVertex;
	if (Args.IsInfBegin) {
		if ((0 < Args.IntersectXWithZeroY && Args.IntersectXWithZeroY < AnotherEndpointVertex.c.x)) {
			PtrVoronoiVertex = VoronoiVertices.size();
			VoronoiVertices.push_back(VoronoiVertex{ Args.IntersectXWithZeroY, 0.0, nullptr, true });
		}
		else if (0 < Args.IntersectXWithMaxY && Args.IntersectXWithMaxY < AnotherEndpointVertex.c.x) {
			PtrVoronoiVertex = VoronoiVertices.size();
			VoronoiVertices.push_back(VoronoiVertex{ Args.IntersectXWithMaxY, MapHeight, nullptr, true });
		} 
		else {
			PtrVoronoiVertex = VoronoiVertices.size();
			VoronoiVertices.push_back(VoronoiVertex{ 0.0, Args.IntersectYWithZeroX, nullptr, true });
		}
	}
	else if(Args.IsInfEnd) {
		if (AnotherEndpointVertex.c.x < Args.IntersectXWithMaxY && Args.IntersectXWithMaxY < MapWidth) {
			PtrVoronoiVertex = VoronoiVertices.size();
			VoronoiVertices.push_back(VoronoiVertex{ Args.IntersectXWithMaxY, MapHeight, nullptr, true });
		}
		else if ((AnotherEndpointVertex.c.x < Args.IntersectXWithZeroY && Args.IntersectXWithZeroY < MapWidth)) {
			PtrVoronoiVertex = VoronoiVertices.size();
			VoronoiVertices.push_back(VoronoiVertex{ Args.IntersectXWithZeroY, 0.0, nullptr, true });
		}
		else {
			PtrVoronoiVertex = VoronoiVertices.size();
			VoronoiVertices.push_back(VoronoiVertex{ MapWidth, Args.IntersectYWithMaxX, nullptr, true });
		}
	}
	return PtrVoronoiVertex;
}

bool AMapPointGenerator::IsInsideMap(double x, double y) {
	return (0.0 < x || (0.0 - x < Eps)) &&
		(x < MapWidth || (x - MapWidth < Eps)) &&
		(0.0 < y || (0.0 - y < Eps)) &&
		(y < MapHeight || (y - MapHeight < Eps));
}

void AMapPointGenerator::Reset() {
	SiteLines.Reset();
	VertexLines.Reset();
	VoronoiEdges.clear();

	for (auto it = SiteVoronoiPoints.begin(); it != SiteVoronoiPoints.end(); ++it) {
		it->pointActor->Destroy();
		it->pointActor = nullptr;
	}
	SiteVoronoiPoints.clear();

	for (auto it = VoronoiVertices.begin(); it != VoronoiVertices.end(); ++it) {
		it->PointActor->Destroy();
		it->PointActor = nullptr;
	}
	VoronoiVertices.clear();
}