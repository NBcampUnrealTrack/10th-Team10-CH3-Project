#include "BonusPickup.h"

#include "Chapter3GameInstance.h"
#include "Chapter3_ShooterGame_GameMode.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "PickupRange.h"

DEFINE_LOG_CATEGORY_STATIC(LogBonusPickup, Log, All);

ABonusPickup::ABonusPickup()
{
    PrimaryActorTick.bCanEverTick = false;

    pickupSphere_ = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
    SetRootComponent(pickupSphere_);
    pickupSphere_->InitSphereRadius(pickupRadius_);
    pickupSphere_->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    pickupSphere_->SetCollisionResponseToAllChannels(ECR_Ignore);
    pickupSphere_->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    pickupSphere_->SetGenerateOverlapEvents(true);

    itemMesh_ = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
    itemMesh_->SetupAttachment(pickupSphere_);
    itemMesh_->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABonusPickup::OnConstruction(const FTransform& transform)
{
    Super::OnConstruction(transform);
    const float safeRadius = FMath::IsFinite(pickupRadius_) ? FMath::Max(1.0f, pickupRadius_) : 1.0f;
    pickupSphere_->SetSphereRadius(safeRadius);
}

bool ABonusPickup::IsCollectorInRange(const APawn* collector) const
{
    return !isCollected_ && !IsActorBeingDestroyed() && Chapter3PickupRange::Contains(pickupSphere_, collector);
}

bool ABonusPickup::TryCollect(APawn* collector)
{
    if (isCollecting_ || isCollected_ || IsActorBeingDestroyed() || !IsValid(collector) ||
        !IsValid(Cast<APlayerController>(collector->GetController())) || !IsCollectorInRange(collector))
    {
        return false;
    }

    if (moneyReward_ < 0 || scoreReward_ < 0 || (moneyReward_ == 0 && scoreReward_ == 0))
    {
        UE_LOG(LogBonusPickup, Warning, TEXT("%s: Assign a positive money or score reward without negative values."), *GetName());
        return false;
    }

    AChapter3_ShooterGame_GameMode* gameMode = Cast<AChapter3_ShooterGame_GameMode>(UGameplayStatics::GetGameMode(this));
    if (IsValid(gameMode) && (gameMode->IsGameOver() || gameMode->IsGameCleared()))
    {
        return false;
    }
    if (scoreReward_ > 0 && (!IsValid(gameMode) || gameMode->GetCurrentScore() > MAX_int32 - scoreReward_))
    {
        UE_LOG(LogBonusPickup, Warning, TEXT("%s: Score reward requires an active Chapter3 GameMode and sufficient score capacity."), *GetName());
        return false;
    }

    UChapter3GameInstance* progress = Cast<UChapter3GameInstance>(UGameplayStatics::GetGameInstance(this));
    if (moneyReward_ > 0 && (!IsValid(progress) || !progress->IsProgressReady() || progress->GetMoney() > MAX_int64 - moneyReward_))
    {
        UE_LOG(LogBonusPickup, Warning, TEXT("%s: Money reward requires ready progress data and sufficient balance capacity."), *GetName());
        return false;
    }

    // 저장 및 점수 알림에서 다시 상호작용해도 같은 보상을 중복 지급하지 않는다.
    TGuardValue<bool> collectingGuard(isCollecting_, true);
    const int64 awardedMoney = moneyReward_;
    const int32 awardedScore = scoreReward_;
    if (awardedMoney > 0 && !progress->AddMoney(awardedMoney))
    {
        return false;
    }
    if (awardedScore > 0)
    {
        gameMode->AddScore(awardedScore);
    }

    isCollected_ = true;
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
    OnCollected(collector, awardedMoney, awardedScore);
    Destroy();
    return true;
}
