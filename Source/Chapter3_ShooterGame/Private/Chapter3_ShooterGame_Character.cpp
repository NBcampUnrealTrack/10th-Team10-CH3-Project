// Fill out your copyright notice in the Description page of Project Settings.

#include "Chapter3_ShooterGame_Character.h"
#include "UnlockInventoryPickup.h"
#include "BonusPickup.h"
#include "DistractionCoin.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

AChapter3_ShooterGame_Character::AChapter3_ShooterGame_Character()
{
 	PrimaryActorTick.bCanEverTick = true;

	// 카메라 
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// 1인칭 카메라 
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, BaseEyeHeight));
	// Lean(Roll)을 직접 제어하기 위해 컨트롤러 회전 자동 적용은 끄고, Tick에서 Pitch/Yaw/Roll을 직접 합성한다.
	FirstPersonCameraComponent->bUsePawnControlRotation = false;
	DefaultCameraRelativeLocation = FirstPersonCameraComponent->GetRelativeLocation();

	// 무기
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	// 전신 메쉬
	GetMesh()->SetOwnerNoSee(true);

	// 체력 초기화
	CurrentHealth = MaxHealth;
}

void AChapter3_ShooterGame_Character::BeginPlay()
{
	Super::BeginPlay();
}

void AChapter3_ShooterGame_Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateLean(DeltaTime);
}

void AChapter3_ShooterGame_Character::Move(FVector2D MovementVector)
{
	if (Controller)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AChapter3_ShooterGame_Character::Look(FVector2D LookAxisVector)
{
	if (Controller)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AChapter3_ShooterGame_Character::SetSprinting(bool bSprint)
{
	if (bSprint && GetCharacterMovement()->IsCrouching())
	{
		return;
	}

	bIsSprinting = bSprint;
	UpdateMovementSpeed();
}

void AChapter3_ShooterGame_Character::SetCrouching(bool bCrouch)
{
	if (bCrouch)
	{
		bIsSprinting = false;
		Crouch();
	}
	else
	{
		UnCrouch();
	}

	UpdateMovementSpeed();
}

void AChapter3_ShooterGame_Character::UpdateMovementSpeed()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	if (Movement->IsCrouching())
	{
		Movement->MaxWalkSpeedCrouched = CrouchSpeed;
	}
	else if (bIsSprinting)
	{
		Movement->MaxWalkSpeed = SprintSpeed;
	}
	else
	{
		Movement->MaxWalkSpeed = WalkSpeed;
	}
}

void AChapter3_ShooterGame_Character::TryParkour()
{
	if (GetCharacterMovement()->IsCrouching())
	{
		OnCrawlStart();
		return;
	}

	TryVaultOrClimb();
}

void AChapter3_ShooterGame_Character::TryVaultOrClimb()
{
	const FVector Start = GetActorLocation();
	const FVector Forward = GetActorForwardVector();
	const FVector End = Start + Forward * ParkourTraceDistance;

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);

	if (bHit)
	{
		const FVector LedgeCheckStart = Hit.ImpactPoint + FVector(0, 0, VaultCheckHeight);
		const FVector LedgeCheckEnd = LedgeCheckStart - FVector(0, 0, VaultCheckHeight);

		FHitResult LedgeHit;
		const bool bLedgeHit = GetWorld()->LineTraceSingleByChannel(LedgeHit, LedgeCheckStart, LedgeCheckEnd, ECC_Visibility, QueryParams);

		if (bLedgeHit)
		{
			OnVaultStart();

			LaunchCharacter(Forward * (SprintSpeed * 0.5f) + FVector(0, 0, VaultCheckHeight * 4.f), true, true);
		}
	}
}

