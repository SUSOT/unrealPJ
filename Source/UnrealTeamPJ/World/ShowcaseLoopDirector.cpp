#include "World/ShowcaseLoopDirector.h"
#include "World/ShowcaseEscapeDoor.h"
#include "World/ShowcaseRepeatExtension.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

AShowcaseLoopDirector::AShowcaseLoopDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = .05f;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AShowcaseLoopDirector::BeginPlay()
{
	Super::BeginPlay();
	Progress = FShowcaseEscapeProgress();
	CurrentStage = 0;
	ForwardProgress = BacktrackProgress = 0.f;
	bEscapeComplete = bHasSample = false;
	bLinearDoorNoticed = bChairRevealPending = bExitSignsReversed = false;
	CurrentHorrorEvent = EShowcaseHorrorEvent::None;
	HorrorEventSerial = CompletedHorrorEvents = HorrorEventHistoryMask = 0;
	AlteredPropCount = AlteredChairCount = AlteredTableCount = AlteredFrameCount = ReversedExitSignCount = 0;
	NextHorrorEventAt = GetWorld()->GetTimeSeconds() + 100000.f;
	EventBag.Reset();
	AlteredActorProps.Reset();
	AlteredTableInstances.Reset();
	HorrorRandom.Initialize(HorrorSeed != 0 ? HorrorSeed : static_cast<int32>(FPlatformTime::Cycles()));

	LightStates.Reset();
	BulbStates.Reset();
	const float SafeLightScale = FMath::Clamp(BaseLightScale, 0.f, 1.f);
	for (int32 LampIndex = 0; LampIndex < Lamps.Num(); ++LampIndex)
	{
		AActor* Lamp = Lamps[LampIndex];
		if (!IsValid(Lamp)) continue;
		TInlineComponentArray<ULightComponent*> Lights(Lamp);
		for (ULightComponent* Light : Lights)
		{
			FLightState State;
			State.Light = Light;
			State.Intensity = Light->Intensity * SafeLightScale;
			State.Color = Light->GetLightColor();
			State.LampIndex = LampIndex;
			Light->SetIntensity(State.Intensity);
			LightStates.Add(State);
		}

		TInlineComponentArray<UMeshComponent*> Meshes(Lamp);
		for (UMeshComponent* Mesh : Meshes)
		{
			for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
			{
				UMaterialInterface* Material = Mesh->GetMaterial(Slot);
				float Emissive = 0.f;
				// This misspelling is the authored parameter name in RestaurantScene.
				if (Material && Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("EmissiveStrenght")), Emissive) && Emissive > 0.f)
				{
					FBulbState State;
					State.Material = Mesh->CreateDynamicMaterialInstance(Slot);
					State.Emissive = Emissive * SafeLightScale;
					State.LampIndex = LampIndex;
					if (State.Material.IsValid()) State.Material->SetScalarParameterValue(TEXT("EmissiveStrenght"), State.Emissive);
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
	ViewCosine = FMath::Cos(FMath::DegreesToRadians(FMath::Min(100.f, FOV * .5f + 25.f)));
	const FVector Relative = Pawn->GetActorLocation() - LoopCenter;
	const float Angle = bStraightCorridor ? 0.f : FMath::Atan2(Relative.X, -Relative.Y);
	const float Radius = Relative.Size2D();
	const bool bInCorridor = (bStraightCorridor ? FMath::Abs(Pawn->GetActorLocation().Y - 50.f) < 440.f : FMath::Abs(Radius - LoopRadius) < 500.f)
		&& Relative.Z > 50.f && Relative.Z < 360.f;
	PlayerPathPosition = bStraightCorridor ? Pawn->GetActorLocation().X : Angle * LoopRadius;
	if (TrackedPawn != Pawn || !bHasSample || !bInCorridor)
	{
		TrackedPawn = Pawn;
		PreviousAngle = Angle;
		PreviousStraightX = Pawn->GetActorLocation().X;
		bHasSample = bInCorridor;
		return;
	}

	const float Delta = bStraightCorridor ? Pawn->GetActorLocation().X - PreviousStraightX : FMath::FindDeltaAngleRadians(PreviousAngle, Angle) * LoopRadius;
	PreviousAngle = Angle;
	PreviousStraightX = Pawn->GetActorLocation().X;
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
		// A new intensity tier receives its own freshly shuffled set instead of
		// consuming low-intensity events selected in the previous tier.
		EventBag.Reset();
		const FVector2D Interval = GetHorrorEventIntervalRange(CurrentStage);
		NextHorrorEventAt = GetWorld()->GetTimeSeconds() + Interval.X * .25f;
		UE_LOG(LogTemp, Display, TEXT("ShowcaseEscape: stage %d at %.1f m"), CurrentStage, ForwardProgress / 100.f);
	}

	if (bStraightCorridor) UpdateStraightDoor();
	else if (Progress.bDoorRequested && EscapeDoor && !EscapeDoor->bRevealed) TryRevealDoor(Angle);
	UpdateExitSigns();
	UpdateHorrorEvents();
}

FVector AShowcaseLoopDirector::PathPoint(float Distance, float Side, float Height) const
{
	if (bStraightCorridor) return FVector(PlayerPathPosition + Distance, 165.f + Side, LoopCenter.Z + Height);
	const float Angle = (PlayerPathPosition + Distance) / LoopRadius;
	const float Radius = LoopRadius + WalkwayRadiusOffset + Side;
	return LoopCenter + FVector(Radius * FMath::Sin(Angle), -Radius * FMath::Cos(Angle), Height);
}

FVector2D AShowcaseLoopDirector::GetHorrorEventIntervalRange(int32 Stage)
{
	switch (FMath::Clamp(Stage, 0, 3))
	{
	case 1: return FVector2D(10.f, 14.f);
	case 2: return FVector2D(6.5f, 9.5f);
	case 3: return FVector2D(3.5f, 6.5f);
	default: return FVector2D(100000.f, 100001.f);
	}
}

int32 AShowcaseLoopDirector::GetHorrorPropChangeCount(int32 Stage)
{
	static constexpr int32 Counts[] = {0, 1, 2, 4};
	return Counts[FMath::Clamp(Stage, 0, 3)];
}

float AShowcaseLoopDirector::GetHorrorEventDuration(EShowcaseHorrorEvent Event, int32 Stage)
{
	const int32 SafeStage = FMath::Clamp(Stage, 0, 3);
	if (SafeStage == 0 || Event == EShowcaseHorrorEvent::None) return 0.f;
	switch (Event)
	{
	case EShowcaseHorrorEvent::ForwardBlackout: return SafeStage == 1 ? 1.1f : SafeStage == 2 ? 1.9f : 3.f;
	case EShowcaseHorrorEvent::RedPulse: return SafeStage == 1 ? .55f : SafeStage == 2 ? .75f : 1.f;
	case EShowcaseHorrorEvent::FlickerOut: return SafeStage == 1 ? 1.8f : SafeStage == 2 ? 2.8f : 4.f;
	case EShowcaseHorrorEvent::ChairReveal: return SafeStage == 1 ? 1.4f : SafeStage == 2 ? 2.2f : 3.2f;
	case EShowcaseHorrorEvent::PropDisplacement: return .15f;
	default: return 0.f;
	}
}

bool AShowcaseLoopDirector::ShouldReverseExitSigns(int32 Stage, bool bDoorRevealed)
{
	return Stage >= 3 && bDoorRevealed;
}

void AShowcaseLoopDirector::RefillEventBag()
{
	EventBag = {
		EShowcaseHorrorEvent::ForwardBlackout,
		EShowcaseHorrorEvent::RedPulse,
		EShowcaseHorrorEvent::FlickerOut,
		EShowcaseHorrorEvent::ChairReveal,
		EShowcaseHorrorEvent::PropDisplacement,
	};
	for (int32 Index = EventBag.Num() - 1; Index > 0; --Index)
	{
		const int32 Other = HorrorRandom.RandRange(0, Index);
		EventBag.Swap(Index, Other);
	}
}

void AShowcaseLoopDirector::StartHorrorEvent()
{
	if (EventBag.IsEmpty()) RefillEventBag();
	CurrentHorrorEvent = EventBag.Pop(EAllowShrinking::No);
	HorrorEventStartedAt = GetWorld()->GetTimeSeconds();
	HorrorEventEndsAt = HorrorEventStartedAt + GetHorrorEventDuration(CurrentHorrorEvent, CurrentStage);
	FlickerEndsAt = HorrorEventStartedAt + (CurrentStage == 1 ? .8f : CurrentStage == 2 ? 1.15f : 1.45f);
	bChairRevealPending = CurrentHorrorEvent == EShowcaseHorrorEvent::ChairReveal;
	++HorrorEventSerial;
	HorrorEventHistoryMask |= 1 << static_cast<int32>(CurrentHorrorEvent);
	if (CurrentHorrorEvent == EShowcaseHorrorEvent::PropDisplacement) ApplyRandomPropChanges();
	UE_LOG(LogTemp, Display, TEXT("ShowcaseHorror: event %d stage %d"), static_cast<int32>(CurrentHorrorEvent), CurrentStage);
}

void AShowcaseLoopDirector::FinishHorrorEvent()
{
	if (bChairRevealPending) RevealFallenChair();
	bChairRevealPending = false;
	CurrentHorrorEvent = EShowcaseHorrorEvent::None;
	++CompletedHorrorEvents;
	const FVector2D Interval = GetHorrorEventIntervalRange(CurrentStage);
	NextHorrorEventAt = GetWorld()->GetTimeSeconds() + HorrorRandom.FRandRange(Interval.X, Interval.Y);
}

bool AShowcaseLoopDirector::IsLampAhead(int32 LampIndex) const
{
	if (!Lamps.IsValidIndex(LampIndex) || !IsValid(Lamps[LampIndex])) return false;
	const FVector Location = Lamps[LampIndex]->GetActorLocation();
	if (bStraightCorridor)
	{
		const float Ahead = Location.X - PlayerPathPosition;
		return Ahead > 120.f && Ahead < StraightRepeatSpan * .5f;
	}
	const FVector ToLamp = (Location - ViewPosition).GetSafeNormal2D();
	return FVector::DotProduct(ToLamp, ViewForward.GetSafeNormal2D()) > 0.f;
}

void AShowcaseLoopDirector::ApplyHorrorLighting(float Now)
{
	const float Elapsed = Now - HorrorEventStartedAt;
	for (FLightState& State : LightStates)
	{
		ULightComponent* Light = State.Light.Get();
		if (!Light) continue;
		float Multiplier = 1.f;
		FLinearColor Color = State.Color;
		if (IsLampAhead(State.LampIndex))
		{
			switch (CurrentHorrorEvent)
			{
			case EShowcaseHorrorEvent::ForwardBlackout:
			case EShowcaseHorrorEvent::ChairReveal:
				Multiplier = 0.f;
				break;
			case EShowcaseHorrorEvent::RedPulse:
				Multiplier = .72f;
				Color = FLinearColor(1.f, .012f, .004f);
				break;
			case EShowcaseHorrorEvent::FlickerOut:
				if (Now >= FlickerEndsAt) Multiplier = 0.f;
				else
				{
					const float Rate = 11.f + CurrentStage * 3.f;
					const float Phase = FMath::Frac(Elapsed * Rate + State.LampIndex * .37f);
					Multiplier = Phase < .48f ? 0.f : 1.f;
				}
				break;
			default:
				break;
			}
		}
		Light->SetIntensity(State.Intensity * Multiplier);
		Light->SetLightColor(Color);
	}

	for (FBulbState& Bulb : BulbStates)
	{
		UMaterialInstanceDynamic* Material = Bulb.Material.Get();
		if (!Material) continue;
		float Multiplier = 1.f;
		if (IsLampAhead(Bulb.LampIndex))
		{
			if (CurrentHorrorEvent == EShowcaseHorrorEvent::ForwardBlackout || CurrentHorrorEvent == EShowcaseHorrorEvent::ChairReveal)
				Multiplier = 0.f;
			else if (CurrentHorrorEvent == EShowcaseHorrorEvent::FlickerOut)
			{
				if (Now >= FlickerEndsAt) Multiplier = 0.f;
				else Multiplier = FMath::Frac(Elapsed * (11.f + CurrentStage * 3.f) + Bulb.LampIndex * .37f) < .48f ? 0.f : 1.f;
			}
		}
		Material->SetScalarParameterValue(TEXT("EmissiveStrenght"), Bulb.Emissive * Multiplier);
	}
}

void AShowcaseLoopDirector::UpdateHorrorEvents()
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (!bEnableHorrorEvents || CurrentStage <= 0)
	{
		if (CurrentHorrorEvent != EShowcaseHorrorEvent::None) FinishHorrorEvent();
		ApplyHorrorLighting(Now);
		return;
	}
	if (CurrentHorrorEvent != EShowcaseHorrorEvent::None && Now >= HorrorEventEndsAt) FinishHorrorEvent();
	if (CurrentHorrorEvent == EShowcaseHorrorEvent::None && Now >= NextHorrorEventAt) StartHorrorEvent();
	ApplyHorrorLighting(Now);
}

