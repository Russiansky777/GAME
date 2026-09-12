#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CoastalScene.h"
#include "HubDressingActor.h"
#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LowTideGameMode.h"
#include "JobBoardActor.h"
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
        int32 HubNatureFamilies = 0;
        for (const UHierarchicalInstancedStaticMeshComponent* Mesh : InstancedMeshes)
        {
            if (Mesh && Mesh->ComponentHasTag(TEXT("M1Boundary")))
            {
                TaggedBoundaryCount += Mesh->GetInstanceCount();
            }
            if (Mesh && Mesh->ComponentHasTag(TEXT("HubNature")))
            {
                ++HubNatureFamilies;
                TestTrue(TEXT("Every curated hub nature family has placed instances"), Mesh->GetInstanceCount() > 0);
                TestTrue(TEXT("Curated hub nature remains decorative"),
                    Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
            }
        }
        TestEqual(TEXT("All ten curated CC0 hub nature families load"), HubNatureFamilies, 10);
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
    TestNotNull(TEXT("Mission board is spawned as a separate hub interaction"), GameMode->GetJobBoard());
    if (AJobBoardActor* Board = GameMode->GetJobBoard())
    {
        TInlineComponentArray<UStaticMeshComponent*> BoardMeshes(Board);
        const UStaticMeshComponent* BoardMesh = nullptr;
        int32 AnimatedBoardPieces = 0;
        int32 AnimatedPapers = 0;
        const UStaticMeshComponent* AnimatedLantern = nullptr;
        const UStaticMeshComponent* PaperA = nullptr;
        const UStaticMeshComponent* PaperB = nullptr;
        for (const UStaticMeshComponent* Mesh : BoardMeshes)
        {
            if (!Mesh)
            {
                continue;
            }
            if (Mesh->ComponentTags.Contains(TEXT("JobBoardHeroMesh")))
            {
                BoardMesh = Mesh;
            }
            if (Mesh->ComponentTags.Contains(TEXT("JobBoardAnimatedLantern")))
            {
                AnimatedLantern = Mesh;
                ++AnimatedBoardPieces;
                TestEqual(TEXT("Animated lantern is movable"), Mesh->Mobility, EComponentMobility::Movable);
                TestFalse(TEXT("Animated lantern cannot generate overlap work"), Mesh->GetGenerateOverlapEvents());
            }
            if (Mesh->ComponentTags.Contains(TEXT("JobBoardAnimatedPaper")))
            {
                ++AnimatedBoardPieces;
                ++AnimatedPapers;
                TestEqual(TEXT("Animated paper is movable"), Mesh->Mobility, EComponentMobility::Movable);
                TestFalse(TEXT("Animated paper cannot generate overlap work"), Mesh->GetGenerateOverlapEvents());
                if (Mesh->GetFName() == TEXT("JobBoardPaperAMesh"))
                {
                    PaperA = Mesh;
                }
                else if (Mesh->GetFName() == TEXT("JobBoardPaperBMesh"))
                {
                    PaperB = Mesh;
                }
            }
            TestEqual(TEXT("Hero board visual and animated decorations stay collision-free"),
                Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
            TestNotNull(TEXT("Hero board assembly mesh is available for cooking"), Mesh->GetStaticMesh().Get());
        }
        TestNotNull(TEXT("Mission board loads its authored static hero body"), BoardMesh);
        if (BoardMesh && BoardMesh->GetStaticMesh())
        {
            const FVector HeroBoardSize = BoardMesh->GetStaticMesh()->GetBoundingBox().GetSize();
            TestTrue(TEXT("Hero board main assembly preserves its 180 cm width and 210 cm height"),
                FMath::IsNearlyEqual(HeroBoardSize.Y, 180.0f, 1.0f)
                && FMath::IsNearlyEqual(HeroBoardSize.Z, 210.0f, 1.0f));
        }
        TestEqual(TEXT("Mission board assembles one lantern and two pinned paper decorations"), AnimatedBoardPieces, 3);
        TestEqual(TEXT("Mission board assembles two independent paper decorations"), AnimatedPapers, 2);
        TestTrue(TEXT("Job board motion updates at no more than 30 Hz"), Board->PrimaryActorTick.TickInterval >= (1.0f / 30.0f));
        if (AnimatedLantern)
        {
            const USceneComponent* LanternAnchor = AnimatedLantern->GetAttachParent();
            TestNotNull(TEXT("Separated lantern has a dedicated animation anchor"), LanternAnchor);
            if (LanternAnchor)
            {
                TestTrue(TEXT("Lantern anchor uses the reviewed Interchange-flipped Y pivot"),
                    FMath::IsNearlyEqual(LanternAnchor->GetRelativeLocation().Y, 74.1814f, 0.1f));
                TestTrue(TEXT("Lantern baked board-space mesh is cancelled back to the board origin"),
                    (LanternAnchor->GetRelativeLocation() + AnimatedLantern->GetRelativeLocation()).IsNearlyZero(0.1f));
            }
        }
        TestNotNull(TEXT("Hero board has the upper pinned paper component"), PaperA);
        TestNotNull(TEXT("Hero board has the lower pinned paper component"), PaperB);
        if (PaperA && PaperB)
        {
            const USceneComponent* PaperAAnchor = PaperA->GetAttachParent();
            const USceneComponent* PaperBAnchor = PaperB->GetAttachParent();
            TestNotNull(TEXT("Upper paper has its own pinned-edge anchor"), PaperAAnchor);
            TestNotNull(TEXT("Lower paper has its own pinned-edge anchor"), PaperBAnchor);
            if (PaperAAnchor && PaperBAnchor)
            {
                TestTrue(TEXT("Paper anchors use the reviewed Interchange-flipped Y pivots"),
                    FMath::IsNearlyEqual(PaperAAnchor->GetRelativeLocation().Y, 12.0f, 0.1f)
                    && FMath::IsNearlyEqual(PaperBAnchor->GetRelativeLocation().Y, -20.0f, 0.1f));
                TestTrue(TEXT("Paper meshes cancel their baked board-space offsets"),
                    (PaperAAnchor->GetRelativeLocation() + PaperA->GetRelativeLocation()).IsNearlyZero(0.1f)
                    && (PaperBAnchor->GetRelativeLocation() + PaperB->GetRelativeLocation()).IsNearlyZero(0.1f));
            }
        }
        if (AnimatedLantern)
        {
            Board->ApplyLivelinessPoseForTesting(0.0f);
            const FRotator FirstPose = AnimatedLantern->GetAttachParent()->GetRelativeRotation();
            Board->ApplyLivelinessPoseForTesting(0.7f);
            TestFalse(TEXT("Subtle board liveliness changes the separated lantern transform"),
                AnimatedLantern->GetAttachParent()->GetRelativeRotation().Equals(FirstPose, 0.01f));
        }

        TInlineComponentArray<UBoxComponent*> BoardBoxes(Board);
        int32 StructuralBoardBoxes = 0;
        int32 InteractionBoardBoxes = 0;
        for (const UBoxComponent* Box : BoardBoxes)
        {
            if (!Box)
            {
                continue;
            }
            if (Box->ComponentTags.Contains(TEXT("JobBoardStructuralCollision")))
            {
                ++StructuralBoardBoxes;
                TestEqual(TEXT("Hero board structural pieces block traversal"),
                    Box->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
                TestTrue(TEXT("Hero board structural pieces remain tighter than the authored full mesh bounds"),
                    Box->GetScaledBoxExtent().X <= 13.0f && Box->GetScaledBoxExtent().Y <= 68.0f);
                TestTrue(TEXT("Hero board structural pieces remain within the authored 210 cm height"),
                    Box->GetScaledBoxExtent().Z <= 105.0f);
            }
            if (Box->ComponentTags.Contains(TEXT("JobBoardInteraction")))
            {
                ++InteractionBoardBoxes;
                TestEqual(TEXT("Board sightline target is query-only"),
                    Box->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
                TestEqual(TEXT("Board sightline target cannot block pawn movement"),
                    Box->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);
                TestEqual(TEXT("Board sightline target remains visible to the interaction trace"),
                    Box->GetCollisionResponseToChannel(ECC_Visibility), ECR_Block);
            }
        }
        TestEqual(TEXT("Mission board has only two posts and one notice-panel collision proxy"), StructuralBoardBoxes, 3);
        TestEqual(TEXT("Mission board keeps a separate interaction sightline target"), InteractionBoardBoxes, 1);
        const FTransform BoardTransform = Board->GetActorTransform();
        FHitResult UnderBoardHit;
        FCollisionQueryParams BoardPawnQuery(SCENE_QUERY_STAT(LowTideJobBoardPawnClearance), false);
        TestFalse(TEXT("Board under-panel opening has no phantom pawn blocker"),
            World->LineTraceSingleByChannel(UnderBoardHit,
                BoardTransform.TransformPosition(FVector(-120.0f, 0.0f, 35.0f)),
                BoardTransform.TransformPosition(FVector(40.0f, 0.0f, 35.0f)), ECC_Pawn, BoardPawnQuery));
        FHitResult LanternSpaceHit;
        TestFalse(TEXT("Board front lantern space has no phantom pawn blocker"),
            World->LineTraceSingleByChannel(LanternSpaceHit,
                BoardTransform.TransformPosition(FVector(-120.0f, 75.0f, 130.0f)),
                BoardTransform.TransformPosition(FVector(-30.0f, 75.0f, 130.0f)), ECC_Pawn, BoardPawnQuery));
        const UPointLightComponent* LanternLight = nullptr;
        TInlineComponentArray<UPointLightComponent*> BoardLights(Board);
        for (const UPointLightComponent* Light : BoardLights)
        {
            if (Light && Light->ComponentTags.Contains(TEXT("JobBoardLanternLight")))
            {
                LanternLight = Light;
                break;
            }
        }
        TestNotNull(TEXT("Board includes its small warm lantern light"), LanternLight);
        if (LanternLight)
        {
            TestFalse(TEXT("Board lantern light does not add shadow cost"), LanternLight->CastShadows);
            TestTrue(TEXT("Board lantern light stays local to the board"), LanternLight->AttenuationRadius <= 200.0f);
            TestTrue(TEXT("Board lantern light follows the separately animated lantern"),
                LanternLight->GetAttachParent() == (AnimatedLantern ? AnimatedLantern->GetAttachParent() : nullptr));
        }
        FHitResult BoardHit;
        FCollisionQueryParams BoardQuery(SCENE_QUERY_STAT(LowTideJobBoardSightline), false);
        const FVector BoardCenter = Board->GetActorLocation() + FVector(0.0f, 0.0f, 135.0f);
        const FVector BoardFront = Board->GetActorForwardVector() * -300.0f;
        TestTrue(TEXT("Mission board is visible from its local front approach"),
            World->LineTraceSingleByChannel(BoardHit, BoardCenter + BoardFront, BoardCenter,
                ECC_Visibility, BoardQuery) && BoardHit.GetActor() == Board);
    }
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
        int32 HubSliceMeshCount = 0;
        int32 MeshyHeroPropCount = 0;
        int32 CraneMeshCount = 0;
        UStaticMeshComponent* SalvageBoat = nullptr;
        for (const UStaticMeshComponent* Mesh : HubMeshes)
        {
            TraderHubMeshCount += Mesh && Mesh->ComponentTags.Contains(TEXT("TraderHubMesh")) ? 1 : 0;
            HubSliceMeshCount += Mesh && Mesh->ComponentTags.Contains(TEXT("HubSliceMesh")) ? 1 : 0;
            MeshyHeroPropCount += Mesh && Mesh->ComponentTags.Contains(TEXT("MeshyHubHeroProp")) ? 1 : 0;
            CraneMeshCount += Mesh && Mesh->ComponentTags.Contains(TEXT("MeshyHubCrane")) ? 1 : 0;
            if (Mesh && Mesh->ComponentTags.Contains(TEXT("MeshyHubHut")))
            {
                const FVector HutSize = Mesh->GetStaticMesh()->GetBoundingBox().GetSize();
                TestTrue(TEXT("Meshy hut preserves its substantial authored envelope"),
                    HutSize.X >= 480.0f && HutSize.Y >= 350.0f && HutSize.Z >= 300.0f);
                TestEqual(TEXT("Meshy hut stays visual-only; only simple proxies affect movement"),
                    Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
            }
            if (Mesh && Mesh->ComponentTags.Contains(TEXT("MeshyHubWorkbench")))
            {
                const FVector WorkbenchSize = Mesh->GetStaticMesh()->GetBoundingBox().GetSize();
                TestTrue(TEXT("Salvage workbench preserves its authored working envelope"),
                    WorkbenchSize.X >= 180.0f && WorkbenchSize.Y >= 80.0f && WorkbenchSize.Z >= 110.0f);
                TestEqual(TEXT("Workbench source mesh remains collision-free"), Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
            }
            if (Mesh && Mesh->ComponentTags.Contains(TEXT("MeshyHubBoat")))
            {
                SalvageBoat = const_cast<UStaticMeshComponent*>(Mesh);
                const FVector BoatSize = Mesh->GetStaticMesh()->GetBoundingBox().GetSize();
                TestTrue(TEXT("Salvage boat preserves its readable coastal silhouette"),
                    BoatSize.X >= 390.0f && BoatSize.Y >= 170.0f && BoatSize.Z >= 115.0f);
                TestEqual(TEXT("Boat source mesh remains collision-free"), Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
            }
            if (Mesh && Mesh->ComponentTags.Contains(TEXT("MeshyHubCrane")))
            {
                const FVector CraneSize = Mesh->GetStaticMesh()->GetBoundingBox().GetSize();
                TestTrue(TEXT("Salvage crane preserves its authored working silhouette"),
                    CraneSize.X >= 220.0f && CraneSize.Y >= 125.0f && CraneSize.Z >= 295.0f);
                TestEqual(TEXT("Crane source mesh remains visual-only"), Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
            }
        }
        TestEqual(TEXT("Trader hub uses the one intact authored Meshy hut"), TraderHubMeshCount, 1);
        TestEqual(TEXT("Only low ground dressing remains beside the authored hub assets"), HubSliceMeshCount, 1);
        TestEqual(TEXT("Hub includes static Meshy workbench and boat anchors"), MeshyHeroPropCount, 2);
        TestEqual(TEXT("Dock includes one authored salvage crane"), CraneMeshCount, 1);
        TestNotNull(TEXT("Meshy boat is present beside the base"), SalvageBoat);
        if (SalvageBoat && Layout.WaterActor && Layout.WaterActor->GetRootComponent())
        {
            USceneComponent* WaterRoot = Layout.WaterActor->GetRootComponent();
            TestEqual(TEXT("Boat is attached to the tide-driven water transform"), SalvageBoat->GetAttachParent(), WaterRoot);
            const float BoatDraft = SalvageBoat->GetRelativeLocation().Z;
            const FVector OriginalWaterLocation = WaterRoot->GetComponentLocation();
            const float OriginalBoatZ = SalvageBoat->GetComponentLocation().Z;
            WaterRoot->SetWorldLocation(OriginalWaterLocation + FVector(0.0f, 0.0f, 75.0f));
            TestTrue(TEXT("Boat follows a water-height change without a separate tick"),
                FMath::IsNearlyEqual(SalvageBoat->GetComponentLocation().Z, OriginalBoatZ + 75.0f, 0.1f));
            TestTrue(TEXT("Boat retains its local water draft"),
                FMath::IsNearlyEqual(SalvageBoat->GetRelativeLocation().Z, BoatDraft, 0.01f));
            WaterRoot->SetWorldLocation(OriginalWaterLocation);
        }
        TInlineComponentArray<UBoxComponent*> HubBoxes(TraderHubScene);
        int32 TraderHubProxyCount = 0;
        int32 WorkbenchProxyCount = 0;
        int32 BoatProxyCount = 0;
        for (const UBoxComponent* Box : HubBoxes)
        {
            if (Box && Box->ComponentTags.Contains(TEXT("TraderHubCollision")))
            {
                ++TraderHubProxyCount;
                TestTrue(TEXT("Trader hub collision is owned by the coastal scene, not Mara"),
                    Box->GetOwner() != Layout.Mara.Get());
                TestEqual(TEXT("Structural hut collision is query-only"),
                    Box->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
                TestEqual(TEXT("Structural hut collision blocks pawn movement"),
                    Box->GetCollisionResponseToChannel(ECC_Pawn), ECR_Block);
                TestEqual(TEXT("Structural hut collision occludes visibility from closed sides"),
                    Box->GetCollisionResponseToChannel(ECC_Visibility), ECR_Block);
            }
            if (Box && (Box->ComponentTags.Contains(TEXT("MeshyHubWorkbenchCollision"))
                || Box->ComponentTags.Contains(TEXT("MeshyHubBoatCollision"))))
            {
                WorkbenchProxyCount += Box->ComponentTags.Contains(TEXT("MeshyHubWorkbenchCollision")) ? 1 : 0;
                BoatProxyCount += Box->ComponentTags.Contains(TEXT("MeshyHubBoatCollision")) ? 1 : 0;
                TestEqual(TEXT("Hero prop collision is query-only"), Box->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
                TestEqual(TEXT("Hero prop collision blocks pawns without per-poly source collision"),
                    Box->GetCollisionResponseToChannel(ECC_Pawn), ECR_Block);
            }
        }
        TestTrue(TEXT("Trader hut has compact structural floor, counter and shell collision"), TraderHubProxyCount >= 5);
        TestEqual(TEXT("Workbench has one compact under-worktop proxy"), WorkbenchProxyCount, 1);
        TestEqual(TEXT("Boat has two compact hull proxies"), BoatProxyCount, 2);
        if (SalvageBoat && Layout.RouteWaypoints.Num() >= 2)
        {
            FCollisionQueryParams BoatClearanceQuery(SCENE_QUERY_STAT(LowTideBoatRouteClearance), false);
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                if (*It != TraderHubScene)
                {
                    BoatClearanceQuery.AddIgnoredActor(*It);
                }
            }
            TInlineComponentArray<UPrimitiveComponent*> ScenePrimitives(TraderHubScene);
            for (UPrimitiveComponent* Primitive : ScenePrimitives)
            {
                if (Primitive && !Primitive->ComponentTags.Contains(TEXT("MeshyHubBoatCollision")))
                {
                    BoatClearanceQuery.AddIgnoredComponent(Primitive);
                }
            }
            FHitResult BoatClearanceHit;
            TestFalse(TEXT("Boat hull stays more than 500 cm clear of the actual main-route launch sweep"),
                World->SweepSingleByChannel(BoatClearanceHit, Layout.RouteWaypoints[0], Layout.RouteWaypoints[1],
                    FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(500.0f, 500.0f), BoatClearanceQuery));
        }

        TInlineComponentArray<UHierarchicalInstancedStaticMeshComponent*> HubVisualFamilies(TraderHubScene);
        int32 StandardDeckInstances = 0;
        int32 LongDeckInstances = 0;
        int32 DockInstances = 0;
        int32 DockPilingInstances = 0;
        int32 RouteFloorFamilies = 0;
        TArray<FTransform> StandardDeckTransforms;
        for (const UHierarchicalInstancedStaticMeshComponent* Family : HubVisualFamilies)
        {
            if (!Family)
            {
                continue;
            }
            if (Family->ComponentTags.Contains(TEXT("MeshyHubDeckStandard")))
            {
                StandardDeckInstances += Family->GetInstanceCount();
                TestEqual(TEXT("Deck visuals never add detailed collision"), Family->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
                for (int32 InstanceIndex = 0; InstanceIndex < Family->GetInstanceCount(); ++InstanceIndex)
                {
                    FTransform Transform;
                    if (Family->GetInstanceTransform(InstanceIndex, Transform, true))
                    {
                        StandardDeckTransforms.Add(Transform);
                        const FVector Up = Transform.GetRotation().RotateVector(FVector::UpVector);
                        TestTrue(TEXT("Fitted deck stays on a gentle support plane"), Up.Z >= 0.995f);
                    }
                }
            }
            if (Family->ComponentTags.Contains(TEXT("MeshyHubDeckLong")))
            {
                LongDeckInstances += Family->GetInstanceCount();
                TestEqual(TEXT("Deck extension never adds detailed collision"), Family->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
            }
            if (Family->ComponentTags.Contains(TEXT("MeshyHubDock")))
            {
                DockInstances += Family->GetInstanceCount();
                TestEqual(TEXT("Decorative dock never creates an unvalidated traversal surface"),
                    Family->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
            }
            if (Family->ComponentTags.Contains(TEXT("MeshyHubDockPiling")))
            {
                DockPilingInstances += Family->GetInstanceCount();
                TestNotNull(TEXT("Dock piling has a cooked authored mesh"), Family->GetStaticMesh().Get());
                TestEqual(TEXT("Dock piling stays visual-only"), Family->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
            }
            if (Family->ComponentTags.Contains(TEXT("M1RouteFloor")))
            {
                ++RouteFloorFamilies;
                TestEqual(TEXT("Hidden route floor retains its established gameplay collision"),
                    Family->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
            }
        }
        TestTrue(TEXT("Authored deck provides a bounded visual apron and NE support cover"), StandardDeckInstances >= 100);
        TestTrue(TEXT("Long boards provide the reviewed foreground trim"), LongDeckInstances >= 19);
        TestEqual(TEXT("Dock uses the reviewed four-module visual composition"), DockInstances, 4);
        TestEqual(TEXT("Working platform and outer berth have source-derived supports"), DockPilingInstances, 8);
        TestEqual(TEXT("Deck fitting preserves the one hidden route-floor contract"), RouteFloorFamilies, 1);
        for (int32 FirstIndex = 0; FirstIndex < StandardDeckTransforms.Num(); ++FirstIndex)
        {
            for (int32 SecondIndex = FirstIndex + 1; SecondIndex < StandardDeckTransforms.Num(); ++SecondIndex)
            {
                const FVector FirstLocation = StandardDeckTransforms[FirstIndex].GetLocation();
                const FVector SecondLocation = StandardDeckTransforms[SecondIndex].GetLocation();
                const float HorizontalDistance = FVector::Dist2D(FirstLocation, SecondLocation);
                if (HorizontalDistance >= 150.0f && HorizontalDistance <= 210.0f)
                {
                    // Compare the actual fitted planes at their shared edge, not their pivots:
                    // two adjoining sloped tiles can have different center heights with a flush seam.
                    const FVector Midpoint = (FirstLocation + SecondLocation) * 0.5f;
                    const auto EdgeHeight = [&Midpoint](const FTransform& Transform)
                    {
                        const FVector Top = Transform.TransformPosition(FVector(0.0f, 0.0f, 24.28f));
                        const FVector Normal = Transform.GetRotation().RotateVector(FVector::UpVector);
                        return Top.Z - (Normal.X * (Midpoint.X - Top.X)
                            + Normal.Y * (Midpoint.Y - Top.Y)) / Normal.Z;
                    };
                    const float EdgeGap = FMath::Abs(EdgeHeight(StandardDeckTransforms[FirstIndex])
                        - EdgeHeight(StandardDeckTransforms[SecondIndex]));
                    TestTrue(FString::Printf(TEXT("Adjacent fitted deck edge gap %.2f cm at %s remains bounded"),
                        EdgeGap, *Midpoint.ToCompactString()), EdgeGap <= 20.0f);
                }
            }
        }
    }
    int32 DonorActors = 0;
    for (TActorIterator<AHubDressingActor> It(World); It; ++It)
    {
        ++DonorActors;
        TInlineComponentArray<UHierarchicalInstancedStaticMeshComponent*> DonorFamilies(*It);
        int32 DonorInstances = 0;
        for (const UHierarchicalInstancedStaticMeshComponent* Family : DonorFamilies)
        {
            TestNotNull(TEXT("Hub donor mesh is available for cooking"), Family->GetStaticMesh().Get());
            TestEqual(TEXT("Donor dressing cannot change traversal or interaction traces"),
                Family->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
            DonorInstances += Family->GetInstanceCount();
        }
        TestTrue(TEXT("Hub includes composed donor dressing"), DonorInstances >= 12);
    }
    TestEqual(TEXT("Exactly one hub donor composition is spawned"), DonorActors, 1);
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
        const FVector HutBackDirection = FRotator(0.0f, -38.0f, 0.0f).RotateVector(FVector::ForwardVector);
        FHitResult RearHit;
        TestTrue(TEXT("Trader hut rear remains a physical visibility occluder"),
            World->LineTraceSingleByChannel(RearHit, TraceEnd + HutBackDirection * 380.0f, TraceEnd,
                ECC_Visibility, Params) && RearHit.GetActor() == TraderHubScene);
    }
    return true;
}

#endif
