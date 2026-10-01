#include "World/FallRestartVolume.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

AFallRestartVolume::AFallRestartVolume()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
}

void AFallRestartVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bRestartRequested || !HasAuthority()) return;
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!IsValid(Player)) return;

	float PlayerZ = Player->GetActorLocation().Z;
	if (ACharacter* Character = Cast<ACharacter>(Player))
	{
		// During ragdoll the body can fall independently of its capsule.
		USkeletalMeshComponent* Mesh = Character->GetMesh();
		if (Mesh && Mesh->IsAnySimulatingPhysics())
		{
			const FVector BodyLocation = Mesh->DoesSocketExist(TEXT("pelvis"))
				? Mesh->GetSocketLocation(TEXT("pelvis")) : Mesh->GetComponentLocation();
			PlayerZ = FMath::Min(PlayerZ, static_cast<float>(BodyLocation.Z));
		}
	}
	if (PlayerZ < RestartBelowZ)
	{
		bRestartRequested = true;
		UE_LOG(LogTemp, Display, TEXT("FallRestart: reloading level at player Z %.1f"), PlayerZ);
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
	}
}
