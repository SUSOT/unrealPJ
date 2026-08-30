#include "World/InfiniteMapInstances.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "LandscapeProxy.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace InfiniteMap
{
	constexpr float GrassSpacing = 100.0f;
	constexpr float GrassJitter = 28.0f;
	constexpr float RoadExclusionHalfWidth = 500.0f;
	constexpr float MinimumGrassHeight = -500.0f;
	constexpr float MinimumSurfaceNormalZ = 0.72f;
	constexpr float RoadSegmentHalfLength = 1000.0f;
}

AInfiniteMapInstances::AInfiniteMapInstances()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Static);
	SetRootComponent(SceneRoot);

	Grass01 = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Grass01"));
	Grass02 = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Grass02"));
	Grass03 = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Grass03"));
	ExtendedRoad = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("ExtendedRoad"));

	Grass01->SetupAttachment(SceneRoot);
	Grass02->SetupAttachment(SceneRoot);
	Grass03->SetupAttachment(SceneRoot);
	ExtendedRoad->SetupAttachment(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Grass01Asset(TEXT("/Game/PN_GrassLibrary/Meshes/grassMesh/grass_01_01_mesh.grass_01_01_mesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Grass02Asset(TEXT("/Game/PN_GrassLibrary/Meshes/grassMesh/grass_01_05_mesh.grass_01_05_mesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Grass03Asset(TEXT("/Game/PN_GrassLibrary/Meshes/grassMesh/grass_02_03_mesh.grass_02_03_mesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RoadMaterialAsset(TEXT("/Game/Fab/Megascans/Surfaces/Fine_American_Road_sjfnbeaa/Medium/sjfnbeaa_tier_2/Materials/MI_sjfnbeaa.MI_sjfnbeaa"));

	Grass01->SetStaticMesh(Grass01Asset.Object);
	Grass02->SetStaticMesh(Grass02Asset.Object);
	Grass03->SetStaticMesh(Grass03Asset.Object);
	ExtendedRoad->SetStaticMesh(CubeAsset.Object);
	ExtendedRoad->SetMaterial(0, RoadMaterialAsset.Object);

	for (UHierarchicalInstancedStaticMeshComponent* GrassComponent : {Grass01.Get(), Grass02.Get(), Grass03.Get()})
	{
		GrassComponent->SetMobility(EComponentMobility::Static);
		GrassComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GrassComponent->SetCastShadow(false);
		GrassComponent->InstanceStartCullDistance = 8000;
		GrassComponent->InstanceEndCullDistance = 15000;
	}

	ExtendedRoad->SetMobility(EComponentMobility::Static);
	ExtendedRoad->SetCollisionProfileName(TEXT("BlockAll"));
	ExtendedRoad->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AInfiniteMapInstances::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildInstances();
}

void AInfiniteMapInstances::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Grass01 && Grass02 && Grass03 && ExtendedRoad &&
		(Grass01->GetInstanceCount() == 0 || ExtendedRoad->GetInstanceCount() == 0))
	{
		RebuildInstances();
	}
}

void AInfiniteMapInstances::RebuildInstances()
{
	if (!Grass01 || !Grass02 || !Grass03 || !ExtendedRoad)
	{
		return;
	}

	Grass01->ClearInstances();
	Grass02->ClearInstances();
	Grass03->ClearInstances();
	ExtendedRoad->ClearInstances();


	ALandscapeProxy* Landscape = nullptr;
	for (TActorIterator<ALandscapeProxy> It(GetWorld()); It; ++It)
	{
		Landscape = *It;
		break;
	}

	if (!Landscape)
	{
		return;
	}

	FVector LandscapeOrigin;
	FVector LandscapeExtent;
	Landscape->GetActorBounds(false, LandscapeOrigin, LandscapeExtent);
	PopulateGrass(Landscape);
	PopulateRoad(LandscapeOrigin.Y - LandscapeExtent.Y, LandscapeOrigin.Y + LandscapeExtent.Y);
}

void AInfiniteMapInstances::PopulateGrass(ALandscapeProxy* Landscape)
{
	FVector LandscapeOrigin;
	FVector LandscapeExtent;
	Landscape->GetActorBounds(false, LandscapeOrigin, LandscapeExtent);

	const float MinimumX = LandscapeOrigin.X - LandscapeExtent.X + InfiniteMap::GrassSpacing * 0.5f;
	const float MaximumX = LandscapeOrigin.X + LandscapeExtent.X - InfiniteMap::GrassSpacing * 0.5f;
	const float MinimumY = LandscapeOrigin.Y - LandscapeExtent.Y + InfiniteMap::GrassSpacing * 0.5f;
	const float MaximumY = LandscapeOrigin.Y + LandscapeExtent.Y - InfiniteMap::GrassSpacing * 0.5f;
	const int32 Columns = FMath::FloorToInt((MaximumX - MinimumX) / InfiniteMap::GrassSpacing) + 1;
	const int32 Rows = FMath::FloorToInt((MaximumY - MinimumY) / InfiniteMap::GrassSpacing) + 1;
	FRandomStream Random(73421);

	UHierarchicalInstancedStaticMeshComponent* GrassComponents[] = {Grass01, Grass02, Grass03};
	const FVector2D ScaleRanges[] = {
		FVector2D(1.05f, 1.25f),
		FVector2D(1.10f, 1.32f),
		FVector2D(1.20f, 1.45f)};

	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Column = 0; Column < Columns; ++Column)
		{
			const float X = MinimumX + Column * InfiniteMap::GrassSpacing + Random.FRandRange(-InfiniteMap::GrassJitter, InfiniteMap::GrassJitter);
			const float Y = MinimumY + Row * InfiniteMap::GrassSpacing + Random.FRandRange(-InfiniteMap::GrassJitter, InfiniteMap::GrassJitter);

			if (FMath::Abs(X) <= InfiniteMap::RoadExclusionHalfWidth)
			{
				continue;
			}

			const TOptional<float> Height = Landscape->GetHeightAtLocation(FVector(X, Y, 0.0f), EHeightfieldSource::Complex);
			if (!Height.IsSet() || Height.GetValue() < InfiniteMap::MinimumGrassHeight)
			{
				continue;
			}

			const TOptional<float> HeightX = Landscape->GetHeightAtLocation(FVector(X + InfiniteMap::GrassSpacing, Y, 0.0f), EHeightfieldSource::Complex);
			const TOptional<float> HeightY = Landscape->GetHeightAtLocation(FVector(X, Y + InfiniteMap::GrassSpacing, 0.0f), EHeightfieldSource::Complex);
			if (!HeightX.IsSet() || !HeightY.IsSet())
			{
				continue;
			}

			const FVector SurfaceNormal(
				-(HeightX.GetValue() - Height.GetValue()) / InfiniteMap::GrassSpacing,
				-(HeightY.GetValue() - Height.GetValue()) / InfiniteMap::GrassSpacing,
				1.0f);
			if (SurfaceNormal.GetSafeNormal().Z < InfiniteMap::MinimumSurfaceNormalZ)
			{
				continue;
			}

			const int32 ComponentIndex = (Row * 17 + Column * 31) % UE_ARRAY_COUNT(GrassComponents);
			const float UniformScale = Random.FRandRange(ScaleRanges[ComponentIndex].X, ScaleRanges[ComponentIndex].Y);
			GrassComponents[ComponentIndex]->AddInstance(
				FTransform(
					FRotator(0.0f, Random.FRandRange(-180.0f, 180.0f), 0.0f),
					FVector(X, Y, Height.GetValue() + 2.0f),
					FVector(UniformScale)),
				false);
		}
	}
}

void AInfiniteMapInstances::PopulateRoad(const float MinimumY, const float MaximumY)
{
	for (float Y = 18270.0f; Y + InfiniteMap::RoadSegmentHalfLength <= MaximumY; Y += 2000.0f)
	{
		ExtendedRoad->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, Y, 202.0f), FVector(7.0f, 20.0f, 1.0f)), false);
	}

	for (float Y = -17640.0f; Y - InfiniteMap::RoadSegmentHalfLength >= MinimumY; Y -= 2000.0f)
	{
		ExtendedRoad->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, Y, 202.0f), FVector(7.0f, 20.0f, 1.0f)), false);
	}
}
