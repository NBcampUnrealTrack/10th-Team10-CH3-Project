// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponCustomizationWidget.h"

#include "Chapter3GameInstance.h"
#include "Chapter3_ShooterGame_PlayerController.h"
#include "WeaponAttachment.h"
#include "WeaponAttachmentComponent.h"
#include "M1911WeaponView.h"
#include "Kismet/GameplayStatics.h"

const FAttachmentShopItem* UWeaponCustomizationWidget::FindShopRow(FName ItemId) const
{
    return IsValid(AttachmentShopTable)
        ? AttachmentShopTable->FindRow<FAttachmentShopItem>(ItemId, TEXT("WeaponCustomizationWidget"))
        : nullptr;
}

UChapter3GameInstance* UWeaponCustomizationWidget::GetChapter3GameInstance() const
{
    return Cast<UChapter3GameInstance>(UGameplayStatics::GetGameInstance(this));
}

AChapter3_ShooterGame_PlayerController* UWeaponCustomizationWidget::GetShooterController() const
{
    return Cast<AChapter3_ShooterGame_PlayerController>(GetOwningPlayer());
}

UWeaponAttachmentComponent* UWeaponCustomizationWidget::GetCurrentWeaponAttachments() const
{
    const AChapter3_ShooterGame_PlayerController* controller = GetShooterController();
    AM1911WeaponView* weapon = IsValid(controller) ? controller->GetCurrentWeapon() : nullptr;
    return IsValid(weapon) ? weapon->FindComponentByClass<UWeaponAttachmentComponent>() : nullptr;
}

int64 UWeaponCustomizationWidget::GetPlayerMoney() const
{
    const UChapter3GameInstance* gameInstance = GetChapter3GameInstance();
    return IsValid(gameInstance) ? gameInstance->GetMoney() : 0;
}

bool UWeaponCustomizationWidget::IsAttachmentOwned(FName ItemId) const
{
    const UChapter3GameInstance* gameInstance = GetChapter3GameInstance();
    return IsValid(gameInstance) && gameInstance->HasCollectedItem(ItemId);
}

bool UWeaponCustomizationWidget::IsAttachmentEquipped(FName ItemId) const
{
    const FAttachmentShopItem* row = FindShopRow(ItemId);
    UWeaponAttachmentComponent* attachments = GetCurrentWeaponAttachments();
    if (row == nullptr || attachments == nullptr)
    {
        return false;
    }

    const AWeaponAttachment* equipped = attachments->GetAttachment(row->AttachmentSlot);
    return IsValid(equipped) && equipped->GetClass() == row->AttachmentClass;
}

bool UWeaponCustomizationWidget::BuyAttachment(FName ItemId)
{
    if (IsAttachmentOwned(ItemId))
    {
        return false; 
    }

    const FAttachmentShopItem* row = FindShopRow(ItemId);
    UChapter3GameInstance* gameInstance = GetChapter3GameInstance();
    if (row == nullptr || !IsValid(gameInstance))
    {
        return false;
    }

    if (!gameInstance->SpendMoney(row->Price))
    {
        return false; 
    }

    
    gameInstance->CollectItem(ItemId);
    return true;
}

bool UWeaponCustomizationWidget::EquipAttachment(FName ItemId)
{
    if (!IsAttachmentOwned(ItemId))
    {
        return false; 
    }

    const FAttachmentShopItem* row = FindShopRow(ItemId);
    UWeaponAttachmentComponent* attachments = GetCurrentWeaponAttachments();
    if (row == nullptr || attachments == nullptr || row->AttachmentClass == nullptr)
    {
        return false;
    }

    
    return attachments->EquipAttachment(row->AttachmentSlot, row->AttachmentClass) != nullptr;
}

bool UWeaponCustomizationWidget::UnequipSlot(FName SlotName)
{
    UWeaponAttachmentComponent* attachments = GetCurrentWeaponAttachments();
    return IsValid(attachments) && attachments->UnequipAttachment(SlotName);
}

TArray<FAttachmentShopEntry> UWeaponCustomizationWidget::GetShopEntries() const
{
    TArray<FAttachmentShopEntry> entries;
    if (!IsValid(AttachmentShopTable))
    {
        return entries;
    }

    for (const FName& rowName : AttachmentShopTable->GetRowNames())
    {
        const FAttachmentShopItem* row = AttachmentShopTable->FindRow<FAttachmentShopItem>(rowName, TEXT("GetShopEntries"));
        if (row == nullptr)
        {
            continue;
        }

        FAttachmentShopEntry entry;
        entry.ItemId = rowName;
        entry.DisplayName = row->DisplayName;
        entry.Icon = row->Icon;
        entry.Price = row->Price;
        entry.AttachmentSlot = row->AttachmentSlot;
        entry.bOwned = IsAttachmentOwned(rowName);
        entry.bEquipped = IsAttachmentEquipped(rowName);
        entries.Add(entry);
    }
    return entries;
}