bool AShowcaseLoopDirector::ChangeActorProp(const TArray<TObjectPtr<AActor>>& Props, bool bFrame, bool bAllowVisible)
{
	TArray<AActor*> Candidates;
	for (AActor* Prop : Props)
	{
		if (!IsValid(Prop) || AlteredActorProps.Contains(Prop)) continue;
		const FVector Location = Prop->GetActorLocation();
		const float Along = bStraightCorridor ? Location.X - PlayerPathPosition : FVector::Dist2D(Location, ViewPosition);
		if (bStraightCorridor && (Along < -2600.f || Along > 3000.f)) continue;
		FVector Center, Extent;
		Prop->GetActorBounds(false, Center, Extent);
		if (!bAllowVisible && !IsUnseen(Center, Extent.Size())) continue;
		Candidates.Add(Prop);
	}
	if (Candidates.IsEmpty()) return false;

	AActor* Target = Candidates[HorrorRandom.RandRange(0, Candidates.Num() - 1)];
	FTransform Changed = Target->GetActorTransform();
	FVector Location = Changed.GetLocation();
	FRotator Rotation = Changed.Rotator();
	if (bFrame)
	{
		Location.X += HorrorRandom.FRandRange(-65.f, 65.f);
		Location.Y += Location.Y > 165.f ? -HorrorRandom.FRandRange(25.f, 55.f) : HorrorRandom.FRandRange(25.f, 55.f);
		Location.Z += HorrorRandom.FRandRange(-85.f, 65.f);
		Rotation.Pitch += HorrorRandom.FRandRange(-10.f, 10.f);
		Rotation.Roll += HorrorRandom.RandRange(0, 1) != 0 ? HorrorRandom.FRandRange(18.f, 35.f) : -HorrorRandom.FRandRange(18.f, 35.f);
	}
	else
	{
		Location.X += HorrorRandom.FRandRange(-55.f, 55.f);
		Location.Y += Location.Y > 165.f ? HorrorRandom.FRandRange(35.f, 80.f) : -HorrorRandom.FRandRange(35.f, 80.f);
		Location.Z += 12.f;
		Rotation.Yaw += HorrorRandom.FRandRange(-35.f, 35.f);
		Rotation.Roll += HorrorRandom.RandRange(0, 1) != 0 ? HorrorRandom.FRandRange(76.f, 92.f) : -HorrorRandom.FRandRange(76.f, 92.f);
	}
	Rotation.Normalize();
	Changed.SetLocation(Location);
	Changed.SetRotation(Rotation.Quaternion());
	Target->SetActorTransform(Changed, false, nullptr, ETeleportType::TeleportPhysics);
	AlteredActorProps.Add(Target);
	++AlteredPropCount;
	if (bFrame) ++AlteredFrameCount;
	else ++AlteredChairCount;
	return true;
}

