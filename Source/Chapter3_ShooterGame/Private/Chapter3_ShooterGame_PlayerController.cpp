#include "Chapter3_ShooterGame_PlayerController.h"
#include "Chapter3_ShooterGame_Character.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "M1911WeaponView.h"
#include "DistractionCoin.h"
#include "SlowMotionSkillComponent.h"
#include "AssassinationTargetComponent.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogShooting, Log, All);

namespace
{
    constexpr float kDefaultFireRange = 10000.0f;
    constexpr float kDefaultDamage = 20.0f;
    constexpr float kDefaultFireInterval = 0.2f;
    constexpr float kDefaultReloadDuration = 1.8f;
    constexpr float kMinimumReloadDuration = 0.2f;
    constexpr int32 kFireMappingPriority = 1;
    constexpr float kDebugDuration = 1.0f;
    constexpr float kDebugLineThickness = 1.0f;
    constexpr float kDebugHitRadius = 8.0f;
    constexpr int32 kDebugSphereSegments = 12;
    constexpr float kFirstPersonScale = 0.8f;
    constexpr float kDefaultCoinThrowSpeed = 1000.0f;
    constexpr float kDefaultCoinUpwardSpeed = 300.0f;
    constexpr float kDefaultCoinThrowInterval = 0.6f;
}

AChapter3_ShooterGame_PlayerController::AChapter3_ShooterGame_PlayerController()
	: inputMappingContext_(nullptr)
	, MoveAction(nullptr)
	, LookAction(nullptr)
	, SprintAction(nullptr)
	, CrouchAction(nullptr)
	, ParkourAction(nullptr)
	, InteractAction(nullptr)
	, FireAction(nullptr)
	, AimAction(nullptr)
	, Skill1Action(nullptr)
	, Skill2Action(nullptr)
	, LeanAction(nullptr)
{
	OverridePlayerInputClass = UEnhancedPlayerInput::StaticClass();
	fireRange_ = kDefaultFireRange;
	damage_ = kDefaultDamage;
	fireInterval_ = kDefaultFireInterval;
	reloadDuration_ = kDefaultReloadDuration;
	coinThrowSpeed_ = kDefaultCoinThrowSpeed;
	coinUpwardSpeed_ = kDefaultCoinUpwardSpeed;
	coinThrowInterval_ = kDefaultCoinThrowInterval;
	slowMotionSkill_ = CreateDefaultSubobject<USlowMotionSkillComponent>(TEXT("SlowMotionSkill"));
    coinThrowSkill_ = CreateDefaultSubobject<UCoinThrowSkillComponent>(TEXT("CoinThrowSkill"));

};

void AChapter3_ShooterGame_PlayerController::BeginPlay()
{
    coinThrowSkill_->InitializeFromLegacySettings(coinClass_, coinThrowSpeed_, coinUpwardSpeed_, coinThrowInterval_);
    coinThrowSkill_->onCoinThrown_.AddUniqueDynamic(this, &AChapter3_ShooterGame_PlayerController::ForwardCoinThrown);

    magazineCapacity_ = GetMagazineCapacity();
    currentAmmo_ = magazineCapacity_;
    InitializeWeaponInventory();

	Super::BeginPlay();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (inputMappingContext_)
			{
				Subsystem->AddMappingContext(inputMappingContext_, 0);
			}
		}
	}
}

void AChapter3_ShooterGame_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AChapter3_ShooterGame_PlayerController::HandleMove);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AChapter3_ShooterGame_PlayerController::HandleLook);

		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AChapter3_ShooterGame_PlayerController::HandleSprintStart);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AChapter3_ShooterGame_PlayerController::HandleSprintStop);

		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AChapter3_ShooterGame_PlayerController::HandleCrouchStart);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AChapter3_ShooterGame_PlayerController::HandleCrouchStop);

		EnhancedInputComponent->BindAction(ParkourAction, ETriggerEvent::Started, this, &AChapter3_ShooterGame_PlayerController::HandleParkour);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AChapter3_ShooterGame_PlayerController::HandleInteract);

		EnhancedInputComponent->BindAction(LeanAction, ETriggerEvent::Triggered, this, &AChapter3_ShooterGame_PlayerController::HandleLean);
		EnhancedInputComponent->BindAction(LeanAction, ETriggerEvent::Completed, this, &AChapter3_ShooterGame_PlayerController::HandleLean);
	}

    // 프로젝트의 전역 입력 설정과 관계없이 이 컨트롤러는 Enhanced Input을 사용한다.
    if (!InputComponent)
    {
        InputComponent = NewObject<UEnhancedInputComponent>(this, TEXT("ShootingInputComponent"));
        InputComponent->RegisterComponent();
    }

    UEnhancedInputComponent* enhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
    if (!enhancedInput)
    {
        UE_LOG(LogShooting, Error, TEXT("Set Default Input Component Class to EnhancedInputComponent in Project Settings > Input."));
        return;
    }

    ULocalPlayer* localPlayer = GetLocalPlayer();
    if (!localPlayer)
    {
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* subsystem = localPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    if (!subsystem)
    {
        return;
    }

    if (!inputMappingContext_)
    {
        UE_LOG(LogShooting, Warning, TEXT("Assign the Input Mapping Context in the controller Blueprint."));
        return;
    }

    BindGameplayInput(enhancedInput);

    // 다른 이동/시점 매핑은 유지하고 사격용 매핑만 등록한다.
    subsystem->AddMappingContext(inputMappingContext_, kFireMappingPriority);
    inputSubsystem_ = subsystem;
}

