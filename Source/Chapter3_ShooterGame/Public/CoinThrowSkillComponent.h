#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoinThrowSkillComponent.generated.h"

class ADistractionCoin;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoinThrown, ADistractionCoin*, coin);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCoinCountChanged, int32, currentCount, int32, maxCount);

UCLASS(Blueprintable, ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class CHAPTER3_SHOOTERGAME_API UCoinThrowSkillComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCoinThrowSkillComponent();

    UFUNCTION(BlueprintCallable, Category = "Skill|Coin Throw")
    bool TryThrowCoin();

    UFUNCTION(BlueprintPure, Category = "Skill|Coin Throw")
    bool CanThrowCoin() const;

    UFUNCTION(BlueprintPure, Category = "Skill|Coin Throw")
    float GetRemainingCooldown() const;

    UFUNCTION(BlueprintPure, Category = "Skill|Coin Throw")
    float GetCooldownDuration() const;

    UFUNCTION(BlueprintPure, Category = "Skill|Coin Throw")
    int32 GetCurrentCoinCount() const;

    UFUNCTION(BlueprintPure, Category = "Skill|Coin Throw")
    int32 GetMaxCoinCount() const;

    UPROPERTY(BlueprintAssignable, Category = "Skill|Coin Throw")
    FOnCoinCountChanged onCoinCountChanged_;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Coin Throw", meta = (ClampMin = "1"))
    int32 maxCoinCount_ = 3;

    UPROPERTY(BlueprintAssignable, Category = "Skill|Coin Throw")
    FOnCoinThrown onCoinThrown_;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Skill|Coin Throw")
    TSubclassOf<ADistractionCoin> coinClass_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Skill|Coin Throw", meta = (ClampMin = "0.0", Units = "cm/s"))
    float coinThrowSpeed_ = 1000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Skill|Coin Throw", meta = (ClampMin = "0.0", Units = "cm/s"))
    float coinUpwardSpeed_ = 300.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Skill|Coin Throw", meta = (ClampMin = "0.0", Units = "s"))
    float cooldown_ = 0.6f;

    // 기존 컨트롤러 BP의 설정은 컴포넌트에 코인 클래스를 지정하지 않았을 때만 가져온다.
    void InitializeFromLegacySettings(TSubclassOf<ADistractionCoin> coinClass,
        float throwSpeed, float upwardSpeed, float cooldown);

    void SetIgnoredWeapon(AActor* weapon);

protected:
    virtual void BeginPlay() override;

private:
    friend class ADistractionCoin;

    bool TryRestoreCoin();
    APlayerController* GetThrowingController() const;
    bool FindSpawnTransform(FVector& spawnLocation, FRotator& viewRotation) const;

    double nextCoinThrowTime_ = 0.0;
    bool isThrowing_ = false;
    UPROPERTY(Transient)
    int32 currentCoinCount_ = 0;

    TWeakObjectPtr<AActor> ignoredWeapon_;
};
