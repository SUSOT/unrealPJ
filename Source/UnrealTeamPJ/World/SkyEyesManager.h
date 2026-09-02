#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkyEyesManager.generated.h"

class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

USTRUCT()
struct FSkyEyeRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Component = nullptr;

	FVector Direction = FVector::ForwardVector;
	float WidthScale = 1.0f;
	float OpenHeightScale = 1.0f;
	float BlinkAmount = 0.0f;
	float CloseZoneElapsed = 0.0f;
};

/**
 * Keeps a lightweight hemisphere of uncanny eyes around the local player.
 * Eyes pre-close near the camera frustum, but a short reaction delay lets a
 * fast camera turn catch them open for a moment.
 */
UCLASS()
class UNREALTEAMPJ_API ASkyEyesManager : public AActor
{
	GENERATED_BODY()

public:
	ASkyEyesManager();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaSeconds) override;

private:
	void CreateEyes();
	void DestroyEyes();
	void InitializeFromCamera(const FVector& CameraForward, float HorizontalFOV);
	void ApplyEyeVisual(FSkyEyeRuntimeState& Eye);
	float GetTargetBlinkAmount(const FVector& CameraForward, float HorizontalFOV, const FVector& EyeDirection) const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Sky Eyes")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, Category = "Sky Eyes|Assets")
	TSoftObjectPtr<UStaticMesh> EyePlaneMesh;

	UPROPERTY(EditAnywhere, Category = "Sky Eyes|Assets")
	TSoftObjectPtr<UMaterialInterface> EyeMaterial;

	UPROPERTY(EditAnywhere, Category = "Sky Eyes|Layout", meta = (ClampMin = "8", ClampMax = "64"))
	int32 EyeCount = 32;

	UPROPERTY(EditAnywhere, Category = "Sky Eyes|Layout", meta = (ClampMin = "5000.0"))
	float SphereRadius = 22000.0f;

	UPROPERTY(EditAnywhere, Category = "Sky Eyes|Layout", meta = (ClampMin = "0.0", ClampMax = "85.0"))
	float MinimumElevation = 12.0f;

	UPROPERTY(EditAnywhere, Category = "Sky Eyes|Layout", meta = (ClampMin = "5.0", ClampMax = "89.0"))
	float MaximumElevation = 74.0f;

	UPROPERTY(EditAnywhere, Category = "Sky Eyes|Layout", meta = (ClampMin = "100.0"))
	float MinimumEyeWidth = 900.0f;

	UPROPERTY(EditAnywhere, Category = "Sky Eyes|Layout", meta = (ClampMin = "100.0"))
	float MaximumEyeWidth = 2400.0f;

	UPROPERTY(EditAnywhere, Category = "Sky Eyes|Layout", meta = (ClampMin = "0.1"))
	float EyeTextureAspectRatio = 1.5f;

	/** Starts closing this many degrees before an eye reaches the camera frustum. */
	UPROPERTY(EditAnywhere, Category = "Sky Eyes|Behavior", meta = (ClampMin = "0.0", ClampMax = "60.0"))
	float PartialCloseAnticipationAngle = 30.0f;

	/** Is fully closed this many degrees before an eye reaches the camera frustum. */
	UPROPERTY(EditAnywhere, Category = "Sky Eyes|Behavior", meta = (ClampMin = "0.0", ClampMax = "60.0"))
	float FullyClosedAnticipationAngle = 22.0f;

	UPROPERTY(EditAnywhere, Category = "Sky Eyes|Behavior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReactionDelay = 0.14f;

	UPROPERTY(EditAnywhere, Category = "Sky Eyes|Behavior", meta = (ClampMin = "0.01", ClampMax = "2.0"))
	float CloseDuration = 0.14f;

	UPROPERTY(EditAnywhere, Category = "Sky Eyes|Behavior", meta = (ClampMin = "0.01", ClampMax = "3.0"))
	float OpenDuration = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Sky Eyes|Layout")
	int32 RandomSeed = 19341;

private:
	UPROPERTY(Transient)
	TArray<FSkyEyeRuntimeState> RuntimeEyes;

	bool bCameraInitialized = false;
};
