#include "TideController.h"

ATideController::ATideController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ATideController::Configure(AActor* InWater, AActor* InCausewayBlocker)
{
    Water = InWater;
    CausewayBlocker = InCausewayBlocker;
    ApplyState();
}

void ATideController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
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
    switch (Phase)
    {
    case ETidePhase::Falling: return FallingDuration;
    case ETidePhase::Low: return LowDuration;
    case ETidePhase::Rising: return RisingDuration;
    case ETidePhase::High: return HighDuration;
    default: return LowDuration;
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

    const float RisingOpenDuration = RisingDuration * (AccessClosingWaterZ - LowWaterZ) / (HighWaterZ - LowWaterZ);
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
    return bAccessOpen && GetSecondsUntilAccessCloses() <= ClosingWarningDuration;
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
    // The water cube is 20 cm thick. Collision follows the visible surface crossing the path top.
    const bool bNewAccessOpen = WaterZ < AccessClosingWaterZ;
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
