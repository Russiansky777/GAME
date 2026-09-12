#include "LowTideGameMode.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoastalAudio.h"
#include "CoastalDressing.h"
#include "CoastalScene.h"
#include "HubLivelinessActor.h"
#include "HubDressingActor.h"
#include "HAL/FileManager.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GroundingPlinthActor.h"
#include "JobBoardActor.h"
#include "LowTideCharacter.h"
#include "LowTideHUD.h"
#include "LowTideInventoryComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "PickupActor.h"
#include "StoryClueActor.h"
#include "TideController.h"
#include "TimerManager.h"
#include "TraderActor.h"
#include "Kismet/KismetSystemLibrary.h"
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
    const bool bReviewStart = ReviewView != EM1ReviewView::None;
    const FRotator StartRotation = bReviewStart ? ReviewViewRotation : FRotator::ZeroRotator;
    const FVector StartLocation = bReviewStart ? ReviewViewLocation : SafePlayerLocation;
    RestartPlayerAtTransform(NewPlayer, FTransform(StartRotation, StartLocation));
    if (bReviewStart)
    {
        if (ALowTideCharacter* Character = Cast<ALowTideCharacter>(NewPlayer->GetPawn()))
        {
            Character->GetCharacterMovement()->GravityScale = 0.0f;
            Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
        }
        NewPlayer->SetControlRotation(ReviewViewRotation);
    }
}

void ALowTideGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!ItemCatalog.Load(CatalogError))
    {
        UE_LOG(LogTemp, Error, TEXT("LOW TIDE item catalog failed: %s"), *CatalogError);
    }

    bM05Fixture = GetWorld()->URL.HasOption(TEXT("M05")) || FParse::Param(FCommandLine::Get(), TEXT("M05"));
    if (bM05Fixture)
    {
        BuildGreybox();
    }
    else
    {
        BuildM1Slice();
    }
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

    if (bReviewCaptureRequested)
    {
        if (!bReviewCaptureDispatched)
        {
            ReviewCaptureSettleCountdown -= DeltaSeconds;
            if (ReviewCaptureSettleCountdown <= 0.0f)
            {
                RequestReviewCaptureScreenshot();
            }
        }
        else if (!bReviewCaptureCompleted)
        {
            ReviewCaptureFallbackSeconds += DeltaSeconds;
            if (ReviewCaptureFallbackSeconds >= ReviewCaptureFallbackTimeoutSeconds)
            {
                UE_LOG(LogTemp, Warning, TEXT("LOW TIDE review capture timeout (%0.1f sec), exiting review capture flow."), ReviewCaptureFallbackSeconds);
                if (APlayerController* CapturePlayerController = GetWorld()->GetFirstPlayerController())
                {
                    CapturePlayerController->SetIgnoreLookInput(false);
                    CapturePlayerController->SetIgnoreMoveInput(false);
                }
                FScreenshotRequest::OnScreenshotRequestProcessed().RemoveAll(this);
                QueueReviewCaptureExit();
                bReviewCaptureRequested = false;
                bReviewCaptureCompleted = true;
            }
        }
    }

    // Geometry failures are separate from the authored tide/watcher risk rules.
    // The threshold is below even the submerged terrain skirts, not a normal low-route elevation.
    if (!bM05Fixture && Character->GetActorLocation().Z < -180.0f)
    {
        if (Character->GetActorLocation().Z < -1000.0f)
        {
            RecoverInvalidFall(Character);
        }
        // While falling below every valid M1 walking surface, neither wet-depth nor a
        // watcher's 2D proximity may turn the geometry bug into a salvage penalty.
        return;
    }

    if (!bM05Fixture)
    {
        RememberSafeRecoveryPoint(Character);
        UpdatePhenomenon(Character, DeltaSeconds);
    }

    if (bM05Fixture && Character->GetActorLocation().Z < -180.0f)
    {
        RecoverStrandedPlayer();
    }
    else if (bM05Fixture && TideController && TideController->IsAccessOpen())
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
    else if (!bM05Fixture)
    {
        const bool bSafe = IsInSafeSettlement(Character->GetActorLocation());
        float FeetZ = Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        bool bSupportedGround = true;
        if (Character->GetCharacterMovement()->IsFalling())
        {
            // A short jump over flooded ground must not restart the five-second escape grace.
            FHitResult Ground;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(M1JumpWaterDepth), false, Character);
            bSupportedGround = GetWorld()->LineTraceSingleByChannel(Ground, Character->GetActorLocation(),
                Character->GetActorLocation() - FVector(0.0f, 0.0f, 300.0f), ECC_Pawn, Query);
            if (bSupportedGround)
            {
                FeetZ = Ground.ImpactPoint.Z;
            }
        }
        const bool bWetLowerGround = TideController && !bSafe && bSupportedGround
            && TideController->GetWaterSurfaceZ() - FeetZ > 45.0f;
        if (bWetLowerGround)
        {
            WetExposureSeconds += DeltaSeconds;
            if (!bWetGroundWarningShown)
            {
                bWetGroundWarningShown = true;
                Character->ShowFeedback(TEXT("Water is over your boots. Reach the elevated BLUE RIDGE now."), 6.0f);
            }
            if (WetExposureSeconds >= 5.0f)
            {
                RecoverStrandedPlayer(TEXT("Rising water overtook the lower ground"));
                return;
            }
        }
        else
        {
            WetExposureSeconds = 0.0f;
            bWetGroundWarningShown = false;
        }
        if (!bSafe && MissionState != EM1MissionState::NotAccepted)
        {
            BeginExpeditionIfNeeded(Character);
        }
        else if (bSafe && bExpeditionActive)
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
        TideController->Configure(Water, CausewayBlocker, true);
        TideController->OnPhaseChanged.AddUObject(this, &ALowTideGameMode::HandleTidePhaseChanged);
        TideController->OnAccessChanged.AddUObject(this, &ALowTideGameMode::HandleAccessChanged);
    }
}

