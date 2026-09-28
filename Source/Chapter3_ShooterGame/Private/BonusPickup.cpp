#include "BonusPickup.h"

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

    if (scoreReward_ <= 0)
    {
        UE_LOG(LogBonusPickup, Warning, TEXT("%s: Assign a positive score reward."), *GetName());
        return false;
    }

    AChapter3_ShooterGame_GameMode* gameMode = Cast<AChapter3_ShooterGame_GameMode>(UGameplayStatics::GetGameMode(this));
    if (!IsValid(gameMode) || gameMode->IsGameOver() || gameMode->IsGameCleared())
    {
        return false;
    }

    // 저장 및 점수 알림에서 다시 상호작용해도 같은 보상을 중복 지급하지 않는다.
    TGuardValue<bool> collectingGuard(isCollecting_, true);
    const int32 awardedScore = scoreReward_;

    // GameMode의 파밍 추가 점수로 통합 더하기
    gameMode->AddFarmingReward(awardedScore);

    isCollected_ = true;
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);

    OnCollected(collector, awardedScore);
    Destroy();
    return true;
}