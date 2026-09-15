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
};
