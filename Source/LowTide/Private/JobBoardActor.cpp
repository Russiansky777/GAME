#include "JobBoardActor.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "LowTideCharacter.h"
#include "LowTideGameMode.h"
#include "UObject/ConstructorHelpers.h"

AJobBoardActor::AJobBoardActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    PrimaryActorTick.TickInterval = NearTickInterval;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
    InteractionBody = CreateDefaultSubobject<UBoxComponent>(TEXT("JobBoardInteractionBody"));
    InteractionBody->SetupAttachment(SceneRoot);
    // The hero board is 180 cm wide (local Y), 41.6 cm deep and 210 cm tall from its bottom pivot.
    // This query-only volume gives the interaction trace a compact target without changing traversal.
    InteractionBody->SetBoxExtent(FVector(22.0f, 90.0f, 105.0f));
    InteractionBody->SetRelativeLocation(FVector(0.0f, 0.0f, 105.0f));
    InteractionBody->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    InteractionBody->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBody->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBody->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    InteractionBody->ComponentTags.Add(TEXT("JobBoardInteraction"));

    // Keep traversal collision on the actual supports and notice panel. The mesh includes
    // lantern/rope space and an under-board opening that must stay passable.
    auto ConfigureStructuralBody = [this](UBoxComponent* Body, const FVector& Location, const FVector& Extent)
    {
        Body->SetupAttachment(SceneRoot);
        Body->SetRelativeLocation(Location);
        Body->SetBoxExtent(Extent);
        Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
        Body->ComponentTags.Add(TEXT("JobBoardStructuralCollision"));
    };
    LeftPostBody = CreateDefaultSubobject<UBoxComponent>(TEXT("JobBoardLeftPostBody"));
    ConfigureStructuralBody(LeftPostBody, FVector(0.0f, -75.0f, 105.0f), FVector(13.0f, 7.0f, 105.0f));
    RightPostBody = CreateDefaultSubobject<UBoxComponent>(TEXT("JobBoardRightPostBody"));
    ConfigureStructuralBody(RightPostBody, FVector(0.0f, 75.0f, 105.0f), FVector(13.0f, 7.0f, 105.0f));
    NoticePanelBody = CreateDefaultSubobject<UBoxComponent>(TEXT("JobBoardNoticePanelBody"));
    ConfigureStructuralBody(NoticePanelBody, FVector(4.0f, 0.0f, 130.0f), FVector(10.0f, 68.0f, 65.0f));

    BoardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("JobBoardMesh"));
    BoardMesh->SetupAttachment(SceneRoot);
    BoardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BoardMesh->ComponentTags.Add(TEXT("JobBoardHeroMesh"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(
        TEXT("/Game/Generated/HeroJobBoard/SM_JobBoard_Hero_motion/StaticMeshes/SM_JobBoard_Hero_Main.SM_JobBoard_Hero_Main"));
    if (MeshAsset.Succeeded())
    {
        BoardMesh->SetStaticMesh(MeshAsset.Object);
    }

    auto ConfigureAnimatedDecoration = [](UStaticMeshComponent* Mesh, USceneComponent* Anchor, const TCHAR* Tag)
    {
        Mesh->SetupAttachment(Anchor);
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->ComponentTags.Add(FName(Tag));
    };
    LanternAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("JobBoardLanternAnchor"));
    LanternAnchor->SetupAttachment(SceneRoot);
    LanternAnchor->SetRelativeLocation(FVector(-2.2833f, 74.1814f, 176.8394f));
    LanternAnchor->ComponentTags.Add(TEXT("JobBoardAnimatedLanternAnchor"));
    LanternMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("JobBoardLanternMesh"));
    ConfigureAnimatedDecoration(LanternMesh, LanternAnchor, TEXT("JobBoardAnimatedLantern"));
    LanternMesh->SetRelativeLocation(FVector(2.2833f, -74.1814f, -176.8394f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> LanternAsset(
        TEXT("/Game/Generated/HeroJobBoard/SM_JobBoard_Hero_motion/StaticMeshes/SM_JobBoard_Hero_Lantern.SM_JobBoard_Hero_Lantern"));
    if (LanternAsset.Succeeded())
    {
        LanternMesh->SetStaticMesh(LanternAsset.Object);
    }

    PaperAAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("JobBoardPaperAAnchor"));
    PaperAAnchor->SetupAttachment(SceneRoot);
    PaperAAnchor->SetRelativeLocation(FVector(-9.2f, 12.0f, 134.0f));
    PaperAAnchor->ComponentTags.Add(TEXT("JobBoardAnimatedPaperAnchor"));
    PaperAMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("JobBoardPaperAMesh"));
    ConfigureAnimatedDecoration(PaperAMesh, PaperAAnchor, TEXT("JobBoardAnimatedPaper"));
    PaperAMesh->SetRelativeLocation(FVector(9.2f, -12.0f, -134.0f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PaperAAsset(
        TEXT("/Game/Generated/HeroJobBoard/SM_JobBoard_Hero_motion/StaticMeshes/SM_JobBoard_Hero_Paper_A.SM_JobBoard_Hero_Paper_A"));
    if (PaperAAsset.Succeeded())
    {
        PaperAMesh->SetStaticMesh(PaperAAsset.Object);
    }

    PaperBAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("JobBoardPaperBAnchor"));
    PaperBAnchor->SetupAttachment(SceneRoot);
    PaperBAnchor->SetRelativeLocation(FVector(-9.4f, -20.0f, 110.0f));
    PaperBAnchor->ComponentTags.Add(TEXT("JobBoardAnimatedPaperAnchor"));
    PaperBMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("JobBoardPaperBMesh"));
    ConfigureAnimatedDecoration(PaperBMesh, PaperBAnchor, TEXT("JobBoardAnimatedPaper"));
    PaperBMesh->SetRelativeLocation(FVector(9.4f, 20.0f, -110.0f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PaperBAsset(
        TEXT("/Game/Generated/HeroJobBoard/SM_JobBoard_Hero_motion/StaticMeshes/SM_JobBoard_Hero_Paper_B.SM_JobBoard_Hero_Paper_B"));
    if (PaperBAsset.Succeeded())
    {
        PaperBMesh->SetStaticMesh(PaperBAsset.Object);
    }

    LanternLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("JobBoardLanternLight"));
    LanternLight->SetupAttachment(LanternAnchor);
    LanternLight->SetMobility(EComponentMobility::Movable);
    LanternLight->SetRelativeLocation(FVector(-8.0f, 0.0f, -45.0f));
    LanternLight->SetLightColor(FLinearColor(1.0f, 0.48f, 0.18f));
    LanternLight->SetIntensity(260.0f);
    LanternLight->SetAttenuationRadius(190.0f);
    LanternLight->SetCastShadows(false);
    LanternLight->ComponentTags.Add(TEXT("JobBoardLanternLight"));
}

void AJobBoardActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    DistanceCheckSeconds -= DeltaSeconds;
    if (DistanceCheckSeconds <= 0.0f)
    {
        const bool bWasActive = bLivelinessActive;
        bLivelinessActive = IsPlayerCloseEnoughToAnimate();
        DistanceCheckSeconds = bLivelinessActive ? 0.25f : FarTickInterval;
        SetActorTickInterval(bLivelinessActive ? NearTickInterval : FarTickInterval);
        if (bWasActive && !bLivelinessActive)
        {
            ApplyRestPose();
        }
    }
    if (!bLivelinessActive)
    {
        return;
    }

    MotionTimeSeconds = FMath::Fmod(MotionTimeSeconds + DeltaSeconds, 142.8f);
    ApplyLivelinessPose(MotionTimeSeconds);
}

void AJobBoardActor::ApplyLivelinessPose(float PhaseSeconds)
{
    // The separated pivots pin the lantern to its hanger and each paper to its top edge.
    LanternAnchor->SetRelativeRotation(FRotator(0.85f * FMath::Sin(PhaseSeconds * (2.0f * PI / 2.8f)), 0.0f, 0.0f));
    PaperAAnchor->SetRelativeRotation(FRotator(0.45f * FMath::Sin(PhaseSeconds * (2.0f * PI / 1.7f) + 0.45f), 0.0f, 0.0f));
    PaperBAnchor->SetRelativeRotation(FRotator(0.35f * FMath::Sin(PhaseSeconds * (2.0f * PI / 2.1f) + 1.15f), 0.0f, 0.0f));
}

void AJobBoardActor::ApplyRestPose()
{
    LanternAnchor->SetRelativeRotation(FRotator::ZeroRotator);
    PaperAAnchor->SetRelativeRotation(FRotator::ZeroRotator);
    PaperBAnchor->SetRelativeRotation(FRotator::ZeroRotator);
}

bool AJobBoardActor::IsPlayerCloseEnoughToAnimate() const
{
    const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    return PlayerPawn && FVector::DistSquared(PlayerPawn->GetActorLocation(), GetActorLocation())
        <= FMath::Square(AnimationCullDistance);
}

FString AJobBoardActor::GetInteractionPrompt(const AActor* Interactor) const
{
    const ALowTideGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALowTideGameMode>() : nullptr;
    if (GameMode && GameMode->GetMissionState() == EM1MissionState::NotAccepted)
    {
        return TEXT("[E] Accept job: Signal Station Logbook - 75 credits");
    }
    return GameMode && GameMode->GetMissionState() == EM1MissionState::Complete
        ? TEXT("[E] Review completed job") : TEXT("[E] Review active job");
}

bool AJobBoardActor::Interact(AActor* Interactor)
{
    ALowTideCharacter* Character = Cast<ALowTideCharacter>(Interactor);
    ALowTideGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALowTideGameMode>() : nullptr;
    return Character && GameMode && GameMode->HandleJobBoardInteraction(Character);
}
