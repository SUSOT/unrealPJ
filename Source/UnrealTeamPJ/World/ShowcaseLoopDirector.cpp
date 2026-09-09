#include "World/ShowcaseLoopDirector.h"
#include "World/ShowcaseEscapeDoor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/LightComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "Engine/Light.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Materials/MaterialInstanceDynamic.h"

AShowcaseLoopDirector::AShowcaseLoopDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = .05f;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	RoomTone = CreateDefaultSubobject<UAudioComponent>(TEXT("RoomTone"));
	RoomTone->SetupAttachment(RootComponent);
	RoomTone->bAutoActivate = false;
	RoomTone->bAllowSpatialization = false;
	for (int32 Index=0; Index<8; ++Index)
	{
		UAudioComponent* Cue = CreateDefaultSubobject<UAudioComponent>(*FString::Printf(TEXT("HorrorCue_%d"),Index));
		Cue->SetupAttachment(RootComponent);
		Cue->bAutoActivate = false;
		Cue->bOverrideAttenuation = true;
		Cue->AttenuationOverrides.bAttenuate = true;
		Cue->AttenuationOverrides.bSpatialize = true;
		Cue->AttenuationOverrides.AttenuationShapeExtents = FVector(180.f,0,0);
		Cue->AttenuationOverrides.FalloffDistance = 1600.f;
		CuePlayers.Add(Cue);
	}
}

void AShowcaseLoopDirector::BeginPlay()
{
	Super::BeginPlay();
	Progress = FShowcaseEscapeProgress();
	Presence = FShowcasePresenceRhythm();
	ReverseEncounter = FShowcaseReverseEncounterState();
	ReverseEncounterStage = 0;
	ReverseQuietAmount = 0.f;
	DreadBeatStage=0;
	bLinearDoorNoticed=false;
	OriginalChangeX.Reset();
	for (int32 I=0; I<SpatialChanges.Num(); ++I)
		if (IsValid(SpatialChanges[I].Target)) OriginalChangeX.Add(I,SpatialChanges[I].Target->GetActorLocation().X);
	if (IsValid(ReverseFocusLight))
	{
		ReverseFocusIntensity = ReverseFocusLight->GetLightComponent()->Intensity;
		ReverseFocusLight->GetLightComponent()->SetIntensity(0.f);
	}
	CurrentStage = 0;
	ForwardProgress = BacktrackProgress = 0.f;
	bEscapeComplete = bHasSample = false;
	StageStartedAt = GetWorld()->GetTimeSeconds();
	AppliedChanges.Reset();
	ReversedSigns.Reset();
	WitnessedChanges.Reset();
	ObservedTime.Reset();
	NextSpatialChangeAt = StageStartedAt+3.f;
	NextLightBeatAt = StageStartedAt+100000.f;
	LightBeatStartedAt = -100.f;
	LightBeatSerial = NextCuePlayer = StepVariation = 0;
	SilenceUntil = 0.f;
	ToneGain = 1.f;
	if (bEnablePresenceAudio && RoomToneSound)
	{
		RoomTone->SetSound(RoomToneSound);
		RoomTone->SetVolumeMultiplier(.38f * FMath::Clamp(HorrorVolume,0.f,1.f));
		RoomTone->Play();
	}
	LightStates.Reset();
	BulbStates.Reset();
	for (int32 Index = 0; Index < Lamps.Num(); ++Index)
	{
		if (!IsValid(Lamps[Index])) continue;
		TInlineComponentArray<ULightComponent*> Components(Lamps[Index]);
		for (ULightComponent* Light : Components)
		{
			FLightState State;
			State.Light = Light;
			State.Intensity = Light->Intensity;
			State.SmoothedIntensity = State.Intensity;
			State.Color = Light->GetLightColor();
			State.Index = Index;
			LightStates.Add(State);
		}
		TInlineComponentArray<UMeshComponent*> Meshes(Lamps[Index]);
		for (UMeshComponent* Mesh : Meshes)
		{
			for (int32 Slot=0; Slot<Mesh->GetNumMaterials(); ++Slot)
			{
				UMaterialInterface* Material = Mesh->GetMaterial(Slot);
				float Emissive = 0.f;
				// This spelling is the parameter in the existing RestaurantScene asset.
				if (Material && Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("EmissiveStrenght")),Emissive) && Emissive > 0.f)
				{
					FBulbState State;
					State.Material = Mesh->CreateDynamicMaterialInstance(Slot);
					State.Emissive = Emissive;
					State.LampIndex = Index;
					BulbStates.Add(State);
				}
			}
		}
	}
	if (EscapeDoor && EscapeDoor->Destination)
	{
		EscapeDoor->OnEscaped.AddDynamic(this, &AShowcaseLoopDirector::HandleEscaped);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ShowcaseEscape: assign the door and a safe destination."));
		SilencePresence();
		SetActorTickEnabled(false);
	}
}

