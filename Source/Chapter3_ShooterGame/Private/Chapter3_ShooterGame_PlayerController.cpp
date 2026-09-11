#include "Chapter3_ShooterGame_PlayerController.h"
#include "Chapter3_ShooterGame_Character.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

AChapter3_ShooterGame_PlayerController::AChapter3_ShooterGame_PlayerController()
	: InputMappingContext(nullptr)
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
}

void AChapter3_ShooterGame_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (InputMappingContext)
			{
				Subsystem->AddMappingContext(InputMappingContext, 0);
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

		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AChapter3_ShooterGame_PlayerController::HandleFireStart);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AChapter3_ShooterGame_PlayerController::HandleFireStop);

		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AChapter3_ShooterGame_PlayerController::HandleAimStart);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AChapter3_ShooterGame_PlayerController::HandleAimStop);

		EnhancedInputComponent->BindAction(Skill1Action, ETriggerEvent::Started, this, &AChapter3_ShooterGame_PlayerController::HandleSkill1);
		EnhancedInputComponent->BindAction(Skill2Action, ETriggerEvent::Started, this, &AChapter3_ShooterGame_PlayerController::HandleSkill2);

		EnhancedInputComponent->BindAction(LeanAction, ETriggerEvent::Triggered, this, &AChapter3_ShooterGame_PlayerController::HandleLean);
		EnhancedInputComponent->BindAction(LeanAction, ETriggerEvent::Completed, this, &AChapter3_ShooterGame_PlayerController::HandleLean);
	}
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
