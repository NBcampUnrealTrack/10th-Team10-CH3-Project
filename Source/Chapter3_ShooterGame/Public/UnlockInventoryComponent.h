#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnlockInventoryComponent.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct CHAPTER3_SHOOTERGAME_API FUnlockInventoryItem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock Inventory")
    FName itemId_ = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock Inventory")
    FText displayName_ = FText::GetEmpty();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock Inventory", meta = (MultiLine = "true"))
    FText description_ = FText::GetEmpty();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock Inventory")
    TObjectPtr<UTexture2D> icon_ = nullptr;
};

USTRUCT(BlueprintType)
struct CHAPTER3_SHOOTERGAME_API FUnlockInventorySlot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Unlock Inventory")
    FUnlockInventoryItem item_ = {};

    UPROPERTY(BlueprintReadOnly, Category = "Unlock Inventory")
    bool isUnlocked_ = false;
};

UENUM(BlueprintType)
enum class EUnlockInventoryResult : uint8
{
    InvalidItem,
    AlreadyUnlocked,
    Unlocked,
    ProgressUnavailable
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemUnlocked, int32, slotIndex, FUnlockInventorySlot, slot);

// UI는 슬롯을 조회하고 해금 이벤트를 구독한다.
UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class CHAPTER3_SHOOTERGAME_API UUnlockInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UUnlockInventoryComponent();

    // 배열 순서가 UI 슬롯 순서다. None 또는 중복 ID는 초기화 때 제외한다.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unlock Inventory", meta = (TitleProperty = "itemId_"))
    TArray<FUnlockInventoryItem> itemDefinitions_ = {};

    UPROPERTY(BlueprintAssignable, Category = "Unlock Inventory")
    FOnInventoryItemUnlocked onItemUnlocked_;

    UFUNCTION(BlueprintCallable, Category = "Unlock Inventory")
    EUnlockInventoryResult UnlockItem(FName itemId);

    UFUNCTION(BlueprintPure, Category = "Unlock Inventory")
    TArray<FUnlockInventorySlot> GetSlots();

    // false이면 등록되지 않은 ID이며 slot은 기본값으로 반환한다.
    UFUNCTION(BlueprintPure, Category = "Unlock Inventory")
    bool GetSlot(FName itemId, FUnlockInventorySlot& slot);

    UFUNCTION(BlueprintPure, Category = "Unlock Inventory")
    bool IsItemUnlocked(FName itemId);

protected:
    virtual void BeginPlay() override;

private:
    // 첫 조회 또는 획득 때도 초기화한다. 재조회로 해금 상태를 초기화하지 않는다.
    void EnsureInitialized();
    int32 FindSlotIndex(FName itemId) const;

    UPROPERTY(Transient)
    TArray<FUnlockInventorySlot> slots_ = {};

    bool isInitialized_ = false;
};
