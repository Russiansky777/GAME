#include "StoryClueActor.h"

#include "CoastalAudio.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "LowTideCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AStoryClueActor::AStoryClueActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClueMarker"));
    SetRootComponent(Mesh);
    Mesh->SetRelativeScale3D(FVector(0.5f, 0.18f, 0.7f));
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (MeshAsset.Succeeded())
    {
        Mesh->SetStaticMesh(MeshAsset.Object);
    }
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (MaterialAsset.Succeeded())
    {
        Mesh->SetMaterial(0, MaterialAsset.Object);
    }
    Tags.Add(TEXT("M1StoryClue"));
}

void AStoryClueActor::BeginPlay()
{
    Super::BeginPlay();
    if (UMaterialInstanceDynamic* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0))
    {
        Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.88f, 0.48f, 0.10f));
    }
}

void AStoryClueActor::Configure(const FString& InLabel, const FString& InText)
{
    Label = InLabel;
    ClueText = InText;
}

FString AStoryClueActor::GetInteractionPrompt(const AActor* Interactor) const
{
    return FString::Printf(TEXT("[E] Inspect %s"), *Label);
}

bool AStoryClueActor::Interact(AActor* Interactor)
{
    ALowTideCharacter* Character = Cast<ALowTideCharacter>(Interactor);
    if (!Character || FVector::DistSquared(GetActorLocation(), Character->GetActorLocation()) > FMath::Square(475.0f))
    {
        return false;
    }
    for (TActorIterator<ACoastalAudio> It(GetWorld()); It; ++It)
    {
        It->PlayCue(ECoastalAudioCue::Interaction);
        break;
    }
    Character->ShowFeedback(ClueText, 12.0f);
    return true;
}
