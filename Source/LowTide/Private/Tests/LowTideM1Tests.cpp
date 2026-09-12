#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GroundingPlinthActor.h"
#include "LowTideCharacter.h"
#include "LowTideGameMode.h"
#include "JobBoardActor.h"
#include "LowTideInventoryComponent.h"
#include "PickupActor.h"
#include "TideController.h"
#include "TraderActor.h"
#include "InputCoreTypes.h"

namespace
{
class FM1TestWorld
{
public:
    FM1TestWorld()
    {
        CachedFrameCounter = GFrameCounter;
        Context = &GEngine->CreateNewWorldContext(EWorldType::Game);
        World = UWorld::CreateWorld(EWorldType::Game, false,
            MakeUniqueObjectName(nullptr, UWorld::StaticClass(), NAME_None), GetTransientPackage());
        World->AddToRoot();
        Context->SetCurrentWorld(World);
        GameInstance = NewObject<UGameInstance>();
        GameInstance->AddToRoot();
        Context->OwningGameInstance = GameInstance;
        World->SetGameInstance(GameInstance);
        GameInstance->Init();

        FURL Url;
        Url.AddOption(TEXT("game=/Script/LowTide.LowTideGameMode"));
        World->SetGameMode(Url);
        World->InitializeActorsForPlay(Url);
        World->BeginPlay();

        Controller = World->SpawnActor<APlayerController>();
        Character = World->SpawnActor<ALowTideCharacter>(FVector::ZeroVector, FRotator::ZeroRotator);
        if (Controller && Character)
        {
            Controller->SetPlayer(NewObject<ULocalPlayer>(GEngine));
            Controller->Possess(Character);
        }
    }

    ~FM1TestWorld()
    {
        if (GameInstance)
        {
            GameInstance->Shutdown();
            GameInstance->RemoveFromRoot();
        }
        if (World)
        {
            World->BeginTearingDown();
            World->EndPlay(EEndPlayReason::LevelTransition);
            GFrameCounter = CachedFrameCounter;
            GEngine->ShutdownWorldNetDriver(World);
            World->DestroyWorld(true);
            World->SetPhysicsScene(nullptr);
            GEngine->DestroyWorldContext(World);
            World->RemoveFromRoot();
        }
    }

    void Tick(float DeltaSeconds)
    {
        World->Tick(LEVELTICK_All, DeltaSeconds);
        ++GFrameCounter;
    }