void ALowTideGameMode::BuildM1Slice()
{
    FActorSpawnParameters Parameters;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    CoastalScene = GetWorld()->SpawnActor<ACoastalScene>(FVector::ZeroVector, FRotator::ZeroRotator, Parameters);
    if (!CoastalScene)
    {
        CatalogError = TEXT("The authored M1 coastal scene could not be created.");
        return;
    }
    CoastalScene->BuildScene();
    SceneLayout = CoastalScene->GetLayout();
    JobBoard = GetWorld()->SpawnActor<AJobBoardActor>(FVector(1450.0f, 170.0f, 125.0f),
        FRotator(0.0f, 7.0f, 0.0f), Parameters);
    if (AHubDressingActor* HubDressing = GetWorld()->SpawnActor<AHubDressingActor>(
        FVector::ZeroVector, FRotator::ZeroRotator, Parameters))
    {
        HubDressing->BuildDressing();
    }
    // The perch projects from the shop's right front post, clear of Mara and the counter opening.
    const FTransform ShopTransform(FRotator(0.0f, -38.0f, 0.0f), FVector(900.0f, -1350.0f, 125.0f));
    if (AHubLivelinessActor* HubLife = GetWorld()->SpawnActor<AHubLivelinessActor>(
        ShopTransform.TransformPosition(FVector(-87.0f, 312.0f, 185.0f)),
        FRotator(0.0f, 142.0f, 0.0f), Parameters))
    {
        HubLife->SetLanternEnabled(false);
        HubLife->Tags.Add(TEXT("M1HubLife"));
    }
    CoastalDressing = GetWorld()->SpawnActor<ACoastalDressing>(FVector::ZeroVector, FRotator::ZeroRotator, Parameters);
    if (CoastalDressing)
    {
        CoastalDressing->BuildDressing(SceneLayout);
    }
    SafePlayerLocation = SceneLayout.PlayerStart;
    if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
    {
        if (ALowTideCharacter* ExistingCharacter = Cast<ALowTideCharacter>(PlayerController->GetPawn()))
        {
            FRotator ShoreView = SceneLayout.Mara
                ? (SceneLayout.Mara->GetActorLocation() - SceneLayout.PlayerStart).Rotation()
                : FRotator::ZeroRotator;
            ShoreView.Pitch = 0.0f;
            // Frame the shop on the left and the expedition departure on the right.
            // This is only the initial composition; recovery and route anchors are unchanged.
            ShoreView.Yaw = -30.0f;
            ShoreView.Roll = 0.0f;
            ExistingCharacter->SetActorLocationAndRotation(SceneLayout.PlayerStart, ShoreView, false, nullptr, ETeleportType::TeleportPhysics);
            ExistingCharacter->GetCharacterMovement()->StopMovementImmediately();
            ExistingCharacter->GetCharacterMovement()->GravityScale = 1.0f;
            ExistingCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            PlayerController->SetControlRotation(ShoreView);
        }
    }

    TideController = GetWorld()->SpawnActor<ATideController>(FVector::ZeroVector, FRotator::ZeroRotator, Parameters);
    if (TideController)
    {
        TideController->Configure(SceneLayout.WaterActor, SceneLayout.ShortcutBlocker, false);
        TideController->OnPhaseChanged.AddUObject(this, &ALowTideGameMode::HandleTidePhaseChanged);
        TideController->OnAccessChanged.AddUObject(this, &ALowTideGameMode::HandleAccessChanged);
    }

    CoastalAudio = GetWorld()->SpawnActor<ACoastalAudio>(FVector::ZeroVector, FRotator::ZeroRotator, Parameters);
    if (CoastalAudio && TideController)
    {
        CoastalAudio->SetTidePhase(TideController->GetPhase());
    }

    for (const FVector& WardLocation : SceneLayout.WardLocations)
    {
        if (AGroundingPlinthActor* Plinth = GetWorld()->SpawnActor<AGroundingPlinthActor>(WardLocation + FVector(0.0f, 0.0f, 35.0f), FRotator::ZeroRotator, Parameters))
        {
            GroundingPlinths.Add(Plinth);
        }
    }

    static const TCHAR* ClueLabels[] = { TEXT("receiver slate"), TEXT("tide chart"), TEXT("wreck warning") };
    static const TCHAR* ClueTexts[] = {
        TEXT("RECEIVER SLATE: At 03:10 the dead station acknowledged a distress call. Its transmitter was still disconnected."),
        TEXT("TIDE CHART: Someone circled tonight's low water and wrote, 'It gives things back when the bells sing.'"),
        TEXT("CHALK ON THE WRECK: 'If the station answers before you call, do not answer it twice.'")
    };
    for (int32 Index = 0; Index < SceneLayout.RouteClueLocations.Num() && Index < UE_ARRAY_COUNT(ClueTexts); ++Index)
    {
        if (AStoryClueActor* Clue = GetWorld()->SpawnActor<AStoryClueActor>(SceneLayout.RouteClueLocations[Index] + FVector(0.0f, 0.0f, 70.0f), FRotator::ZeroRotator, Parameters))
        {
            Clue->Configure(ClueLabels[Index], ClueTexts[Index]);
            StoryClues.Add(Clue);
        }
    }

    if (SceneLayout.RareArtifactVisual)
    {
        SceneLayout.RareArtifactVisual->SetActorHiddenInGame(false);
    }
    if (SceneLayout.PhenomenonVisual)
    {
        SceneLayout.PhenomenonVisual->SetActorHiddenInGame(true);
        SceneLayout.PhenomenonVisual->SetActorEnableCollision(false);
    }

    ApplyReviewStart();
    BeginReviewCapture(TEXT("M1ReviewCapture"));
    if (FParse::Param(FCommandLine::Get(), TEXT("M1ReviewRising")) && TideController)
    {
        TideController->StartClock();
        TideController->Tick(300.1f);
    }
}

