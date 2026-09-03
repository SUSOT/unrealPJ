// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/UTPCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "TimerManager.h"
#include "Engine/World.h"

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

	static ConstructorHelpers::FObjectFinder<UAnimSequence> GetUpFaceDownRef(TEXT("/Game/Animation/Recovery/A_UTP_GetUp_FaceDown.A_UTP_GetUp_FaceDown"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> GetUpFaceUpRef(TEXT("/Game/Animation/Recovery/A_UTP_GetUp_FaceUp.A_UTP_GetUp_FaceUp"));
	GetUpFaceDownAnimation = GetUpFaceDownRef.Object;
	GetUpFaceUpAnimation = GetUpFaceUpRef.Object;

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// 3인칭용 스프링 암은 1인칭 시점을 망치므로 다시 비활성화(주석 처리)합니다.
	// SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	// SpringArm->SetupAttachment(GetMesh(), FName("head"));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetMesh(), FName("head"));
	
	// Y값을 0으로 맞춰서 카메라를 정중앙으로 맞추고, Z값을 확 내려서 코가 안 보이게 합니다. (X: 상하, Y: 앞, Z: 좌우)
	Camera->SetRelativeLocation(FVector(15.0f, 21.0f, 0.0f)); 
	Camera->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	Camera->bUsePawnControlRotation = true; // 평소엔 컨트롤러 회전 따름

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
	CachedAnimBlueprintClass = GetMesh()->GetAnimClass();
}

// Called every frame
void AUTPCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsRecoveringFromRagdoll)
	{
		UpdateRagdollRecovery(DeltaTime);
		return;
	}

	// 떨어지는 중일 때 가장 높은 Z값을 갱신 (점프 등으로 더 높아질 수 있으므로)
	if (GetCharacterMovement()->IsFalling())
	{
		if (GetActorLocation().Z > FallStartZ)
		{
			FallStartZ = GetActorLocation().Z;
		}

		// 점프/낙하 최고점에서 지정된 높이(RagdollTriggerHeight) 이상 떨어졌을 때만 레그돌 발동
		// 일반적인 점프 높이보다 크기 때문에 점프 시에는 작동하지 않고 깊은 낙하 시에만 작동합니다.
		if (!bIsRagdolled && (FallStartZ - GetActorLocation().Z) > RagdollTriggerHeight)
		{
			StartRagdoll();
		}
	}
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

		//EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &AUTPCharacter::Sprint);
		//EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AUTPCharacter::StopSprint);
		
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
	}
	
}

void AUTPCharacter::Input_Move(const FInputActionValue& InputValue)
{
	if (bIsRagdolled || bIsRecoveringFromRagdoll)
	{
		return;
	}

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

void AUTPCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	// 떨어지기 시작할 때 현재 높이를 기록
	if (GetCharacterMovement()->MovementMode == MOVE_Falling)
	{
		FallStartZ = GetActorLocation().Z;

		// 다시 공중에 떴으므로 회복 타이머 취소 (허공에서 일어나는 것 방지)
		GetWorldTimerManager().ClearTimer(RecoverRagdollTimerHandle);
	}
	// 떨어지는 상태가 끝났을 때(착지, 수영 등) 원래 상태로 복구
	else if (PrevMovementMode == MOVE_Falling)
	{
		if (bIsRagdolled)
		{
			GetWorldTimerManager().SetTimer(RecoverRagdollTimerHandle, this, &AUTPCharacter::RecoverFromRagdoll, RagdollRestTime, false);
		}
	}
}

void AUTPCharacter::StartRagdoll()
{
	// 여전히 떨어지는 중인지 확인
	if (!GetCharacterMovement()->IsFalling()) return;

	// 이미 레그돌 상태라면 무시 (물리 엔진 초기화로 인해 끊기는 현상 방지)
	if (bIsRagdolled) return;

	bIsRagdolled = true;

	// 메쉬가 바닥을 뚫고 지나가는 것을 방지 (Continuous Collision Detection)
	GetMesh()->SetAllUseCCD(true);

	// 몸에 힘이 빠지도록 레그돌 즉시 활성화
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);
	
	// 캐릭터의 현재 이동 속도를 가져와 물리 엔진에 덮어씌움 (관성을 유지)
	FVector CurrentVelocity = GetVelocity();
	GetMesh()->SetPhysicsLinearVelocity(CurrentVelocity);
	
	// 이동 방향 구하기 (제자리 낙하라면 바라보는 방향)
	FVector VelocityDir = CurrentVelocity.GetSafeNormal2D();
	if (VelocityDir.IsNearlyZero())
	{
		VelocityDir = GetActorForwardVector();
	}

	// 위쪽(Z) 벡터와 이동 방향 벡터를 외적(Cross Product)하여 회전 축(오른쪽 방향)을 구함
	FVector SpinAxis = FVector::CrossProduct(FVector::UpVector, VelocityDir);
	
	// 진행 방향으로 머리가 아래로 쏠리며 빙글빙글(somersault) 고꾸라지도록 강력한 회전력(Angular Impulse) 추가
	// bVelChange가 true이므로 라디안/초 단위의 직접적인 회전 속도를 추가합니다. (초당 약 1바퀴 = 6.28)
	GetMesh()->AddAngularImpulseInRadians(SpinAxis * 6.0f, NAME_None, true);
	
	// 레그돌 중에는 카메라가 마우스 회전을 무시하고 머리가 구르는 대로 360도 같이 뒹굴게 함
	if (Camera)
	{
		Camera->bUsePawnControlRotation = false;
	}
}