bool AShowcaseLoopDirector::IsUnseen(const FVector& Point, float Margin) const
{
	const FVector Offset = Point - ViewPosition;
	if (Offset.SizeSquared() < FMath::Square(Margin + 200.f)) return false;
	if (FVector::DotProduct(Offset.GetSafeNormal(), ViewForward) < ViewCosine) return true;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ShowcaseHiddenChange), false, this);
	Query.AddIgnoredActor(TrackedPawn.Get());
	Query.AddIgnoredActor(EscapeDoor);
	FHitResult Hit;
	return GetWorld()->LineTraceSingleByChannel(Hit, ViewPosition, Point, ECC_Visibility, Query)
		&& Hit.Distance + Margin < Offset.Size();
}

void AShowcaseLoopDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!Pawn || !PC || bEscapeComplete) return;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewPosition, ViewRotation);
	ViewForward = ViewRotation.Vector();
	const float FOV = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetFOVAngle() : 90.f;
	// Conservative 3D cone includes the viewport corners and a pop-in guard band.
	ViewCosine = FMath::Cos(FMath::DegreesToRadians(FMath::Min(100.f, FOV * .5f + 25.f)));
	const FVector Relative = Pawn->GetActorLocation() - LoopCenter;
	const float Angle = bStraightCorridor ? 0.f : FMath::Atan2(Relative.X, -Relative.Y);
	const float Radius = Relative.Size2D();
	const bool bInCorridor = (bStraightCorridor ? FMath::Abs(Pawn->GetActorLocation().Y-50.f)<440.f : FMath::Abs(Radius-LoopRadius)<500.f)
		&& Relative.Z>50.f && Relative.Z<360.f;
	PlayerPathPosition=bStraightCorridor ? Pawn->GetActorLocation().X : Angle*LoopRadius;
	if (TrackedPawn != Pawn || !bHasSample || !bInCorridor)
	{
		Presence = FShowcasePresenceRhythm();
		TrackedPawn = Pawn;
		PreviousAngle = Angle;
		PreviousStraightX=Pawn->GetActorLocation().X;
		bHasSample = bInCorridor;
		return;
	}
	const float Delta = bStraightCorridor ? Pawn->GetActorLocation().X-PreviousStraightX : FMath::FindDeltaAngleRadians(PreviousAngle, Angle)*LoopRadius;
	PreviousAngle = Angle;
	PreviousStraightX=Pawn->GetActorLocation().X;
	// Exclude teleports, falling out of the map, and editor relocation from progress.
	if (FMath::Abs(Delta) > FMath::Max(150.f, DeltaSeconds * 1400.f)) return;
	const FVector ForwardTangent(FMath::Cos(Angle), FMath::Sin(Angle), 0);
	const bool bLookingBack = FVector::DotProduct(ViewForward.GetSafeNormal2D(), ForwardTangent) < -.45f;
	Progress.Advance(Delta, bLookingBack, TurnBackUnlockDistance, RequiredBacktrackDistance);
	ForwardProgress = Progress.FurthestForward;
	BacktrackProgress = Progress.ReverseDistance;
	const int32 NewStage = ForwardProgress >= TurnBackUnlockDistance ? 3 : ForwardProgress >= SecondChangeDistance ? 2 : ForwardProgress >= FirstChangeDistance ? 1 : 0;
	if (NewStage != CurrentStage)
	{
		CurrentStage = NewStage;
		StageStartedAt = GetWorld()->GetTimeSeconds();
		NextLightBeatAt = StageStartedAt+.65f;
		UE_LOG(LogTemp, Display, TEXT("ShowcaseEscape: stage %d at %.1f m"), CurrentStage, ForwardProgress / 100.f);
	}
	UpdateReverseEncounter(DeltaSeconds, Angle);
	if (bStraightCorridor) UpdateStraightDread();
	UpdatePresence(DeltaSeconds, FMath::Abs(Delta), bLookingBack, Angle);
	UpdateAtmosphere(DeltaSeconds);
	if (!bStraightCorridor && Progress.bDoorRequested && EscapeDoor && !EscapeDoor->bRevealed) TryRevealDoor(Angle);
}

