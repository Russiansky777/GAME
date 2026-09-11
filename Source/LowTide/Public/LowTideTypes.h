#pragma once

#include "CoreMinimal.h"
#include "LowTideTypes.generated.h"

USTRUCT(BlueprintType)
struct FItemDefinition
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FName Id = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString DisplayName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString Description;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Value = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bSellable = false;
};

UENUM()
enum class ETidePhase : uint8
{
    Falling,
    Low,
    Rising,
    High
};

UENUM()
enum class EM1MissionState : uint8
{
    NotAccepted,
    FindLogbook,
    ReturnToMara,
    Complete
};
