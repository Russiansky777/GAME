#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/InputComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "LowTideCharacter.h"
#include "LowTideGameMode.h"
#include "LowTideInventoryComponent.h"
#include "PickupActor.h"
#include "TideController.h"
#include "TraderActor.h"

namespace
{
class FLowTideTestWorld
{
public:
    FLowTideTestWorld()
    {
        CachedFrameCounter = GFrameCounter;
        Context = &GEngine->CreateNewWorldContext(EWorldType::Game);
        World = UWorld::CreateWorld(EWorldType::Game, false, MakeUniqueObjectName(nullptr, UWorld::StaticClass(), NAME_None), GetTransientPackage());
        World->AddToRoot();
        Context->SetCurrentWorld(World);

        GameInstance = NewObject<UGameInstance>();
        GameInstance->AddToRoot();
        Context->OwningGameInstance = GameInstance;
        World->SetGameInstance(GameInstance);
        GameInstance->Init();

        FURL Url;
        Url.AddOption(TEXT("game=/Script/LowTide.LowTideGameMode"));
        Url.AddOption(TEXT("M05"));
        World->URL = Url;
        World->SetGameMode(Url);
        World->InitializeActorsForPlay(Url);
        World->BeginPlay();
    }

    ~FLowTideTestWorld()
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

    UWorld* World = nullptr;

private:
    uint64 CachedFrameCounter = 0;
    FWorldContext* Context = nullptr;
    UGameInstance* GameInstance = nullptr;
};

template <typename T>
T* FindActor(UWorld* World)
{
    for (TActorIterator<T> It(World); It; ++It)
    {
        return *It;
    }
    return nullptr;
}

TArray<APickupActor*> FindPickups(UWorld* World)
{
    TArray<APickupActor*> Pickups;
    for (TActorIterator<APickupActor> It(World); It; ++It)
    {
        Pickups.Add(*It);
    }
    return Pickups;
}

bool ExecuteActionBinding(UInputComponent* InputComponent, FName ActionName, EInputEvent Event)
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
            Binding.ActionDelegate.Execute(EKeys::LeftShift);
            return true;
        }
    }
    return false;
}

