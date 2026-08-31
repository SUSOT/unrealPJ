#include "World/SkyEyesManager.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

ASkyEyesManager::ASkyEyesManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Movable);
	SetRootComponent(SceneRoot);
	SetActorEnableCollision(false);

	EyePlaneMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Plane.Plane")));
	EyeMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Developers/LCM/Material/M_SkyEye.M_SkyEye")));
}

void ASkyEyesManager::BeginPlay()
{
	Super::BeginPlay();
	CreateEyes();
}

void ASkyEyesManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyEyes();
	Super::EndPlay(EndPlayReason);
}

void ASkyEyesManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!CameraManager || RuntimeEyes.IsEmpty())
	{
		return;
	}

	const FVector CameraLocation = CameraManager->GetCameraLocation();
	const FRotator CameraRotation = CameraManager->GetCameraRotation();
	const FVector CameraForward = CameraRotation.Vector().GetSafeNormal();
	const FVector CameraRight = FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::Y);
	const float HorizontalFOV = CameraManager->GetFOVAngle();

	SetActorLocation(CameraLocation, false, nullptr, ETeleportType::TeleportPhysics);

	if (!bCameraInitialized)
	{
		InitializeFromCamera(CameraForward, HorizontalFOV);
		bCameraInitialized = true;
	}

	for (FSkyEyeRuntimeState& Eye : RuntimeEyes)
	{
		const bool bShouldClose = ShouldEyeClose(CameraForward, HorizontalFOV, Eye.Direction);
		float TargetBlink = 0.0f;

		if (bShouldClose)
		{
			Eye.CloseZoneElapsed += DeltaSeconds;
			TargetBlink = Eye.CloseZoneElapsed >= ReactionDelay ? 1.0f : 0.0f;
		}
		else
		{
			Eye.CloseZoneElapsed = 0.0f;
		}

		const float AnimationDuration = TargetBlink > Eye.BlinkAmount ? CloseDuration : OpenDuration;
		const float AnimationRate = 1.0f / FMath::Max(AnimationDuration, KINDA_SMALL_NUMBER);
		Eye.BlinkAmount = FMath::FInterpConstantTo(Eye.BlinkAmount, TargetBlink, DeltaSeconds, AnimationRate);
		ApplyEyeVisual(Eye, CameraRight);
	}
}

void ASkyEyesManager::CreateEyes()
{
	DestroyEyes();

	UStaticMesh* PlaneMesh = EyePlaneMesh.LoadSynchronous();
	UMaterialInterface* Material = EyeMaterial.LoadSynchronous();
	if (!PlaneMesh || !Material)
	{
		UE_LOG(LogTemp, Error, TEXT("SkyEyesManager could not load its eye mesh or material."));
		return;
	}

	const int32 SafeEyeCount = FMath::Clamp(EyeCount, 8, 64);
	const float SafeAspectRatio = FMath::Max(EyeTextureAspectRatio, 0.1f);
	const float YawStep = 360.0f / static_cast<float>(SafeEyeCount);
	FRandomStream Random(RandomSeed);
	RuntimeEyes.Reserve(SafeEyeCount);

	for (int32 Index = 0; Index < SafeEyeCount; ++Index)
	{
		const float Yaw = Index * YawStep + Random.FRandRange(-YawStep * 0.32f, YawStep * 0.32f);
		const float Elevation = Random.FRandRange(MinimumElevation, MaximumElevation);
		const FVector Direction = FRotator(Elevation, Yaw, 0.0f).Vector().GetSafeNormal();
		const float EyeWidth = Random.FRandRange(MinimumEyeWidth, MaximumEyeWidth);
		const float EyeHeightVariation = Random.FRandRange(0.84f, 1.16f);

		UStaticMeshComponent* EyeComponent = NewObject<UStaticMeshComponent>(
			this,
			*FString::Printf(TEXT("SkyEye_%02d"), Index),
			RF_Transient);
		EyeComponent->CreationMethod = EComponentCreationMethod::Instance;
		EyeComponent->SetupAttachment(SceneRoot);
		EyeComponent->SetStaticMesh(PlaneMesh);
		EyeComponent->SetMaterial(0, Material);
		EyeComponent->SetMobility(EComponentMobility::Movable);
		EyeComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		EyeComponent->SetGenerateOverlapEvents(false);
		EyeComponent->SetCastShadow(false);
		EyeComponent->SetCanEverAffectNavigation(false);
		EyeComponent->SetReceivesDecals(false);
		EyeComponent->SetTranslucentSortPriority(-10);
		EyeComponent->SetRelativeLocation(Direction * SphereRadius);
		EyeComponent->SetVisibility(false);
		AddInstanceComponent(EyeComponent);
		EyeComponent->RegisterComponent();

		FSkyEyeRuntimeState& Eye = RuntimeEyes.AddDefaulted_GetRef();
		Eye.Component = EyeComponent;
		Eye.Direction = Direction;
		Eye.WidthScale = EyeWidth / 100.0f;
		Eye.OpenHeightScale = EyeWidth / (100.0f * SafeAspectRatio) * EyeHeightVariation;
	}

	UE_LOG(LogTemp, Display, TEXT("SkyEyesManager created %d reactive sky eyes."), RuntimeEyes.Num());
}

void ASkyEyesManager::DestroyEyes()
{
	for (FSkyEyeRuntimeState& Eye : RuntimeEyes)
	{
		if (Eye.Component)
		{
			Eye.Component->DestroyComponent();
		}
	}

	RuntimeEyes.Reset();
	bCameraInitialized = false;
}

void ASkyEyesManager::InitializeFromCamera(const FVector& CameraForward, float HorizontalFOV)
{
	for (FSkyEyeRuntimeState& Eye : RuntimeEyes)
	{
		const bool bStartsClosed = ShouldEyeClose(CameraForward, HorizontalFOV, Eye.Direction);
		Eye.BlinkAmount = bStartsClosed ? 1.0f : 0.0f;
		Eye.CloseZoneElapsed = bStartsClosed ? ReactionDelay : 0.0f;
	}
}

void ASkyEyesManager::ApplyEyeVisual(FSkyEyeRuntimeState& Eye, const FVector& CameraRight)
{
	if (!Eye.Component)
	{
		return;
	}

	const float Openness = FMath::Clamp(1.0f - Eye.BlinkAmount, 0.0f, 1.0f);
	const bool bShouldRender = Openness > 0.015f;
	Eye.Component->SetVisibility(bShouldRender);
	if (!bShouldRender)
	{
		return;
	}

	const FVector ToCamera = -Eye.Direction;
	const FRotator FacingRotation = FRotationMatrix::MakeFromZX(ToCamera, CameraRight).Rotator();
	Eye.Component->SetWorldRotation(FacingRotation);
	Eye.Component->SetRelativeScale3D(FVector(
		Eye.WidthScale,
		FMath::Max(Eye.OpenHeightScale * Openness, 0.001f),
		1.0f));
}

bool ASkyEyesManager::ShouldEyeClose(
	const FVector& CameraForward,
	float HorizontalFOV,
	const FVector& EyeDirection) const
{
	const float CloseAngle = FMath::Clamp(HorizontalFOV * 0.5f + AnticipationAngle, 1.0f, 120.0f);
	const float MinimumDot = FMath::Cos(FMath::DegreesToRadians(CloseAngle));
	return FVector::DotProduct(CameraForward, EyeDirection) >= MinimumDot;
}
