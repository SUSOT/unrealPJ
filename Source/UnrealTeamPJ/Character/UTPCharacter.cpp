// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/UTPCharacter.h"

// Sets default values
AUTPCharacter::AUTPCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AUTPCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AUTPCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AUTPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

