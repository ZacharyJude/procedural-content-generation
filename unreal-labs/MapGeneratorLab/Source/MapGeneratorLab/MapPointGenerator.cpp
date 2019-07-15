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

	const auto IsInfBegin = Edge.b == Sweepline.inf;
	const auto IsInfEnd = Edge.e == Sweepline.inf;
	const auto deltaX = Edge.l->x - Edge.r->x;
	const auto deltaY = Edge.l->y - Edge.r->y;
	auto m = -deltaX / deltaY;

	// Base on the precondition of Fortune's algorithm implementation, at most one of the two vertices will be inf. --Zachary
	if (!IsInfBegin) {
		auto &verticeBegin = Sweepline.vertices_[Edge.b];
		auto k = verticeBegin.c.y - m * verticeBegin.c.x;
		auto intersectXWithZeroY = -k / m;
		auto intersectXWithMaxY = (MapHeight - k) / m;
		auto intersectYWithZeroX = k;
		auto intersectYWithMaxX = m * MapWidth + k;

		if (IsInsideMap(verticeBegin.c.x, verticeBegin.c.y)) {
			CreatingEdge.PVertexBegin = VoronoiVertices.size();
			VoronoiVertices.push_back(VoronoiVertex{ verticeBegin.c.x, verticeBegin.c.y, nullptr, false });
		}
		else {
			UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] begin (%f, %f) not in map, m is [%f]"), verticeBegin.c.x, verticeBegin.c.y, m);
			VoronoiVertex TruncatedVertex;
			TruncatedVertex.IsTruncated = true;
			TruncatedVertex.PointActor = nullptr;

			if (0.0 < intersectXWithZeroY && intersectXWithZeroY < MapWidth) {
				if (verticeBegin.c.y < 0.0) {
					TruncatedVertex.X = intersectXWithZeroY;
					TruncatedVertex.Y = 0.0;
				}
			}

			if (0.0 < intersectXWithMaxY && intersectXWithMaxY < MapWidth) {
				if (verticeBegin.c.y > MapHeight) {
					TruncatedVertex.X = intersectXWithMaxY;
					TruncatedVertex.Y = MapHeight;
				}
			}

			if (0.0 < intersectYWithZeroX && intersectYWithZeroX < MapHeight) {
				if (verticeBegin.c.x < 0.0) {
					TruncatedVertex.X = 0.0;
					TruncatedVertex.Y = intersectYWithZeroX;
				}
			}

			if (0.0 < intersectYWithMaxX && intersectYWithMaxX < MapHeight) {
				if (verticeBegin.c.x > MapWidth) {
					TruncatedVertex.X = MapWidth;
					TruncatedVertex.Y = intersectYWithMaxX;
				}
			}

			CreatingEdge.PVertexBegin = VoronoiVertices.size();
			VoronoiVertices.push_back(TruncatedVertex);
			UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] truncated begin (%f, %f), m for trauncation is [%f]"), TruncatedVertex.X, TruncatedVertex.Y, (TruncatedVertex.Y - verticeBegin.c.y) / (TruncatedVertex.X - verticeBegin.c.x));
		}

		if (IsInfEnd) {
			if (verticeBegin.c.x < intersectXWithMaxY && intersectXWithMaxY < MapWidth) {
				CreatingEdge.PVertexEnd = VoronoiVertices.size();
				VoronoiVertices.push_back(VoronoiVertex{ intersectXWithMaxY, MapHeight, nullptr, true });
			}
			else if ((verticeBegin.c.x < intersectXWithZeroY && intersectXWithZeroY < MapWidth)) {
				CreatingEdge.PVertexEnd = VoronoiVertices.size();
				VoronoiVertices.push_back(VoronoiVertex{ intersectXWithZeroY, 0.0, nullptr, true });
			}
			else {
				CreatingEdge.PVertexEnd = VoronoiVertices.size();
				VoronoiVertices.push_back(VoronoiVertex{ MapWidth, intersectYWithMaxX, nullptr, true });
			}
		}
	}

	if (!IsInfEnd) {
		auto &verticeEnd = Sweepline.vertices_[Edge.e];
		auto k = verticeEnd.c.y - m * verticeEnd.c.x;
		auto intersectXWithZeroY = -k / m;
		auto intersectXWithMaxY = (MapHeight - k) / m;
		auto intersectYWithZeroX = k;
		auto intersectYWithMaxX = m * MapWidth + k;

		if (IsInsideMap(verticeEnd.c.x, verticeEnd.c.y)) {
			CreatingEdge.PVertexEnd = VoronoiVertices.size();
			VoronoiVertices.push_back(VoronoiVertex{ verticeEnd.c.x, verticeEnd.c.y, nullptr, false });
		}
		else {
			UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] end (%f, %f) not in map, m is [%f]"), verticeEnd.c.x, verticeEnd.c.y, m);
			VoronoiVertex TruncatedVertex;
			TruncatedVertex.IsTruncated = true;
			TruncatedVertex.PointActor = nullptr;

			if (0.0 < intersectXWithZeroY && intersectXWithZeroY < MapWidth) {
				if (verticeEnd.c.y < 0.0) {
					TruncatedVertex.X = intersectXWithZeroY;
					TruncatedVertex.Y = 0.0;
				}
			}

			if (0.0 < intersectXWithMaxY && intersectXWithMaxY < MapWidth) {
				if (verticeEnd.c.y > MapHeight) {
					TruncatedVertex.X = intersectXWithMaxY;
					TruncatedVertex.Y = MapHeight;
				}
			}

			if (0.0 < intersectYWithZeroX && intersectYWithZeroX < MapHeight) {
				if (verticeEnd.c.x < 0.0) {
					TruncatedVertex.X = 0.0;
					TruncatedVertex.Y = intersectYWithZeroX;
				}
			}

			if (0.0 < intersectYWithMaxX && intersectYWithMaxX < MapHeight) {
				if (verticeEnd.c.x > MapWidth) {
					TruncatedVertex.X = MapWidth;
					TruncatedVertex.Y = intersectYWithMaxX;
				}
			}
			
			CreatingEdge.PVertexEnd = VoronoiVertices.size();
			VoronoiVertices.push_back(TruncatedVertex);
			UE_LOG(LogTemp, Log, TEXT("[AMapPointGenerator.Log] truncated end (%f, %f), m for trauncation is [%f]"), TruncatedVertex.X, TruncatedVertex.Y, (TruncatedVertex.Y - verticeEnd.c.y) / (TruncatedVertex.X - verticeEnd.c.x));
		}

		if (IsInfBegin) {
			if (0 < intersectXWithZeroY && intersectXWithZeroY < verticeEnd.c.x) {
				CreatingEdge.PVertexBegin = VoronoiVertices.size();
				VoronoiVertices.push_back(VoronoiVertex{ intersectXWithZeroY, 0.0, nullptr, true });
			}
			else if (0 < intersectXWithMaxY && intersectXWithMaxY < verticeEnd.c.x) {
				CreatingEdge.PVertexBegin = VoronoiVertices.size();
				VoronoiVertices.push_back(VoronoiVertex{ intersectXWithMaxY, MapHeight, nullptr, true });
			}
			else {
				CreatingEdge.PVertexBegin = VoronoiVertices.size();
				VoronoiVertices.push_back(VoronoiVertex{ 0.0, intersectYWithZeroX, nullptr, true });
			}
		}
	}

	VoronoiEdges.push_back(CreatingEdge);
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