#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "LowTideInventoryComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLowTideInventoryBoundariesTest,
    "LowTide.M05.Inventory.CapacityAndQuantityBoundaries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLowTideInventoryBoundariesTest::RunTest(const FString& Parameters)
{
    ULowTideInventoryComponent* Inventory = NewObject<ULowTideInventoryComponent>();
    FString Reason;

    TestFalse(TEXT("Zero quantity is rejected"), Inventory->TryAdd(TEXT("scrap_metal"), 0, Reason));
    TestFalse(TEXT("None id is rejected"), Inventory->TryAdd(NAME_None, 1, Reason));
    TestTrue(TEXT("Capacity-sized transfer succeeds"), Inventory->TryAdd(TEXT("scrap_metal"), Inventory->GetCapacity(), Reason));
    TestEqual(TEXT("Capacity reports exact quantity"), Inventory->GetUsedCapacity(), Inventory->GetCapacity());
    TestFalse(TEXT("Full inventory rejects another item"), Inventory->TryAdd(TEXT("sea_glass"), 1, Reason));
    TestEqual(TEXT("Rejected item was not added"), Inventory->GetQuantity(TEXT("sea_glass")), 0);
    TestFalse(TEXT("Cannot remove more than owned"), Inventory->TryRemove(TEXT("scrap_metal"), Inventory->GetCapacity() + 1));
    TestTrue(TEXT("Exact removal succeeds"), Inventory->TryRemove(TEXT("scrap_metal"), Inventory->GetCapacity()));
    TestEqual(TEXT("Exact removal empties inventory"), Inventory->GetUsedCapacity(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLowTideAtomicSaleTest,
    "LowTide.M05.Inventory.AtomicIndividualSale",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLowTideAtomicSaleTest::RunTest(const FString& Parameters)
{
    ULowTideInventoryComponent* Inventory = NewObject<ULowTideInventoryComponent>();
    FString Reason;
    TestTrue(TEXT("Fixture item added"), Inventory->TryAdd(TEXT("copper_wire"), 2, Reason));
    TestFalse(TEXT("Sale rejects quantity beyond stock"), Inventory->TrySell(TEXT("copper_wire"), 3, 18));
    TestEqual(TEXT("Rejected sale preserves stock"), Inventory->GetQuantity(TEXT("copper_wire")), 2);
    TestEqual(TEXT("Rejected sale preserves credits"), Inventory->GetCredits(), 0);
    TestFalse(TEXT("Sale rejects negative credit value"), Inventory->TrySell(TEXT("copper_wire"), 1, -1));
    TestTrue(TEXT("One-item sale succeeds"), Inventory->TrySell(TEXT("copper_wire"), 1, 18));
    TestEqual(TEXT("Sale removes exactly one"), Inventory->GetQuantity(TEXT("copper_wire")), 1);
    TestEqual(TEXT("Sale credits exact price"), Inventory->GetCredits(), 18);
    return true;
}

#endif
