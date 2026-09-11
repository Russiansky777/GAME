#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LowTideInteractable.h"
#include "GroundingPlinthActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class LOWTIDE_API AGroundingPlinthActor : public AActor, public ILowTideInteractable
{
    GENERATED_BODY()

public:
    AGroundingPlinthActor();
    virtual void BeginPlay() override;
    virtual FString GetInteractionPrompt(const AActor* Interactor) const override;
    virtual bool Interact(AActor* Interactor) override;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Mesh;
};
