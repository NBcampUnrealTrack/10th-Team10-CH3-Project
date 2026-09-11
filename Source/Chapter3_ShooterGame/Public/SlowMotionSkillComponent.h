#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SlowMotionSkillComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSlowMotionStateChanged);

UCLASS(Blueprintable, ClassGroup = (Skills), meta = (BlueprintSpawnableComponent))
class CHAPTER3_SHOOTERGAME_API USlowMotionSkillComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USlowMotionSkillComponent();

    UFUNCTION(BlueprintCallable, Category = "Skill|Slow Motion")
    bool TryActivateSlowMotion();

    UFUNCTION(BlueprintCallable, Category = "Skill|Slow Motion")
    void CancelSlowMotion();

    UFUNCTION(BlueprintPure, Category = "Skill|Slow Motion")
    bool CanActivateSlowMotion() const;

    UFUNCTION(BlueprintPure, Category = "Skill|Slow Motion")
    float GetRemainingDuration() const;

    UFUNCTION(BlueprintPure, Category = "Skill|Slow Motion")
    float GetRemainingCooldown() const;

    UPROPERTY(BlueprintAssignable, Category = "Skill|Slow Motion")
    FOnSlowMotionStateChanged onSlowMotionStarted_;

    UPROPERTY(BlueprintAssignable, Category = "Skill|Slow Motion")
    FOnSlowMotionStateChanged onSlowMotionEnded_;

    virtual void TickComponent(float deltaTime, ELevelTick tickType,
        FActorComponentTickFunction* thisTickFunction) override;

protected:
    virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Skill|Slow Motion", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float slowMotionScale_ = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Skill|Slow Motion", meta = (ClampMin = "0.01", Units = "s"))
    float duration_ = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Skill|Slow Motion", meta = (ClampMin = "0.0", Units = "s"))
    float cooldown_ = 0.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skill|Slow Motion")
    bool isSlowMotionActive_ = false;

private:
    void RestoreTimeDilation();

    double endRealTime_ = 0.0;
    double nextActivationRealTime_ = 0.0;
    float previousTimeDilation_ = 1.0f;
    float appliedTimeDilation_ = 1.0f;
};
