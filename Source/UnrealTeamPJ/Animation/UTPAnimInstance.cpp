// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/UTPAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/UTPCharacter.h"


UUTPAnimInstance::UUTPAnimInstance()
{
}

void UUTPAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<ACharacter>(TryGetPawnOwner());

	if (Character)
	{
		MovementComponent = Character->GetCharacterMovement();
	}
}

void UUTPAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (Character == nullptr)
		return;

	if (MovementComponent == nullptr)
		return;

	Velocity = MovementComponent->Velocity;

	GroundSpeed = Velocity.Size2D();

	Direction = CalculateDirection(Velocity, Character->GetActorRotation());

	bShouldMove = (GroundSpeed > 3.0f && MovementComponent->GetCurrentAcceleration() != FVector::ZeroVector);

	bIsFalling = MovementComponent->IsFalling();
}
