#include "EnemyCharacter.h"
#include "EnemyAIController.h"
#include "Kismet/GameplayStatics.h"
//#include "PhysicalMaterials/PhysicalMaterial.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyCharacter::AEnemyCharacter()
{
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		runSpeed_ = walkSpeed_ * 1.5f;

		if (enemyType_ == EEnemyType::bodyguard)
		{
			walkSpeed_ *= 0.75f;
			runSpeed_ *= 0.75f;

			Movement->MaxWalkSpeed = walkSpeed_;
		}
		else
		{
			Movement->MaxWalkSpeed = walkSpeed_;
		}

		//Character가 이동하는 방향을 바라보도록 설정합니다.
		Movement->bOrientRotationToMovement = true;
		//Character가 회전할 때의 속도를 설정합니다.
		Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	}

	PrimaryActorTick.bCanEverTick = false;
}
void AEnemyCharacter::SetMovementSpeed(void)
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		if (alertType_ == EAlertType::detection)
		{
			Movement->MaxWalkSpeed = runSpeed_;
			return;
		}

		Movement->MaxWalkSpeed = walkSpeed_;
		return;
	}
}
void AEnemyCharacter::AlertCalculation(void)
{
	if (AEnemyAIController* enemyAIController = Cast<AEnemyAIController>(GetController()))
	{
		if (enemyAIController->isCaptured_)
		{
			if (APawn* playerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
			{
				float distanceToPlayer = FVector::Distance(GetActorLocation(), playerPawn->GetActorLocation());

				if (distanceToPlayer > sightRadius_ / 2.0f)
				{
					if (distanceToPlayer < sightRadius_ - 50.0f)
					{
						//적 AI의 시야 범위 안에 처음 들어온 경우
						if (alertType_ != EAlertType::caution)
						{
							GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, FString::Printf(TEXT("Mode: Caution!")));

							alertType_ = EAlertType::caution;
							SetMovementSpeed();
						}
					}
					enemyAIController->MoveToPlayerLocation();
					return;
				}

				if (distanceToPlayer > sightRadius_ / 4.0f)
				{
					if (distanceToPlayer < sightRadius_ / 3.0f)
					{
						//적 AI의 발각 범위 안에 처음 들어온 경우
						if (alertType_ != EAlertType::detection)
						{
							GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, FString::Printf(TEXT("Mode: Detection!")));

							alertType_ = EAlertType::detection;
							enemyAIController->ClearFocus(EAIFocusPriority::Gameplay);
							SetMovementSpeed();
						}
					}
					enemyAIController->MoveToPlayerLocation();
					return;
				}

				if (distanceToPlayer < sightRadius_ / 5.0f)
				{
					//적 AI의 공격 범위 안에 처음 들어온 경우
					if (alertType_ != EAlertType::attack)
					{
						GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("Mode: Attack!")));

						alertType_ = EAlertType::attack;
						enemyAIController->SetFocus(playerPawn);
						enemyAIController->StopEnemy();

						//적 AI 공격 구현
						GetWorldTimerManager().SetTimer(EnemyAttackIntervalTimer_, this, &AEnemyCharacter::StartFire, 1.0f, true);
					}
					return;
				}
			}
			enemyAIController->MoveToPlayerLocation();
			return;
		}
		else
		{
			if (alertType_ != EAlertType::patrol)
			{
				GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, FString::Printf(TEXT("Mode: Patrol!")));

				alertType_ = EAlertType::patrol;
				GetWorldTimerManager().ClearTimer(EnemyAttackIntervalTimer_);
				SetMovementSpeed();

				return;
			}
			return;
		}
	}
}
void AEnemyCharacter::DestroyEnemy(void)
{
	if (AEnemyAIController* enemyAIController = Cast<AEnemyAIController>(GetController()))
	{
		GetWorldTimerManager().ClearTimer(EnemyStateUpdateTimer_);
		GetWorldTimerManager().ClearTimer(EnemyAttackIntervalTimer_);
		enemyAIController->ClearControllerTimer();

		Destroy();
	}
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	alertType_ = EAlertType::patrol;
	if (AEnemyAIController* enemyAIController = Cast<AEnemyAIController>(GetController()))
	{
		sightRadius_ = enemyAIController->GetSightRadius();
	}

	GetWorldTimerManager().SetTimer(EnemyStateUpdateTimer_, this, &AEnemyCharacter::AlertCalculation, 0.25f, true);
}
float AEnemyCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	ActualDamage -= defense_;

	currentHealth_ = FMath::Clamp(currentHealth_ - ActualDamage, 0.0f, maxHealth_);
	//UpdateOverheadHP();

	if (currentHealth_ <= 0.0f) {
		//적 AI 사망 시, OnDeath() 함수 호출
		OnDeath();
	}

	return ActualDamage;
}

void AEnemyCharacter::StartFire(void) {
	if (AEnemyAIController* enemyAIController = Cast<AEnemyAIController>(GetController()))
	{
		enemyAIController->Fire();
	}
}
void AEnemyCharacter::OnDeath(void) {
	//적 AI 사망 정보 GameState에 전송해야 함!

	//적 AI 사망 로직 구현
	DestroyEnemy();
}

void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
