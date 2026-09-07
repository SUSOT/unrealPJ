#include "World/ShowcaseEscapeProgress.h"
#include "Misc/AutomationTest.h"
#include "World/ShowcaseLoopDirector.h"
#include "World/ShowcasePresenceRhythm.h"
#include "World/ShowcaseReverseEncounterState.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseReverseEncounterTest, "UnrealTeamPJ.Showcase.ReverseEncounter", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShowcaseReverseEncounterTest::RunTest(const FString& Parameters)
{
	FShowcaseReverseEncounterState State;
	FShowcaseReverseEncounterState::FInput Input;
	Input.Eligible = true; Input.SignedTravel = -900.f; Input.ChairDistance = 450.f;
	int32 Scrapes=0, Tucks=0, Cutlery=0;
	for (int32 I=0; I<20; ++I) Scrapes += State.Advance(Input).ScrapeBehind;
	TestEqual(TEXT("Passing without seeing the chair cannot trigger the scene"),Scrapes,0);
	Input.ChairVisible = true;
	for (int32 I=0; I<20; ++I) Scrapes += State.Advance(Input).ScrapeBehind;
	TestEqual(TEXT("Seeing the pulled chair triggers one sound behind"),Scrapes,1);
	for (int32 I=0; I<40; ++I) Tucks += State.Advance(Input).TuckChair;
	TestEqual(TEXT("Chair cannot move while watched"),Tucks,0);
	Input.ChairVisible=false; Input.ChairHidden=true; Input.FacingReturn=true;
	for (int32 I=0; I<10; ++I) Tucks += State.Advance(Input).TuckChair;
	TestEqual(TEXT("Looking behind permits exactly one hidden chair change"),Tucks,1);
	Input.ChairVisible=true; Input.ChairHidden=false; Input.FacingReturn=false;
	for (int32 I=0; I<10; ++I) State.Advance(Input);
	Input.SignedTravel=-550.f; Input.FacingReturn=true; Input.ReturnSeatHidden=true;
	for (int32 I=0; I<60; ++I) Cutlery += State.Advance(Input).Cutlery;
	TestEqual(TEXT("Returning after observing the change produces one cutlery cue"),Cutlery,1);
	TestTrue(TEXT("Encounter completes without forcing an exit or death"),State.Phase==FShowcaseReverseEncounterState::EPhase::Complete);
	State=FShowcaseReverseEncounterState(); Input.Eligible=false;
	for (int32 I=0; I<60; ++I) Scrapes += State.Advance(Input).ScrapeBehind;
	TestEqual(TEXT("Forward-route play cannot start the reverse encounter"),Scrapes,1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcasePresenceTest, "UnrealTeamPJ.Showcase.PresenceRhythm", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShowcasePresenceTest::RunTest(const FString& Parameters)
{
	FShowcasePresenceRhythm Rhythm;
	int32 Own = 0, Follower = 0, Stops = 0;
	for (int32 I=0; I<100; ++I)
	{
		auto E = Rhythm.Advance(.05f, 15.f, 0, false);
		Own += E.OwnStep; Follower += E.FollowerStep; Stops += E.AfterStop;
	}
	TestTrue(TEXT("Ordinary walk establishes a footstep rhythm"), Own >= 10 && Own <= 12);
	TestEqual(TEXT("No presence before the first change"), Follower+Stops, 0);
	for (int32 I=0; I<200; ++I) Follower += Rhythm.Advance(.05f,15.f,2,false).FollowerStep;
	TestTrue(TEXT("Occasional delayed steps while moving"), Follower > 0 && Follower < 16);
	for (int32 I=0; I<200; ++I) Stops += Rhythm.Advance(.05f,0.f,2,false).AfterStop;
	TestEqual(TEXT("Exactly one extra step after player stops"), Stops, 1);
	int32 SeenPresence = 0;
	for (int32 I=0; I<200; ++I)
	{
		auto E = Rhythm.Advance(.05f,I<100?15.f:0.f,3,true);
		SeenPresence += E.FollowerStep + E.AfterStop;
	}
	TestEqual(TEXT("Looking back silences the unseen follower"), SeenPresence, 0);
	Rhythm = FShowcasePresenceRhythm();
	int32 IdleCues = 0;
	for (int32 I=0; I<400; ++I)
	{
		auto E = Rhythm.Advance(.05f,0.f,3,false);
		IdleCues += E.OwnStep + E.FollowerStep + E.AfterStop;
	}
	TestEqual(TEXT("Standing idle cannot manufacture footsteps"), IdleCues, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseWalkingPaceTest, "UnrealTeamPJ.Showcase.WalkingPace", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShowcaseWalkingPaceTest::RunTest(const FString& Parameters)
{
	const AShowcaseLoopDirector* Defaults = GetDefault<AShowcaseLoopDirector>();
	TestEqual(TEXT("First change after 40 m of walking"), Defaults->FirstChangeDistance, 4000.f);
	TestEqual(TEXT("Second change after 70 m of walking"), Defaults->SecondChangeDistance, 7000.f);
	TestEqual(TEXT("Turn-back available after 90 m"), Defaults->TurnBackUnlockDistance, 9000.f);
	TestEqual(TEXT("Deliberate 5 m backtrack remains required"), Defaults->RequiredBacktrackDistance, 500.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseEscapeProgressTest, "UnrealTeamPJ.Showcase.EscapeProgress", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShowcaseEscapeProgressTest::RunTest(const FString& Parameters)
{
	FShowcaseEscapeProgress P;
	for (int32 I=0; I<100; ++I) { P.Advance(100,false,20500,500); P.Advance(-100,true,20500,500); }
	TestEqual(TEXT("Back/forth cannot farm forward progress"), P.FurthestForward, 100.f);
	TestFalse(TEXT("Early reversal cannot reveal the door"), P.bDoorRequested);
	P = FShowcaseEscapeProgress();
	for (int32 I=0; I<205; ++I) P.Advance(100,false,20500,500);
	TestFalse(TEXT("Forward travel alone cannot reveal"), P.bDoorRequested);
	P.Advance(0,true,20500,500);
	TestFalse(TEXT("Camera turn without walking cannot reveal"), P.bDoorRequested);
	P.Advance(-600,false,20500,500);
	TestFalse(TEXT("Walking backward while looking forward cannot reveal"), P.bDoorRequested);
	P.Advance(-300,true,20500,500);
	P.Advance(10,true,20500,500);
	TestEqual(TEXT("Forward movement cancels partial reversal"), P.ReverseDistance, 0.f);
	P.Advance(-300,true,20500,500);
	P.Advance(0,true,20500,500);
	TestFalse(TEXT("Partial backtrack remains locked"), P.bDoorRequested);
	P.Advance(-200,true,20500,500);
	TestTrue(TEXT("Deliberate 5 m turn-back unlocks"), P.bDoorRequested);
	P.Advance(500,false,20500,500);
	TestTrue(TEXT("Door request stays latched"), P.bDoorRequested);
	const float Wrap = FMath::FindDeltaAngleRadians(FMath::DegreesToRadians(179.f), FMath::DegreesToRadians(-179.f));
	TestTrue(TEXT("Crossing angular seam gives positive 2 degrees"), FMath::IsNearlyEqual(Wrap, FMath::DegreesToRadians(2.f), .001f));
	return true;
}
#endif