bool AShowcaseLoopDirector::ChangeTableInstance()
{
	if (!IsValid(RepeatExtension) || !IsValid(TableMesh)) return false;
	struct FTableCandidate
	{
		UHierarchicalInstancedStaticMeshComponent* Component = nullptr;
		int32 InstanceIndex = INDEX_NONE;
		FTransform Transform;
		uint64 Key = 0;
	};
	TArray<FTableCandidate> Candidates;
	TInlineComponentArray<UHierarchicalInstancedStaticMeshComponent*> Components(RepeatExtension);
	for (UHierarchicalInstancedStaticMeshComponent* Component : Components)
	{
		if (!Component || Component->GetStaticMesh() != TableMesh) continue;
		for (int32 InstanceIndex = 0; InstanceIndex < Component->GetInstanceCount(); ++InstanceIndex)
		{
			const uint64 Key = (static_cast<uint64>(Component->GetUniqueID()) << 32) | static_cast<uint32>(InstanceIndex);
			if (AlteredTableInstances.Contains(Key)) continue;
			FTransform Transform;
			if (!Component->GetInstanceTransform(InstanceIndex, Transform, true)) continue;
			const float Along = bStraightCorridor ? Transform.GetLocation().X - PlayerPathPosition : FVector::Dist2D(Transform.GetLocation(), ViewPosition);
			if (bStraightCorridor && (Along < -2600.f || Along > 3000.f)) continue;
			if (!IsUnseen(Transform.GetLocation() + FVector(0,0,45.f), 90.f)) continue;
			Candidates.Add({Component, InstanceIndex, Transform, Key});
		}
	}
	if (Candidates.IsEmpty()) return false;

	FTableCandidate& Candidate = Candidates[HorrorRandom.RandRange(0, Candidates.Num() - 1)];
	FVector Location = Candidate.Transform.GetLocation();
	Location.X += HorrorRandom.FRandRange(-90.f, 90.f);
	Location.Y += Location.Y > 165.f ? HorrorRandom.FRandRange(55.f, 125.f) : -HorrorRandom.FRandRange(55.f, 125.f);
	FRotator Rotation = Candidate.Transform.Rotator();
	Rotation.Yaw += HorrorRandom.RandRange(0, 1) != 0 ? HorrorRandom.FRandRange(12.f, 28.f) : -HorrorRandom.FRandRange(12.f, 28.f);
	Candidate.Transform.SetLocation(Location);
	Candidate.Transform.SetRotation(Rotation.Quaternion());
	Candidate.Component->UpdateInstanceTransform(Candidate.InstanceIndex, Candidate.Transform, true, true, true);
	AlteredTableInstances.Add(Candidate.Key);
	++AlteredPropCount;
	++AlteredTableCount;
	return true;
}

