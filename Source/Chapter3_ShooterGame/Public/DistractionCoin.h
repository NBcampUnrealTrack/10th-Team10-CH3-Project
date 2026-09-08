#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DistractionCoin.generated.h"

class APawn;
class USphereComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UProjectileMovementComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCoinLanded,
    FVector, landingLocation, APawn*, thrower, float, noiseRange);

// 첫 충돌 위치를 알리는 투척물. 적의 반응은 AI 쪽에서 처리한다.
UCLASS()
class CHAPTER3_SHOOTERGAME_API ADistractionCoin : public AActor
{
    GENERATED_BODY()

public:
    ADistractionCoin();
    virtual void OnConstruction(const FTransform& transform) override;

    UFUNCTION(BlueprintCallable, Category = "Coin")
    void LaunchCoin(const FVector& launchVelocity);

    UFUNCTION(BlueprintPure, Category = "Coin")
    float GetCollisionRadius() const;

    UPROPERTY(BlueprintAssignable, Category = "Coin")
    FOnCoinLanded onCoinLanded_;

    static const FName kNoiseTag;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coin|Assets")
    TObjectPtr<UStaticMesh> coinMeshAsset_ = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin")
    TObjectPtr<USphereComponent> collision_ = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin")
    TObjectPtr<UStaticMeshComponent> coinMesh_ = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin")
    TObjectPtr<UProjectileMovementComponent> projectileMovement_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coin", meta = (ClampMin = "0.1", Units = "cm"))
    float coinDiameter_ = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Coin|Distraction", meta = (ClampMin = "0.0", Units = "cm"))
    float noiseRange_ = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Coin|Distraction")
    TObjectPtr<USoundBase> landingSound_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coin", meta = (ClampMin = "0.1", Units = "s"))
    float flightLifeSpan_ = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Coin", meta = (ClampMin = "0.1", Units = "s"))
    float landedLifeSpan_ = 0.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Coin")
    bool hasLanded_ = false;

private:
    void FitCoinMesh();
    void IgnoreThrower();

    UFUNCTION()
    void HandleProjectileStop(const FHitResult& hitResult);

    bool hasLaunched_ = false;
};
