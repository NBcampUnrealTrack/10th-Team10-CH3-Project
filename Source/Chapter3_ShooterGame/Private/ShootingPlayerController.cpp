// Fill out your copyright notice in the Description page of Project Settings.


#include "ShootingPlayerController.h"

#include "M1911WeaponView.h"
#include "DistractionCoin.h"
#include "Camera/PlayerCameraManager.h"

#include "DrawDebugHelpers.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
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
    constexpr int32 kFireMappingPriority = 1;
    constexpr float kDebugDuration = 1.0f;
    constexpr float kDebugLineThickness = 1.0f;
    constexpr float kDebugHitRadius = 8.0f;
    constexpr int32 kDebugSphereSegments = 12;
    constexpr float kFirstPersonScale = 0.8f;
    constexpr float kDefaultCoinThrowSpeed = 1000.0f;
    constexpr float kDefaultCoinUpwardSpeed = 300.0f;
    constexpr float kDefaultCoinThrowInterval = 0.6f;
    constexpr float kCoinSpawnForwardOffset = 45.0f;
    constexpr float kCoinSpawnRightOffset = 12.0f;
    constexpr float kCoinSpawnDownOffset = 10.0f;
}

AShootingPlayerController::AShootingPlayerController()
{
    OverridePlayerInputClass = UEnhancedPlayerInput::StaticClass();
    fireRange_ = kDefaultFireRange;
    damage_ = kDefaultDamage;
    fireInterval_ = kDefaultFireInterval;
    coinThrowSpeed_ = kDefaultCoinThrowSpeed;
    coinUpwardSpeed_ = kDefaultCoinUpwardSpeed;
    coinThrowInterval_ = kDefaultCoinThrowInterval;
}

