#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
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

    TestEqual(TEXT("Catalog has the five expedition definitions"), GameMode->GetItemCatalog().GetOrderedItems().Num(), 5);
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
    TestEqual(TEXT("Five collected items occupy five slots"), Character->GetInventory()->GetUsedCapacity(), 5);

    ATraderActor* Trader = FindActor<ATraderActor>(World);
    TestNotNull(TEXT("Settlement trader exists"), Trader);
    if (!Trader)
    {
        return false;
    }
    Character->SetActorLocation(Trader->GetActorLocation());
    const FItemDefinition& Scrap = GameMode->GetItemCatalog().GetOrderedItems()[0];
    const FItemDefinition& Evidence = GameMode->GetItemCatalog().GetOrderedItems()[4];
    TestTrue(TEXT("Trader sells a collected salvage slot at range"), Trader->TrySellSlot(Character, 0));
    TestEqual(TEXT("Sale removes exactly one salvage item"), Character->GetInventory()->GetQuantity(Scrap.Id), 0);
    TestEqual(TEXT("Sale credits the catalog value"), Character->GetInventory()->GetCredits(), Scrap.Value);
    TestFalse(TEXT("Evidence is protected from sale"), Trader->TrySellSlot(Character, 4));
    TestEqual(TEXT("Evidence remains after rejected trade"), Character->GetInventory()->GetQuantity(Evidence.Id), 1);
    TestEqual(TEXT("Rejected evidence trade preserves credits"), Character->GetInventory()->GetCredits(), Scrap.Value);

    Character->SetActorLocation(FVector(2800.0f, 0.0f, 65.0f));
    Tide->Tick(Tide->GetSecondsRemaining() + 0.1f);
    Tide->Tick(15.0f);
    TestTrue(TEXT("Closing access recovers a stranded player"), Character->GetActorLocation().X <= 650.0f);
    TestEqual(TEXT("Recovery loses unsold salvage"), Character->GetInventory()->GetUsedCapacity(), 1);
    TestEqual(TEXT("Recovery keeps evidence"), Character->GetInventory()->GetQuantity(Evidence.Id), 1);

    Tide->Tick(15.0f);
    Tide->Tick(10.0f);
    Tide->Tick(20.1f);
    TestEqual(TEXT("A second falling cycle reaches low tide"), Tide->GetLowCycle(), 2);
    TestEqual(TEXT("A second low cycle replenishes salvage without duplicating retained evidence"), FindPickups(World).Num(), 4);
    return true;
}

#endif
