#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "InputActionValue.h"
#include "Chapter3_ShooterGame_PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;

class UEnhancedInputLocalPlayerSubsystem;
class UEnhancedInputComponent;
class AM1911WeaponView;
class ADistractionCoin;
class USlowMotionSkillComponent;
class UAssassinationTargetComponent;


USTRUCT()
struct FWeaponViewSlotState
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TSubclassOf<AM1911WeaponView> weaponClass_ = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AM1911WeaponView> instance_ = nullptr;

	int32 ammo_ = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoinThrown, ADistractionCoin*, coin);

UCLASS()
class CHAPTER3_SHOOTERGAME_API AChapter3_ShooterGame_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AChapter3_ShooterGame_PlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);

	void HandleSprintStart(const FInputActionValue& Value);
	void HandleSprintStop(const FInputActionValue& Value);

	void HandleCrouchStart(const FInputActionValue& Value);
	void HandleCrouchStop(const FInputActionValue& Value);

	void HandleParkour(const FInputActionValue& Value);
	void HandleInteract(const FInputActionValue& Value);

	void HandleFireStart(const FInputActionValue& Value);
	void HandleFireStop(const FInputActionValue& Value);

	void HandleAimStart(const FInputActionValue& Value);
	void HandleAimStop(const FInputActionValue& Value);

	void HandleSkill1(const FInputActionValue& Value);
	void HandleSkill2(const FInputActionValue& Value);

	void HandleLean(const FInputActionValue& Value);

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

    // 휠 순서대로 총 BP를 지정한다. 비어 있으면 기존 Weapon View Class 하나를 사용한다.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooting|Weapon")
    TArray<TSubclassOf<AM1911WeaponView>> weaponViewClasses_ = {};

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


public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputMappingContext* InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* ParkourAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* Skill1Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* Skill2Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* LeanAction;

    UFUNCTION(BlueprintCallable, Category = "Shooting")
    void Fire();

    UFUNCTION(BlueprintCallable, Category = "Shooting|Magazine")
    void StartReload();

    UFUNCTION(BlueprintCallable, Category = "Shooting|Weapon")
    void NextWeapon();

    UFUNCTION(BlueprintCallable, Category = "Shooting|Weapon")
    void PreviousWeapon();

    UFUNCTION(BlueprintCallable, Category = "Shooting|Weapon")
    bool EquipWeaponAtIndex(int32 weaponIndex);

    UFUNCTION(BlueprintPure, Category = "Shooting|Weapon")
    AM1911WeaponView* GetCurrentWeapon() const
    {
        return weaponView_;
    }

    UFUNCTION(BlueprintPure, Category = "Shooting|Weapon")
    int32 GetEquippedWeaponIndex() const
    {
        return equippedWeaponIndex_;
    }

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

private:
    void InitializeWeaponInventory();
    void CycleWeapon(int32 direction);
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

    UPROPERTY(Transient)
    TArray<FWeaponViewSlotState> weaponSlots_ = {};

    int32 equippedWeaponIndex_ = INDEX_NONE;
    bool weaponInventoryInitialized_ = false;
    bool switchingWeapon_ = false;
};
