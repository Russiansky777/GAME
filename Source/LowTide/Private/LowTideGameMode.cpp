#include "LowTideGameMode.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "LowTideCharacter.h"
#include "LowTideHUD.h"
#include "LowTideInventoryComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PickupActor.h"
#include "TideController.h"
#include "TraderActor.h"
#include "UObject/ConstructorHelpers.h"

ALowTideGameMode::ALowTideGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = ALowTideCharacter::StaticClass();
    HUDClass = ALowTideHUD::StaticClass();

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    CubeMesh = CubeAsset.Object;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    CylinderMesh = CylinderAsset.Object;
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    BasicMaterial = MaterialAsset.Object;
}

void ALowTideGameMode::RestartPlayer(AController* NewPlayer)
{
    if (!NewPlayer || NewPlayer->GetPawn())
    {
        return;
    }
    RestartPlayerAtTransform(NewPlayer, FTransform(FRotator(0.0f, 0.0f, 0.0f), SafePlayerLocation));
}

void ALowTideGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!ItemCatalog.Load(CatalogError))
    {
        UE_LOG(LogTemp, Error, TEXT("LOW TIDE item catalog failed: %s"), *CatalogError);
    }

    BuildGreybox();
}

void ALowTideGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    ALowTideCharacter* Character = PlayerController ? Cast<ALowTideCharacter>(PlayerController->GetPawn()) : nullptr;
    if (!Character)
    {
        return;
    }

    if (Character->GetActorLocation().Z < -180.0f)
    {
        RecoverStrandedPlayer();
    }
    else if (TideController && TideController->IsAccessOpen())
    {
        if (Character->GetActorLocation().X > SettlementEdgeX)
        {
            BeginExpeditionIfNeeded(Character);
        }
        else if (bExpeditionActive && Character->GetActorLocation().X <= SettlementReturnX)
        {
            CompleteExpedition();
        }
    }
}

void ALowTideGameMode::SpawnInvisibleBoundary(const FVector& Location, const FVector& Scale)
{
    if (AStaticMeshActor* Boundary = SpawnPrimitive(CubeMesh, Location, Scale, FLinearColor::Transparent, true))
    {
        Boundary->Tags.Add(TEXT("M05Boundary"));
        Boundary->SetActorHiddenInGame(true);
    }
}

AStaticMeshActor* ALowTideGameMode::SpawnPrimitive(UStaticMesh* Mesh, const FVector& Location, const FVector& Scale,
    const FLinearColor& Color, bool bCollision, bool bMovable)
{
    if (!Mesh)
    {
        return nullptr;
    }

    FActorSpawnParameters Parameters;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, Parameters);
    if (!Actor)
    {
        return nullptr;
    }

    UStaticMeshComponent* Component = Actor->GetStaticMeshComponent();
    Component->SetMobility(bMovable ? EComponentMobility::Movable : EComponentMobility::Static);
    Component->SetStaticMesh(Mesh);
    Actor->SetActorScale3D(Scale);
    Component->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    if (BasicMaterial)
    {
        Component->SetMaterial(0, BasicMaterial);
        if (UMaterialInstanceDynamic* DynamicMaterial = Component->CreateAndSetMaterialInstanceDynamic(0))
        {
            DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
        }
    }
    return Actor;
}

