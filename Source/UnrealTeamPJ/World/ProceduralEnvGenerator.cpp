#include "ProceduralEnvGenerator.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "FoliageType_InstancedStaticMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

AProceduralEnvGenerator::AProceduralEnvGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	GroundHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("GroundHISM"));
	GroundHISM->SetupAttachment(RootComponent);
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshAsset(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMeshAsset.Succeeded())
	{
		GroundHISM->SetStaticMesh(PlaneMeshAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GroundMatAsset(TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Fab/Megascans/Surfaces/Dirt_Ground_xdhhdgq/Low/xdhhdgq_tier_3/Materials/MI_xdhhdgq.MI_xdhhdgq'"));
	if (GroundMatAsset.Succeeded())
	{
		GroundHISM->SetMaterial(0, GroundMatAsset.Object);
	}
	
	GroundHISM->SetCollisionProfileName(TEXT("BlockAll"));
}

void AProceduralEnvGenerator::BeginPlay()
{
	Super::BeginPlay();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	TArray<FAssetData> AssetData;
	FARFilter Filter;
	Filter.PackagePaths.Add(TEXT("/Game/PN_giantReed/FoliageTypes"));
	
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);

	for (const FAssetData& Data : AssetData)
	{
		UObject* LoadedAsset = Data.GetAsset();
		if (UFoliageType_InstancedStaticMesh* FoliageType = Cast<UFoliageType_InstancedStaticMesh>(LoadedAsset))
		{
			if (UStaticMesh* FoliageMesh = FoliageType->GetStaticMesh())
			{
				UHierarchicalInstancedStaticMeshComponent* NewFoliageHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
				NewFoliageHISM->SetupAttachment(RootComponent);
				NewFoliageHISM->RegisterComponent();
				NewFoliageHISM->SetStaticMesh(FoliageMesh);
				NewFoliageHISM->SetCollisionProfileName(TEXT("NoCollision")); 
				
				NewFoliageHISM->InstanceStartCullDistance = DeletionRadius;
				NewFoliageHISM->InstanceEndCullDistance = DeletionRadius + 1000.0f;
				
				FoliageHISMs.Add(NewFoliageHISM);
			}
		}
	}
	
	GroundHISM->InstanceStartCullDistance = DeletionRadius + 1000.0f;
	GroundHISM->InstanceEndCullDistance = DeletionRadius + 2000.0f;
}

void AProceduralEnvGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateGeneration();
}

