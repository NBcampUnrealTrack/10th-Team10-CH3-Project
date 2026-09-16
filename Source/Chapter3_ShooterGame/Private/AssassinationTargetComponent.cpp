#include "AssassinationTargetComponent.h"

#include "AIController.h"
#include "BrainComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

UAssassinationTargetComponent::UAssassinationTargetComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UAssassinationTargetComponent::IsAssassinated() const
{
    return assassinated_;
}

bool UAssassinationTargetComponent::CanBeAssassinatedBy(const APawn* instigatorPawn) const
{
    const AActor* owner = GetOwner();
    const UWorld* world = GetWorld();
    if (!assassinationEnabled_ || assassinated_ || !IsValid(owner) || !IsValid(instigatorPawn)
        || owner == instigatorPawn || !world || world != instigatorPawn->GetWorld() || world->IsPaused()
        || !FMath::IsFinite(assassinationDistance_) || !FMath::IsFinite(rearHalfAngle_)
        || !FMath::IsFinite(maxHeightDifference_))
    {
        return false;
    }

    const FVector toPlayer = instigatorPawn->GetActorLocation() - GetComponentLocation();
    const float distance = FMath::Max(0.0f, assassinationDistance_);
    if (toPlayer.SizeSquared() > FMath::Square(distance)
        || FMath::Abs(toPlayer.Z) > FMath::Max(0.0f, maxHeightDifference_))
    {
        return false;
    }

    const FVector directionToPlayer = toPlayer.GetSafeNormal2D();
    const FVector backwardDirection = -GetForwardVector().GetSafeNormal2D();
    const float halfAngle = FMath::Clamp(rearHalfAngle_, 0.0f, 89.0f);
    const float minimumDot = FMath::Cos(FMath::DegreesToRadians(halfAngle));
    if (directionToPlayer.IsNearlyZero()
        || FVector::DotProduct(backwardDirection, directionToPlayer) < minimumDot)
    {
        return false;
    }

    FCollisionQueryParams queryParams(SCENE_QUERY_STAT(AssassinationVisibility), false);
    queryParams.AddIgnoredActor(owner);
    queryParams.AddIgnoredActor(instigatorPawn);
    TArray<AActor*> attachedActors = {};
    instigatorPawn->GetAttachedActors(attachedActors, true, true);
    queryParams.AddIgnoredActors(attachedActors);
    attachedActors.Reset();
    owner->GetAttachedActors(attachedActors, true, true);
    queryParams.AddIgnoredActors(attachedActors);
    return !world->LineTraceTestByChannel(instigatorPawn->GetActorLocation(),
        GetComponentLocation(), ECC_Visibility, queryParams);
}

bool UAssassinationTargetComponent::TryAssassinate(APawn* instigatorPawn)
{
    AActor* owner = GetOwner();
    if (!IsValid(owner) || !owner->HasAuthority() || !CanBeAssassinatedBy(instigatorPawn))
    {
        return false;
    }

    assassinated_ = true;
    StopOwnerGameplay();
    onAssassinated_.Broadcast(instigatorPawn);
    if (destroyOwnerOnAssassination_ && IsValid(owner))
    {
        owner->Destroy();
    }
    return true;
}

void UAssassinationTargetComponent::StopOwnerGameplay()
{
    AActor* owner = GetOwner();
    owner->SetCanBeDamaged(false);
    owner->SetActorEnableCollision(false);
    owner->SetActorTickEnabled(false);
    GetWorld()->GetTimerManager().ClearAllTimersForObject(owner);

    if (ACharacter* character = Cast<ACharacter>(owner))
    {
        character->GetCharacterMovement()->StopMovementImmediately();
        character->GetCharacterMovement()->DisableMovement();
    }
    if (APawn* pawn = Cast<APawn>(owner))
    {
        if (AAIController* controller = Cast<AAIController>(pawn->GetController()))
        {
            controller->StopMovement();
            controller->ClearFocus(EAIFocusPriority::Gameplay);
            if (UBrainComponent* brain = controller->GetBrainComponent())
            {
                brain->StopLogic(TEXT("BackAttack"));
            }
            pawn->DetachFromControllerPendingDestroy();
        }
    }
}