void ALowTideGameMode::ApplyReviewStart()
{
    const EM1ReviewView RequestedView = ParseReviewViewFromCommandLine(TEXT("M1Review"));
    if (RequestedView == EM1ReviewView::None)
    {
        return;
    }
    ApplyReviewView(RequestedView, false);
}

EM1ReviewView ALowTideGameMode::ParseReviewViewFromCommandLine(const TCHAR* CommandLineKey) const
{
    FString ReviewName;
    if (!FParse::Value(FCommandLine::Get(), *FString::Printf(TEXT("-%s="), CommandLineKey), ReviewName))
    {
        return EM1ReviewView::None;
    }
    return ParseReviewViewName(ReviewName);
}

EM1ReviewView ALowTideGameMode::ParseReviewViewName(const FString& ReviewName) const
{
    const FString Sanitized = ReviewName.ToLower();
    if (Sanitized == TEXT("shore"))
    {
        return EM1ReviewView::Shore;
    }
    if (Sanitized == TEXT("return"))
    {
        return EM1ReviewView::Return;
    }
    if (Sanitized == TEXT("wreck"))
    {
        return EM1ReviewView::Wreck;
    }
    if (Sanitized == TEXT("station"))
    {
        return EM1ReviewView::Station;
    }
    if (Sanitized == TEXT("shrine"))
    {
        return EM1ReviewView::Shrine;
    }

    return EM1ReviewView::None;
}

int32 ALowTideGameMode::GetReviewViewIndex(EM1ReviewView View) const
{
    switch (View)
    {
    case EM1ReviewView::Shore:
        return 0;
    case EM1ReviewView::Return:
        return 1;
    case EM1ReviewView::Wreck:
        return 2;
    case EM1ReviewView::Station:
        return 3;
    case EM1ReviewView::Shrine:
        return 4;
    default:
        return INDEX_NONE;
    }
}

FString ALowTideGameMode::GetReviewViewToken(EM1ReviewView View) const
{
    switch (View)
    {
    case EM1ReviewView::Shore:
        return TEXT("shore");
    case EM1ReviewView::Return:
        return TEXT("return");
    case EM1ReviewView::Wreck:
        return TEXT("wreck");
    case EM1ReviewView::Station:
        return TEXT("station");
    case EM1ReviewView::Shrine:
        return TEXT("shrine");
    default:
        return TEXT("none");
    }
}

void ALowTideGameMode::ApplyReviewView(EM1ReviewView View, bool bIgnoreInputForReviewCapture)
{
    const int32 RequestedIndex = GetReviewViewIndex(View);
    if (RequestedIndex == INDEX_NONE || !SceneLayout.ReviewViews.IsValidIndex(RequestedIndex)
        || !SceneLayout.ReviewLookAt.IsValidIndex(RequestedIndex))
    {
        return;
    }

    ReviewView = View;
    ReviewViewLocation = SceneLayout.ReviewViews[RequestedIndex];
    ReviewViewRotation = (SceneLayout.ReviewLookAt[RequestedIndex] - ReviewViewLocation).Rotation();
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (PlayerController)
    {
        if (ALowTideCharacter* Character = Cast<ALowTideCharacter>(PlayerController->GetPawn()))
        {
            Character->SetActorLocationAndRotation(ReviewViewLocation, ReviewViewRotation, false, nullptr, ETeleportType::TeleportPhysics);
            Character->GetCharacterMovement()->StopMovementImmediately();
            Character->GetCharacterMovement()->GravityScale = 0.0f;
            Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
        }
        PlayerController->SetControlRotation(ReviewViewRotation);
        PlayerController->SetIgnoreLookInput(bIgnoreInputForReviewCapture);
        PlayerController->SetIgnoreMoveInput(bIgnoreInputForReviewCapture);
    }
}

