#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BonusPickup.generated.h"

class APawn;
class USphereComponent;
class UStaticMeshComponent;

// 현금과 보석처럼 맵을 다시 시작하면 다시 획득할 수 있는 보너스 아이템이다.
UCLASS()
class CHAPTER3_SHOOTERGAME_API ABonusPickup : public AActor
{
    GENERATED_BODY()

public:
    ABonusPickup();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bonus Pickup")
    TObjectPtr<USphereComponent> pickupSphere_ = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bonus Pickup")
    TObjectPtr<UStaticMeshComponent> itemMesh_ = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonus Pickup", meta = (ClampMin = "1.0", Units = "cm"))
    float pickupRadius_ = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonus Pickup|Reward", meta = (ClampMin = "0", UIMin = "0"))
    int32 scoreReward_ = 250;

    bool IsCollectorInRange(const APawn* collector) const;

    // 범위 안에서 상호작용할 때만 지급한다. 같은 Actor는 한 번만 지급한다.
    UFUNCTION(BlueprintCallable, Category = "Bonus Pickup")
    bool TryCollect(APawn* collector);

    // 보상 지급 후 호출되며, 이벤트 직후 Actor가 파괴된다.
    UFUNCTION(BlueprintImplementableEvent, Category = "Bonus Pickup")
    void OnCollected(APawn* collector, int32 score);

protected:
    virtual void OnConstruction(const FTransform& transform) override;

private:
    bool isCollecting_ = false;
    bool isCollected_ = false;
};
