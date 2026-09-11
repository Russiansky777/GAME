#include "LowTideItemCatalog.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool FLowTideItemCatalog::Load(FString& OutError)
{
    OrderedItems.Reset();
    IndicesById.Reset();

    const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Items.json"));
    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *Path))
    {
        OutError = FString::Printf(TEXT("Could not read item catalog: %s"), *Path);
        return false;
    }

    TArray<TSharedPtr<FJsonValue>> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root))
    {
        OutError = TEXT("Item catalog JSON root must be an array.");
        return false;
    }

    for (const TSharedPtr<FJsonValue>& Value : Root)
    {
        const TSharedPtr<FJsonObject> Object = Value.IsValid() ? Value->AsObject() : nullptr;
        if (!Object.IsValid())
        {
            OutError = TEXT("Item catalog contains a non-object entry.");
            return false;
        }

        FString IdText;
        FString DisplayName;
        FString Description;
        double Price = 0.0;
        bool bSellable = false;
        if (!Object->TryGetStringField(TEXT("id"), IdText)
            || !Object->TryGetStringField(TEXT("name"), DisplayName)
            || !Object->TryGetStringField(TEXT("description"), Description)
            || !Object->TryGetNumberField(TEXT("value"), Price)
            || !Object->TryGetBoolField(TEXT("sellable"), bSellable))
        {
            OutError = TEXT("Every item requires id, name, description, value and sellable fields.");
            return false;
        }

        const FName Id(*IdText);
        if (Id.IsNone() || IndicesById.Contains(Id) || !FMath::IsFinite(Price)
            || Price < 0.0 || Price > static_cast<double>(MAX_int32) || FMath::FloorToDouble(Price) != Price)
        {
            OutError = FString::Printf(TEXT("Invalid or duplicate item id/value: %s"), *IdText);
            return false;
        }

        FItemDefinition& Definition = OrderedItems.AddDefaulted_GetRef();
        Definition.Id = Id;
        Definition.DisplayName = MoveTemp(DisplayName);
        Definition.Description = MoveTemp(Description);
        Definition.Value = FMath::RoundToInt(Price);
        Definition.bSellable = bSellable;
        IndicesById.Add(Id, OrderedItems.Num() - 1);
    }

    if (OrderedItems.IsEmpty())
    {
        OutError = TEXT("Item catalog cannot be empty.");
        OrderedItems.Reset();
        IndicesById.Reset();
        return false;
    }

    return true;
}

const FItemDefinition* FLowTideItemCatalog::Find(FName ItemId) const
{
    const int32* Index = IndicesById.Find(ItemId);
    return Index ? &OrderedItems[*Index] : nullptr;
}