void AChapter3_ShooterGame_PlayerController::HandleMove(const FInputActionValue& Value)
{
	if (AChapter3_ShooterGame_Character* Char = GetPawn<AChapter3_ShooterGame_Character>())
	{
		Char->Move(Value.Get<FVector2D>());
	}
}

void AChapter3_ShooterGame_PlayerController::HandleLook(const FInputActionValue& Value)
{
	if (AChapter3_ShooterGame_Character* Char = GetPawn<AChapter3_ShooterGame_Character>())
	{
		Char->Look(Value.Get<FVector2D>());
	}
}

void AChapter3_ShooterGame_PlayerController::HandleSprintStart(const FInputActionValue& Value)
{
	if (AChapter3_ShooterGame_Character* Char = GetPawn<AChapter3_ShooterGame_Character>())
	{
		Char->SetSprinting(true);
	}
}

void AChapter3_ShooterGame_PlayerController::HandleSprintStop(const FInputActionValue& Value)
{
	if (AChapter3_ShooterGame_Character* Char = GetPawn<AChapter3_ShooterGame_Character>())
	{
		Char->SetSprinting(false);
	}
}

void AChapter3_ShooterGame_PlayerController::HandleCrouchStart(const FInputActionValue& Value)
{
	if (AChapter3_ShooterGame_Character* Char = GetPawn<AChapter3_ShooterGame_Character>())
	{
		Char->SetCrouching(true);
	}
}

void AChapter3_ShooterGame_PlayerController::HandleCrouchStop(const FInputActionValue& Value)
{
	if (AChapter3_ShooterGame_Character* Char = GetPawn<AChapter3_ShooterGame_Character>())
	{
		Char->SetCrouching(false);
	}
}

void AChapter3_ShooterGame_PlayerController::HandleParkour(const FInputActionValue& Value)
{
	if (AChapter3_ShooterGame_Character* Char = GetPawn<AChapter3_ShooterGame_Character>())
	{
		Char->TryParkour();
	}
}

void AChapter3_ShooterGame_PlayerController::HandleInteract(const FInputActionValue& Value)
{
	if (AChapter3_ShooterGame_Character* Char = GetPawn<AChapter3_ShooterGame_Character>())
	{
		Char->Interact();
	}
}

void AChapter3_ShooterGame_PlayerController::HandleFireStart(const FInputActionValue& Value)
{
	if (AChapter3_ShooterGame_Character* Char = GetPawn<AChapter3_ShooterGame_Character>())
	{
		Char->StartFire();
	}
}

void AChapter3_ShooterGame_PlayerController::HandleFireStop(const FInputActionValue& Value)
{
	if (AChapter3_ShooterGame_Character* Char = GetPawn<AChapter3_ShooterGame_Character>())
	{
		Char->StopFire();
	}
}

void AChapter3_ShooterGame_PlayerController::HandleAimStart(const FInputActionValue& Value)
{
	if (AChapter3_ShooterGame_Character* Char = GetPawn<AChapter3_ShooterGame_Character>())
	{
		Char->SetAiming(true);
	}
}

void AChapter3_ShooterGame_PlayerController::HandleAimStop(const FInputActionValue& Value)
{
	if (AChapter3_ShooterGame_Character* Char = GetPawn<AChapter3_ShooterGame_Character>())
	{
		Char->SetAiming(false);
	}
}

