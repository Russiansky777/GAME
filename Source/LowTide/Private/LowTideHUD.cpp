#include "LowTideHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "LowTideCharacter.h"
#include "LowTideGameMode.h"
#include "LowTideInventoryComponent.h"
#include "TideController.h"

void ALowTideHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas)
    {
        return;
    }

    const ALowTideCharacter* Character = Cast<ALowTideCharacter>(GetOwningPawn());
    const ALowTideGameMode* GameMode = GetWorld()->GetAuthGameMode<ALowTideGameMode>();
    if (!Character || !GameMode)
    {
        return;
    }

    const float CenterX = Canvas->SizeX * 0.5f;
    const float CenterY = Canvas->SizeY * 0.5f;
    DrawLine(CenterX - 6.0f, CenterY, CenterX + 6.0f, CenterY, FLinearColor(0.9f, 0.95f, 1.0f, 0.9f), 1.5f);
    DrawLine(CenterX, CenterY - 6.0f, CenterX, CenterY + 6.0f, FLinearColor(0.9f, 0.95f, 1.0f, 0.9f), 1.5f);

    DrawRect(FLinearColor(0.02f, 0.06f, 0.08f, 0.82f), 24.0f, 20.0f, 490.0f, 98.0f);
    DrawText(TEXT("LOW TIDE"), FLinearColor(0.60f, 0.90f, 0.95f), 40.0f, 31.0f, GEngine->GetLargeFont(), 0.85f, false);
    DrawText(TEXT("Recover salvage during low tide. Return to Mara and sell it."), FLinearColor::White, 40.0f, 62.0f, GEngine->GetSmallFont(), 1.0f, false);

    if (const ATideController* Tide = GameMode->GetTideController())
    {
        const FString TideText = FString::Printf(TEXT("TIDE: %s   %02d s   |   ACCESS %s"), *Tide->GetPhaseName(),
            FMath::CeilToInt(Tide->GetSecondsRemaining()), Tide->IsAccessOpen() ? TEXT("OPEN") : TEXT("CLOSED"));
        const FLinearColor TideColor = Tide->IsClosingWarning() ? FLinearColor(1.0f, 0.35f, 0.18f) : FLinearColor(0.55f, 0.85f, 1.0f);
        DrawText(TideText, TideColor, 40.0f, 88.0f, GEngine->GetSmallFont(), 1.1f, false);
        if (Tide->IsClosingWarning())
        {
            DrawRect(FLinearColor(0.22f, 0.02f, 0.01f, 0.88f), CenterX - 240.0f, 124.0f, 480.0f, 36.0f);
            DrawText(TEXT("WARNING: LEAVE THE SHELF - THE TIDE IS CLOSING ACCESS"), FLinearColor(1.0f, 0.72f, 0.25f), CenterX - 220.0f, 133.0f, GEngine->GetSmallFont(), 1.0f, false);
        }
    }

    if (!GameMode->GetCatalogError().IsEmpty())
    {
        DrawRect(FLinearColor(0.35f, 0.01f, 0.01f, 0.96f), 24.0f, 126.0f, 720.0f, 48.0f);
        DrawText(FString::Printf(TEXT("CONTENT ERROR: %s"), *GameMode->GetCatalogError()), FLinearColor::White,
            38.0f, 140.0f, GEngine->GetSmallFont(), 0.95f, false);
    }

    const FString Prompt = Character->GetInteractionPrompt();
    if (!Prompt.IsEmpty())
    {
        DrawRect(FLinearColor(0.02f, 0.06f, 0.08f, 0.88f), CenterX - 150.0f, CenterY + 42.0f, 300.0f, 34.0f);
        DrawText(Prompt, FLinearColor::White, CenterX - 132.0f, CenterY + 50.0f, GEngine->GetSmallFont(), 1.05f, false);
    }

    const ULowTideInventoryComponent* Inventory = Character->GetInventory();
    DrawText(FString::Printf(TEXT("PACK %d/%d     CREDITS %d"), Inventory->GetUsedCapacity(), Inventory->GetCapacity(), Inventory->GetCredits()),
        FLinearColor::White, 32.0f, Canvas->SizeY - 54.0f, GEngine->GetSmallFont(), 1.05f, false);
    DrawText(TEXT("WASD move  |  Mouse look  |  E interact  |  I inventory  |  Esc quit"),
        FLinearColor(0.72f, 0.78f, 0.80f), 32.0f, Canvas->SizeY - 30.0f, GEngine->GetSmallFont(), 0.9f, false);

    if (Character->IsInventoryOpen() || Character->GetActiveTrader())
    {
        DrawInventoryPanel(CenterX - 300.0f, 150.0f, 600.0f, Character->GetActiveTrader() != nullptr);
    }

    if (!Character->GetFeedback().IsEmpty())
    {
        DrawRect(FLinearColor(0.02f, 0.06f, 0.08f, 0.92f), CenterX - 360.0f, Canvas->SizeY - 112.0f, 720.0f, 38.0f);
        DrawText(Character->GetFeedback(), FLinearColor(0.95f, 0.88f, 0.55f), CenterX - 340.0f, Canvas->SizeY - 102.0f, GEngine->GetSmallFont(), 1.0f, false);
    }
}

void ALowTideHUD::DrawInventoryPanel(float X, float Y, float Width, bool bTrading)
{
    const ALowTideCharacter* Character = Cast<ALowTideCharacter>(GetOwningPawn());
    const ALowTideGameMode* GameMode = GetWorld()->GetAuthGameMode<ALowTideGameMode>();
    if (!Character || !GameMode)
    {
        return;
    }

    DrawRect(FLinearColor(0.025f, 0.055f, 0.065f, 0.96f), X, Y, Width, 330.0f);
    DrawText(bTrading ? TEXT("MARA - SALVAGE BUYER") : TEXT("FIELD PACK"), FLinearColor(0.60f, 0.90f, 0.95f), X + 24.0f, Y + 20.0f, GEngine->GetLargeFont(), 0.8f, false);
    DrawText(bTrading ? TEXT("Press 1-5 to sell one item. E closes trade.") : TEXT("Evidence survives a forced return. I closes the pack."),
        FLinearColor(0.78f, 0.84f, 0.85f), X + 24.0f, Y + 55.0f, GEngine->GetSmallFont(), 0.95f, false);

    const TArray<FItemDefinition>& Items = GameMode->GetItemCatalog().GetOrderedItems();
    for (int32 Index = 0; Index < Items.Num(); ++Index)
    {
        const FItemDefinition& Item = Items[Index];
        const int32 Quantity = Character->GetInventory()->GetQuantity(Item.Id);
        const FString Price = Item.bSellable ? FString::Printf(TEXT("%d credits each"), Item.Value) : TEXT("EVIDENCE - NOT FOR SALE");
        const FString Prefix = bTrading ? FString::Printf(TEXT("[%d] "), Index + 1) : TEXT("");
        DrawText(FString::Printf(TEXT("%s%s   x%d   |   %s"), *Prefix, *Item.DisplayName, Quantity, *Price),
            Item.bSellable ? FLinearColor::White : FLinearColor(0.95f, 0.78f, 0.34f), X + 30.0f, Y + 92.0f + Index * 42.0f,
            GEngine->GetSmallFont(), 1.0f, false);
        DrawText(Item.Description, FLinearColor(0.58f, 0.66f, 0.68f), X + 50.0f, Y + 112.0f + Index * 42.0f,
            GEngine->GetSmallFont(), 0.8f, false);
    }
}
