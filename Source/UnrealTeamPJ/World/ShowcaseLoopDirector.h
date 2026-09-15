#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/ShowcaseEscapeProgress.h"
#include "ShowcaseLoopDirector.generated.h"

class AShowcaseEscapeDoor;
class AShowcaseRepeatExtension;
class UHierarchicalInstancedStaticMeshComponent;
class ULightComponent;
class UMaterialInstanceDynamic;
class UStaticMesh;

UENUM(BlueprintType)
enum class EShowcaseHorrorEvent : uint8
{
	None,
	ForwardBlackout,
	RedPulse,
	FlickerOut,
	ChairReveal,
	PropDisplacement,
	SequentialBlackout,
	SequentialRedPulse,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FShowcaseLoopEscaped);

/** Level-local single-player sequence; leaves the character and other maps untouched. */
UCLASS()
class UNREALTEAMPJ_API AShowcaseLoopDirector : public AActor
{
	GENERATED_BODY()

public:
	AShowcaseLoopDirector();
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Geometry") bool bStraightCorridor = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Geometry") float StraightRepeatSpan = 28800.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Geometry") FVector LoopCenter = FVector(0,3105.7749,0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Geometry") float LoopRadius = 3055.7749f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Geometry") float WalkwayRadiusOffset = -115.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Pacing", meta=(Units="cm", ClampMin="100")) float FirstChangeDistance = 4000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Pacing", meta=(Units="cm", ClampMin="100")) float SecondChangeDistance = 7000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Pacing", meta=(Units="cm", ClampMin="100")) float TurnBackUnlockDistance = 9000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Pacing", meta=(Units="cm", ClampMin="100")) float RequiredBacktrackDistance = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Pacing", meta=(Units="cm", ClampMin="450")) float DoorDistanceAhead = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Horror") bool bEnableHorrorEvents = true;
	/** Applied once to both the light components and visible bulb emissive. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Horror", meta=(ClampMin="0", ClampMax="1")) float BaseLightScale = .72f;
	/** Non-zero gives reproducible shuffled event order; zero chooses a new seed each play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Horror") int32 HorrorSeed = 91357;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Actors") TObjectPtr<AShowcaseEscapeDoor> EscapeDoor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Actors") TObjectPtr<AShowcaseRepeatExtension> RepeatExtension;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Actors") TObjectPtr<UStaticMesh> TableMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Actors") TArray<TObjectPtr<AActor>> Lamps;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Actors") TArray<TObjectPtr<AActor>> ChairProps;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Actors") TArray<TObjectPtr<AActor>> FrameProps;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Actors") TArray<TObjectPtr<AActor>> EmergencyExitSigns;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") int32 CurrentStage = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") float ForwardProgress = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") float BacktrackProgress = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") bool bEscapeComplete = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") EShowcaseHorrorEvent CurrentHorrorEvent = EShowcaseHorrorEvent::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") int32 HorrorEventSerial = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") int32 CompletedHorrorEvents = 0;
	/** Bit N records that event enum value N has occurred during this play. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") int32 HorrorEventHistoryMask = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") int32 AlteredPropCount = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") int32 AlteredChairCount = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") int32 AlteredTableCount = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") int32 AlteredFrameCount = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") int32 ReversedExitSignCount = 0;
	UPROPERTY(BlueprintAssignable, Category="Loop") FShowcaseLoopEscaped OnEscapeCompleted;

	static FVector2D GetHorrorEventIntervalRange(int32 Stage);
	static int32 GetHorrorPropChangeCount(int32 Stage);
	static float GetHorrorEventDuration(EShowcaseHorrorEvent Event, int32 Stage);
	static bool ShouldReverseExitSigns(int32 Stage, bool bDoorRevealed);
	static TArray<int32> GetFarToNearLampOrder(const TArray<float>& ForwardDistances);
	static float GetSequentialLampDelay(int32 OrderIndex);

protected:
	virtual void BeginPlay() override;

private:
	struct FLightState
	{
		TWeakObjectPtr<ULightComponent> Light;
		float Intensity = 0.f;
		FLinearColor Color;
		int32 LampIndex = 0;
	};
	struct FBulbState
	{
		TWeakObjectPtr<UMaterialInstanceDynamic> Material;
		float Emissive = 0.f;
		int32 LampIndex = 0;
	};

	TArray<FLightState> LightStates;
	TArray<FBulbState> BulbStates;
	TArray<EShowcaseHorrorEvent> EventBag;
	/** Snapshot of lamp indices, farthest first, fixed for the current wave. */
	TArray<int32> SequentialLampOrder;
	TSet<TWeakObjectPtr<AActor>> AlteredActorProps;
	TSet<uint64> AlteredTableInstances;
	FShowcaseEscapeProgress Progress;
	FRandomStream HorrorRandom;
	TWeakObjectPtr<APawn> TrackedPawn;
	float PreviousAngle = 0.f;
	float PreviousStraightX = 0.f;
	float PlayerPathPosition = 0.f;
	float ViewCosine = 0.f;
	float NextHorrorEventAt = 0.f;
	float HorrorEventStartedAt = 0.f;
	float HorrorEventEndsAt = 0.f;
	float FlickerEndsAt = 0.f;
	FVector LinearDoorPosition;
	FVector ViewPosition;
	FVector ViewForward;
	bool bHasSample = false;
	bool bLinearDoorNoticed = false;
	bool bChairRevealPending = false;
	bool bExitSignsReversed = false;

	bool IsUnseen(const FVector& Point, float Margin = 100.f) const;
	bool IsLampAhead(int32 LampIndex) const;
	bool IsLampReachedBySequence(int32 LampIndex, float Elapsed) const;
	void UpdateHorrorEvents();
	void StartHorrorEvent();
	void FinishHorrorEvent();
	void ApplyHorrorLighting(float Now);
	void RefillEventBag();
	void RevealFallenChair();
	void ApplyRandomPropChanges();
	bool ChangeActorProp(const TArray<TObjectPtr<AActor>>& Props, bool bFrame, bool bAllowVisible);
	bool ChangeTableInstance();
	void UpdateExitSigns();
	void TryRevealDoor(float Angle);
	void UpdateStraightDoor();
	FVector PathPoint(float Distance, float Side, float Height) const;
	UFUNCTION() void HandleEscaped();
};
