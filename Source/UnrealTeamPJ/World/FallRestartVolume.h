#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FallRestartVolume.generated.h"

/** Level-local safety plane: reload the level when the player falls below it. */
UCLASS()
class UNREALTEAMPJ_API AFallRestartVolume : public AActor
{
	GENERATED_BODY()
public:
	AFallRestartVolume();
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fall Restart")
	float RestartBelowZ = -1500.f;

private:
	bool bRestartRequested = false;
};