    UWorld* World = nullptr;
    APlayerController* Controller = nullptr;
    ALowTideCharacter* Character = nullptr;

private:
    uint64 CachedFrameCounter = 0;
    FWorldContext* Context = nullptr;
    UGameInstance* GameInstance = nullptr;
};

APickupActor* FindPickup(UWorld* World, FName ItemId)
{
    for (TActorIterator<APickupActor> It(World); It; ++It)
    {
        if (It->GetItemId() == ItemId)
        {
            return *It;
        }
    }
    return nullptr;
}

template <typename T>
T* FindM1Actor(UWorld* World)
{
    for (TActorIterator<T> It(World); It; ++It)
    {
        return *It;
    }
    return nullptr;
}

bool Collect(ALowTideCharacter* Character, APickupActor* Pickup)
{
    if (!Character || !Pickup)
    {
        return false;
    }
    Character->SetActorLocation(Pickup->GetActorLocation());
    return Pickup->Interact(Character);
}

bool ExecuteActionBinding(UInputComponent* InputComponent, FName ActionName, EInputEvent Event, const FKey& Key)
{
    if (!InputComponent)
    {
        return false;
    }
    for (int32 Index = 0; Index < InputComponent->GetNumActionBindings(); ++Index)
    {
        FInputActionBinding& Binding = InputComponent->GetActionBinding(Index);
        if (Binding.GetActionName() == ActionName && Binding.KeyEvent == Event)
        {
            Binding.ActionDelegate.Execute(Key);
            return true;
        }
    }
    return false;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLowTideM1MissionTest,
    "LowTide.M1.Mission.UniqueRewardAndRareChoice",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLowTideM1MissionTest::RunTest(const FString& Parameters)
{
    FM1TestWorld TestWorld;
    ALowTideGameMode* GameMode = TestWorld.World ? TestWorld.World->GetAuthGameMode<ALowTideGameMode>() : nullptr;
    TestNotNull(TEXT("M1 game mode exists"), GameMode);
    TestNotNull(TEXT("M1 player exists"), TestWorld.Character);
    if (!GameMode || !TestWorld.Character)
    {
        return false;
    }
    TestFalse(TEXT("Default launch is the M1 slice"), GameMode->IsM05Fixture());
    ATraderActor* Mara = GameMode->GetSceneLayout().Mara;
    AJobBoardActor* JobBoard = GameMode->GetJobBoard();
    TestNotNull(TEXT("Mara exists"), Mara);
    TestNotNull(TEXT("Job board exists"), JobBoard);
    if (!Mara || !JobBoard)
    {
        return false;
    }

    TestTrue(TEXT("Job board offers the current expedition and reward"),
        JobBoard->GetInteractionPrompt(TestWorld.Character).Contains(TEXT("Signal Station Logbook - 75 credits")));
    TestWorld.Character->SetActorLocation(JobBoard->GetActorLocation() + FVector(600.0f, 0.0f, 0.0f));
    TestFalse(TEXT("Job board rejects out-of-range interaction"), JobBoard->Interact(TestWorld.Character));

    TestWorld.Character->SetActorLocation(Mara->GetActorLocation());
    TestTrue(TEXT("Mara remains usable before mission acceptance"), Mara->Interact(TestWorld.Character));
    TestEqual(TEXT("Mara does not accept jobs"), GameMode->GetMissionState(), EM1MissionState::NotAccepted);
    TestWorld.Character->SetActorLocation(JobBoard->GetActorLocation());
    TestTrue(TEXT("Job board interaction accepts the mission"), JobBoard->Interact(TestWorld.Character));
    TestEqual(TEXT("Mission enters logbook objective"), GameMode->GetMissionState(), EM1MissionState::FindLogbook);
    TestTrue(TEXT("Tide clock starts only after acceptance"), GameMode->GetTideController()->IsClockRunning());
    GameMode->GetTideController()->Tick(1.0f);
    const float RemainingAfterAcceptance = GameMode->GetTideController()->GetSecondsRemaining();
    TestTrue(TEXT("Active job remains reviewable at the board"), JobBoard->Interact(TestWorld.Character));
    TestEqual(TEXT("Repeated board use does not restart the tide clock"),
        GameMode->GetTideController()->GetSecondsRemaining(), RemainingAfterAcceptance);

    APickupActor* Logbook = FindPickup(TestWorld.World, TEXT("signal_station_logbook"));
    APickupActor* Artifact = FindPickup(TestWorld.World, TEXT("singing_shard"));
    TestNotNull(TEXT("Mission logbook spawns"), Logbook);
    TestNotNull(TEXT("Optional rare artifact spawns"), Artifact);

    const auto VisibilityHits = [this, &TestWorld](const FString& Label, AActor* Target, const FVector& ApproachDirection, float AimHeight)
    {
        if (!Target)
        {
            TestTrue(Label, false);
            return;
        }
        const FVector AimPoint = Target->GetActorLocation() + FVector(0.0f, 0.0f, AimHeight);
        const FVector Start = AimPoint - ApproachDirection.GetSafeNormal2D() * 400.0f + FVector(0.0f, 0.0f, 20.0f);
        FHitResult Hit;
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(M1CoreInteractionVisibility), false, TestWorld.Character);
        const bool bHit = TestWorld.World->LineTraceSingleByChannel(Hit, Start, AimPoint, ECC_Visibility, QueryParams);
        TestTrue(Label, bHit && Hit.GetActor() == Target);
    };
    VisibilityHits(TEXT("Mara is visible to an interaction ray from her walkable approach"), Mara,
        Mara->GetActorLocation() - GameMode->GetSceneLayout().PlayerStart, 60.0f);
    const FVector LogbookApproach = GameMode->GetSceneLayout().MainObjectiveLocation - GameMode->GetSceneLayout().RouteWaypoints.Last(1);
    VisibilityHits(TEXT("Main objective is visible to an interaction ray from the station approach"), Logbook, LogbookApproach, 10.0f);
    const TArray<FVector>& OptionalRoute = GameMode->GetSceneLayout().OptionalRouteWaypoints;
    const FVector ArtifactApproach = GameMode->GetSceneLayout().RareArtifactLocation - OptionalRoute[OptionalRoute.Num() - 2];
    VisibilityHits(TEXT("Rare artifact is visible to an interaction ray from the shrine approach"), Artifact, ArtifactApproach, 10.0f);

    TestTrue(TEXT("Logbook pickup succeeds"), Collect(TestWorld.Character, Logbook));
    TestEqual(TEXT("Logbook reveals return objective"), GameMode->GetMissionState(), EM1MissionState::ReturnToMara);
    TestTrue(TEXT("Story reveal is explicit"), GameMode->GetObjectiveText().Contains(TEXT("before it was sent")));
    TestTrue(TEXT("Return objective presents the unclaimed optional find"), GameMode->GetObjectiveText().Contains(TEXT("Optional")));
    TestTrue(TEXT("Rare artifact can be retained in the pack"), Collect(TestWorld.Character, Artifact));
    TestTrue(TEXT("Carrying rare artifact activates watcher"), GameMode->IsPhenomenonActive());
    TestFalse(TEXT("Carried optional find is no longer offered as unclaimed"), GameMode->GetObjectiveText().Contains(TEXT("Optional")));
    TestTrue(TEXT("Rare pickup feedback preserves value and reaction"), TestWorld.Character->GetFeedback().Contains(TEXT("180"))
        && TestWorld.Character->GetFeedback().Contains(TEXT("reacted")));

    TestWorld.Character->SetActorLocation(Mara->GetActorLocation());
    const int32 CreditsBeforeReward = TestWorld.Character->GetInventory()->GetCredits();
    TestTrue(TEXT("Mara resolves the mission"), Mara->Interact(TestWorld.Character));
    TestEqual(TEXT("Mission completes"), GameMode->GetMissionState(), EM1MissionState::Complete);
    TestEqual(TEXT("Mission pays exact reward once"), TestWorld.Character->GetInventory()->GetCredits(), CreditsBeforeReward + 75);
    TestTrue(TEXT("Second Mara interaction opens ordinary trading"), Mara->Interact(TestWorld.Character));
    TestEqual(TEXT("Repeated resolution cannot duplicate reward"), TestWorld.Character->GetInventory()->GetCredits(), CreditsBeforeReward + 75);
    TestTrue(TEXT("Rare artifact sells through ordinary slot transaction"), Mara->TrySellSlot(TestWorld.Character, 6));
    TestEqual(TEXT("Rare sale pays its catalog value"), TestWorld.Character->GetInventory()->GetCredits(), CreditsBeforeReward + 75 + 180);
    TestFalse(TEXT("Selling rare artifact ends pursuit"), GameMode->IsPhenomenonActive());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLowTideM1LivingTideTest,
    "LowTide.M1.Expedition.LivingTideAndAlternateRoute",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLowTideM1LivingTideTest::RunTest(const FString& Parameters)
{
    FM1TestWorld TestWorld;
    ALowTideGameMode* GameMode = TestWorld.World ? TestWorld.World->GetAuthGameMode<ALowTideGameMode>() : nullptr;
    if (!GameMode || !TestWorld.Character)
    {
        AddError(TEXT("M1 route fixture failed to initialize."));
        return false;
    }
    const FCoastalSceneLayout& Layout = GameMode->GetSceneLayout();
    TestTrue(TEXT("Authored main loop supports multi-minute travel"), Layout.MainRouteLengthCm >= 70000.0f && Layout.MainRouteLengthCm <= 110000.0f);
    TestTrue(TEXT("Optional detour has meaningful additional distance"), Layout.OptionalRouteLengthCm >= 30000.0f && Layout.OptionalRouteLengthCm <= 50000.0f);
    TestTrue(TEXT("Alternate route is authored"), Layout.AlternateRouteWaypoints.Num() >= 5);

    ULowTideInventoryComponent* Inventory = TestWorld.Character->GetInventory();
    FString InventoryReason;
    TestTrue(TEXT("Wet-exposure fixture banks prior ordinary stock"), Inventory->TryAdd(TEXT("scrap_metal"), 1, InventoryReason));
    TestTrue(TEXT("Wet-exposure fixture banks protected evidence"), Inventory->TryAddProtected(TEXT("signal_station_logbook"), 1, InventoryReason));
    Inventory->AddCredits(19);

    AJobBoardActor* JobBoard = GameMode->GetJobBoard();
    TestWorld.Character->SetActorLocation(JobBoard->GetActorLocation());
    JobBoard->Interact(TestWorld.Character);
    ATideController* Tide = GameMode->GetTideController();
    TestEqual(TEXT("Mission begins during comfortable low tide"), Tide->GetPhase(), ETidePhase::Low);
    TestTrue(TEXT("Lower shortcut begins open"), Tide->IsAccessOpen());

    const auto PlaceOnWalkableGround = [this, &TestWorld](const FString& Label, const FVector& Anchor)
    {
        FHitResult GroundHit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(M1TraversalGround), false, TestWorld.Character);
        const bool bGround = TestWorld.World->LineTraceSingleByChannel(GroundHit,
            Anchor + FVector(0.0f, 0.0f, 1000.0f), Anchor - FVector(0.0f, 0.0f, 1000.0f), ECC_Pawn, Params);
        if (!bGround)
        {
            AddError(FString::Printf(TEXT("%s has no walkable support near %s"), *Label, *Anchor.ToString()));
            return false;
        }
        UCharacterMovementComponent* Movement = TestWorld.Character->GetCharacterMovement();
        Movement->SetMovementMode(MOVE_Walking);
        Movement->StopMovementImmediately();
        TestWorld.Character->SetActorLocation(GroundHit.ImpactPoint
            + FVector(0.0f, 0.0f, TestWorld.Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 3.0f));
        for (int32 Frame = 0; Frame < 10; ++Frame)
        {
            TestWorld.Tick(0.05f);
        }
        return true;
    };

    const auto WalkWaypoints = [this, &TestWorld, Tide](const FString& RouteName, const TArray<FVector>& Points, bool bReverse, bool bSprint)
    {
        if (bSprint && !ExecuteActionBinding(TestWorld.Character->InputComponent, TEXT("Sprint"), IE_Pressed, EKeys::LeftShift))
        {
            AddError(RouteName + TEXT(" has no sprint pressed binding"));
            return false;
        }
        const int32 Start = bReverse ? Points.Num() - 2 : 1;
        const int32 End = bReverse ? -1 : Points.Num();
        const int32 Step = bReverse ? -1 : 1;
        for (int32 Index = Start; Index != End; Index += Step)
        {
            const FVector Target = Points[Index];
            int32 Frames = 0;
            int32 StalledFrames = 0;
            while (FVector::Dist2D(TestWorld.Character->GetActorLocation(), Target) > 180.0f && Frames++ < 1800)
            {
                const FVector Before = TestWorld.Character->GetActorLocation();
                const FVector Direction = (Target - Before).GetSafeNormal2D();
                TestWorld.Character->AddMovementInput(Direction, 1.0f);
                TestWorld.Tick(0.05f);
                const float Moved = FVector::Dist2D(TestWorld.Character->GetActorLocation(), Before);
                StalledFrames = Moved < 0.5f ? StalledFrames + 1 : 0;
                if (StalledFrames >= 20)
                {
                    FHitResult FloorHit;
                    const FVector Current = TestWorld.Character->GetActorLocation();
                    FCollisionQueryParams Params(SCENE_QUERY_STAT(M1TraversalDiagnostic), false, TestWorld.Character);
                    TestWorld.World->LineTraceSingleByChannel(FloorHit, Current + FVector(0.0f, 0.0f, 500.0f),
                        Current - FVector(0.0f, 0.0f, 500.0f), ECC_Pawn, Params);
                    AddError(FString::Printf(TEXT("%s stalled for %.1fs before waypoint %d after %.1fs: target %s, actual %s, mode %d, support %s/%s, tide %s water %.0f"),
                        *RouteName, StalledFrames * 0.05f, Index, Frames * 0.05f,
                        *Target.ToString(), *Current.ToString(),
                        static_cast<int32>(TestWorld.Character->GetCharacterMovement()->MovementMode),
                        FloorHit.GetActor() ? *FloorHit.GetActor()->GetName() : TEXT("none"),
                        FloorHit.GetComponent() ? *FloorHit.GetComponent()->GetName() : TEXT("none"),
                        *Tide->GetPhaseName(), Tide->GetWaterSurfaceZ()));
                    if (bSprint)
                    {
                        ExecuteActionBinding(TestWorld.Character->InputComponent, TEXT("Sprint"), IE_Released, EKeys::LeftShift);
                    }
                    return false;
                }
            }
            if (Frames >= 1800)
            {
                AddError(FString::Printf(TEXT("%s timed out before waypoint %d after %.1fs: target %s, actual %s, mode %d, tide %s water %.0f"),
                    *RouteName, Index, Frames * 0.05f, *Target.ToString(), *TestWorld.Character->GetActorLocation().ToString(),
                    static_cast<int32>(TestWorld.Character->GetCharacterMovement()->MovementMode),
                    *Tide->GetPhaseName(), Tide->GetWaterSurfaceZ()));
                if (bSprint)
                {
                    ExecuteActionBinding(TestWorld.Character->InputComponent, TEXT("Sprint"), IE_Released, EKeys::LeftShift);
                }
                return false;
            }
        }
        if (bSprint)
        {
            ExecuteActionBinding(TestWorld.Character->InputComponent, TEXT("Sprint"), IE_Released, EKeys::LeftShift);
        }
        return true;
    };

    // Exercise normal CharacterMovement walking, acceleration, gravity, slopes and collision along all routes.
    TestTrue(TEXT("Main route starts on walkable ground"), PlaceOnWalkableGround(TEXT("main route start"), Layout.RouteWaypoints[0]));
    TestTrue(TEXT("Walking CharacterMovement traverses every winding main-route segment without jumping"), WalkWaypoints(TEXT("main route walking"), Layout.RouteWaypoints, false, false));
    TestTrue(TEXT("Sprint main route starts on walkable ground"), PlaceOnWalkableGround(TEXT("sprint main route start"), Layout.RouteWaypoints[0]));
    TestTrue(TEXT("Sprinting CharacterMovement traverses every main-route segment without jumping"), WalkWaypoints(TEXT("main route sprint"), Layout.RouteWaypoints, false, true));
    const TArray<FVector> EntranceRoute = { Layout.RouteWaypoints[0], Layout.RouteWaypoints[1], Layout.RouteWaypoints[2] };
    TestTrue(TEXT("Entrance return landing starts on walkable ground"), PlaceOnWalkableGround(TEXT("entrance return walking start"), EntranceRoute.Last()));
    TestTrue(TEXT("Walking returns from the new entrance terrain to the original settlement floor without jumping"), WalkWaypoints(TEXT("entrance return walking"), EntranceRoute, true, false));
    TestTrue(TEXT("Sprint entrance return landing starts on walkable ground"), PlaceOnWalkableGround(TEXT("entrance return sprint start"), EntranceRoute.Last()));
    TestTrue(TEXT("Sprinting returns from the new entrance terrain to the original settlement floor without jumping"), WalkWaypoints(TEXT("entrance return sprint"), EntranceRoute, true, true));
    TestTrue(TEXT("Optional route starts on walkable ground"), PlaceOnWalkableGround(TEXT("optional route start"), Layout.OptionalRouteWaypoints[0]));
    TestTrue(TEXT("Walking CharacterMovement traverses every optional-risk segment outward without jumping"), WalkWaypoints(TEXT("optional outward walking"), Layout.OptionalRouteWaypoints, false, false));
    TestTrue(TEXT("Sprint optional route starts on walkable ground"), PlaceOnWalkableGround(TEXT("sprint optional route start"), Layout.OptionalRouteWaypoints[0]));
    TestTrue(TEXT("Sprinting CharacterMovement traverses every optional-risk segment outward without jumping"), WalkWaypoints(TEXT("optional outward sprint"), Layout.OptionalRouteWaypoints, false, true));
    TestTrue(TEXT("Optional return starts on walkable ground"), PlaceOnWalkableGround(TEXT("optional return start"), Layout.OptionalRouteWaypoints.Last()));
    TestTrue(TEXT("Walking CharacterMovement traverses the optional-risk route back to its joint without jumping"), WalkWaypoints(TEXT("optional return walking"), Layout.OptionalRouteWaypoints, true, false));
    TestTrue(TEXT("Sprint optional return starts on walkable ground"), PlaceOnWalkableGround(TEXT("sprint optional return start"), Layout.OptionalRouteWaypoints.Last()));
    TestTrue(TEXT("Sprinting CharacterMovement traverses the optional-risk route back to its joint without jumping"), WalkWaypoints(TEXT("optional return sprint"), Layout.OptionalRouteWaypoints, true, true));
    TestEqual(TEXT("Normal main traversal remains inside low phase"), Tide->GetPhase(), ETidePhase::Low);
    TestTrue(TEXT("Low water visibly sits below the lower route"), Tide->GetWaterSurfaceZ() < Layout.RouteWaypoints[9].Z);

    Tide->Tick(Tide->GetSecondsRemaining() + 0.1f);
    TestEqual(TEXT("Tide enters rising phase"), Tide->GetPhase(), ETidePhase::Rising);
    TestTrue(TEXT("Shortcut remains open for first half of rising tide"), Tide->IsAccessOpen());
    TestWorld.Character->SetActorLocation(Layout.AlternateRouteWaypoints[2] + FVector(0.0f, 0.0f, 100.0f));
    const FVector PositionBeforeClosure = TestWorld.Character->GetActorLocation();
    Tide->Tick(Tide->GetSecondsUntilAccessCloses() + 0.1f);
    TestFalse(TEXT("Rising water closes the lower shortcut"), Tide->IsAccessOpen());
    TestTrue(TEXT("Shortcut closes when visible water reaches its deck"), Tide->GetWaterSurfaceZ() >= -50.0f && Layout.ShortcutBlocker->GetActorEnableCollision());
    TestTrue(TEXT("Shortcut closure does not teleport a player on the ridge"), TestWorld.Character->GetActorLocation().Equals(PositionBeforeClosure, 1.0f));

    TestTrue(TEXT("Blue ridge starts on walkable ground"), PlaceOnWalkableGround(TEXT("blue ridge start"), Layout.AlternateRouteWaypoints[0]));
    TestTrue(TEXT("Walking CharacterMovement traverses the full elevated escape after shortcut closure without jumping"), WalkWaypoints(TEXT("blue ridge walking"), Layout.AlternateRouteWaypoints, false, false));
    TestTrue(TEXT("Sprint blue ridge starts on walkable ground"), PlaceOnWalkableGround(TEXT("sprint blue ridge start"), Layout.AlternateRouteWaypoints[0]));
    TestTrue(TEXT("Sprinting CharacterMovement traverses the full elevated escape after shortcut closure without jumping"), WalkWaypoints(TEXT("blue ridge sprint"), Layout.AlternateRouteWaypoints, false, true));
    TestTrue(TEXT("Elevated escape returns to the settlement"), Layout.SettlementSafeBounds.IsInsideOrOn(TestWorld.Character->GetActorLocation()));

    TestWorld.Character->SetActorLocation(Layout.AlternateRouteWaypoints[2] + FVector(0.0f, 0.0f, 100.0f));
    while (Tide->GetPhase() != ETidePhase::High)
    {
        Tide->Tick(Tide->GetSecondsRemaining() + 0.1f);
    }
    const FVector DryRidgeAtHigh = TestWorld.Character->GetActorLocation();
    TestWorld.Tick(1.0f);
    TestEqual(TEXT("High tide visibly reaches the lower expedition elevation"), Tide->GetWaterSurfaceZ(), 40.0f);
    TestTrue(TEXT("Elevated route remains a safe late fallback at high tide"), FVector::Dist(TestWorld.Character->GetActorLocation(), DryRidgeAtHigh) < 200.0f
        && !Layout.SettlementSafeBounds.IsInsideOrOn(TestWorld.Character->GetActorLocation()));

    // This is distinct from watcher recovery: exposed lower ground must give the player a short, readable window,
    // then remove only salvage acquired after this new expedition started.
    const float CapsuleHalfHeight = TestWorld.Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector WetLowerGround = Layout.RouteWaypoints[9] + FVector(0.0f, 0.0f, CapsuleHalfHeight + 3.0f);
    TestWorld.Character->SetActorLocation(WetLowerGround, false, nullptr, ETeleportType::TeleportPhysics);
    GameMode->Tick(0.1f);
    TestTrue(TEXT("High water over lower ground shows an elevated-route warning"),
        TestWorld.Character->GetFeedback().Contains(TEXT("Water is over your boots"))
            && TestWorld.Character->GetFeedback().Contains(TEXT("BLUE RIDGE")));
    TestTrue(TEXT("New wet-ground expedition salvage is acquired after the prior stock snapshot"),
        Inventory->TryAdd(TEXT("copper_wire"), 1, InventoryReason));
    TestTrue(TEXT("Wet-ground grace test can press jump"), ExecuteActionBinding(TestWorld.Character->InputComponent, TEXT("Jump"), IE_Pressed, EKeys::SpaceBar));
    TestWorld.Tick(0.05f);
    TestTrue(TEXT("Wet-ground grace test is physically airborne after the jump"), TestWorld.Character->GetCharacterMovement()->IsFalling());
    TestTrue(TEXT("Wet-ground grace test can release jump"), ExecuteActionBinding(TestWorld.Character->InputComponent, TEXT("Jump"), IE_Released, EKeys::SpaceBar));
    const FVector PositionDuringWarning = TestWorld.Character->GetActorLocation();
    GameMode->Tick(4.7f);
    TestTrue(TEXT("Less than five seconds of continuous wet exposure does not teleport"),
        TestWorld.Character->GetActorLocation().Equals(PositionDuringWarning, 1.0f)
            && !Layout.SettlementSafeBounds.IsInsideOrOn(TestWorld.Character->GetActorLocation()));
    TestTrue(TEXT("Jumping over wet lower ground does not reset its five-second recovery grace"),
        TestWorld.Character->GetCharacterMovement()->IsFalling());
    GameMode->Tick(0.2f);
    TestTrue(TEXT("Five seconds of continuous wet exposure recovers to settlement"),
        Layout.SettlementSafeBounds.IsInsideOrOn(TestWorld.Character->GetActorLocation()));
    TestEqual(TEXT("Wet recovery retains prior ordinary stock"), Inventory->GetQuantity(TEXT("scrap_metal")), 1);
    TestEqual(TEXT("Wet recovery removes only newly acquired ordinary salvage"), Inventory->GetQuantity(TEXT("copper_wire")), 0);
    TestEqual(TEXT("Wet recovery retains protected evidence"), Inventory->GetQuantity(TEXT("signal_station_logbook")), 1);
    TestEqual(TEXT("Wet recovery retains prior credits"), Inventory->GetCredits(), 19);
    TestTrue(TEXT("Wet recovery explains retained permanent state"), TestWorld.Character->GetFeedback().Contains(TEXT("prior stock, evidence and credits kept")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLowTideM1InvalidGroundRecoveryTest,
    "LowTide.M1.Risk.InvalidGroundRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLowTideM1InvalidGroundRecoveryTest::RunTest(const FString& Parameters)
{
    FM1TestWorld TestWorld;
    ALowTideGameMode* GameMode = TestWorld.World ? TestWorld.World->GetAuthGameMode<ALowTideGameMode>() : nullptr;
    ALowTideCharacter* Character = TestWorld.Character;
    if (!GameMode || !Character)
    {
        AddError(TEXT("Invalid-ground fixture failed to initialize."));
        return false;
    }
    const FCoastalSceneLayout& Layout = GameMode->GetSceneLayout();
    ULowTideInventoryComponent* Inventory = Character->GetInventory();
    FString Reason;
    Character->SetActorLocation(GameMode->GetJobBoard()->GetActorLocation());
    TestTrue(TEXT("Invalid-ground fixture accepts the expedition"), GameMode->GetJobBoard()->Interact(Character));
    TestTrue(TEXT("Invalid-ground fixture adds expedition ordinary salvage"), Inventory->TryAdd(TEXT("copper_wire"), 1, Reason));
    TestTrue(TEXT("Invalid-ground fixture adds protected expedition evidence"), Inventory->TryAddProtected(TEXT("signal_station_logbook"), 1, Reason));
    Inventory->AddCredits(23);

    FHitResult GroundHit;
    const FVector MidRoute = Layout.RouteWaypoints[5];
    FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(M1InvalidGroundCheckpoint), false, Character);
    TestTrue(TEXT("Mid-route checkpoint has dry physical support"), TestWorld.World->LineTraceSingleByChannel(GroundHit,
        MidRoute + FVector(0.0f, 0.0f, 1000.0f), MidRoute - FVector(0.0f, 0.0f, 1000.0f), ECC_Pawn, GroundQuery));
    if (!GroundHit.bBlockingHit)
    {
        return false;
    }
    Character->SetActorLocation(GroundHit.ImpactPoint + FVector(0.0f, 0.0f,
        Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 3.0f), false, nullptr, ETeleportType::TeleportPhysics);
    for (int32 Frame = 0; Frame < 12; ++Frame)
    {
        TestWorld.Tick(0.05f);
    }
    GameMode->Tick(0.1f);
    const FVector Checkpoint = Character->GetActorLocation();
    const int32 SalvageBeforeFall = Inventory->GetQuantity(TEXT("copper_wire"));
    const int32 EvidenceBeforeFall = Inventory->GetQuantity(TEXT("signal_station_logbook"));
    const int32 CreditsBeforeFall = Inventory->GetCredits();
    const EM1MissionState MissionBeforeFall = GameMode->GetMissionState();

    Character->SetActorLocation(FVector(Checkpoint.X, Checkpoint.Y, -1200.0f), false, nullptr, ETeleportType::TeleportPhysics);
    GameMode->Tick(0.1f);
    TestTrue(TEXT("Invalid fall restores the recently supported dry expedition checkpoint"), FVector::Dist(Character->GetActorLocation(), Checkpoint) < 200.0f);
    TestEqual(TEXT("Invalid fall retains current expedition ordinary salvage"), Inventory->GetQuantity(TEXT("copper_wire")), SalvageBeforeFall);
    TestEqual(TEXT("Invalid fall retains protected evidence"), Inventory->GetQuantity(TEXT("signal_station_logbook")), EvidenceBeforeFall);
    TestEqual(TEXT("Invalid fall retains credits"), Inventory->GetCredits(), CreditsBeforeFall);
    TestEqual(TEXT("Invalid fall retains mission progress"), GameMode->GetMissionState(), MissionBeforeFall);
    TestTrue(TEXT("Invalid fall gives clear no-penalty feedback"), Character->GetFeedback().Contains(TEXT("Recovered from invalid ground; all items kept")));

    ATideController* Tide = GameMode->GetTideController();
    while (Tide->GetPhase() != ETidePhase::High)
    {
        Tide->Tick(Tide->GetSecondsRemaining() + 0.1f);
    }
    Character->SetActorLocation(Layout.RouteWaypoints[9] + FVector(0.0f, 0.0f,
        Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 3.0f), false, nullptr, ETeleportType::TeleportPhysics);
    GameMode->Tick(5.1f);
    TestTrue(TEXT("Normal high-water recovery still returns to the settlement"), Layout.SettlementSafeBounds.IsInsideOrOn(Character->GetActorLocation()));
    TestEqual(TEXT("Normal high-water recovery still removes post-snapshot ordinary salvage"), Inventory->GetQuantity(TEXT("copper_wire")), 0);
    TestEqual(TEXT("Normal high-water recovery still retains protected evidence"), Inventory->GetQuantity(TEXT("signal_station_logbook")), EvidenceBeforeFall);
    TestEqual(TEXT("Normal high-water recovery still retains credits"), Inventory->GetCredits(), CreditsBeforeFall);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLowTideM1JumpContainmentTest,
    "LowTide.M1.Traversal.JumpContainment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLowTideM1JumpContainmentTest::RunTest(const FString& Parameters)
{
    FM1TestWorld TestWorld;
    ALowTideGameMode* GameMode = TestWorld.World ? TestWorld.World->GetAuthGameMode<ALowTideGameMode>() : nullptr;
    ALowTideCharacter* Character = TestWorld.Character;
    if (!GameMode || !Character || !GameMode->GetSceneLayout().ShortcutBlocker)
    {
        AddError(TEXT("Jump-containment fixture failed to initialize."));
        return false;
    }
    const FCoastalSceneLayout& Layout = GameMode->GetSceneLayout();
    ATideController* Tide = GameMode->GetTideController();
    Character->SetActorLocation(GameMode->GetJobBoard()->GetActorLocation());
    TestTrue(TEXT("Jump-containment fixture accepts the expedition and starts the tide clock"), GameMode->GetJobBoard()->Interact(Character));
    TestTrue(TEXT("Jump-containment fixture tide clock is running"), Tide->IsClockRunning());
    Tide->Tick(20.1f);
    Tide->Tick(Tide->GetSecondsRemaining() + 0.1f);
    Tide->Tick(Tide->GetSecondsUntilAccessCloses() + 0.1f);
    TestFalse(TEXT("Jump-containment fixture closes the low shortcut"), Tide->IsAccessOpen());

    const auto PlaceOnGround = [&TestWorld, Character](const FVector& Anchor)
    {
        FHitResult Ground;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(M1JumpContainmentGround), false, Character);
        if (!TestWorld.World->LineTraceSingleByChannel(Ground, Anchor + FVector(0.0f, 0.0f, 1000.0f),
            Anchor - FVector(0.0f, 0.0f, 1000.0f), ECC_Pawn, Query))
        {
            return false;
        }
        Character->SetActorLocation(Ground.ImpactPoint + FVector(0.0f, 0.0f,
            Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 3.0f), false, nullptr, ETeleportType::TeleportPhysics);
        Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        Character->GetCharacterMovement()->StopMovementImmediately();
        for (int32 Frame = 0; Frame < 5; ++Frame)
        {
            TestWorld.Tick(0.05f);
        }
        return true;
    };
    const auto SprintJumpAttempt = [this, &TestWorld, Character](const FString& Label, const FVector& Direction, float MaxAdvance)
    {
        const FVector Start = Character->GetActorLocation();
        TestTrue(Label + TEXT(" starts grounded"), Character->GetCharacterMovement()->IsMovingOnGround());
        TestTrue(Label + TEXT(" starts sprinting"), ExecuteActionBinding(Character->InputComponent, TEXT("Sprint"), IE_Pressed, EKeys::LeftShift));
        TestTrue(Label + TEXT(" sends a jump press"), ExecuteActionBinding(Character->InputComponent, TEXT("Jump"), IE_Pressed, EKeys::SpaceBar));
        float FurthestAdvance = 0.0f;
        for (int32 Frame = 0; Frame < 40; ++Frame)
        {
            Character->AddMovementInput(Direction, 1.0f);
            TestWorld.Tick(0.05f);
            FurthestAdvance = FMath::Max(FurthestAdvance, FVector::DotProduct(Character->GetActorLocation() - Start, Direction));
            if (Frame == 3)
            {
                ExecuteActionBinding(Character->InputComponent, TEXT("Jump"), IE_Released, EKeys::SpaceBar);
            }
        }
        ExecuteActionBinding(Character->InputComponent, TEXT("Sprint"), IE_Released, EKeys::LeftShift);
        TestTrue(Label + TEXT(" never crosses its authored collision limit"), FurthestAdvance < MaxAdvance);
        TestFalse(Label + TEXT(" is blocked rather than masked by any recovery"), Character->GetFeedback().Contains(TEXT("Recovered")));
    };
    const auto ClearWetExposure = [&TestWorld, Character, &Layout]()
    {
        Character->SetActorLocation(Layout.PlayerStart, false, nullptr, ETeleportType::TeleportPhysics);
        TestWorld.Tick(0.1f);
    };

    const FVector ShortcutDirection = (Layout.RouteWaypoints[8] - Layout.RouteWaypoints[7]).GetSafeNormal2D();
    const FVector ShortcutAcross(-ShortcutDirection.Y, ShortcutDirection.X, 0.0f);
    const FVector BlockerCenter = Layout.ShortcutBlocker->GetActorLocation();
    ClearWetExposure();
    TestTrue(TEXT("Closed shortcut center approach has walkable support"), PlaceOnGround(BlockerCenter - ShortcutDirection * 250.0f));
    SprintJumpAttempt(TEXT("Sprint jump at closed shortcut center"), ShortcutDirection, 210.0f);
    ClearWetExposure();
    TestTrue(TEXT("Closed shortcut end approach has walkable support"), PlaceOnGround(BlockerCenter - ShortcutDirection * 250.0f + ShortcutAcross * 390.0f));
    SprintJumpAttempt(TEXT("Sprint jump at closed shortcut near its end"), ShortcutDirection, 280.0f);

    ClearWetExposure();
    TestTrue(TEXT("Settlement boundary approach has walkable support"), PlaceOnGround(FVector(2600.0f, 0.0f, 0.0f)));
    SprintJumpAttempt(TEXT("Sprint jump at settlement boundary"), FVector::ForwardVector, 500.0f);
    const FVector OptionalStart = (Layout.OptionalRouteWaypoints[3] + Layout.OptionalRouteWaypoints[4]) * 0.5f;
    const FVector OptionalDirection = (Layout.OptionalRouteWaypoints[4] - Layout.OptionalRouteWaypoints[3]).GetSafeNormal2D();
    const FVector OptionalOutward(-OptionalDirection.Y, OptionalDirection.X, 0.0f);
    ClearWetExposure();
    TestTrue(TEXT("Optional branch boundary approach has walkable support"), PlaceOnGround(OptionalStart + OptionalOutward * 240.0f));
    SprintJumpAttempt(TEXT("Sprint jump at optional branch boundary"), OptionalOutward, 500.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLowTideM1PhenomenonRecoveryTest,
    "LowTide.M1.Risk.PhenomenonRecoveryAndSecondTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLowTideM1PhenomenonRecoveryTest::RunTest(const FString& Parameters)
{
    FM1TestWorld TestWorld;
    ALowTideGameMode* GameMode = TestWorld.World ? TestWorld.World->GetAuthGameMode<ALowTideGameMode>() : nullptr;
    if (!GameMode || !TestWorld.Character)
    {
        AddError(TEXT("M1 phenomenon fixture failed to initialize."));
        return false;
    }
    ALowTideCharacter* Character = TestWorld.Character;
    ULowTideInventoryComponent* Inventory = Character->GetInventory();
    FString Reason;
    TestTrue(TEXT("Prior ordinary stock exists"), Inventory->TryAdd(TEXT("scrap_metal"), 1, Reason));
    Inventory->AddCredits(12);

    AJobBoardActor* JobBoard = GameMode->GetJobBoard();
    Character->SetActorLocation(JobBoard->GetActorLocation());
    JobBoard->Interact(Character);
    TestTrue(TEXT("Current expedition ordinary salvage added"), Inventory->TryAdd(TEXT("scrap_metal"), 1, Reason));
    TestTrue(TEXT("Protected mission objective collected"), Collect(Character, FindPickup(TestWorld.World, TEXT("signal_station_logbook"))));
    TestTrue(TEXT("Rare artifact collected"), Collect(Character, FindPickup(TestWorld.World, TEXT("singing_shard"))));

    AActor* Watcher = GameMode->GetPhenomenonActor();
    TestNotNull(TEXT("Visible watcher actor exists"), Watcher);
    if (!Watcher)
    {
        return false;
    }
    Character->SetActorLocation(GameMode->GetSceneLayout().OptionalRouteWaypoints.Last());
    Watcher->SetActorLocation(Character->GetActorLocation() + FVector(1200.0f, 0.0f, 0.0f));
    Character->GetCharacterMovement()->Velocity = FVector::ZeroVector;
    const FVector StoppedWatcherLocation = Watcher->GetActorLocation();
    GameMode->Tick(1.0f);
    TestTrue(TEXT("Watcher stops when artifact carrier stands still"), Watcher->GetActorLocation().Equals(StoppedWatcherLocation, 1.0f));

    Character->GetCharacterMovement()->Velocity = FVector(500.0f, 0.0f, 0.0f);
    const float BeforeMoveDistance = FVector::Dist2D(Watcher->GetActorLocation(), Character->GetActorLocation());
    GameMode->Tick(0.5f);
    TestTrue(TEXT("Watcher advances while artifact carrier moves"), FVector::Dist2D(Watcher->GetActorLocation(), Character->GetActorLocation()) < BeforeMoveDistance);

    AGroundingPlinthActor* Plinth = FindM1Actor<AGroundingPlinthActor>(TestWorld.World);
    TestNotNull(TEXT("Grounding ward exists"), Plinth);
    Character->SetActorLocation(Plinth->GetActorLocation());
    TestTrue(TEXT("Player can relinquish the artifact at a ward"), Plinth && Plinth->Interact(Character));
    TestEqual(TEXT("Grounding removes exactly the artifact"), Inventory->GetQuantity(TEXT("singing_shard")), 0);
    TestFalse(TEXT("Grounding ends pursuit"), GameMode->IsPhenomenonActive());
    TestFalse(TEXT("Repeated grounding cannot duplicate or underflow"), Plinth && Plinth->Interact(Character));

    // Reactivate through the same public acquisition path to exercise catch recovery in this bounded fixture.
    TestTrue(TEXT("Catch fixture carries an artifact"), Inventory->TryAdd(TEXT("singing_shard"), 1, Reason));
    GameMode->NotifyItemCollected(Character, TEXT("singing_shard"));
    Character->SetActorLocation(GameMode->GetSceneLayout().RareArtifactLocation);
    Watcher->SetActorLocation(Character->GetActorLocation() + FVector(100.0f, 0.0f, 0.0f));
    Character->GetCharacterMovement()->Velocity = FVector::ZeroVector;
    GameMode->Tick(0.1f);
    TestTrue(TEXT("Watcher catch recovers player to settlement"), GameMode->GetSceneLayout().SettlementSafeBounds.IsInsideOrOn(Character->GetActorLocation()));
    TestEqual(TEXT("Catch preserves prior ordinary stock"), Inventory->GetQuantity(TEXT("scrap_metal")), 1);
    TestEqual(TEXT("Catch preserves protected logbook evidence"), Inventory->GetQuantity(TEXT("signal_station_logbook")), 1);
    TestEqual(TEXT("Catch preserves prior credits"), Inventory->GetCredits(), 12);
    TestEqual(TEXT("Catch loses expedition artifact"), Inventory->GetQuantity(TEXT("singing_shard")), 0);
    TestFalse(TEXT("Recovery prevents immediate repeated catch"), GameMode->IsPhenomenonActive());

    ATideController* Tide = GameMode->GetTideController();
    Tide->Tick(Tide->GetSecondsRemaining() + 0.1f);
    Tide->Tick(Tide->GetSecondsRemaining() + 0.1f);
    Tide->Tick(Tide->GetSecondsRemaining() + 0.1f);
    Tide->Tick(Tide->GetSecondsRemaining() + 0.1f);
    APickupActor* SecondTripPickup = FindPickup(TestWorld.World, TEXT("copper_wire"));
    TestNotNull(TEXT("A later low tide replenishes ordinary salvage"), SecondTripPickup);
    TestTrue(TEXT("Second trip can collect salvage"), Collect(Character, SecondTripPickup));
    Character->SetActorLocation(GameMode->GetSceneLayout().PlayerStart);
    GameMode->Tick(0.1f);
    TestEqual(TEXT("Safe second return banks new salvage"), Inventory->GetQuantity(TEXT("copper_wire")), 1);

    // The newly walkable outer berth belongs to the base, even beyond the old settlement box.
    const FVector BerthLocation(3575.0f, 350.0f, 225.0f);
    TestTrue(TEXT("Outer berth is covered by the narrow dock safe area"),
        GameMode->GetSceneLayout().DockSafeBounds.IsInsideOrOn(BerthLocation));
    TestTrue(TEXT("Dock safety fixture carries a shard"), Inventory->TryAdd(TEXT("singing_shard"), 1, Reason));
    GameMode->NotifyItemCollected(Character, TEXT("singing_shard"));
    Character->SetActorLocation(BerthLocation);
    Watcher->SetActorLocation(BerthLocation + FVector(100.0f, 0.0f, 0.0f));
    Character->GetCharacterMovement()->Velocity = FVector::ZeroVector;
    GameMode->Tick(0.1f);
    TestTrue(TEXT("Watcher stays hidden at the working dock"), Watcher->IsHidden());
    TestTrue(TEXT("Approaching the crane does not force a recovery"), Character->GetActorLocation().Equals(BerthLocation, 1.0f));
    TestEqual(TEXT("Dock safety retains carried salvage"), Inventory->GetQuantity(TEXT("singing_shard")), 1);
    return true;
}

#endif