FVector AShowcaseLoopDirector::PathPoint(float Distance, float Side, float Height) const
{
	if (bStraightCorridor) return FVector(PlayerPathPosition+Distance,165.f+Side,LoopCenter.Z+Height);
	const float Angle=(PlayerPathPosition+Distance)/LoopRadius;
	const float Radius=LoopRadius+WalkwayRadiusOffset+Side;
	return LoopCenter+FVector(Radius*FMath::Sin(Angle),-Radius*FMath::Cos(Angle),Height);
}

void AShowcaseLoopDirector::UpdateStraightDread()
{
	if (CurrentStage>DreadBeatStage)
	{
		DreadBeatStage=CurrentStage;
		const FVector Source=PathPoint(CurrentStage==1 ? -420.f : 950.f,CurrentStage==1 ? -535.f : 300.f,160.f);
		PlayCue(DistantKnockSound,Source,CurrentStage==1 ? .7f : .9f,CurrentStage==1 ? .92f : .76f,TEXT("DistantKnock"));
		SilenceUntil=GetWorld()->GetTimeSeconds()+2.4f;
	}
	if (CurrentStage<3 || !EscapeDoor) return;
	if (EscapeDoor->bRevealed && !IsUnseen(EscapeDoor->GetActorLocation()+FVector(0,0,160),100.f)) bLinearDoorNoticed=true;
	if (bLinearDoorNoticed) return;
	// Place while looking forward, so the complete doorway is already present on
	// the first turn. Keep it nearby if the player chooses to walk farther first.
	FVector Position;
	bool bClear=false;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ShowcaseDoorClearance),false,this);
	Query.AddIgnoredActor(TrackedPawn.Get());
	Query.AddIgnoredActor(EscapeDoor);
	for (float Adjustment : {0.f,50.f,-50.f,100.f,-100.f,150.f,200.f,250.f,300.f,350.f})
	{
		const float Distance=FMath::Clamp(DoorDistanceAhead+Adjustment,450.f,900.f);
		Position=PathPoint(-Distance,0,20);
		// Keep the frame clear of tabletops/booths. Bottom is above the floor.
		if (!GetWorld()->OverlapBlockingTestByChannel(Position+FVector(0,0,160),FQuat::Identity,ECC_Visibility,
			FCollisionShape::MakeBox(FVector(28,125,155)),Query)) { bClear=true; break; }
	}
	if (!bClear) return;
	for (float Y : {-125.f,0.f,125.f})
		for (float Z : {40.f,320.f})
			if (!IsUnseen(Position+FVector(0,Y,Z),125.f)) return;
	if (EscapeDoor->bRevealed)
	{
		if (!IsUnseen(EscapeDoor->GetActorLocation()+FVector(0,0,160),160.f)) return;
		if (FVector::Dist2D(LinearDoorPosition,Position)<120.f) return;
		EscapeDoor->SetActorLocation(Position,false,nullptr,ETeleportType::TeleportPhysics);
	}
	else EscapeDoor->RevealAt(FTransform(FRotator::ZeroRotator,Position));
	LinearDoorPosition=Position;
}

