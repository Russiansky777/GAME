#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LowTideInteractable.h"
#include "PickupActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class LOWTIDE_API APickupActor : public AActor, public ILowTideInteractable
{
    GENERATED_BODY()

public:
    APickupActor();
    void Configure(FName InItemId, const FLinearColor& Color);
    virtual FString GetInteractionPrompt(const AActor* Interactor) const override;
    virtual bool Interact(AActor* Interactor) override;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY()
    FName ItemId = NAME_None;

    bool bClaimed = false;
};