void AChapter3_ShooterGame_PlayerController::HandleSkill1(const FInputActionValue& Value)
{
	if (AChapter3_ShooterGame_Character* Char = GetPawn<AChapter3_ShooterGame_Character>())
	{
		Char->UseSkill1();
	}
}

void AChapter3_ShooterGame_PlayerController::HandleSkill2(const FInputActionValue& Value)
{
	if (AChapter3_ShooterGame_Character* Char = GetPawn<AChapter3_ShooterGame_Character>())
	{
		Char->UseSkill2();
	}
}

void AChapter3_ShooterGame_PlayerController::HandleLean(const FInputActionValue& Value)
{
	if (AChapter3_ShooterGame_Character* Char = GetPawn<AChapter3_ShooterGame_Character>())
	{
		Char->SetLean(Value.Get<float>());
	}
}


bool AChapter3_ShooterGame_PlayerController::TryActivateSlowMotion()
{
    return IsLocalController() && IsValid(GetPawn()) && slowMotionSkill_
        && slowMotionSkill_->TryActivateSlowMotion();
}

void AChapter3_ShooterGame_PlayerController::BindGameplayInput(UEnhancedInputComponent* enhancedInput)
{
    if (reloadAction_)
    {
        enhancedInput->BindAction(reloadAction_, ETriggerEvent::Started, this,
            &AChapter3_ShooterGame_PlayerController::StartReload);
    }
    else
    {
        UE_LOG(LogShooting, Warning, TEXT("컨트롤러 Blueprint에 장전 Input Action을 지정하세요."));
    }

    if (assassinationAction_)
    {
        enhancedInput->BindAction(assassinationAction_, ETriggerEvent::Started, this,
            &AChapter3_ShooterGame_PlayerController::HandleAssassinationInput);
    }
    else
    {
        UE_LOG(LogShooting, Warning, TEXT("컨트롤러 Blueprint에 암살 Input Action을 지정하세요."));
    }

    if (fireAction_)
    {
        enhancedInput->BindAction(fireAction_, ETriggerEvent::Started, this, &AChapter3_ShooterGame_PlayerController::Fire);
    }

    if (aimAction_)
    {
        enhancedInput->BindAction(aimAction_, ETriggerEvent::Started, this, &AChapter3_ShooterGame_PlayerController::StartAiming);
        enhancedInput->BindAction(aimAction_, ETriggerEvent::Completed, this, &AChapter3_ShooterGame_PlayerController::StopAiming);
        enhancedInput->BindAction(aimAction_, ETriggerEvent::Canceled, this, &AChapter3_ShooterGame_PlayerController::StopAiming);
    }

    if (throwCoinAction_)
    {
        enhancedInput->BindAction(throwCoinAction_, ETriggerEvent::Started, this, &AChapter3_ShooterGame_PlayerController::ThrowCoin);
    }
}