void AShowcaseLoopDirector::UpdateAtmosphere(float DeltaSeconds)
{
	const float Now = GetWorld()->GetTimeSeconds();
	for (int32 Index = 0; Index < SpatialChanges.Num(); ++Index)
	{
		const FShowcaseSpatialChange& Change = SpatialChanges[Index];
		if (AppliedChanges.Contains(Index) || !IsValid(Change.Target)) continue;
		FVector Center, Extent;
		Change.Target->GetActorBounds(false, Center, Extent);
		// Only change a nearby object the player actually had a chance to see.
		if (FVector::DistSquared(Center, ViewPosition) > FMath::Square(1100.f)) continue;
		if (FVector::DotProduct((Center-ViewPosition).GetSafeNormal(),ViewForward) > ViewCosine && !IsUnseen(Center, Extent.Size()))
		{
			float& SeenFor = ObservedTime.FindOrAdd(Index);
			SeenFor += DeltaSeconds;
			if (SeenFor > .35f) WitnessedChanges.Add(Index);
		}
		if (CurrentStage < Change.Stage || !WitnessedChanges.Contains(Index) || Now < NextSpatialChangeAt) continue;
		FTransform Changed=Change.ChangedTransform;
		if (bStraightCorridor && StraightRepeatSpan>0.f && OriginalChangeX.Contains(Index))
			Changed.AddToTranslation(FVector(FMath::RoundToDouble((Change.Target->GetActorLocation().X-OriginalChangeX[Index])/StraightRepeatSpan)*StraightRepeatSpan,0,0));
		if (!IsUnseen(Center, Extent.Size()) || !IsUnseen(Changed.GetLocation(), Extent.Size())) continue;
		Change.Target->SetActorTransform(Changed, false, nullptr, ETeleportType::TeleportPhysics);
		AppliedChanges.Add(Index);
		PlayCue(ChairDragSound, Center, .7f, Change.Stage == 1 ? 1.f : .87f, TEXT("FurnitureMoved"));
		NextSpatialChangeAt = Now+9.f;
		break;
	}
	if (CurrentStage == 3)
	{
		for (int32 Index = 0; Index < DirectionSigns.Num(); ++Index)
		{
			ATextRenderActor* Sign = DirectionSigns[Index];
			if (!IsValid(Sign) || ReversedSigns.Contains(Index) || !IsUnseen(Sign->GetActorLocation())) continue;
			Sign->GetTextRender()->SetText(FText::FromString(TEXT("<  EXIT")));
			ReversedSigns.Add(Index);
		}
	}
	for (FLightState& State : LightStates)
	{
		ULightComponent* Light = State.Light.Get();
		if (!Light) continue;
		if (State.AppliedStage < CurrentStage && IsUnseen(Light->GetComponentLocation(), 150.f)) State.AppliedStage = CurrentStage;
		const int32 Stage = State.AppliedStage;
		float Multiplier = Stage == 0 ? 1.f : Stage == 1 ? .92f : Stage == 2 ? .82f : .70f;
		if (bStraightCorridor)
		{
			const float Ahead=Light->GetComponentLocation().X-PlayerPathPosition;
			// Adjacent pools remain readable; distant lamps disappear into darkness.
			Multiplier*=1.f-FMath::SmoothStep(1500.f,6500.f,FMath::Abs(Ahead))*.92f;
			if (CurrentStage>=2 && Ahead>900.f) Multiplier*=.32f;
		}
		// A slow, shallow drift, not strobing or a jumpscare blackout.
		if (Stage == 3) Multiplier *= 1.f + .035f * FMath::Sin(GetWorld()->GetTimeSeconds() * .55f + State.Index);
		// Keep base illumination separate: filtering the flickered intensity would
		// erase short dips and leave lamps dim after a pulse has already finished.
		State.SmoothedIntensity = FMath::FInterpTo(State.SmoothedIntensity, State.Intensity * Multiplier, DeltaSeconds, .7f);
		Light->SetIntensity(State.SmoothedIntensity * GetFlickerMultiplier(State.Index) * GetReverseLampMultiplier(State.Index));
		const FLinearColor Target = FMath::Lerp(State.Color, FLinearColor(.62f,.76f,1.f), Stage * .10f);
		Light->SetLightColor(FMath::Lerp(Light->GetLightColor(), Target, FMath::Min(1.f, DeltaSeconds * .5f)));
	}
	for (FBulbState& Bulb : BulbStates)
		if (UMaterialInstanceDynamic* Material = Bulb.Material.Get())
			Material->SetScalarParameterValue(TEXT("EmissiveStrenght"),Bulb.Emissive*GetFlickerMultiplier(Bulb.LampIndex)*GetReverseLampMultiplier(Bulb.LampIndex));
}

