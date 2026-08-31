// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/UTPCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
// Sets default values
AUTPCharacter::AUTPCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f ));

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshRef(TEXT("/Script/Engine.SkeletalMesh'/Game/Scanned3DPeoplePack/RP_Character/rp_manuel_rigged_001_ue4/rp_manuel_rigged_001_ue4.rp_manuel_rigged_001_ue4'"));
	if (MeshRef.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MeshRef.Object);
	}

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	//SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	//SpringArm->SetupAttachment(GetRootComponent());
	//SpringArm->SetRelativeLocation(FVector(20.0f, 0.0f, 83.0f));
	//SpringArm->TargetArmLength = 0.0f;
	//SpringArm->bUsePawnControlRotation = true;
	//SpringArm->bEnableCameraRotationLag = false;
	//SpringArm->bEnableCameraLag = false;
	//SpringArm->CameraLagSpeed = 15.0f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetMesh(), FName("head"));
	Camera->SetRelativeLocation(FVector(18.0f, 17.0f, 0.0f));
	Camera->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	Camera->bUsePawnControlRotation = true; // 컨트롤러 회전 사용 (마우스 상하좌우)

	// CameraCollider = CreateDefaultSubobject<USphereComponent>(TEXT("CameraCollider"));
	// CameraCollider->SetupAttachment(Camera);
	// CameraCollider->InitSphereRadius(30.0f);
	// CameraCollider->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);

	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCapsuleRadius(45.0f);
	}

	//CameraNoiseComp = CreateDefaultSubobject<UCameraNoiseComponent>(TEXT("CameraNoise"));	
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

void AUTPCharacter::NotifyControllerChanged()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());

		if (Subsystem)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

// Called to bind functionality to input
void AUTPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	auto* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInputComponent)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AUTPCharacter::Input_Move);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this,&AUTPCharacter::Input_Look);

		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &AUTPCharacter::Sprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AUTPCharacter::StopSprint);
	}
	
}

void AUTPCharacter::Input_Move(const FInputActionValue& InputValue)
{
	FVector2D MovementVector = InputValue.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardVector, MovementVector.X);
		AddMovementInput(RightVector, MovementVector.Y);
	}
}

void AUTPCharacter::Input_Look(const FInputActionValue& InputValue)
{
	FVector2D LookVector = InputValue.Get<FVector2D>();

	AddControllerYawInput(LookVector.X);
	AddControllerPitchInput(-LookVector.Y);
}

void AUTPCharacter::Sprint()
{
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AUTPCharacter::StopSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}
