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
    TestTrue(TEXT("Near animation never exceeds 30 Hz"), Actor->PrimaryActorTick.TickInterval >= (1.0f / 30.0f));

    TArray<UStaticMeshComponent*> Meshes;
    Actor->GetComponents<UStaticMeshComponent>(Meshes);
    TestEqual(TEXT("Bird, perch and optional lantern use eight bounded mesh components"), Meshes.Num(), 8);

    UStaticMeshComponent* Body = nullptr;
    UStaticMeshComponent* Head = nullptr;
    UStaticMeshComponent* LeftWing = nullptr;
    UStaticMeshComponent* RightWing = nullptr;
    for (UStaticMeshComponent* Mesh : Meshes)
    {
        TestTrue(FString::Printf(TEXT("%s is decorative and non-colliding"), *Mesh->GetName()),
            Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
        TestNotNull(FString::Printf(TEXT("%s has its authored mesh binding"), *Mesh->GetName()), Mesh->GetStaticMesh().Get());
        if (Mesh->GetFName() == TEXT("ParrotHead"))
        {
            Head = Mesh;
        }
        else if (Mesh->GetFName() == TEXT("ParrotBody"))
        {
            Body = Mesh;
        }
        else if (Mesh->GetFName() == TEXT("ParrotWingLeft"))
        {
            LeftWing = Mesh;
        }
        else if (Mesh->GetFName() == TEXT("ParrotWingRight"))
        {
            RightWing = Mesh;
        }
    }

    TestNotNull(TEXT("Parrot body provides the assembly envelope"), Body);
    TestNotNull(TEXT("Parrot head is a separately pivoted animated part"), Head);
    TestNotNull(TEXT("Parrot has a separately pivoted left wing"), LeftWing);
    TestNotNull(TEXT("Parrot has a separately pivoted right wing"), RightWing);
    if (Body && Head && LeftWing && RightWing)
    {
        const FBox BodyBox = Body->GetStaticMesh()->GetBoundingBox().TransformBy(Body->GetRelativeTransform());
        const FBox HeadBox = Head->GetStaticMesh()->GetBoundingBox().TransformBy(Head->GetRelativeTransform());
        const FBox LeftWingBox = LeftWing->GetStaticMesh()->GetBoundingBox().TransformBy(LeftWing->GetRelativeTransform());
        const FBox RightWingBox = RightWing->GetStaticMesh()->GetBoundingBox().TransformBy(RightWing->GetRelativeTransform());
        TestTrue(TEXT("Head overlaps the body at its neck rather than floating"), BodyBox.Intersect(HeadBox));
        TestTrue(TEXT("Left wing overlaps the body rather than floating"), BodyBox.Intersect(LeftWingBox));
        TestTrue(TEXT("Right wing overlaps the body rather than floating"), BodyBox.Intersect(RightWingBox));
    }
    if (Head)
    {
        Actor->ApplyIdlePoseForTesting(0.0f);
        const FRotator FirstPose = Head->GetRelativeRotation();
        Actor->ApplyIdlePoseForTesting(2.25f);
        TestFalse(TEXT("Deterministic idle loop changes the visible head transform"),
            Head->GetRelativeRotation().Equals(FirstPose, 0.01f));
    }
    World->DestroyWorld(false);
    World->RemoveFromRoot();
    return true;
}

#endif
