#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LowTideHUD.generated.h"

class UFont;

UCLASS()
class LOWTIDE_API ALowTideHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    void DrawInventoryPanel(float X, float Y, float Width, bool bTrading);
    TArray<FString> WrapText(const FString& Text, UFont* Font, float Scale, float MaxWidth) const;
    float DrawWrappedText(const FString& Text, const FLinearColor& Color, float X, float Y,
        float MaxWidth, UFont* Font, float Scale, float LineHeight);
};
