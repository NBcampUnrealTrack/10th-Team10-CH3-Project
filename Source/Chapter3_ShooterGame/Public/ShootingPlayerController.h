// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "ShootingPlayerController.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;
class AM1911WeaponView;
class ADistractionCoin;
class USlowMotionSkillComponent;
class UAssassinationTargetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoinThrown, ADistractionCoin*, coin);

// 화면 중앙을 기준으로 단발 사격을 처리하는 플레이어 컨트롤러.
UCLASS()
class CHAPTER3_SHOOTERGAME_API AShootingPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
    AShootingPlayerController();

    UFUNCTION(BlueprintCallable, Category = "Shooting")
    void Fire();

    UFUNCTION(BlueprintCallable, Category = "Shooting|Magazine")
    void StartReload();

    UFUNCTION(BlueprintPure, Category = "Shooting|Magazine")
    int32 GetCurrentAmmo() const
    {
        return currentAmmo_;
    }

    UFUNCTION(BlueprintPure, Category = "Shooting|Magazine")
    int32 GetMagazineCapacity() const
    {
        return FMath::Max(1, magazineCapacity_);
    }

    UFUNCTION(BlueprintPure, Category = "Shooting|Magazine")
    bool IsReloading() const
    {
        return reloading_;
    }

    UFUNCTION(BlueprintPure, Category = "Shooting|Magazine")
    float GetReloadProgress() const;

    UFUNCTION(BlueprintCallable, Category = "Shooting")
    void StartAiming();

    UFUNCTION(BlueprintCallable, Category = "Shooting")
    void StopAiming();

    UFUNCTION(BlueprintCallable, Category = "Coin")
    void ThrowCoin();

    UFUNCTION(BlueprintCallable, Category = "Skill|Slow Motion")
    bool TryActivateSlowMotion();

    UFUNCTION(BlueprintCallable, Category = "BackAttack")
    bool TryAssassinate();

    UFUNCTION(BlueprintPure, Category = "BackAttack")
    UAssassinationTargetComponent* FindAssassinationTarget() const;

    // 생성 직후 호출된다. 여기서 coin의 onCoinLanded_에 바인딩할 수 있다.
    UPROPERTY(BlueprintAssignable, Category = "Coin")
    FOnCoinThrown onCoinThrown_;

    virtual void UpdateCameraManager(float deltaSeconds) override;
    virtual void FlushPressedKeys() override;

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shooting", meta = (ClampMin = "0.0", Units = "cm"))
    float fireRange_ = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shooting", meta = (ClampMin = "0.0"))
    float damage_ = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shooting", meta = (ClampMin = "0.0", Units = "s"))
    float fireInterval_ = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooting|Magazine", meta = (ClampMin = "1", UIMin = "1"))
    int32 magazineCapacity_ = 10;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooting|Magazine", meta = (ClampMin = "0.2", Units = "s"))
    float reloadDuration_ = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shooting|Debug")
    bool drawDebugShot_ = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooting|Weapon")
    TSubclassOf<AM1911WeaponView> weaponViewClass_ = nullptr;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shooting|Weapon")
    TObjectPtr<AM1911WeaponView> weaponView_ = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skills")
    TObjectPtr<USlowMotionSkillComponent> slowMotionSkill_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Coin")
    TSubclassOf<ADistractionCoin> coinClass_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Coin", meta = (ClampMin = "0.0", Units = "cm/s"))
    float coinThrowSpeed_ = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Coin", meta = (ClampMin = "0.0", Units = "cm/s"))
    float coinUpwardSpeed_ = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Coin", meta = (ClampMin = "0.0", Units = "s"))
    float coinThrowInterval_ = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> fireAction_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> reloadAction_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> aimAction_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> throwCoinAction_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> assassinationAction_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> inputMappingContext_ = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon View|Recoil", meta = (ClampMin = "0.01", Units = "s"))
    float reloadDelay_ = 0.1f;

    FTimerHandle reloadDelayTimer_ = {};

private:
    void HandleAssassinationInput();
    void QueueAutomaticReload();
    void FinishReload();
    void BindGameplayInput(UEnhancedInputComponent* enhancedInput);
    void ApplyShotDamage(const FHitResult& hitResult, const FVector& shotDirection);
    void DrawShotDebug(const FVector& start, const FVector& end, const FHitResult& hitResult);

    UPROPERTY(Transient)
    TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> inputSubsystem_ = nullptr;

    double nextFireTime_ = 0.0;
    double nextCoinThrowTime_ = 0.0;
    bool aimHeld_ = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shooting|Magazine", meta = (AllowPrivateAccess = "true"))
    int32 currentAmmo_ = 0;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shooting|Magazine", meta = (AllowPrivateAccess = "true"))
    bool reloading_ = false;

    FTimerHandle reloadTimer_ = {};
};
