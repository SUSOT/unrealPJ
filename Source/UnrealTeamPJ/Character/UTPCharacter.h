// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/CameraNoiseComponent.h"
#include "UTPCharacter.generated.h"

struct FInputActionValue;

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
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	//TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UCameraComponent> Camera;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	//TObjectPtr<class UCameraNoiseComponent> CameraNoiseComp;

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


protected:
	UPROPERTY(EditAnywhere, Category = Move)
	float WalkSpeed = 300.0f;

	UPROPERTY(EditAnywhere, Category = Move)
	float SprintSpeed = 600.0f;



};