void AUTPCharacter::RecoverFromRagdoll()
{
	// 메쉬가 아직도 빠르게 움직이고 있다면 (비탈길을 구르거나 허공을 떨어지는 중) 회복을 연기함
	// 주의: GetPhysicsLinearVelocity()에 본 이름을 명시하지 않으면 루트(캡슐)의 속도를 가져와 0이 될 수 있습니다.
	if (GetMesh()->GetPhysicsLinearVelocity(TEXT("pelvis")).Size() > 50.0f)
	{
		// 1초 뒤에 다시 일어날 수 있는지 확인
		GetWorldTimerManager().SetTimer(RecoverRagdollTimerHandle, this, &AUTPCharacter::RecoverFromRagdoll, 1.0f, false);
		return;
	}

	// 레그돌 메쉬의 현재 위치(골반 부근)를 가져옴
	FVector PelvisLocation = GetMesh()->GetSocketLocation(TEXT("pelvis"));
	const FVector HeadLocation = GetMesh()->GetSocketLocation(TEXT("head"));
	const FQuat PelvisRotation = GetMesh()->GetSocketQuaternion(TEXT("pelvis"));
	bRagdollFaceUp = PelvisRotation.RotateVector(FVector::UpVector).Z > 0.0f;

	FVector RecoveryForward = (HeadLocation - PelvisLocation).GetSafeNormal2D();
	if (bRagdollFaceUp)
	{
		RecoveryForward *= -1.0f;
	}
	if (!RecoveryForward.IsNearlyZero())
	{
		SetActorRotation(RecoveryForward.Rotation());
	}

	// 레그돌 비활성화 전 캡슐(액터)을 메쉬가 굴러간 위치로 이동시킴
	// 바닥을 찾아서 캡슐이 땅에 정확히 서도록 설정
	FHitResult HitResult;
	FVector StartTrace = PelvisLocation + FVector(0.0f, 0.0f, 50.0f); // 펠비스에서 조금 위부터 시작
	FVector EndTrace = PelvisLocation - FVector(0.0f, 0.0f, 500.0f);  // 아래로 레이캐스트

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECC_Visibility, QueryParams))
	{
		FVector NewLocation = HitResult.Location + FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.0f);
		SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		SetActorLocation(PelvisLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	bIsRagdolled = false;
	bIsRecoveringFromRagdoll = true;
	bRecoveryPhysicsBlendFinished = false;
	RagdollRecoveryElapsed = 0.0f;

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetMesh()->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
	GetMesh()->SetAllPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->PutAllRigidBodiesToSleep();
	GetMesh()->bBlendPhysics = true;
	GetMesh()->SetAllBodiesPhysicsBlendWeight(1.0f);

	UAnimSequence* RecoveryAnimation = bRagdollFaceUp ? GetUpFaceUpAnimation : GetUpFaceDownAnimation;
	RecoveryAnimationDuration = RecoveryAnimation ? RecoveryAnimation->GetPlayLength() : RagdollRecoveryBlendDuration;
	PlayGetUpAnimation(RecoveryAnimation);
}

void AUTPCharacter::PlayGetUpAnimation(UAnimSequence* Animation)
{
	if (!Animation)
	{
		return;
	}

	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(Animation, false);
	if (UAnimSingleNodeInstance* SingleNodeInstance = GetMesh()->GetSingleNodeInstance())
	{
		SingleNodeInstance->SetPosition(Animation->GetPlayLength(), false);
		SingleNodeInstance->SetReverse(true);
		SingleNodeInstance->SetPlaying(true);
	}
}

void AUTPCharacter::UpdateRagdollRecovery(float DeltaTime)
{
	RagdollRecoveryElapsed += DeltaTime;
	const float BlendDuration = FMath::Max(RagdollRecoveryBlendDuration, 0.1f);
	const float Alpha = FMath::Clamp(RagdollRecoveryElapsed / BlendDuration, 0.0f, 1.0f);
	const float SmoothedAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);

	if (!bRecoveryPhysicsBlendFinished)
	{
		GetMesh()->SetAllBodiesPhysicsBlendWeight(1.0f - SmoothedAlpha);
	}

	if (Alpha >= 1.0f)
	{
		FinalizeRecoveryPhysicsBlend();
	}

	if (RagdollRecoveryElapsed >= RecoveryAnimationDuration)
	{
		FinishRagdollRecovery();
	}
}

void AUTPCharacter::FinalizeRecoveryPhysicsBlend()
{
	if (bRecoveryPhysicsBlendFinished)
	{
		return;
	}

	GetMesh()->SetAllBodiesPhysicsBlendWeight(0.0f);
	GetMesh()->bBlendPhysics = false;
	GetMesh()->SetAllUseCCD(false);
	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
	
	if (Camera)
	{
		Camera->bUsePawnControlRotation = true;
		
		// 틱(Tick) 등에서 임의로 변경된 카메라 위치/회전값이 있다면 1인칭 기본 위치로 완전히 초기화
		Camera->SetRelativeLocation(FVector(15.0f, 21.0f, 0.0f));
		Camera->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	}
	
	// 캡슐 컴포넌트에 다시 어태치(Attach)하고 기본 트랜스폼으로 초기화
	GetMesh()->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	bRecoveryPhysicsBlendFinished = true;
}

void AUTPCharacter::RestoreAnimationBlueprint()
{
	if (CachedAnimBlueprintClass)
	{
		GetMesh()->SetAnimInstanceClass(CachedAnimBlueprintClass);
	}
	else
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	}
}

void AUTPCharacter::FinishRagdollRecovery()
{
	FinalizeRecoveryPhysicsBlend();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	bIsRecoveringFromRagdoll = false;
	RagdollRecoveryElapsed = 0.0f;
	RecoveryAnimationDuration = 0.0f;
	RestoreAnimationBlueprint();
}
