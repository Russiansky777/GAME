#include "TraderActor.h"

#include "Components/StaticMeshComponent.h"
#include "LowTideCharacter.h"
#include "LowTideGameMode.h"
#include "LowTideInventoryComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ATraderActor::ATraderActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TraderMesh"));
    SetRootComponent(Mesh);
    Mesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.7f));
    Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

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

FString ATraderActor::GetInteractionPrompt(const AActor* Interactor) const
{
    if (const ALowTideGameMode* GameMode = GetWorld()->GetAuthGameMode<ALowTideGameMode>())
    {
        if (!GameMode->IsM05Fixture())
        {
            switch (GameMode->GetMissionState())
            {
            case EM1MissionState::NotAccepted: return TEXT("[E] Ask Mara about the signal station");
            case EM1MissionState::ReturnToMara: return TEXT("[E] Give Mara the Signal Station Logbook");
            default: break;
            }
        }
    }
    return TEXT("[E] Trade with Mara");
}

bool ATraderActor::Interact(AActor* Interactor)
{
    ALowTideCharacter* Character = Cast<ALowTideCharacter>(Interactor);
    if (!Character || FVector::DistSquared(GetActorLocation(), Character->GetActorLocation()) > FMath::Square(475.0f))
    {
        return false;
    }
    if (ALowTideGameMode* GameMode = GetWorld()->GetAuthGameMode<ALowTideGameMode>())
    {
        if (!GameMode->IsM05Fixture())
        {
            return GameMode->HandleMaraInteraction(Character);
        }
    }
    Character->OpenTrader(this);
    Character->ShowFeedback(TEXT("Mara buys salvage one piece at a time."));
    return true;
}

bool ATraderActor::TrySellSlot(ALowTideCharacter* Character, int32 Slot) const
{
    if (!Character || FVector::DistSquared(GetActorLocation(), Character->GetActorLocation()) > FMath::Square(475.0f))
    {
        if (Character)
        {
            Character->ShowFeedback(TEXT("Move closer to Mara to trade."));
            Character->CloseMenus();
        }
        return false;
    }

    const ALowTideGameMode* GameMode = GetWorld()->GetAuthGameMode<ALowTideGameMode>();
    const TArray<FItemDefinition>* Items = GameMode ? &GameMode->GetItemCatalog().GetOrderedItems() : nullptr;
    if (!Items || !Items->IsValidIndex(Slot))
    {
        Character->ShowFeedback(TEXT("Invalid trade selection."));
        return false;
    }

    const FItemDefinition& Item = (*Items)[Slot];
    if (!Item.bSellable)
    {
        Character->ShowFeedback(FString::Printf(TEXT("%s is evidence and cannot be sold."), *Item.DisplayName));
        return false;
    }

    ULowTideInventoryComponent* Inventory = Character->GetInventory();
    if (Inventory->GetQuantity(Item.Id) < 1)
    {
        Character->ShowFeedback(FString::Printf(TEXT("No %s to sell."), *Item.DisplayName));
        return false;
    }
    if (!Inventory->TrySell(Item.Id, 1, Item.Value))
    {
        Character->ShowFeedback(TEXT("Sale could not be completed."));
        return false;
    }
    if (ALowTideGameMode* MutableGameMode = GetWorld()->GetAuthGameMode<ALowTideGameMode>())
    {
        MutableGameMode->NotifyItemSold(Item.Id);
    }
    Character->ShowFeedback(FString::Printf(TEXT("Sold 1 %s for %d credits."), *Item.DisplayName, Item.Value));
    return true;
}
