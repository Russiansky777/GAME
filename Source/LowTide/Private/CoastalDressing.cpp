#include "CoastalDressing.h"

#include "CoastalScene.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ACoastalDressing::ACoastalDressing()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DressingRoot"));
    SetRootComponent(SceneRoot);

    CreamInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CreamTrim"));
    TealInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TealPaint"));
    CoralInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CoralPaint"));
    GrassInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("GrassClumps"));
    MarketInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("MarketProps"));
    BasaltInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("BasaltSkirts"));
    DistantCliffInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("DistantCliffs"));

    UHierarchicalInstancedStaticMeshComponent* Families[] = {
        CreamInstances, TealInstances, CoralInstances, GrassInstances, MarketInstances, BasaltInstances, DistantCliffInstances
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

void ACoastalDressing::AddMarketCluster(const FVector& Location, float YawDegrees)
{
    const FRotator Rotation(0.0f, YawDegrees, 0.0f);
    const FVector Forward = Rotation.RotateVector(FVector::ForwardVector);
    const FVector Side = Rotation.RotateVector(FVector::RightVector);
    // Crates, barrel stacks and a low coiled-rope motif give Mara's canopy a useful visual rhythm.
    Add(MarketInstances, Location + Forward * 340.0f + Side * 220.0f + FVector(0.0f, 0.0f, 58.0f),
        FVector(0.78f, 0.78f, 0.58f), Rotation);
    Add(MarketInstances, Location + Forward * 430.0f + Side * 225.0f + FVector(0.0f, 0.0f, 142.0f),
        FVector(0.62f, 0.62f, 0.62f), Rotation);
    Add(MarketInstances, Location - Forward * 300.0f + Side * 220.0f + FVector(0.0f, 0.0f, 70.0f),
        FVector(0.58f, 0.58f, 0.70f), Rotation);
    Add(CoralInstances, Location + Forward * 60.0f - Side * 315.0f + FVector(0.0f, 0.0f, 50.0f),
        FVector(1.35f, 1.35f, 0.12f), FRotator(90.0f, YawDegrees, 0.0f));
    for (int32 Index = 0; Index < 5; ++Index)
    {
        const float Angle = Index * UE_TWO_PI / 5.0f;
        Add(TealInstances, Location + Forward * 55.0f - Side * 315.0f
                + FVector(FMath::Cos(Angle) * 88.0f, FMath::Sin(Angle) * 88.0f, 48.0f),
            FVector(0.13f, 0.13f, 0.13f));
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
    ConfigureFamily(GrassInstances, ConeMesh, FLinearColor(0.20f, 0.42f, 0.25f), 0.92f);
    ConfigureFamily(MarketInstances, CylinderMesh, FLinearColor(0.38f, 0.20f, 0.09f), 0.86f);
    ConfigureFamily(BasaltInstances, FacetedRockMesh, FLinearColor(0.12f, 0.16f, 0.18f), 0.96f);
    ConfigureFamily(DistantCliffInstances, FacetedRockMesh, FLinearColor(0.17f, 0.25f, 0.27f), 0.97f);

    AddHutFinish(FVector(-1100.0f, -1650.0f, 150.0f), 18.0f, 1.0f);
    AddHutFinish(FVector(1250.0f, -2100.0f, 145.0f), -20.0f, 0.82f);
    AddHutFinish(FVector(-1550.0f, 900.0f, 155.0f), 125.0f, 0.75f);
    AddMarketCluster(FVector(900.0f, -700.0f, 150.0f), 10.0f);

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
            + GrassInstances->GetInstanceCount() + MarketInstances->GetInstanceCount() + BasaltInstances->GetInstanceCount()
            + DistantCliffInstances->GetInstanceCount());
}
