#include "TideController.h"

ATideController::ATideController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ATideController::Configure(AActor* InWater, AActor* InCausewayBlocker, bool bInM05Fixture)
{
    Water = InWater;
    CausewayBlocker = InCausewayBlocker;
    bM05Fixture = bInM05Fixture;
    PhaseElapsed = 0.0f;
    if (bM05Fixture)
    {
        Phase = ETidePhase::Falling;
        LowCycle = 0;
        bClockRunning = true;
    }
    else
    {
        // The M1 expedition is already exposed, but time does not advance until Mara briefs the player.
        Phase = ETidePhase::Low;
        LowCycle = 1;
        bClockRunning = false;
    }
    ApplyState();
}

void ATideController::StartClock()
{
    bClockRunning = true;
}

void ATideController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bClockRunning)
    {
        ApplyState();
        return;
    }
    PhaseElapsed += DeltaSeconds;
    while (PhaseElapsed >= GetPhaseDuration())
    {
        PhaseElapsed -= GetPhaseDuration();
        AdvancePhase();
    }
    ApplyState();
}

float ATideController::GetPhaseDuration() const
{
    if (bM05Fixture)
    {
        switch (Phase)
        {
        case ETidePhase::Falling: return M05FallingDuration;
        case ETidePhase::Low: return M05LowDuration;
        case ETidePhase::Rising: return M05RisingDuration;
        case ETidePhase::High: return M05HighDuration;
        default: return M05LowDuration;
        }
    }
    switch (Phase)
    {
    case ETidePhase::Falling: return M1FallingDuration;
    case ETidePhase::Low: return M1LowDuration;
    case ETidePhase::Rising: return M1RisingDuration;
    case ETidePhase::High: return M1HighDuration;
    default: return M1LowDuration;
    }
}

float ATideController::GetSecondsRemaining() const
{
    return FMath::Max(0.0f, GetPhaseDuration() - PhaseElapsed);
}

float ATideController::GetSecondsUntilAccessCloses() const
{
    if (!bAccessOpen)
    {
        return 0.0f;
    }

    const float LowDuration = bM05Fixture ? M05LowDuration : M1LowDuration;
    const float RisingDuration = bM05Fixture ? M05RisingDuration : M1RisingDuration;
    const float LowWaterZ = bM05Fixture ? M05LowWaterZ : M1LowWaterZ;
    const float ClosingWaterZ = bM05Fixture ? M05AccessClosingWaterZ : M1ShortcutClosingWaterZ;
    const float RisingOpenDuration = RisingDuration * (ClosingWaterZ - LowWaterZ) / (HighWaterZ - LowWaterZ);
    switch (Phase)
    {
    case ETidePhase::Falling: return GetSecondsRemaining() + LowDuration + RisingOpenDuration;
    case ETidePhase::Low: return GetSecondsRemaining() + RisingOpenDuration;
    case ETidePhase::Rising: return FMath::Max(0.0f, RisingOpenDuration - PhaseElapsed);
    default: return 0.0f;
    }
}

FString ATideController::GetPhaseName() const
{
    switch (Phase)
    {
    case ETidePhase::Falling: return TEXT("FALLING");
    case ETidePhase::Low: return TEXT("LOW - CAUSEWAY OPEN");
    case ETidePhase::Rising: return TEXT("RISING");
    case ETidePhase::High: return TEXT("HIGH");
    default: return TEXT("UNKNOWN");
    }
}

bool ATideController::IsClosingWarning() const
{
    const float WarningDuration = bM05Fixture ? M05ClosingWarningDuration : M1ClosingWarningDuration;
    return bAccessOpen && bClockRunning && GetSecondsUntilAccessCloses() <= WarningDuration;
}

void ATideController::AdvancePhase()
{
    switch (Phase)
    {
    case ETidePhase::Falling:
        Phase = ETidePhase::Low;
        ++LowCycle;
        break;
    case ETidePhase::Low:
        Phase = ETidePhase::Rising;
        break;
    case ETidePhase::Rising:
        Phase = ETidePhase::High;
        break;
    case ETidePhase::High:
        Phase = ETidePhase::Falling;
        break;
    }
    OnPhaseChanged.Broadcast(Phase);
}

void ATideController::ApplyState()
{
    const float FallingDuration = bM05Fixture ? M05FallingDuration : M1FallingDuration;
    const float RisingDuration = bM05Fixture ? M05RisingDuration : M1RisingDuration;
    const float LowWaterZ = bM05Fixture ? M05LowWaterZ : M1LowWaterZ;
    const float ClosingWaterZ = bM05Fixture ? M05AccessClosingWaterZ : M1ShortcutClosingWaterZ;
    float WaterZ = HighWaterZ;
    if (Phase == ETidePhase::Falling)
    {
        WaterZ = FMath::Lerp(HighWaterZ, LowWaterZ, PhaseElapsed / FallingDuration);
    }
    else if (Phase == ETidePhase::Low)
    {
        WaterZ = LowWaterZ;
    }
    else if (Phase == ETidePhase::Rising)
    {
        WaterZ = FMath::Lerp(LowWaterZ, HighWaterZ, PhaseElapsed / RisingDuration);
    }

    if (Water)
    {
        FVector Location = Water->GetActorLocation();
        Location.Z = WaterZ;
        Water->SetActorLocation(Location);
    }
    CurrentWaterZ = WaterZ;
    // The water cube is 20 cm thick. Collision follows the visible surface crossing the path top.
    const bool bNewAccessOpen = WaterZ < ClosingWaterZ;
    if (CausewayBlocker)
    {
        CausewayBlocker->SetActorEnableCollision(!bNewAccessOpen);
    }
    if (bNewAccessOpen != bAccessOpen)
    {
        bAccessOpen = bNewAccessOpen;
        OnAccessChanged.Broadcast(bAccessOpen);
    }
}