void ALowTideGameMode::BuildGreybox()
{
    const FLinearColor Rock(0.16f, 0.22f, 0.24f);
    const FLinearColor Path(0.28f, 0.30f, 0.28f);
    const FLinearColor Wood(0.25f, 0.18f, 0.12f);
    const FLinearColor Roof(0.11f, 0.17f, 0.19f);

    SpawnPrimitive(CubeMesh, FVector(0.0f, 0.0f, 30.0f), FVector(14.0f, 12.0f, 1.0f), Rock, true);
    SpawnPrimitive(CubeMesh, FVector(1400.0f, 0.0f, 5.0f), FVector(14.0f, 1.4f, 0.3f), Path, true);
    SpawnPrimitive(CubeMesh, FVector(2700.0f, 0.0f, 5.0f), FVector(12.0f, 14.0f, 0.5f), Rock, true);
    SpawnPrimitive(CubeMesh, FVector(760.0f, 0.0f, 40.0f), FVector(1.2f, 1.4f, 0.4f), Path, true);
    SpawnPrimitive(CubeMesh, FVector(880.0f, 0.0f, 20.0f), FVector(1.2f, 1.4f, 0.4f), Path, true);

    // Temporary invisible containment follows every exposed edge, with openings only at the intended route joins.
    constexpr float BoundaryZ = 150.0f;
    SpawnInvisibleBoundary(FVector(-710.0f, 0.0f, BoundaryZ), FVector(0.2f, 12.2f, 4.0f));
    SpawnInvisibleBoundary(FVector(0.0f, -610.0f, BoundaryZ), FVector(14.2f, 0.2f, 4.0f));
    SpawnInvisibleBoundary(FVector(0.0f, 610.0f, BoundaryZ), FVector(14.2f, 0.2f, 4.0f));
    SpawnInvisibleBoundary(FVector(710.0f, -340.0f, BoundaryZ), FVector(0.2f, 5.4f, 4.0f));
    SpawnInvisibleBoundary(FVector(710.0f, 340.0f, BoundaryZ), FVector(0.2f, 5.4f, 4.0f));

    SpawnInvisibleBoundary(FVector(1400.0f, -80.0f, BoundaryZ), FVector(14.0f, 0.2f, 4.0f));
    SpawnInvisibleBoundary(FVector(1400.0f, 80.0f, BoundaryZ), FVector(14.0f, 0.2f, 4.0f));

    SpawnInvisibleBoundary(FVector(2700.0f, -710.0f, BoundaryZ), FVector(12.2f, 0.2f, 4.0f));
    SpawnInvisibleBoundary(FVector(2700.0f, 710.0f, BoundaryZ), FVector(12.2f, 0.2f, 4.0f));
    SpawnInvisibleBoundary(FVector(3310.0f, 0.0f, BoundaryZ), FVector(0.2f, 14.2f, 4.0f));
    SpawnInvisibleBoundary(FVector(2090.0f, -390.0f, BoundaryZ), FVector(0.2f, 6.4f, 4.0f));
    SpawnInvisibleBoundary(FVector(2090.0f, 390.0f, BoundaryZ), FVector(0.2f, 6.4f, 4.0f));

    // Settlement huts frame the safe shore and use only cooked engine primitives.
    SpawnPrimitive(CubeMesh, FVector(-300.0f, -430.0f, 180.0f), FVector(3.0f, 2.4f, 2.0f), Wood, true);
    SpawnPrimitive(CubeMesh, FVector(-300.0f, -430.0f, 310.0f), FVector(3.4f, 2.8f, 0.6f), Roof, true);
    SpawnPrimitive(CubeMesh, FVector(330.0f, -380.0f, 155.0f), FVector(2.4f, 2.0f, 1.5f), Wood, true);
    SpawnPrimitive(CubeMesh, FVector(330.0f, -380.0f, 260.0f), FVector(2.8f, 2.4f, 0.6f), Roof, true);
    SpawnPrimitive(CylinderMesh, FVector(560.0f, 300.0f, 150.0f), FVector(0.18f, 0.18f, 1.4f), FLinearColor(0.35f, 0.28f, 0.17f), true);

    AStaticMeshActor* Water = SpawnPrimitive(CubeMesh, FVector(1500.0f, 0.0f, 40.0f), FVector(70.0f, 50.0f, 0.2f),
        FLinearColor(0.025f, 0.24f, 0.34f), false, true);
    AStaticMeshActor* CausewayBlocker = SpawnPrimitive(CubeMesh, FVector(1400.0f, 0.0f, 115.0f), FVector(14.0f, 1.45f, 2.2f),
        FLinearColor::Transparent, true);
    if (CausewayBlocker)
    {
        CausewayBlocker->SetActorHiddenInGame(true);
    }

    FActorSpawnParameters Parameters;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ATraderActor* Trader = GetWorld()->SpawnActor<ATraderActor>(FVector(250.0f, 250.0f, 165.0f), FRotator::ZeroRotator, Parameters);
    if (Trader)
    {
        if (UStaticMeshComponent* TraderMesh = Trader->FindComponentByClass<UStaticMeshComponent>())
        {
            if (UMaterialInstanceDynamic* Material = TraderMesh->CreateAndSetMaterialInstanceDynamic(0))
            {
                Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.68f, 0.33f, 0.15f));
            }
        }
    }

    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-42.0f, -35.0f, 0.0f), Parameters);
    if (Sun)
    {
        Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Sun->GetLightComponent()->SetIntensity(4.0f);
        Sun->GetLightComponent()->SetLightColor(FLinearColor(0.78f, 0.86f, 1.0f));
        if (UDirectionalLightComponent* DirectionalComponent = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
        {
            DirectionalComponent->SetAtmosphereSunLight(true);
        }
    }
    GetWorld()->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator, Parameters);
    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator, Parameters);
    if (Sky)
    {
        Sky->GetLightComponent()->SetIntensity(0.7f);
        Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Sky->GetLightComponent()->RecaptureSky();
    }
    AExponentialHeightFog* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>(FVector(0.0f, 0.0f, -80.0f), FRotator::ZeroRotator, Parameters);
    if (Fog)
    {
        Fog->GetComponent()->SetFogDensity(0.012f);
        Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(0.34f, 0.47f, 0.52f));
    }
    TideController = GetWorld()->SpawnActor<ATideController>(FVector::ZeroVector, FRotator::ZeroRotator, Parameters);
    if (TideController)
    {
        TideController->Configure(Water, CausewayBlocker);
        TideController->OnPhaseChanged.AddUObject(this, &ALowTideGameMode::HandleTidePhaseChanged);
        TideController->OnAccessChanged.AddUObject(this, &ALowTideGameMode::HandleAccessChanged);
    }
}

