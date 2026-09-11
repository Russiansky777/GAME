#include "CoastalScene.h"

#include "Components/BoxComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "TraderActor.h"
#include "UObject/ConstructorHelpers.h"

namespace CoastalScene
{
    constexpr float CubeSize = 100.0f;
    // Keep the floor slightly beneath the route boundaries so their inner faces never expose invisible footing.
    constexpr float RouteFloorWidthScale = 1.16f;

    FRotator FacingRotation(const FVector& From, const FVector& Toward)
    {
        return (Toward - From).Rotation() + FRotator(0.0f, 180.0f, 0.0f);
    }
}

ACoastalScene::ACoastalScene()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    SandInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("SandFamily"));
    StoneInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("StoneFamily"));
    CliffInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CliffFamily"));
    DarkRockInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("DarkRockFamily"));
    DeepGroundInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("DeepGroundFamily"));
    WoodInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("WoodFamily"));
    WoodDetailInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("WoodDetailFamily"));
    RoofInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RoofFamily"));
    RopeInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RopeFamily"));
    MetalInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("MetalFamily"));
    WarmAccentInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("WarmAccentFamily"));
    TideAccentInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TideAccentFamily"));
    AnomalyInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("AnomalyFamily"));
    RouteBoundaryInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RouteBoundaryFamily"));
    RouteFloorCollision = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RouteFloorCollision"));
    CoastalTerrainSand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoastalTerrainSand"));
    CoastalTerrainStone = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoastalTerrainStone"));
    CoastalTerrainDeep = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoastalTerrainDeep"));

    UHierarchicalInstancedStaticMeshComponent* Families[] = {
        SandInstances, StoneInstances, CliffInstances, DarkRockInstances, DeepGroundInstances, WoodInstances,
        WoodDetailInstances, RoofInstances,
        RopeInstances, MetalInstances, WarmAccentInstances, TideAccentInstances, AnomalyInstances
    };
    for (UHierarchicalInstancedStaticMeshComponent* Family : Families)
    {
        Family->SetupAttachment(SceneRoot);
        Family->SetCullDistances(25000, 90000);
        Family->SetCastShadow(true);
    }
    RouteBoundaryInstances->SetupAttachment(SceneRoot);
    RouteBoundaryInstances->SetCastShadow(false);
    RouteBoundaryInstances->SetHiddenInGame(true);
    RouteBoundaryInstances->ComponentTags.Add(TEXT("M1Boundary"));
    UStaticMeshComponent* TerrainSections[] = { CoastalTerrainSand, CoastalTerrainStone, CoastalTerrainDeep };
    for (UStaticMeshComponent* TerrainSection : TerrainSections)
    {
        TerrainSection->SetupAttachment(SceneRoot);
        TerrainSection->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        TerrainSection->SetCastShadow(true);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> FacetedRockAsset(
        TEXT("/Game/Generated/M1/SM_LT_FacetedRock.SM_LT_FacetedRock"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CoastalTerrainSandAsset(
        TEXT("/Game/Generated/M1/SM_LT_CoastalTerrainSand.SM_LT_CoastalTerrainSand"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CoastalTerrainStoneAsset(
        TEXT("/Game/Generated/M1/SM_LT_CoastalTerrainStone.SM_LT_CoastalTerrainStone"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CoastalTerrainDeepAsset(
        TEXT("/Game/Generated/M1/SM_LT_CoastalTerrainDeep.SM_LT_CoastalTerrainDeep"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> AuthoredMaterial(
        TEXT("/Game/Generated/M1/M_LT_StylizedOpaque.M_LT_StylizedOpaque"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> AuthoredWater(
        TEXT("/Game/Generated/M1/M_LT_StylizedWater.M_LT_StylizedWater"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackMaterial(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

    CubeMesh = CubeAsset.Object;
    CylinderMesh = CylinderAsset.Object;
    SphereMesh = SphereAsset.Object;
    ConeMesh = ConeAsset.Object;
    FacetedRockMesh = FacetedRockAsset.Succeeded() ? FacetedRockAsset.Object : SphereAsset.Object;
    CoastalTerrainSandMesh = CoastalTerrainSandAsset.Object;
    CoastalTerrainStoneMesh = CoastalTerrainStoneAsset.Object;
    CoastalTerrainDeepMesh = CoastalTerrainDeepAsset.Object;
    CoastalTerrainSand->SetStaticMesh(CoastalTerrainSandMesh);
    CoastalTerrainStone->SetStaticMesh(CoastalTerrainStoneMesh);
    CoastalTerrainDeep->SetStaticMesh(CoastalTerrainDeepMesh);
    StylizedMaterial = AuthoredMaterial.Succeeded() ? AuthoredMaterial.Object : FallbackMaterial.Object;
    WaterMaterial = AuthoredWater.Succeeded() ? AuthoredWater.Object : StylizedMaterial;
    RouteBoundaryInstances->SetStaticMesh(CubeMesh);
    RouteBoundaryInstances->SetCollisionProfileName(TEXT("BlockAll"));
    RouteBoundaryInstances->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    RouteFloorCollision->SetupAttachment(SceneRoot);
    RouteFloorCollision->SetStaticMesh(CubeMesh);
    RouteFloorCollision->SetCollisionProfileName(TEXT("BlockAll"));
    RouteFloorCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    RouteFloorCollision->SetCastShadow(false);
    RouteFloorCollision->SetHiddenInGame(true);
    RouteFloorCollision->SetVisibility(false);
    RouteFloorCollision->ComponentTags.Add(TEXT("M1RouteFloor"));
}

void ACoastalScene::BuildScene()
{
    if (bBuilt || !GetWorld())
    {
        return;
    }
    bBuilt = true;

    InitializeLayout();

    ConfigureInstanceFamily(SandInstances, CubeMesh, FLinearColor(0.56f, 0.42f, 0.25f), 0.92f, true);
    ConfigureInstanceFamily(StoneInstances, CubeMesh, FLinearColor(0.30f, 0.34f, 0.32f), 0.88f, true);
    // Authored rocks conceal the invisible route boundary. Collision stays on the filtered boundary union,
    // avoiding random decorative mesh bounds narrowing the playable ribbon.
    ConfigureInstanceFamily(CliffInstances, FacetedRockMesh, FLinearColor(0.24f, 0.29f, 0.28f), 0.96f, false);
    ConfigureInstanceFamily(DarkRockInstances, FacetedRockMesh, FLinearColor(0.075f, 0.105f, 0.13f), 0.9f, false);
    ConfigureInstanceFamily(DeepGroundInstances, CubeMesh, FLinearColor(0.11f, 0.14f, 0.15f), 0.94f, true);
    ConfigureInstanceFamily(WoodInstances, CubeMesh, FLinearColor(0.50f, 0.27f, 0.11f), 0.82f, true);
    ConfigureInstanceFamily(WoodDetailInstances, CubeMesh, FLinearColor(0.50f, 0.27f, 0.11f), 0.82f, false);
    ConfigureInstanceFamily(RoofInstances, CubeMesh, FLinearColor(0.14f, 0.19f, 0.20f), 0.9f, true);
    ConfigureInstanceFamily(RopeInstances, CylinderMesh, FLinearColor(0.31f, 0.23f, 0.13f), 0.98f, false);
    ConfigureInstanceFamily(MetalInstances, CylinderMesh, FLinearColor(0.24f, 0.31f, 0.32f), 0.62f, true);
    ConfigureInstanceFamily(WarmAccentInstances, SphereMesh, FLinearColor(0.95f, 0.42f, 0.09f), 0.58f, false);
    ConfigureInstanceFamily(TideAccentInstances, CubeMesh, FLinearColor(0.10f, 0.58f, 0.66f), 0.45f, false);
    ConfigureInstanceFamily(AnomalyInstances, ConeMesh, FLinearColor(0.20f, 0.85f, 0.70f), 0.34f, false);
    const auto ConfigureTerrainSection = [&](UStaticMeshComponent* Section, const FLinearColor& Color)
    {
        if (Section && StylizedMaterial)
        {
            UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(StylizedMaterial, this);
            Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
            Material->SetScalarParameterValue(TEXT("Roughness"), 0.96f);
            Section->SetMaterial(0, Material);
        }
    };
    ConfigureTerrainSection(CoastalTerrainSand, FLinearColor(0.56f, 0.42f, 0.25f));
    ConfigureTerrainSection(CoastalTerrainStone, FLinearColor(0.30f, 0.34f, 0.32f));
    ConfigureTerrainSection(CoastalTerrainDeep, FLinearColor(0.11f, 0.14f, 0.15f));

    Layout.WaterActor = SpawnWater();
    Layout.ShortcutBlocker = SpawnShortcutBlocker();
    BuildLighting();
    BuildSettlement();
    BuildMainRoute();
    BuildAlternateRoute();
    BuildOptionalRoute();

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Layout.Mara = GetWorld()->SpawnActor<ATraderActor>(FVector(900.0f, -1350.0f, 215.0f),
        FRotator(0.0f, 145.0f, 0.0f), SpawnParameters);
    if (Layout.Mara)
    {
        Layout.Mara->Tags.Add(TEXT("Mara"));
        SpawnedActors.Add(Layout.Mara);

        auto AddMaraPart = [&](UStaticMesh* Mesh, const FVector& WorldLocation, const FVector& Scale,
            const FRotator& Rotation, const FLinearColor& Color, const TCHAR* Name)
        {
            UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Layout.Mara, FName(Name));
            Layout.Mara->AddInstanceComponent(Part);
            Part->SetupAttachment(Layout.Mara->GetRootComponent());
            Part->SetAbsolute(true, true, true);
            Part->SetStaticMesh(Mesh);
            Part->SetWorldLocation(WorldLocation);
            Part->SetWorldRotation(Rotation);
            Part->SetWorldScale3D(Scale);
            Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            if (StylizedMaterial)
            {
                UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(StylizedMaterial, Layout.Mara);
                Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
                Material->SetVectorParameterValue(TEXT("Color"), Color);
                Material->SetScalarParameterValue(TEXT("Roughness"), 0.78f);
                Part->SetMaterial(0, Material);
            }
            Part->RegisterComponent();
        };
        const FVector MaraLocation = Layout.Mara->GetActorLocation();
        AddMaraPart(SphereMesh, MaraLocation + FVector(0.0f, 0.0f, 125.0f), FVector(0.46f, 0.46f, 0.50f),
            FRotator::ZeroRotator, FLinearColor(0.72f, 0.43f, 0.27f), TEXT("MaraHead"));
        AddMaraPart(ConeMesh, MaraLocation + FVector(0.0f, 0.0f, 182.0f), FVector(0.82f, 0.82f, 0.48f),
            FRotator::ZeroRotator, FLinearColor(0.10f, 0.19f, 0.22f), TEXT("MaraSouWester"));
        AddMaraPart(CubeMesh, MaraLocation + FVector(0.0f, 0.0f, 22.0f), FVector(0.62f, 0.58f, 0.82f),
            FRotator(0.0f, 145.0f, 0.0f), FLinearColor(0.82f, 0.33f, 0.12f), TEXT("MaraApron"));
        BuildTraderHub();
    }

    Layout.RareArtifactVisual = CreateRareArtifactVisual(Layout.RareArtifactLocation);
    Layout.PhenomenonVisual = CreatePhenomenonVisual(Layout.PhenomenonStartLocation);

    UE_LOG(LogTemp, Display,
        TEXT("LOW TIDE M1 coastal scene built: main loop %.0fm, optional detour %.0fm, %d instanced pieces, %d backup boundaries."),
        Layout.MainRouteLengthCm / 100.0f, Layout.OptionalRouteLengthCm / 100.0f,
        SandInstances->GetInstanceCount() + StoneInstances->GetInstanceCount() + CliffInstances->GetInstanceCount()
            + DarkRockInstances->GetInstanceCount() + DeepGroundInstances->GetInstanceCount()
            + WoodInstances->GetInstanceCount() + WoodDetailInstances->GetInstanceCount() + RoofInstances->GetInstanceCount()
            + RopeInstances->GetInstanceCount() + MetalInstances->GetInstanceCount()
            + WarmAccentInstances->GetInstanceCount() + TideAccentInstances->GetInstanceCount()
            + AnomalyInstances->GetInstanceCount(),
        BoundaryComponents.Num() + RouteBoundaryInstances->GetInstanceCount());
}

void ACoastalScene::InitializeLayout()
{
    Layout.PlayerStart = FVector(0.0f, 0.0f, 220.0f);
    Layout.RouteWaypoints = {
        FVector(0.0f, 0.0f, 140.0f), FVector(2000.0f, -800.0f, 90.0f),
        FVector(4200.0f, -4800.0f, 30.0f), FVector(7200.0f, -6400.0f, -20.0f),
        FVector(8800.0f, -2000.0f, -60.0f), FVector(11200.0f, 1800.0f, -30.0f),
        FVector(13400.0f, -1200.0f, -55.0f), FVector(15600.0f, 4800.0f, 100.0f),
        FVector(18400.0f, 2200.0f, -80.0f), FVector(20200.0f, -3200.0f, -100.0f),
        FVector(22000.0f, -1400.0f, 90.0f)
    };
    Layout.AlternateRouteWaypoints = {
        Layout.RouteWaypoints.Last(), FVector(20500.0f, 3000.0f, 300.0f),
        FVector(17400.0f, 6900.0f, 520.0f), FVector(13200.0f, 6700.0f, 420.0f),
        FVector(9400.0f, 4100.0f, 330.0f), FVector(6000.0f, -300.0f, 210.0f),
        FVector(3200.0f, 1600.0f, 180.0f), Layout.RouteWaypoints[0]
    };
    Layout.OptionalRouteWaypoints = {
        Layout.RouteWaypoints.Last(), FVector(21000.0f, -3000.0f, -65.0f),
        FVector(18500.0f, -5500.0f, -105.0f), FVector(18500.0f, -7300.0f, -125.0f),
        FVector(22500.0f, -7300.0f, -130.0f), FVector(22500.0f, -5000.0f, -115.0f),
        FVector(25000.0f, -5800.0f, -20.0f)
    };

    Layout.MainObjectiveLocation = Layout.RouteWaypoints.Last() + FVector(900.0f, 900.0f, 275.0f);
    Layout.RareArtifactLocation = Layout.OptionalRouteWaypoints.Last() + FVector(0.0f, 0.0f, 15.0f);
    Layout.PhenomenonStartLocation = FVector(21000.0f, -3500.0f, 80.0f);
    Layout.CommonSalvageLocations = {
        FVector(5000.0f, -5230.0f, 120.0f), FVector(9100.0f, -1525.0f, 65.0f),
        FVector(14700.0f, 5100.0f, 205.0f), FVector(18100.0f, 2480.0f, 15.0f),
        FVector(20100.0f, -2900.0f, -5.0f)
    };
    Layout.RouteClueLocations = {
        FVector(7100.0f, -6100.0f, 130.0f), FVector(15500.0f, 4500.0f, 205.0f),
        Layout.MainObjectiveLocation
    };
    Layout.WardLocations = {
        FVector(20750.0f, -3000.0f, 40.0f), FVector(18800.0f, -6900.0f, -5.0f),
        FVector(22600.0f, -5200.0f, -5.0f)
    };
    Layout.MainRouteLengthCm = CalculatePolylineLength(Layout.RouteWaypoints)
        + CalculatePolylineLength(Layout.AlternateRouteWaypoints);
    Layout.OptionalRouteLengthCm = 2.0f * CalculatePolylineLength(Layout.OptionalRouteWaypoints);
    Layout.SettlementSafeBounds = FBox(FVector(-3200.0f, -3600.0f, -250.0f), FVector(3300.0f, 3200.0f, 1700.0f));
    Layout.ExpeditionBounds = FBox(FVector(-3500.0f, -9000.0f, -600.0f), FVector(28000.0f, 9000.0f, 2200.0f));

    Layout.ReviewViews = {
        FVector(-2400.0f, 2700.0f, 1150.0f), FVector(6000.0f, -1200.0f, 900.0f),
        FVector(13900.0f, 7300.0f, 1200.0f), FVector(19000.0f, 1200.0f, 1150.0f),
        FVector(23800.0f, -8200.0f, 850.0f)
    };
    Layout.ReviewLookAt = {
        FVector(0.0f, -500.0f, 250.0f), FVector(7200.0f, -6400.0f, 250.0f),
        FVector(15600.0f, 4800.0f, 250.0f), FVector(22900.0f, -500.0f, 450.0f),
        FVector(25000.0f, -5800.0f, 200.0f)
    };
}

void ACoastalScene::ConfigureInstanceFamily(UHierarchicalInstancedStaticMeshComponent* Component,
    UStaticMesh* Mesh, const FLinearColor& Color, float Roughness, bool bCollision)
{
    if (!Component || !Mesh)
    {
        return;
    }
    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    Component->SetCollisionProfileName(bCollision ? TEXT("BlockAll") : TEXT("NoCollision"));
    if (StylizedMaterial)
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(StylizedMaterial, this);
        Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
        Material->SetVectorParameterValue(TEXT("Color"), Color);
        Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
        Material->SetScalarParameterValue(TEXT("Specular"), 0.18f);
        Component->SetMaterial(0, Material);
    }
}

void ACoastalScene::AddInstance(UHierarchicalInstancedStaticMeshComponent* Component, const FVector& Location,
    const FVector& Scale, const FRotator& Rotation)
{
    if (Component)
    {
        Component->AddInstance(FTransform(Rotation, Location, Scale));
    }
}

void ACoastalScene::AddBoxBetween(UHierarchicalInstancedStaticMeshComponent* Component, const FVector& Start,
    const FVector& End, float Width, float Height, float ZOffset)
{
    const FVector Delta = End - Start;
    const float Length = Delta.Size();
    if (Length < 1.0f)
    {
        return;
    }
    const FVector Midpoint = (Start + End) * 0.5f + FVector(0.0f, 0.0f, ZOffset);
    const float HorizontalLength = FVector(Delta.X, Delta.Y, 0.0f).Size();
    // FRotator pitch raises local +X for positive values, matching the segment's signed Z delta.
    const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(Delta.Z, HorizontalLength));
    const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    AddInstance(Component, Midpoint, FVector(Length / CoastalScene::CubeSize, Width / CoastalScene::CubeSize,
        Height / CoastalScene::CubeSize), FRotator(Pitch, Yaw, 0.0f));
}

void ACoastalScene::AddCylinderBetween(UHierarchicalInstancedStaticMeshComponent* Component, const FVector& Start,
    const FVector& End, float Radius)
{
    const FVector Delta = End - Start;
    if (Delta.IsNearlyZero())
    {
        return;
    }
    AddInstance(Component, (Start + End) * 0.5f,
        FVector(Radius / 50.0f, Radius / 50.0f, Delta.Size() / CoastalScene::CubeSize),
        FRotationMatrix::MakeFromZ(Delta).Rotator());
}

UBoxComponent* ACoastalScene::AddBoundarySegment(const FVector& Start, const FVector& End, float ZBase,
    float Height, FName Name)
{
    const FVector FlatDelta(End.X - Start.X, End.Y - Start.Y, 0.0f);
    const float Length = FlatDelta.Size();
    if (Length < 50.0f)
    {
        return nullptr;
    }

    UBoxComponent* Boundary = NewObject<UBoxComponent>(this, MakeUniqueObjectName(this, UBoxComponent::StaticClass(), Name));
    AddInstanceComponent(Boundary);
    Boundary->SetupAttachment(SceneRoot);
    Boundary->SetBoxExtent(FVector(Length * 0.5f, 50.0f, Height * 0.5f));
    Boundary->SetRelativeLocation(FVector((Start.X + End.X) * 0.5f, (Start.Y + End.Y) * 0.5f, ZBase + Height * 0.5f));
    Boundary->SetRelativeRotation(FRotator(0.0f, FlatDelta.Rotation().Yaw, 0.0f));
    Boundary->SetCollisionProfileName(TEXT("BlockAll"));
    Boundary->ComponentTags.Add(TEXT("M1Boundary"));
    Boundary->SetHiddenInGame(true);
    Boundary->SetVisibility(false);
    Boundary->RegisterComponent();
    BoundaryComponents.Add(Boundary);
    return Boundary;
}

void ACoastalScene::AddFilteredRouteBoundary(const FVector& Start, const FVector& End,
    const FVector& OwnRouteStart, const FVector& OwnRouteEnd, float Height)
{
    constexpr float ChunkLengthCm = 120.0f;
    const FVector Delta = End - Start;
    const float FlatLength = FVector2D(Delta.X, Delta.Y).Size();
    if (!RouteBoundaryInstances || FlatLength < 1.0f)
    {
        return;
    }

    const int32 ChunkCount = FMath::Max(1, FMath::CeilToInt(FlatLength / ChunkLengthCm));
    for (int32 ChunkIndex = 0; ChunkIndex < ChunkCount; ++ChunkIndex)
    {
        const float StartAlpha = static_cast<float>(ChunkIndex) / ChunkCount;
        const float EndAlpha = static_cast<float>(ChunkIndex + 1) / ChunkCount;
        const FVector ChunkStart = FMath::Lerp(Start, End, StartAlpha);
        const FVector ChunkEnd = FMath::Lerp(Start, End, EndAlpha);
        const FVector Midpoint = (ChunkStart + ChunkEnd) * 0.5f;
        if (ShouldOmitRouteBoundary(Midpoint, OwnRouteStart, OwnRouteEnd))
        {
            continue;
        }

        const FVector ChunkDelta = ChunkEnd - ChunkStart;
        const float ChunkFlatLength = FVector2D(ChunkDelta.X, ChunkDelta.Y).Size();
        const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(ChunkDelta.Y, ChunkDelta.X));
        const float ZBase = FMath::Min(ChunkStart.Z, ChunkEnd.Z) - 80.0f;
        AddInstance(RouteBoundaryInstances,
            FVector(Midpoint.X, Midpoint.Y, ZBase + Height * 0.5f),
            FVector(ChunkFlatLength / CoastalScene::CubeSize, 1.0f, Height / CoastalScene::CubeSize),
            FRotator(0.0f, Yaw, 0.0f));
    }
}

bool ACoastalScene::ShouldOmitRouteBoundary(const FVector& Point, const FVector& OwnRouteStart,
    const FVector& OwnRouteEnd) const
{
    if (Layout.SettlementSafeBounds.IsInsideOrOn(Point))
    {
        return true;
    }

    const auto IsInsidePolyline = [&](const TArray<FVector>& Points, float Width)
    {
        const float Clearance = Width * 0.5f + 42.0f + 50.0f + 90.0f;
        for (int32 Index = 0; Index + 1 < Points.Num(); ++Index)
        {
            const FVector& SegmentStart = Points[Index];
            const FVector& SegmentEnd = Points[Index + 1];
            const bool bOwnSegment = (SegmentStart.Equals(OwnRouteStart, 1.0f) && SegmentEnd.Equals(OwnRouteEnd, 1.0f))
                || (SegmentStart.Equals(OwnRouteEnd, 1.0f) && SegmentEnd.Equals(OwnRouteStart, 1.0f));
            if (bOwnSegment)
            {
                continue;
            }

            const FVector2D SegmentDelta(SegmentEnd.X - SegmentStart.X, SegmentEnd.Y - SegmentStart.Y);
            const FVector2D ToPoint(Point.X - SegmentStart.X, Point.Y - SegmentStart.Y);
            const float Alpha = FMath::Clamp(FVector2D::DotProduct(ToPoint, SegmentDelta)
                / FMath::Max(1.0f, SegmentDelta.SizeSquared()), 0.0f, 1.0f);
            const FVector2D Closest = FVector2D(SegmentStart.X, SegmentStart.Y) + SegmentDelta * Alpha;
            const float RouteZ = FMath::Lerp(SegmentStart.Z, SegmentEnd.Z, Alpha);
            if (FVector2D::DistSquared(FVector2D(Point.X, Point.Y), Closest) <= FMath::Square(Clearance)
                && FMath::Abs(Point.Z - RouteZ) <= 520.0f)
            {
                return true;
            }
        }
        return false;
    };

    return IsInsidePolyline(Layout.RouteWaypoints, 900.0f)
        || IsInsidePolyline(Layout.AlternateRouteWaypoints, 820.0f)
        || IsInsidePolyline(Layout.OptionalRouteWaypoints, 760.0f);
}

AActor* ACoastalScene::SpawnMarkerActor(const FVector& Location, FName Tag)
{
    FActorSpawnParameters Parameters;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* Marker = GetWorld()->SpawnActor<AActor>(Location, FRotator::ZeroRotator, Parameters);
    if (!Marker)
    {
        return nullptr;
    }
    USceneComponent* Root = NewObject<USceneComponent>(Marker, TEXT("MarkerRoot"));
    Marker->AddInstanceComponent(Root);
    Marker->SetRootComponent(Root);
    Root->RegisterComponent();
    Marker->SetActorLocation(Location);
    Marker->Tags.Add(Tag);
    SpawnedActors.Add(Marker);
    return Marker;
}

AActor* ACoastalScene::SpawnWater()
{
    AActor* Water = SpawnMarkerActor(FVector(12500.0f, 0.0f, -140.0f), TEXT("M1Water"));
    if (!Water || !CubeMesh)
    {
        return Water;
    }

    UStaticMeshComponent* Surface = NewObject<UStaticMeshComponent>(Water, TEXT("WaterSurface"));
    Water->AddInstanceComponent(Surface);
    Surface->SetupAttachment(Water->GetRootComponent());
    Surface->SetStaticMesh(CubeMesh);
    Surface->SetRelativeScale3D(FVector(2200.0f, 2200.0f, 0.12f));
    Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Surface->SetMobility(EComponentMobility::Movable);
    if (WaterMaterial)
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(WaterMaterial, Water);
        Material->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.025f, 0.25f, 0.34f));
        Material->SetVectorParameterValue(TEXT("ShallowColor"), FLinearColor(0.10f, 0.48f, 0.54f));
        Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.025f, 0.25f, 0.34f));
        Material->SetScalarParameterValue(TEXT("Roughness"), 0.24f);
        Surface->SetMaterial(0, Material);
    }
    Surface->RegisterComponent();
    Water->GetRootComponent()->SetMobility(EComponentMobility::Movable);
    return Water;
}

AActor* ACoastalScene::SpawnShortcutBlocker()
{
    if (!CubeMesh)
    {
        return nullptr;
    }
    FActorSpawnParameters Parameters;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const FVector ShortcutDirection = Layout.RouteWaypoints[8] - Layout.RouteWaypoints[7];
    const float AcrossYaw = ShortcutDirection.Rotation().Yaw + 90.0f;
    AStaticMeshActor* Blocker = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(17000.0f, 3500.0f, 130.0f),
        FRotator(0.0f, AcrossYaw, 0.0f), Parameters);
    if (!Blocker)
    {
        return nullptr;
    }
    Blocker->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
    Blocker->SetActorScale3D(FVector(10.0f, 1.0f, 5.0f));
    Blocker->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
    Blocker->SetActorHiddenInGame(true);
    Blocker->Tags.Add(TEXT("M1Shortcut"));
    SpawnedActors.Add(Blocker);
    return Blocker;
}

void ACoastalScene::BuildLighting()
{
    FActorSpawnParameters Parameters;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector,
        FRotator(-38.0f, -28.0f, 0.0f), Parameters))
    {
        Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Sun->GetLightComponent()->SetIntensity(5.5f);
        Sun->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.78f, 0.56f));
        if (UDirectionalLightComponent* Directional = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
        {
            Directional->SetAtmosphereSunLight(true);
        }
        Sun->Tags.Add(TEXT("M1Sun"));
        SpawnedActors.Add(Sun);
    }

    if (ASkyAtmosphere* Atmosphere = GetWorld()->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector,
        FRotator::ZeroRotator, Parameters))
    {
        SpawnedActors.Add(Atmosphere);
    }
    if (ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator, Parameters))
    {
        Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Sky->GetLightComponent()->SetIntensity(1.35f);
        Sky->GetLightComponent()->RecaptureSky();
        SpawnedActors.Add(Sky);
    }
    if (AExponentialHeightFog* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>(FVector(12000.0f, 0.0f, -250.0f),
        FRotator::ZeroRotator, Parameters))
    {
        Fog->GetComponent()->SetFogDensity(0.0065f);
        Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(0.34f, 0.46f, 0.48f));
        SpawnedActors.Add(Fog);
    }
}

