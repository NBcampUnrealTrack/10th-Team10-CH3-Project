#include "M1911WeaponView.h"

#include "Components/PointLightComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"

namespace
{
    constexpr float kAimEyeDistance = 22.0f;
    constexpr float kAimTransitionTime = 0.18f;
    constexpr float kRecoilPitch = 4.0f;
    constexpr float kRecoilKickTime = 0.025f;
    constexpr float kRecoilReturnTime = 0.14f;
    constexpr float kRecoilDuration = kRecoilKickTime + kRecoilReturnTime;
    constexpr float kRecoilBackDistance = 1.4f;
    constexpr float kAimRecoilMultiplier = 0.55f;
    constexpr float kFlashDuration = 0.045f;
    constexpr float kFlashLightIntensity = 1200.0f;
    constexpr float kFlashLightRadius = 100.0f;
    constexpr float kReloadTiltStart = 0.1f;
    constexpr float kReloadTiltEnd = 0.3f;
    constexpr float kReloadReturnStart = 0.8f;
    constexpr float kMagazineOutStart = 0.3f;
    constexpr float kMagazineOutEnd = 0.45f;
    constexpr float kMagazineInStart = 0.65f;
    constexpr float kMagazineInEnd = 0.80f;
    constexpr float kMagazineTravelDistance = 60.0f;
    constexpr float kReloadForwardDistance = 12.0f;
    constexpr float kReloadLeftDistance = 8.0f;
    constexpr float kReloadUpDistance = 16.0f;
    constexpr float kReloadPitch = 18.0f;
    constexpr float kReloadRoll = -40.0f;
    const FName kMagazineBoneName(TEXT("Mag"));
}

AM1911WeaponView::AM1911WeaponView()
{
    PrimaryActorTick.bCanEverTick = false;
    SetReplicates(false);

    hipLocation_ = FVector(28.0f, 14.0f, -12.0f);
    hipRotation_ = FRotator(-4.0f, 172.0f, -6.0f);
    // 원본 NonRigged_M1911의 정점과 Rigged_M1911의 기준 형상을 대조한 값(cm).
    rearSightLocal_ = FVector(3.1136f, 0.0f, 6.4983f);
    frontSightLocal_ = FVector(-14.5810f, 0.0f, 6.4369f);
    aimEyeDistance_ = kAimEyeDistance;
    aimTransitionTime_ = kAimTransitionTime;
    recoilPitch_ = kRecoilPitch;
    shotElapsed_ = kRecoilDuration;

    viewRoot_ = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponViewRoot"));
    SetRootComponent(viewRoot_);

    gunMesh_ = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("M1911Mesh"));
    gunMesh_->SetupAttachment(viewRoot_);
    gunMesh_->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    gunMesh_->SetGenerateOverlapEvents(false);
    gunMesh_->SetOnlyOwnerSee(true);
    gunMesh_->SetCastShadow(false);
    gunMesh_->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
    gunMesh_->SetComponentTickEnabled(false);

    reloadMesh_ = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("ReloadMesh"));
    reloadMesh_->SetupAttachment(gunMesh_);
    reloadMesh_->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    reloadMesh_->SetGenerateOverlapEvents(false);
    reloadMesh_->SetOnlyOwnerSee(true);
    reloadMesh_->SetCastShadow(false);
    reloadMesh_->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
    reloadMesh_->SetComponentTickEnabled(false);
    reloadMesh_->SetVisibility(false);

    muzzleFlash_ = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MuzzleFlash"));
    muzzleFlash_->SetupAttachment(viewRoot_);
    muzzleFlash_->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    muzzleFlash_->SetGenerateOverlapEvents(false);
    muzzleFlash_->SetOnlyOwnerSee(true);
    muzzleFlash_->SetCastShadow(false);
    muzzleFlash_->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
    muzzleFlash_->SetRelativeLocation(FVector(-17.0f, 0.0f, 4.2f));
    muzzleFlash_->SetRelativeScale3D(FVector(0.06f, 0.012f, 0.012f));
    muzzleFlash_->SetVisibility(false);

    muzzleLight_ = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLight"));
    muzzleLight_->SetupAttachment(viewRoot_);
    muzzleLight_->SetRelativeLocation(FVector(-17.0f, 0.0f, 4.2f));
    muzzleLight_->SetLightColor(FLinearColor(1.0f, 0.45f, 0.08f));
    muzzleLight_->SetIntensity(kFlashLightIntensity);
    muzzleLight_->SetAttenuationRadius(kFlashLightRadius);
    muzzleLight_->SetCastShadows(false);
    muzzleLight_->SetVisibility(false);
}

void AM1911WeaponView::OnConstruction(const FTransform& transform)
{
    Super::OnConstruction(transform);
    ApplyVisualAssets();
}

void AM1911WeaponView::ApplyVisualAssets()
{
    gunMesh_->SetSkeletalMeshAsset(gunMeshAsset_);
    reloadMesh_->SetSkinnedAssetAndUpdate(gunMeshAsset_);
    muzzleFlash_->SetStaticMesh(muzzleFlashMeshAsset_);
    muzzleFlash_->SetMaterial(0, muzzleFlashMaterial_);
}

void AM1911WeaponView::BeginPlay()
{
    Super::BeginPlay();
    ApplyVisualAssets();

    UMaterialInstanceDynamic* flashMaterial = muzzleFlash_->CreateDynamicMaterialInstance(0);
    if (flashMaterial)
    {
        flashMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(20.0f, 8.0f, 1.0f));
    }
}

