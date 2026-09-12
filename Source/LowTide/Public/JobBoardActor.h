#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LowTideInteractable.h"
#include "JobBoardActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class USceneComponent;

UCLASS()
class LOWTIDE_API AJobBoardActor : public AActor, public ILowTideInteractable
{
    GENERATED_BODY()

public:
    AJobBoardActor();
    virtual FString GetInteractionPrompt(const AActor* Interactor) const override;
    virtual bool Interact(AActor* Interactor) override;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> InteractionBody;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> BoardMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UTextRenderComponent> Header;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UTextRenderComponent> JobNote;
};
