#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LowTideInteractable.h"
#include "StoryClueActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class LOWTIDE_API AStoryClueActor : public AActor, public ILowTideInteractable
{
    GENERATED_BODY()

public:
    AStoryClueActor();
    virtual void BeginPlay() override;
    void Configure(const FString& InLabel, const FString& InText);
    virtual FString GetInteractionPrompt(const AActor* Interactor) const override;
    virtual bool Interact(AActor* Interactor) override;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Mesh;

    FString Label;
    FString ClueText;
};