void AShowcaseLoopDirector::RevealFallenChair()
{
	ChangeActorProp(ChairProps, false, true);
}

void AShowcaseLoopDirector::ApplyRandomPropChanges()
{
	const int32 Count = GetHorrorPropChangeCount(CurrentStage);
	const int32 Start = HorrorRandom.RandRange(0, 2);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		bool bChanged = false;
		for (int32 Attempt = 0; Attempt < 3 && !bChanged; ++Attempt)
		{
			switch ((Start + Index + Attempt) % 3)
			{
			case 0: bChanged = ChangeActorProp(ChairProps, false, false); break;
			case 1: bChanged = ChangeTableInstance(); break;
			case 2: bChanged = ChangeActorProp(FrameProps, true, false); break;
			default: break;
			}
		}
	}
}

void AShowcaseLoopDirector::UpdateExitSigns()
{
	if (bExitSignsReversed || !ShouldReverseExitSigns(CurrentStage, EscapeDoor && EscapeDoor->bRevealed)) return;
	for (AActor* Sign : EmergencyExitSigns)
	{
		if (!IsValid(Sign)) continue;
		FVector Scale = Sign->GetActorScale3D();
		Scale.X = -FMath::Abs(Scale.X);
		Sign->SetActorScale3D(Scale);
		++ReversedExitSignCount;
	}
	bExitSignsReversed = true;
}

