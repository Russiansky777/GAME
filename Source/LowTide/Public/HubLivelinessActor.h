#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HubLivelinessActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/** A single authored parrot and its stand for the trader hub. */
UCLASS()
class LOWTIDE_API AHubLivelinessActor : public AActor
{
    GENERATED_BODY()

public:
    AHubLivelinessActor();

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Parrot;
};