void TickWorld(UWorld* World, int32 Frames, float DeltaSeconds = 1.0f / 60.0f)
{
    for (int32 Frame = 0; Frame < Frames; ++Frame)
    {
        World->Tick(LEVELTICK_All, DeltaSeconds);
        ++GFrameCounter;
    }
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLowTideTraversalControlsTest,
    "LowTide.M05.Traversal.ControlsAndGrounding",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLowTideTraversalControlsTest::RunTest(const FString& Parameters)
{
    FLowTideTestWorld TestWorld;
    UWorld* World = TestWorld.World;
    APlayerController* Controller = World ? World->SpawnActor<APlayerController>() : nullptr;
    ALowTideCharacter* Character = World ? World->SpawnActor<ALowTideCharacter>(FVector(-300.0f, 0.0f, 190.0f), FRotator::ZeroRotator) : nullptr;
    TestNotNull(TEXT("Traversal fixture controller exists"), Controller);
    TestNotNull(TEXT("Traversal fixture character exists"), Character);
    if (!World || !Controller || !Character)
    {
        return false;
    }
    Controller->SetPlayer(NewObject<ULocalPlayer>(GEngine));
    Controller->Possess(Character);
    TickWorld(World, 90);

    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    TestEqual(TEXT("Shared character walk speed is exploration tuning"), Character->GetWalkSpeed(), 650.0f);
    TestEqual(TEXT("Shared character sprint speed preserves the 60 percent advantage"), Character->GetSprintSpeed(), 1040.0f);
    TestEqual(TEXT("Jump uses a modest initial vertical speed"), Movement->JumpZVelocity, 420.0f);
    TestEqual(TEXT("Character uses the intended heavier gravity"), Movement->GravityScale, 1.3f);
    TestEqual(TEXT("Character has restrained air control"), Movement->AirControl, 0.2f);
    TestEqual(TEXT("Small curbs use the configured automatic step height"), Movement->MaxStepHeight, 45.0f);
    TestEqual(TEXT("Comfortable terrain uses the configured walkable slope"), Movement->GetWalkableFloorAngle(), 45.0f);
    TestEqual(TEXT("Jump count is limited to one"), Character->JumpMaxCount, 1);
    const float GroundZ = Character->GetActorLocation().Z;
    TestTrue(TEXT("Jump pressed binding exists and executes"), ExecuteActionBinding(Character->InputComponent, TEXT("Jump"), IE_Pressed));
    TickWorld(World, 1);
    const float VerticalVelocityAfterFirstJumpFrame = Movement->Velocity.Z;
    TestTrue(TEXT("First jump physically enters the air"), Movement->IsFalling() && Character->JumpCurrentCount == 1);
    TestTrue(TEXT("Jump release binding exists and executes"), ExecuteActionBinding(Character->InputComponent, TEXT("Jump"), IE_Released));
    TestTrue(TEXT("A second jump press is received while airborne"), ExecuteActionBinding(Character->InputComponent, TEXT("Jump"), IE_Pressed));
    TickWorld(World, 1);
    TestTrue(TEXT("Airborne second press cannot reset upward velocity or add a jump"), Character->JumpCurrentCount == 1
        && Movement->Velocity.Z < VerticalVelocityAfterFirstJumpFrame);
    ExecuteActionBinding(Character->InputComponent, TEXT("Jump"), IE_Released);
    float PeakZ = GroundZ;
    for (int32 Frame = 0; Frame < 180; ++Frame)
    {
        TickWorld(World, 1);
        PeakZ = FMath::Max(PeakZ, Character->GetActorLocation().Z);
    }
    TestTrue(TEXT("Jump reaches a natural roughly 69 cm height"), PeakZ - GroundZ >= 50.0f && PeakZ - GroundZ <= 100.0f);
    TestTrue(TEXT("Jump returns to stable walking ground"), Movement->IsMovingOnGround());

    const FVector CurbStart = Character->GetActorLocation();
    AActor* Curb = World->SpawnActor<AActor>();
    UBoxComponent* CurbBox = Curb ? NewObject<UBoxComponent>(Curb, TEXT("TraversalCurb")) : nullptr;
    TestNotNull(TEXT("Physical curb fixture spawns"), CurbBox);
    if (!CurbBox)
    {
        return false;
    }
    Curb->SetRootComponent(CurbBox);
    CurbBox->SetBoxExtent(FVector(25.0f, 250.0f, 20.0f));
    CurbBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CurbBox->SetCollisionResponseToAllChannels(ECR_Block);
    CurbBox->RegisterComponent();
    Curb->SetActorLocation(CurbStart + FVector(160.0f, 0.0f,
        -Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 20.0f));
    for (int32 Frame = 0; Frame < 90; ++Frame)
    {
        Character->AddMovementInput(FVector::ForwardVector, 1.0f);
        TickWorld(World, 1);
    }
    TestTrue(TEXT("CharacterMovement steps over a 40 cm curb without jumping"), Character->GetActorLocation().X > CurbStart.X + 240.0f);
    TestTrue(TEXT("Curb traversal finishes grounded"), Movement->IsMovingOnGround());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLowTideExpeditionRoundTripTest,
    "LowTide.M05.Expedition.RoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLowTideExpeditionRoundTripTest::RunTest(const FString& Parameters)
{
    FLowTideTestWorld TestWorld;
    UWorld* World = TestWorld.World;
    ALowTideGameMode* GameMode = World ? World->GetAuthGameMode<ALowTideGameMode>() : nullptr;
    TestNotNull(TEXT("URL selects LowTideGameMode"), GameMode);
    if (!GameMode)
    {
        return false;
    }

    APlayerController* Controller = World->SpawnActor<APlayerController>();
    ALowTideCharacter* Character = World->SpawnActor<ALowTideCharacter>(FVector(-300.0f, 0.0f, 190.0f), FRotator::ZeroRotator);
    TestNotNull(TEXT("Player controller spawns"), Controller);
    TestNotNull(TEXT("Low Tide character spawns"), Character);
    if (!Controller || !Character)
    {
        return false;
    }
    Controller->SetPlayer(NewObject<ULocalPlayer>(GEngine));
    Controller->Possess(Character);
    TestTrue(TEXT("Fixture has a local player controlling movement"), Character->IsLocallyControlled());

    TestEqual(TEXT("Normal walk speed uses greybox tuning"), Character->GetCharacterMovement()->MaxWalkSpeed, Character->GetWalkSpeed());
    TestTrue(TEXT("Sprint pressed binding exists and executes"), ExecuteActionBinding(Character->InputComponent, TEXT("Sprint"), IE_Pressed));
    TestTrue(TEXT("Sprint remains active while held"), Character->IsSprinting());
    TestEqual(TEXT("Held sprint is 1.6 times walking speed"), Character->GetCharacterMovement()->MaxWalkSpeed, Character->GetWalkSpeed() * 1.6f);
    TestTrue(TEXT("Sprint released binding exists and executes"), ExecuteActionBinding(Character->InputComponent, TEXT("Sprint"), IE_Released));
    TestFalse(TEXT("Sprint ends on release"), Character->IsSprinting());
    TestEqual(TEXT("Release restores normal walk speed"), Character->GetCharacterMovement()->MaxWalkSpeed, Character->GetWalkSpeed());

    TestTrue(TEXT("Catalog preserves the original five definitions and adds M1 finds"), GameMode->GetItemCatalog().GetOrderedItems().Num() >= 7);
    ATideController* Tide = GameMode->GetTideController();
    TestNotNull(TEXT("Tide controller is created by game mode"), Tide);
    if (!Tide)
    {
        return false;
    }

    Tide->Tick(20.1f);
    TestEqual(TEXT("Falling tide reaches low tide"), Tide->GetPhase(), ETidePhase::Low);
    TestTrue(TEXT("Causeway is open at low tide"), Tide->IsAccessOpen());
    TArray<APickupActor*> Pickups = FindPickups(World);
    TestEqual(TEXT("A low cycle spawns five real pickups"), Pickups.Num(), 5);
    if (Pickups.Num() != 5)
    {
        return false;
    }


    int32 BoundaryCount = 0;
    for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
    {
        BoundaryCount += It->ActorHasTag(TEXT("M05Boundary")) ? 1 : 0;
    }
    TestEqual(TEXT("All settlement, causeway and shelf perimeter sections are present"), BoundaryCount, 12);

    const auto BoundaryCapsuleBlocks = [this, World](const FString& Label, const FVector& Start, const FVector& End)
    {
        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(LowTideBoundaryTest), false);
        const bool bHit = World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeCapsule(42.0f, 92.0f), Params);
        TestTrue(Label + TEXT(" blocks a player capsule"), bHit);
        TestTrue(Label + TEXT(" hits the tagged perimeter rather than scene clutter"), bHit && Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("M05Boundary")));
    };
    const auto OpeningIsClear = [this, World](const FString& Label, const FVector& Start, const FVector& End)
    {
        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(LowTideOpeningTest), false);
        TestFalse(Label, World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeCapsule(42.0f, 92.0f), Params));
    };
    constexpr float SweepZ = 210.0f;
    for (const float Y : { -540.0f, 0.0f, 540.0f })
    {
        BoundaryCapsuleBlocks(FString::Printf(TEXT("Settlement west edge y %.0f"), Y), FVector(-640.0f, Y, SweepZ), FVector(-780.0f, Y, SweepZ));
    }
    for (const float X : { -600.0f, 0.0f, 600.0f })
    {
        BoundaryCapsuleBlocks(FString::Printf(TEXT("Settlement north edge x %.0f"), X), FVector(X, 530.0f, SweepZ), FVector(X, 680.0f, SweepZ));
        BoundaryCapsuleBlocks(FString::Printf(TEXT("Settlement south edge x %.0f"), X), FVector(X, -530.0f, SweepZ), FVector(X, -680.0f, SweepZ));
    }
    for (const float Y : { -540.0f, -300.0f, -120.0f, 120.0f, 300.0f, 540.0f })
    {
        BoundaryCapsuleBlocks(FString::Printf(TEXT("Settlement east route wall y %.0f"), Y), FVector(630.0f, Y, SweepZ), FVector(780.0f, Y, SweepZ));
    }
    for (const float X : { 740.0f, 1000.0f, 1400.0f, 1800.0f, 2060.0f })
    {
        BoundaryCapsuleBlocks(FString::Printf(TEXT("Causeway north edge x %.0f"), X), FVector(X, 0.0f, SweepZ), FVector(X, 140.0f, SweepZ));
        BoundaryCapsuleBlocks(FString::Printf(TEXT("Causeway south edge x %.0f"), X), FVector(X, 0.0f, SweepZ), FVector(X, -140.0f, SweepZ));
    }
    for (const float X : { 2160.0f, 2400.0f, 2700.0f, 3000.0f, 3240.0f })
    {
        BoundaryCapsuleBlocks(FString::Printf(TEXT("Shelf north edge x %.0f"), X), FVector(X, 630.0f, SweepZ), FVector(X, 780.0f, SweepZ));
        BoundaryCapsuleBlocks(FString::Printf(TEXT("Shelf south edge x %.0f"), X), FVector(X, -630.0f, SweepZ), FVector(X, -780.0f, SweepZ));
    }
    for (const float Y : { -650.0f, -350.0f, -120.0f, 120.0f, 350.0f, 650.0f })
    {
        BoundaryCapsuleBlocks(FString::Printf(TEXT("Shelf west route wall y %.0f"), Y), FVector(2170.0f, Y, SweepZ), FVector(2020.0f, Y, SweepZ));
        BoundaryCapsuleBlocks(FString::Printf(TEXT("Shelf east edge y %.0f"), Y), FVector(3230.0f, Y, SweepZ), FVector(3380.0f, Y, SweepZ));
    }
    const FVector CornerStarts[] = {
        FVector(-630.0f, -530.0f, SweepZ), FVector(-630.0f, 530.0f, SweepZ),
        FVector(630.0f, -530.0f, SweepZ), FVector(630.0f, 530.0f, SweepZ),
        FVector(2170.0f, -630.0f, SweepZ), FVector(2170.0f, 630.0f, SweepZ),
        FVector(3230.0f, -630.0f, SweepZ), FVector(3230.0f, 630.0f, SweepZ)
    };
    const FVector CornerEnds[] = {
        FVector(-790.0f, -690.0f, SweepZ), FVector(-790.0f, 690.0f, SweepZ),
        FVector(790.0f, -690.0f, SweepZ), FVector(790.0f, 690.0f, SweepZ),
        FVector(2010.0f, -790.0f, SweepZ), FVector(2010.0f, 790.0f, SweepZ),
        FVector(3390.0f, -790.0f, SweepZ), FVector(3390.0f, 790.0f, SweepZ)
    };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(CornerStarts); ++Index)
    {
        BoundaryCapsuleBlocks(FString::Printf(TEXT("Platform outside corner %d"), Index + 1), CornerStarts[Index], CornerEnds[Index]);
    }
    BoundaryCapsuleBlocks(TEXT("Settlement north route join"), FVector(640.0f, 20.0f, SweepZ), FVector(760.0f, 130.0f, SweepZ));
    BoundaryCapsuleBlocks(TEXT("Settlement south route join"), FVector(640.0f, -20.0f, SweepZ), FVector(760.0f, -130.0f, SweepZ));
    BoundaryCapsuleBlocks(TEXT("Shelf north route join"), FVector(2160.0f, 20.0f, SweepZ), FVector(2040.0f, 130.0f, SweepZ));
    BoundaryCapsuleBlocks(TEXT("Shelf south route join"), FVector(2160.0f, -20.0f, SweepZ), FVector(2040.0f, -130.0f, SweepZ));
    OpeningIsClear(TEXT("Settlement-to-causeway opening remains capsule-clear"), FVector(600.0f, 0.0f, SweepZ), FVector(800.0f, 0.0f, SweepZ));
    OpeningIsClear(TEXT("Causeway-to-shelf opening remains capsule-clear"), FVector(2000.0f, 0.0f, SweepZ), FVector(2200.0f, 0.0f, SweepZ));

    Character->SetActorLocation(FVector(0.0f, 500.0f, 190.0f), false, nullptr, ETeleportType::TeleportPhysics);
    Character->GetCharacterMovement()->StopMovementImmediately();
    TestTrue(TEXT("Sprint can be pressed for perimeter run"), ExecuteActionBinding(Character->InputComponent, TEXT("Sprint"), IE_Pressed));
    for (int32 Frame = 0; Frame < 90; ++Frame)
    {
        Character->AddMovementInput(FVector::RightVector, 1.0f);
        World->Tick(LEVELTICK_All, 1.0f / 60.0f);
        ++GFrameCounter;
    }
    TestTrue(FString::Printf(TEXT("Settlement wall contains sprinting player (position %s)"), *Character->GetActorLocation().ToString()),
        Character->GetActorLocation().Y < 570.0f && Character->GetActorLocation().Z > 150.0f);
    TestTrue(TEXT("Sprint release executes after perimeter run"), ExecuteActionBinding(Character->InputComponent, TEXT("Sprint"), IE_Released));
    Character->SetActorLocation(FVector(-300.0f, 0.0f, 190.0f), false, nullptr, ETeleportType::TeleportPhysics);
    Character->GetCharacterMovement()->StopMovementImmediately();

    const FItemDefinition& Scrap = GameMode->GetItemCatalog().GetOrderedItems()[0];
    FString CarryoverReason;
    TestTrue(TEXT("Fixture has two ordinary items from a prior completed trip"), Character->GetInventory()->TryAdd(Scrap.Id, 2, CarryoverReason));
    ATraderActor* Trader = FindActor<ATraderActor>(World);
    TestNotNull(TEXT("Settlement trader exists"), Trader);
    if (!Trader)
    {
        return false;
    }
    Character->SetActorLocation(Trader->GetActorLocation());
    TestTrue(TEXT("Prior trip earns permanent credits before departure"), Trader->TrySellSlot(Character, 0));
    TestEqual(TEXT("One prior ordinary item remains unsold"), Character->GetInventory()->GetQuantity(Scrap.Id), 1);
    TestEqual(TEXT("Prior trip credits exact catalog value"), Character->GetInventory()->GetCredits(), Scrap.Value);
    Character->SetActorLocation(FVector(-300.0f, 0.0f, 190.0f), false, nullptr, ETeleportType::TeleportPhysics);

    // Exercise CharacterMovement against the actual path and return steps.
    for (int32 Frame = 0; Frame < 360; ++Frame)
    {
        Character->AddMovementInput(FVector::ForwardVector, 1.0f);
        World->Tick(LEVELTICK_All, 1.0f / 60.0f);
        ++GFrameCounter;
    }
    TestTrue(FString::Printf(TEXT("Walking reaches shelf (position %s)"), *Character->GetActorLocation().ToString()), Character->GetActorLocation().X > 2200.0f);
    for (int32 Frame = 0; Frame < 360; ++Frame)
    {
        Character->AddMovementInput(-FVector::ForwardVector, 1.0f);
        World->Tick(LEVELTICK_All, 1.0f / 60.0f);
        ++GFrameCounter;
    }
    TestTrue(TEXT("Walking climbs the return steps to settlement"), Character->GetActorLocation().X < 650.0f);
    TestTrue(TEXT("Settlement collision supports the player"), Character->GetActorLocation().Z > 150.0f);

    for (int32 Index = 0; Index < Pickups.Num(); ++Index)
    {
        Character->SetActorLocation(Pickups[Index]->GetActorLocation());
        TestTrue(FString::Printf(TEXT("Pickup %d collects through its interactable API"), Index + 1), Pickups[Index]->Interact(Character));
        if (Index == 0)
        {
            TestFalse(TEXT("A claimed pickup cannot be collected twice"), Pickups[Index]->Interact(Character));
        }
    }
    TestEqual(TEXT("Five collected items plus prior stock fill six slots"), Character->GetInventory()->GetUsedCapacity(), 6);

    const FItemDefinition& Evidence = GameMode->GetItemCatalog().GetOrderedItems()[4];
    TestEqual(TEXT("Collected scrap sits beside prior ordinary stock"), Character->GetInventory()->GetQuantity(Scrap.Id), 2);
    const FVector TraderSettlementLocation = Trader->GetActorLocation();
    Trader->SetActorLocation(Character->GetActorLocation());
    TestFalse(TEXT("Evidence is protected from sale"), Trader->TrySellSlot(Character, 4));
    Trader->SetActorLocation(TraderSettlementLocation);
    TestEqual(TEXT("Evidence remains after rejected trade"), Character->GetInventory()->GetQuantity(Evidence.Id), 1);
    TestEqual(TEXT("Rejected evidence trade preserves prior credits"), Character->GetInventory()->GetCredits(), Scrap.Value);

    Character->SetActorLocation(FVector(2800.0f, 0.0f, 65.0f));
    const float AdvanceToWarning = Tide->GetSecondsUntilAccessCloses() - 19.0f;
    Tide->Tick(AdvanceToWarning);
    TestTrue(TEXT("Warning activates before route collision closes"), Tide->IsClosingWarning());
    TestTrue(TEXT("Warning countdown reports actual open-access time"), FMath::IsNearlyEqual(Tide->GetSecondsUntilAccessCloses(), 19.0f, 0.2f));
    TestTrue(TEXT("Route remains open during the warning"), Tide->IsAccessOpen());
    TestTrue(TEXT("Sprint can be held when forced recovery begins"), ExecuteActionBinding(Character->InputComponent, TEXT("Sprint"), IE_Pressed));
    Tide->Tick(Tide->GetSecondsUntilAccessCloses() + 0.1f);
    TestTrue(TEXT("Closing access recovers a stranded player"), Character->GetActorLocation().X <= 650.0f);
    TestEqual(TEXT("Recovery preserves prior ordinary stock and protected evidence"), Character->GetInventory()->GetUsedCapacity(), 2);
    TestEqual(TEXT("Recovery preserves ordinary stock carried into this expedition"), Character->GetInventory()->GetQuantity(Scrap.Id), 1);
    TestEqual(TEXT("Recovery keeps evidence"), Character->GetInventory()->GetQuantity(Evidence.Id), 1);
    TestEqual(TEXT("Recovery keeps money earned before failure"), Character->GetInventory()->GetCredits(), Scrap.Value);
    TestFalse(TEXT("Recovery clears held sprint state"), Character->IsSprinting());
    TestTrue(TEXT("Recovery clears carried movement velocity"), Character->GetVelocity().IsNearlyZero());
    TestTrue(TEXT("Recovery explains its trigger and retained permanent state"), Character->GetFeedback().Contains(TEXT("Access submerged"))
        && Character->GetFeedback().Contains(TEXT("evidence and credits kept")));

    Tide->Tick(Tide->GetSecondsRemaining() + 0.1f);
    Tide->Tick(Tide->GetSecondsRemaining() + 0.1f);
    Tide->Tick(Tide->GetSecondsRemaining() + 0.1f);
    TestEqual(TEXT("A second falling cycle reaches low tide"), Tide->GetLowCycle(), 2);
    TArray<APickupActor*> SecondPickups = FindPickups(World);
    TestEqual(TEXT("A second low cycle replenishes salvage without duplicating retained evidence"), SecondPickups.Num(), 4);
    if (SecondPickups.Num() > 0)
    {
        Character->SetActorLocation(SecondPickups[0]->GetActorLocation());
        TestTrue(TEXT("A second expedition can collect new salvage"), SecondPickups[0]->Interact(Character));
        Character->SetActorLocation(FVector(-300.0f, 0.0f, 190.0f));
        World->Tick(LEVELTICK_All, 1.0f / 60.0f);
        ++GFrameCounter;
        TestTrue(TEXT("Returning safely completes the second expedition with its salvage"), Character->GetInventory()->GetUsedCapacity() > 2);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLowTideSafeEdgeReturnTest,
    "LowTide.M05.Expedition.SafeEdgeReturn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLowTideSafeEdgeReturnTest::RunTest(const FString& Parameters)
{
    FLowTideTestWorld TestWorld;
    UWorld* World = TestWorld.World;
    ALowTideGameMode* GameMode = World ? World->GetAuthGameMode<ALowTideGameMode>() : nullptr;
    APlayerController* Controller = World ? World->SpawnActor<APlayerController>() : nullptr;
    ALowTideCharacter* Character = World ? World->SpawnActor<ALowTideCharacter>(FVector(-300.0f, 0.0f, 190.0f), FRotator::ZeroRotator) : nullptr;
    TestNotNull(TEXT("Safe-edge fixture game mode exists"), GameMode);
    TestNotNull(TEXT("Safe-edge fixture controller exists"), Controller);
    TestNotNull(TEXT("Safe-edge fixture character exists"), Character);
    if (!GameMode || !Controller || !Character)
    {
        return false;
    }
    Controller->SetPlayer(NewObject<ULocalPlayer>(GEngine));
    Controller->Possess(Character);

    ATideController* Tide = GameMode->GetTideController();
    TestNotNull(TEXT("Safe-edge fixture tide exists"), Tide);
    if (!Tide)
    {
        return false;
    }
    Tide->Tick(20.1f);
    const FItemDefinition& Scrap = GameMode->GetItemCatalog().GetOrderedItems()[0];
    FString Reason;

    Character->SetActorLocation(FVector(800.0f, 0.0f, 190.0f));
    World->Tick(LEVELTICK_All, 1.0f / 60.0f);
    ++GFrameCounter;
    TestTrue(TEXT("First trip collects one ordinary item"), Character->GetInventory()->TryAdd(Scrap.Id, 1, Reason));
    Character->SetActorLocation(FVector(680.0f, 0.0f, 190.0f));
    Tide->Tick(Tide->GetSecondsUntilAccessCloses() + 0.1f);
    TestEqual(TEXT("Standing at X680 counts as safely back on shore at closure"), Character->GetInventory()->GetQuantity(Scrap.Id), 1);

    Tide->Tick(Tide->GetSecondsRemaining() + 0.1f);
    Tide->Tick(Tide->GetSecondsRemaining() + 0.1f);
    Tide->Tick(Tide->GetSecondsRemaining() + 0.1f);
    Character->SetActorLocation(FVector(800.0f, 0.0f, 190.0f));
    World->Tick(LEVELTICK_All, 1.0f / 60.0f);
    ++GFrameCounter;
    TestTrue(TEXT("Next trip collects another ordinary item"), Character->GetInventory()->TryAdd(Scrap.Id, 1, Reason));
    Character->SetActorLocation(FVector(2800.0f, 0.0f, 190.0f));
    Tide->Tick(Tide->GetSecondsUntilAccessCloses() + 0.1f);
    TestEqual(TEXT("Later failure preserves stock banked by the X680 safe return"), Character->GetInventory()->GetQuantity(Scrap.Id), 1);
    return true;
}

#endif