void ALowTideGameMode::BeginReviewCapture(const FString& CommandLineSwitch)
{
    const bool bCaptureSwitch = FParse::Param(FCommandLine::Get(), *CommandLineSwitch);
    const FString CaptureSwitch = FString::Printf(TEXT("-%s="), *CommandLineSwitch);
    FString CaptureName;
    const bool bCaptureValueProvided = FParse::Value(FCommandLine::Get(), *CaptureSwitch, CaptureName);
    const EM1ReviewView CaptureView = bCaptureValueProvided ? ParseReviewViewName(CaptureName) : EM1ReviewView::None;

    if (!bCaptureSwitch && !bCaptureValueProvided)
    {
        return;
    }
    if (bCaptureValueProvided && CaptureView == EM1ReviewView::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("LOW TIDE review capture ignored: invalid -%s=%s"), *CommandLineSwitch, *CaptureName);
        return;
    }
    if (CaptureView != EM1ReviewView::None)
    {
        ApplyReviewView(CaptureView, true);
    }
    else if (ReviewView != EM1ReviewView::None)
    {
        ApplyReviewView(ReviewView, true);
    }
    else
    {
        return;
    }

    if (ReviewView != EM1ReviewView::None)
    {
        bReviewCaptureRequested = true;
        bReviewCaptureCompleted = false;
        bReviewCaptureDispatched = false;
        float RequestedCaptureDelay = ReviewCaptureSettleDelaySeconds;
        FParse::Value(FCommandLine::Get(), TEXT("-M1ReviewCaptureDelay="), RequestedCaptureDelay);
        ReviewCaptureSettleCountdown = FMath::Clamp(RequestedCaptureDelay, 3.0f, 60.0f);
        ReviewCaptureFallbackSeconds = 0.0f;
    }
}

void ALowTideGameMode::RequestReviewCaptureScreenshot()
{
    if (!bReviewCaptureRequested || bReviewCaptureDispatched)
    {
        return;
    }

    const FString ScreenshotDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"), TEXT("Windows"));
    if (!IFileManager::Get().DirectoryExists(*ScreenshotDirectory))
    {
        IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
    }
    ReviewCaptureFilepath = FPaths::Combine(ScreenshotDirectory, FString::Printf(TEXT("M1-%s.png"), *GetReviewViewToken(ReviewView)));
    FScreenshotRequest::OnScreenshotRequestProcessed().RemoveAll(this);
    FScreenshotRequest::OnScreenshotRequestProcessed().AddUObject(this, &ALowTideGameMode::OnReviewCaptureScreenshotProcessed);
    FScreenshotRequest::RequestScreenshot(ReviewCaptureFilepath, true, false);
    bReviewCaptureDispatched = true;
    ReviewCaptureFallbackSeconds = 0.0f;
    UE_LOG(LogTemp, Log, TEXT("LOW TIDE review capture requested: %s"), *ReviewCaptureFilepath);
}

void ALowTideGameMode::OnReviewCaptureScreenshotProcessed()
{
    if (!bReviewCaptureRequested || bReviewCaptureCompleted)
    {
        return;
    }

    FScreenshotRequest::OnScreenshotRequestProcessed().RemoveAll(this);
    bReviewCaptureCompleted = true;
    bReviewCaptureDispatched = false;
    bReviewCaptureRequested = false;
    const FString SavedFile = ReviewCaptureFilepath.IsEmpty() ? FScreenshotRequest::GetFilename() : ReviewCaptureFilepath;
    if (IFileManager::Get().FileExists(*SavedFile))
    {
        UE_LOG(LogTemp, Display, TEXT("LOW TIDE review capture saved: %s"), *SavedFile);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("LOW TIDE review capture request completed but file not found yet: %s"), *SavedFile);
    }
    ReviewCaptureFilepath = SavedFile;

    if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
    {
        PlayerController->SetIgnoreLookInput(false);
        PlayerController->SetIgnoreMoveInput(false);
    }
    QueueReviewCaptureExit();
}

void ALowTideGameMode::QueueReviewCaptureExit()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ReviewCaptureExitTimer);
        GetWorld()->GetTimerManager().SetTimer(
            ReviewCaptureExitTimer,
            this,
            &ALowTideGameMode::ExitGameForReviewCapture,
            ReviewCaptureExitDelaySeconds,
            false
        );
    }
}

