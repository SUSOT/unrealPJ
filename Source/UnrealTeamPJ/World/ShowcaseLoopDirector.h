#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/ShowcaseEscapeProgress.h"
#include "World/ShowcasePresenceRhythm.h"
#include "World/ShowcaseReverseEncounterState.h"
#include "ShowcaseLoopDirector.generated.h"

class AShowcaseEscapeDoor;
class ULightComponent;
class ATextRenderActor;
class UAudioComponent;
class USoundBase;
class UMaterialInstanceDynamic;
class ALight;

USTRUCT(BlueprintType)
struct FShowcaseSpatialChange
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<AActor> Target;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FTransform ChangedTransform;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="3")) int32 Stage = 1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FShowcaseLoopEscaped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FShowcaseHorrorCue, FName, Cue, FVector, Location);

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Flicker") bool bEnableLightFlicker = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Flicker", meta=(ClampMin="0", ClampMax="1")) float FlickerStrength = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Flicker", meta=(Units="s", ClampMin="6", ClampMax="60")) float FlickerInterval = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Actors") TObjectPtr<AShowcaseEscapeDoor> EscapeDoor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Actors") TArray<FShowcaseSpatialChange> SpatialChanges;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Actors") TArray<TObjectPtr<AActor>> Lamps;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Actors") TArray<TObjectPtr<ATextRenderActor>> DirectionSigns;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Sound") TArray<TObjectPtr<USoundBase>> FootstepSounds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Sound") TObjectPtr<USoundBase> ChairDragSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Sound") TObjectPtr<USoundBase> RelaySound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Sound") TObjectPtr<USoundBase> RoomToneSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Sound") TObjectPtr<USoundBase> DistantKnockSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Sound", meta=(ClampMin="0", ClampMax="1")) float HorrorVolume = .8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Sound") bool bEnablePresenceAudio = true;
	/** Optional hook for directional captions/accessibility, emitted at actual cue playback. */
	UPROPERTY(BlueprintAssignable, Category="Loop|Sound") FShowcaseHorrorCue OnHorrorCue;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Sound") TObjectPtr<UAudioComponent> RoomTone;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Sound") TArray<TObjectPtr<UAudioComponent>> CuePlayers;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") int32 CurrentStage = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") float ForwardProgress = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") float BacktrackProgress = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") bool bEscapeComplete = false;
	UPROPERTY(BlueprintAssignable, Category="Loop") FShowcaseLoopEscaped OnEscapeCompleted;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Reverse Encounter") TObjectPtr<AActor> ReverseChair;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Reverse Encounter") FTransform ReverseChairTuckedPose;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Reverse Encounter") TObjectPtr<ALight> ReverseFocusLight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Reverse Encounter", meta=(MakeEditWidget)) FVector ReverseReturnSeatLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loop|Reverse Encounter") TObjectPtr<USoundBase> ReverseCutlerySound;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Loop|Runtime") int32 ReverseEncounterStage = 0;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	struct FLightState
	{
		TWeakObjectPtr<ULightComponent> Light;
		float Intensity = 0.f;
		float SmoothedIntensity = 0.f;
		FLinearColor Color;
		int32 Index = 0;
		int32 AppliedStage = 0;
	};
	struct FBulbState
	{
		TWeakObjectPtr<UMaterialInstanceDynamic> Material;
		float Emissive = 0.f;
		int32 LampIndex = 0;
	};
	TArray<FLightState> LightStates;
	TArray<FBulbState> BulbStates;
	TSet<int32> AppliedChanges;
	TSet<int32> ReversedSigns;
	TSet<int32> WitnessedChanges;
	TMap<int32,float> ObservedTime;
	TMap<int32,float> OriginalChangeX;
	FShowcaseEscapeProgress Progress;
	FShowcasePresenceRhythm Presence;
	FShowcaseReverseEncounterState ReverseEncounter;
	float ReverseQuietAmount = 0.f, ReverseFocusIntensity = 0.f;
	TWeakObjectPtr<APawn> TrackedPawn;
	float PreviousAngle = 0.f;
	float PreviousStraightX = 0.f;
	float PlayerPathPosition = 0.f;
	int32 DreadBeatStage = 0;
	FVector LinearDoorPosition;
	bool bLinearDoorNoticed = false;
	bool bHasSample = false;
	float StageStartedAt = 0.f;
	int32 NextCuePlayer = 0, StepVariation = 0;
	float NextSpatialChangeAt = 0.f, NextLightBeatAt = 0.f, LightBeatStartedAt = -100.f;
	float SilenceUntil = 0.f, ToneGain = 1.f;
	int32 LightBeatLead = 0, LightBeatSerial = 0;
	FVector PendingFootstepPosition;
	FVector ViewPosition;
	FVector ViewForward;
	float ViewCosine = 0.f;
	bool IsUnseen(const FVector& Point, float Margin = 100.f) const;
	void UpdateAtmosphere(float DeltaSeconds);
	float GetFlickerMultiplier(int32 LampIndex) const;
	void UpdatePresence(float DeltaSeconds, float Distance, bool bLookingBack, float Angle);
	void PlayCue(USoundBase* Sound, FVector Position, float Gain, float Pitch, FName Cue);
	void StartLightBeat(float Angle);
	void UpdateReverseEncounter(float DeltaSeconds, float Angle);
	float GetReverseLampMultiplier(int32 LampIndex) const;
	void SilencePresence();
	void TryRevealDoor(float Angle);
	FVector PathPoint(float Distance, float Side, float Height) const;
	void UpdateStraightDread();
	UFUNCTION() void HandleEscaped();
};
