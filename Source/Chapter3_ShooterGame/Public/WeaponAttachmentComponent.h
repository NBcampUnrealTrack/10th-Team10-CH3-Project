#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponAttachmentComponent.generated.h"

class AWeaponAttachment;
class USceneComponent;
class UChildActorComponent;

USTRUCT(BlueprintType)
struct CHAPTER3_SHOOTERGAME_API FWeaponAttachmentSlot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Attachment")
    FName slotName_ = NAME_None;

    // None이면 메시 원점에 부착한다. 그 외에는 실제 소켓/본이 있어야 한다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Attachment")
    FName socketName_ = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Attachment")
    FTransform relativeTransform_ = FTransform::Identity;

    // 비워 두면 게임 시작 시 빈 슬롯으로 유지한다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Attachment")
    TSubclassOf<AWeaponAttachment> defaultAttachmentClass_ = nullptr;
};

// 총 Actor에 추가해서 사용한다. 슬롯마다 하나의 부착물을 소유한다.
UCLASS(ClassGroup = (Weapon), meta = (BlueprintSpawnableComponent))
class CHAPTER3_SHOOTERGAME_API UWeaponAttachmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeaponAttachmentComponent();

    // BeginPlay 전에 대상을 지정하면 기본 부착물도 자동 장착된다.
    // 실행 중 대상 교체는 장전용 메시 전환에 사용한다.
    UFUNCTION(BlueprintCallable, Category = "Weapon Attachment")
    bool SetAttachmentTarget(USceneComponent* target);

    UFUNCTION(BlueprintCallable, Category = "Weapon Attachment")
    AWeaponAttachment* EquipAttachment(FName slotName, TSubclassOf<AWeaponAttachment> attachmentClass);

    // 해제한 Actor는 파괴한다. 인벤토리/월드 드롭은 개별 게임 로직에서 처리한다.
    UFUNCTION(BlueprintCallable, Category = "Weapon Attachment")
    bool UnequipAttachment(FName slotName);

    UFUNCTION(BlueprintPure, Category = "Weapon Attachment")
    AWeaponAttachment* GetAttachment(FName slotName) const;

    // 총의 Construction Script에서도 호출할 수 있다. 게임 월드에서는 아무것도 생성하지 않는다.
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Weapon Attachment|Preview")
    void RefreshAttachmentPreview();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Attachment")
    TArray<FWeaponAttachmentSlot> attachmentSlots_;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Attachment")
    bool firstPersonOnly_ = true;

#if WITH_EDITORONLY_DATA
    UPROPERTY(EditDefaultsOnly, Category = "Weapon Attachment|Preview")
    bool showAttachmentPreview_ = true;
#endif

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
    virtual void OnUnregister() override;

private:
    const FWeaponAttachmentSlot* FindSlot(FName slotName) const;
    bool CanUseSlot(const FWeaponAttachmentSlot& slot, USceneComponent* target) const;
    void RemoveAttachment(FName slotName);

#if WITH_EDITOR
    void ClearAttachmentPreview();
#endif

#if WITH_EDITORONLY_DATA
    // 저장/복제하지 않는 에디터 전용 컴포넌트. 게임의 equippedAttachments_와 분리한다.
    UPROPERTY(Transient, DuplicateTransient, TextExportTransient)
    TArray<TObjectPtr<UChildActorComponent>> previewComponents_;
#endif

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> attachmentTarget_ = nullptr;

    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<AWeaponAttachment>> equippedAttachments_;

    bool isChangingAttachments_ = false;
    bool isEndingPlay_ = false;
};
