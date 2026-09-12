#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HubDressingActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class USceneComponent;

/** Visual-only, donor-authored prop composition for the M1 trader hub. */
UCLASS()
class LOWTIDE_API AHubDressingActor : public AActor
{
    GENERATED_BODY()

public:
    AHubDressingActor();

    /** Adds the fixed hub composition once. Safe to call after spawning at world origin. */
    void BuildDressing();

private:
    void Configure(UHierarchicalInstancedStaticMeshComponent* Component);
    void Add(UHierarchicalInstancedStaticMeshComponent* Component, const FVector& Location,
        const FRotator& Rotation, const FVector& Scale = FVector::OneVector);

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Barrels;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Crates;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BottleCrates;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Chests;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Rowboats;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Paddles;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> MastRopes;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ShipWrecks;

    bool bBuilt = false;
};