void ACoastalScene::BuildSettlement()
{
    // Collision-bearing settlement floor remains explicit while the authored terrain masks its outer silhouette.
    AddInstance(StoneInstances, FVector(0.0f, -200.0f, 35.0f), FVector(62.0f, 68.0f, 1.8f), FRotator(0.0f, -4.0f, 0.0f));

    AddRockCluster(FVector(-2200.0f, -2400.0f, 0.0f), FVector(1700.0f, 1400.0f, 500.0f), 17, false);
    AddRockCluster(FVector(2400.0f, 2600.0f, 0.0f), FVector(500.0f, 300.0f, 500.0f), 29, false);

    // Continuous hidden backup follows the shelf edge; the only landward exit is the authored southeast trail.
    AddBoundarySegment(FVector(-2750.0f, -3050.0f, 0.0f), FVector(-2750.0f, 2900.0f, 0.0f), -100.0f, 520.0f, TEXT("SettlementWest"));
    AddBoundarySegment(FVector(-2750.0f, 2900.0f, 0.0f), FVector(2750.0f, 2900.0f, 0.0f), -100.0f, 520.0f, TEXT("SettlementNorth"));
    AddBoundarySegment(FVector(-2750.0f, -3050.0f, 0.0f), FVector(2750.0f, -3050.0f, 0.0f), -100.0f, 520.0f, TEXT("SettlementSouth"));
    AddBoundarySegment(FVector(2750.0f, -3050.0f, 0.0f), FVector(2750.0f, -2520.0f, 0.0f), -100.0f, 520.0f, TEXT("SettlementEastLow"));
    AddBoundarySegment(FVector(2750.0f, -1780.0f, 0.0f), FVector(2750.0f, 880.0f, 0.0f), -100.0f, 520.0f, TEXT("SettlementEastMiddle"));
    AddBoundarySegment(FVector(2750.0f, 1870.0f, 0.0f), FVector(2750.0f, 2900.0f, 0.0f), -100.0f, 520.0f, TEXT("SettlementEastHigh"));
    AddInstance(CliffInstances, FVector(2750.0f, -2520.0f, 90.0f), FVector(1.2f, 1.2f, 4.0f));
    AddInstance(CliffInstances, FVector(2750.0f, -1780.0f, 90.0f), FVector(1.2f, 1.2f, 4.0f));
    AddInstance(CliffInstances, FVector(2750.0f, 880.0f, 90.0f), FVector(1.2f, 1.2f, 4.0f));
    AddInstance(CliffInstances, FVector(2750.0f, 1870.0f, 90.0f), FVector(1.2f, 1.2f, 4.0f));
    AddInstance(StoneInstances, FVector(2600.0f, 1380.0f, 120.0f), FVector(6.0f, 12.0f, 1.0f), FRotator(0.0f, 26.0f, 0.0f));
    AddRockCluster(FVector(-2800.0f, 0.0f, -120.0f), FVector(350.0f, 2600.0f, 600.0f), 31, false);
    AddRockCluster(FVector(0.0f, 2950.0f, -120.0f), FVector(2500.0f, 350.0f, 560.0f), 37, false);

    BuildFishingHut(FVector(-1100.0f, -1650.0f, 150.0f), 18.0f, 1.0f);
    BuildFishingHut(FVector(1250.0f, -2100.0f, 145.0f), -20.0f, 0.82f);
    BuildFishingHut(FVector(-1550.0f, 900.0f, 155.0f), 125.0f, 0.75f);
    BuildBoat(FVector(1450.0f, 1250.0f, 130.0f), -28.0f, 0.9f);

    BuildRopeFence(FVector(-400.0f, 2600.0f, 110.0f), FVector(2500.0f, 2100.0f, 120.0f), 6);
    BuildRopeFence(FVector(-2600.0f, -2900.0f, 110.0f), FVector(-500.0f, -3200.0f, 105.0f), 5);
}