void ALowTideGameMode::BeginExpeditionIfNeeded(const ALowTideCharacter* Character)
{
    if (bExpeditionActive || !Character)
    {
        return;
    }

    ExpeditionStartQuantities.Reset();
    for (const FItemDefinition& Item : ItemCatalog.GetOrderedItems())
    {
        if (Item.bSellable)
        {
            ExpeditionStartQuantities.Add(Item.Id, Character->GetInventory()->GetQuantity(Item.Id));
        }
    }
    bExpeditionActive = true;
}

void ALowTideGameMode::CompleteExpedition()
{
    bExpeditionActive = false;
    ExpeditionStartQuantities.Reset();
}

void ALowTideGameMode::SpawnSalvageForCycle()
{
    if (!TideController || SpawnedLowCycle == TideController->GetLowCycle())
    {
        return;
    }

    for (APickupActor* Pickup : ActivePickups)
    {
        if (IsValid(Pickup))
        {
            Pickup->Destroy();
        }
    }
    ActivePickups.Reset();

    const TArray<FItemDefinition>& Items = ItemCatalog.GetOrderedItems();
    const FVector Locations[] = {
        FVector(2300.0f, -330.0f, 65.0f), FVector(2550.0f, 260.0f, 65.0f),
        FVector(2800.0f, -240.0f, 65.0f), FVector(3020.0f, 330.0f, 65.0f), FVector(3200.0f, -20.0f, 65.0f)
    };
    const FLinearColor Colors[] = {
        FLinearColor(0.34f, 0.38f, 0.40f), FLinearColor(0.72f, 0.30f, 0.12f),
        FLinearColor(0.12f, 0.65f, 0.72f), FLinearColor(0.55f, 0.62f, 0.50f), FLinearColor(0.90f, 0.72f, 0.18f)
    };

    FActorSpawnParameters Parameters;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const int32 SpawnCount = FMath::Min(Items.Num(), static_cast<int32>(UE_ARRAY_COUNT(Locations)));
    const APlayerController* Player = GetWorld()->GetFirstPlayerController();
    const ALowTideCharacter* Character = Player ? Cast<ALowTideCharacter>(Player->GetPawn()) : nullptr;
    for (int32 Index = 0; Index < SpawnCount; ++Index)
    {
        // A retained clue is unique; repeat cycles must not fill the pack with unsellable copies.
        if (!Items[Index].bSellable && Character && Character->GetInventory()->GetQuantity(Items[Index].Id) > 0)
        {
            continue;
        }
        APickupActor* Pickup = GetWorld()->SpawnActor<APickupActor>(Locations[Index], FRotator::ZeroRotator, Parameters);
        if (Pickup)
        {
            Pickup->Configure(Items[Index].Id, Colors[Index]);
            ActivePickups.Add(Pickup);
        }
    }
    SpawnedLowCycle = TideController->GetLowCycle();
}

