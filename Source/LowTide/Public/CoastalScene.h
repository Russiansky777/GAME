#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoastalScene.generated.h"

class ATraderActor;
class UBoxComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

USTRUCT(BlueprintType)
struct LOWTIDE_API FCoastalSceneLayout
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<AActor> WaterActor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<AActor> ShortcutBlocker;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<ATraderActor> Mara;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<AActor> RareArtifactVisual;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<AActor> PhenomenonVisual;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector PlayerStart = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector MainObjectiveLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector RareArtifactLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector PhenomenonStartLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FVector> CommonSalvageLocations;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FVector> WardLocations;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FVector> RouteClueLocations;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FVector> RouteWaypoints;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FVector> AlternateRouteWaypoints;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FVector> OptionalRouteWaypoints;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float MainRouteLengthCm = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float OptionalRouteLengthCm = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FBox SettlementSafeBounds = FBox(EForceInit::ForceInit);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FBox ExpeditionBounds = FBox(EForceInit::ForceInit);

    // Deterministic camera positions paired with ReviewLookAt points for visual QA.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FVector> ReviewViews;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FVector> ReviewLookAt;
};

/**
 * Authored M1 coastal slice. Repeated details are instanced; landmark silhouettes are
 * assembled from reusable project/engine geometry so the scene remains cook-safe and
 * reproducible without external assets.
 */
UCLASS()
class LOWTIDE_API ACoastalScene : public AActor
{
    GENERATED_BODY()

public:
    ACoastalScene();

    UFUNCTION(BlueprintCallable)
    void BuildScene();

    const FCoastalSceneLayout& GetLayout() const { return Layout; }

    UFUNCTION(BlueprintCallable)
    AActor* CreateRareArtifactVisual(const FVector& Location);

    UFUNCTION(BlueprintCallable)
    AActor* CreatePhenomenonVisual(const FVector& Location);

private:
    void InitializeLayout();
    void ConfigureInstanceFamily(UHierarchicalInstancedStaticMeshComponent* Component, UStaticMesh* Mesh,
        const FLinearColor& Color, float Roughness, bool bCollision);
    void AddInstance(UHierarchicalInstancedStaticMeshComponent* Component, const FVector& Location,
        const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator);
    void AddBoxBetween(UHierarchicalInstancedStaticMeshComponent* Component, const FVector& Start,
        const FVector& End, float Width, float Height, float ZOffset = 0.0f);
    void AddCylinderBetween(UHierarchicalInstancedStaticMeshComponent* Component, const FVector& Start,
        const FVector& End, float Radius);
    UBoxComponent* AddBoundarySegment(const FVector& Start, const FVector& End, float ZBase,
        float Height, FName Name);
    void AddFilteredRouteBoundary(const FVector& Start, const FVector& End, const FVector& OwnRouteStart,
        const FVector& OwnRouteEnd, float Height);
    bool ShouldOmitRouteBoundary(const FVector& Point, const FVector& OwnRouteStart,
        const FVector& OwnRouteEnd) const;
    AActor* SpawnMarkerActor(const FVector& Location, FName Tag);
    AActor* SpawnWater();
    AActor* SpawnShortcutBlocker();
    void BuildLighting();
    void BuildSettlement();
    void BuildMainRoute();
    void BuildAlternateRoute();
    void BuildOptionalRoute();
    void BuildPathRibbon(const TArray<FVector>& Points, UHierarchicalInstancedStaticMeshComponent* Surface,
        float Width, bool bBoundLeft, bool bBoundRight, bool bDeep);
    void BuildRockArch(const FVector& Location, float YawDegrees);
    void BuildWreck(const FVector& Location, float YawDegrees);
    void BuildSignalStation(const FVector& Location, float YawDegrees);
    void BuildShrine(const FVector& Location, float YawDegrees);
    void BuildFishingHut(const FVector& Location, float YawDegrees, float Scale);
    void BuildBoat(const FVector& Location, float YawDegrees, float Scale);
    void BuildRopeFence(const FVector& Start, const FVector& End, int32 Sections);
    void AddRockCluster(const FVector& Location, const FVector& Extent, int32 Seed, bool bDark);
    void AddWorldLabel(const FString& Text, const FVector& Location, const FRotator& Rotation,
        const FLinearColor& Color, float WorldSize = 42.0f);
    static float CalculatePolylineLength(const TArray<FVector>& Points);

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> SandInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> StoneInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> CliffInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> DarkRockInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> DeepGroundInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> WoodInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> WoodDetailInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RoofInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RopeInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> MetalInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> WarmAccentInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TideAccentInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> AnomalyInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RouteBoundaryInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RouteFloorCollision;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> CoastalTerrainSand;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> CoastalTerrainStone;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> CoastalTerrainDeep;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CylinderMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> SphereMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> ConeMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> FacetedRockMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CoastalTerrainSandMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CoastalTerrainStoneMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CoastalTerrainDeepMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> StylizedMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> WaterMaterial;

    UPROPERTY(VisibleAnywhere)
    FCoastalSceneLayout Layout;

    UPROPERTY()
    TArray<TObjectPtr<AActor>> SpawnedActors;

    UPROPERTY()
    TArray<TObjectPtr<UBoxComponent>> BoundaryComponents;

    bool bBuilt = false;
};
