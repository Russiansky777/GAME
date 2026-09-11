#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LowTideItemCatalog.h"
#include "LowTideTypes.h"
#include "LowTideGameMode.generated.h"

class APickupActor;
class ALowTideCharacter;
class ATideController;
class UMaterialInterface;
class UStaticMesh;

UCLASS()
class LOWTIDE_API ALowTideGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ALowTideGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void RestartPlayer(AController* NewPlayer) override;

    const FLowTideItemCatalog& GetItemCatalog() const { return ItemCatalog; }
    ATideController* GetTideController() const { return TideController; }
    const FString& GetCatalogError() const { return CatalogError; }
    void BeginExpeditionIfNeeded(const ALowTideCharacter* Character);

private:
    class AStaticMeshActor* SpawnPrimitive(UStaticMesh* Mesh, const FVector& Location, const FVector& Scale,
        const FLinearColor& Color, bool bCollision, bool bMovable = false);
    void BuildGreybox();
    void SpawnSalvageForCycle();
    void HandleTidePhaseChanged(ETidePhase NewPhase);
    void HandleAccessChanged(bool bOpen);
    void RecoverStrandedPlayer();
    void CompleteExpedition();
    void SpawnInvisibleBoundary(const FVector& Location, const FVector& Scale);

    FLowTideItemCatalog ItemCatalog;
    FString CatalogError;

    UPROPERTY()
    TObjectPtr<ATideController> TideController;

    UPROPERTY()
    TArray<TObjectPtr<APickupActor>> ActivePickups;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CylinderMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> BasicMaterial;

    int32 SpawnedLowCycle = INDEX_NONE;
    FVector SafePlayerLocation = FVector(-300.0f, 0.0f, 190.0f);
    TMap<FName, int32> ExpeditionStartQuantities;
    bool bExpeditionActive = false;

    static constexpr float SettlementEdgeX = 700.0f;
    static constexpr float SettlementReturnX = 650.0f;
};
