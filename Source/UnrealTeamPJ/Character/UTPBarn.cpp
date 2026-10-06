// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/UTPBarn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h" // 플레이어(캐릭터) 판정을 위해 추가

// Sets default values
AUTPBarn::AUTPBarn()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// 메쉬 컴포넌트 생성 및 루트 컴포넌트로 지정
	BarnMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarnMesh"));
	RootComponent = BarnMesh;

	// 요청하신 헛간 스태틱 메쉬 에셋 로드 및 적용
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Script/Engine.StaticMesh'/Game/Fab/Old_Wooden_Barn__House_4_/ruined_house_4/StaticMeshes/ruined_house_4.ruined_house_4'"));
	if (MeshAsset.Succeeded())
	{
		BarnMesh->SetStaticMesh(MeshAsset.Object);
	}

	// 플레이어 진입을 감지할 박스 트리거 볼륨 생성
	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(RootComponent);
	
	// 트리거 볼륨의 기본 크기와 위치 설정 (에디터에서 쉽게 변경 가능)
	TriggerVolume->SetBoxExtent(FVector(300.0f, 300.0f, 200.0f)); 
	TriggerVolume->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f)); 
	
	// 겹침(Overlap)만 감지하도록 콜리전 프리셋 설정
	TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));

	// 플레이어가 박스에 닿았을 때 실행될 함수 연결(바인딩)
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AUTPBarn::OnOverlapBegin);
}

// Called when the game starts or when spawned
void AUTPBarn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AUTPBarn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// 플레이어가 진입하면 실행되는 구역
void AUTPBarn::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 닿은 대상이 자기 자신이 아니고 유효한 액터인지 확인
	if (OtherActor && (OtherActor != this))
	{
		// 닿은 액터가 플레이어 캐릭터인지 확인 (몬스터나 다른 물체에 반응하지 않게 함)
		ACharacter* PlayerCharacter = Cast<ACharacter>(OtherActor);
		if (PlayerCharacter)
		{
			// ==============================================================
			// TODO: 여기에 나중에 원하시는 스크립트(로직)를 추가하시면 됩니다!
			// 예: 레벨 이동, UI 띄우기, 엔딩 컷신 재생, 게임 클리어 등
			// ==============================================================
			
			UE_LOG(LogTemp, Warning, TEXT("플레이어가 개쩌는 헛간에 진입했습니다! 이벤트 시작!"));
		}
	}
}
