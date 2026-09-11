#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShootingTarget.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
class UPrimitiveComponent;

// 로컬 -X 방향이 정면인 표적. Point Damage의 실제 명중 위치에 표시를 남긴다.
UCLASS()
class CHAPTER3_SHOOTERGAME_API AShootingTarget : public AActor
{
    GENERATED_BODY()

public:
    AShootingTarget();
    virtual void OnConstruction(const FTransform& transform) override;
    virtual float TakeDamage(float damageAmount, const FDamageEvent& damageEvent,
        AController* eventInstigator, AActor* damageCauser) override;

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Target")
    void ClearHitMarks();

    UFUNCTION(BlueprintPure, Category = "Target")
    int32 GetHitMarkCount() const;

protected:
    virtual void BeginPlay() override;
    virtual bool IsTargetComponent(const UPrimitiveComponent* component) const;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target")
    bool showPaperTarget_ = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target|Assets")
    TObjectPtr<UStaticMesh> boardMeshAsset_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target|Assets")
    TObjectPtr<UStaticMesh> diskMeshAsset_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target|Assets")
    TObjectPtr<UMaterialInterface> targetMaterial_ = nullptr;

private:
    void ApplyTargetAssets();
    void UpdateTargetColors();
    void AddHitMark(const FHitResult& hitResult);
    void SetMeshColor(UStaticMeshComponent* mesh, const FLinearColor& color);

    UPROPERTY(VisibleAnywhere, Category = "Target")
    TObjectPtr<UStaticMeshComponent> targetBoard_ = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Target")
    TArray<TObjectPtr<UStaticMeshComponent>> targetRings_ = {};

    UPROPERTY(VisibleAnywhere, Category = "Target")
    TObjectPtr<UInstancedStaticMeshComponent> hitMarks_ = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Target")
    TObjectPtr<UInstancedStaticMeshComponent> hitMarkOutlines_ = nullptr;

    int32 nextMarkIndex_ = 0;
};