void AShootingPlayerController::SetupInputComponent()
{
    // 프로젝트의 전역 입력 설정과 관계없이 이 컨트롤러는 Enhanced Input을 사용한다.
    if (!InputComponent)
    {
        InputComponent = NewObject<UEnhancedInputComponent>(this, TEXT("ShootingInputComponent"));
        InputComponent->RegisterComponent();
    }

    Super::SetupInputComponent();

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

void AShootingPlayerController::BindGameplayInput(UEnhancedInputComponent* enhancedInput)
{
    if (fireAction_)
    {
        enhancedInput->BindAction(fireAction_, ETriggerEvent::Started, this, &AShootingPlayerController::Fire);
    }

    if (aimAction_)
    {
        enhancedInput->BindAction(aimAction_, ETriggerEvent::Started, this, &AShootingPlayerController::StartAiming);
        enhancedInput->BindAction(aimAction_, ETriggerEvent::Completed, this, &AShootingPlayerController::StopAiming);
        enhancedInput->BindAction(aimAction_, ETriggerEvent::Canceled, this, &AShootingPlayerController::StopAiming);
    }

    if (throwCoinAction_)
    {
        enhancedInput->BindAction(throwCoinAction_, ETriggerEvent::Started, this, &AShootingPlayerController::ThrowCoin);
    }
}

void AShootingPlayerController::ThrowCoin()
{
    UWorld* world = GetWorld();
    APawn* controlledPawn = GetPawn();
    if (!world || !IsValid(controlledPawn) || !IsLocalController() || !HasAuthority()
        || world->IsPaused() || !coinClass_ || coinThrowSpeed_ <= 0.0f)
    {
        return;
    }

    const double currentTime = world->GetTimeSeconds();
    if (currentTime < nextCoinThrowTime_)
    {
        return;
    }

    FVector viewLocation = FVector::ZeroVector;
    FRotator viewRotation = FRotator::ZeroRotator;
    GetPlayerViewPoint(viewLocation, viewRotation);
    const FVector throwDirection = viewRotation.Vector();
    const FVector spawnLocation = viewLocation + viewRotation.RotateVector(
        FVector(kCoinSpawnForwardOffset, kCoinSpawnRightOffset, -kCoinSpawnDownOffset));

    FCollisionQueryParams queryParams(SCENE_QUERY_STAT(CoinSpawn), false);
    queryParams.AddIgnoredActor(this);
    queryParams.AddIgnoredActor(controlledPawn);
    queryParams.AddIgnoredActor(weaponView_);
    TArray<AActor*> attachedActors = {};
    controlledPawn->GetAttachedActors(attachedActors, true, true);
    queryParams.AddIgnoredActors(attachedActors);

    // 카메라와 생성 위치 사이의 벽을 검사해 벽 너머에서 코인이 생성되는 것을 막는다.
    const ADistractionCoin* defaultCoin = coinClass_.GetDefaultObject();
    const FCollisionShape collisionShape = FCollisionShape::MakeSphere(defaultCoin->GetCollisionRadius());
    FHitResult obstruction = {};
    if (world->SweepSingleByChannel(obstruction, viewLocation, spawnLocation,
        FQuat::Identity, ECC_WorldDynamic, collisionShape, queryParams))
    {
        return;
    }

    FActorSpawnParameters spawnParams = {};
    spawnParams.Owner = controlledPawn;
    spawnParams.Instigator = controlledPawn;
    spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    ADistractionCoin* coin = world->SpawnActor<ADistractionCoin>(coinClass_, spawnLocation, viewRotation, spawnParams);
    if (!IsValid(coin))
    {
        return;
    }

    nextCoinThrowTime_ = currentTime + FMath::Max(0.0f, coinThrowInterval_);
    onCoinThrown_.Broadcast(coin);
    if (IsValid(coin))
    {
        coin->LaunchCoin(throwDirection * coinThrowSpeed_ + FVector::UpVector * coinUpwardSpeed_);
    }
}

void AShootingPlayerController::Fire()
{
    UWorld* world = GetWorld();
    APawn* controlledPawn = GetPawn();
    if (!world || !controlledPawn || !IsLocalController() || world->IsPaused() || fireRange_ <= 0.0f)
    {
        return;
    }

    const double currentTime = world->GetTimeSeconds();
    if (currentTime < nextFireTime_)
    {
        return;
    }

    nextFireTime_ = currentTime + FMath::Max(0.0f, fireInterval_);

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
}

void AShootingPlayerController::ApplyShotDamage(const FHitResult& hitResult, const FVector& shotDirection)
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

void AShootingPlayerController::DrawShotDebug(const FVector& start, const FVector& end, const FHitResult& hitResult)
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

void AShootingPlayerController::EndPlay(const EEndPlayReason::Type endPlayReason)
{
    if (IsValid(weaponView_))
    {
        weaponView_->Destroy();
        weaponView_ = nullptr;
    }
    if (UEnhancedInputLocalPlayerSubsystem* subsystem = inputSubsystem_.Get())
    {
        subsystem->RemoveMappingContext(inputMappingContext_);
    }

    Super::EndPlay(endPlayReason);
}

void AShootingPlayerController::StartAiming()
{
    aimHeld_ = true;
}

void AShootingPlayerController::StopAiming()
{
    aimHeld_ = false;
}

void AShootingPlayerController::FlushPressedKeys()
{
    Super::FlushPressedKeys();
    StopAiming();
}

void AShootingPlayerController::UpdateCameraManager(float deltaSeconds)
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
            weaponView_->SetActorHiddenInGame(true);
        }
        return;
    }

    if (!IsValid(weaponView_) && weaponViewClass_)
    {
        FActorSpawnParameters spawnParams = {};
        spawnParams.Owner = controlledPawn;
        spawnParams.Instigator = controlledPawn;
        spawnParams.ObjectFlags |= RF_Transient;
        spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        weaponView_ = GetWorld()->SpawnActor<AM1911WeaponView>(weaponViewClass_, FTransform::Identity, spawnParams);
    }

    if (IsValid(weaponView_))
    {
        weaponView_->SetOwner(controlledPawn);
        weaponView_->SetActorHiddenInGame(false);

        // 카메라 갱신 후 같은 프레임의 위치를 사용해 이동/회전 중 총이 뒤처지지 않게 한다.
        FMinimalViewInfo viewInfo = PlayerCameraManager->GetCameraCacheView();
        viewInfo.bUseFirstPersonParameters = true;
        viewInfo.FirstPersonFOV = viewInfo.FOV;
        viewInfo.FirstPersonScale = kFirstPersonScale;
        PlayerCameraManager->SetCameraCachePOV(viewInfo);
        weaponView_->UpdateView(deltaSeconds, viewInfo.Location, viewInfo.Rotation, aimHeld_);
    }
}