FTransform AM1911WeaponView::GetAimTransform() const
{
    const FVector sightDirection = (frontSightLocal_ - rearSightLocal_).GetSafeNormal();
    const FQuat aimRotation = FRotationMatrix::MakeFromXZ(sightDirection, FVector::UpVector).ToQuat().Inverse();
    const FVector eyeToRearSight(aimEyeDistance_, 0.0f, 0.0f);
    const FVector aimLocation = eyeToRearSight - aimRotation.RotateVector(rearSightLocal_);
    return FTransform(aimRotation, aimLocation);
}

void AM1911WeaponView::UpdateView(float deltaTime, FVector cameraLocation, FRotator cameraRotation, bool aimHeld)
{
    const float safeDeltaTime = FMath::Max(0.0f, deltaTime);
    const float targetAlpha = aimHeld ? 1.0f : 0.0f;
    aimAlpha_ = FMath::FInterpConstantTo(aimAlpha_, targetAlpha, safeDeltaTime,
        1.0f / FMath::Max(aimTransitionTime_, UE_SMALL_NUMBER));
    const float poseAlpha = FMath::SmoothStep(0.0f, 1.0f, aimAlpha_);

    UpdateFireFeedback(safeDeltaTime);

    const FTransform aimTransform = GetAimTransform();
    FVector viewLocation = FMath::Lerp(hipLocation_, aimTransform.GetLocation(), poseAlpha);
    FQuat viewRotation = FQuat::Slerp(hipRotation_.Quaternion(), aimTransform.GetRotation(), poseAlpha);

    const float recoilAmount = recoilWeight_ * FMath::Lerp(1.0f, kAimRecoilMultiplier, poseAlpha);
    viewLocation.X -= recoilAmount * kRecoilBackDistance;
    viewRotation = FRotator(recoilPitch_ * recoilAmount, 0.0f, 0.0f).Quaternion() * viewRotation;

    ApplyReloadPose(viewLocation, viewRotation);

    const FTransform cameraTransform(cameraRotation, cameraLocation);
    const FTransform relativeTransform(viewRotation, viewLocation);
    SetActorTransform(relativeTransform * cameraTransform);
}

void AM1911WeaponView::SetReloadState(bool isReloading, float progress)
{
    if (isReloading && !isReloading_)
    {
        reloadMesh_->CopyPoseFromSkeletalComponent(gunMesh_);
        magazineRestTransform_ = reloadMesh_->GetBoneTransformByName(kMagazineBoneName, EBoneSpaces::ComponentSpace);
        for (int32 materialIndex = 0; materialIndex < gunMesh_->GetNumMaterials(); ++materialIndex)
        {
            reloadMesh_->SetMaterial(materialIndex, gunMesh_->GetMaterial(materialIndex));
        }
    }

    isReloading_ = isReloading;
    reloadProgress_ = FMath::Clamp(progress, 0.0f, 1.0f);
    gunMesh_->SetVisibility(!isReloading_);
    reloadMesh_->SetVisibility(isReloading_);
}

void AM1911WeaponView::ApplyReloadPose(FVector& viewLocation, FQuat& viewRotation)
{
    if (!isReloading_)
    {
        return;
    }

    const float tiltWeight = FMath::SmoothStep(kReloadTiltStart, kReloadTiltEnd, reloadProgress_)
        * (1.0f - FMath::SmoothStep(kReloadReturnStart, 1.0f, reloadProgress_));
    viewLocation += FVector(kReloadForwardDistance, -kReloadLeftDistance, kReloadUpDistance) * tiltWeight;
    viewRotation = FRotator(kReloadPitch * tiltWeight, 0.0f, kReloadRoll * tiltWeight).Quaternion() * viewRotation;

    const float magazineWeight = FMath::SmoothStep(kMagazineOutStart, kMagazineOutEnd, reloadProgress_)
        * (1.0f - FMath::SmoothStep(kMagazineInStart, kMagazineInEnd, reloadProgress_));
    FTransform magazineTransform = magazineRestTransform_;
    magazineTransform.AddToTranslation(FVector::DownVector * kMagazineTravelDistance * magazineWeight);
    reloadMesh_->SetBoneTransformByName(kMagazineBoneName, magazineTransform, EBoneSpaces::ComponentSpace);
    reloadMesh_->RefreshBoneTransforms();
}

void AM1911WeaponView::PlayFireFeedback()
{
    shotElapsed_ = 0.0f;
    muzzleFlash_->SetVisibility(true);
    muzzleLight_->SetVisibility(true);
}

void AM1911WeaponView::UpdateFireFeedback(float deltaTime)
{
    shotElapsed_ = FMath::Min(shotElapsed_ + deltaTime, kRecoilDuration);
    if (shotElapsed_ < kRecoilKickTime)
    {
        recoilWeight_ = shotElapsed_ / kRecoilKickTime;
    }
    else
    {
        const float returnAlpha = (shotElapsed_ - kRecoilKickTime) / kRecoilReturnTime;
        recoilWeight_ = 1.0f - FMath::SmoothStep(0.0f, 1.0f, returnAlpha);
    }

    const bool flashVisible = shotElapsed_ < kFlashDuration;
    muzzleFlash_->SetVisibility(flashVisible);
    muzzleLight_->SetVisibility(flashVisible);
}

FVector AM1911WeaponView::GetRearSightWorldLocation() const
{
    return GetActorTransform().TransformPosition(rearSightLocal_);
}

FVector AM1911WeaponView::GetFrontSightWorldLocation() const
{
    return GetActorTransform().TransformPosition(frontSightLocal_);
}
