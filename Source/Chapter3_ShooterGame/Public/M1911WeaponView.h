#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "M1911WeaponView.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class UPoseableMeshComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class USkeletalMesh;
class UStaticMesh;
class UMaterialInterface;
class UWeaponAttachmentComponent;

// 카메라 앞의 총 표시와 조준/반동/장전 모션을 담당한다. 명중 판정과 탄약은 컨트롤러가 담당한다.
UCLASS()
class CHAPTER3_SHOOTERGAME_API AM1911WeaponView : public AActor
{
    GENERATED_BODY()

public:
    AM1911WeaponView();
    virtual void OnConstruction(const FTransform& transform) override;

    UFUNCTION(BlueprintCallable, Category = "Weapon View")
    void UpdateView(float deltaTime, FVector cameraLocation, FRotator cameraRotation, bool aimHeld);

    UFUNCTION(BlueprintCallable, Category = "Weapon View")
    void PlayFireFeedback();

    void SetReloadState(bool isReloading, float progress);

    UFUNCTION(BlueprintPure, Category = "Weapon View")
    FTransform GetAimTransform() const;

    UFUNCTION(BlueprintPure, Category = "Weapon View")
    FVector GetRearSightWorldLocation() const;

    UFUNCTION(BlueprintPure, Category = "Weapon View")
    FVector GetFrontSightWorldLocation() const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon View")
    TObjectPtr<USkeletalMeshComponent> gunMesh_ = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon View|Attachments")
    TObjectPtr<UWeaponAttachmentComponent> attachmentComponent_ = nullptr;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon View|Assets")
    TObjectPtr<USkeletalMesh> gunMeshAsset_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon View|Assets")
    TObjectPtr<UStaticMesh> muzzleFlashMeshAsset_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon View|Assets")
    TObjectPtr<UMaterialInterface> muzzleFlashMaterial_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon View|Pose")
    FVector hipLocation_ = FVector::ZeroVector;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon View|Pose")
    FRotator hipRotation_ = FRotator::ZeroRotator;

    // 원본 메시 로컬 좌표의 조준기 윗면. 두 점을 잇는 선을 화면 중앙에 맞춘다.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon View|Sights")
    FVector rearSightLocal_ = FVector::ZeroVector;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon View|Sights")
    FVector frontSightLocal_ = FVector::ZeroVector;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon View|Sights", meta = (ClampMin = "15.0", Units = "cm"))
    float aimEyeDistance_ = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon View|Pose", meta = (ClampMin = "0.01", Units = "s"))
    float aimTransitionTime_ = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon View|Recoil", meta = (ClampMin = "0.0", Units = "deg"))
    float recoilPitch_ = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon View|Recoil", meta = (ClampMin = "0.0", Units = "cm"))
    float kMagazineTravelDistance = 60.0f;

private:
    void ApplyVisualAssets();
    void UpdateFireFeedback(float deltaTime);
    void ApplyReloadPose(FVector& viewLocation, FQuat& viewRotation);

    UPROPERTY(VisibleAnywhere, Category = "Weapon View")
    TObjectPtr<USceneComponent> viewRoot_ = nullptr;

    // 기존 사격 메시와 Blueprint 설정을 유지하고 장전 중에만 뼈대를 직접 움직인다.
    UPROPERTY(VisibleAnywhere, Category = "Weapon View|Reload")
    TObjectPtr<UPoseableMeshComponent> reloadMesh_ = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Weapon View")
    TObjectPtr<UStaticMeshComponent> muzzleFlash_ = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Weapon View")
    TObjectPtr<UPointLightComponent> muzzleLight_ = nullptr;

    float aimAlpha_ = 0.0f;
    float shotElapsed_ = 0.0f;
    float recoilWeight_ = 0.0f;
    float reloadProgress_ = 0.0f;
    bool isReloading_ = false;
    FTransform magazineRestTransform_ = FTransform::Identity;
};
