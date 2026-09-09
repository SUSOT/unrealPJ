#include "World/ShowcaseRepeatExtension.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

AShowcaseRepeatExtension::AShowcaseRepeatExtension()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = .2f;
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Static);
	SetRootComponent(SceneRoot);
}

void AShowcaseRepeatExtension::RebuildExtension()
{
	Modify();
	for (UHierarchicalInstancedStaticMeshComponent* Component : GeneratedMeshes)
	{
		if (IsValid(Component))
		{
			RemoveInstanceComponent(Component);
			Component->DestroyComponent();
		}
	}
	GeneratedMeshes.Reset();

	const int32 SafeCount = FMath::Clamp(RepeatCount, 1, 1000);
	const float SafeSpacing = FMath::Max(RepeatSpacing, 1.0f);
	for (const FShowcaseRepeatMeshGroup& Group : MeshGroups)
	{
		if (!Group.Mesh || Group.SourceTransforms.IsEmpty())
		{
			continue;
		}

		UHierarchicalInstancedStaticMeshComponent* Component = NewObject<UHierarchicalInstancedStaticMeshComponent>(
			this, NAME_None, RF_Transactional);
		Component->SetupAttachment(GetRootComponent());
		Component->SetMobility(EComponentMobility::Static);
		Component->SetStaticMesh(Group.Mesh);
		for (int32 MaterialIndex = 0; MaterialIndex < Group.Materials.Num(); ++MaterialIndex)
		{
			Component->SetMaterial(MaterialIndex, Group.Materials[MaterialIndex]);
		}
		Component->SetCollisionProfileName(Group.bCollisionEnabled ? TEXT("BlockAll") : TEXT("NoCollision"));
		Component->SetCollisionEnabled(Group.bCollisionEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		Component->SetCastShadow(Group.bCastShadow);
		if (Group.DetailCullDistance > 0)
		{
			Component->SetCullDistances(Group.DetailCullDistance * 3 / 4, Group.DetailCullDistance);
		}
		// Build the spatial tree once after the batch, not once per instance.
		Component->bAutoRebuildTreeOnInstanceChanges = false;
		AddInstanceComponent(Component);
		Component->RegisterComponent();

		TArray<FTransform> Transforms;
		Transforms.Reserve(SafeCount * Group.SourceTransforms.Num());
		for (int32 Cell = 0; Cell < SafeCount; ++Cell)
		{
			for (const FTransform& Source : Group.SourceTransforms)
			{
				FTransform Instance = Source;
				Instance.AddToTranslation(FVector((Cell + FirstCellIndex) * SafeSpacing, 0.0f, 0.0f));
				Transforms.Add(Instance);
			}
		}
		Component->AddInstances(Transforms, false, false, false);
		Component->BuildTreeIfOutdated(false, true);
		Component->bAutoRebuildTreeOnInstanceChanges = true;
		GeneratedMeshes.Add(Component);
	}
	MarkPackageDirty();
}

int32 AShowcaseRepeatExtension::GetRepeatedInstanceCount() const
{
	int32 Result = 0;
	for (const UHierarchicalInstancedStaticMeshComponent* Component : GeneratedMeshes)
	{
		if (IsValid(Component))
		{
			Result += Component->GetInstanceCount();
		}
	}
	return Result;
}

AActor* AShowcaseRepeatExtension::DuplicateFixture(AActor* Source, FVector WorldOffset)
{
	if (!IsValid(Source) || Source->GetWorld() != GetWorld())
	{
		return nullptr;
	}
	FActorSpawnParameters Parameters;
	Parameters.Template = Source;
	Parameters.OverrideLevel = GetLevel();
	Parameters.ObjectFlags = RF_Transactional;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FTransform Transform = Source->GetActorTransform();
	Transform.AddToTranslation(WorldOffset);
	AActor* Duplicate = GetWorld()->SpawnActor<AActor>(Source->GetClass(), Transform, Parameters);
	if (Duplicate)
	{
		// Blueprint template roots may already contain the source's world transform.
		// Correct the post-construction transform instead of composing it twice.
		Duplicate->SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
	}
	return Duplicate;
}

void AShowcaseRepeatExtension::ApplyFixtureDistanceLimits()
{
	for (AActor* Fixture : RepeatedFixtures)
	{
		if (!IsValid(Fixture))
		{
			continue;
		}
		TInlineComponentArray<ULightComponent*> Lights(Fixture);
		for (ULightComponent* Light : Lights)
		{
			// Original level instances use movable lights; the asset defaults are static.
			// This project has static lighting disabled.
			Light->SetMobility(EComponentMobility::Movable);
			Light->MaxDrawDistance = 7000.0f;
			Light->MaxDistanceFadeRange = 2000.0f;
			Light->MarkRenderStateDirty();
		}
		TInlineComponentArray<UStaticMeshComponent*> Meshes(Fixture);
		for (UStaticMeshComponent* Mesh : Meshes)
		{
			Mesh->SetCullDistance(7000.0f);
			if (Lights.IsEmpty())
			{
				Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Mesh->SetGenerateOverlapEvents(false);
			}
		}
	}
}

void AShowcaseRepeatExtension::BeginPlay()
{
	Super::BeginPlay();
	// The asset's construction script recreates light components on reload.
	// Reapply after all level actors have finished construction, once only.
	ApplyFixtureDistanceLimits();
	CellIndices.Reset();
	for (int32 I=0; I<RepeatCount; ++I) CellIndices.Add(FirstCellIndex+I);
	RecycledCellCount=0;
	SetActorTickEnabled(bInfiniteStraight && RepeatCount>=16 && RepeatSpacing>=100.f);
}

void AShowcaseRepeatExtension::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const APawn* Pawn=UGameplayStatics::GetPlayerPawn(this,0);
	if (!Pawn || !bInfiniteStraight || CellIndices.Num()!=RepeatCount) return;
	const FVector Local=GetActorTransform().InverseTransformPosition(Pawn->GetActorLocation());
	if (FMath::Abs(Local.Y-50.f)>600.f || Local.Z<-100.f || Local.Z>600.f) return;
	const int32 First=FMath::FloorToInt(Local.X/RepeatSpacing)-RepeatCount/2;
	const int32 Last=First+RepeatCount-1;
	for (int32 Slot=0; Slot<CellIndices.Num(); ++Slot)
	{
		int32 Cell=CellIndices[Slot];
		if (Cell>=First && Cell<=Last) continue;
		const int32 Remainder=((Cell-First)%RepeatCount+RepeatCount)%RepeatCount;
		Cell=First+Remainder;
		int32 ComponentIndex=0;
		for (const FShowcaseRepeatMeshGroup& Group : MeshGroups)
		{
			if (!Group.Mesh || Group.SourceTransforms.IsEmpty()) continue;
			if (!GeneratedMeshes.IsValidIndex(ComponentIndex)) return;
			UHierarchicalInstancedStaticMeshComponent* Component=GeneratedMeshes[ComponentIndex++];
			Component->bAutoRebuildTreeOnInstanceChanges=false;
			for (int32 I=0; I<Group.SourceTransforms.Num(); ++I)
			{
				FTransform T=Group.SourceTransforms[I];
				T.AddToTranslation(FVector(Cell*RepeatSpacing,0,0));
				Component->UpdateInstanceTransform(Slot*Group.SourceTransforms.Num()+I,T,false,false,true);
			}
			Component->MarkRenderStateDirty();
			Component->BuildTreeIfOutdated(false,true);
			Component->bAutoRebuildTreeOnInstanceChanges=true;
		}
		CellIndices[Slot]=Cell;
		++RecycledCellCount;
	}
	const float Span=RepeatCount*RepeatSpacing;
	for (AActor* Fixture : RepeatedFixtures)
	{
		if (!IsValid(Fixture)) continue;
		const FVector P=GetActorTransform().InverseTransformPosition(Fixture->GetActorLocation());
		const int32 Cell=FMath::FloorToInt(P.X/RepeatSpacing);
		if (Cell>=First && Cell<=Last) continue;
		const float Shift=FMath::FloorToFloat((First*RepeatSpacing-P.X)/Span)+1.f;
		Fixture->AddActorWorldOffset(GetActorTransform().TransformVector(FVector(Shift*Span,0,0)),false,nullptr,ETeleportType::TeleportPhysics);
	}
}

void AShowcaseRepeatExtension::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyFixtureDistanceLimits();
}
