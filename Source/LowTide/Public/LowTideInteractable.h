#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LowTideInteractable.generated.h"

UINTERFACE(MinimalAPI)
class ULowTideInteractable : public UInterface
{
    GENERATED_BODY()
};

class LOWTIDE_API ILowTideInteractable
{
    GENERATED_BODY()

public:
    virtual FString GetInteractionPrompt(const AActor* Interactor) const = 0;
    virtual bool Interact(AActor* Interactor) = 0;
};