void ALowTideGameMode::ExitGameForReviewCapture()
{
    if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
    {
        PlayerController->SetIgnoreLookInput(false);
        PlayerController->SetIgnoreMoveInput(false);
    }
    UKismetSystemLibrary::QuitGame(
        GetWorld(),
        GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr,
        EQuitPreference::Quit,
        false
    );
}

APickupActor* ALowTideGameMode::SpawnPickup(FName ItemId, const FVector& Location, const FLinearColor& Color)
{
    FActorSpawnParameters Parameters;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    APickupActor* Pickup = GetWorld()->SpawnActor<APickupActor>(Location, FRotator::ZeroRotator, Parameters);
    if (Pickup)
    {
        Pickup->Configure(ItemId, Color);
        ActivePickups.Add(Pickup);
    }
    return Pickup;
}

void ALowTideGameMode::SpawnM1Pickups()
{
    if (bM1PickupsSpawned)
    {
        return;
    }
    static const FName CommonIds[] = { TEXT("scrap_metal"), TEXT("copper_wire"), TEXT("sealed_tin") };
    static const FLinearColor CommonColors[] = {
        FLinearColor(0.34f, 0.38f, 0.40f), FLinearColor(0.72f, 0.30f, 0.12f), FLinearColor(0.55f, 0.62f, 0.50f)
    };
    for (int32 Index = 0; Index < SceneLayout.CommonSalvageLocations.Num(); ++Index)
    {
        SpawnPickup(CommonIds[Index % UE_ARRAY_COUNT(CommonIds)], SceneLayout.CommonSalvageLocations[Index],
            CommonColors[Index % UE_ARRAY_COUNT(CommonColors)]);
    }
    if (MissionState == EM1MissionState::FindLogbook)
    {
        SpawnPickup(TEXT("signal_station_logbook"), SceneLayout.MainObjectiveLocation, FLinearColor(0.92f, 0.64f, 0.18f));
    }
    if (!bRareArtifactClaimed && !bRareArtifactResolved)
    {
        if (SceneLayout.RareArtifactVisual)
        {
            SceneLayout.RareArtifactVisual->SetActorHiddenInGame(false);
        }
        SpawnPickup(TEXT("singing_shard"), SceneLayout.RareArtifactLocation, FLinearColor(0.30f, 0.95f, 1.0f));
    }
    bM1PickupsSpawned = true;
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

bool ALowTideGameMode::HandleMaraInteraction(ALowTideCharacter* Character)
{
    if (!Character || !SceneLayout.Mara
        || FVector::DistSquared(SceneLayout.Mara->GetActorLocation(), Character->GetActorLocation()) > FMath::Square(475.0f))
    {
        return false;
    }

    if (CoastalAudio)
    {
        CoastalAudio->PlayCue(ECoastalAudioCue::Interaction);
    }
    if (MissionState == EM1MissionState::NotAccepted)
    {
        Character->OpenTrader(SceneLayout.Mara);
        Character->ShowFeedback(TEXT("Mara: Expedition jobs are posted on the board. Bring salvage back here when you return."), 7.0f);
        return true;
    }
    if (MissionState == EM1MissionState::ReturnToMara
        && Character->GetInventory()->GetQuantity(TEXT("signal_station_logbook")) > 0)
    {
        MissionState = EM1MissionState::Complete;
        Character->GetInventory()->AddCredits(MissionRewardCredits);
        CompleteExpedition();
        Character->CloseMenus();
        Character->ShowFeedback(TEXT("Mara pays 75 credits. The log says the station answered a distress signal hours before it was sent."), 12.0f);
        return true;
    }

    Character->OpenTrader(SceneLayout.Mara);
    Character->ShowFeedback(MissionState == EM1MissionState::FindLogbook
        ? TEXT("Mara: Follow the amber station markers. The blue ridge is the return route. I can still buy salvage.")
        : TEXT("Mara buys salvage one piece at a time. Keep or sell the Singing Shard; either choice is yours."), 7.0f);
    return true;
}

bool ALowTideGameMode::HandleJobBoardInteraction(ALowTideCharacter* Character)
{
    if (!Character || !JobBoard
        || FVector::DistSquared(JobBoard->GetActorLocation(), Character->GetActorLocation()) > FMath::Square(475.0f))
    {
        return false;
    }
    if (CoastalAudio)
    {
        CoastalAudio->PlayCue(ECoastalAudioCue::Interaction);
    }
    Character->CloseMenus();
    if (MissionState == EM1MissionState::NotAccepted)
    {
        MissionState = EM1MissionState::FindLogbook;
        BeginExpeditionIfNeeded(Character);
        SpawnM1Pickups();
        if (TideController)
        {
            TideController->StartClock();
        }
        Character->ShowFeedback(TEXT("Job accepted from board: recover the Signal Station Logbook. Reward: 75 credits."), 9.0f);
        return true;
    }
    const FString BoardStatus = MissionState == EM1MissionState::Complete
        ? TEXT("COMPLETED: Signal Station Logbook - paid 75 credits.")
        : MissionState == EM1MissionState::ReturnToMara
            ? TEXT("JOB UPDATE: Logbook secured. Return it to Mara for 75 credits.")
            : TEXT("ACTIVE JOB: Recover the Signal Station Logbook. Follow the amber markers.");
    Character->ShowFeedback(BoardStatus, 7.0f);
    return true;
}

void ALowTideGameMode::NotifyItemCollected(ALowTideCharacter* Character, FName ItemId)
{
    if (!Character)
    {
        return;
    }
    if (CoastalAudio)
    {
        CoastalAudio->PlayCue(ECoastalAudioCue::Interaction);
    }
    const FItemDefinition* Definition = ItemCatalog.Find(ItemId);
    if (ItemId == TEXT("signal_station_logbook"))
    {
        if (MissionState == EM1MissionState::FindLogbook)
        {
            MissionState = EM1MissionState::ReturnToMara;
        }
        Character->ShowFeedback(TEXT("LOGBOOK: 'We answered the distress call at 03:10. At 06:40, the same call arrived for the first time.' Return to Mara."), 12.0f);
    }
    else if (ItemId == TEXT("singing_shard"))
    {
        bRareArtifactClaimed = true;
        if (SceneLayout.RareArtifactVisual)
        {
            SceneLayout.RareArtifactVisual->SetActorHiddenInGame(true);
        }
        SetPhenomenonActive(true);
        Character->ShowFeedback(TEXT("Singing Shard secured: worth 180 credits. Something reacted in the cove."), 12.0f);
    }
    else
    {
        Character->ShowFeedback(Definition
            ? FString::Printf(TEXT("Collected %s."), *Definition->DisplayName)
            : TEXT("Collected salvage."));
    }
}

void ALowTideGameMode::NotifyItemSold(FName ItemId)
{
    if (ItemId == TEXT("singing_shard"))
    {
        bRareArtifactResolved = true;
        SetPhenomenonActive(false);
    }
}

bool ALowTideGameMode::GroundArtifact(ALowTideCharacter* Character)
{
    if (!Character || Character->GetInventory()->GetQuantity(TEXT("singing_shard")) < 1)
    {
        if (Character)
        {
            Character->ShowFeedback(TEXT("The ward hums quietly. You have nothing anomalous to ground."));
        }
        return false;
    }
    if (!Character->GetInventory()->TryRemove(TEXT("singing_shard"), 1))
    {
        return false;
    }
    bRareArtifactResolved = true;
    SetPhenomenonActive(false);
    Character->ShowFeedback(TEXT("You relinquish the Singing Shard. The blue ward grounds its song and the watcher retreats."), 9.0f);
    return true;
}

bool ALowTideGameMode::IsArtifactCarried() const
{
    const APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    const ALowTideCharacter* Character = PlayerController ? Cast<ALowTideCharacter>(PlayerController->GetPawn()) : nullptr;
    return Character && Character->GetInventory()->GetQuantity(TEXT("singing_shard")) > 0;
}

void ALowTideGameMode::SetPhenomenonActive(bool bActive)
{
    bPhenomenonActive = bActive;
    PhenomenonDistance = TNumericLimits<float>::Max();
    bWatcherFarWarningShown = false;
    bWatcherNearWarningShown = false;
    if (SceneLayout.PhenomenonVisual)
    {
        if (!bActive)
        {
            SceneLayout.PhenomenonVisual->SetActorLocation(SceneLayout.PhenomenonStartLocation);
        }
        SceneLayout.PhenomenonVisual->SetActorHiddenInGame(!bActive);
    }
    if (CoastalAudio)
    {
        CoastalAudio->SetThreatIntensity(bActive ? 0.25f : 0.0f);
        CoastalAudio->PlayCue(bActive ? ECoastalAudioCue::AnomalyAwakened : ECoastalAudioCue::AnomalyRetreat);
    }
}

void ALowTideGameMode::UpdatePhenomenon(ALowTideCharacter* Character, float DeltaSeconds)
{
    AActor* Phenomenon = SceneLayout.PhenomenonVisual;
    if (!bPhenomenonActive || !Character || !Phenomenon)
    {
        return;
    }
    if (Character->GetInventory()->GetQuantity(TEXT("singing_shard")) < 1)
    {
        SetPhenomenonActive(false);
        return;
    }

    if (IsInSafeSettlement(Character->GetActorLocation()))
    {
        Phenomenon->SetActorHiddenInGame(true);
        if (CoastalAudio)
        {
            CoastalAudio->SetThreatIntensity(0.0f);
        }
        return;
    }
    Phenomenon->SetActorHiddenInGame(false);

    bool bAtWard = false;
    for (const FVector& WardLocation : SceneLayout.WardLocations)
    {
        if (FVector::DistSquared2D(Character->GetActorLocation(), WardLocation) <= FMath::Square(500.0f))
        {
            bAtWard = true;
            break;
        }
    }

    FVector PhenomenonLocation = Phenomenon->GetActorLocation();
    if (bAtWard)
    {
        const FVector RetreatDirection = (SceneLayout.PhenomenonStartLocation - PhenomenonLocation).GetSafeNormal();
        PhenomenonLocation += RetreatDirection * 900.0f * DeltaSeconds;
        Phenomenon->SetActorLocation(PhenomenonLocation);
    }
    else if (Character->GetVelocity().SizeSquared2D() > FMath::Square(25.0f))
    {
        const FVector AdvanceDirection = (Character->GetActorLocation() - PhenomenonLocation).GetSafeNormal();
        // Preserve the original 1.3x walking pursuit ratio after the Director's +30% movement tune.
        PhenomenonLocation += AdvanceDirection * 845.0f * DeltaSeconds;
        Phenomenon->SetActorLocation(PhenomenonLocation);
    }

    PhenomenonDistance = FVector::Dist2D(Phenomenon->GetActorLocation(), Character->GetActorLocation());
    if (CoastalAudio)
    {
        CoastalAudio->SetThreatIntensity(FMath::Clamp(1.0f - PhenomenonDistance / 3000.0f, 0.12f, 1.0f));
    }
    if (!bWatcherFarWarningShown && PhenomenonDistance < 1500.0f)
    {
        bWatcherFarWarningShown = true;
        Character->ShowFeedback(TEXT("Something follows the shard. Blue lights lie ahead."), 7.0f);
    }
    if (!bWatcherNearWarningShown && PhenomenonDistance < 700.0f)
    {
        bWatcherNearWarningShown = true;
        Character->ShowFeedback(TEXT("The presence is close. Blue lights ahead."), 8.0f);
    }
    if (!bAtWard && PhenomenonDistance < 170.0f)
    {
        RecoverStrandedPlayer(TEXT("The watcher caught the Singing Shard's trail"));
    }
}

bool ALowTideGameMode::IsInSafeSettlement(const FVector& Location) const
{
    return bM05Fixture ? Location.X <= SettlementEdgeX : SceneLayout.SettlementSafeBounds.IsInsideOrOn(Location);
}

FString ALowTideGameMode::GetObjectiveText() const
{
    switch (MissionState)
    {
    case EM1MissionState::NotAccepted:
        return TEXT("NEW JOB: Check the jobs board at the salvage outpost.");
    case EM1MissionState::FindLogbook:
        return TEXT("OBJECTIVE: Follow amber markers to the signal station and recover its logbook.");
    case EM1MissionState::ReturnToMara:
        return bRareArtifactClaimed || bRareArtifactResolved
            ? TEXT("LOGBOOK SECURED: the station answered a signal before it was sent. Return it to Mara at the striped salvage shop.")
            : TEXT("LOGBOOK SECURED: the station answered a signal before it was sent. Return it to Mara at the striped salvage shop. Optional: the Singing Shard is worth 180 credits.");
    case EM1MissionState::Complete:
        return !bRareArtifactClaimed && !bRareArtifactResolved
            ? TEXT("MISSION COMPLETE: Mara paid 75 credits. Optional: the Singing Shard remains in the cove, worth 180 credits.")
            : TEXT("MISSION COMPLETE: Mara paid 75 credits.");
    default:
        return FString();
    }
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
        if (bM05Fixture)
        {
            SpawnSalvageForCycle();
        }
        else
        {
            for (APickupActor* Pickup : ActivePickups)
            {
                if (IsValid(Pickup))
                {
                    Pickup->Destroy();
                }
            }
            ActivePickups.Reset();
            bM1PickupsSpawned = false;
            if (MissionState != EM1MissionState::NotAccepted)
            {
                SpawnM1Pickups();
            }
        }
        APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
        if (ALowTideCharacter* Character = PlayerController ? Cast<ALowTideCharacter>(PlayerController->GetPawn()) : nullptr)
        {
            Character->ShowFeedback(bM05Fixture
                ? TEXT("Low tide: the causeway is open and salvage has washed onto the shelf.")
                : TEXT("Low tide: the lower trail is exposed again. The elevated blue ridge remains the reliable return."), 6.0f);
        }
    }
    else if (NewPhase == ETidePhase::Rising)
    {
        APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
        if (ALowTideCharacter* Character = PlayerController ? Cast<ALowTideCharacter>(PlayerController->GetPawn()) : nullptr)
        {
            Character->ShowFeedback(bM05Fixture
                ? TEXT("The tide is rising. Return now; access closes when water covers the path.")
                : TEXT("TIDE RISING: the whitewater is climbing toward the lower shortcut. The elevated BLUE RIDGE remains open."), 8.0f);
        }
    }
    else if (NewPhase == ETidePhase::High)
    {
        if (bM05Fixture)
        {
            RecoverStrandedPlayer(TEXT("Access submerged"));
        }
    }
    if (CoastalAudio)
    {
        CoastalAudio->SetTidePhase(NewPhase);
    }
}

