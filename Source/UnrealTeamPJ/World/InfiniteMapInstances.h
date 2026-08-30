#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InfiniteMapInstances.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class USceneComponent;
class ALandscapeProxy;

/**
 * Persistent, draw-call-friendly decoration for TestLevel's visual extension.
 * The instances are rebuilt deterministically so they remain available after
 * an editor reload and in packaged builds.
 */
UCLASS()
class UNREALTEAMPJ_API AInfiniteMapInstances : public AActor
{
	GENERATED_BODY()

public:
	AInfiniteMapInstances();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;

private:
	void RebuildInstances();
	void PopulateGrass(ALandscapeProxy* Landscape);
	void PopulateRoad(float MinimumY, float MaximumY);

	UPROPERTY(VisibleAnywhere, Category = "Infinite Map")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Infinite Map")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Grass01;

	UPROPERTY(VisibleAnywhere, Category = "Infinite Map")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Grass02;

	UPROPERTY(VisibleAnywhere, Category = "Infinite Map")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Grass03;

	UPROPERTY(VisibleAnywhere, Category = "Infinite Map")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ExtendedRoad;
};