void AChapter3_ShooterGame_Character::Interact()
{
    TArray<AActor*> nearbyPickups = {};
    GetOverlappingActors(nearbyPickups);
    nearbyPickups.RemoveAll([](const AActor* actor)
    {
        return !IsValid(actor) || !actor->IsA<ADistractionCoin>();
    });
    // 수집품은 겹침 캐시가 비어 있어도 실제 범위로 찾는다. 동전의 기존 판정은 유지한다.
    for (TActorIterator<AUnlockInventoryPickup> it(GetWorld()); it; ++it)
    {
        if (it->IsCollectorInRange(this))
        {
            nearbyPickups.Add(*it);
        }
    }
    for (TActorIterator<ABonusPickup> it(GetWorld()); it; ++it)
    {
        if (it->IsCollectorInRange(this))
        {
            nearbyPickups.Add(*it);
        }
    }
    const FVector collectorLocation = GetActorLocation();
    nearbyPickups.Sort([collectorLocation](const AActor& left, const AActor& right)
    {
        return FVector::DistSquared(collectorLocation, left.GetActorLocation())
            < FVector::DistSquared(collectorLocation, right.GetActorLocation());
    });

    for (AActor* actor : nearbyPickups)
    {
        AUnlockInventoryPickup* pickup = Cast<AUnlockInventoryPickup>(actor);
        if (IsValid(pickup) && pickup->TryCollect(this))
        {
            return;
        }

        ABonusPickup* bonus = Cast<ABonusPickup>(actor);
        if (IsValid(bonus) && bonus->TryCollect(this))
        {
            return;
        }

        ADistractionCoin* coin = Cast<ADistractionCoin>(actor);
        if (IsValid(coin) && coin->TryCollect(this))
        {
            return;
        }
    }

	const FVector Start = GetPawnViewLocation();
	const FVector End = Start + GetControlRotation().Vector() * InteractTraceDistance;

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
	{
		if (Hit.GetActor())
		{
			OnInteract(Hit.GetActor());
		}
	}
}

void AChapter3_ShooterGame_Character::StartFire()
{
	const FVector Start = GetPawnViewLocation();
	const FVector End = Start + GetControlRotation().Vector() * FireTraceDistance;

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);

	OnFire(Hit);
}

void AChapter3_ShooterGame_Character::StopFire()
{
}

float AChapter3_ShooterGame_Character::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 이미 죽었거나 데미지가 0 이하면 처리하지 않음
	if (bIsDead || ActualDamage <= 0.f)
	{
		return ActualDamage;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.f, MaxHealth);

	OnDamaged(ActualDamage, CurrentHealth);

	if (CurrentHealth <= 0.f)
	{
		bIsDead = true;
		OnDeath();
	}

	return ActualDamage;
}

void AChapter3_ShooterGame_Character::SetAiming(bool bAim)
{
	bIsAiming = bAim;

	if (bAim)
	{
		OnAimStart();
	}
	else
	{
		OnAimEnd();
	}
}

void AChapter3_ShooterGame_Character::UseSkill1()
{
	OnSkill1();
}

void AChapter3_ShooterGame_Character::UseSkill2()
{
	OnSkill2();
}

void AChapter3_ShooterGame_Character::SetLean(float LeanValue)
{
	// -1(왼쪽) ~ 1(오른쪽) 범위
	TargetLeanValue = FMath::Clamp(LeanValue, -1.f, 1.f);
}

float AChapter3_ShooterGame_Character::CalculateSafeLeanAlpha(float DesiredAlpha) const
{
	if (FMath::IsNearlyZero(DesiredAlpha) || !GetWorld())
	{
		return DesiredAlpha;
	}

	const FVector Start = GetPawnViewLocation();
	const FVector Right = GetActorRightVector();
	const FVector End = Start + Right * (LeanSideOffset * DesiredAlpha);

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
	{
		const float MaxDistance = FMath::Abs(LeanSideOffset * DesiredAlpha);
		const float SafeDistance = (Hit.ImpactPoint - Start).Size();
		const float Ratio = (MaxDistance > KINDA_SMALL_NUMBER) ? FMath::Clamp(SafeDistance / MaxDistance, 0.f, 1.f) : 0.f;

		return DesiredAlpha * Ratio;
	}

	return DesiredAlpha;
}

void AChapter3_ShooterGame_Character::UpdateLean(float DeltaTime)
{
	if (!FirstPersonCameraComponent)
	{
		return;
	}

	const float SafeTarget = CalculateSafeLeanAlpha(TargetLeanValue);
	const float TargetRoll = SafeTarget * LeanAngle;
	CurrentLeanValue = FMath::FInterpTo(CurrentLeanValue, TargetRoll, DeltaTime, LeanInterpSpeed);

	const float LeanAlpha = (LeanAngle != 0.f) ? (CurrentLeanValue / LeanAngle) : 0.f;

	const FVector LeanedLocation = DefaultCameraRelativeLocation
		+ FVector(0.f, LeanSideOffset * LeanAlpha, -FMath::Abs(LeanAlpha) * LeanHeightDrop);
	FirstPersonCameraComponent->SetRelativeLocation(LeanedLocation);

	const FRotator ControlRotation = Controller ? Controller->GetControlRotation() : GetActorRotation();
	const FRotator LeanedRotation(ControlRotation.Pitch, ControlRotation.Yaw, CurrentLeanValue);
	FirstPersonCameraComponent->SetWorldRotation(LeanedRotation);
}