void AShowcaseLoopDirector::UpdateStraightDoor()
{
	if (CurrentStage < 3 || !EscapeDoor) return;
	if (EscapeDoor->bRevealed && !IsUnseen(EscapeDoor->GetActorLocation() + FVector(0,0,160), 100.f)) bLinearDoorNoticed = true;
	if (bLinearDoorNoticed) return;

	FVector Position;
	bool bClear = false;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ShowcaseDoorClearance), false, this);
	Query.AddIgnoredActor(TrackedPawn.Get());
	Query.AddIgnoredActor(EscapeDoor);
	for (float Adjustment : {0.f, 50.f, -50.f, 100.f, -100.f, 150.f, 200.f, 250.f, 300.f, 350.f})
	{
		const float Distance = FMath::Clamp(DoorDistanceAhead + Adjustment, 450.f, 900.f);
		Position = PathPoint(-Distance, 0, 20);
		if (!GetWorld()->OverlapBlockingTestByChannel(Position + FVector(0,0,160), FQuat::Identity, ECC_Visibility,
			FCollisionShape::MakeBox(FVector(28,125,155)), Query))
		{
			bClear = true;
			break;
		}
	}
	if (!bClear) return;
	for (float Y : {-125.f, 0.f, 125.f})
		for (float Z : {40.f, 320.f})
			if (!IsUnseen(Position + FVector(0,Y,Z), 125.f)) return;
	if (EscapeDoor->bRevealed)
	{
		if (!IsUnseen(EscapeDoor->GetActorLocation() + FVector(0,0,160), 160.f)) return;
		if (FVector::Dist2D(LinearDoorPosition, Position) < 120.f) return;
		EscapeDoor->SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else EscapeDoor->RevealAt(FTransform(FRotator::ZeroRotator, Position));
	LinearDoorPosition = Position;
}

