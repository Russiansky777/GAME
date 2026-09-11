#include "LowTideInventoryComponent.h"

ULowTideInventoryComponent::ULowTideInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool ULowTideInventoryComponent::TryAdd(FName ItemId, int32 Quantity, FString& OutReason)
{
    if (ItemId.IsNone() || Quantity <= 0)
    {
        OutReason = TEXT("Invalid item transfer.");
        return false;
    }
    const int32 Used = GetUsedCapacity();
    if (Used > Capacity || Quantity > Capacity - Used)
    {
        OutReason = FString::Printf(TEXT("Pack full (%d/%d). Sell salvage before collecting more."), GetUsedCapacity(), Capacity);
        return false;
    }

    Quantities.FindOrAdd(ItemId) += Quantity;
    OnInventoryChanged.Broadcast();
    OutReason.Reset();
    return true;
}

bool ULowTideInventoryComponent::TryAddProtected(FName ItemId, int32 Quantity, FString& OutReason)
{
    if (ItemId.IsNone() || Quantity <= 0)
    {
        OutReason = TEXT("Invalid protected item transfer.");
        return false;
    }
    Quantities.FindOrAdd(ItemId) += Quantity;
    CapacityExemptItems.Add(ItemId);
    OnInventoryChanged.Broadcast();
    OutReason.Reset();
    return true;
}

bool ULowTideInventoryComponent::TrySell(FName ItemId, int32 Quantity, int32 CreditValue)
{
    int32* Existing = Quantities.Find(ItemId);
    if (!Existing || Quantity <= 0 || CreditValue < 0 || *Existing < Quantity || Credits > MAX_int32 - CreditValue)
    {
        return false;
    }

    *Existing -= Quantity;
    if (*Existing == 0)
    {
        Quantities.Remove(ItemId);
        CapacityExemptItems.Remove(ItemId);
    }
    Credits += CreditValue;
    OnInventoryChanged.Broadcast();
    return true;
}

bool ULowTideInventoryComponent::TryRemove(FName ItemId, int32 Quantity)
{
    int32* Existing = Quantities.Find(ItemId);
    if (!Existing || Quantity <= 0 || *Existing < Quantity)
    {
        return false;
    }

    *Existing -= Quantity;
    if (*Existing == 0)
    {
        Quantities.Remove(ItemId);
        CapacityExemptItems.Remove(ItemId);
    }
    OnInventoryChanged.Broadcast();
    return true;
}

int32 ULowTideInventoryComponent::GetQuantity(FName ItemId) const
{
    return Quantities.FindRef(ItemId);
}

int32 ULowTideInventoryComponent::GetUsedCapacity() const
{
    int32 Total = 0;
    for (const TPair<FName, int32>& Pair : Quantities)
    {
        if (!CapacityExemptItems.Contains(Pair.Key))
        {
            Total += FMath::Max(0, Pair.Value);
        }
    }
    return Total;
}

void ULowTideInventoryComponent::AddCredits(int32 Amount)
{
    if (Amount > 0 && Credits <= MAX_int32 - Amount)
    {
        Credits += Amount;
        OnInventoryChanged.Broadcast();
    }
}