void AShowcaseLoopDirector::UpdateReverseEncounter(float DeltaSeconds, float Angle)
{
	if (!IsValid(ReverseChair) || !IsValid(ReverseFocusLight)) return;
	FVector Center, Extent;
	ReverseChair->GetActorBounds(false,Center,Extent);
	const bool bEligible = CurrentStage == 0 && !bEscapeComplete
		&& (!bStraightCorridor || FMath::Abs(ReverseChair->GetActorLocation().X-ReverseChairTuckedPose.GetLocation().X)<200.f);
	const float Distance = FVector::Dist(ViewPosition,Center);
	// A local vignette, not a permanent global blackout or a movement barrier.
	const float TargetQuiet = bEligible ? FMath::Clamp((-Progress.Position-200.f)/450.f,0.f,1.f)
		* (1.f-FMath::SmoothStep(1200.f,2200.f,Distance)) : 0.f;
	ReverseQuietAmount = FMath::FInterpTo(ReverseQuietAmount,TargetQuiet,DeltaSeconds,1.1f);
	ReverseFocusLight->GetLightComponent()->SetIntensity(ReverseFocusIntensity*ReverseQuietAmount);
	FShowcaseReverseEncounterState::FInput Input;
	Input.DeltaSeconds=DeltaSeconds; Input.SignedTravel=Progress.Position; Input.ChairDistance=Distance;
	Input.Eligible=bEligible && Distance<2000.f;
	// The tabletop can hide the seat while its backrest remains clearly visible.
	// Test both, rather than requiring a ray to the occluded bounds centre.
	const FVector Backrest=Center+FVector(0,0,Extent.Z*.85f);
	const FVector TuckedBackrest=ReverseChairTuckedPose.TransformPosition(ReverseChair->GetActorTransform().InverseTransformPosition(Backrest));
	const auto IsVisible=[this](const FVector& Point)
	{
		return FVector::DotProduct((Point-ViewPosition).GetSafeNormal(),ViewForward)>ViewCosine && !IsUnseen(Point,15.f);
	};
	Input.ChairVisible=IsVisible(Center) || IsVisible(Backrest);
	Input.ChairHidden=IsUnseen(Center,Extent.Size()) && IsUnseen(Backrest,Extent.Size())
		&& IsUnseen(ReverseChairTuckedPose.GetLocation(),Extent.Size()) && IsUnseen(TuckedBackrest,Extent.Size());
	Input.FacingReturn=FVector::DotProduct(ViewForward.GetSafeNormal2D(),FVector(FMath::Cos(Angle),FMath::Sin(Angle),0))>.45f;
	Input.ReturnSeatHidden=IsUnseen(ReverseReturnSeatLocation,20.f) && FVector::DistSquared(ViewPosition,ReverseReturnSeatLocation)<FMath::Square(1400.f);
	const auto Events=ReverseEncounter.Advance(Input);
	ReverseEncounterStage=static_cast<int32>(ReverseEncounter.Phase);
	if (Events.ScrapeBehind)
	{
		const FVector Source=PathPoint(360.f,0,45.f);
		PlayCue(ChairDragSound,Source,.8f,.92f,TEXT("ReverseScrapeBehind"));
	}
	if (Events.TuckChair)
		ReverseChair->SetActorTransform(ReverseChairTuckedPose,false,nullptr,ETeleportType::TeleportPhysics);
	if (Events.Cutlery)
		PlayCue(ReverseCutlerySound,ReverseReturnSeatLocation,.72f,1.f,TEXT("ReverseCutlery"));
}