void ACoastalScene::BuildTraderHub()
{
    if (!Layout.Mara)
    {
        return;
    }

    // The authored kit shares a local origin. Its front is -X and its rear points +X,
    // so it is deliberately anchored independently of Mara's placeholder rotation.
    struct FTraderHubMesh
    {
        const TCHAR* Name;
        const TCHAR* AssetPath;
    };
    const FTraderHubMesh HubMeshes[] = {
        { TEXT("Structure"), TEXT("/Game/Generated/TraderHub/SM_LT_TraderHub_Structure.SM_LT_TraderHub_Structure") },
        { TEXT("Awning"), TEXT("/Game/Generated/TraderHub/SM_LT_TraderHub_Awning.SM_LT_TraderHub_Awning") },
        { TEXT("Counter"), TEXT("/Game/Generated/TraderHub/SM_LT_TraderHub_Counter.SM_LT_TraderHub_Counter") },
        { TEXT("Storage"), TEXT("/Game/Generated/TraderHub/SM_LT_TraderHub_Storage.SM_LT_TraderHub_Storage") },
        { TEXT("Nautical"), TEXT("/Game/Generated/TraderHub/SM_LT_TraderHub_Nautical.SM_LT_TraderHub_Nautical") },
        { TEXT("Workbench"), TEXT("/Game/Generated/TraderHub/SM_LT_TraderHub_Workbench.SM_LT_TraderHub_Workbench") },
        { TEXT("Sign"), TEXT("/Game/Generated/TraderHub/SM_LT_TraderHub_Sign.SM_LT_TraderHub_Sign") },
    };

    TArray<UStaticMesh*> LoadedMeshes;
    LoadedMeshes.Reserve(UE_ARRAY_COUNT(HubMeshes));
    for (const FTraderHubMesh& HubMesh : HubMeshes)
    {
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, HubMesh.AssetPath);
        if (!Mesh)
        {
            UE_LOG(LogTemp, Warning, TEXT("LOW TIDE trader hub kit unavailable: %s. Mara remains playable without hub visuals."),
                HubMesh.AssetPath);
            return;
        }
        LoadedMeshes.Add(Mesh);
    }

    USceneComponent* HubRoot = NewObject<USceneComponent>(this, TEXT("TraderHubRoot"));
    AddInstanceComponent(HubRoot);
    HubRoot->SetupAttachment(SceneRoot);
    HubRoot->SetAbsolute(true, true, true);
    HubRoot->SetWorldLocation(FVector(900.0f, -1350.0f, 125.0f));
    HubRoot->SetWorldRotation(FRotator(0.0f, -38.0f, 0.0f));
    HubRoot->SetWorldScale3D(FVector::OneVector);
    HubRoot->RegisterComponent();

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(HubMeshes); ++Index)
    {
        UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this,
            *FString::Printf(TEXT("TraderHub_%s"), HubMeshes[Index].Name));
        AddInstanceComponent(Part);
        Part->SetupAttachment(HubRoot);
        Part->SetStaticMesh(LoadedMeshes[Index]);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetCastShadow(true);
        Part->ComponentTags.Add(TEXT("TraderHubMesh"));
        Part->SetRelativeTransform(FTransform::Identity);
        Part->RegisterComponent();
    }

    // These are the only hub collision additions: back and side shells. The authored front remains open
    // so Mara's existing interaction ray, floor and route approach retain their validated behavior.
    struct FTraderHubProxy
    {
        FVector Center;
        FVector Extent;
        const TCHAR* Name;
    };
    const FTraderHubProxy Proxies[] = {
        { FVector(442.0f, 0.0f, 260.0f), FVector(22.0f, 306.0f, 225.0f), TEXT("BackWall") },
        { FVector(225.0f, -312.0f, 190.0f), FVector(178.0f, 18.0f, 170.0f), TEXT("LeftWall") },
        { FVector(225.0f, 312.0f, 190.0f), FVector(178.0f, 18.0f, 170.0f), TEXT("RightWall") },
    };
    for (const FTraderHubProxy& Proxy : Proxies)
    {
        UBoxComponent* Box = NewObject<UBoxComponent>(this, FName(Proxy.Name));
        AddInstanceComponent(Box);
        Box->SetupAttachment(HubRoot);
        Box->SetRelativeLocation(Proxy.Center);
        Box->SetBoxExtent(Proxy.Extent);
        Box->SetCollisionProfileName(TEXT("BlockAll"));
        Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Box->ComponentTags.Add(TEXT("TraderHubCollision"));
        Box->RegisterComponent();
    }

    UE_LOG(LogTemp, Display, TEXT("LOW TIDE trader hub kit loaded: %d authored meshes, 3 structural collision proxies."),
        UE_ARRAY_COUNT(HubMeshes));
}

