#include "HubLivelinessActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
void ConfigureDecorativeMesh(UStaticMeshComponent* Mesh)
{
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetGenerateOverlapEvents(false);
    Mesh->SetCastShadow(true);
    Mesh->SetCullDistance(6000.0f);
}
}

AHubLivelinessActor::AHubLivelinessActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Parrot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshyParrot"));
    Parrot->SetupAttachment(SceneRoot);
    // Interchange reflects the source facing across Y; preserve the composed actor placement.
    Parrot->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    ConfigureDecorativeMesh(Parrot);
    // The full bird and its floor-standing perch are deliberately retained as one source mesh.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ParrotAsset(
        TEXT("/Game/Generated/MeshyHub/Papug2/SM_Meshy_Papug2_import/StaticMeshes/SM_Meshy_Papug2_import.SM_Meshy_Papug2_import"));
    if (ParrotAsset.Succeeded()) Parrot->SetStaticMesh(ParrotAsset.Object);

    Tags.Add(TEXT("HubLiveliness"));
}
