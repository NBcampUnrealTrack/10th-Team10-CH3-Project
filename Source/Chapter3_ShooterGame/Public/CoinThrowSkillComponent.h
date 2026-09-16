#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoinThrowSkillComponent.generated.h"

class ADistractionCoin;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoinThrown, ADistractionCoin*, coin);

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

private:
    APlayerController* GetThrowingController() const;
    bool FindSpawnTransform(FVector& spawnLocation, FRotator& viewRotation) const;

    double nextCoinThrowTime_ = 0.0;
    bool isThrowing_ = false;
    TWeakObjectPtr<AActor> ignoredWeapon_;
};
