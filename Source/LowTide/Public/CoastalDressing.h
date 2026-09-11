#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoastalDressing.generated.h"

struct FCoastalSceneLayout;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;

/**
 * Visual-only coastal set dressing for the M1 opening cove. All components use
 * NoCollision, leaving authored navigation and containment entirely to ACoastalScene.
 */
UCLASS()
class LOWTIDE_API ACoastalDressing : public AActor
{
    GENERATED_BODY()

public:
    ACoastalDressing();

    void BuildDressing(const FCoastalSceneLayout& Layout);

private:
    void ConfigureFamily(UHierarchicalInstancedStaticMeshComponent* Component, UStaticMesh* Mesh,
        const FLinearColor& Color, float Roughness);
    void Add(UHierarchicalInstancedStaticMeshComponent* Component, const FVector& Location,
        const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator);
    void AddHutFinish(const FVector& Location, float YawDegrees, float Scale);
    void AddRouteEntrance(const FCoastalSceneLayout& Layout);
    void AddSignalStationHero(const FCoastalSceneLayout& Layout);
    void AddShrineHero(const FCoastalSceneLayout& Layout);
    void AddBeam(UHierarchicalInstancedStaticMeshComponent* Component, const FVector& Start,
        const FVector& End, float Thickness);
    void AddBasaltSkirt(const FVector& Center, const FVector& Extent, int32 Seed);
    void AddGrassClump(const FVector& Location, float Scale, float YawDegrees);

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> CreamInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TealInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> CoralInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> VioletInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GrassInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> MarketInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BasaltInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> DistantCliffInstances;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CylinderMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> ConeMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> FacetedRockMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> StylizedMaterial;

    bool bBuilt = false;
};
