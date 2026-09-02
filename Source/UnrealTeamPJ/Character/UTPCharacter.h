// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/CameraNoiseComponent.h"
#include "UTPCharacter.generated.h"

struct FInputActionValue;
class UAnimInstance;
class UAnimSequence;

UCLASS()
class UNREALTEAMPJ_API AUTPCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AUTPCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void NotifyControllerChanged() override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UCameraComponent> Camera;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	//TObjectPtr<class UCameraNoiseComponent> CameraNoiseComp;

protected:
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;

	// 공중에 떨어지는 동안 재생할 카메라 흔들림 클래스 (블루프린트에서 설정, Looping 추천)
	UPROPERTY(EditAnywhere, Category = "Camera")
	TSubclassOf<class UCameraShakeBase> FallCameraShakeClass;

	// 카메라 흔들림이 발생하기 시작하는 최소 낙하 높이
	UPROPERTY(EditAnywhere, Category = "Camera")
	float MinFallHeightForShake = 500.0f; 

private:
	// 낙하 시작(또는 최고점) 높이 기록용
	float FallStartZ;
	
	// 현재 재생 중인 낙하 카메라 쉐이크 추적용
	UPROPERTY()
	class UCameraShakeBase* ActiveFallShake;

protected:
	FTimerHandle FallRagdollTimerHandle;
	FTimerHandle RecoverRagdollTimerHandle;
	bool bIsRagdolled = false;
	bool bIsRecoveringFromRagdoll = false;
	bool bRecoveryPhysicsBlendFinished = false;
	bool bRagdollFaceUp = false;
	float RagdollRecoveryElapsed = 0.0f;
	float RecoveryAnimationDuration = 0.0f;

	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> CachedAnimBlueprintClass;

	void StartRagdoll();
	void RecoverFromRagdoll();
	void UpdateRagdollRecovery(float DeltaTime);
	void FinalizeRecoveryPhysicsBlend();
	void FinishRagdollRecovery();
	void PlayGetUpAnimation(UAnimSequence* Animation);
	void RestoreAnimationBlueprint();

public:
	void Input_Move(const FInputActionValue& InputValue);
	void Input_Look(const FInputActionValue& InputValue);
	void Sprint();
	void StopSprint();

protected:
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<class UInputAction> MoveAction;
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<class UInputAction> LookAction;
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<class UInputAction> SprintAction;
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<class UInputAction> JumpAction;

	// 레그돌이 발동되는 최소 낙하 거리 (점프 높이보다 높게 설정해야 일반 점프 시 레그돌 방지)
	UPROPERTY(EditAnywhere, Category = "Ragdoll")
	float RagdollTriggerHeight = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Ragdoll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RagdollRestTime = 1.25f;

	UPROPERTY(EditAnywhere, Category = "Ragdoll", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float RagdollRecoveryBlendDuration = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Animation|Recovery")
	TObjectPtr<UAnimSequence> GetUpFaceDownAnimation;

	UPROPERTY(EditAnywhere, Category = "Animation|Recovery")
	TObjectPtr<UAnimSequence> GetUpFaceUpAnimation;

protected:
	UPROPERTY(EditAnywhere, Category = Move)
	float WalkSpeed = 300.0f;

	UPROPERTY(EditAnywhere, Category = Move)
	float SprintSpeed = 600.0f;



};
