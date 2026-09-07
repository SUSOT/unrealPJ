#pragma once

#include "CoreMinimal.h"

/** Distance-driven footsteps and one extra step after stopping. No world/audio ownership. */
struct FShowcasePresenceRhythm
{
	struct FEvents { bool OwnStep = false; bool FollowerStep = false; bool AfterStop = false; };
	FEvents Advance(float Dt, float Distance, int32 Stage, bool bLookingBack)
	{
		FEvents Events;
		Dt = FMath::Clamp(Dt, 0.f, .25f);
		const bool bMoving = Distance > FMath::Max(.5f, Dt * 10.f);
		Cooldown = FMath::Max(0.f, Cooldown-Dt);
		if (bMoving)
		{
			WalkTime += Dt;
			StillTime = 0.f;
			StepDistance += FMath::Abs(Distance);
			if (StepDistance >= 130.f) { StepDistance = FMath::Fmod(StepDistance,130.f); Events.OwnStep = true; }
			if (WalkTime > 1.2f && Stage > 0 && !bLookingBack) bStopArmed = true;
		}
		else
		{
			StillTime += Dt;
			if (StillTime > .2f) WalkTime = 0.f;
		}
		if (Stage == 0 || bLookingBack)
		{
			PendingDelay = -1.f;
			BurstRemaining = 0;
			bStopArmed = false;
			Cooldown = FMath::Max(Cooldown, 2.f);
			return Events;
		}
		if (bStopArmed && StillTime >= .55f)
		{
			Events.AfterStop = true;
			bStopArmed = false;
			PendingDelay = -1.f;
			BurstRemaining = 0;
			Cooldown = 5.f;
			return Events;
		}
		if (PendingDelay >= 0.f)
		{
			PendingDelay -= Dt;
			if (PendingDelay <= 0.f) { Events.FollowerStep = true; PendingDelay = -1.f; }
		}
		if (Events.OwnStep)
		{
			if (Cooldown <= 0.f && BurstRemaining == 0) BurstRemaining = FMath::Clamp(Stage+1,2,4);
			if (BurstRemaining > 0)
			{
				PendingDelay = .26f;
				if (--BurstRemaining == 0) Cooldown = 8.f-Stage;
			}
		}
		return Events;
	}
	private:
	float StepDistance = 0.f, WalkTime = 0.f, StillTime = 0.f;
	float PendingDelay = -1.f, Cooldown = 2.f;
	int32 BurstRemaining = 0;
	bool bStopArmed = false;
};
