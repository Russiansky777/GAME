#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LowTideInteractable.h"
#include "JobBoardActor.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class USceneComponent;

UCLASS()
class LOWTIDE_API AJobBoardActor : public AActor, public ILowTideInteractable
{
    GENERATED_BODY()

public:
    AJobBoardActor();
    virtual void Tick(float DeltaSeconds) override;
    virtual FString GetInteractionPrompt(const AActor* Interactor) const override;
    virtual bool Interact(AActor* Interactor) override;

#if WITH_DEV_AUTOMATION_TESTS
    void ApplyLivelinessPoseForTesting(float PhaseSeconds) { ApplyLivelinessPose(PhaseSeconds); }
#endif

private:
    void ApplyLivelinessPose(float PhaseSeconds);
    void ApplyRestPose();
    bool IsPlayerCloseEnoughToAnimate() const;
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> InteractionBody;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> LeftPostBody;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> RightPostBody;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> NoticePanelBody;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> BoardMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> LanternAnchor;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> LanternMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> PaperAAnchor;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> PaperAMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> PaperBAnchor;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> PaperBMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UPointLightComponent> LanternLight;

    float MotionTimeSeconds = 0.0f;
    float DistanceCheckSeconds = 0.0f;
    bool bLivelinessActive = true;

    static constexpr float NearTickInterval = 1.0f / 30.0f;
    static constexpr float FarTickInterval = 0.5f;
    static constexpr float AnimationCullDistance = 3200.0f;
};
