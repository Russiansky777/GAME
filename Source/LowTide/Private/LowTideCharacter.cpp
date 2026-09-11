#include "LowTideCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LowTideInteractable.h"
#include "LowTideInventoryComponent.h"
#include "TraderActor.h"

ALowTideCharacter::ALowTideCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 92.0f);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->JumpZVelocity = 0.0f;

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(-10.0f, 0.0f, 64.0f));
    FirstPersonCamera->bUsePawnControlRotation = true;

    Inventory = CreateDefaultSubobject<ULowTideInventoryComponent>(TEXT("Inventory"));
}

void ALowTideCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    FocusedActor = ActiveTrader.IsValid() || bInventoryOpen ? nullptr : TraceInteractable();
    if (!Feedback.IsEmpty() && GetWorld()->GetTimeSeconds() >= FeedbackUntil)
    {
        Feedback.Reset();
    }
}

void ALowTideCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ALowTideCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ALowTideCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ALowTideCharacter::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ALowTideCharacter::LookUp);
    PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &ALowTideCharacter::Interact);
    PlayerInputComponent->BindAction(TEXT("Inventory"), IE_Pressed, this, &ALowTideCharacter::ToggleInventory);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &ALowTideCharacter::StartSprint);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &ALowTideCharacter::StopSprint);
    PlayerInputComponent->BindAction(TEXT("Quit"), IE_Pressed, this, &ALowTideCharacter::QuitGame);
    PlayerInputComponent->BindAction(TEXT("Sell1"), IE_Pressed, this, &ALowTideCharacter::SellSlot1);
    PlayerInputComponent->BindAction(TEXT("Sell2"), IE_Pressed, this, &ALowTideCharacter::SellSlot2);
    PlayerInputComponent->BindAction(TEXT("Sell3"), IE_Pressed, this, &ALowTideCharacter::SellSlot3);
    PlayerInputComponent->BindAction(TEXT("Sell4"), IE_Pressed, this, &ALowTideCharacter::SellSlot4);
    PlayerInputComponent->BindAction(TEXT("Sell5"), IE_Pressed, this, &ALowTideCharacter::SellSlot5);
    PlayerInputComponent->BindAction(TEXT("Sell6"), IE_Pressed, this, &ALowTideCharacter::SellSlot6);
    PlayerInputComponent->BindAction(TEXT("Sell7"), IE_Pressed, this, &ALowTideCharacter::SellSlot7);
    PlayerInputComponent->BindAction(TEXT("Sell8"), IE_Pressed, this, &ALowTideCharacter::SellSlot8);
}

void ALowTideCharacter::MoveForward(float Value)
{
    if (!FMath::IsNearlyZero(Value) && !bInventoryOpen && !ActiveTrader.IsValid())
    {
        AddMovementInput(GetActorForwardVector(), Value);
    }
}

void ALowTideCharacter::MoveRight(float Value)
{
    if (!FMath::IsNearlyZero(Value) && !bInventoryOpen && !ActiveTrader.IsValid())
    {
        AddMovementInput(GetActorRightVector(), Value);
    }
}

void ALowTideCharacter::Turn(float Value)
{
    if (!bInventoryOpen && !ActiveTrader.IsValid())
    {
        AddControllerYawInput(Value);
    }
}

void ALowTideCharacter::LookUp(float Value)
{
    if (!bInventoryOpen && !ActiveTrader.IsValid())
    {
        AddControllerPitchInput(Value);
    }
}

void ALowTideCharacter::StartSprint()
{
    bSprinting = true;
    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void ALowTideCharacter::StopSprint()
{
    bSprinting = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

AActor* ALowTideCharacter::TraceInteractable() const
{
    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + FirstPersonCamera->GetForwardVector() * 450.0f;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(LowTideInteraction), false, this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)
        && Hit.GetActor() && Hit.GetActor()->Implements<ULowTideInteractable>())
    {
        return Hit.GetActor();
    }
    return nullptr;
}

FString ALowTideCharacter::GetInteractionPrompt() const
{
    if (const AActor* Actor = FocusedActor.Get())
    {
        if (const ILowTideInteractable* Interactable = Cast<ILowTideInteractable>(Actor))
        {
            return Interactable->GetInteractionPrompt(this);
        }
    }
    return FString();
}

void ALowTideCharacter::Interact()
{
    if (ActiveTrader.IsValid())
    {
        CloseMenus();
        return;
    }
    if (bInventoryOpen)
    {
        bInventoryOpen = false;
        return;
    }
    if (AActor* Actor = FocusedActor.Get())
    {
        if (ILowTideInteractable* Interactable = Cast<ILowTideInteractable>(Actor))
        {
            Interactable->Interact(this);
        }
    }
}

void ALowTideCharacter::ToggleInventory()
{
    if (ActiveTrader.IsValid())
    {
        CloseMenus();
    }
    bInventoryOpen = !bInventoryOpen;
}

void ALowTideCharacter::QuitGame()
{
    UKismetSystemLibrary::QuitGame(this, Cast<APlayerController>(GetController()), EQuitPreference::Quit, false);
}

void ALowTideCharacter::ShowFeedback(const FString& Message, float Duration)
{
    Feedback = Message;
    FeedbackUntil = GetWorld()->GetTimeSeconds() + Duration;
}

void ALowTideCharacter::OpenTrader(ATraderActor* Trader)
{
    ActiveTrader = Trader;
    bInventoryOpen = false;
}

void ALowTideCharacter::CloseMenus()
{
    ActiveTrader.Reset();
    bInventoryOpen = false;
}

void ALowTideCharacter::ResetMovementAfterRecovery()
{
    StopSprint();
    GetCharacterMovement()->StopMovementImmediately();
}

void ALowTideCharacter::SellSlot(int32 Slot)
{
    if (ATraderActor* Trader = ActiveTrader.Get())
    {
        Trader->TrySellSlot(this, Slot);
    }
}

void ALowTideCharacter::SellSlot1() { SellSlot(0); }
void ALowTideCharacter::SellSlot2() { SellSlot(1); }
void ALowTideCharacter::SellSlot3() { SellSlot(2); }
void ALowTideCharacter::SellSlot4() { SellSlot(3); }
void ALowTideCharacter::SellSlot5() { SellSlot(4); }
void ALowTideCharacter::SellSlot6() { SellSlot(5); }
void ALowTideCharacter::SellSlot7() { SellSlot(6); }
void ALowTideCharacter::SellSlot8() { SellSlot(7); }
