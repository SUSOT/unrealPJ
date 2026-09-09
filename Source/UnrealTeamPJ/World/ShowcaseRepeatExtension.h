#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShowcaseRepeatExtension.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

USTRUCT(BlueprintType)
struct FShowcaseRepeatMeshGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repeat")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repeat")
	TArray<TObjectPtr<UMaterialInterface>> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repeat")
	TArray<FTransform> SourceTransforms;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repeat")
	bool bCollisionEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repeat")
	bool bCastShadow = true;

	// Zero keeps the architectural shell visible at any distance.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repeat")
	int32 DetailCullDistance = 0;
};

/** Serialized HISM geometry for Showcase1. No per-frame spawning or construction rebuild. */
UCLASS()
class UNREALTEAMPJ_API AShowcaseRepeatExtension : public AActor
{
	GENERATED_BODY()

public:
	AShowcaseRepeatExtension();
	virtual void Tick(float DeltaSeconds) override;

	/** Recycle distant straight cells around the player; never move the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Showcase Extension")
	bool bInfiniteStraight = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Showcase Extension")
	int32 FirstCellIndex = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Showcase Extension")
	int32 RecycledCellCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Showcase Extension", meta = (ClampMin = "1", ClampMax = "1000"))
	int32 RepeatCount = 196;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Showcase Extension", meta = (ClampMin = "1"))
	float RepeatSpacing = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Showcase Extension")
	TArray<FShowcaseRepeatMeshGroup> MeshGroups;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Showcase Extension")
	TArray<TObjectPtr<AActor>> RepeatedFixtures;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Showcase Extension")
	void RebuildExtension();

	UFUNCTION(BlueprintPure, Category = "Showcase Extension")
	int32 GetRepeatedInstanceCount() const;

	// Template spawning also works in unattended editor commandlets, unlike
	// the editor clipboard duplication path which requires a full UI editor.
	UFUNCTION(BlueprintCallable, Category = "Showcase Extension")
	AActor* DuplicateFixture(AActor* Source, FVector WorldOffset);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Showcase Extension")
	void ApplyFixtureDistanceLimits();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	TArray<int32> CellIndices;
	UPROPERTY(VisibleAnywhere, Instanced, Category = "Showcase Extension")
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> GeneratedMeshes;
};
