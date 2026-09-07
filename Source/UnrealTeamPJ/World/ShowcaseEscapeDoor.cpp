#include "World/ShowcaseEscapeDoor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"

AShowcaseEscapeDoor::AShowcaseEscapeDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	auto MakeBox = [&](FName Name, FVector Location, FVector Scale, USceneComponent* Parent)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetStaticMesh(Cube.Object);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionProfileName(TEXT("BlockAll"));
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		return Part;
	};
	Frame.Add(MakeBox(TEXT("LeftJamb"), FVector(0,-102,146), FVector(.26,.24,2.92), GetRootComponent()));
	Frame.Add(MakeBox(TEXT("RightJamb"), FVector(0,102,146), FVector(.26,.24,2.92), GetRootComponent()));
	Frame.Add(MakeBox(TEXT("Lintel"), FVector(0,0,292), FVector(.26,2.28,.24), GetRootComponent()));
	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(GetRootComponent());
	Hinge->SetRelativeLocation(FVector(0,-90,0));
	Leaf = MakeBox(TEXT("DoorLeaf"), FVector(0,90,140), FVector(.10,1.8,2.8), Hinge);
	UStaticMeshComponent* Handle = MakeBox(TEXT("Handle"), FVector(10,158,130), FVector(.12,.22,.06), Hinge);
	Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Threshold = MakeBox(TEXT("PaleInterior"), FVector(-24,0,140), FVector(.04,1.8,2.8), GetRootComponent());
	Threshold->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Threshold->SetCastShadow(false);
	ExitSign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ExitSign"));
	ExitSign->SetupAttachment(GetRootComponent());
	ExitSign->SetRelativeLocation(FVector(15,0,330));
	ExitSign->SetHorizontalAlignment(EHTA_Center);
	ExitSign->SetVerticalAlignment(EVRTA_TextCenter);
	ExitSign->SetWorldSize(30);
	ExitSign->SetText(FText::FromString(TEXT("EXIT")));
	ExitSign->SetTextRenderColor(FColor(188,218,204));
	SpillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("DoorSpill"));
	SpillLight->SetupAttachment(GetRootComponent());
	SpillLight->SetRelativeLocation(FVector(90,0,220));
	SpillLight->SetIntensity(12.f);
	SpillLight->SetAttenuationRadius(500.f);
	SpillLight->SetLightColor(FLinearColor(.77f,.86f,1.f));
	SpillLight->SetCastShadows(false);
}

void AShowcaseEscapeDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	for (UStaticMeshComponent* Part : Frame) if (FrameMaterial) Part->SetMaterial(0, FrameMaterial);
	if (LeafMaterial) Leaf->SetMaterial(0, LeafMaterial);
	if (ThresholdMaterial) Threshold->SetMaterial(0, ThresholdMaterial);
}

void AShowcaseEscapeDoor::BeginPlay()
{
	Super::BeginPlay();
	bRevealed = bEscaped = bTravelPending = false;
	OpenAmount = 0.f;
	Hinge->SetRelativeRotation(FRotator::ZeroRotator);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void AShowcaseEscapeDoor::RevealAt(const FTransform& Transform)
{
	if (bRevealed || !IsValid(Destination)) return;
	SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
	bRevealed = true;
	bHasPreviousSample = false;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);
	UE_LOG(LogTemp, Display, TEXT("ShowcaseEscape: door revealed at %s"), *GetActorLocation().ToString());
}

void AShowcaseEscapeDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn || bEscaped || bTravelPending || !bRevealed) return;
	const FVector Local = GetActorTransform().InverseTransformPosition(Pawn->GetActorLocation());
	const bool bApproaching = Local.X > -80 && Local.X < 360 && FMath::Abs(Local.Y) < 220;
	// Once opened, stay open; do not close on the player or require an input binding.
	if (bApproaching || OpenAmount > 0.f)
	{
		OpenAmount = FMath::Min(1.f, OpenAmount + DeltaSeconds / 1.25f);
		Hinge->SetRelativeRotation(FRotator(0, 108.f * FMath::SmoothStep(0.f, 1.f, OpenAmount), 0));
	}
	if (bHasPreviousSample && OpenAmount > .8f && PreviousLocalX > 10.f && Local.X <= 10.f
		&& Local.X > -150.f && FMath::Abs(Local.Y) < 65.f && Local.Z > 40.f && Local.Z < 220.f)
	{
		bTravelPending = true;
		EscapingPawn = Pawn;
		if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			if (PC->PlayerCameraManager) PC->PlayerCameraManager->StartCameraFade(0, 1, .15f, FLinearColor::Black, true, true);
		}
		GetWorldTimerManager().SetTimer(TravelTimer, this, &AShowcaseEscapeDoor::CompleteTravel, .18f, false);
	}
	PreviousLocalX = Local.X;
	bHasPreviousSample = true;
}

void AShowcaseEscapeDoor::CompleteTravel()
{
	APawn* Pawn = EscapingPawn.Get();
	bTravelPending = false;
	if (!Pawn) return;
	const bool bArrived = IsValid(Destination) && Pawn->TeleportTo(Destination->GetActorLocation(), Destination->GetActorRotation());
	if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
	{
		if (bArrived) PC->SetControlRotation(Destination->GetActorRotation());
		if (PC->PlayerCameraManager) PC->PlayerCameraManager->StartCameraFade(1, 0, .4f, FLinearColor::Black, true, false);
	}
	if (ACharacter* Character = Cast<ACharacter>(Pawn)) Character->GetCharacterMovement()->StopMovementImmediately();
	if (!bArrived)
	{
		bHasPreviousSample = false;
		UE_LOG(LogTemp, Warning, TEXT("ShowcaseEscape: destination obstructed; doorway remains available."));
		return;
	}
	bEscaped = true;
	SetActorTickEnabled(false);
	OnEscaped.Broadcast();
	UE_LOG(LogTemp, Display, TEXT("ShowcaseEscape: escaped successfully."));
}

void AShowcaseEscapeDoor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(TravelTimer);
	Super::EndPlay(EndPlayReason);
}
