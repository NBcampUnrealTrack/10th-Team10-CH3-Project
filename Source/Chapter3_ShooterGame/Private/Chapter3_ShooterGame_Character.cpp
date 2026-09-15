// Fill out your copyright notice in the Description page of Project Settings.

#include "Chapter3_ShooterGame_Character.h"
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

	// 1인칭 카메라 
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, BaseEyeHeight));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// 무기
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	// 전신 메쉬
	GetMesh()->SetOwnerNoSee(true);
}

void AChapter3_ShooterGame_Character::BeginPlay()
{
	Super::BeginPlay();
}

void AChapter3_ShooterGame_Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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
	CurrentLeanValue = LeanValue * LeanAngle;
}
