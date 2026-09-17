#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnlockInventoryPickup.generated.h"

class APawn;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class CHAPTER3_SHOOTERGAME_API AUnlockInventoryPickup : public AActor
{
    GENERATED_BODY()

public:
    AUnlockInventoryPickup();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unlock Pickup")
    TObjectPtr<USphereComponent> pickupSphere_ = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unlock Pickup")
    TObjectPtr<UStaticMeshComponent> itemMesh_ = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock Pickup")
    FName itemId_ = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock Pickup")
    bool collectOnOverlap_ = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock Pickup", meta = (ClampMin = "1.0", Units = "cm"))
    float pickupRadius_ = 75.0f;

    // 직접 상호작용에서도 호출 가능. 중복 획득은 소비하지만 해금 알림은 반복하지 않는다.
    UFUNCTION(BlueprintCallable, Category = "Unlock Pickup")
    bool TryCollect(APawn* collector);

    // 이 이벤트 직후 Actor를 파괴한다. 획득 이펙트 등의 확장용이다.
    UFUNCTION(BlueprintImplementableEvent, Category = "Unlock Pickup")
    void OnCollected(APawn* collector, bool newlyUnlocked);

protected:
    virtual void OnConstruction(const FTransform& transform) override;
    virtual void BeginPlay() override;

private:
    UFUNCTION()
    void HandleOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor,
        UPrimitiveComponent* otherComponent, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

    bool isCollecting_ = false;
    bool isCollected_ = false;
};
