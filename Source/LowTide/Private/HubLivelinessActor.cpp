#include "HubLivelinessActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
void ConfigureDecorativeMesh(UStaticMeshComponent* Mesh)
{
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetGenerateOverlapEvents(false);
    Mesh->SetCastShadow(true);
    Mesh->SetCullDistance(6000.0f);
}
}

AHubLivelinessActor::AHubLivelinessActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    PrimaryActorTick.TickInterval = NearTickInterval;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Perch = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ParrotPerch"));
    Perch->SetupAttachment(SceneRoot);
    ConfigureDecorativeMesh(Perch);

    BirdRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BirdRoot"));
    BirdRoot->SetupAttachment(SceneRoot);

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ParrotBody"));
    Body->SetupAttachment(BirdRoot);
    ConfigureDecorativeMesh(Body);

    Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ParrotHead"));
    Head->SetupAttachment(BirdRoot);
    Head->SetRelativeLocation(FVector(17.0f, 0.0f, 54.0f));
    ConfigureDecorativeMesh(Head);

    LeftWing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ParrotWingLeft"));
    LeftWing->SetupAttachment(BirdRoot);
    LeftWing->SetRelativeLocation(FVector(1.0f, -8.0f, 41.0f));
    ConfigureDecorativeMesh(LeftWing);

    RightWing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ParrotWingRight"));
    RightWing->SetupAttachment(BirdRoot);
    RightWing->SetRelativeLocation(FVector(1.0f, 8.0f, 41.0f));
    ConfigureDecorativeMesh(RightWing);

    Tail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ParrotTail"));
    Tail->SetupAttachment(BirdRoot);
    Tail->SetRelativeLocation(FVector(-13.0f, 0.0f, 29.0f));
    ConfigureDecorativeMesh(Tail);

    LanternAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("LanternSwingAnchor"));
    LanternAnchor->SetupAttachment(SceneRoot);
    LanternAnchor->SetRelativeLocation(FVector(-45.0f, -70.0f, 105.0f));

    LanternRope = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LanternRope"));
    LanternRope->SetupAttachment(LanternAnchor);
    ConfigureDecorativeMesh(LanternRope);

    Lantern = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HangingLantern"));
    Lantern->SetupAttachment(LanternAnchor);
    ConfigureDecorativeMesh(Lantern);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> PerchAsset(
        TEXT("/Game/Generated/HubLiveliness/SM_LT_Parrot_Perch.SM_LT_Parrot_Perch"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyAsset(
        TEXT("/Game/Generated/HubLiveliness/SM_LT_Parrot_Body.SM_LT_Parrot_Body"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HeadAsset(
        TEXT("/Game/Generated/HubLiveliness/SM_LT_Parrot_Head.SM_LT_Parrot_Head"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> LeftWingAsset(
        TEXT("/Game/Generated/HubLiveliness/SM_LT_Parrot_Wing_L.SM_LT_Parrot_Wing_L"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> RightWingAsset(
        TEXT("/Game/Generated/HubLiveliness/SM_LT_Parrot_Wing_R.SM_LT_Parrot_Wing_R"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> TailAsset(
        TEXT("/Game/Generated/HubLiveliness/SM_LT_Parrot_Tail.SM_LT_Parrot_Tail"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> RopeAsset(
        TEXT("/Game/Generated/HubLiveliness/SM_LT_Lantern_Rope.SM_LT_Lantern_Rope"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> LanternAsset(
        TEXT("/Game/Generated/HubLiveliness/SM_LT_Hanging_Lantern.SM_LT_Hanging_Lantern"));

    if (PerchAsset.Succeeded()) Perch->SetStaticMesh(PerchAsset.Object);
    if (BodyAsset.Succeeded()) Body->SetStaticMesh(BodyAsset.Object);
    if (HeadAsset.Succeeded()) Head->SetStaticMesh(HeadAsset.Object);
    if (LeftWingAsset.Succeeded()) LeftWing->SetStaticMesh(LeftWingAsset.Object);
    if (RightWingAsset.Succeeded()) RightWing->SetStaticMesh(RightWingAsset.Object);
    if (TailAsset.Succeeded()) Tail->SetStaticMesh(TailAsset.Object);
    if (RopeAsset.Succeeded()) LanternRope->SetStaticMesh(RopeAsset.Object);
    if (LanternAsset.Succeeded()) Lantern->SetStaticMesh(LanternAsset.Object);

    Tags.Add(TEXT("HubLiveliness"));
    ApplyRestPose();
    SetLanternEnabled(false);
}

void AHubLivelinessActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    DistanceCheckTime -= DeltaSeconds;
    if (DistanceCheckTime <= 0.0f)
    {
        const bool bWasAnimating = bAnimate;
        bAnimate = IsPlayerCloseEnoughToAnimate();
        DistanceCheckTime = bAnimate ? 0.25f : FarTickInterval;
        SetActorTickInterval(bAnimate ? NearTickInterval : FarTickInterval);
        if (bWasAnimating && !bAnimate)
        {
            ApplyRestPose();
        }
    }

    if (!bAnimate)
    {
        return;
    }

    AnimationTime = FMath::Fmod(AnimationTime + DeltaSeconds, 24.0f);
    ApplyIdlePose(AnimationTime);
}

void AHubLivelinessActor::SetLanternEnabled(bool bEnabled)
{
    bLanternEnabled = bEnabled;
    LanternRope->SetVisibility(bEnabled, true);
    Lantern->SetVisibility(bEnabled, true);
}

void AHubLivelinessActor::ApplyIdlePose(float PhaseSeconds)
{
    // Slow breathing keeps the silhouette alive. Two brief beats add memorable alert head turns.
    const float Breath = FMath::Sin(PhaseSeconds * 1.7f);
    BirdRoot->SetRelativeLocation(FVector(0.0f, 0.0f, 0.7f * Breath));

    const float HeadBeat = 0.5f + 0.5f * FMath::Sin(PhaseSeconds * 0.68f);
    const float Alert = FMath::SmoothStep(0.72f, 0.92f, HeadBeat);
    const float HeadYaw = 5.0f * FMath::Sin(PhaseSeconds * 0.9f) + 24.0f * Alert;
    Head->SetRelativeRotation(FRotator(-3.0f + 2.0f * Breath, HeadYaw, -5.0f * Alert));

    // Wings remain folded: a small asymmetric shrug reads as feather settling, not flight.
    const float WingSettle = FMath::Square(FMath::Max(0.0f, FMath::Sin(PhaseSeconds * 0.41f - 1.3f)));
    LeftWing->SetRelativeRotation(FRotator(0.0f, -2.0f, -4.0f - 10.0f * WingSettle));
    RightWing->SetRelativeRotation(FRotator(0.0f, 2.0f, 4.0f + 7.0f * WingSettle));
    Tail->SetRelativeRotation(FRotator(2.0f * Breath, 3.5f * FMath::Sin(PhaseSeconds * 0.53f), 0.0f));

    if (bLanternEnabled)
    {
        LanternAnchor->SetRelativeRotation(FRotator(
            1.6f * FMath::Sin(PhaseSeconds * 0.61f),
            0.0f,
            2.3f * FMath::Sin(PhaseSeconds * 0.47f + 0.8f)));
    }
}

void AHubLivelinessActor::ApplyRestPose()
{
    BirdRoot->SetRelativeLocation(FVector::ZeroVector);
    Head->SetRelativeRotation(FRotator(-3.0f, 0.0f, 0.0f));
    LeftWing->SetRelativeRotation(FRotator(0.0f, -2.0f, -4.0f));
    RightWing->SetRelativeRotation(FRotator(0.0f, 2.0f, 4.0f));
    Tail->SetRelativeRotation(FRotator::ZeroRotator);
    LanternAnchor->SetRelativeRotation(FRotator::ZeroRotator);
}

bool AHubLivelinessActor::IsPlayerCloseEnoughToAnimate() const
{
    const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    return PlayerPawn && FVector::DistSquared(PlayerPawn->GetActorLocation(), GetActorLocation())
        <= FMath::Square(AnimationCullDistance);
}
