// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "UTPAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class UNREALTEAMPJ_API UUTPAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UUTPAnimInstance();

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = References)
	TObjectPtr<class ACharacter> Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = References)
	TObjectPtr<class UCharacterMovementComponent> MovementComponent;

protected:
	UPROPERTY(BlueprintReadOnly, Category = MovementData)
	float Direction = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = MovementData)
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = MovementData)
	float GroundSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = MovementData)
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = MovementData)
	bool bIsFalling = false;
};
