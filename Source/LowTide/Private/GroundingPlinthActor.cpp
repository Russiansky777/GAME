#include "GroundingPlinthActor.h"

#include "Components/StaticMeshComponent.h"
#include "LowTideCharacter.h"
#include "LowTideGameMode.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AGroundingPlinthActor::AGroundingPlinthActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundingPlinth"));
    SetRootComponent(Mesh);
    Mesh->SetRelativeScale3D(FVector(0.65f, 0.65f, 0.35f));
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (MeshAsset.Succeeded())
    {
        Mesh->SetStaticMesh(MeshAsset.Object);
    }
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (MaterialAsset.Succeeded())
    {
        Mesh->SetMaterial(0, MaterialAsset.Object);
    }
    Tags.Add(TEXT("M1Ward"));
}

void AGroundingPlinthActor::BeginPlay()
{
    Super::BeginPlay();
    if (UMaterialInstanceDynamic* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0))
    {
        Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.08f, 0.65f, 0.72f));
    }
}

FString AGroundingPlinthActor::GetInteractionPrompt(const AActor* Interactor) const
{
    const ALowTideGameMode* GameMode = GetWorld()->GetAuthGameMode<ALowTideGameMode>();
    return GameMode && GameMode->IsArtifactCarried()
        ? TEXT("[E] Ground the Singing Shard here (relinquish it)")
        : TEXT("Grounding ward: its blue light repels the watcher");
}

bool AGroundingPlinthActor::Interact(AActor* Interactor)
{
    ALowTideCharacter* Character = Cast<ALowTideCharacter>(Interactor);
    if (!Character || FVector::DistSquared(GetActorLocation(), Character->GetActorLocation()) > FMath::Square(475.0f))
    {
        return false;
    }
    if (ALowTideGameMode* GameMode = GetWorld()->GetAuthGameMode<ALowTideGameMode>())
    {
        return GameMode->GroundArtifact(Character);
    }
    return false;
}
