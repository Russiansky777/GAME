#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LowTideHUD.generated.h"

UCLASS()
class LOWTIDE_API ALowTideHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    void DrawInventoryPanel(float X, float Y, float Width, bool bTrading);
};
