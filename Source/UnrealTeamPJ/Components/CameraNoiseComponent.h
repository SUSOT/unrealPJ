// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraNoiseComponent.generated.h"

/**
 * 카메라에 Perlin 노이즈 기반의 자연스러운 흔들림(떨림)을 적용하는 컴포넌트.
 * - 정지 상태: 미세한 호흡/손 떨림 같은 느낌
 * - 이동 상태: 걸음에 의한 더 큰 흔들림
 * 캐릭터의 이동 속도에 따라 강도가 부드럽게 보간됩니다.
 */
UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class UNREALTEAMPJ_API UCameraNoiseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCameraNoiseComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ====== 정지(Idle) 상태 설정 ======

	/** 정지 시 위치 흔들림 크기 (cm 단위) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Noise|Idle")
	float IdleLocationAmplitude = 0.15f;

	/** 정지 시 회전 흔들림 크기 (도 단위) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Noise|Idle")
	float IdleRotationAmplitude = 0.2f;

	// ====== 이동(Moving) 상태 설정 ======

	/** 이동 시 위치 흔들림 크기 (cm 단위) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Noise|Moving")
	float MovingLocationAmplitude = 0.7f;

	/** 이동 시 회전 흔들림 크기 (도 단위) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Noise|Moving")
	float MovingRotationAmplitude = 0.9f;

	// ====== 공통 설정 ======

	/** 노이즈 주파수 (높을수록 빠르게 흔들림) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Noise|General", meta = (ClampMin = "0.1", ClampMax = "20.0"))
	float NoiseFrequency = 1.25f;

	/** 캐릭터가 이 속도 이상이면 '이동 중'으로 판정 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Noise|General", meta = (ClampMin = "1.0"))
	float MovingSpeedThreshold = 50.0f;

	/** 정지↔이동 보간 속도 (높을수록 빠르게 전환) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Noise|General", meta = (ClampMin = "0.5", ClampMax = "20.0"))
	float BlendSpeed = 5.0f;

private:
	/** 타겟 카메라 컴포넌트 (자동 탐색) */
	UPROPERTY()
	TObjectPtr<class UCameraComponent> CameraComp;

	/** 누적 시간 (Perlin 노이즈 입력용) */
	float NoiseTime = 0.0f;

	/** 현재 블렌드 알파 (0 = 정지, 1 = 이동) */
	float CurrentBlendAlpha = 0.0f;

	/** 각 축에 독립적인 오프셋을 만들기 위한 시드 값 */
	float NoiseSeedX = 0.0f;
	float NoiseSeedY = 0.0f;
	float NoiseSeedZ = 0.0f;
	float NoiseSeedPitch = 0.0f;
	float NoiseSeedYaw = 0.0f;
	float NoiseSeedRoll = 0.0f;

	/** Perlin 노이즈 유사 함수 (여러 옥타브 사인파 합성) */
	float SmoothNoise(float Time, float Seed) const;
};