void ALowTideGameMode::HandleTidePhaseChanged(ETidePhase NewPhase)
{
    if (NewPhase == ETidePhase::Low)
    {
        SpawnSalvageForCycle();
        APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
        if (ALowTideCharacter* Character = PlayerController ? Cast<ALowTideCharacter>(PlayerController->GetPawn()) : nullptr)
        {
            Character->ShowFeedback(TEXT("Low tide: the causeway is open and salvage has washed onto the shelf."), 5.0f);
        }
    }
    else if (NewPhase == ETidePhase::Rising)
    {
        APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
        if (ALowTideCharacter* Character = PlayerController ? Cast<ALowTideCharacter>(PlayerController->GetPawn()) : nullptr)
        {
            Character->ShowFeedback(TEXT("The tide is rising. Return now; access closes when water covers the path."), 6.0f);
        }
    }
    else if (NewPhase == ETidePhase::High)
    {
        RecoverStrandedPlayer();
    }
}

void ALowTideGameMode::HandleAccessChanged(bool bOpen)
{
    if (!bOpen)
    {
        RecoverStrandedPlayer();
        APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
        const ALowTideCharacter* Character = PlayerController ? Cast<ALowTideCharacter>(PlayerController->GetPawn()) : nullptr;
        if (Character && Character->GetActorLocation().X <= SettlementEdgeX)
        {
            CompleteExpedition();
        }
    }
}

void ALowTideGameMode::RecoverStrandedPlayer()
{
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    ALowTideCharacter* Character = PlayerController ? Cast<ALowTideCharacter>(PlayerController->GetPawn()) : nullptr;
    if (!Character || (Character->GetActorLocation().X <= SettlementEdgeX && Character->GetActorLocation().Z >= -180.0f))
    {
        return;
    }

    int32 LostCount = 0;
    for (const FItemDefinition& Item : ItemCatalog.GetOrderedItems())
    {
        if (Item.bSellable && bExpeditionActive)
        {
            const int32 Quantity = Character->GetInventory()->GetQuantity(Item.Id);
            const int32 ExpeditionQuantity = FMath::Max(0, Quantity - ExpeditionStartQuantities.FindRef(Item.Id));
            if (ExpeditionQuantity > 0 && Character->GetInventory()->TryRemove(Item.Id, ExpeditionQuantity))
            {
                LostCount += ExpeditionQuantity;
            }
        }
    }

    Character->CloseMenus();
    Character->ResetMovementAfterRecovery();
    Character->SetActorLocation(SafePlayerLocation, false, nullptr, ETeleportType::TeleportPhysics);
    CompleteExpedition();
    Character->ShowFeedback(FString::Printf(TEXT("Access submerged. Recovered to shore: lost %d expedition salvage; evidence and credits kept."), LostCount), 7.0f);
}
