#include "PickupActor.h"

#include "Components/StaticMeshComponent.h"
#include "LowTideCharacter.h"
#include "LowTideGameMode.h"
#include "LowTideInventoryComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

APickupActor::APickupActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
    SetRootComponent(Mesh);
    Mesh->SetRelativeScale3D(FVector(0.35f));
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
}

void APickupActor::Configure(FName InItemId, const FLinearColor& Color)
{
    ItemId = InItemId;
    if (UMaterialInstanceDynamic* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0))
    {
        Material->SetVectorParameterValue(TEXT("Color"), Color);
    }
}

FString APickupActor::GetInteractionPrompt(const AActor* Interactor) const
{
    if (const ALowTideGameMode* GameMode = GetWorld()->GetAuthGameMode<ALowTideGameMode>())
    {
        if (const FItemDefinition* Definition = GameMode->GetItemCatalog().Find(ItemId))
        {
            if (ItemId == TEXT("signal_station_logbook"))
            {
                return TEXT("[E] Recover Signal Station Logbook (mission objective)");
            }
            if (ItemId == TEXT("singing_shard"))
            {
                return TEXT("[E] Take Singing Shard - movement draws the watcher; stillness and blue wards repel it");
            }
            return FString::Printf(TEXT("[E] Collect %s"), *Definition->DisplayName);
        }
    }
    return TEXT("[E] Collect salvage");
}

bool APickupActor::Interact(AActor* Interactor)
{
    ALowTideCharacter* Character = Cast<ALowTideCharacter>(Interactor);
    if (bClaimed || !Character || FVector::DistSquared(GetActorLocation(), Character->GetActorLocation()) > FMath::Square(475.0f))
    {
        return false;
    }

    ALowTideGameMode* GameMode = GetWorld()->GetAuthGameMode<ALowTideGameMode>();
    const FItemDefinition* Definition = GameMode ? GameMode->GetItemCatalog().Find(ItemId) : nullptr;
    if (!Definition)
    {
        Character->ShowFeedback(TEXT("That salvage is not registered."));
        return false;
    }

    GameMode->BeginExpeditionIfNeeded(Character);

    FString Reason;
    const bool bProtectedObjective = ItemId == TEXT("signal_station_logbook");
    if (!(bProtectedObjective ? Character->GetInventory()->TryAddProtected(ItemId, 1, Reason)
                             : Character->GetInventory()->TryAdd(ItemId, 1, Reason)))
    {
        Character->ShowFeedback(Reason);
        return false;
    }

    bClaimed = true;
    SetActorEnableCollision(false);
    GameMode->NotifyItemCollected(Character, ItemId);
    Destroy();
    return true;
}
