#pragma once

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"

namespace Chapter3PickupRange
{
// Query geometry directly: collection must not depend on overlap-event settings.
inline bool Contains(const USphereComponent* sphere, const APawn* collector)
{
    if (!IsValid(sphere) || !IsValid(collector)) return false;

    const FVector center = sphere->GetComponentLocation();
    double radius = sphere->GetScaledSphereRadius();
    FVector closestPoint = collector->GetActorLocation();
    if (const ACharacter* character = Cast<ACharacter>(collector))
    {
        const UCapsuleComponent* capsule = character->GetCapsuleComponent();
        const FVector offset = capsule->GetUpVector() * FMath::Max(0.0f,
            capsule->GetScaledCapsuleHalfHeight() - capsule->GetScaledCapsuleRadius());
        closestPoint = FMath::ClosestPointOnSegment(center,
            capsule->GetComponentLocation() - offset, capsule->GetComponentLocation() + offset);
        radius += capsule->GetScaledCapsuleRadius();
    }
    return FVector::DistSquared(center, closestPoint) <= FMath::Square(radius);
}
}
