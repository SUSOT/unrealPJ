#include "World/ShowcaseEscapeProgress.h"
#include "Misc/AutomationTest.h"
#include "World/ShowcaseLoopDirector.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseSequentialLightsTest, "UnrealTeamPJ.Showcase.SequentialLights", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShowcaseSequentialLightsTest::RunTest(const FString& Parameters)
{
	// New event values are appended so existing saved enum values stay intact.
	TestEqual(TEXT("Sequential blackout holds after reaching the player-side lamp"),
		AShowcaseLoopDirector::GetHorrorEventDuration(EShowcaseHorrorEvent::SequentialBlackout, 1), 1.1f);
	TestEqual(TEXT("Sequential red lighting holds before restoring the original color"),
		AShowcaseLoopDirector::GetHorrorEventDuration(EShowcaseHorrorEvent::SequentialRedPulse, 1), .55f);
	const TArray<int32> Order = AShowcaseLoopDirector::GetFarToNearLampOrder({-1.f, 2400.f, 6000.f, 1200.f, 6000.f, 0.f});
	TestEqual(TEXT("Only valid forward lamps enter the sequence"), Order.Num(), 4);
	if (Order.Num() == 4)
	{
		TestEqual(TEXT("The farthest lamp changes first"), Order[0], 2);
		TestEqual(TEXT("Equidistant lamps have stable ordering"), Order[1], 4);
		TestEqual(TEXT("The next nearer lamp changes third"), Order[2], 1);
		TestEqual(TEXT("The player-side lamp changes last"), Order[3], 3);
	}
	TestEqual(TEXT("The farthest lamp changes immediately"), AShowcaseLoopDirector::GetSequentialLampDelay(0), 0.f);
	TestEqual(TEXT("The second lamp changes after 0.2 seconds"), AShowcaseLoopDirector::GetSequentialLampDelay(1), .2f);
	TestEqual(TEXT("The third lamp changes after 0.4 seconds"), AShowcaseLoopDirector::GetSequentialLampDelay(2), .4f);
	TestEqual(TEXT("The last of twelve lamps changes after 2.2 seconds"), AShowcaseLoopDirector::GetSequentialLampDelay(11), 2.2f);
	TestEqual(TEXT("No lamps produces an empty wave"), AShowcaseLoopDirector::GetFarToNearLampOrder({}).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShowcaseRandomVisualEventsTest, "UnrealTeamPJ.Showcase.RandomVisualEvents", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShowcaseRandomVisualEventsTest::RunTest(const FString& Parameters)
{
	const FVector2D Quiet=AShowcaseLoopDirector::GetHorrorEventIntervalRange(0);
	const FVector2D Early=AShowcaseLoopDirector::GetHorrorEventIntervalRange(1);
	const FVector2D Middle=AShowcaseLoopDirector::GetHorrorEventIntervalRange(2);
	const FVector2D Final=AShowcaseLoopDirector::GetHorrorEventIntervalRange(3);
	TestTrue(TEXT("No random horror event before the first threshold"),Quiet.X>10000.f && Quiet.Y>Quiet.X);
	TestTrue(TEXT("Random events accelerate with forward progress"),Early.X>Middle.X && Middle.X>Final.X && Early.Y>Middle.Y && Middle.Y>Final.Y);
	TestEqual(TEXT("One prop changes in the early stage"),AShowcaseLoopDirector::GetHorrorPropChangeCount(1),1);
	TestEqual(TEXT("Two props change in the middle stage"),AShowcaseLoopDirector::GetHorrorPropChangeCount(2),2);
	TestEqual(TEXT("Four props change near the escape threshold"),AShowcaseLoopDirector::GetHorrorPropChangeCount(3),4);
	TestTrue(TEXT("Late blackouts last longer"),
		AShowcaseLoopDirector::GetHorrorEventDuration(EShowcaseHorrorEvent::ForwardBlackout,3)
		> AShowcaseLoopDirector::GetHorrorEventDuration(EShowcaseHorrorEvent::ForwardBlackout,1));
	TestFalse(TEXT("Signs do not reverse before the door exists"),AShowcaseLoopDirector::ShouldReverseExitSigns(3,false));
	TestTrue(TEXT("Signs reverse when the turn-back exit is revealed"),AShowcaseLoopDirector::ShouldReverseExitSigns(3,true));
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
