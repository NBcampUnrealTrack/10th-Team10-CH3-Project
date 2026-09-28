#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KeyCard.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class APlayerController;

UCLASS()
class CHAPTER3_SHOOTERGAME_API AKeyCard : public AActor
{
    GENERATED_BODY()

public:
    AKeyCard();

    UFUNCTION(BlueprintPure, Category = "Card")
    bool IsCollected() const
    {
        return bCollected;
    }

protected:
    virtual void BeginPlay() override;

    virtual void EndPlay(
        const EEndPlayReason::Type EndPlayReason) override;

private:
  
    void TryCollect();

   
    UPROPERTY(VisibleAnywhere, Category = "Card")
    TObjectPtr<UBoxComponent> InteractionBox;

  
    UPROPERTY(VisibleAnywhere, Category = "Card")
    TObjectPtr<UStaticMeshComponent> CardMesh;

  
    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Card",
        meta = (AllowPrivateAccess = "true"))
    bool bCollected = false;

    UPROPERTY(Transient)
    TObjectPtr<APlayerController> LocalController;
};