void ALowTideGameMode::HandleAccessChanged(bool bOpen)
{
    if (!bOpen)
    {
        if (!bM05Fixture)
        {
            if (CoastalAudio)
            {
                CoastalAudio->PlayCue(ECoastalAudioCue::RouteLost);
            }
            APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
            if (ALowTideCharacter* Character = PlayerController ? Cast<ALowTideCharacter>(PlayerController->GetPawn()) : nullptr)
            {
                Character->ShowFeedback(TEXT("LOWER CROSSING FLOODED - FOLLOW THE ELEVATED BLUE RIDGE TO SHORE."), 10.0f);
            }
            return;
        }
        RecoverStrandedPlayer();
        APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
        const ALowTideCharacter* Character = PlayerController ? Cast<ALowTideCharacter>(PlayerController->GetPawn()) : nullptr;
        if (Character && Character->GetActorLocation().X <= SettlementEdgeX)
        {
            CompleteExpedition();
        }
    }
}

bool ALowTideGameMode::IsSafeRecoveryPoint(const ALowTideCharacter* Character, const FVector& Location) const
{
    const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    // Probe a small margin around the capsule, so the saved spot is not a precarious lip.
    const FVector Offsets[] = { FVector::ZeroVector, FVector(75, 0, 0), FVector(-75, 0, 0),
        FVector(0, 75, 0), FVector(0, -75, 0) };
    FCollisionQueryParams Query(SCENE_QUERY_STAT(M1SafeRecoverySupport), false, Character);
    for (const FVector& Offset : Offsets)
    {
        FHitResult Ground;
        if (!GetWorld()->LineTraceSingleByChannel(Ground, Location + Offset + FVector(0, 0, 10),
            Location + Offset - FVector(0, 0, HalfHeight + Movement->MaxStepHeight + 10), ECC_Pawn, Query)
            || Ground.ImpactNormal.Z < Movement->GetWalkableFloorZ()
            || FMath::Abs(Ground.ImpactPoint.Z - (Location.Z - HalfHeight)) > Movement->MaxStepHeight
            || (TideController && !IsInSafeSettlement(Location)
                && Ground.ImpactPoint.Z < TideController->GetWaterSurfaceZ()))
        {
            return false;
        }
    }
    return !GetWorld()->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_Pawn,
        FCollisionShape::MakeCapsule(Character->GetCapsuleComponent()->GetScaledCapsuleRadius() - 1.0f,
            HalfHeight - 1.0f), Query);
}

