#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralEnvGenerator.generated.h"

class UHierarchicalInstancedStaticMeshComponent;

USTRUCT()
struct FFoliageInstanceData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 HISMIndex = 0; // 어떤 Foliage 타입인지

	UPROPERTY()
	int32 InstanceIndex = -1; // HISM 내부에서의 인덱스
};

USTRUCT()
struct FTileData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 GroundInstanceIndex = -1;

	UPROPERTY()
	TArray<FFoliageInstanceData> Foliages;
};

UCLASS()
class UNREALTEAMPJ_API AProceduralEnvGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	AProceduralEnvGenerator();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	// 시야 끝에서 자연스럽게 생성되도록 기본값을 10000(100m)으로 대폭 늘렸습니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Environment")
	float GenerationRadius = 2500.0f;
	
	// 삭제 반경도 여유있게 12000(120m)으로 늘립니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Environment")
	float DeletionRadius = 4000.0f;

	// 타일 크기. 넓은 공간을 덮도록 1000(10m)으로 설정.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Environment")
	float TileSize = 1000.0f;

	// 타일 당 생성할 최소 풀 개수 (비어있는 공간 방지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Environment")
	int32 MinFoliagePerTile = 750;

	// 타일 당 생성할 최대 풀 개수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Environment")
	int32 MaxFoliagePerTile = 1000;

	// --- Gimmick Variables ---
	// 스폰할 헛간 블루프린트 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Environment | Gimmick")
	TSubclassOf<AActor> BarnClass;

	// 헛간을 등장시키기 위해 직진해야 하는 목표 거리 (기본 150m = 15000.0f)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Environment | Gimmick")
	float TargetDistance = 15000.0f;

	// 헛간 주변 풀이 자라지 않는 반경 (기본 15m = 1500.0f)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Environment | Gimmick")
	float BarnClearRadius = 1500.0f;

	// 헛간이 스폰될 전방 거리 (기본 50m = 5000.0f)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Environment | Gimmick")
	float BarnSpawnAheadDistance = 5000.0f;

	// 허용 이탈 각도(내적 값). 0.8은 약 36도.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Environment | Gimmick")
	float AllowedDeviationDot = 0.8f;

	// 헛간이 스폰된 후, 플레이어가 이 거리만큼 멀어지면 헛간 삭제 및 스택 리셋 (기본 55m)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Environment | Gimmick")
	float BarnDeleteRadius = 5500.0f;

	// 시야각을 판단할 내적 값 (0.0 이면 시야의 90도 밖, 즉 등 뒤로 간주)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Environment | Gimmick")
	float BarnDisappearDot = 0.0f;

	// 헛간에 진입했을 때 고개를 돌렸다고 사라지는 것을 막는 안전 거리 (기본 15m)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Environment | Gimmick")
	float BarnSafeDistance = 1500.0f;

	// 헛간 주변 지형(풀, 바닥)을 확정적으로 생성시킬 반경 (허공이 보이지 않게 넉넉히 30m)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Environment | Gimmick")
	float BarnFloorGenerateRange = 3000.0f;

	// 헛간 주변 지형이 사라지지 않고 유지되는 반경 (생성 반경보다 커야 깜빡임 없음, 45m)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Environment | Gimmick")
	float BarnFloorDeleteRange = 4500.0f;

private:
	void UpdateGeneration();
	
	UPROPERTY()
	UHierarchicalInstancedStaticMeshComponent* GroundHISM;

	UPROPERTY()
	TArray<UHierarchicalInstancedStaticMeshComponent*> FoliageHISMs;

	// 현재 활성화된 타일의 데이터를 보관 (좌표 -> 데이터)
	UPROPERTY()
	TMap<FIntPoint, FTileData> ActiveTiles;

	// 깜빡임 방지를 위한 인스턴스 풀링(재활용) 데이터
	TArray<int32> FreeGroundIndices;
	TMap<int32, TArray<int32>> FreeFoliageIndices;

	// 기믹용 내부 상태 변수
	FVector StartLocation;
	FVector InitialDirection;
	bool bIsTracking = false;
	bool bIsBarnSpawned = false;
	FVector BarnLocation;
	
	UPROPERTY()
	AActor* SpawnedBarnInstance = nullptr;
};
