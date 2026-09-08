#pragma once

#include "CoreMinimal.h"
#include "ShootingTarget.h"
#include "TrainingDummyTarget.generated.h"

UCLASS()
class CHAPTER3_SHOOTERGAME_API ATrainingDummyTarget : public AShootingTarget
{
    GENERATED_BODY()

public:
    ATrainingDummyTarget();
    virtual void OnConstruction(const FTransform& transform) override;

protected:
    virtual void BeginPlay() override;
    virtual bool IsTargetComponent(const UPrimitiveComponent* component) const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target|Dummy")
    TObjectPtr<UStaticMeshComponent> dummyMesh_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target|Dummy", meta = (ClampMin = "1.0", Units = "cm"))
    float dummyHeight_ = 0.0f;

private:
    void FitDummyMesh();
};
