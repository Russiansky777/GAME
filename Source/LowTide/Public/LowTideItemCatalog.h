#pragma once

#include "CoreMinimal.h"
#include "LowTideTypes.h"

class LOWTIDE_API FLowTideItemCatalog
{
public:
    bool Load(FString& OutError);
    const FItemDefinition* Find(FName ItemId) const;
    const TArray<FItemDefinition>& GetOrderedItems() const { return OrderedItems; }

private:
    TArray<FItemDefinition> OrderedItems;
    TMap<FName, int32> IndicesById;
};
