#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "AssassinationTargetComponent.generated.h"

class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssassinated, APawn*, instigatorPawn);

UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class CHAPTER3_SHOOTERGAME_API UAssassinationTargetComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UAssassinationTargetComponent();

    UFUNCTION(BlueprintPure, Category = "BackAttack")
    bool CanBeAssassinatedBy(const APawn* instigatorPawn) const;

    UFUNCTION(BlueprintCallable, Category = "BackAttack")
    bool TryAssassinate(APawn* instigatorPawn);

    UFUNCTION(BlueprintPure, Category = "BackAttack")
    bool IsAssassinated() const;

    UPROPERTY(BlueprintAssignable, Category = "BackAttack")
    FOnAssassinated onAssassinated_;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackAttack")
    bool assassinationEnabled_ = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackAttack", meta = (ClampMin = "0.0", Units = "cm"))
    float assassinationDistance_ = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackAttack", meta = (ClampMin = "0.0", ClampMax = "89.0", Units = "deg"))
    float rearHalfAngle_ = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackAttack", meta = (ClampMin = "0.0", Units = "cm"))
    float maxHeightDifference_ = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BackAttack")
    bool destroyOwnerOnAssassination_ = true;

private:
    void StopOwnerGameplay();

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "BackAttack", meta = (AllowPrivateAccess = "true"))
    bool assassinated_ = false;
};
