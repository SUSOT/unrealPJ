// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UTPBarn.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

UCLASS()
class UNREALTEAMPJ_API AUTPBarn : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AUTPBarn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 플레이어가 헛간 내부 트리거(박스)에 닿았을 때 호출될 함수
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 헛간의 외형을 담당할 스태틱 메쉬 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barn")
	UStaticMeshComponent* BarnMesh;

	// 플레이어 진입을 감지할 트리거 볼륨 (충돌 영역)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barn")
	UBoxComponent* TriggerVolume;
};
