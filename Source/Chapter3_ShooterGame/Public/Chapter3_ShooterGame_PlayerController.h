#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "ShooterGame_PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;

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
};
