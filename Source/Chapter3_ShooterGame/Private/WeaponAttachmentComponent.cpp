#include "WeaponAttachmentComponent.h"

#include "WeaponAttachment.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Components/ChildActorComponent.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogWeaponAttachment, Log, All);

UWeaponAttachmentComponent::UWeaponAttachmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

const FWeaponAttachmentSlot* UWeaponAttachmentComponent::FindSlot(FName slotName) const
{
    const FWeaponAttachmentSlot* foundSlot = nullptr;
    for (const FWeaponAttachmentSlot& slot : attachmentSlots_)
    {
        if (slot.slotName_ == slotName && !slotName.IsNone())
        {
            if (foundSlot != nullptr)
            {
                UE_LOG(LogWeaponAttachment, Warning, TEXT("Duplicate attachment slot: %s"), *slotName.ToString());
                return nullptr;
            }
            foundSlot = &slot;
        }
    }
    return foundSlot;
}

bool UWeaponAttachmentComponent::CanUseSlot(const FWeaponAttachmentSlot& slot, USceneComponent* target) const
{
    return IsValid(target) && (slot.socketName_.IsNone() || target->DoesSocketExist(slot.socketName_))
        && !slot.relativeTransform_.ContainsNaN();
}

void UWeaponAttachmentComponent::BeginPlay()
{
#if WITH_EDITOR
    ClearAttachmentPreview();
#endif
    Super::BeginPlay();
    for (const FWeaponAttachmentSlot& slot : attachmentSlots_)
    {
        if (slot.defaultAttachmentClass_ != nullptr)
        {
            EquipAttachment(slot.slotName_, slot.defaultAttachmentClass_);
        }
    }
}

