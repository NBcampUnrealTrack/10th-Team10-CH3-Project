#include "EnemyCharacter.h"
#include "EnemyAIController.h"
#include "DistractionCoin.h"
#include "MainGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyCharacter::AEnemyCharacter()
{
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	soundTriggerCollision_ = CreateDefaultSubobject<USphereComponent>(TEXT("SoundTriggerCollision"));
	soundTriggerCollision_->SetupAttachment(RootComponent);

	soundTriggerCollision_->InitSphereRadius(300.0f);
	soundTriggerCollision_->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));

	soundTriggerCollision_->CanCharacterStepUpOn = ECB_No;
	soundTriggerCollision_->SetCollisionProfileName(TEXT("OverlapAll"));
	soundTriggerCollision_->SetGenerateOverlapEvents(true);

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
							alertType_ = EAlertType::detection;
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

		if (currentHealth_ > 0)
		{
			Destroy();
			return;
		}
	}

	FTimerHandle deleyDestroyTimer;
	USkeletalMeshComponent* meshComponent = GetMesh();
	UCharacterMovementComponent* movementComponent = GetCharacterMovement();

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	movementComponent->StopMovementImmediately();
	movementComponent->DisableMovement();

	meshComponent->SetCollisionObjectType(ECC_WorldStatic);
	meshComponent->SetSimulatePhysics(true);
	meshComponent->WakeAllRigidBodies();

	GetWorldTimerManager().SetTimer(deleyDestroyTimer, [this](){Destroy();}, 5.0f, false);
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (soundTriggerCollision_)
	{
		soundTriggerCollision_->OnComponentBeginOverlap.AddDynamic(this, &AEnemyCharacter::OnOverlapBegin);
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		runSpeed_ = walkSpeed_ * 1.5f;

		if (enemyType_ == EEnemyType::bodyguard)
		{
			defense_ = 2.5f;

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

void AEnemyCharacter::StartFire(void)
{
	if (AEnemyAIController* enemyAIController = Cast<AEnemyAIController>(GetController()))
	{
		enemyAIController->Fire();
	}
}
void AEnemyCharacter::OnDeath(void)
{
	//적 AI 사망 정보 GameState에 전송해야 함!
	if (AMainGameState* mainGameState = GetWorld()->GetGameState<AMainGameState>())
	{
		mainGameState->SetPlayerKillCount(mainGameState->GetPlayerKillCount() + 1);
	}

	//적 AI 사망 로직 구현
	DestroyEnemy();
}

void AEnemyCharacter::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA<ADistractionCoin>())
	{
		ADistractionCoin* coin = Cast<ADistractionCoin>(OtherActor);
		coin->onCoinLanded_.AddDynamic(this, &AEnemyCharacter::AcceptedLocation);
	}
}
void AEnemyCharacter::AcceptedLocation(FVector landingLocation)
{
	FVector Direction = landingLocation - GetActorLocation();

	FRotator LookAtRotation = Direction.Rotation();
	LookAtRotation.Pitch = 0.0f;
	LookAtRotation.Roll = 0.0f;

	if (AEnemyAIController* enemyAIController = Cast<AEnemyAIController>(GetController()))
	{
		if (alertType_ != EAlertType::patrol)
		{
			return;
		}

		if (enemyAIController->GetFocusActor())
		{
			enemyAIController->ClearFocus(EAIFocusPriority::Gameplay);
		}

		SetActorRotation(LookAtRotation);
		enemyAIController->PauseEnemyBehaviorTimer(FMath::FRandRange(5.0f, 10.0f));
	}
}

void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