void AShowcaseLoopDirector::TryRevealDoor(float Angle)
{
	for (int32 Attempt = 0; Attempt < 7; ++Attempt)
	{
		const float DoorAngle = Angle - (DoorDistanceAhead + Attempt * 300.f) / LoopRadius;
		const float Radius = LoopRadius + WalkwayRadiusOffset;
		const FVector Position = LoopCenter + FVector(Radius * FMath::Sin(DoorAngle), -Radius * FMath::Cos(DoorAngle), 20.f);
		const FVector Side(-FMath::Sin(DoorAngle), FMath::Cos(DoorAngle), 0);
		bool bCandidateUnseen = true;
		for (float Offset : {-120.f, 0.f, 120.f})
			if (!IsUnseen(Position + Side * Offset + FVector(0,0,170), 160.f)) bCandidateUnseen = false;
		if (bCandidateUnseen)
		{
			EscapeDoor->RevealAt(FTransform(FRotator(0, FMath::RadiansToDegrees(DoorAngle), 0), Position));
			return;
		}
	}
}

void AShowcaseLoopDirector::HandleEscaped()
{
	bEscapeComplete = true;
	CurrentHorrorEvent = EShowcaseHorrorEvent::None;
	ApplyHorrorLighting(GetWorld()->GetTimeSeconds());
	OnEscapeCompleted.Broadcast();
	SetActorTickEnabled(false);
}
