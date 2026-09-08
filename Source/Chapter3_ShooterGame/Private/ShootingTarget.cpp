#include "ShootingTarget.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"

namespace
{
    constexpr float kBasicMeshSize = 100.0f;
    constexpr float kBoardWidth = 110.0f;
    constexpr float kBoardThickness = 2.0f;
    constexpr float kRingThickness = 0.05f;
    constexpr float kRingSpacing = 0.1f;
    constexpr float kRingDiameters[] = {90.0f, 72.0f, 54.0f, 36.0f, 18.0f};
    constexpr float kHitMarkDiameter = 3.0f;
    constexpr float kHitMarkOutlineDiameter = 4.5f;
    constexpr float kHitMarkOutlineDepth = 0.1f;
    constexpr float kHitMarkThickness = 0.08f;
    constexpr float kHitMarkSurfaceOffset = 0.8f;
    constexpr int32 kMaxHitMarks = 100;
    constexpr float kTargetRoughness = 1.0f;
    const FLinearColor kPaperColor(0.85f, 0.85f, 0.8f);
    const FLinearColor kBlackColor(0.01f, 0.01f, 0.01f);
    const FLinearColor kBullseyeColor(0.8f, 0.025f, 0.015f);
    const FLinearColor kHitMarkColor(0.0f, 1.0f, 1.0f);
}

AShootingTarget::AShootingTarget()
{
    PrimaryActorTick.bCanEverTick = false;
    SetCanBeDamaged(true);

    USceneComponent* sceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TargetRoot"));
    SetRootComponent(sceneRoot);

    targetBoard_ = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetBoard"));
    targetBoard_->SetupAttachment(sceneRoot);
    // 루트 자체에는 스케일을 주지 않아 동심원과 명중 표시가 함께 찌그러지는 것을 막는다.
    targetBoard_->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    targetBoard_->SetCollisionResponseToAllChannels(ECR_Ignore);
    targetBoard_->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    targetBoard_->SetGenerateOverlapEvents(false);
    targetBoard_->SetCastShadow(false);

    targetBoard_->SetRelativeScale3D(FVector(kBoardThickness, kBoardWidth, kBoardWidth) / kBasicMeshSize);

    for (int32 ringIndex = 0; ringIndex < UE_ARRAY_COUNT(kRingDiameters); ++ringIndex)
    {
        const FName componentName(*FString::Printf(TEXT("TargetRing%d"), ringIndex));
        UStaticMeshComponent* ring = CreateDefaultSubobject<UStaticMeshComponent>(componentName);
        ring->SetupAttachment(sceneRoot);
        ring->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
        ring->SetRelativeLocation(FVector(-kBoardThickness * 0.5f - kRingSpacing * (ringIndex + 1), 0.0f, 0.0f));
        ring->SetRelativeScale3D(FVector(kRingDiameters[ringIndex], kRingDiameters[ringIndex], kRingThickness) / kBasicMeshSize);
        ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ring->SetGenerateOverlapEvents(false);
        ring->SetCastShadow(false);
        targetRings_.Add(ring);
    }

    hitMarks_ = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HitMarks"));
    hitMarks_->SetupAttachment(sceneRoot);
    hitMarks_->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    hitMarks_->SetGenerateOverlapEvents(false);
    hitMarks_->SetCastShadow(false);

    hitMarkOutlines_ = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HitMarkOutlines"));
    hitMarkOutlines_->SetupAttachment(sceneRoot);
    hitMarkOutlines_->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    hitMarkOutlines_->SetGenerateOverlapEvents(false);
    hitMarkOutlines_->SetCastShadow(false);
}

void AShootingTarget::OnConstruction(const FTransform& transform)
{
    Super::OnConstruction(transform);
    ApplyTargetAssets();
    UpdateTargetColors();
}

void AShootingTarget::BeginPlay()
{
    Super::BeginPlay();
    ApplyTargetAssets();
    UpdateTargetColors();
}

