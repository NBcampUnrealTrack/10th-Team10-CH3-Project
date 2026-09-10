#include "TrainingDummyTarget.h"
#include "AssassinationTargetComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
    constexpr float kDefaultDummyHeight = 200.0f;
}

ATrainingDummyTarget::ATrainingDummyTarget()
{
    showPaperTarget_ = false;
    dummyHeight_ = kDefaultDummyHeight;
    dummyMesh_ = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DummyMesh"));
    dummyMesh_->SetupAttachment(GetRootComponent());
    dummyMesh_->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    dummyMesh_->SetCollisionResponseToAllChannels(ECR_Ignore);
    dummyMesh_->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    dummyMesh_->SetGenerateOverlapEvents(false);
    assassinationTarget_ = CreateDefaultSubobject<UAssassinationTargetComponent>(TEXT("AssassinationTarget"));
    assassinationTarget_->SetupAttachment(GetRootComponent());
}

void ATrainingDummyTarget::OnConstruction(const FTransform& transform)
{
    Super::OnConstruction(transform);
    FitDummyMesh();
}

void ATrainingDummyTarget::BeginPlay()
{
    Super::BeginPlay();
    FitDummyMesh();
}

void ATrainingDummyTarget::FitDummyMesh()
{
    const UStaticMesh* mesh = dummyMesh_->GetStaticMesh();
    if (!mesh)
    {
        return;
    }

    const FBoxSphereBounds bounds = mesh->GetBounds();
    const float sourceHeight = bounds.BoxExtent.Z * 2.0f;
    if (sourceHeight <= UE_SMALL_NUMBER)
    {
        return;
    }

    const float meshScale = FMath::Max(1.0f, dummyHeight_) / sourceHeight;
    dummyMesh_->SetRelativeScale3D(FVector(meshScale));
    dummyMesh_->SetRelativeLocation(FVector(-bounds.Origin.X, -bounds.Origin.Y,
        bounds.BoxExtent.Z - bounds.Origin.Z) * meshScale);
    constexpr float kBodyCenterRatio = 0.5f;
    assassinationTarget_->SetRelativeLocation(FVector(0.0f, 0.0f,
        FMath::Max(1.0f, dummyHeight_) * kBodyCenterRatio));
}

bool ATrainingDummyTarget::IsTargetComponent(const UPrimitiveComponent* component) const
{
    return component == dummyMesh_;
}
