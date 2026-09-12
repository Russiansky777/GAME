#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HubLivelinessActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/** A cheap, scene-specific perched parrot and optional hanging lantern loop for the trader hub. */
UCLASS()
class LOWTIDE_API AHubLivelinessActor : public AActor
{
    GENERATED_BODY()

public:
    AHubLivelinessActor();
    virtual void Tick(float DeltaSeconds) override;

    /** Lets the hub composition hide the optional lantern when only the bird is wanted. */
    void SetLanternEnabled(bool bEnabled);

#if WITH_DEV_AUTOMATION_TESTS
    void ApplyIdlePoseForTesting(float PhaseSeconds) { ApplyIdlePose(PhaseSeconds); }
#endif

private:
    void ApplyIdlePose(float PhaseSeconds);
    void ApplyRestPose();
    bool IsPlayerCloseEnoughToAnimate() const;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Perch;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> BirdRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Head;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> LeftWing;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> RightWing;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Tail;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> LanternAnchor;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> LanternRope;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Lantern;

    float AnimationTime = 0.0f;
    float DistanceCheckTime = 0.0f;
    bool bAnimate = true;
    bool bLanternEnabled = false;

    static constexpr float NearTickInterval = 1.0f / 30.0f;
    static constexpr float FarTickInterval = 0.5f;
    static constexpr float AnimationCullDistance = 3200.0f;
};
