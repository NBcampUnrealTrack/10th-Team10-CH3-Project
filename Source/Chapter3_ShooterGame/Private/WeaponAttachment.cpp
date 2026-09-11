#include "WeaponAttachment.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AWeaponAttachment::AWeaponAttachment()
{
    PrimaryActorTick.bCanEverTick = false;
    SetReplicates(false);

    attachmentRoot_ = CreateDefaultSubobject<USceneComponent>(TEXT("AttachmentRoot"));
    SetRootComponent(attachmentRoot_);
    attachmentMesh_ = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttachmentMesh"));
    attachmentMesh_->SetupAttachment(attachmentRoot_);
    attachmentMesh_->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    attachmentMesh_->SetGenerateOverlapEvents(false);
}

AActor* AWeaponAttachment::GetAttachedWeapon() const
{
    return attachedWeapon_;
}

FName AWeaponAttachment::GetAttachmentSlot() const
{
    return attachmentSlot_;
}

void AWeaponAttachment::NotifyEquipped(AActor* weapon, FName slotName)
{
    attachedWeapon_ = weapon;
    attachmentSlot_ = slotName;
    OnEquipped(weapon);
}

void AWeaponAttachment::NotifyUnequipped()
{
    if (attachedWeapon_ != nullptr)
    {
        AActor* weapon = attachedWeapon_;
        attachedWeapon_ = nullptr;
        attachmentSlot_ = NAME_None;
        OnUnequipped(weapon);
    }
}

void AWeaponAttachment::EndPlay(const EEndPlayReason::Type endPlayReason)
{
    NotifyUnequipped();
    Super::EndPlay(endPlayReason);
}

void AWeaponAttachment::OnEquipped_Implementation(AActor* weapon)
{
}

void AWeaponAttachment::OnUnequipped_Implementation(AActor* weapon)
{
}
