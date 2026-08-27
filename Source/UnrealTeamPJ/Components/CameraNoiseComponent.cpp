// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/CameraNoiseComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UCameraNoiseComponent::UCameraNoiseComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// 각 축마다 다른 노이즈 패턴을 만들기 위해 서로 다른 시드를 사용합니다.
	// 임의로 크게 떨어뜨려 놓으면 독립적인 노이즈처럼 보입니다.
	NoiseSeedX = 432;
	NoiseSeedY = 123;
	NoiseSeedZ = 245;
	NoiseSeedPitch = 513;
	NoiseSeedYaw = 375;
	NoiseSeedRoll = 34;
}

void UCameraNoiseComponent::BeginPlay()
{
	Super::BeginPlay();

	// 오너 액터에서 카메라 컴포넌트를 자동으로 찾습니다.
	AActor* Owner = GetOwner();
	if (Owner)
	{
		CameraComp = Owner->FindComponentByClass<UCameraComponent>();

		if (!CameraComp)
		{
			UE_LOG(LogTemp, Warning, TEXT("CameraNoiseComponent: 오너(%s)에서 UCameraComponent를 찾지 못했습니다."), *Owner->GetName());
		}
	}
}

void UCameraNoiseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CameraComp)
	{
		return;
	}

	// 1) 캐릭터 이동 속도 계산
	float Speed = 0.0f;
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter && OwnerCharacter->GetCharacterMovement())
	{
		Speed = OwnerCharacter->GetCharacterMovement()->Velocity.Size();
	}

	// 2) 이동 여부에 따라 블렌드 알파를 부드럽게 보간
	//    Speed가 클수록 TargetAlpha가 1에 가까워짐 (비선형 매핑으로 부드럽게)
	float SpeedRatio = FMath::Clamp(Speed / MovingSpeedThreshold, 0.0f, 1.0f);
	float TargetAlpha = SpeedRatio * SpeedRatio; // ease-in 곡선으로 자연스럽게
	CurrentBlendAlpha = FMath::FInterpTo(CurrentBlendAlpha, TargetAlpha, DeltaTime, BlendSpeed);

	// 3) 현재 흔들림 강도 계산 (Idle과 Moving 사이 선형 보간)
	float CurrentLocationAmp = FMath::Lerp(IdleLocationAmplitude, MovingLocationAmplitude, CurrentBlendAlpha);
	float CurrentRotationAmp = FMath::Lerp(IdleRotationAmplitude, MovingRotationAmplitude, CurrentBlendAlpha);

	// 달리기(Sprint) 상태일 때 더 격렬하게 하기 위한 처리
	// UTPCharacter의 기본 WalkSpeed는 500.0f, SprintSpeed는 800.0f입니다.
	// 속도가 500을 넘어서는 비율에 따라 진폭과 흔들림 속도를 증가시킵니다.
	float SprintRatio = FMath::Clamp((Speed - 500.0f) / 300.0f, 0.0f, 1.0f);
	float FinalLocationAmp = CurrentLocationAmp * FMath::Lerp(1.0f, 1.75f, SprintRatio);
	float FinalRotationAmp = CurrentRotationAmp * FMath::Lerp(1.0f, 1.75f, SprintRatio);
	float FinalFrequency = NoiseFrequency * FMath::Lerp(1.0f, 1.75f, SprintRatio);

	// 4) 노이즈 시간 누적
	NoiseTime += DeltaTime * FinalFrequency;

	// 5) 각 축에 독립적인 부드러운 노이즈 생성 (-1 ~ +1 범위)
	float NoiseLocX = SmoothNoise(NoiseTime, NoiseSeedX);
	float NoiseLocY = SmoothNoise(NoiseTime, NoiseSeedY);
	float NoiseLocZ = SmoothNoise(NoiseTime, NoiseSeedZ);

	float NoisePitch = SmoothNoise(NoiseTime, NoiseSeedPitch);
	float NoiseYaw = SmoothNoise(NoiseTime, NoiseSeedYaw);
	float NoiseRoll = SmoothNoise(NoiseTime, NoiseSeedRoll);

	// 6) 카메라에 오프셋 적용
	//    위치: 좌우와 상하에만 적용 (앞뒤는 1인칭에서 어색할 수 있으므로 약하게)
	FVector LocationOffset(
		NoiseLocX * FinalLocationAmp * 0.3f,  // 앞뒤 (약하게)
		NoiseLocY * FinalLocationAmp,           // 좌우
		NoiseLocZ * FinalLocationAmp * 0.8f     // 상하
	);

	//    회전: Pitch와 Roll 위주 (Yaw는 약하게 — 과하면 어지러움 유발)
	FRotator RotationOffset(
		NoisePitch * FinalRotationAmp,           // Pitch (고개 끄덕)
		NoiseYaw * FinalRotationAmp * 0.8f,      // Yaw (좌우 회전, 약하게)
		NoiseRoll * FinalRotationAmp * 0.3f      // Roll (기울기)
	);

	// 카메라의 상대 위치/회전에 노이즈를 적용합니다.
	// 매 프레임 새로 계산하므로 기존 오프셋 위에 누적되지 않습니다.
	CameraComp->SetRelativeLocation(LocationOffset);
	CameraComp->SetRelativeRotation(RotationOffset);
}

float UCameraNoiseComponent::SmoothNoise(float Time, float Seed) const
{
	// 여러 주파수의 사인파를 합성하여 Perlin 노이즈와 유사한 부드럽고
	// 불규칙한 패턴을 생성합니다. (Fractional Brownian Motion 방식)
	//
	// 옥타브별로 주파수를 높이고 진폭을 줄여서 자연스러운 느낌을 만듭니다.
	float Value = 0.0f;

	// 옥타브 1: 기본 저주파 흔들림 (느린 큰 움직임)
	Value += FMath::Sin(Time * 1.0f + Seed) * 0.5f;

	// 옥타브 2: 중간 주파수 (약간 빠른 움직임)
	Value += FMath::Sin(Time * 2.13f + Seed + 17.3f) * 0.25f;

	// 옥타브 3: 고주파 디테일 (미세한 떨림)
	Value += FMath::Sin(Time * 4.57f + Seed + 43.7f) * 0.125f;

	// 옥타브 4: 매우 고주파 (아주 미세한 노이즈)
	Value += FMath::Sin(Time * 8.91f + Seed + 91.1f) * 0.0625f;

	// 정규화 (합의 최대값으로 나눠서 -1 ~ +1 범위로)
	// 0.5 + 0.25 + 0.125 + 0.0625 = 0.9375
	Value /= 0.9375f;

	return Value;
}