void AChapter3_ShooterGame_PlayerController::InitializeWeaponInventory()
{
    if (weaponInventoryInitialized_)
    {
        return;
    }
    weaponInventoryInitialized_ = true;
    TArray<TSubclassOf<AM1911WeaponView>> classes = weaponViewClasses_;
    if (classes.IsEmpty() && weaponViewClass_)
    {
        classes.Add(weaponViewClass_);
    }
    for (TSubclassOf<AM1911WeaponView> weaponClass : classes)
    {
        if (!weaponClass || weaponClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
        {
            continue;
        }
        FWeaponViewSlotState slot = {};
        slot.weaponClass_ = weaponClass;
        slot.ammo_ = GetMagazineCapacity();
        weaponSlots_.Add(slot);
        if (equippedWeaponIndex_ == INDEX_NONE && weaponClass == weaponViewClass_)
        {
            equippedWeaponIndex_ = weaponSlots_.Num() - 1;
        }
    }
    if (equippedWeaponIndex_ == INDEX_NONE && !weaponSlots_.IsEmpty())
    {
        equippedWeaponIndex_ = 0;
    }
}

void AChapter3_ShooterGame_PlayerController::NextWeapon()
{
    CycleWeapon(1);
}

void AChapter3_ShooterGame_PlayerController::PreviousWeapon()
{
    CycleWeapon(-1);
}

void AChapter3_ShooterGame_PlayerController::CycleWeapon(int32 direction)
{
    InitializeWeaponInventory();
    const int32 count = weaponSlots_.Num();
    if (count < 2)
    {
        return;
    }
    // 생성할 수 없는 슬롯은 건너뛰되 현재 총은 그대로 유지한다.
    for (int32 step = 1; step < count; ++step)
    {
        const int32 nextIndex = (equippedWeaponIndex_ + direction * step + count) % count;
        if (EquipWeaponAtIndex(nextIndex))
        {
            return;
        }
    }
}

bool AChapter3_ShooterGame_PlayerController::EquipWeaponAtIndex(int32 weaponIndex)
{
    UWorld* world = GetWorld();
    APawn* controlledPawn = GetPawn();
    if (switchingWeapon_ || !world || world->IsPaused() || !IsLocalController() || !IsValid(controlledPawn))
    {
        return false;
    }
    InitializeWeaponInventory();
    if (!weaponSlots_.IsValidIndex(weaponIndex)
        || (weaponIndex == equippedWeaponIndex_ && IsValid(weaponView_)))
    {
        return false;
    }
    TGuardValue<bool> switchGuard(switchingWeapon_, true);
    FWeaponViewSlotState& nextSlot = weaponSlots_[weaponIndex];
    if (!IsValid(nextSlot.instance_))
    {
        FActorSpawnParameters spawnParams = {};
        spawnParams.Owner = controlledPawn;
        spawnParams.Instigator = controlledPawn;
        spawnParams.ObjectFlags |= RF_Transient;
        spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        const FTransform spawnTransform = IsValid(weaponView_)
            ? weaponView_->GetActorTransform() : controlledPawn->GetActorTransform();
        nextSlot.instance_ = world->SpawnActor<AM1911WeaponView>(nextSlot.weaponClass_, spawnTransform, spawnParams);
        if (!IsValid(nextSlot.instance_))
        {
            return false;
        }
    }

    if (weaponSlots_.IsValidIndex(equippedWeaponIndex_))
    {
        weaponSlots_[equippedWeaponIndex_].ammo_ = currentAmmo_;
    }
    // 이전 총의 장전 타이머가 새 총의 탄약을 채우지 않게 취소한다.
    GetWorldTimerManager().ClearTimer(reloadTimer_);
    GetWorldTimerManager().ClearTimer(reloadDelayTimer_);
    reloading_ = false;
    if (IsValid(weaponView_))
    {
        weaponView_->SetWeaponEquipped(false);
    }
    equippedWeaponIndex_ = weaponIndex;
    weaponView_ = nextSlot.instance_;
    coinThrowSkill_->SetIgnoredWeapon(weaponView_);
    currentAmmo_ = FMath::Clamp(nextSlot.ammo_, 0, GetMagazineCapacity());
    weaponView_->SetOwner(controlledPawn);
    weaponView_->SetInstigator(controlledPawn);
    weaponView_->SetWeaponEquipped(true);
    if (PlayerCameraManager)
    {
        const FMinimalViewInfo& viewInfo = PlayerCameraManager->GetCameraCacheView();
        weaponView_->UpdateView(0.0f, viewInfo.Location, viewInfo.Rotation, aimHeld_);
    }
    return true;
}

UAssassinationTargetComponent* AChapter3_ShooterGame_PlayerController::FindAssassinationTarget() const
{
    const APawn* controlledPawn = GetPawn();
    if (!IsValid(controlledPawn) || !GetWorld() || GetWorld()->IsPaused())
    {
        return nullptr;
    }

    UAssassinationTargetComponent* closestTarget = nullptr;
    double closestDistanceSquared = TNumericLimits<double>::Max();
    for (TActorIterator<AActor> actorIterator(GetWorld()); actorIterator; ++actorIterator)
    {
        TInlineComponentArray<UAssassinationTargetComponent*> targets(*actorIterator);
        for (UAssassinationTargetComponent* target : targets)
        {
            if (!IsValid(target) || !target->CanBeAssassinatedBy(controlledPawn))
            {
                continue;
            }
            const double distanceSquared = FVector::DistSquared(controlledPawn->GetActorLocation(),
                target->GetComponentLocation());
            if (distanceSquared < closestDistanceSquared)
            {
                closestDistanceSquared = distanceSquared;
                closestTarget = target;
            }
        }
    }
    return closestTarget;
}

bool AChapter3_ShooterGame_PlayerController::TryAssassinate()
{
    if (!IsLocalController() || !HasAuthority())
    {
        return false;
    }
    UAssassinationTargetComponent* target = FindAssassinationTarget();
    return IsValid(target) && target->TryAssassinate(GetPawn());
}

void AChapter3_ShooterGame_PlayerController::HandleAssassinationInput()
{
    TryAssassinate();
}

void AChapter3_ShooterGame_PlayerController::ThrowCoin()
{
    if (coinThrowSkill_)
    {
        coinThrowSkill_->TryThrowCoin();
    }
}

void AChapter3_ShooterGame_PlayerController::ForwardCoinThrown(ADistractionCoin* coin)
{
    onCoinThrown_.Broadcast(coin);
}

void AChapter3_ShooterGame_PlayerController::Fire()
{


    UWorld* world = GetWorld();
    APawn* controlledPawn = GetPawn();
    if (GetWorldTimerManager().IsTimerActive(reloadDelayTimer_))
    {
        return;
    }


    if (!world || !controlledPawn || !IsLocalController() || world->IsPaused()
        || fireRange_ <= 0.0f || reloading_)
    {
        return;
    }

    if (currentAmmo_ <= 0)
    {
        QueueAutomaticReload();
        return;
    }

    const double currentTime = world->GetTimeSeconds();
    if (currentTime < nextFireTime_)
    {
        return;
    }

    nextFireTime_ = currentTime + FMath::Max(0.0f, fireInterval_);
    --currentAmmo_;

    if (IsValid(weaponView_))
    {
        weaponView_->PlayFireFeedback();
    }

    // 카메라 위치에서 조준 방향으로 검사한다. 실제 투사체를 생성하지 않는 방식이다.
    FVector start = FVector::ZeroVector;
    FRotator viewRotation = FRotator::ZeroRotator;
    GetPlayerViewPoint(start, viewRotation);

    const FVector shotDirection = viewRotation.Vector();
    const FVector end = start + shotDirection * fireRange_;

    FCollisionQueryParams queryParams(SCENE_QUERY_STAT(PlayerShot), true);
    queryParams.AddIgnoredActor(this);
    queryParams.AddIgnoredActor(controlledPawn);
    queryParams.AddIgnoredActor(weaponView_);

    // 캐릭터에 부착된 총 등의 액터도 자기 자신에 맞지 않도록 제외한다.
    TArray<AActor*> attachedActors = {};
    controlledPawn->GetAttachedActors(attachedActors, true, true);
    queryParams.AddIgnoredActors(attachedActors);

    FHitResult hitResult = {};
    world->LineTraceSingleByChannel(hitResult, start, end, ECC_Visibility, queryParams);

    if (hitResult.bBlockingHit)
    {
        ApplyShotDamage(hitResult, shotDirection);
    }

    if (drawDebugShot_)
    {
        DrawShotDebug(start, end, hitResult);
    }

    if (currentAmmo_ == 0)
    {
        QueueAutomaticReload();
    }
}

void AChapter3_ShooterGame_PlayerController::QueueAutomaticReload()
{
    if (!GetWorld() || reloading_ || GetWorldTimerManager().IsTimerActive(reloadDelayTimer_))
    {
        return;
    }

    const float delay = FMath::IsFinite(reloadDelay_) ? FMath::Max(0.0f, reloadDelay_) : 0.0f;
    if (delay <= 0.0f)
    {
        StartReload();
        return;
    }
    GetWorldTimerManager().SetTimer(reloadDelayTimer_, this,
        &AChapter3_ShooterGame_PlayerController::StartReload, delay, false);
}

void AChapter3_ShooterGame_PlayerController::StartReload()
{
    UWorld* world = GetWorld();
    if (!world || world->IsPaused() || !IsLocalController() || !IsValid(GetPawn())
        || reloading_ || currentAmmo_ >= GetMagazineCapacity())
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(reloadDelayTimer_);
    const float duration = FMath::IsFinite(reloadDuration_)
        ? FMath::Max(kMinimumReloadDuration, reloadDuration_) : kDefaultReloadDuration;
    reloading_ = true;
    GetWorldTimerManager().SetTimer(reloadTimer_, this,
        &AChapter3_ShooterGame_PlayerController::FinishReload, duration, false);
}

void AChapter3_ShooterGame_PlayerController::FinishReload()
{
    // 예비 탄약 제한은 추후 추가한다. 지금은 장전이 끝날 때마다 탄창을 채운다.
    currentAmmo_ = GetMagazineCapacity();
    reloading_ = false;
    if (IsValid(weaponView_))
    {
        weaponView_->SetReloadState(false, 0.0f);
    }
}

float AChapter3_ShooterGame_PlayerController::GetReloadProgress() const
{
    if (!reloading_ || !GetWorld())
    {
        return 0.0f;
    }

    const FTimerManager& timerManager = GetWorld()->GetTimerManager();
    const float duration = timerManager.GetTimerRate(reloadTimer_);
    return duration > 0.0f
        ? FMath::Clamp(timerManager.GetTimerElapsed(reloadTimer_) / duration, 0.0f, 1.0f) : 0.0f;
}

void AChapter3_ShooterGame_PlayerController::ApplyShotDamage(const FHitResult& hitResult, const FVector& shotDirection)
{
    AActor* hitActor = hitResult.GetActor();
    if (!hitActor)
    {
        return;
    }

    UGameplayStatics::ApplyPointDamage(
        hitActor,
        FMath::Max(0.0f, damage_),
        shotDirection,
        hitResult,
        this,
        GetPawn(),
        UDamageType::StaticClass());
}

void AChapter3_ShooterGame_PlayerController::DrawShotDebug(const FVector& start, const FVector& end, const FHitResult& hitResult)
{
    const FVector traceEnd = hitResult.bBlockingHit ? hitResult.ImpactPoint : end;
    const FColor traceColor = hitResult.bBlockingHit ? FColor::Green : FColor::Red;
    DrawDebugLine(GetWorld(), start, traceEnd, traceColor, false, kDebugDuration, 0, kDebugLineThickness);

    if (hitResult.bBlockingHit)
    {
        DrawDebugSphere(GetWorld(), hitResult.ImpactPoint, kDebugHitRadius, kDebugSphereSegments,
            FColor::Green, false, kDebugDuration);
        UE_LOG(LogShooting, Log, TEXT("Shot hit: %s"), *GetNameSafe(hitResult.GetActor()));
    }
    else
    {
        UE_LOG(LogShooting, Log, TEXT("Shot missed"));
    }
}

void AChapter3_ShooterGame_PlayerController::EndPlay(const EEndPlayReason::Type endPlayReason)
{
    GetWorldTimerManager().ClearTimer(reloadTimer_);
    GetWorldTimerManager().ClearTimer(reloadDelayTimer_);
    reloading_ = false;
    for (FWeaponViewSlotState& slot : weaponSlots_)
    {
        if (IsValid(slot.instance_))
        {
            slot.instance_->Destroy();
        }
    }
    weaponSlots_.Empty();
    weaponView_ = nullptr;
    if (UEnhancedInputLocalPlayerSubsystem* subsystem = inputSubsystem_.Get())
    {
        subsystem->RemoveMappingContext(inputMappingContext_);
    }

    Super::EndPlay(endPlayReason);
}

void AChapter3_ShooterGame_PlayerController::StartAiming()
{
    aimHeld_ = true;
}

void AChapter3_ShooterGame_PlayerController::StopAiming()
{
    aimHeld_ = false;
}

void AChapter3_ShooterGame_PlayerController::FlushPressedKeys()
{
    Super::FlushPressedKeys();
    StopAiming();
}

void AChapter3_ShooterGame_PlayerController::UpdateCameraManager(float deltaSeconds)
{
    Super::UpdateCameraManager(deltaSeconds);

    if (!IsLocalController() || !PlayerCameraManager)
    {
        return;
    }

    APawn* controlledPawn = GetPawn();
    const bool showWeapon = IsValid(controlledPawn) && GetViewTarget() == controlledPawn;
    if (!showWeapon)
    {
        StopAiming();
        if (IsValid(weaponView_))
        {
            weaponView_->SetWeaponEquipped(false);
        }
        return;
    }

    if (!IsValid(weaponView_))
    {
        InitializeWeaponInventory();
        EquipWeaponAtIndex(equippedWeaponIndex_);
    }

    if (IsValid(weaponView_))
    {
        weaponView_->SetOwner(controlledPawn);
        weaponView_->SetWeaponEquipped(true);

        // 카메라 갱신 후 같은 프레임의 위치를 사용해 이동/회전 중 총이 뒤처지지 않게 한다.
        FMinimalViewInfo viewInfo = PlayerCameraManager->GetCameraCacheView();
        viewInfo.bUseFirstPersonParameters = true;
        viewInfo.FirstPersonFOV = viewInfo.FOV;
        viewInfo.FirstPersonScale = kFirstPersonScale;
        PlayerCameraManager->SetCameraCachePOV(viewInfo);
        weaponView_->SetReloadState(reloading_, GetReloadProgress());
        weaponView_->UpdateView(deltaSeconds, viewInfo.Location, viewInfo.Rotation, aimHeld_ && !reloading_);
    }
}
