#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "HubLivelinessActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLowTideHubLivelinessContractTest,
    "LowTide.M1.Hub.LivelinessContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLowTideHubLivelinessContractTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    World->AddToRoot();
    AHubLivelinessActor* Actor = World->SpawnActor<AHubLivelinessActor>();
    TestNotNull(TEXT("Hub liveliness actor constructs"), Actor);
    if (!Actor)
    {
        World->DestroyWorld(false);
        World->RemoveFromRoot();
        return false;
    }

    TestTrue(TEXT("Actor carries the scene composition tag"), Actor->ActorHasTag(TEXT("HubLiveliness")));
    TestFalse(TEXT("Merged source parrot has no per-frame animation cost"), Actor->PrimaryActorTick.bCanEverTick);

    TArray<UStaticMeshComponent*> Meshes;
    Actor->GetComponents<UStaticMeshComponent>(Meshes);
    TestEqual(TEXT("Full bird and floor stand remain one intact authored mesh"), Meshes.Num(), 1);
    for (UStaticMeshComponent* Mesh : Meshes)
    {
        TestTrue(FString::Printf(TEXT("%s is decorative and non-colliding"), *Mesh->GetName()),
            Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
        TestNotNull(FString::Printf(TEXT("%s has its authored mesh binding"), *Mesh->GetName()), Mesh->GetStaticMesh().Get());
    }
    UStaticMeshComponent* Parrot = Meshes.Num() == 1 ? Meshes[0] : nullptr;
    TestNotNull(TEXT("Merged parrot component remains available"), Parrot);
    if (Parrot)
    {
        const FVector Size = Parrot->GetStaticMesh()->GetBoundingBox().GetSize();
        TestTrue(TEXT("Merged parrot includes a readable elevated bird silhouette"), Size.Z >= 90.0f);
        TestTrue(TEXT("Merged parrot includes a compact floor stand"), Size.X <= 80.0f && Size.Y <= 80.0f);
    }
    World->DestroyWorld(false);
    World->RemoveFromRoot();
    return true;
}

#endif
