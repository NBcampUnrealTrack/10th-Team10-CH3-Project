#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "WeaponCustomizationWidget.generated.h"

class AWeaponAttachment;
class UDataTable;
class UTexture2D;
class UWeaponAttachmentComponent;
class AChapter3_ShooterGame_PlayerController;
class UChapter3GameInstance;

USTRUCT(BlueprintType)
struct FAttachmentShopItem : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
    TObjectPtr<UTexture2D> Icon = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop", meta = (ClampMin = "0"))
    int64 Price = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
    FName AttachmentSlot = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
    TSubclassOf<AWeaponAttachment> AttachmentClass;
};


USTRUCT(BlueprintType)
struct FAttachmentShopEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Shop")
    FName ItemId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Shop")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Shop")
    TObjectPtr<UTexture2D> Icon = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Shop")
    int64 Price = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Shop")
    FName AttachmentSlot = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Shop")
    bool bOwned = false;

    UPROPERTY(BlueprintReadOnly, Category = "Shop")
    bool bEquipped = false;
};

UCLASS()
class CHAPTER3_SHOOTERGAME_API UWeaponCustomizationWidget : public UUserWidget
{
    GENERATED_BODY()

public:
   
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    TObjectPtr<UDataTable> AttachmentShopTable;

    UFUNCTION(BlueprintCallable, Category = "Shop")
    TArray<FAttachmentShopEntry> GetShopEntries() const;

    UFUNCTION(BlueprintPure, Category = "Shop")
    int64 GetPlayerMoney() const;

    UFUNCTION(BlueprintPure, Category = "Shop")
    bool IsAttachmentOwned(FName ItemId) const;

    UFUNCTION(BlueprintPure, Category = "Shop")
    bool IsAttachmentEquipped(FName ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "Shop")
    bool BuyAttachment(FName ItemId);

    UFUNCTION(BlueprintCallable, Category = "Shop")
    bool EquipAttachment(FName ItemId);

    UFUNCTION(BlueprintCallable, Category = "Shop")
    bool UnequipSlot(FName SlotName);

private:
    const FAttachmentShopItem* FindShopRow(FName ItemId) const;
    UChapter3GameInstance* GetChapter3GameInstance() const;
    AChapter3_ShooterGame_PlayerController* GetShooterController() const;
    UWeaponAttachmentComponent* GetCurrentWeaponAttachments() const;
};
