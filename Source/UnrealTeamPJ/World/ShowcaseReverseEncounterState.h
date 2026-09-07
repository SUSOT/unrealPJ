#pragma once
#include "CoreMinimal.h"

/** One-shot observation sequence, independent of actors/audio and forward escape progress. */
struct FShowcaseReverseEncounterState
{
	enum class EPhase : uint8 { Dormant, HeardBehind, ChairTucked, Complete };
	struct FInput
	{
		float DeltaSeconds = .05f;
		float SignedTravel = 0.f;
		float ChairDistance = 10000.f;
		bool Eligible = false;
		bool ChairVisible = false;
		bool ChairHidden = false;
		bool FacingReturn = false;
		bool ReturnSeatHidden = false;
	};
	struct FEvents { bool ScrapeBehind = false; bool TuckChair = false; bool Cutlery = false; };
	EPhase Phase = EPhase::Dormant;
	FEvents Advance(const FInput& Input)
	{
		FEvents Events;
		if (!Input.Eligible || Phase == EPhase::Complete) return Events;
		const float Dt = FMath::Clamp(Input.DeltaSeconds,0.f,.25f);
		if (Phase == EPhase::Dormant)
		{
			SeenTime = Input.SignedTravel < -500.f && Input.ChairDistance < 600.f && Input.ChairVisible ? SeenTime+Dt : 0.f;
			if (SeenTime >= .45f) { Phase=EPhase::HeardBehind; SinceScrape=0.f; Events.ScrapeBehind=true; }
		}
		else if (Phase == EPhase::HeardBehind)
		{
			SinceScrape += Dt;
			if (SinceScrape >= .3f && Input.ChairHidden && Input.FacingReturn)
			{
				Phase=EPhase::ChairTucked; ChangedAt=Input.SignedTravel; SeenTime=0.f;
				Events.TuckChair=true;
			}
		}
		else if (Phase == EPhase::ChairTucked)
		{
			if (Input.ChairVisible) SeenTime += Dt;
			if (SeenTime >= .25f && Input.SignedTravel > ChangedAt+250.f && Input.FacingReturn && Input.ReturnSeatHidden)
			{
				Phase=EPhase::Complete; Events.Cutlery=true;
			}
		}
		return Events;
	}
private:
	float SeenTime=0.f, SinceScrape=0.f, ChangedAt=0.f;
};
