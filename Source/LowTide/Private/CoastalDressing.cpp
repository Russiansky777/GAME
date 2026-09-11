#include "CoastalDressing.h"

#include "CoastalScene.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "UObject/ConstructorHelpers.h"

ACoastalDressing::ACoastalDressing()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DressingRoot"));
    SetRootComponent(SceneRoot);

    CreamInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CreamTrim"));
    TealInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TealPaint"));
    CoralInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CoralPaint"));
    VioletInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("MysticViolet"));
    GrassInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("GrassClumps"));
    MarketInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("MarketProps"));
    BasaltInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("BasaltSkirts"));
    DistantCliffInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("DistantCliffs"));

    UHierarchicalInstancedStaticMeshComponent* Families[] = {
        CreamInstances, TealInstances, CoralInstances, VioletInstances, GrassInstances, MarketInstances, BasaltInstances,
        DistantCliffInstances
    };
    for (UHierarchicalInstancedStaticMeshComponent* Family : Families)
    {
        Family->SetupAttachment(SceneRoot);
        Family->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Family->SetGenerateOverlapEvents(false);
        Family->SetCastShadow(true);
        Family->SetCullDistances(18000, 75000);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> RockAsset(TEXT("/Game/Generated/M1/SM_LT_FacetedRock.SM_LT_FacetedRock"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(
        TEXT("/Game/Generated/M1/M_LT_StylizedOpaque.M_LT_StylizedOpaque"));
    CubeMesh = CubeAsset.Object;
    CylinderMesh = CylinderAsset.Object;
    ConeMesh = ConeAsset.Object;
    FacetedRockMesh = RockAsset.Succeeded() ? RockAsset.Object : CubeAsset.Object;
    StylizedMaterial = MaterialAsset.Object;
}

void ACoastalDressing::ConfigureFamily(UHierarchicalInstancedStaticMeshComponent* Component, UStaticMesh* Mesh,
    const FLinearColor& Color, float Roughness)
{
    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (StylizedMaterial)
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(StylizedMaterial, this);
        Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
        Material->SetVectorParameterValue(TEXT("Color"), Color);
        Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
        Component->SetMaterial(0, Material);
    }
}

void ACoastalDressing::Add(UHierarchicalInstancedStaticMeshComponent* Component, const FVector& Location,
    const FVector& Scale, const FRotator& Rotation)
{
    Component->AddInstance(FTransform(Rotation, Location, Scale));
}

void ACoastalDressing::AddBeam(UHierarchicalInstancedStaticMeshComponent* Component, const FVector& Start,
    const FVector& End, float Thickness)
{
    const FVector Delta = End - Start;
    if (Delta.IsNearlyZero())
    {
        return;
    }
    Add(Component, (Start + End) * 0.5f,
        FVector(Thickness / 100.0f, Thickness / 100.0f, Delta.Size() / 100.0f),
        FRotationMatrix::MakeFromZ(Delta).Rotator());
}

void ACoastalDressing::AddHutFinish(const FVector& Location, float YawDegrees, float Scale)
{
    const FRotator Rotation(0.0f, YawDegrees, 0.0f);
    const FVector Forward = Rotation.RotateVector(FVector::ForwardVector);
    const FVector Side = Rotation.RotateVector(FVector::RightVector);
    // A framed door and two shuttered windows make the existing hut volumes legible from the cove.
    Add(TealInstances, Location - Side * 258.0f * Scale + FVector(0.0f, 0.0f, 125.0f * Scale),
        FVector(0.12f, 1.55f, 2.15f) * Scale, Rotation);
    Add(CreamInstances, Location - Side * 290.0f * Scale + FVector(0.0f, 0.0f, 210.0f * Scale),
        FVector(0.10f, 0.46f, 0.80f) * Scale, Rotation);
    for (int32 Sign : {-1, 1})
    {
        const FVector Window = Location + Forward * Sign * 205.0f * Scale - Side * 267.0f * Scale
            + FVector(0.0f, 0.0f, 245.0f * Scale);
        Add(CreamInstances, Window, FVector(0.13f, 0.56f, 0.70f) * Scale, Rotation);
        Add(CoralInstances, Window - Side * 16.0f * Scale, FVector(0.07f, 0.43f, 0.50f) * Scale, Rotation);
    }
    // Offset vertical paint boards prevent the huts reading as a single brown primitive.
    for (int32 Board = -2; Board <= 2; ++Board)
    {
        Add(Board % 2 == 0 ? TealInstances : CoralInstances,
            Location + Forward * Board * 92.0f * Scale + Side * 264.0f * Scale + FVector(0.0f, 0.0f, 150.0f * Scale),
            FVector(0.48f, 0.08f, 2.35f) * Scale, Rotation);
    }
}

void ACoastalDressing::AddRouteEntrance(const FCoastalSceneLayout& Layout)
{
    if (Layout.RouteWaypoints.Num() < 3)
    {
        return;
    }
    const FVector Direction = (Layout.RouteWaypoints[2] - Layout.RouteWaypoints[1]).GetSafeNormal2D();
    const FVector Side(-Direction.Y, Direction.X, 0.0f);
    // Repeated compact markers establish a deliberate low-tide road without occupying the walk lane.
    for (int32 Marker = 0; Marker < 4; ++Marker)
    {
        const float Alpha = 0.18f + Marker * 0.19f;
        const FVector Center = FMath::Lerp(Layout.RouteWaypoints[1], Layout.RouteWaypoints[2], Alpha);
        const FVector MarkerBase = Center + Side * (Marker % 2 == 0 ? 610.0f : -610.0f);
        AddBeam(MarketInstances, MarkerBase, MarkerBase + FVector(0.0f, 0.0f, 245.0f), 12.0f);
        Add(Marker % 2 == 0 ? CoralInstances : TealInstances, MarkerBase + FVector(0.0f, 0.0f, 225.0f),
            FVector(0.22f, 0.22f, 0.22f));
    }
}

void ACoastalDressing::AddSignalStationHero(const FCoastalSceneLayout& Layout)
{
    const FVector Location = Layout.RouteWaypoints.Last() + FVector(900.0f, 900.0f, 20.0f);
    const FRotator StationRotation(0.0f, 22.0f, 0.0f);
    const FVector Back = StationRotation.RotateVector(FVector::ForwardVector);
    const FVector Side = StationRotation.RotateVector(FVector::RightVector);

    // Three-sided signal room: the front and central desk sightline remain completely open.
    Add(CreamInstances, Location + Back * 330.0f + FVector(0.0f, 0.0f, 620.0f),
        FVector(0.18f, 5.6f, 7.9f), StationRotation);
    Add(TealInstances, Location + Back * 340.0f + FVector(0.0f, 0.0f, 275.0f),
        FVector(0.10f, 5.72f, 1.0f), StationRotation);
    for (int32 SideSign : {-1, 1})
    {
        const FVector WallCenter = Location + Side * SideSign * 270.0f + Back * 115.0f + FVector(0.0f, 0.0f, 620.0f);
        Add(CreamInstances, WallCenter, FVector(4.3f, 0.18f, 7.9f), StationRotation);
        Add(TealInstances, WallCenter - Back * 12.0f + FVector(0.0f, 0.0f, -340.0f),
            FVector(4.42f, 0.10f, 1.0f), StationRotation);
        Add(CoralInstances, Location + Side * SideSign * 282.0f - Back * 105.0f + FVector(0.0f, 0.0f, 610.0f),
            FVector(0.12f, 1.05f, 1.35f), StationRotation);
    }
    for (int32 Brace = -2; Brace <= 2; ++Brace)
    {
        Add(MarketInstances, Location + Back * 470.0f + Side * Brace * 145.0f + FVector(0.0f, 0.0f, 530.0f),
            FVector(0.11f, 0.11f, 6.25f), FRotator::ZeroRotator);
    }

    // Tiered crown and offset semaphore make the objective readable from the opening route.
    Add(TealInstances, Location + FVector(0.0f, 0.0f, 1140.0f), FVector(3.4f, 3.4f, 0.34f), StationRotation);
    Add(CreamInstances, Location + FVector(0.0f, 0.0f, 1225.0f), FVector(2.35f, 2.35f, 1.35f), StationRotation);
    Add(CoralInstances, Location + FVector(0.0f, 0.0f, 1302.0f), FVector(2.65f, 2.65f, 0.18f), StationRotation);
    const FVector MastBase = Location + FVector(0.0f, 0.0f, 1302.0f);
    AddBeam(MarketInstances, MastBase, MastBase + FVector(0.0f, 0.0f, 920.0f), 24.0f);
    AddBeam(MarketInstances, MastBase + FVector(0.0f, 0.0f, 550.0f) - Side * 390.0f,
        MastBase + FVector(0.0f, 0.0f, 550.0f) + Side * 390.0f, 15.0f);
    AddBeam(CoralInstances, MastBase + FVector(0.0f, 0.0f, 735.0f),
        MastBase + Back * 510.0f + FVector(0.0f, 0.0f, 930.0f), 13.0f);
    Add(CoralInstances, MastBase + Back * 425.0f + FVector(0.0f, 0.0f, 900.0f),
        FVector(0.10f, 1.05f, 0.48f), FRotator(0.0f, StationRotation.Yaw, 12.0f));
    Add(TealInstances, MastBase - Side * 265.0f + FVector(0.0f, 0.0f, 490.0f),
        FVector(0.10f, 0.72f, 0.38f), StationRotation);
}

void ACoastalDressing::AddShrineHero(const FCoastalSceneLayout& Layout)
{
    const FVector Pickup = Layout.RareArtifactLocation;
    const FVector Approach = (Layout.OptionalRouteWaypoints[Layout.OptionalRouteWaypoints.Num() - 2] - Pickup).GetSafeNormal2D();
    const FVector Back = -Approach;
    const FVector Side(-Approach.Y, Approach.X, 0.0f);
    const FVector ArchCenter = Pickup + Back * 510.0f;

    // An asymmetric broken arch cradles the real artifact from behind and keeps the approach/pickup clear.
    Add(BasaltInstances, ArchCenter + Side * 390.0f + FVector(0.0f, 0.0f, 260.0f),
        FVector(2.6f, 2.1f, 7.2f), FRotator(-8.0f, Approach.Rotation().Yaw + 12.0f, -10.0f));
    Add(BasaltInstances, ArchCenter - Side * 420.0f + FVector(0.0f, 0.0f, 205.0f),
        FVector(2.2f, 1.8f, 5.7f), FRotator(13.0f, Approach.Rotation().Yaw - 18.0f, 9.0f));
    const FVector LeftFoot = ArchCenter + Side * 345.0f;
    const FVector RightFoot = ArchCenter - Side * 305.0f;
    const FVector LeftTop = LeftFoot + FVector(0.0f, 0.0f, 610.0f);
    const FVector Crown = ArchCenter - Side * 95.0f + Back * 35.0f + FVector(0.0f, 0.0f, 735.0f);
    const FVector RightTop = RightFoot + FVector(0.0f, 0.0f, 545.0f);
    AddBeam(VioletInstances, LeftFoot, LeftTop, 72.0f);
    AddBeam(VioletInstances, RightFoot, RightTop, 64.0f);
    AddBeam(VioletInstances, LeftTop, Crown, 82.0f);
    AddBeam(VioletInstances, Crown, RightTop, 70.0f);
    Add(BasaltInstances, ArchCenter - Side * 500.0f + Back * 35.0f + FVector(0.0f, 0.0f, 570.0f),
        FVector(1.35f, 1.15f, 2.5f), FRotator(24.0f, Approach.Rotation().Yaw, 18.0f));

    // Low steps focus the approach while inset color belongs to the architecture, not a second collectible.
    for (int32 Step = 0; Step < 3; ++Step)
    {
        Add(BasaltInstances, Pickup + Back * (100.0f + Step * 75.0f) + FVector(0.0f, 0.0f, -58.0f + Step * 15.0f),
            FVector(4.2f - Step * 0.55f, 3.4f - Step * 0.45f, 0.70f), Approach.Rotation());
    }
    for (int32 Rune = -2; Rune <= 2; ++Rune)
    {
        Add(Rune % 2 == 0 ? TealInstances : VioletInstances,
            Pickup + Side * Rune * 120.0f + Back * 260.0f + FVector(0.0f, 0.0f, -12.0f),
            FVector(0.45f, 0.12f, 0.06f), FRotator(0.0f, Approach.Rotation().Yaw, Rune * 18.0f));
    }
}

void ACoastalDressing::AddBasaltSkirt(const FVector& Center, const FVector& Extent, int32 Seed)
{
    FRandomStream Random(Seed);
    for (int32 Index = 0; Index < 14; ++Index)
    {
        const FVector Location = Center + FVector(Random.FRandRange(-Extent.X, Extent.X),
            Random.FRandRange(-Extent.Y, Extent.Y), Random.FRandRange(-75.0f, 45.0f));
        Add(BasaltInstances, Location, FVector(Random.FRandRange(1.2f, 2.7f), Random.FRandRange(1.1f, 2.4f),
            Random.FRandRange(1.7f, 4.0f)), FRotator(Random.FRandRange(-12.0f, 12.0f),
                Random.FRandRange(0.0f, 180.0f), Random.FRandRange(-12.0f, 12.0f)));
    }
}

void ACoastalDressing::AddGrassClump(const FVector& Location, float Scale, float YawDegrees)
{
    for (int32 Blade = 0; Blade < 5; ++Blade)
    {
        Add(GrassInstances, Location + FVector((Blade - 2) * 21.0f, (Blade % 2) * 18.0f, 62.0f * Scale),
            FVector(0.10f, 0.12f, (0.72f + Blade * 0.08f) * Scale),
            FRotator(-18.0f + Blade * 8.0f, YawDegrees + Blade * 34.0f, 0.0f));
    }
}

void ACoastalDressing::BuildDressing(const FCoastalSceneLayout& Layout)
{
    if (bBuilt)
    {
        return;
    }
    bBuilt = true;
    ConfigureFamily(CreamInstances, CubeMesh, FLinearColor(0.88f, 0.76f, 0.55f), 0.78f);
    ConfigureFamily(TealInstances, CubeMesh, FLinearColor(0.055f, 0.38f, 0.39f), 0.72f);
    ConfigureFamily(CoralInstances, CubeMesh, FLinearColor(0.78f, 0.25f, 0.17f), 0.76f);
    ConfigureFamily(VioletInstances, CubeMesh, FLinearColor(0.12f, 0.085f, 0.19f), 0.80f);
    ConfigureFamily(GrassInstances, ConeMesh, FLinearColor(0.20f, 0.42f, 0.25f), 0.92f);
    ConfigureFamily(MarketInstances, CylinderMesh, FLinearColor(0.38f, 0.20f, 0.09f), 0.86f);
    ConfigureFamily(BasaltInstances, FacetedRockMesh, FLinearColor(0.12f, 0.16f, 0.18f), 0.96f);
    ConfigureFamily(DistantCliffInstances, FacetedRockMesh, FLinearColor(0.17f, 0.25f, 0.27f), 0.97f);

    AddHutFinish(FVector(-1100.0f, -1650.0f, 150.0f), 18.0f, 1.0f);
    AddHutFinish(FVector(1250.0f, -2100.0f, 145.0f), -20.0f, 0.82f);
    AddHutFinish(FVector(-1550.0f, 900.0f, 155.0f), 125.0f, 0.75f);
    AddRouteEntrance(Layout);
    AddSignalStationHero(Layout);
    AddShrineHero(Layout);

    AddBasaltSkirt(FVector(-2350.0f, -2450.0f, 20.0f), FVector(650.0f, 520.0f, 0.0f), 113);
    AddBasaltSkirt(FVector(2350.0f, 2450.0f, 20.0f), FVector(420.0f, 300.0f, 0.0f), 197);
    for (int32 Index = 0; Index < 18; ++Index)
    {
        const float Angle = Index * UE_TWO_PI / 18.0f;
        AddGrassClump(FVector(FMath::Cos(Angle) * (1900.0f + (Index % 3) * 350.0f),
            FMath::Sin(Angle) * (1850.0f + (Index % 4) * 260.0f), 70.0f), 0.72f + (Index % 3) * 0.12f, Index * 27.0f);
    }

    // Far offshore silhouettes add a horizon layer without changing the walkable scene or collision queries.
    for (int32 Index = 0; Index < 7; ++Index)
    {
        Add(DistantCliffInstances, FVector(-9000.0f + Index * 2600.0f, 9700.0f + (Index % 2) * 750.0f, 150.0f),
            FVector(8.0f + (Index % 3) * 2.0f, 5.0f + (Index % 2), 7.0f + (Index % 4) * 1.2f),
            FRotator(-4.0f, Index * 31.0f, 5.0f));
    }

    UE_LOG(LogTemp, Display, TEXT("LOW TIDE M1 visual dressing built: %d non-colliding instances."),
        CreamInstances->GetInstanceCount() + TealInstances->GetInstanceCount() + CoralInstances->GetInstanceCount()
            + VioletInstances->GetInstanceCount() + GrassInstances->GetInstanceCount() + MarketInstances->GetInstanceCount()
            + BasaltInstances->GetInstanceCount()
            + DistantCliffInstances->GetInstanceCount());
}
