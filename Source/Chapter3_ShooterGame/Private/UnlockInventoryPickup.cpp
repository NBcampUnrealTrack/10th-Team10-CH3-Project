#include "UnlockInventoryPickup.h"

#include "UnlockInventoryComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogUnlockPickup, Log, All);

AUnlockInventoryPickup::AUnlockInventoryPickup()
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

void AUnlockInventoryPickup::OnConstruction(const FTransform& transform)
{
    Super::OnConstruction(transform);
    constexpr float kMinimumRadius = 1.0f;
    const float safeRadius = FMath::IsFinite(pickupRadius_) ? FMath::Max(kMinimumRadius, pickupRadius_) : kMinimumRadius;
    pickupSphere_->SetSphereRadius(safeRadius);
}

void AUnlockInventoryPickup::BeginPlay()
{
    Super::BeginPlay();
    if (itemId_.IsNone())
    {
        UE_LOG(LogUnlockPickup, Warning, TEXT("%s: Assign Item ID matching the player's Unlock Inventory Item Definitions."), *GetName());
    }
}

bool AUnlockInventoryPickup::TryCollect(APawn* collector)
{
    if (isCollecting_ || isCollected_ || IsActorBeingDestroyed() || !IsValid(collector) || itemId_.IsNone())
    {
        return false;
    }

    if (!IsValid(pickupSphere_) || !pickupSphere_->IsOverlappingActor(collector))
    {
        return false;
    }

    APlayerController* playerController = Cast<APlayerController>(collector->GetController());
    if (!IsValid(playerController))
    {
        return false;
    }

    UUnlockInventoryComponent* inventory = playerController->FindComponentByClass<UUnlockInventoryComponent>();
    if (!IsValid(inventory))
    {
        UE_LOG(LogUnlockPickup, Warning, TEXT("%s: Add UnlockInventoryComponent to the player's controller."), *GetName());
        return false;
    }

    // 해금 이벤트가 이 Actor를 다시 호출하는 경우의 중복 처리를 막는다.
    TGuardValue<bool> collectingGuard(isCollecting_, true);
    const EUnlockInventoryResult result = inventory->UnlockItem(itemId_);
    if (result == EUnlockInventoryResult::InvalidItem)
    {
        UE_LOG(LogUnlockPickup, Warning, TEXT("%s: Item ID '%s' is not registered in the player's Item Definitions."), *GetName(), *itemId_.ToString());
        return false;
    }

    isCollected_ = true;
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
    OnCollected(collector, result == EUnlockInventoryResult::Unlocked);
    Destroy();
    return true;
}
