#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LowTideTypes.h"
#include "TideController.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FLowTidePhaseChanged, ETidePhase);
DECLARE_MULTICAST_DELEGATE_OneParam(FLowTideAccessChanged, bool);

UCLASS()
class LOWTIDE_API ATideController : public AActor
{
    GENERATED_BODY()

public:
    ATideController();
    virtual void Tick(float DeltaSeconds) override;
    void Configure(AActor* InWater, AActor* InCausewayBlocker);

    ETidePhase GetPhase() const { return Phase; }
    float GetSecondsRemaining() const;
    float GetSecondsUntilAccessCloses() const;
    FString GetPhaseName() const;
    int32 GetLowCycle() const { return LowCycle; }
    bool IsAccessOpen() const { return bAccessOpen; }
    bool IsClosingWarning() const;

    FLowTidePhaseChanged OnPhaseChanged;
    FLowTideAccessChanged OnAccessChanged;

private:
    void AdvancePhase();
    void ApplyState();
    float GetPhaseDuration() const;

    UPROPERTY()
    TObjectPtr<AActor> Water;

    UPROPERTY()
    TObjectPtr<AActor> CausewayBlocker;

    ETidePhase Phase = ETidePhase::Falling;
    float PhaseElapsed = 0.0f;
    int32 LowCycle = 0;
    bool bAccessOpen = false;

    static constexpr float FallingDuration = 20.0f;
    static constexpr float LowDuration = 60.0f;
    static constexpr float RisingDuration = 30.0f;
    static constexpr float HighDuration = 10.0f;
    static constexpr float LowWaterZ = -20.0f;
    static constexpr float HighWaterZ = 40.0f;
    static constexpr float AccessClosingWaterZ = 8.0f;
    static constexpr float ClosingWarningDuration = 20.0f;
};