void AShootingTarget::ApplyTargetAssets()
{
    targetBoard_->SetStaticMesh(boardMeshAsset_);
    hitMarks_->SetStaticMesh(diskMeshAsset_);
    hitMarkOutlines_->SetStaticMesh(diskMeshAsset_);
    for (UStaticMeshComponent* ring : targetRings_)
    {
        ring->SetStaticMesh(diskMeshAsset_);
    }
}

void AShootingTarget::UpdateTargetColors()
{
    SetMeshColor(targetBoard_, kPaperColor);
    SetMeshColor(hitMarks_, kHitMarkColor);
    SetMeshColor(hitMarkOutlines_, kBlackColor);

    for (int32 ringIndex = 0; ringIndex < targetRings_.Num(); ++ringIndex)
    {
        const bool isBullseye = ringIndex == targetRings_.Num() - 1;
        const FLinearColor ringColor = isBullseye ? kBullseyeColor :
            (ringIndex % 2 == 0 ? kBlackColor : kPaperColor);
        SetMeshColor(targetRings_[ringIndex], ringColor);
    }
}

void AShootingTarget::SetMeshColor(UStaticMeshComponent* mesh, const FLinearColor& color)
{
    UMaterialInstanceDynamic* material = mesh->CreateDynamicMaterialInstance(0, targetMaterial_);
    if (material)
    {
        material->SetVectorParameterValue(TEXT("Color"), color);
        material->SetScalarParameterValue(TEXT("Roughness"), kTargetRoughness);
    }
}

float AShootingTarget::TakeDamage(float damageAmount, const FDamageEvent& damageEvent,
    AController* eventInstigator, AActor* damageCauser)
{
    const float appliedDamage = Super::TakeDamage(damageAmount, damageEvent, eventInstigator, damageCauser);
    if (appliedDamage > 0.0f && damageEvent.IsOfType(FPointDamageEvent::ClassID))
    {
        const FPointDamageEvent& pointDamage = static_cast<const FPointDamageEvent&>(damageEvent);
        if (pointDamage.HitInfo.GetComponent() == targetBoard_)
        {
            AddHitMark(pointDamage.HitInfo);
        }
    }
    return appliedDamage;
}

void AShootingTarget::AddHitMark(const FHitResult& hitResult)
{
    const float surfaceOffset = kHitMarkSurfaceOffset * GetActorScale3D().GetAbsMax();
    const FVector markLocation = hitResult.ImpactPoint + hitResult.ImpactNormal * surfaceOffset;
    const FQuat markRotation = FRotationMatrix::MakeFromZ(hitResult.ImpactNormal).ToQuat();
    const FVector markScale = FVector(kHitMarkDiameter, kHitMarkDiameter, kHitMarkThickness) / kBasicMeshSize;
    const FTransform markTransform(markRotation, markLocation, markScale);
    const FVector outlineLocation = markLocation - hitResult.ImpactNormal * kHitMarkOutlineDepth;
    const FVector outlineScale = FVector(kHitMarkOutlineDiameter, kHitMarkOutlineDiameter, kHitMarkThickness) / kBasicMeshSize;
    const FTransform outlineTransform(markRotation, outlineLocation, outlineScale);

    // 판정에는 참여하지 않는 인스턴스로 남겨, 이전 표시가 다음 탄을 가로막지 않게 한다.
    if (hitMarks_->GetInstanceCount() < kMaxHitMarks)
    {
        hitMarks_->AddInstance(markTransform, true);
        hitMarkOutlines_->AddInstance(outlineTransform, true);
    }
    else
    {
        hitMarks_->UpdateInstanceTransform(nextMarkIndex_, markTransform, true, true, true);
        hitMarkOutlines_->UpdateInstanceTransform(nextMarkIndex_, outlineTransform, true, true, true);
        nextMarkIndex_ = (nextMarkIndex_ + 1) % kMaxHitMarks;
    }
}

void AShootingTarget::ClearHitMarks()
{
    hitMarks_->ClearInstances();
    hitMarkOutlines_->ClearInstances();
    nextMarkIndex_ = 0;
}

int32 AShootingTarget::GetHitMarkCount() const
{
    return hitMarks_->GetInstanceCount();
}