void UWeaponAttachmentComponent::RefreshAttachmentPreview()
{
#if WITH_EDITOR
    ClearAttachmentPreview();
    UWorld* world = GetWorld();
    if (!showAttachmentPreview_ || IsTemplate() || !IsValid(GetOwner()) || !IsValid(world)
        || world->IsGameWorld() || !IsValid(attachmentTarget_))
    {
        return;
    }

    for (const FWeaponAttachmentSlot& slot : attachmentSlots_)
    {
        if (FindSlot(slot.slotName_) == nullptr || !CanUseSlot(slot, attachmentTarget_)
            || slot.defaultAttachmentClass_ == nullptr
            || slot.defaultAttachmentClass_->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
        {
            continue;
        }

        UChildActorComponent* preview = NewObject<UChildActorComponent>(GetOwner(), NAME_None,
            RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
        preview->CreationMethod = EComponentCreationMethod::UserConstructionScript;
        preview->SetIsVisualizationComponent(true);
        preview->SetupAttachment(attachmentTarget_, slot.socketName_);
        preview->SetRelativeTransform(slot.relativeTransform_);
        preview->SetChildActorClass(slot.defaultAttachmentClass_);
        previewComponents_.Add(preview);
        preview->OnComponentCreated();
        preview->RegisterComponent();

        // 자식 Blueprint의 추가 컴포넌트와 메시 보정을 포함하되 장착 효과는 실행하지 않는다.
        AActor* previewActor = preview->GetChildActor();
        if (IsValid(previewActor))
        {
            previewActor->SetActorEnableCollision(false);
            TArray<UPrimitiveComponent*> primitives;
            previewActor->GetComponents<UPrimitiveComponent>(primitives);
            for (UPrimitiveComponent* primitive : primitives)
            {
                primitive->SetSimulatePhysics(false);
                primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                primitive->SetGenerateOverlapEvents(false);
                primitive->SetOnlyOwnerSee(false);
                primitive->SetOwnerNoSee(false);
                primitive->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
            }
        }
    }
#endif
}

#if WITH_EDITOR
void UWeaponAttachmentComponent::ClearAttachmentPreview()
{
    // 재구성 과정에서 자식 컴포넌트가 먼저 제거된 경우에도 안전하게 정리한다.
    const TArray<TObjectPtr<UChildActorComponent>> previousPreviews = MoveTemp(previewComponents_);
    previewComponents_.Reset();
    for (UChildActorComponent* preview : previousPreviews)
    {
        if (IsValid(preview))
        {
            preview->DestroyComponent();
        }
    }
}
#endif

void UWeaponAttachmentComponent::OnUnregister()
{
#if WITH_EDITOR
    ClearAttachmentPreview();
#endif
    Super::OnUnregister();
}

bool UWeaponAttachmentComponent::SetAttachmentTarget(USceneComponent* target)
{
    if (isChangingAttachments_ || isEndingPlay_ || !IsValid(target) || target->GetOwner() != GetOwner())
    {
        return false;
    }
    for (const TPair<FName, TObjectPtr<AWeaponAttachment>>& entry : equippedAttachments_)
    {
        const FWeaponAttachmentSlot* slot = FindSlot(entry.Key);
        if (IsValid(entry.Value) && (slot == nullptr || !CanUseSlot(*slot, target)))
        {
            return false;
        }
    }
    attachmentTarget_ = target;
    for (const TPair<FName, TObjectPtr<AWeaponAttachment>>& entry : equippedAttachments_)
    {
        const FWeaponAttachmentSlot* slot = FindSlot(entry.Key);
        if (IsValid(entry.Value) && slot != nullptr)
        {
            entry.Value->AttachToComponent(target, FAttachmentTransformRules::SnapToTargetIncludingScale, slot->socketName_);
            entry.Value->SetActorRelativeTransform(slot->relativeTransform_);
        }
    }
    return true;
}

AWeaponAttachment* UWeaponAttachmentComponent::EquipAttachment(FName slotName, TSubclassOf<AWeaponAttachment> attachmentClass)
{
    const FWeaponAttachmentSlot* slot = FindSlot(slotName);
    if (isChangingAttachments_ || isEndingPlay_ || !IsValid(GetOwner()) || GetOwner()->IsActorBeingDestroyed()
        || !IsValid(GetWorld()) || slot == nullptr || !CanUseSlot(*slot, attachmentTarget_)
        || attachmentClass == nullptr || attachmentClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
    {
        UE_LOG(LogWeaponAttachment, Warning, TEXT("Cannot equip slot '%s': check target, unique slot, socket and attachment class."), *slotName.ToString());
        return nullptr;
    }

    TGuardValue<bool> changeGuard(isChangingAttachments_, true);
    // Blueprint 콜백 전에 복사해서 설정 배열 변경으로 인한 포인터 무효화를 피한다.
    const FWeaponAttachmentSlot slotSettings = *slot;
    FActorSpawnParameters spawnParameters = {};
    spawnParameters.Owner = GetOwner();
    spawnParameters.Instigator = GetOwner()->GetInstigator();
    spawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AWeaponAttachment* attachment = GetWorld()->SpawnActor<AWeaponAttachment>(attachmentClass, GetOwner()->GetActorTransform(), spawnParameters);
    if (!IsValid(attachment))
    {
        return nullptr;
    }
    if (isEndingPlay_ || !IsValid(attachmentTarget_) || GetOwner()->IsActorBeingDestroyed()
        || !attachment->AttachToComponent(attachmentTarget_, FAttachmentTransformRules::SnapToTargetIncludingScale, slotSettings.socketName_))
    {
        attachment->Destroy();
        return nullptr;
    }
    attachment->SetActorRelativeTransform(slotSettings.relativeTransform_);
    TArray<UPrimitiveComponent*> primitives;
    attachment->GetComponents<UPrimitiveComponent>(primitives);
    for (UPrimitiveComponent* primitive : primitives)
    {
        primitive->SetSimulatePhysics(false);
        primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        primitive->SetGenerateOverlapEvents(false);
        primitive->SetOnlyOwnerSee(firstPersonOnly_);
        primitive->SetCastShadow(!firstPersonOnly_);
        primitive->SetFirstPersonPrimitiveType(firstPersonOnly_ ? EFirstPersonPrimitiveType::FirstPerson : EFirstPersonPrimitiveType::None);
    }

    // 새 부착물 생성과 연결에 성공한 뒤 기존 부착물을 제거한다.
    RemoveAttachment(slotName);
    if (isEndingPlay_ || GetOwner()->IsActorBeingDestroyed() || !IsValid(attachment))
    {
        if (IsValid(attachment))
        {
            attachment->Destroy();
        }
        return nullptr;
    }
    equippedAttachments_.Add(slotName, attachment);
    attachment->NotifyEquipped(GetOwner(), slotName);
    return GetAttachment(slotName);
}

AWeaponAttachment* UWeaponAttachmentComponent::GetAttachment(FName slotName) const
{
    const TObjectPtr<AWeaponAttachment>* attachment = equippedAttachments_.Find(slotName);
    return attachment != nullptr && IsValid(*attachment) ? attachment->Get() : nullptr;
}

void UWeaponAttachmentComponent::RemoveAttachment(FName slotName)
{
    AWeaponAttachment* attachment = GetAttachment(slotName);
    equippedAttachments_.Remove(slotName);
    if (IsValid(attachment))
    {
        attachment->NotifyUnequipped();
        attachment->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        attachment->Destroy();
    }
}

bool UWeaponAttachmentComponent::UnequipAttachment(FName slotName)
{
    if (isChangingAttachments_ || isEndingPlay_ || GetAttachment(slotName) == nullptr)
    {
        return false;
    }
    TGuardValue<bool> changeGuard(isChangingAttachments_, true);
    RemoveAttachment(slotName);
    return true;
}

void UWeaponAttachmentComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
    isEndingPlay_ = true;
    TArray<FName> slotNames;
    equippedAttachments_.GetKeys(slotNames);
    for (FName slotName : slotNames)
    {
        RemoveAttachment(slotName);
    }
    attachmentTarget_ = nullptr;
    Super::EndPlay(endPlayReason);
}
