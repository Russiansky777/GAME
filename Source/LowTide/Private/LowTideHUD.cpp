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
    const bool bMenuOpen = Character->IsInventoryOpen() || Character->GetActiveTrader();
    float HeaderBottom = 20.0f;
    if (!bMenuOpen)
    {
        DrawLine(CenterX - 6.0f, CenterY, CenterX + 6.0f, CenterY, FLinearColor(0.9f, 0.95f, 1.0f, 0.9f), 1.5f);
        DrawLine(CenterX, CenterY - 6.0f, CenterX, CenterY + 6.0f, FLinearColor(0.9f, 0.95f, 1.0f, 0.9f), 1.5f);
    }

    if (!bMenuOpen)
    {
        const float HeaderWidth = FMath::Min(660.0f, Canvas->SizeX - 48.0f);
        const FString Objective = GameMode->IsM05Fixture() ? TEXT("Recover salvage during low tide. Return to Mara and sell it.") : GameMode->GetObjectiveText();
        const TArray<FString> ObjectiveLines = WrapText(Objective, GEngine->GetSmallFont(), 1.0f, HeaderWidth - 32.0f);
        const float TideY = 66.0f + ObjectiveLines.Num() * 18.0f;
        const float HeaderHeight = TideY + (GameMode->IsM05Fixture() ? 28.0f : 30.0f) - 20.0f;
        HeaderBottom += HeaderHeight;
        DrawRect(FLinearColor(0.02f, 0.06f, 0.08f, 0.82f), 24.0f, 20.0f, HeaderWidth, HeaderHeight);
        DrawText(TEXT("LOW TIDE"), FLinearColor(0.60f, 0.90f, 0.95f), 40.0f, 31.0f, GEngine->GetLargeFont(), 0.85f, false);
        DrawWrappedText(Objective, FLinearColor::White, 40.0f, 62.0f, HeaderWidth - 32.0f, GEngine->GetSmallFont(), 1.0f, 18.0f);

        if (const ATideController* Tide = GameMode->GetTideController())
        {
            FString TideText;
            if (GameMode->IsM05Fixture())
            {
                TideText = FString::Printf(TEXT("TIDE: %s   %s   |   ACCESS %s"), *Tide->GetPhaseName(),
                    Tide->IsClockRunning() ? *FString::Printf(TEXT("%02d s"), FMath::CeilToInt(Tide->GetSecondsRemaining())) : TEXT("WAITING FOR MISSION"),
                    Tide->IsAccessOpen() ? TEXT("OPEN") : TEXT("CLOSED"));
            }
            else
            {
                switch (Tide->GetPhase())
                {
                case ETidePhase::Low: TideText = TEXT("TIDE: LOW — EXPLORE THE EXPOSED COVE"); break;
                case ETidePhase::Rising: TideText = Tide->IsAccessOpen()
                    ? TEXT("TIDE: RISING — LOWER ROUTE IS CHANGING")
                    : TEXT("TIDE: RISING — LOWER ROUTE CLOSED / USE BLUE RIDGE"); break;
                case ETidePhase::High: TideText = TEXT("TIDE: HIGH — BLUE RIDGE IS THE ESCAPE"); break;
                default: TideText = TEXT("TIDE: FALLING — ROUTES WILL REOPEN"); break;
                }
            }
            const FLinearColor TideColor = Tide->IsClosingWarning() ? FLinearColor(1.0f, 0.35f, 0.18f) : FLinearColor(0.55f, 0.85f, 1.0f);
            DrawWrappedText(TideText, TideColor, 40.0f, TideY, HeaderWidth - 32.0f, GEngine->GetSmallFont(), 1.05f, 18.0f);
        }
    }

    if (const ATideController* Tide = GameMode->GetTideController(); Tide && Tide->IsClosingWarning())
    {
        const float WarningY = HeaderBottom + 8.0f;
        DrawRect(FLinearColor(0.22f, 0.02f, 0.01f, 0.88f), CenterX - 330.0f, WarningY, 660.0f, 36.0f);
        const int32 SecondsUntilClosure = FMath::CeilToInt(Tide->GetSecondsUntilAccessCloses());
        const FString WarningText = GameMode->IsM05Fixture()
            ? FString::Printf(TEXT("WARNING: RETURN TO SHORE - ACCESS CLOSES IN %d s"), SecondsUntilClosure)
            : FString::Printf(TEXT("LOWER SHORTCUT FLOODS IN %d s - BLUE RIDGE STAYS OPEN"), SecondsUntilClosure);
        DrawText(WarningText, FLinearColor(1.0f, 0.72f, 0.25f),
            CenterX - 310.0f, WarningY + 9.0f, GEngine->GetSmallFont(), 1.0f, false);
    }

    if (!GameMode->GetCatalogError().IsEmpty())
    {
        DrawRect(FLinearColor(0.35f, 0.01f, 0.01f, 0.96f), 24.0f, 126.0f, 720.0f, 48.0f);
        DrawText(FString::Printf(TEXT("CONTENT ERROR: %s"), *GameMode->GetCatalogError()), FLinearColor::White,
            38.0f, 140.0f, GEngine->GetSmallFont(), 0.95f, false);
    }

    const FString Prompt = bMenuOpen ? FString() : Character->GetInteractionPrompt();
    if (!Prompt.IsEmpty())
    {
        const float PromptWidth = FMath::Min(720.0f, Canvas->SizeX - 40.0f);
        const int32 PromptLines = WrapText(Prompt, GEngine->GetSmallFont(), 1.0f, PromptWidth - 36.0f).Num();
        const float PromptHeight = 18.0f + PromptLines * 19.0f;
        DrawRect(FLinearColor(0.02f, 0.06f, 0.08f, 0.88f), CenterX - PromptWidth * 0.5f, CenterY + 42.0f, PromptWidth, PromptHeight);
        DrawWrappedText(Prompt, FLinearColor::White, CenterX - PromptWidth * 0.5f + 18.0f, CenterY + 50.0f,
            PromptWidth - 36.0f, GEngine->GetSmallFont(), 1.0f, 19.0f);
    }

    const ULowTideInventoryComponent* Inventory = Character->GetInventory();
    DrawText(FString::Printf(TEXT("PACK %d/%d     CREDITS %d"), Inventory->GetUsedCapacity(), Inventory->GetCapacity(), Inventory->GetCredits()),
        FLinearColor::White, 32.0f, Canvas->SizeY - 54.0f, GEngine->GetSmallFont(), 1.05f, false);
    DrawText(TEXT("WASD move  |  Shift sprint  |  Space jump  |  E interact  |  I inventory  |  Esc quit"),
        FLinearColor(0.72f, 0.78f, 0.80f), 32.0f, Canvas->SizeY - 30.0f, GEngine->GetSmallFont(), 0.9f, false);

    if (bMenuOpen)
    {
        const float PanelHeight = 95.0f + GameMode->GetItemCatalog().GetOrderedItems().Num() * 42.0f;
        const float PanelY = FMath::Max(72.0f, FMath::Min(150.0f, Canvas->SizeY - PanelHeight - 128.0f));
        DrawInventoryPanel(CenterX - 300.0f, PanelY, 600.0f, Character->GetActiveTrader() != nullptr);
    }

    if (!Character->GetFeedback().IsEmpty())
    {
        const float FeedbackWidth = FMath::Min(720.0f, Canvas->SizeX - 40.0f);
        const int32 FeedbackLines = WrapText(Character->GetFeedback(), GEngine->GetSmallFont(), 0.95f, FeedbackWidth - 36.0f).Num();
        const float FeedbackHeight = 18.0f + FeedbackLines * 18.0f;
        const float FeedbackY = Canvas->SizeY - 74.0f - FeedbackHeight;
        DrawRect(FLinearColor(0.02f, 0.06f, 0.08f, 0.92f), CenterX - FeedbackWidth * 0.5f, FeedbackY, FeedbackWidth, FeedbackHeight);
        DrawWrappedText(Character->GetFeedback(), FLinearColor(0.95f, 0.88f, 0.55f), CenterX - FeedbackWidth * 0.5f + 18.0f,
            FeedbackY + 8.0f, FeedbackWidth - 36.0f, GEngine->GetSmallFont(), 0.95f, 18.0f);
    }
}

