#pragma once

#include "CoreMinimal.h"

/** Distance along the loop, not time, camera motion, or total footsteps. */
struct FShowcaseEscapeProgress
{
	float Position = 0.f;
	float FurthestForward = 0.f;
	float ReverseDistance = 0.f;
	bool bDoorRequested = false;

	void Advance(float Delta, bool bLookingBack, float ReadyDistance, float RequiredReverse)
	{
		Position += Delta;
		FurthestForward = FMath::Max(FurthestForward, Position);
		if (FurthestForward < ReadyDistance || bDoorRequested)
		{
			return;
		}
		// Jitter or briefly standing still does not cancel a deliberate reversal.
		if (!bLookingBack || Delta > 2.f)
		{
			ReverseDistance = 0.f;
		}
		else if (Delta < -0.5f)
		{
			ReverseDistance -= Delta;
		}
		bDoorRequested = ReverseDistance >= RequiredReverse;
	}
};