void ACoastalScene::BuildMainRoute()
{
    BuildPathRibbon(Layout.RouteWaypoints, SandInstances, 900.0f, true, true, false);
    // Carry the 125 cm settlement floor beyond its furthest edge across the full trail width,
    // then descend on one shallow proxy to meet the authored main-route plane in both directions.
    AddBoxBetween(RouteFloorCollision, FVector(2000.0f, -800.0f, 128.0f),
        FVector(3430.0f, -3400.0f, 128.0f), 900.0f * CoastalScene::RouteFloorWidthScale, 70.0f, -38.0f);
    AddBoxBetween(RouteFloorCollision, FVector(3430.0f, -3400.0f, 128.0f),
        FVector(4002.0f, -4440.0f, 35.4f), 900.0f * CoastalScene::RouteFloorWidthScale, 70.0f, -38.0f);
    for (int32 Index = 1; Index < Layout.RouteWaypoints.Num(); ++Index)
    {
        if (Index == 8)
        {
            continue; // The blue tide pylons deliberately replace amber at the shortcut choke.
        }
        const FVector MarkerLocation = Layout.RouteWaypoints[Index] + FVector(0.0f, 0.0f, 215.0f);
        AddInstance(WarmAccentInstances, MarkerLocation, FVector(0.30f, 0.30f, 0.30f));
        AddInstance(RopeInstances, MarkerLocation - FVector(0.0f, 0.0f, 110.0f), FVector(0.10f, 0.10f, 2.2f));
    }
    BuildRockArch(Layout.RouteWaypoints[3] + FVector(0.0f, 0.0f, 40.0f), 18.0f);
    BuildWreck(Layout.RouteWaypoints[7] + FVector(-900.0f, 300.0f, 55.0f), -34.0f);
    const FVector SignalStationLocation = Layout.RouteWaypoints.Last() + FVector(900.0f, 900.0f, 20.0f);
    AddBoxBetween(StoneInstances, Layout.RouteWaypoints.Last() + FVector(0.0f, 0.0f, 20.0f),
        SignalStationLocation + FVector(0.0f, 0.0f, 235.0f), 520.0f, 45.0f);
    BuildRopeFence(Layout.RouteWaypoints.Last() + FVector(-120.0f, 250.0f, 120.0f),
        SignalStationLocation + FVector(-120.0f, 250.0f, 330.0f), 4);
    BuildSignalStation(SignalStationLocation, 22.0f);

    AddRockCluster(FVector(5800.0f, -3500.0f, -120.0f), FVector(900.0f, 700.0f, 650.0f), 43, false);
    AddRockCluster(FVector(10500.0f, -2700.0f, -180.0f), FVector(900.0f, 700.0f, 750.0f), 71, false);
    AddRockCluster(FVector(13200.0f, 3000.0f, -200.0f), FVector(800.0f, 800.0f, 800.0f), 97, false);
    AddRockCluster(FVector(17500.0f, -1800.0f, -230.0f), FVector(750.0f, 700.0f, 650.0f), 113, false);

    // Route language is consistent everywhere: amber continues outward; tide-blue marks the safe return ridge.
    AddWorldLabel(TEXT("TIDE ROAD  >  QUICK WHILE EXPOSED"), FVector(15100.0f, 4200.0f, 510.0f),
        FRotator(0.0f, -110.0f, 0.0f), FLinearColor(1.0f, 0.55f, 0.12f), 40.0f);
    AddWorldLabel(TEXT("CLIFF TRAIL  ^  SAFE RETURN"), FVector(15300.0f, 4650.0f, 650.0f),
        FRotator(0.0f, -110.0f, 0.0f), FLinearColor(0.18f, 0.84f, 0.92f), 40.0f);

    // Two blue pylons frame the route section that visibly disappears below the returning tide.
    AddInstance(TideAccentInstances, FVector(16600.0f, 3160.0f, 120.0f), FVector(0.25f, 0.25f, 4.8f));
    AddInstance(TideAccentInstances, FVector(17380.0f, 3870.0f, 120.0f), FVector(0.25f, 0.25f, 4.8f));
}

