#include "SlowMotionSkillComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectIterator.h"

namespace
{
    constexpr float kDefaultSlowMotionScale = 0.2f;
    constexpr float kDefaultDuration = 3.0f;
    constexpr float kDefaultCooldown = 8.0f;
}

USlowMotionSkillComponent::USlowMotionSkillComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    slowMotionScale_ = kDefaultSlowMotionScale;
    duration_ = kDefaultDuration;
    cooldown_ = kDefaultCooldown;
}

bool USlowMotionSkillComponent::CanActivateSlowMotion() const
{
    const UWorld* world = GetWorld();
    const AActor* owner = GetOwner();
    const bool canActivate = world && world->IsGameWorld() && !world->IsPaused()
        && IsValid(owner) && owner->HasAuthority() && !isSlowMotionActive_
        && GetRemainingCooldown() <= 0.0f
        && FMath::IsFinite(slowMotionScale_) && slowMotionScale_ > 0.0f && slowMotionScale_ < 1.0f
        && FMath::IsFinite(duration_) && duration_ > 0.0f
        && FMath::IsFinite(cooldown_) && cooldown_ >= 0.0f;
    if (!canActivate)
    {
        return false;
    }

    for (TObjectIterator<USlowMotionSkillComponent> skill; skill; ++skill)
    {
        if (skill->GetWorld() == world && skill->isSlowMotionActive_)
        {
            return false;
        }
    }
    return true;
}

bool USlowMotionSkillComponent::TryActivateSlowMotion()
{
    if (!CanActivateSlowMotion())
    {
        return false;
    }

    previousTimeDilation_ = UGameplayStatics::GetGlobalTimeDilation(this);
    UGameplayStatics::SetGlobalTimeDilation(this, previousTimeDilation_ * slowMotionScale_);
    appliedTimeDilation_ = UGameplayStatics::GetGlobalTimeDilation(this);
    endRealTime_ = GetWorld()->GetRealTimeSeconds() + duration_;
    isSlowMotionActive_ = true;
    SetComponentTickEnabled(true);
    onSlowMotionStarted_.Broadcast();
    return true;
}

void USlowMotionSkillComponent::CancelSlowMotion()
{
    if (!isSlowMotionActive_)
    {
        return;
    }

    RestoreTimeDilation();
    isSlowMotionActive_ = false;
    const UWorld* world = GetWorld();
    nextActivationRealTime_ = world ? world->GetRealTimeSeconds() + FMath::Max(0.0f, cooldown_) : 0.0;
    SetComponentTickEnabled(false);
    onSlowMotionEnded_.Broadcast();
}

void USlowMotionSkillComponent::RestoreTimeDilation()
{
    if (GetWorld() && isSlowMotionActive_
        && FMath::IsNearlyEqual(UGameplayStatics::GetGlobalTimeDilation(this), appliedTimeDilation_))
    {
        UGameplayStatics::SetGlobalTimeDilation(this, previousTimeDilation_);
    }
}

float USlowMotionSkillComponent::GetRemainingDuration() const
{
    const UWorld* world = GetWorld();
    return world && isSlowMotionActive_
        ? static_cast<float>(FMath::Max(0.0, endRealTime_ - world->GetRealTimeSeconds())) : 0.0f;
}

float USlowMotionSkillComponent::GetRemainingCooldown() const
{
    const UWorld* world = GetWorld();
    return world ? static_cast<float>(FMath::Max(0.0, nextActivationRealTime_ - world->GetRealTimeSeconds())) : 0.0f;
}

void USlowMotionSkillComponent::TickComponent(float deltaTime, ELevelTick tickType,
    FActorComponentTickFunction* thisTickFunction)
{
    Super::TickComponent(deltaTime, tickType, thisTickFunction);
    if (isSlowMotionActive_ && GetRemainingDuration() <= 0.0f)
    {
        CancelSlowMotion();
    }
}

void USlowMotionSkillComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
    RestoreTimeDilation();
    isSlowMotionActive_ = false;
    Super::EndPlay(endPlayReason);
}
