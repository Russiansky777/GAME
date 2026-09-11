#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CoastalScene.h"
#include "LowTideItemCatalog.h"
#include "LowTideTypes.h"
#include "LowTideGameMode.generated.h"

class APickupActor;
class ACoastalAudio;
class ACoastalDressing;
class ACoastalScene;
class AGroundingPlinthActor;
class AStoryClueActor;
class ALowTideCharacter;
class ATideController;
class UMaterialInterface;
class UStaticMesh;

enum class EM1ReviewView : uint8
{
    None,
    Shore,
    Return,
    Wreck,
    Station,
    Shrine
};

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
    bool HandleMaraInteraction(ALowTideCharacter* Character);
    void NotifyItemCollected(ALowTideCharacter* Character, FName ItemId);
    void NotifyItemSold(FName ItemId);
    bool GroundArtifact(ALowTideCharacter* Character);
    bool IsArtifactCarried() const;
    bool IsPhenomenonActive() const { return bPhenomenonActive; }
    float GetPhenomenonDistance() const { return PhenomenonDistance; }
    AActor* GetPhenomenonActor() const { return SceneLayout.PhenomenonVisual; }
    EM1MissionState GetMissionState() const { return MissionState; }
    FString GetObjectiveText() const;
    bool IsM05Fixture() const { return bM05Fixture; }
    const FCoastalSceneLayout& GetSceneLayout() const { return SceneLayout; }

private:
    class AStaticMeshActor* SpawnPrimitive(UStaticMesh* Mesh, const FVector& Location, const FVector& Scale,
        const FLinearColor& Color, bool bCollision, bool bMovable = false);
    void BuildGreybox();
    void BuildM1Slice();
    void SpawnM1Pickups();
    APickupActor* SpawnPickup(FName ItemId, const FVector& Location, const FLinearColor& Color);
    void UpdatePhenomenon(ALowTideCharacter* Character, float DeltaSeconds);
    void SetPhenomenonActive(bool bActive);
    bool IsInSafeSettlement(const FVector& Location) const;
    void ApplyReviewStart();
    void SpawnSalvageForCycle();
    void HandleTidePhaseChanged(ETidePhase NewPhase);
    void HandleAccessChanged(bool bOpen);
    void RecoverStrandedPlayer(const FString& Trigger = TEXT("Access submerged"));
    void CompleteExpedition();
    void SpawnInvisibleBoundary(const FVector& Location, const FVector& Scale);
    EM1ReviewView ParseReviewViewName(const FString& ReviewName) const;
    EM1ReviewView ParseReviewViewFromCommandLine(const TCHAR* CommandLineKey) const;
    int32 GetReviewViewIndex(EM1ReviewView View) const;
    FString GetReviewViewToken(EM1ReviewView View) const;
    void ApplyReviewView(EM1ReviewView View, bool bIgnoreInputForReviewCapture);
    void BeginReviewCapture(const FString& CommandLineSwitch);
    void RequestReviewCaptureScreenshot();
    void OnReviewCaptureScreenshotProcessed();
    void QueueReviewCaptureExit();
    void ExitGameForReviewCapture();

    FLowTideItemCatalog ItemCatalog;
    FString CatalogError;

    UPROPERTY()
    TObjectPtr<ATideController> TideController;

    UPROPERTY()
    TObjectPtr<ACoastalScene> CoastalScene;

    UPROPERTY()
    TObjectPtr<ACoastalAudio> CoastalAudio;

    UPROPERTY()
    TObjectPtr<ACoastalDressing> CoastalDressing;

    UPROPERTY()
    TArray<TObjectPtr<AGroundingPlinthActor>> GroundingPlinths;

    UPROPERTY()
    TArray<TObjectPtr<AStoryClueActor>> StoryClues;

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
    bool bM05Fixture = false;
    bool bM1PickupsSpawned = false;
    bool bRareArtifactClaimed = false;
    bool bRareArtifactResolved = false;
    bool bPhenomenonActive = false;
    bool bWatcherFarWarningShown = false;
    bool bWatcherNearWarningShown = false;
    float PhenomenonDistance = TNumericLimits<float>::Max();
    float WetExposureSeconds = 0.0f;
    bool bWetGroundWarningShown = false;
    EM1MissionState MissionState = EM1MissionState::NotAccepted;
    FCoastalSceneLayout SceneLayout;
    EM1ReviewView ReviewView = EM1ReviewView::None;
    FVector ReviewViewLocation = FVector::ZeroVector;
    FRotator ReviewViewRotation = FRotator::ZeroRotator;
    bool bReviewCaptureRequested = false;
    bool bReviewCaptureDispatched = false;
    bool bReviewCaptureCompleted = false;
    float ReviewCaptureSettleCountdown = 0.0f;
    float ReviewCaptureFallbackSeconds = 0.0f;
    FTimerHandle ReviewCaptureExitTimer;
    FString ReviewCaptureFilepath;

    static constexpr float ReviewCaptureSettleDelaySeconds = 3.0f;
    static constexpr float ReviewCaptureExitDelaySeconds = 2.0f;
    static constexpr float ReviewCaptureFallbackTimeoutSeconds = 15.0f;

    static constexpr float SettlementEdgeX = 700.0f;
    static constexpr float SettlementReturnX = 650.0f;
    static constexpr int32 MissionRewardCredits = 75;
};