void ACoastalScene::BuildAlternateRoute()
{
    BuildPathRibbon(Layout.AlternateRouteWaypoints, StoneInstances, 820.0f, true, true, false);
    if (AActor* Marker = SpawnMarkerActor(Layout.AlternateRouteWaypoints[2], TEXT("M1AlternateRoute")))
    {
        Marker->Tags.Add(TEXT("AlwaysOpen"));
    }
    for (int32 Index = 1; Index < Layout.AlternateRouteWaypoints.Num() - 1; ++Index)
    {
        const FVector& Point = Layout.AlternateRouteWaypoints[Index];
        AddInstance(TideAccentInstances, Point + FVector(0.0f, 0.0f, 220.0f), FVector(0.28f, 0.28f, 2.8f));
        if ((Index % 2) == 0)
        {
            BuildRopeFence(Point + FVector(-350.0f, -180.0f, 80.0f), Point + FVector(350.0f, 180.0f, 80.0f), 3);
        }
    }
    AddWorldLabel(TEXT("BLUE POSTS LEAD HOME"), FVector(17700.0f, 6500.0f, 900.0f),
        FRotator(0.0f, -135.0f, 0.0f), FLinearColor(0.18f, 0.84f, 0.92f), 42.0f);
}

void ACoastalScene::BuildOptionalRoute()
{
    BuildPathRibbon(Layout.OptionalRouteWaypoints, DeepGroundInstances, 760.0f, true, true, true);
    const FVector ShrineSurface = Layout.OptionalRouteWaypoints.Last();
    const FVector ApproachDirection = (Layout.OptionalRouteWaypoints[Layout.OptionalRouteWaypoints.Num() - 2]
        - ShrineSurface).GetSafeNormal2D();
    const FVector RampStart = ShrineSurface + ApproachDirection * 600.0f + FVector(0.0f, 0.0f, -22.0f);
    AddBoxBetween(StoneInstances, RampStart, ShrineSurface, 500.0f, 40.0f, -20.0f);
    BuildShrine(ShrineSurface - FVector(0.0f, 0.0f, 100.0f), -18.0f);
    AddWorldLabel(TEXT("WARD LINE // TURN BACK WITH WHAT YOU HAVE"), FVector(20700.0f, -2950.0f, 470.0f),
        FRotator(0.0f, 35.0f, 0.0f), FLinearColor(0.24f, 0.92f, 0.76f), 37.0f);

    for (int32 Index = 0; Index < Layout.WardLocations.Num(); ++Index)
    {
        const FVector& Ward = Layout.WardLocations[Index];
        AActor* WardMarker = SpawnMarkerActor(Ward, TEXT("M1Ward"));
        if (WardMarker)
        {
            WardMarker->Tags.Add(*FString::Printf(TEXT("M1Ward%d"), Index + 1));
        }
        AddInstance(AnomalyInstances, Ward, FVector(0.7f, 0.7f, 3.4f), FRotator(0.0f, Index * 37.0f, 0.0f));
        AddInstance(AnomalyInstances, Ward + FVector(0.0f, 0.0f, 165.0f), FVector(0.42f, 0.42f, 1.0f),
            FRotator(180.0f, Index * 37.0f, 0.0f));
    }
}

