#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponAttachment.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UWeaponAttachmentComponent;

// 개별 부착물은 이 클래스를 C++ 또는 Blueprint로 상속한다.
UCLASS(Blueprintable)
class CHAPTER3_SHOOTERGAME_API AWeaponAttachment : public AActor
{
    GENERATED_BODY()

public:
    AWeaponAttachment();

    UFUNCTION(BlueprintPure, Category = "Weapon Attachment")
    AActor* GetAttachedWeapon() const;

    UFUNCTION(BlueprintPure, Category = "Weapon Attachment")
    FName GetAttachmentSlot() const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Attachment")
    TObjectPtr<UStaticMeshComponent> attachmentMesh_ = nullptr;

protected:
    virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

    // 장착 효과를 적용한다. C++ 자식은 _Implementation을 재정의한다.
    UFUNCTION(BlueprintNativeEvent, Category = "Weapon Attachment")
    void OnEquipped(AActor* weapon);
    virtual void OnEquipped_Implementation(AActor* weapon);

    // 장착 때 적용한 효과를 되돌린다. 해제/교체/직접 파괴 시 한 번 호출한다.
    UFUNCTION(BlueprintNativeEvent, Category = "Weapon Attachment")
    void OnUnequipped(AActor* weapon);
    virtual void OnUnequipped_Implementation(AActor* weapon);

private:
    friend class UWeaponAttachmentComponent;
    void NotifyEquipped(AActor* weapon, FName slotName);
    void NotifyUnequipped();

    UPROPERTY(VisibleAnywhere, Category = "Weapon Attachment")
    TObjectPtr<USceneComponent> attachmentRoot_ = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<AActor> attachedWeapon_ = nullptr;

    FName attachmentSlot_ = NAME_None;
};
