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
    void Configure(AActor* InWater, AActor* InCausewayBlocker, bool bInM05Fixture);
    void StartClock();

    ETidePhase GetPhase() const { return Phase; }
    float GetSecondsRemaining() const;
    float GetSecondsUntilAccessCloses() const;
    FString GetPhaseName() const;
    int32 GetLowCycle() const { return LowCycle; }
    bool IsAccessOpen() const { return bAccessOpen; }
    bool IsClockRunning() const { return bClockRunning; }
    float GetWaterSurfaceZ() const { return CurrentWaterZ; }
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
    bool bClockRunning = true;
    bool bM05Fixture = false;
    float CurrentWaterZ = 40.0f;

    static constexpr float M05FallingDuration = 20.0f;
    static constexpr float M05LowDuration = 60.0f;
    static constexpr float M05RisingDuration = 30.0f;
    static constexpr float M05HighDuration = 10.0f;
    static constexpr float M1FallingDuration = 20.0f;
    static constexpr float M1LowDuration = 300.0f;
    static constexpr float M1RisingDuration = 180.0f;
    static constexpr float M1HighDuration = 30.0f;
    static constexpr float M05LowWaterZ = -20.0f;
    static constexpr float M1LowWaterZ = -140.0f;
    static constexpr float HighWaterZ = 40.0f;
    static constexpr float M05AccessClosingWaterZ = 8.0f;
    static constexpr float M1ShortcutClosingWaterZ = -50.0f;
    static constexpr float M05ClosingWarningDuration = 20.0f;
    static constexpr float M1ClosingWarningDuration = 60.0f;
};
