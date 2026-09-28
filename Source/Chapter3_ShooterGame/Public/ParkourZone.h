// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParkourZone.generated.h"

class UBoxComponent;

UCLASS()
class CHAPTER3_SHOOTERGAME_API AParkourZone : public AActor
{
    GENERATED_BODY()

public:
    AParkourZone();

    UFUNCTION(BlueprintCallable, Category = "Parkour")
    void ActivateParkour(AActor* PlayerActor);

protected:
   
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour")
    TObjectPtr<UBoxComponent> ParkourBox;
};