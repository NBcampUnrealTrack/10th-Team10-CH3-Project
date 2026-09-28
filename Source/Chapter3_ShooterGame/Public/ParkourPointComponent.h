#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "ParkourPointComponent.generated.h"

class ACharacter;


UCLASS(ClassGroup = (Parkour), meta = (BlueprintSpawnableComponent))
class CHAPTER3_SHOOTERGAME_API UParkourPointComponent : public UBoxComponent
{
    GENERATED_BODY()

public:
    UParkourPointComponent();


    UFUNCTION(BlueprintPure, Category = "Parkour")
    bool CanBeUsedBy(const ACharacter* character) const;

    UFUNCTION(BlueprintPure, Category = "Parkour")
    FVector GetLaunchDirection() const { return GetForwardVector(); }

   
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parkour", meta = (ClampMin = "0", ClampMax = "180"))
    float MaxActivationAngleDegrees = 60.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parkour", meta = (ClampMin = "0", Units = "cm/s"))
    float LaunchForwardSpeed = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parkour", meta = (ClampMin = "0", Units = "cm/s"))
    float LaunchUpwardSpeed = 400.f;
};