TArray<FString> ALowTideHUD::WrapText(const FString& Text, UFont* Font, float Scale, float MaxWidth) const
{
    TArray<FString> Words;
    Text.ParseIntoArrayWS(Words);
    TArray<FString> Lines;
    FString CurrentLine;
    for (const FString& Word : Words)
    {
        const FString Candidate = CurrentLine.IsEmpty() ? Word : CurrentLine + TEXT(" ") + Word;
        float Width = 0.0f;
        float Height = 0.0f;
        Canvas->StrLen(Font, Candidate, Width, Height);
        if (!CurrentLine.IsEmpty() && Width * Scale > MaxWidth)
        {
            Lines.Add(CurrentLine);
            CurrentLine = Word;
        }
        else
        {
            CurrentLine = Candidate;
        }
    }
    if (!CurrentLine.IsEmpty())
    {
        Lines.Add(CurrentLine);
    }
    return Lines;
}

float ALowTideHUD::DrawWrappedText(const FString& Text, const FLinearColor& Color, float X, float Y,
    float MaxWidth, UFont* Font, float Scale, float LineHeight)
{
    const TArray<FString> Lines = WrapText(Text, Font, Scale, MaxWidth);
    for (int32 Index = 0; Index < Lines.Num(); ++Index)
    {
        DrawText(Lines[Index], Color, X, Y + Index * LineHeight, Font, Scale, false);
    }
    return Lines.Num() * LineHeight;
}