void ALowTideGameMode::RememberSafeRecoveryPoint(ALowTideCharacter* Character)
{
    const FVector Location = Character->GetActorLocation();
    if (Character->GetCharacterMovement()->IsMovingOnGround()
        && (!bHasSafeRecoveryLocation || FVector::DistSquared(Location, LastSafeRecoveryLocation) > FMath::Square(250.0f))
        && IsSafeRecoveryPoint(Character, Location))
    {
        LastSafeRecoveryLocation = Location;
        bHasSafeRecoveryLocation = true;
    }
}

void ALowTideGameMode::RecoverInvalidFall(ALowTideCharacter* Character)
{
    const FVector Destination = bHasSafeRecoveryLocation && IsSafeRecoveryPoint(Character, LastSafeRecoveryLocation)
        ? LastSafeRecoveryLocation : SafePlayerLocation;
    Character->CloseMenus();
    Character->ResetMovementAfterRecovery();
    Character->SetActorLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);
    WetExposureSeconds = 0.0f;
    bWetGroundWarningShown = false;
    // Keep inventory, mission, pursuit and the expedition snapshot intact: this is not a failed expedition.
    Character->ShowFeedback(TEXT("Recovered from invalid ground; all items kept."), 6.0f);
    UE_LOG(LogTemp, Log, TEXT("LOW TIDE invalid-ground recovery to %s (no item penalty)"), *Destination.ToString());
}

void ALowTideGameMode::RecoverStrandedPlayer(const FString& Trigger)
{
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    ALowTideCharacter* Character = PlayerController ? Cast<ALowTideCharacter>(PlayerController->GetPawn()) : nullptr;
    if (!Character || (IsInSafeSettlement(Character->GetActorLocation()) && Character->GetActorLocation().Z >= -180.0f))
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
                if (Item.Id == TEXT("singing_shard") && !bRareArtifactResolved)
                {
                    bRareArtifactClaimed = false;
                }
            }
        }
    }

    Character->CloseMenus();
    Character->ResetMovementAfterRecovery();
    Character->SetActorLocation(SafePlayerLocation, false, nullptr, ETeleportType::TeleportPhysics);
    WetExposureSeconds = 0.0f;
    bWetGroundWarningShown = false;
    SetPhenomenonActive(false);
    CompleteExpedition();
    Character->ShowFeedback(FString::Printf(TEXT("%s. Recovered to shore: lost %d expedition salvage; prior stock, evidence and credits kept."), *Trigger, LostCount), 9.0f);
}
