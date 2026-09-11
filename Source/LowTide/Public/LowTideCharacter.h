#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LowTideCharacter.generated.h"

class ATraderActor;
class UCameraComponent;
class ULowTideInventoryComponent;

UCLASS()
class LOWTIDE_API ALowTideCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ALowTideCharacter();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    ULowTideInventoryComponent* GetInventory() const { return Inventory; }
    FString GetInteractionPrompt() const;
    bool IsInventoryOpen() const { return bInventoryOpen; }
    ATraderActor* GetActiveTrader() const { return ActiveTrader.Get(); }
    const FString& GetFeedback() const { return Feedback; }
    void ShowFeedback(const FString& Message, float Duration = 3.0f);
    void OpenTrader(ATraderActor* Trader);
    void CloseMenus();

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void Interact();
    void ToggleInventory();
    void QuitGame();
    void SellSlot1();
    void SellSlot2();
    void SellSlot3();
    void SellSlot4();
    void SellSlot5();
    void SellSlot(int32 Slot);
    AActor* TraceInteractable() const;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<ULowTideInventoryComponent> Inventory;

    TWeakObjectPtr<AActor> FocusedActor;
    TWeakObjectPtr<ATraderActor> ActiveTrader;
    FString Feedback;
    float FeedbackUntil = 0.0f;
    bool bInventoryOpen = false;
};