void ALowTideHUD::DrawInventoryPanel(float X, float Y, float Width, bool bTrading)
{
    const ALowTideCharacter* Character = Cast<ALowTideCharacter>(GetOwningPawn());
    const ALowTideGameMode* GameMode = GetWorld()->GetAuthGameMode<ALowTideGameMode>();
    if (!Character || !GameMode)
    {
        return;
    }

    const TArray<FItemDefinition>& Items = GameMode->GetItemCatalog().GetOrderedItems();
    DrawRect(FLinearColor(0.025f, 0.055f, 0.065f, 0.96f), X, Y, Width, 95.0f + Items.Num() * 42.0f);
    DrawText(bTrading ? TEXT("MARA - SALVAGE BUYER") : TEXT("FIELD PACK"), FLinearColor(0.60f, 0.90f, 0.95f), X + 24.0f, Y + 20.0f, GEngine->GetLargeFont(), 0.8f, false);
    DrawWrappedText(bTrading ? TEXT("Press 1-8 to sell one item. E closes trade.")
        : TEXT("Evidence is protected and not for sale. I closes."),
        FLinearColor(0.78f, 0.84f, 0.85f), X + 24.0f, Y + 55.0f, Width - 48.0f,
        GEngine->GetSmallFont(), 0.82f, 16.0f);

    for (int32 Index = 0; Index < Items.Num(); ++Index)
    {
        const FItemDefinition& Item = Items[Index];
        const int32 Quantity = Character->GetInventory()->GetQuantity(Item.Id);
        const FString Price = Item.bSellable ? FString::Printf(TEXT("%d credits each"), Item.Value) : TEXT("PROTECTED EVIDENCE - NOT FOR SALE");
        const FString Prefix = bTrading ? FString::Printf(TEXT("[%d] "), Index + 1) : TEXT("");
        DrawText(FString::Printf(TEXT("%s%s   x%d   |   %s"), *Prefix, *Item.DisplayName, Quantity, *Price),
            Item.bSellable ? FLinearColor::White : FLinearColor(0.95f, 0.78f, 0.34f), X + 30.0f, Y + 92.0f + Index * 42.0f,
            GEngine->GetSmallFont(), 1.0f, false);
        const FString Description = Item.Id == TEXT("singing_shard") ? TEXT("A rare resonant find.") : Item.Description;
        DrawWrappedText(Description, FLinearColor(0.58f, 0.66f, 0.68f), X + 50.0f,
            Y + 112.0f + Index * 42.0f, Width - 80.0f, GEngine->GetSmallFont(), 0.68f, 13.0f);
    }
}