void ACoastalScene::BuildPathRibbon(const TArray<FVector>& Points,
    UHierarchicalInstancedStaticMeshComponent* Surface, float Width, bool bBoundLeft, bool bBoundRight, bool bDeep)
{
    if (Points.Num() < 2)
    {
        return;
    }
    for (int32 Index = 0; Index + 1 < Points.Num(); ++Index)
    {
        const FVector Start = Points[Index];
        const FVector End = Points[Index + 1];
        // Generated route ribbons mirror these pitched top planes and horizontal junction caps.
        const float CollisionWidth = Width * CoastalScene::RouteFloorWidthScale;
        AddBoxBetween(RouteFloorCollision, Start, End, CollisionWidth, 70.0f, -38.0f);
        AddInstance(RouteFloorCollision, Start + FVector(0.0f, 0.0f, -38.0f),
            FVector(CollisionWidth / 100.0f, CollisionWidth / 100.0f, 0.7f));

        FVector FlatDirection(End.X - Start.X, End.Y - Start.Y, 0.0f);
        FlatDirection.Normalize();
        const FVector Side(-FlatDirection.Y, FlatDirection.X, 0.0f);
        const float SideOffset = Width * 0.56f;
        const FVector LeftStart = Start + Side * SideOffset;
        const FVector LeftEnd = End + Side * SideOffset;
        const FVector RightStart = Start - Side * SideOffset;
        const FVector RightEnd = End - Side * SideOffset;

        if (bBoundLeft)
        {
            AddFilteredRouteBoundary(LeftStart, LeftEnd, Start, End, 470.0f);
        }
        if (bBoundRight)
        {
            AddFilteredRouteBoundary(RightStart, RightEnd, Start, End, 470.0f);
        }

        // Collision-bearing boulders overlap the trimmed wall ends without projecting across the walk ribbon.
        AddInstance(bDeep ? DarkRockInstances : CliffInstances,
            LeftStart + Side * 120.0f + FVector(0.0f, 0.0f, 90.0f),
            FVector(2.4f, 2.4f, 4.0f), FRotator(0.0f, Index * 29.0f, 0.0f));
        AddInstance(bDeep ? DarkRockInstances : CliffInstances,
            RightStart - Side * 120.0f + FVector(0.0f, 0.0f, 90.0f),
            FVector(2.4f, 2.4f, 4.0f), FRotator(0.0f, Index * 31.0f, 0.0f));

    }
}

void ACoastalScene::BuildRockArch(const FVector& Location, float YawDegrees)
{
    const FRotator Rotation(0.0f, YawDegrees, 0.0f);
    const FVector Side = Rotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));
    AddInstance(CliffInstances, Location + Side * 520.0f + FVector(0.0f, 0.0f, 250.0f), FVector(4.8f, 3.8f, 8.5f), Rotation);
    AddInstance(CliffInstances, Location - Side * 520.0f + FVector(0.0f, 0.0f, 230.0f), FVector(4.3f, 3.5f, 8.2f), Rotation);
    // The cap overlaps both pillars while retaining more than four metres of route headroom.
    AddInstance(CliffInstances, Location + FVector(0.0f, 0.0f, 580.0f), FVector(8.5f, 3.2f, 2.4f),
        FRotator(-7.0f, YawDegrees, 8.0f));
    AddInstance(TideAccentInstances, Location + FVector(0.0f, 0.0f, 505.0f), FVector(0.15f, 3.2f, 0.18f), Rotation);
}

void ACoastalScene::BuildWreck(const FVector& Location, float YawDegrees)
{
    const FRotator YawRotation(0.0f, YawDegrees, 0.0f);
    const FVector Forward = YawRotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
    const FVector Side = YawRotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));
    AddInstance(SandInstances, Location - FVector(0.0f, 0.0f, 95.0f), FVector(18.0f, 14.0f, 1.0f), YawRotation);
    for (int32 Rib = -4; Rib <= 4; ++Rib)
    {
        const float Taper = 1.0f - 0.10f * FMath::Abs(Rib);
        const FVector RibCenter = Location + Forward * Rib * 150.0f;
        AddCylinderBetween(RopeInstances, RibCenter - Side * 250.0f * Taper,
            RibCenter + FVector(0.0f, 0.0f, 245.0f), 23.0f);
        AddCylinderBetween(RopeInstances, RibCenter + Side * 250.0f * Taper,
            RibCenter + FVector(0.0f, 0.0f, 245.0f), 23.0f);
    }
    for (int32 Plank = -2; Plank <= 2; ++Plank)
    {
        AddInstance(WoodInstances, Location + Side * Plank * 95.0f + FVector(0.0f, 0.0f, 105.0f + FMath::Abs(Plank) * 26.0f),
            FVector(10.5f, 0.62f, 0.23f), FRotator(Plank * 3.0f, YawDegrees, Plank * 8.0f));
    }
    AddCylinderBetween(MetalInstances, Location + FVector(0.0f, 0.0f, 85.0f),
        Location + Forward * 140.0f + FVector(0.0f, 0.0f, 1030.0f), 30.0f);
    AddInstance(RoofInstances, Location + Forward * 80.0f + Side * 150.0f + FVector(0.0f, 0.0f, 690.0f),
        FVector(3.2f, 0.12f, 3.8f), FRotator(-12.0f, YawDegrees, 12.0f));
    AddWorldLabel(TEXT("WRECK SHELF // SALVAGE STOP"), Location + FVector(0.0f, 0.0f, 470.0f),
        CoastalScene::FacingRotation(Location, Layout.RouteWaypoints[6]), FLinearColor(0.82f, 0.90f, 0.78f), 38.0f);
}

