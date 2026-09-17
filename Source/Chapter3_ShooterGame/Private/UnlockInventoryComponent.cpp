#include "UnlockInventoryComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogUnlockInventory, Log, All);

UUnlockInventoryComponent::UUnlockInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UUnlockInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    EnsureInitialized();
}

void UUnlockInventoryComponent::EnsureInitialized()
{
    if (isInitialized_)
    {
        return;
    }

    isInitialized_ = true;
    slots_.Reserve(itemDefinitions_.Num());
    for (const FUnlockInventoryItem& item : itemDefinitions_)
    {
        if (item.itemId_.IsNone() || FindSlotIndex(item.itemId_) != INDEX_NONE)
        {
            UE_LOG(LogUnlockInventory, Warning, TEXT("%s: Item Definitions contains an empty or duplicate Item ID '%s'; entry skipped."),
                *GetNameSafe(GetOwner()), *item.itemId_.ToString());
            continue;
        }

        FUnlockInventorySlot slot = {};
        slot.item_ = item;
        slots_.Add(slot);
    }
}

int32 UUnlockInventoryComponent::FindSlotIndex(FName itemId) const
{
    return slots_.IndexOfByPredicate([itemId](const FUnlockInventorySlot& slot)
    {
        return slot.item_.itemId_ == itemId;
    });
}

EUnlockInventoryResult UUnlockInventoryComponent::UnlockItem(FName itemId)
{
    EnsureInitialized();
    const int32 slotIndex = FindSlotIndex(itemId);
    if (slotIndex == INDEX_NONE)
    {
        return EUnlockInventoryResult::InvalidItem;
    }

    if (slots_[slotIndex].isUnlocked_)
    {
        return EUnlockInventoryResult::AlreadyUnlocked;
    }

    slots_[slotIndex].isUnlocked_ = true;
    const FUnlockInventorySlot unlockedSlot = slots_[slotIndex];
    onItemUnlocked_.Broadcast(slotIndex, unlockedSlot);
    return EUnlockInventoryResult::Unlocked;
}

TArray<FUnlockInventorySlot> UUnlockInventoryComponent::GetSlots()
{
    EnsureInitialized();
    return slots_;
}

bool UUnlockInventoryComponent::GetSlot(FName itemId, FUnlockInventorySlot& slot)
{
    EnsureInitialized();
    const int32 slotIndex = FindSlotIndex(itemId);
    slot = slotIndex != INDEX_NONE ? slots_[slotIndex] : FUnlockInventorySlot{};
    return slotIndex != INDEX_NONE;
}

bool UUnlockInventoryComponent::IsItemUnlocked(FName itemId)
{
    FUnlockInventorySlot slot = {};
    return GetSlot(itemId, slot) && slot.isUnlocked_;
}
