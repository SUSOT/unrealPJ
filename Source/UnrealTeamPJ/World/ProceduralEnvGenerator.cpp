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

	// --- 기믹 로직: 방향 및 거리 추적 ---
	if (!bIsTracking)
	{
		StartLocation = PlayerLoc;
		InitialDirection = PlayerPawn->GetActorForwardVector();
		bIsTracking = true;
	}
	else if (!bIsBarnSpawned)
	{
		float TraveledDistance = FVector::Dist(StartLocation, PlayerLoc);
		
		// 너무 가까울 때 방향이 튀는 것을 방지 (1m 이상 이동 시부터 판정)
		if (TraveledDistance > 100.0f)
		{
			FVector CurrentDir = (PlayerLoc - StartLocation).GetSafeNormal();
			float DotResult = FVector::DotProduct(InitialDirection, CurrentDir);
			
			// 허용 각도 이탈 시 스택(시작 위치 및 방향) 초기화
			if (DotResult < AllowedDeviationDot)
			{
				StartLocation = PlayerLoc;
				InitialDirection = PlayerPawn->GetActorForwardVector();
			}
			// 목표 거리 도달 시 헛간 스폰
			else if (TraveledDistance >= TargetDistance)
			{
				bIsBarnSpawned = true;
				// Z축은 0 기준으로 헛간 생성
				BarnLocation = FVector(PlayerLoc.X, PlayerLoc.Y, 0.0f) + (InitialDirection * BarnSpawnAheadDistance);
				
				if (BarnClass)
				{
					SpawnedBarnInstance = GetWorld()->SpawnActor<AActor>(BarnClass, BarnLocation, InitialDirection.Rotation());
				}
			}
		}
	}
	else
	{
		// 헛간이 스폰된 상태: 삭제 조건 판정
		if (SpawnedBarnInstance)
		{
			float DistToBarn = FVector::Dist(PlayerLoc, BarnLocation);
			bool bShouldDelete = false;

			// 조건 1: 단순히 거리가 너무 멀어졌을 때 (55m 이상)
			if (DistToBarn > BarnDeleteRadius)
			{
				bShouldDelete = true;
			}
			// 조건 2: 안전 거리 밖에서, 헛간이 시야(각도)를 완전히 벗어났을 때
			else if (DistToBarn > BarnSafeDistance)
			{
				APlayerCameraManager* CamManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
				if (CamManager)
				{
					FVector CamForward = CamManager->GetCameraRotation().Vector();
					FVector DirToBarn = (BarnLocation - CamManager->GetCameraLocation()).GetSafeNormal();
					
					// 내적 값이 기준치(0.0)보다 낮으면 등 뒤(시야 밖)로 간주
					if (FVector::DotProduct(CamForward, DirToBarn) < BarnDisappearDot)
					{
						bShouldDelete = true;
					}
				}
			}

			if (bShouldDelete)
			{
				SpawnedBarnInstance->Destroy();
				SpawnedBarnInstance = nullptr;
				
				// 헛간이 사라질 때, 헛간 주변 공터 타일들을 풀(Pool)로 반납하여 강제 재생성 유도
				// 이렇게 하면 텅 비어있던 흙바닥이 즉시 풀밭으로 다시 덮이게 됩니다.
				TArray<FIntPoint> TilesToRefresh;
				for (const auto& Pair : ActiveTiles)
				{
					FVector TileCenter(Pair.Key.X * TileSize, Pair.Key.Y * TileSize, PlayerLoc.Z);
					if (FVector::Dist2D(BarnLocation, TileCenter) <= BarnFloorGenerateRange)
					{
						TilesToRefresh.Add(Pair.Key);
					}
				}
				
				for (const FIntPoint& GridPoint : TilesToRefresh)
				{
					FTileData& TileData = ActiveTiles[GridPoint];
					
					if (TileData.GroundInstanceIndex != -1)
					{
						GroundHISM->UpdateInstanceTransform(TileData.GroundInstanceIndex, FTransform(FVector(0, 0, -100000.0f)), true, true, true);
						FreeGroundIndices.Add(TileData.GroundInstanceIndex);
					}

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

				// 기믹 상태 리셋 (다시 직진하면 새로운 헛간이 나올 수 있도록)
				bIsBarnSpawned = false;
				bIsTracking = false; 
			}
		}
	}
	// -----------------------------------

	// 1. 거리가 DeletionRadius를 초과하는 타일을 풀(Pool)로 반환하여 숨김 (깜빡임 방지)
	TArray<FIntPoint> TilesToRemove;
	for (const auto& Pair : ActiveTiles)
	{
		FIntPoint GridPoint = Pair.Key;
		FVector TileCenter(GridPoint.X * TileSize, GridPoint.Y * TileSize, PlayerLoc.Z);
		
		bool bFarFromPlayer = FVector::Dist(PlayerLoc, TileCenter) > DeletionRadius;
		bool bFarFromBarn = bIsBarnSpawned ? (FVector::Dist2D(BarnLocation, TileCenter) > BarnFloorDeleteRange) : true;
		
		// 플레이어와 헛간 양쪽 모두에서 멀어졌을 때만 삭제
		if (bFarFromPlayer && bFarFromBarn)
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

	// 2. GenerationRadius 내의 새로운 타일 생성 (헛간이 있다면 헛간 주변도 포함)
	float MinBoundsX = PlayerLoc.X - GenerationRadius;
	float MaxBoundsX = PlayerLoc.X + GenerationRadius;
	float MinBoundsY = PlayerLoc.Y - GenerationRadius;
	float MaxBoundsY = PlayerLoc.Y + GenerationRadius;

	if (bIsBarnSpawned)
	{
		MinBoundsX = FMath::Min(MinBoundsX, BarnLocation.X - BarnFloorGenerateRange);
		MaxBoundsX = FMath::Max(MaxBoundsX, BarnLocation.X + BarnFloorGenerateRange);
		MinBoundsY = FMath::Min(MinBoundsY, BarnLocation.Y - BarnFloorGenerateRange);
		MaxBoundsY = FMath::Max(MaxBoundsY, BarnLocation.Y + BarnFloorGenerateRange);
	}

	int32 MinX = FMath::FloorToInt(MinBoundsX / TileSize);
	int32 MaxX = FMath::CeilToInt(MaxBoundsX / TileSize);
	int32 MinY = FMath::FloorToInt(MinBoundsY / TileSize);
	int32 MaxY = FMath::CeilToInt(MaxBoundsY / TileSize);

	for (int32 X = MinX; X <= MaxX; ++X)
	{
		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			FIntPoint GridPoint(X, Y);
			FVector TileCenter(X * TileSize, Y * TileSize, PlayerLoc.Z);
			
			bool bNearPlayer = FVector::Dist(PlayerLoc, TileCenter) <= GenerationRadius;
			bool bNearBarn = bIsBarnSpawned ? (FVector::Dist2D(BarnLocation, TileCenter) <= BarnFloorGenerateRange) : false;

			if (bNearPlayer || bNearBarn)
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
							
							// --- 기믹 로직: 헛간 주변 풀 제거 ---
							if (bIsBarnSpawned && FVector::Dist2D(FVector(RandX, RandY, 0.0f), FVector(BarnLocation.X, BarnLocation.Y, 0.0f)) < BarnClearRadius)
							{
								continue;
							}
							// -----------------------------------

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
