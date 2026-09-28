#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TeleportElevator.generated.h"

class UBoxComponent;
class AKeyCard;
class ATargetPoint;
class APlayerController;

UCLASS()
class CHAPTER3_SHOOTERGAME_API ATeleportElevator : public AActor
{
    GENERATED_BODY()

public:
    ATeleportElevator();

protected:
    virtual void BeginPlay() override;

    virtual void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

private:
   
    void TryTeleport();

    
    bool bIsTeleporting = false;

  
    UPROPERTY(EditAnywhere, Category = "Elevator|Fade",
        meta = (ClampMin = "0.01"))
    float FadeDuration = 0.3f;

  
    FTimerHandle TeleportTimerHandle;
    FTimerHandle FadeFinishTimerHandle;

    void ExecuteTeleport();

 
    void FinishTeleport();
    UPROPERTY(VisibleAnywhere, Category = "Elevator")
    TObjectPtr<UBoxComponent> InteractionBox;

    
    UPROPERTY(EditInstanceOnly, Category = "Elevator")
    TObjectPtr<AKeyCard> RequiredCard;

   
    UPROPERTY(EditInstanceOnly, Category = "Elevator")
    TObjectPtr<ATargetPoint> TeleportDestination;

    UPROPERTY(Transient)
    TObjectPtr<APlayerController> LocalController;
};