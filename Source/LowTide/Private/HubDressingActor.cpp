#include "HubDressingActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AHubDressingActor::AHubDressingActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HubDressingRoot"));
    SetRootComponent(SceneRoot);

    Barrels = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("KenneyBarrels"));
    Crates = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("KenneyCrates"));
    BottleCrates = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("KenneyBottleCrates"));
    Chests = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("KenneyChests"));
    Rowboats = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("KenneyRowboats"));
    Paddles = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("KenneyPaddles"));
    MastRopes = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("KenneyMastRopes"));
    ShipWrecks = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("KenneyShipWrecks"));

    UHierarchicalInstancedStaticMeshComponent* Families[] = {
        Barrels, Crates, BottleCrates, Chests, Rowboats, Paddles, MastRopes, ShipWrecks
    };
    for (UHierarchicalInstancedStaticMeshComponent* Family : Families)
    {
        Configure(Family);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> BarrelAsset(TEXT(
        "/Game/Generated/HubDonors/KenneyPirateKitCC0/SM_LT_Kenney_Barrel.SM_LT_Kenney_Barrel"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CrateAsset(TEXT(
        "/Game/Generated/HubDonors/KenneyPirateKitCC0/SM_LT_Kenney_Crate.SM_LT_Kenney_Crate"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BottleCrateAsset(TEXT(
        "/Game/Generated/HubDonors/KenneyPirateKitCC0/SM_LT_Kenney_BottleCrate.SM_LT_Kenney_BottleCrate"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ChestAsset(TEXT(
        "/Game/Generated/HubDonors/KenneyPirateKitCC0/SM_LT_Kenney_Chest.SM_LT_Kenney_Chest"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> RowboatAsset(TEXT(
        "/Game/Generated/HubDonors/KenneyPirateKitCC0/SM_LT_Kenney_Rowboat.SM_LT_Kenney_Rowboat"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PaddleAsset(TEXT(
        "/Game/Generated/HubDonors/KenneyPirateKitCC0/SM_LT_Kenney_Paddle.SM_LT_Kenney_Paddle"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MastAsset(TEXT(
        "/Game/Generated/HubDonors/KenneyPirateKitCC0/SM_LT_Kenney_MastRopes.SM_LT_Kenney_MastRopes"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> WreckAsset(TEXT(
        "/Game/Generated/HubDonors/KenneyPirateKitCC0/SM_LT_Kenney_ShipWreck.SM_LT_Kenney_ShipWreck"));
    Barrels->SetStaticMesh(BarrelAsset.Object);
    Crates->SetStaticMesh(CrateAsset.Object);
    BottleCrates->SetStaticMesh(BottleCrateAsset.Object);
    Chests->SetStaticMesh(ChestAsset.Object);
    Rowboats->SetStaticMesh(RowboatAsset.Object);
    Paddles->SetStaticMesh(PaddleAsset.Object);
    MastRopes->SetStaticMesh(MastAsset.Object);
    ShipWrecks->SetStaticMesh(WreckAsset.Object);
}

void AHubDressingActor::Configure(UHierarchicalInstancedStaticMeshComponent* Component)
{
    Component->SetupAttachment(SceneRoot);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetGenerateOverlapEvents(false);
    Component->SetCanEverAffectNavigation(false);
    Component->SetCastShadow(true);
    Component->SetCullDistances(10000, 26000);
    Component->ComponentTags.Add(TEXT("HubDonorDressing"));
}

void AHubDressingActor::Add(UHierarchicalInstancedStaticMeshComponent* Component, const FVector& Location,
    const FRotator& Rotation, const FVector& Scale)
{
    if (Component && Component->GetStaticMesh())
    {
        // Donor pivots differ and tilted props have non-obvious minima. Treat the
        // authored location as a world-space bottom centre after rotation/scale,
        // so every composition stays grounded from verified static-mesh bounds.
        FTransform InstanceTransform(Rotation, FVector::ZeroVector, Scale);
        const FBox PlacedBounds = Component->GetStaticMesh()->GetBoundingBox().TransformBy(InstanceTransform);
        InstanceTransform.SetTranslation(FVector(
            Location.X - (PlacedBounds.Min.X + PlacedBounds.Max.X) * 0.5f,
            Location.Y - (PlacedBounds.Min.Y + PlacedBounds.Max.Y) * 0.5f,
            Location.Z - PlacedBounds.Min.Z));
        Component->AddInstance(InstanceTransform);
    }
}

void AHubDressingActor::BuildDressing()
{
    if (bBuilt)
    {
        return;
    }
    bBuilt = true;

    // Trader-side working cluster. It sits behind/right of Mara's counter and
    // leaves the broad spawn-to-counter approach and interaction radius open.
    Add(Barrels, FVector(1640.0f, -1875.0f, 128.0f), FRotator(0.0f, -22.0f, 0.0f), FVector(0.72f));
    Add(Barrels, FVector(1755.0f, -1815.0f, 128.0f), FRotator(0.0f, 18.0f, 0.0f), FVector(0.64f));
    Add(Crates, FVector(1510.0f, -2015.0f, 128.0f), FRotator(0.0f, -11.0f, 0.0f), FVector(0.68f));
    Add(Crates, FVector(1520.0f, -2005.0f, 180.5f), FRotator(0.0f, 16.0f, 0.0f), FVector(0.52f));
    Add(BottleCrates, FVector(1430.0f, -1900.0f, 128.0f), FRotator(0.0f, -30.0f, 0.0f), FVector(0.62f));
    Add(Chests, FVector(1850.0f, -2050.0f, 128.0f), FRotator(0.0f, 44.0f, 0.0f), FVector(0.70f));

    // Low foreground work cluster, split around the initial view axis. The near
    // pieces make the apron feel used without occupying the route centre.
    // Foreground crates and the toy-like rowboat were removed for the authored Meshy workbench.

    // Departure-edge cluster: readable silhouettes frame the route opening at +X
    // while remaining beyond the 700 cm clear travel lane centred near Y=-800.
    Add(MastRopes, FVector(2330.0f, -2450.0f, 120.0f), FRotator(0.0f, -18.0f, 0.0f), FVector(0.48f));
    Add(Barrels, FVector(2320.0f, -1920.0f, 126.0f), FRotator(0.0f, 12.0f, 0.0f), FVector(0.66f));
    Add(Crates, FVector(2445.0f, -1835.0f, 126.0f), FRotator(0.0f, -14.0f, 0.0f), FVector(0.62f));
    Add(Paddles, FVector(2250.0f, -2040.0f, 128.0f), FRotator(90.0f, 20.0f, -8.0f), FVector(0.72f));

    // Shoreline storytelling: a hauled-up rowboat and a restrained wreck fragment.
    // Both are collision-free and outside the hub's central playable composition.
    Add(Rowboats, FVector(-1900.0f, -2580.0f, 104.0f), FRotator(3.0f, 26.0f, -8.0f), FVector(0.82f));
    Add(Paddles, FVector(-1780.0f, -2480.0f, 108.0f), FRotator(90.0f, 55.0f, 5.0f), FVector(0.78f));
    Add(ShipWrecks, FVector(4700.0f, -2100.0f, -70.0f), FRotator(-5.0f, 118.0f, 10.0f), FVector(0.30f));

    UE_LOG(LogTemp, Display, TEXT("LOW TIDE hub donor dressing built: %d non-colliding instances across 8 mesh families."),
        Barrels->GetInstanceCount() + Crates->GetInstanceCount() + BottleCrates->GetInstanceCount()
            + Chests->GetInstanceCount() + Rowboats->GetInstanceCount() + Paddles->GetInstanceCount()
            + MastRopes->GetInstanceCount() + ShipWrecks->GetInstanceCount());
}
