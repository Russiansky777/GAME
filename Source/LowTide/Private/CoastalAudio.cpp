#include "CoastalAudio.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Sound/SoundWaveProcedural.h"
#include "TimerManager.h"

namespace CoastalAudio
{
    constexpr int32 SampleRate = 22050;
    constexpr float AmbientSeconds = 7.0f;
    constexpr int32 AmbientSamples = static_cast<int32>(SampleRate * AmbientSeconds);
    constexpr int32 AmbientChunkBytes = AmbientSamples * sizeof(int16);
    constexpr int32 MaxQueuedAmbientBytes = AmbientChunkBytes * 2;
}

ACoastalAudio::ACoastalAudio()
{
    PrimaryActorTick.bCanEverTick = false;
    SetActorHiddenInGame(true);

    SurfComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("Surf"));
    SetRootComponent(SurfComponent);
    SurfComponent->bAutoActivate = false;
    SurfComponent->bIsUISound = false;
    SurfComponent->SetVolumeMultiplier(0.30f);

    WindComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("Wind"));
    WindComponent->SetupAttachment(SurfComponent);
    WindComponent->bAutoActivate = false;
    WindComponent->bIsUISound = false;
    WindComponent->SetVolumeMultiplier(0.18f);

    CueComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("Cue"));
    CueComponent->SetupAttachment(SurfComponent);
    CueComponent->bAutoActivate = false;
    CueComponent->bIsUISound = false;
    CueComponent->SetVolumeMultiplier(0.40f);
}

void ACoastalAudio::BeginPlay()
{
    Super::BeginPlay();

    // Automation uses -nosound; do not synthesize or retain PCM that no audio device can consume.
    if (!GetWorld() || FParse::Param(FCommandLine::Get(), TEXT("nosound")) || !GetWorld()->GetAudioDeviceRaw())
    {
        return;
    }

    SurfWave = CreateWave(TEXT("ProceduralSurf"));
    WindWave = CreateWave(TEXT("ProceduralWind"));
    CueWave = CreateWave(TEXT("ProceduralCue"));
    if (!SurfWave || !WindWave || !CueWave)
    {
        return;
    }

    RefillAmbience();
    SurfComponent->SetSound(SurfWave);
    WindComponent->SetSound(WindWave);
    SurfComponent->Play();
    WindComponent->Play();

    // Refill checks retain no more than two short chunks per ambience wave.
    GetWorldTimerManager().SetTimer(RefillTimer, this, &ACoastalAudio::RefillAmbience, CoastalAudio::AmbientSeconds, true);
}

void ACoastalAudio::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(RefillTimer);
    Super::EndPlay(EndPlayReason);
}

void ACoastalAudio::SetTidePhase(ETidePhase NewPhase)
{
    TidePhase = NewPhase;
    if (NewPhase == ETidePhase::Rising)
    {
        PlayCue(ECoastalAudioCue::TideRising);
    }
}

void ACoastalAudio::SetThreatIntensity(float NewIntensity)
{
    const float Previous = ThreatIntensity;
    ThreatIntensity = FMath::Clamp(NewIntensity, 0.0f, 1.0f);
    if (Previous < 0.35f && ThreatIntensity >= 0.35f)
    {
        PlayCue(ECoastalAudioCue::AnomalyAwakened);
    }
    else if (Previous >= 0.35f && ThreatIntensity < 0.15f)
    {
        PlayCue(ECoastalAudioCue::AnomalyRetreat);
    }
}

void ACoastalAudio::PlayCue(ECoastalAudioCue Cue)
{
    if (!CueWave || !CueComponent)
    {
        return;
    }
    CueComponent->Stop();
    CueWave->ResetAudio();
    QueueCue(Cue);
    CueComponent->SetSound(CueWave);
    CueComponent->Play();
}

void ACoastalAudio::RefillAmbience()
{
    if (!GetWorld() || !GetWorld()->GetAudioDeviceRaw() || !SurfWave || !WindWave)
    {
        return;
    }
    const bool bQueuedSurf = QueueAmbient(SurfWave, true, AmbientElapsed);
    const bool bQueuedWind = QueueAmbient(WindWave, false, AmbientElapsed);
    if (bQueuedSurf || bQueuedWind)
    {
        AmbientElapsed += CoastalAudio::AmbientSeconds;
    }
}

