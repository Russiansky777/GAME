#include "JobBoardActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "LowTideCharacter.h"
#include "LowTideGameMode.h"
#include "UObject/ConstructorHelpers.h"

AJobBoardActor::AJobBoardActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
    InteractionBody = CreateDefaultSubobject<UBoxComponent>(TEXT("JobBoardInteractionBody"));
    InteractionBody->SetupAttachment(SceneRoot);
    InteractionBody->SetBoxExtent(FVector(28.0f, 175.0f, 175.0f));
    InteractionBody->SetRelativeLocation(FVector(0.0f, 0.0f, 175.0f));
    InteractionBody->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    BoardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("JobBoardMesh"));
    BoardMesh->SetupAttachment(SceneRoot);
    BoardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(
        TEXT("/Game/Generated/HubSlice/SM_LT_HubSlice_JobBoard.SM_LT_HubSlice_JobBoard"));
    if (MeshAsset.Succeeded())
    {
        BoardMesh->SetStaticMesh(MeshAsset.Object);
    }

    Header = CreateDefaultSubobject<UTextRenderComponent>(TEXT("JobBoardHeader"));
    Header->SetupAttachment(SceneRoot);
    Header->SetRelativeLocation(FVector(-40.5f, 0.0f, 269.0f));
    Header->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    Header->SetHorizontalAlignment(EHTA_Center);
    Header->SetText(FText::FromString(TEXT("JOBS")));
    Header->SetVerticalAlignment(EVRTA_TextCenter);
    Header->SetWorldSize(16.0f);
    Header->SetTextRenderColor(FColor(66, 34, 20));

    JobNote = CreateDefaultSubobject<UTextRenderComponent>(TEXT("JobBoardActiveNote"));
    JobNote->SetupAttachment(SceneRoot);
    JobNote->SetRelativeLocation(FVector(-31.0f, 18.0f, 225.0f));
    JobNote->SetRelativeRotation(FRotator(0.0f, 177.0f, 0.0f));
    JobNote->SetHorizontalAlignment(EHTA_Center);
    JobNote->SetText(FText::FromString(TEXT("SIGNAL LOGBOOK\n75 CREDITS")));
    JobNote->SetVerticalAlignment(EVRTA_TextCenter);
    JobNote->SetWorldSize(9.0f);
    JobNote->SetTextRenderColor(FColor(72, 40, 24));
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