void ACoastalScene::BuildSignalStation(const FVector& Location, float YawDegrees)
{
    const FRotator Rotation(0.0f, YawDegrees, 0.0f);
    AddInstance(StoneInstances, Location + FVector(0.0f, 0.0f, 35.0f), FVector(14.0f, 12.0f, 1.6f), Rotation);
    AddInstance(StoneInstances, Location + FVector(0.0f, 0.0f, 145.0f), FVector(8.5f, 7.5f, 1.2f), Rotation);
    const FVector Forward = Rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
    const FVector Side = Rotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));
    for (int32 X = -1; X <= 1; X += 2)
    {
        for (int32 Y = -1; Y <= 1; Y += 2)
        {
            AddInstance(WoodInstances, Location + Forward * X * 330.0f + Side * Y * 270.0f + FVector(0.0f, 0.0f, 560.0f),
                FVector(0.42f, 0.42f, 8.2f), Rotation);
        }
    }
    AddInstance(WoodInstances, Location + FVector(0.0f, 0.0f, 880.0f), FVector(8.8f, 7.4f, 0.42f), Rotation);
    // Paired roof planes produce a strong coastal signal-house silhouette.
    AddInstance(RoofInstances, Location - Side * 185.0f + FVector(0.0f, 0.0f, 1110.0f), FVector(9.8f, 4.8f, 0.34f),
        FRotator(0.0f, YawDegrees, -27.0f));
    AddInstance(RoofInstances, Location + Side * 185.0f + FVector(0.0f, 0.0f, 1110.0f), FVector(9.8f, 4.8f, 0.34f),
        FRotator(0.0f, YawDegrees, 27.0f));
    AddCylinderBetween(MetalInstances, Location + FVector(0.0f, 0.0f, 930.0f), Location + FVector(0.0f, 0.0f, 1850.0f), 34.0f);
    AddCylinderBetween(MetalInstances, Location + FVector(0.0f, 0.0f, 1540.0f),
        Location + Forward * 620.0f + FVector(0.0f, 0.0f, 1540.0f), 18.0f);
    AddInstance(WarmAccentInstances, Location + Forward * 610.0f + FVector(0.0f, 0.0f, 1540.0f), FVector(0.6f));
    // The logbook desk is visually distinct and leaves its top free for the gameplay interaction actor.
    AddInstance(WoodInstances, Layout.MainObjectiveLocation - Forward * 120.0f + FVector(0.0f, 0.0f, -85.0f),
        FVector(3.0f, 1.7f, 1.0f), Rotation);
    AddInstance(WarmAccentInstances, Layout.MainObjectiveLocation + FVector(0.0f, 0.0f, 18.0f),
        FVector(0.72f, 0.48f, 0.12f), Rotation);
    AddWorldLabel(TEXT("SIGNAL STATION // LOGBOOK"), Location + FVector(0.0f, 0.0f, 1320.0f),
        CoastalScene::FacingRotation(Location, Layout.RouteWaypoints[9]), FLinearColor(1.0f, 0.62f, 0.18f), 50.0f);
}

void ACoastalScene::BuildShrine(const FVector& Location, float YawDegrees)
{
    for (int32 Index = 0; Index < 7; ++Index)
    {
        const float Angle = Index * UE_TWO_PI / 7.0f;
        const FVector Offset(FMath::Cos(Angle) * 650.0f, FMath::Sin(Angle) * 650.0f, 150.0f);
        AddInstance(DarkRockInstances, Location + Offset, FVector(2.6f, 1.5f, 6.5f),
            FRotator(-8.0f + Index * 2.0f, YawDegrees + Index * 51.0f, Index * 5.0f));
    }
    AddInstance(StoneInstances, Location + FVector(0.0f, 0.0f, 20.0f), FVector(5.4f, 5.4f, 1.6f),
        FRotator(0.0f, YawDegrees, 0.0f));
    AddInstance(AnomalyInstances, Location + FVector(0.0f, 0.0f, 220.0f), FVector(1.2f, 1.2f, 4.8f),
        FRotator(0.0f, YawDegrees, 0.0f));
}

void ACoastalScene::BuildFishingHut(const FVector& Location, float YawDegrees, float Scale)
{
    const FRotator Rotation(0.0f, YawDegrees, 0.0f);
    AddInstance(StoneInstances, Location - FVector(0.0f, 0.0f, 100.0f * Scale), FVector(7.0f, 5.5f, 1.2f) * Scale, Rotation);
    // Separated vertical planks retain readable grain and silhouette at first-person distance.
    for (int32 Plank = -4; Plank <= 4; ++Plank)
    {
        AddInstance(WoodInstances, Location + Rotation.RotateVector(FVector(Plank * 72.0f * Scale, -250.0f * Scale, 135.0f * Scale)),
            FVector(0.62f, 0.45f, 3.6f) * Scale, FRotator(Plank % 2 == 0 ? 1.5f : -1.5f, YawDegrees, 0.0f));
    }
    AddInstance(WoodInstances, Location + Rotation.RotateVector(FVector(0.0f, 245.0f * Scale, 130.0f * Scale)),
        FVector(6.6f, 0.45f, 3.6f) * Scale, Rotation);
    AddInstance(WoodInstances, Location + Rotation.RotateVector(FVector(-310.0f * Scale, 0.0f, 130.0f * Scale)),
        FVector(0.45f, 5.2f, 3.6f) * Scale, Rotation);
    AddInstance(WoodInstances, Location + Rotation.RotateVector(FVector(310.0f * Scale, 0.0f, 130.0f * Scale)),
        FVector(0.45f, 5.2f, 3.6f) * Scale, Rotation);
    AddInstance(RoofInstances, Location + Rotation.RotateVector(FVector(0.0f, -135.0f * Scale, 390.0f * Scale)),
        FVector(7.8f, 3.7f, 0.32f) * Scale, FRotator(0.0f, YawDegrees, -28.0f));
    AddInstance(RoofInstances, Location + Rotation.RotateVector(FVector(0.0f, 135.0f * Scale, 390.0f * Scale)),
        FVector(7.8f, 3.7f, 0.32f) * Scale, FRotator(0.0f, YawDegrees, 28.0f));
    AddInstance(WarmAccentInstances, Location + Rotation.RotateVector(FVector(315.0f * Scale, -265.0f * Scale, 230.0f * Scale)),
        FVector(0.26f * Scale));
}

void ACoastalScene::BuildBoat(const FVector& Location, float YawDegrees, float Scale)
{
    const FRotator Rotation(0.0f, YawDegrees, 0.0f);
    for (int32 Rib = -4; Rib <= 4; ++Rib)
    {
        const float Taper = 1.0f - 0.13f * FMath::Abs(Rib);
        AddInstance(WoodInstances, Location + Rotation.RotateVector(FVector(Rib * 90.0f * Scale, 0.0f, FMath::Abs(Rib) * 20.0f)),
            FVector(0.55f, 4.0f * Taper, 0.24f) * Scale, FRotator(0.0f, YawDegrees, Rib * 2.0f));
    }
    AddInstance(WoodInstances, Location + Rotation.RotateVector(FVector(0.0f, -180.0f * Scale, 95.0f * Scale)),
        FVector(8.5f, 0.38f, 1.6f) * Scale, FRotator(-4.0f, YawDegrees, -11.0f));
    AddInstance(WoodInstances, Location + Rotation.RotateVector(FVector(0.0f, 180.0f * Scale, 95.0f * Scale)),
        FVector(8.5f, 0.38f, 1.6f) * Scale, FRotator(-4.0f, YawDegrees, 11.0f));
}

