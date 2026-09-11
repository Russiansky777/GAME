#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CoastalScene.h"
#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LowTideGameMode.h"
#include "TraderActor.h"

namespace
{
class FLowTideM1SceneWorld
{
public:
    FLowTideM1SceneWorld()
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
    }

    ~FLowTideM1SceneWorld()
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

FVector RouteSide(const FVector& Start, const FVector& End)
{
    FVector Direction(End.X - Start.X, End.Y - Start.Y, 0.0f);
    Direction.Normalize();
    return FVector(-Direction.Y, Direction.X, 0.0f);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLowTideM1SceneContainmentTest,
    "LowTide.M1.Scene.Containment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLowTideM1SceneContainmentTest::RunTest(const FString& Parameters)
{
    FLowTideM1SceneWorld TestWorld;
    UWorld* World = TestWorld.World;
    ALowTideGameMode* GameMode = World ? World->GetAuthGameMode<ALowTideGameMode>() : nullptr;
    TestNotNull(TEXT("M1 scene fixture selects LowTideGameMode"), GameMode);
    if (!GameMode)
    {
        return false;
    }

    const FCoastalSceneLayout& Layout = GameMode->GetSceneLayout();
    TestTrue(TEXT("Authored main loop is at least 700 metres"), Layout.MainRouteLengthCm >= 70000.0f);
    TestTrue(TEXT("Authored main loop stays compact enough for the slice"), Layout.MainRouteLengthCm <= 110000.0f);
    TestTrue(TEXT("Optional detour adds at least 300 metres"), Layout.OptionalRouteLengthCm >= 30000.0f);
    TestTrue(TEXT("Optional detour stays within M1 scope"), Layout.OptionalRouteLengthCm <= 50000.0f);

    int32 TaggedBoundaryCount = 0;
    const ACoastalScene* Scene = nullptr;
    for (TActorIterator<ACoastalScene> It(World); It; ++It)
    {
        Scene = *It;
        break;
    }
    TestNotNull(TEXT("Authored coastal scene actor exists"), Scene);
    if (Scene)
    {
        TArray<UBoxComponent*> Boxes;
        Scene->GetComponents<UBoxComponent>(Boxes);
        for (const UBoxComponent* Box : Boxes)
        {
            TaggedBoundaryCount += Box && Box->ComponentHasTag(TEXT("M1Boundary")) ? 1 : 0;
        }
        TArray<UHierarchicalInstancedStaticMeshComponent*> InstancedMeshes;
        Scene->GetComponents<UHierarchicalInstancedStaticMeshComponent>(InstancedMeshes);
        for (const UHierarchicalInstancedStaticMeshComponent* Mesh : InstancedMeshes)
        {
            if (Mesh && Mesh->ComponentHasTag(TEXT("M1Boundary")))
            {
                TaggedBoundaryCount += Mesh->GetInstanceCount();
            }
        }
    }
    TestTrue(TEXT("Every route and the settlement have a substantial tagged backup perimeter"), TaggedBoundaryCount >= 50);

    const FCollisionShape Capsule = FCollisionShape::MakeCapsule(42.0f, 92.0f);
    const auto SweepBlocks = [this, World, &Capsule](const FString& Label, const FVector& Start, const FVector& End)
    {
        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(LowTideM1BoundarySweep), false);
        const bool bHit = World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn, Capsule, Params);
        TestTrue(Label + TEXT(" contains a player capsule"), bHit);
        TestTrue(Label + TEXT(" reaches authored perimeter collision"), bHit && ((Hit.GetComponent()
            && Hit.GetComponent()->ComponentHasTag(TEXT("M1Boundary")))
            || (Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("M1Shortcut")))));
        return bHit;
    };
    const auto SweepClear = [this, World, &Capsule](const FString& Label, const FVector& Start, const FVector& End)
    {
        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(LowTideM1OpeningSweep), false);
        const bool bHit = World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn, Capsule, Params);
        if (bHit)
        {
            AddInfo(FString::Printf(TEXT("%s hit %s / %s at %s"), *Label,
                Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("no actor"),
                Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("no component"),
                *Hit.ImpactPoint.ToString()));
        }
        TestFalse(Label, bHit);
    };

    constexpr float SettlementSweepZ = 255.0f;
    SweepBlocks(TEXT("Settlement west edge"), FVector(-2450.0f, 0.0f, SettlementSweepZ), FVector(-3050.0f, 0.0f, SettlementSweepZ));
    SweepBlocks(TEXT("Settlement north edge"), FVector(0.0f, 2550.0f, SettlementSweepZ), FVector(0.0f, 3200.0f, SettlementSweepZ));
    SweepBlocks(TEXT("Settlement south edge"), FVector(0.0f, -2650.0f, SettlementSweepZ), FVector(0.0f, -3300.0f, SettlementSweepZ));
    SweepBlocks(TEXT("Settlement east solid edge"), FVector(2450.0f, 0.0f, SettlementSweepZ), FVector(3050.0f, 0.0f, SettlementSweepZ));
    SweepBlocks(TEXT("Settlement northwest corner"), FVector(-2400.0f, 2500.0f, SettlementSweepZ), FVector(-3100.0f, 3200.0f, SettlementSweepZ));
    SweepBlocks(TEXT("Settlement southwest corner"), FVector(-2400.0f, -2600.0f, SettlementSweepZ), FVector(-3100.0f, -3300.0f, SettlementSweepZ));
    SweepBlocks(TEXT("Lower exit jamb"), FVector(2450.0f, -2530.0f, SettlementSweepZ), FVector(3050.0f, -2530.0f, SettlementSweepZ));
    SweepBlocks(TEXT("Upper exit jamb"), FVector(2450.0f, -1770.0f, SettlementSweepZ), FVector(3050.0f, -1770.0f, SettlementSweepZ));
    SweepClear(TEXT("Authored southeast settlement exit remains capsule-clear"),
        FVector(2450.0f, -2150.0f, SettlementSweepZ), FVector(3050.0f, -2150.0f, SettlementSweepZ));
    SweepClear(TEXT("Blue ridge settlement return remains capsule-clear"),
        FVector(3050.0f, 1525.0f, 320.0f), FVector(2450.0f, 1225.0f, 320.0f));

    const auto GroundSupports = [this, World](const FString& Label, const FVector& XY)
    {
        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(LowTideM1GroundProbe), false);
        TestTrue(Label, World->LineTraceSingleByChannel(Hit, XY + FVector(0.0f, 0.0f, 500.0f),
            XY - FVector(0.0f, 0.0f, 500.0f), ECC_Pawn, Params));
    };
    GroundSupports(TEXT("Settlement northwest interior has supporting floor"), FVector(-2500.0f, 2600.0f, 0.0f));
    GroundSupports(TEXT("Settlement northeast interior has supporting floor"), FVector(2500.0f, 2600.0f, 0.0f));
    GroundSupports(TEXT("Settlement southwest interior has supporting floor"), FVector(-2500.0f, -2700.0f, 0.0f));
    GroundSupports(TEXT("Settlement southeast interior has supporting floor"), FVector(2500.0f, -2700.0f, 0.0f));

    if (Layout.ShortcutBlocker)
    {
        Layout.ShortcutBlocker->SetActorEnableCollision(false);
    }

    const auto AuditRoute = [this, &Layout, &SweepBlocks, &SweepClear](const FString& RouteName,
        const TArray<FVector>& Points, float Width)
    {
        for (int32 Index = 0; Index + 1 < Points.Num(); ++Index)
        {
            const FVector Start = Points[Index];
            const FVector End = Points[Index + 1];
            const FVector Mid = (Start + End) * 0.5f;
            const FVector Up(0.0f, 0.0f, 200.0f);
            const FVector Side = RouteSide(Start, End);
            if (!Layout.SettlementSafeBounds.IsInsideOrOn(Mid))
            {
                SweepBlocks(FString::Printf(TEXT("%s segment %d left edge blocks a jump-height capsule"), *RouteName, Index + 1),
                    Mid + Up, Mid + Up + Side * (Width * 0.5f + 3000.0f));
                SweepBlocks(FString::Printf(TEXT("%s segment %d right edge blocks a jump-height capsule"), *RouteName, Index + 1),
                    Mid + Up, Mid + Up - Side * (Width * 0.5f + 3000.0f));
            }
            SweepClear(FString::Printf(TEXT("%s segment %d center stays clear"), *RouteName, Index + 1),
                Start + Up, End + Up);
        }
    };
    AuditRoute(TEXT("Main route"), Layout.RouteWaypoints, 900.0f);
    AuditRoute(TEXT("Blue return ridge"), Layout.AlternateRouteWaypoints, 820.0f);
    AuditRoute(TEXT("Optional shrine branch"), Layout.OptionalRouteWaypoints, 760.0f);

    if (Layout.ShortcutBlocker)
    {
        Layout.ShortcutBlocker->SetActorEnableCollision(true);
        const FVector ShortcutStart = Layout.RouteWaypoints[7] + FVector(0.0f, 0.0f, 210.0f);
        const FVector ShortcutEnd = Layout.RouteWaypoints[8] + FVector(0.0f, 0.0f, 210.0f);
        SweepBlocks(TEXT("Returning tide closes only the low shortcut"), ShortcutStart, ShortcutEnd);
        SweepBlocks(TEXT("Closed low shortcut blocks a jump-height capsule"),
            ShortcutStart + FVector(0.0f, 0.0f, 70.0f), ShortcutEnd + FVector(0.0f, 0.0f, 70.0f));
    }

    TestNotNull(TEXT("Mara is returned by the scene layout"), Layout.Mara.Get());
    ACoastalScene* TraderHubScene = nullptr;
    for (TActorIterator<ACoastalScene> It(World); It; ++It)
    {
        TraderHubScene = *It;
        break;
    }
    TestNotNull(TEXT("Authored coastal scene exists for trader hub"), TraderHubScene);
    if (TraderHubScene)
    {
        TInlineComponentArray<UStaticMeshComponent*> HubMeshes(TraderHubScene);
        int32 TraderHubMeshCount = 0;
        for (const UStaticMeshComponent* Mesh : HubMeshes)
        {
            TraderHubMeshCount += Mesh && Mesh->ComponentTags.Contains(TEXT("TraderHubMesh")) ? 1 : 0;
        }
        TestEqual(TEXT("Approved trader hub loads all seven authored meshes"), TraderHubMeshCount, 7);

        TInlineComponentArray<UBoxComponent*> HubBoxes(TraderHubScene);
        int32 TraderHubProxyCount = 0;
        for (const UBoxComponent* Box : HubBoxes)
        {
            if (Box && Box->ComponentTags.Contains(TEXT("TraderHubCollision")))
            {
                ++TraderHubProxyCount;
                TestTrue(TEXT("Trader hub collision is owned by the coastal scene, not Mara"),
                    Box->GetOwner() != Layout.Mara.Get());
            }
        }
        TestEqual(TEXT("Trader hub has exactly three structural wall proxies"), TraderHubProxyCount, 3);
    }
    if (Layout.Mara)
    {
        const FVector MaraLocation = Layout.Mara->GetActorLocation();
        FVector ApproachDirection = Layout.PlayerStart - MaraLocation;
        ApproachDirection.Z = 0.0f;
        ApproachDirection.Normalize();
        const FVector TraceEnd = MaraLocation + FVector(0.0f, 0.0f, 55.0f);
        const FVector TraceStart = TraceEnd + ApproachDirection * 380.0f;
        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(LowTideMaraSightline), false);
        const bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);
        TestTrue(TEXT("Mara has a direct interaction sightline through her decorative stall"),
            bHit && Hit.GetActor() == Layout.Mara);
    }
    return true;
}

#endif
