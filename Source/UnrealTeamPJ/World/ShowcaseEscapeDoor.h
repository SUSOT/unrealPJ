#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShowcaseEscapeDoor.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UTextRenderComponent;
class UPointLightComponent;
class UMaterialInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FShowcaseDoorEscaped);

/** Impossible doorway. Its local +X is the approach side; walk towards -X. */
UCLASS()
class UNREALTEAMPJ_API AShowcaseEscapeDoor : public AActor
{
	GENERATED_BODY()
public:
	AShowcaseEscapeDoor();
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escape Door")
	TObjectPtr<AActor> Destination;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escape Door")
	TObjectPtr<UMaterialInterface> FrameMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escape Door")
	TObjectPtr<UMaterialInterface> LeafMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Escape Door")
	TObjectPtr<UMaterialInterface> ThresholdMaterial;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Escape Door")
	bool bRevealed = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Escape Door")
	bool bEscaped = false;
	UPROPERTY(BlueprintAssignable, Category="Escape Door")
	FShowcaseDoorEscaped OnEscaped;

	UFUNCTION(BlueprintCallable, Category="Escape Door")
	void RevealAt(const FTransform& Transform);
	UFUNCTION(BlueprintPure, Category="Escape Door")
	float GetOpenAmount() const { return OpenAmount; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Hinge;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Leaf;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Threshold;
	UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> Frame;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> ExitSign;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> SpillLight;
	TWeakObjectPtr<APawn> EscapingPawn;
	FTimerHandle TravelTimer;
	float OpenAmount = 0.f;
	float PreviousLocalX = 0.f;
	bool bHasPreviousSample = false;
	bool bTravelPending = false;
	void CompleteTravel();
};