float AShowcaseLoopDirector::GetReverseLampMultiplier(int32 LampIndex) const
{
	if (ReverseQuietAmount<=0.f || !IsValid(ReverseChair) || !Lamps.IsValidIndex(LampIndex) || !IsValid(Lamps[LampIndex])) return 1.f;
	const float Distance=FVector::Dist2D(Lamps[LampIndex]->GetActorLocation(),ReverseChair->GetActorLocation());
	const float LocalWeight=1.f-FMath::SmoothStep(1800.f,2600.f,Distance);
	return FMath::Lerp(1.f,.10f,ReverseQuietAmount*LocalWeight);
}

float AShowcaseLoopDirector::GetFlickerMultiplier(int32 LampIndex) const
{
	if (!bEnableLightFlicker || CurrentStage == 0 || Lamps.IsEmpty()) return 1.f;
	int32 RelativeIndex = (LampIndex - LightBeatLead + Lamps.Num()) % Lamps.Num();
	if (bStraightCorridor && Lamps.IsValidIndex(LightBeatLead) && IsValid(Lamps[LampIndex]) && IsValid(Lamps[LightBeatLead]))
	{
		const float Delta=Lamps[LampIndex]->GetActorLocation().X-Lamps[LightBeatLead]->GetActorLocation().X;
		if (Delta<-.1f) return 1.f;
		RelativeIndex=FMath::RoundToInt(Delta/1200.f);
	}
	// Fail progressively farther ahead; the route behind stays readable.
	if (RelativeIndex >= CurrentStage) return 1.f;
	const float Time = GetWorld()->GetTimeSeconds() - LightBeatStartedAt - RelativeIndex * .38f;
	// A failing electrical contact: two abrupt interruptions, then an outage.
	// Keep each short interruption longer than two nominal director ticks so
	// the flashes remain visible. Never interpolate this switch waveform.
	const float Jitter = (LampIndex % 3) * .025f;
	const float SecondBlink = .43f + Jitter;
	const float OutageStart = 1.10f + Jitter;
	const float OutageDuration = CurrentStage == 1 ? .65f : CurrentStage == 2 ? 1.6f : 3.f;
	const bool bOff = (Time >= 0.f && Time < .12f)
		|| (Time >= SecondBlink && Time < SecondBlink + .15f)
		|| (Time >= OutageStart && Time < OutageStart + OutageDuration);
	return bOff ? 1.f - FMath::Clamp(FlickerStrength, 0.f, 1.f) : 1.f;
}

void AShowcaseLoopDirector::PlayCue(USoundBase* Sound, FVector Position, float Gain, float Pitch, FName Cue)
{
	if (!bEnablePresenceAudio || !Sound || HorrorVolume <= 0.f || CuePlayers.IsEmpty()) return;
	UAudioComponent* Player = CuePlayers[NextCuePlayer++ % CuePlayers.Num()];
	Player->Stop();
	Player->SetWorldLocation(Position);
	Player->SetSound(Sound);
	Player->SetVolumeMultiplier(FMath::Clamp(Gain,0.f,1.f)*FMath::Clamp(HorrorVolume,0.f,1.f));
	Player->SetPitchMultiplier(Pitch);
	Player->Play();
	OnHorrorCue.Broadcast(Cue, Position);
}

void AShowcaseLoopDirector::StartLightBeat(float Angle)
{
	const FVector Point=PathPoint(700.f,0,250.f);
	float Nearest = TNumericLimits<float>::Max();
	for (int32 Index=0; Index<Lamps.Num(); ++Index)
	{
		if (!IsValid(Lamps[Index])) continue;
		float Distance = FVector::DistSquared2D(Point,Lamps[Index]->GetActorLocation());
		if (Distance < Nearest) { Nearest = Distance; LightBeatLead = Index; }
	}
	LightBeatStartedAt = GetWorld()->GetTimeSeconds();
	if (CurrentStage >= 2) SilenceUntil = LightBeatStartedAt+(CurrentStage == 3 ? 4.8f : 2.6f);
	if (Lamps.IsValidIndex(LightBeatLead) && IsValid(Lamps[LightBeatLead]))
		PlayCue(RelaySound,Lamps[LightBeatLead]->GetActorLocation(),.55f,.95f,TEXT("LightFailure"));
	const float Offsets[] = {1.6f, 4.1f, .4f, 2.8f};
	NextLightBeatAt = LightBeatStartedAt+FMath::Max(8.f,FlickerInterval)+Offsets[LightBeatSerial++ % 4];
}