void AProceduralEnvGenerator::UpdateGeneration()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn) return;

	FVector PlayerLoc = PlayerPawn->GetActorLocation();

	// 1. 거리가 DeletionRadius를 초과하는 타일을 풀(Pool)로 반환하여 숨김 (깜빡임 방지)
	TArray<FIntPoint> TilesToRemove;
	for (const auto& Pair : ActiveTiles)
	{
		FIntPoint GridPoint = Pair.Key;
		FVector TileCenter(GridPoint.X * TileSize, GridPoint.Y * TileSize, PlayerLoc.Z);
		
		if (FVector::Dist(PlayerLoc, TileCenter) > DeletionRadius)
		{
			TilesToRemove.Add(GridPoint);
		}
	}

	for (const FIntPoint& GridPoint : TilesToRemove)
	{
		FTileData& TileData = ActiveTiles[GridPoint];
		
		// 땅 인스턴스 숨기고 풀에 반납
		if (TileData.GroundInstanceIndex != -1)
		{
			// Z를 아주 낮게 설정해서 숨김
			GroundHISM->UpdateInstanceTransform(TileData.GroundInstanceIndex, FTransform(FVector(0, 0, -100000.0f)), true, true, true);
			FreeGroundIndices.Add(TileData.GroundInstanceIndex);
		}

		// 풀 인스턴스 숨기고 풀에 반납
		for (FFoliageInstanceData& FoliageData : TileData.Foliages)
		{
			if (FoliageHISMs.IsValidIndex(FoliageData.HISMIndex) && FoliageData.InstanceIndex != -1)
			{
				FoliageHISMs[FoliageData.HISMIndex]->UpdateInstanceTransform(FoliageData.InstanceIndex, FTransform(FVector(0, 0, -100000.0f)), true, true, true);
				FreeFoliageIndices.FindOrAdd(FoliageData.HISMIndex).Add(FoliageData.InstanceIndex);
			}
		}

		ActiveTiles.Remove(GridPoint);
	}

	// 2. GenerationRadius 내의 새로운 타일 생성
	int32 MinX = FMath::FloorToInt((PlayerLoc.X - GenerationRadius) / TileSize);
	int32 MaxX = FMath::CeilToInt((PlayerLoc.X + GenerationRadius) / TileSize);
	int32 MinY = FMath::FloorToInt((PlayerLoc.Y - GenerationRadius) / TileSize);
	int32 MaxY = FMath::CeilToInt((PlayerLoc.Y + GenerationRadius) / TileSize);

	for (int32 X = MinX; X <= MaxX; ++X)
	{
		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			FIntPoint GridPoint(X, Y);
			FVector TileCenter(X * TileSize, Y * TileSize, PlayerLoc.Z);
			
			if (FVector::Dist(PlayerLoc, TileCenter) <= GenerationRadius)
			{
				if (!ActiveTiles.Contains(GridPoint))
				{
					FTileData NewTileData;

					// 땅 트랜스폼
					FTransform GroundTransform;
					GroundTransform.SetLocation(FVector(X * TileSize, Y * TileSize, 0.0f));
					GroundTransform.SetScale3D(FVector(TileSize / 100.0f, TileSize / 100.0f, 1.0f));
					
					// 빈 인스턴스가 있다면 재활용, 없으면 새로 추가
					if (FreeGroundIndices.Num() > 0)
					{
						NewTileData.GroundInstanceIndex = FreeGroundIndices.Pop();
						GroundHISM->UpdateInstanceTransform(NewTileData.GroundInstanceIndex, GroundTransform, true, true, true);
					}
					else
					{
						NewTileData.GroundInstanceIndex = GroundHISM->AddInstance(GroundTransform);
					}

					// 일관된 프로시저럴 생성을 위해 GridPoint를 기반으로 시드 설정
					FRandomStream RandomStream;
					RandomStream.Initialize(GridPoint.X * 131071 + GridPoint.Y * 524287);

					if (FoliageHISMs.Num() > 0)
					{
						// 최소 수치 보장
						int32 NumFoliage = RandomStream.RandRange(MinFoliagePerTile, MaxFoliagePerTile);
						for (int32 i = 0; i < NumFoliage; ++i)
						{
							FFoliageInstanceData FoliageData;
							FoliageData.HISMIndex = RandomStream.RandRange(0, FoliageHISMs.Num() - 1);
							
							float RandX = (X * TileSize) + RandomStream.FRandRange(-TileSize * 0.5f, TileSize * 0.5f);
							float RandY = (Y * TileSize) + RandomStream.FRandRange(-TileSize * 0.5f, TileSize * 0.5f);
							
							FTransform FoliageTransform;
							FoliageTransform.SetLocation(FVector(RandX, RandY, 0.0f));
							FoliageTransform.SetRotation(FQuat(FRotator(0.0f, RandomStream.FRandRange(0.0f, 360.0f), 0.0f)));
							float RandScale = RandomStream.FRandRange(0.8f, 1.5f);
							FoliageTransform.SetScale3D(FVector(RandScale));

							TArray<int32>& FreeList = FreeFoliageIndices.FindOrAdd(FoliageData.HISMIndex);
							if (FreeList.Num() > 0)
							{
								FoliageData.InstanceIndex = FreeList.Pop();
								FoliageHISMs[FoliageData.HISMIndex]->UpdateInstanceTransform(FoliageData.InstanceIndex, FoliageTransform, true, true, true);
							}
							else
							{
								FoliageData.InstanceIndex = FoliageHISMs[FoliageData.HISMIndex]->AddInstance(FoliageTransform);
							}

							NewTileData.Foliages.Add(FoliageData);
						}
					}

					ActiveTiles.Add(GridPoint, NewTileData);
				}
			}
		}
	}
}
