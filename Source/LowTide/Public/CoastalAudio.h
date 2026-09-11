#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LowTideTypes.h"
#include "CoastalAudio.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

/** Small, code-authored audio vocabulary for the M1 coastal expedition. */
UENUM(BlueprintType)
enum class ECoastalAudioCue : uint8
{
    Interaction,
    TideRising,
    RouteLost,
    AnomalyAwakened,
    AnomalyRetreat
};

/**
 * Generates short PCM buffers on the game thread and feeds them to three bounded procedural waves.
 * Place one instance in a playable world; it starts restrained surf and wind automatically.
 */
UCLASS()
class LOWTIDE_API ACoastalAudio : public AActor
{
    GENERATED_BODY()

public:
    ACoastalAudio();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void PlayCue(ECoastalAudioCue Cue);
    void SetTidePhase(ETidePhase NewPhase);
    void SetThreatIntensity(float NewIntensity);

private:
    void RefillAmbience();
    bool QueueAmbient(USoundWaveProcedural* Wave, bool bSurf, float StartTime);
    void QueueCue(ECoastalAudioCue Cue);
    USoundWaveProcedural* CreateWave(const TCHAR* Name);
    static float NextNoise(uint32& State);

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UAudioComponent> SurfComponent;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UAudioComponent> WindComponent;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UAudioComponent> CueComponent;

    UPROPERTY()
    TObjectPtr<USoundWaveProcedural> SurfWave;

    UPROPERTY()
    TObjectPtr<USoundWaveProcedural> WindWave;

    UPROPERTY()
    TObjectPtr<USoundWaveProcedural> CueWave;

    FTimerHandle RefillTimer;
    ETidePhase TidePhase = ETidePhase::Falling;
    float ThreatIntensity = 0.0f;
    uint32 SurfNoiseState = 0x4C4F5754;
    uint32 WindNoiseState = 0x434F4153;
    float SurfFilteredNoise = 0.0f;
    float WindFilteredNoise = 0.0f;
    float WindPreviousNoise = 0.0f;
    float AmbientElapsed = 0.0f;
};