void ACoastalScene::BuildRopeFence(const FVector& Start, const FVector& End, int32 Sections)
{
    Sections = FMath::Max(1, Sections);
    FVector PreviousTop;
    for (int32 Index = 0; Index <= Sections; ++Index)
    {
        const float Alpha = static_cast<float>(Index) / Sections;
        const FVector Base = FMath::Lerp(Start, End, Alpha);
        const FVector Top = Base + FVector(0.0f, 0.0f, 130.0f);
        AddCylinderBetween(RopeInstances, Base, Top, 10.0f);
        if (Index > 0)
        {
            const FVector Sag(0.0f, 0.0f, -24.0f);
            AddCylinderBetween(RopeInstances, PreviousTop, (PreviousTop + Top) * 0.5f + Sag, 4.5f);
            AddCylinderBetween(RopeInstances, (PreviousTop + Top) * 0.5f + Sag, Top, 4.5f);
        }
        PreviousTop = Top;
    }
}

void ACoastalScene::AddRockCluster(const FVector& Location, const FVector& Extent, int32 Seed, bool bDark)
{
    FRandomStream Random(Seed);
    UHierarchicalInstancedStaticMeshComponent* Family = bDark ? DarkRockInstances : CliffInstances;
    const int32 Count = 5 + Random.RandRange(0, 3);
    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FVector Offset(Random.FRandRange(-Extent.X, Extent.X), Random.FRandRange(-Extent.Y, Extent.Y),
            Random.FRandRange(0.0f, Extent.Z * 0.35f));
        const FVector Scale(Random.FRandRange(2.4f, 5.8f), Random.FRandRange(2.0f, 4.8f),
            Random.FRandRange(2.8f, 7.5f) * (Extent.Z / 600.0f));
        AddInstance(Family, Location + Offset, Scale,
            FRotator(Random.FRandRange(-12.0f, 12.0f), Random.FRandRange(0.0f, 180.0f), Random.FRandRange(-15.0f, 15.0f)));
    }
}

void ACoastalScene::AddWorldLabel(const FString& Text, const FVector& Location, const FRotator& Rotation,
    const FLinearColor& Color, float WorldSize)
{
    AActor* LabelActor = SpawnMarkerActor(Location, TEXT("M1WorldLabel"));
    if (!LabelActor)
    {
        return;
    }
    LabelActor->SetActorRotation(Rotation);
    UTextRenderComponent* Label = NewObject<UTextRenderComponent>(LabelActor, TEXT("Label"));
    LabelActor->AddInstanceComponent(Label);
    Label->SetupAttachment(LabelActor->GetRootComponent());
    Label->SetText(FText::FromString(Text));
    Label->SetTextRenderColor(Color.ToFColor(true));
    Label->SetWorldSize(WorldSize);
    Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    Label->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
    Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label->SetCastShadow(false);
    Label->RegisterComponent();
}

AActor* ACoastalScene::CreateRareArtifactVisual(const FVector& Location)
{
    AActor* Artifact = SpawnMarkerActor(Location, TEXT("M1RareArtifactVisual"));
    if (!Artifact)
    {
        return nullptr;
    }

    auto AddPart = [&](UStaticMesh* Mesh, const FVector& RelativeLocation, const FVector& Scale,
        const FRotator& Rotation, const FLinearColor& Color, const TCHAR* Name)
    {
        UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Artifact, FName(Name));
        Artifact->AddInstanceComponent(Part);
        Part->SetupAttachment(Artifact->GetRootComponent());
        Part->SetStaticMesh(Mesh);
        Part->SetRelativeLocation(RelativeLocation);
        Part->SetRelativeRotation(Rotation);
        Part->SetRelativeScale3D(Scale);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if (StylizedMaterial)
        {
            UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(StylizedMaterial, Artifact);
            Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
            Material->SetVectorParameterValue(TEXT("Color"), Color);
            Material->SetScalarParameterValue(TEXT("Roughness"), 0.24f);
            Part->SetMaterial(0, Material);
        }
        Part->RegisterComponent();
    };

    AddPart(ConeMesh, FVector(0.0f, 0.0f, 80.0f), FVector(0.8f, 0.8f, 2.7f), FRotator::ZeroRotator,
        FLinearColor(0.09f, 0.95f, 0.72f), TEXT("ArtifactCore"));
    AddPart(SphereMesh, FVector(0.0f, 0.0f, 55.0f), FVector(0.62f, 0.62f, 0.42f), FRotator::ZeroRotator,
        FLinearColor(0.22f, 0.13f, 0.38f), TEXT("ArtifactHeart"));
    for (int32 Index = 0; Index < 5; ++Index)
    {
        const float Angle = Index * UE_TWO_PI / 5.0f;
        AddPart(ConeMesh, FVector(FMath::Cos(Angle) * 95.0f, FMath::Sin(Angle) * 95.0f, 105.0f + Index * 8.0f),
            FVector(0.22f, 0.22f, 0.82f), FRotator(35.0f, FMath::RadiansToDegrees(Angle), 0.0f),
            Index % 2 == 0 ? FLinearColor(0.15f, 0.72f, 0.82f) : FLinearColor(0.73f, 0.32f, 0.88f),
            *FString::Printf(TEXT("ArtifactShard%d"), Index));
    }
    UPointLightComponent* Glow = NewObject<UPointLightComponent>(Artifact, TEXT("ArtifactGlow"));
    Artifact->AddInstanceComponent(Glow);
    Glow->SetupAttachment(Artifact->GetRootComponent());
    Glow->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
    Glow->SetLightColor(FLinearColor(0.10f, 0.82f, 0.65f));
    Glow->SetIntensity(900.0f);
    Glow->SetAttenuationRadius(650.0f);
    Glow->SetCastShadows(false);
    Glow->RegisterComponent();
    return Artifact;
}

AActor* ACoastalScene::CreatePhenomenonVisual(const FVector& Location)
{
    AActor* Phenomenon = SpawnMarkerActor(Location, TEXT("M1PhenomenonVisual"));
    if (!Phenomenon)
    {
        return nullptr;
    }
    Phenomenon->GetRootComponent()->SetMobility(EComponentMobility::Movable);

    auto AddPart = [&](UStaticMesh* Mesh, const FVector& RelativeLocation, const FVector& Scale,
        const FRotator& Rotation, const FLinearColor& Color, const TCHAR* Name)
    {
        UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Phenomenon, FName(Name));
        Phenomenon->AddInstanceComponent(Part);
        Part->SetupAttachment(Phenomenon->GetRootComponent());
        Part->SetStaticMesh(Mesh);
        Part->SetRelativeLocation(RelativeLocation);
        Part->SetRelativeRotation(Rotation);
        Part->SetRelativeScale3D(Scale);
        Part->SetMobility(EComponentMobility::Movable);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if (StylizedMaterial)
        {
            UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(StylizedMaterial, Phenomenon);
            Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
            Material->SetVectorParameterValue(TEXT("Color"), Color);
            Material->SetScalarParameterValue(TEXT("Roughness"), 0.3f);
            Part->SetMaterial(0, Material);
        }
        Part->RegisterComponent();
    };

    AddPart(SphereMesh, FVector(0.0f, 0.0f, 125.0f), FVector(1.4f, 1.05f, 2.2f), FRotator(0.0f, 0.0f, -8.0f),
        FLinearColor(0.018f, 0.026f, 0.045f), TEXT("PhenomenonBody"));
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const float Angle = Index * 90.0f + 22.0f;
        AddPart(ConeMesh, FVector(FMath::Cos(FMath::DegreesToRadians(Angle)) * 85.0f,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * 85.0f, 20.0f), FVector(0.38f, 0.38f, 2.4f),
            FRotator(170.0f, Angle, 18.0f), FLinearColor(0.025f, 0.055f, 0.070f),
            *FString::Printf(TEXT("PhenomenonTendril%d"), Index));
    }
    AddPart(SphereMesh, FVector(72.0f, 0.0f, 170.0f), FVector(0.28f, 0.18f, 0.18f), FRotator::ZeroRotator,
        FLinearColor(0.18f, 1.0f, 0.73f), TEXT("PhenomenonEye"));
    Phenomenon->SetActorHiddenInGame(true);
    Phenomenon->SetActorEnableCollision(false);
    return Phenomenon;
}

float ACoastalScene::CalculatePolylineLength(const TArray<FVector>& Points)
{
    float Length = 0.0f;
    for (int32 Index = 1; Index < Points.Num(); ++Index)
    {
        Length += FVector::Dist(Points[Index - 1], Points[Index]);
    }
    return Length;
}
