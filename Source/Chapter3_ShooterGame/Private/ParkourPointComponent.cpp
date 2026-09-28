#include "ParkourPointComponent.h"

#include "GameFramework/Character.h"

UParkourPointComponent::UParkourPointComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    InitBoxExtent(FVector(100.f, 150.f, 100.f)); // 박스 배치후 조절

   
    SetCollisionProfileName(TEXT("Trigger"));
    SetGenerateOverlapEvents(true);

    ShapeColor = FColor::Green; 
}

bool UParkourPointComponent::CanBeUsedBy(const ACharacter* character) const
{
    if (!IsValid(character) || MaxActivationAngleDegrees >= 180.f)
    {
        return IsValid(character);
    }

    const FVector directionToPoint = ((GetComponentLocation() - character->GetActorLocation()) * FVector(1, 1, 0)).GetSafeNormal();
    if (directionToPoint.IsNearlyZero())
    {
        return true; 
    }

    const FVector characterForwardFlat = (character->GetActorForwardVector() * FVector(1, 1, 0)).GetSafeNormal();
    const float dot = FVector::DotProduct(characterForwardFlat, directionToPoint);
    const float angleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(dot, -1.f, 1.f)));

    return angleDegrees <= MaxActivationAngleDegrees;
}
