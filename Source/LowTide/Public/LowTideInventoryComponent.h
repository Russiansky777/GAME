#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LowTideInventoryComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FLowTideInventoryChanged);

UCLASS(ClassGroup=(LowTide), meta=(BlueprintSpawnableComponent))
class LOWTIDE_API ULowTideInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULowTideInventoryComponent();

    bool TryAdd(FName ItemId, int32 Quantity, FString& OutReason);
    bool TryAddProtected(FName ItemId, int32 Quantity, FString& OutReason);
    bool TryRemove(FName ItemId, int32 Quantity);
    bool TrySell(FName ItemId, int32 Quantity, int32 CreditValue);
    int32 GetQuantity(FName ItemId) const;
    int32 GetUsedCapacity() const;
    int32 GetCapacity() const { return Capacity; }
    int32 GetCredits() const { return Credits; }
    void AddCredits(int32 Amount);

    FLowTideInventoryChanged OnInventoryChanged;

private:
    UPROPERTY()
    TMap<FName, int32> Quantities;

    UPROPERTY()
    TSet<FName> CapacityExemptItems;

    UPROPERTY(EditDefaultsOnly, Category="Inventory")
    int32 Capacity = 8;

    UPROPERTY()
    int32 Credits = 0;
};