void AShowcaseLoopDirector::UpdatePresence(float DeltaSeconds, float Distance, bool bLookingBack, float Angle)
{
	const float Now = GetWorld()->GetTimeSeconds();
	const bool bReturningToExit = EscapeDoor && EscapeDoor->bRevealed && (!bStraightCorridor || bLinearDoorNoticed);
	if (CurrentStage > 0 && !bReturningToExit && Now >= NextLightBeatAt) StartLightBeat(Angle);
	const auto Events = Presence.Advance(DeltaSeconds,Distance,bReturningToExit ? 0 : CurrentStage,bLookingBack);
	APawn* Pawn = TrackedPawn.Get();
	if (!Pawn) return;
	if (Events.OwnStep && !FootstepSounds.IsEmpty())
	{
		const FVector Feet(Pawn->GetActorLocation().X,Pawn->GetActorLocation().Y,LoopCenter.Z+25.f);
		USoundBase* Step = FootstepSounds[StepVariation++ % FootstepSounds.Num()];
		PlayCue(Step,Feet,.28f,1.f+(StepVariation%3-1)*.025f,TEXT("OwnFootstep"));
		// The delayed source stays where it was scheduled, not attached to the camera.
		PendingFootstepPosition=PathPoint(-(CurrentStage==1 ? 430.f : 290.f),StepVariation%2 ? 55.f : -55.f,35.f);
	}
	if ((Events.FollowerStep || Events.AfterStop) && !FootstepSounds.IsEmpty())
	{
		if (IsUnseen(PendingFootstepPosition,20.f))
			PlayCue(FootstepSounds[(StepVariation+2)%FootstepSounds.Num()],PendingFootstepPosition,Events.AfterStop ? .78f : .52f,.82f,Events.AfterStop ? TEXT("StepAfterStop") : TEXT("FollowingFootstep"));
	}
	const float TargetGain = bReturningToExit ? .32f : Now < SilenceUntil ? .045f : 1.f;
	ToneGain = FMath::FInterpTo(ToneGain,TargetGain,DeltaSeconds,Now < SilenceUntil ? 5.f : .7f);
	RoomTone->SetVolumeMultiplier(bEnablePresenceAudio ? .38f*FMath::Clamp(HorrorVolume,0.f,1.f)*ToneGain*FMath::Lerp(1.f,.06f,ReverseQuietAmount) : 0.f);
}

void AShowcaseLoopDirector::SilencePresence()
{
	if (RoomTone) RoomTone->Stop();
	for (UAudioComponent* Cue : CuePlayers) if (Cue) Cue->Stop();
}

void AShowcaseLoopDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SilencePresence();
	Super::EndPlay(EndPlayReason);
}

void AShowcaseLoopDirector::TryRevealDoor(float Angle)
{
	// Put it around the bend on the route just walked, never in the current view.
	for (int32 Attempt = 0; Attempt < 7; ++Attempt)
	{
		const float DoorAngle = Angle - (DoorDistanceAhead + Attempt * 300.f) / LoopRadius;
		const float Radius = LoopRadius + WalkwayRadiusOffset;
		const FVector Position = LoopCenter + FVector(Radius * FMath::Sin(DoorAngle), -Radius * FMath::Cos(DoorAngle), 20.f);
		const FVector Side(-FMath::Sin(DoorAngle), FMath::Cos(DoorAngle), 0);
		bool bCandidateUnseen = true;
		for (float Offset : {-120.f, 0.f, 120.f})
		{
			if (!IsUnseen(Position + Side * Offset + FVector(0,0,170), 160.f)) bCandidateUnseen = false;
		}
		if (bCandidateUnseen)
		{
			EscapeDoor->RevealAt(FTransform(FRotator(0,FMath::RadiansToDegrees(DoorAngle),0), Position));
			return;
		}
	}
}

void AShowcaseLoopDirector::HandleEscaped()
{
	bEscapeComplete = true;
	SilencePresence();
	OnEscapeCompleted.Broadcast();
	SetActorTickEnabled(false);
}