bool ACoastalAudio::QueueAmbient(USoundWaveProcedural* Wave, bool bSurf, float StartTime)
{
    if (!Wave || Wave->GetAvailableAudioByteCount() > CoastalAudio::MaxQueuedAmbientBytes - CoastalAudio::AmbientChunkBytes)
    {
        return false;
    }

    TArray<int16> Samples;
    Samples.SetNumUninitialized(CoastalAudio::AmbientSamples);
    uint32& RandomState = bSurf ? SurfNoiseState : WindNoiseState;
    float& FilteredNoise = bSurf ? SurfFilteredNoise : WindFilteredNoise;
    const float TideWeight = TidePhase == ETidePhase::Rising ? 1.25f : TidePhase == ETidePhase::High ? 1.10f : 0.85f;

    for (int32 Index = 0; Index < Samples.Num(); ++Index)
    {
        const float Time = StartTime + static_cast<float>(Index) / CoastalAudio::SampleRate;
        const float Noise = NextNoise(RandomState);
        float Value = 0.0f;
        if (bSurf)
        {
            FilteredNoise = FMath::Lerp(FilteredNoise, Noise, 0.018f);
            const float WaveSet = FMath::Pow(FMath::Max(0.0f, FMath::Sin(Time * 0.48f) * 0.5f + 0.5f), 2.0f);
            Value = (FilteredNoise * 0.060f + Noise * 0.018f) * (0.45f + WaveSet) * TideWeight;
        }
        else
        {
            FilteredNoise = FMath::Lerp(FilteredNoise, Noise, 0.006f);
            const float Gust = 0.55f + 0.35f * FMath::Sin(Time * 0.22f) + 0.10f * FMath::Sin(Time * 0.71f);
            Value = (Noise - WindPreviousNoise) * 0.028f * Gust + FilteredNoise * 0.014f;
            WindPreviousNoise = Noise;
        }
        Samples[Index] = static_cast<int16>(FMath::Clamp(Value, -0.18f, 0.18f) * 32767.0f);
    }
    Wave->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
    return true;
}

void ACoastalAudio::QueueCue(ECoastalAudioCue Cue)
{
    constexpr float Seconds = 2.8f;
    const int32 NumSamples = static_cast<int32>(CoastalAudio::SampleRate * Seconds);
    TArray<int16> Samples;
    Samples.SetNumZeroed(NumSamples);
    for (int32 Index = 0; Index < NumSamples; ++Index)
    {
        const float Time = static_cast<float>(Index) / CoastalAudio::SampleRate;
        float Value = 0.0f;
        switch (Cue)
        {
        case ECoastalAudioCue::Interaction:
            Value = (FMath::Sin(Time * 1100.0f) * FMath::Exp(-Time * 9.0f) + FMath::Sin(Time * 720.0f) * FMath::Exp(-Time * 15.0f)) * 0.18f;
            break;
        case ECoastalAudioCue::TideRising:
            Value = FMath::Sin(Time * (235.0f + Time * 34.0f) * 2.0f * PI) * FMath::Sin(FMath::Min(Time / 1.8f, 1.0f) * PI) * 0.13f;
            break;
        case ECoastalAudioCue::RouteLost:
            Value = (FMath::Sin(Time * 126.0f * 2.0f * PI) + 0.4f * FMath::Sin(Time * 189.0f * 2.0f * PI))
                * FMath::Exp(-Time * 1.25f) * 0.15f;
            break;
        case ECoastalAudioCue::AnomalyAwakened:
            Value = (FMath::Sin(Time * 53.0f * 2.0f * PI) + 0.42f * FMath::Sin(Time * 79.4f * 2.0f * PI))
                * (0.35f + 0.65f * FMath::Sin(Time * 1.8f) * FMath::Sin(Time * 1.8f)) * 0.13f;
            break;
        case ECoastalAudioCue::AnomalyRetreat:
            Value = FMath::Sin(Time * (240.0f - Time * 38.0f) * 2.0f * PI) * FMath::Exp(-Time * 1.4f) * 0.10f;
            break;
        }
        const float Attack = FMath::Clamp(Time / 0.025f, 0.0f, 1.0f);
        const float Release = FMath::Clamp((Seconds - Time) / 0.20f, 0.0f, 1.0f);
        Value *= Attack * Release;
        Samples[Index] = static_cast<int16>(FMath::Clamp(Value, -0.25f, 0.25f) * 32767.0f);
    }
    CueWave->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
}

USoundWaveProcedural* ACoastalAudio::CreateWave(const TCHAR* Name)
{
    USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this, Name);
    if (Wave)
    {
        Wave->SetSampleRate(CoastalAudio::SampleRate);
        Wave->NumChannels = 1;
        Wave->Duration = INDEFINITELY_LOOPING_DURATION;
    }
    return Wave;
}

float ACoastalAudio::NextNoise(uint32& State)
{
    State = State * 1664525u + 1013904223u;
    return static_cast<float>((State >> 8) & 0x00FFFFFF) / 8388607.5f - 1.0f;
}
