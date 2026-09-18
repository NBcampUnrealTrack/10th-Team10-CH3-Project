#include "CoinThrowSkillComponent.h"

#include "DistractionCoin.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogCoinThrowSkill, Log, All);

namespace
{
    constexpr float kCoinSkillSpawnForwardOffset = 45.0f;
    constexpr float kCoinSkillSpawnRightOffset = 12.0f;
    constexpr float kCoinSkillSpawnDownOffset = 10.0f;
}

UCoinThrowSkillComponent::UCoinThrowSkillComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCoinThrowSkillComponent::BeginPlay()
{
    maxCoinCount_ = FMath::Max(1, maxCoinCount_);
    currentCoinCount_ = maxCoinCount_;
    Super::BeginPlay();
}

int32 UCoinThrowSkillComponent::GetCurrentCoinCount() const
{
    return currentCoinCount_;
}

int32 UCoinThrowSkillComponent::GetMaxCoinCount() const
{
    return FMath::Max(1, maxCoinCount_);
}

bool UCoinThrowSkillComponent::TryRestoreCoin()
{
    if (!HasBegunPlay() || currentCoinCount_ >= GetMaxCoinCount())
    {
        return false;
    }

    ++currentCoinCount_;
    onCoinCountChanged_.Broadcast(currentCoinCount_, GetMaxCoinCount());
    return true;
}

void UCoinThrowSkillComponent::InitializeFromLegacySettings(TSubclassOf<ADistractionCoin> coinClass,
    float throwSpeed, float upwardSpeed, float cooldown)
{
    if (!coinClass_ && coinClass)
    {
        coinClass_ = coinClass;
        coinThrowSpeed_ = throwSpeed;
        coinUpwardSpeed_ = upwardSpeed;
        cooldown_ = cooldown;
    }
}

APlayerController* UCoinThrowSkillComponent::GetThrowingController() const
{
    return Cast<APlayerController>(GetOwner());
}

void UCoinThrowSkillComponent::SetIgnoredWeapon(AActor* weapon)
{
    ignoredWeapon_ = weapon;
}

bool UCoinThrowSkillComponent::CanThrowCoin() const
{
    const UWorld* world = GetWorld();
    const APlayerController* controller = GetThrowingController();
    if (!world || !world->IsGameWorld() || world->IsPaused() || !IsValid(controller)
        || !controller->IsLocalController() || !controller->HasAuthority()
        || !IsValid(controller->GetPawn()) || !coinClass_ || isThrowing_ || currentCoinCount_ <= 0
        || !FMath::IsFinite(coinThrowSpeed_) || coinThrowSpeed_ <= 0.0f
        || !FMath::IsFinite(coinUpwardSpeed_) || coinUpwardSpeed_ < 0.0f
        || !FMath::IsFinite(cooldown_) || cooldown_ < 0.0f
        || coinClass_->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)
        || GetRemainingCooldown() > 0.0f)
    {
        return false;
    }

    FVector spawnLocation;
    FRotator viewRotation;
    return FindSpawnTransform(spawnLocation, viewRotation);
}

bool UCoinThrowSkillComponent::FindSpawnTransform(FVector& spawnLocation, FRotator& viewRotation) const
{
    const APlayerController* controller = GetThrowingController();
    const APawn* pawn = controller ? controller->GetPawn() : nullptr;
    const UWorld* world = GetWorld();
    const ADistractionCoin* defaultCoin = coinClass_ ? coinClass_.GetDefaultObject() : nullptr;
    if (!world || !IsValid(controller) || !IsValid(pawn) || !defaultCoin)
    {
        return false;
    }

    FVector viewLocation;
    controller->GetPlayerViewPoint(viewLocation, viewRotation);
    spawnLocation = viewLocation + viewRotation.RotateVector(
        FVector(kCoinSkillSpawnForwardOffset, kCoinSkillSpawnRightOffset, -kCoinSkillSpawnDownOffset));
    if (viewLocation.ContainsNaN() || viewRotation.ContainsNaN() || spawnLocation.ContainsNaN())
    {
        return false;
    }

    FCollisionQueryParams queryParams(SCENE_QUERY_STAT(CoinSkillSpawn), false);
    queryParams.AddIgnoredActor(controller);
    queryParams.AddIgnoredActor(pawn);
    queryParams.AddIgnoredActor(ignoredWeapon_.Get());
    TArray<AActor*> attachedActors;
    pawn->GetAttachedActors(attachedActors, true, true);
    queryParams.AddIgnoredActors(attachedActors);

    const float radius = defaultCoin->GetCollisionRadius();
    if (!FMath::IsFinite(radius) || radius <= 0.0f)
    {
        return false;
    }

    const FCollisionShape collisionShape = FCollisionShape::MakeSphere(radius);
    return !world->SweepTestByChannel(viewLocation, spawnLocation, FQuat::Identity,
        ECC_WorldDynamic, collisionShape, queryParams);
}

bool UCoinThrowSkillComponent::TryThrowCoin()
{
    if (!CanThrowCoin())
    {
        return false;
    }

    TGuardValue<bool> throwingGuard(isThrowing_, true);
    FVector spawnLocation;
    FRotator viewRotation;
    if (!FindSpawnTransform(spawnLocation, viewRotation))
    {
        return false;
    }

    APlayerController* controller = GetThrowingController();
    APawn* pawn = controller->GetPawn();
    FActorSpawnParameters spawnParams;
    spawnParams.Owner = pawn;
    spawnParams.Instigator = pawn;
    spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ADistractionCoin* coin = GetWorld()->SpawnActor<ADistractionCoin>(coinClass_, spawnLocation, viewRotation, spawnParams);
    if (!IsValid(coin))
    {
        return false;
    }

    nextCoinThrowTime_ = GetWorld()->GetTimeSeconds() + cooldown_;
    --currentCoinCount_;
    onCoinThrown_.Broadcast(coin);
    if (IsValid(coin))
    {
        coin->LaunchCoin(viewRotation.Vector() * coinThrowSpeed_ + FVector::UpVector * coinUpwardSpeed_);
        UE_LOG(LogCoinThrowSkill, Log, TEXT("[CoinThrow] Launched %s"), *GetNameSafe(coin));
    }
    onCoinCountChanged_.Broadcast(currentCoinCount_, GetMaxCoinCount());
    return true;
}

float UCoinThrowSkillComponent::GetRemainingCooldown() const
{
    const UWorld* world = GetWorld();
    return world ? static_cast<float>(FMath::Max(0.0, nextCoinThrowTime_ - world->GetTimeSeconds())) : 0.0f;
}

float UCoinThrowSkillComponent::GetCooldownDuration() const
{
    return FMath::Max(0.0f, cooldown_);
}
