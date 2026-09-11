#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LowTideInteractable.h"
#include "TraderActor.generated.h"

class ALowTideCharacter;
class UStaticMeshComponent;

UCLASS()
class LOWTIDE_API ATraderActor : public AActor, public ILowTideInteractable
{
    GENERATED_BODY()

public:
    ATraderActor();
    virtual FString GetInteractionPrompt(const AActor* Interactor) const override;
    virtual bool Interact(AActor* Interactor) override;
    bool TrySellSlot(ALowTideCharacter* Character, int32 Slot) const;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Mesh;
};
