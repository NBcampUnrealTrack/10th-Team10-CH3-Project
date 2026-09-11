#include "EnemyAIController.h"
#include "TimerManager.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AEnemyAIController::AEnemyAIController()
{
	//AIPerception 학습 필요
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SetPerceptionComponent(*AIPerception);

	//AISenseConfig_Sight 학습 필요
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	//시야 범위
	SightConfig->SightRadius = sightRadius_;
	// 시야 범위에서 벗어난 후 시야를 잃는 범위
	SightConfig->LoseSightRadius = loseSightRadius_;
	// 시야각
	SightConfig->PeripheralVisionAngleDegrees = 90.0f;
	SightConfig->SetMaxAge(5.0f);

	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
}

float AEnemyAIController::GetSightRadius(void) const
{
	return sightRadius_;
}
float AEnemyAIController::GetLoseSightRadius(void) const
{
	return loseSightRadius_;
}
float AEnemyAIController::GetMoveRadius(void) const
{
	return moveRadius_;
}

void AEnemyAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Stimulus.WasSuccessfullySensed())
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, FString::Printf(TEXT("CapturedPlayer!")));

		//적 AI가 플레이어를 감지했을 때
		isCaptured_ = true;
		GetWorldTimerManager().ClearTimer(enemyBehaviorTimer_);
		//SetFocus(playerPawn_);
	}
	else
	{
		//적 AI가 플레이어를 감지하지 못했을 때
		isCaptured_ = false;
		GetWorldTimerManager().SetTimer(enemyBehaviorTimer_, this, &AEnemyAIController::MoveToRandomLocation, 2.0f, true);
		//ClearFocus(EAIFocusPriority::Gameplay);
	}
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	myPawn_ = GetPawn();
	playerPawn_ = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	goalPoint_ = myPawn_->GetActorLocation();

	if (AIPerception)
	{
		AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::OnPerceptionUpdated);
	}

	GetWorldTimerManager().SetTimer(enemyBehaviorTimer_, this, &AEnemyAIController::MoveToRandomLocation, 2.0f, true);
}
void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void AEnemyAIController::MoveToRandomLocation(void)
{
	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, FString::Printf(TEXT("MoveToRandomLocation!")));

	if (myPawn_)
	{
		//현재 월드에서 사용 중인 NavigationSystem을 가져옵니다.
		UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());

		//NavigationSystem이 반환하는 위치 정보를 저장할 구조체 선언.
		FNavLocation RandomLocation;
		bool bFoundLocation = NavSystem->GetRandomReachablePointInRadius(myPawn_->GetActorLocation(), moveRadius_, RandomLocation);

		if (bFoundLocation)
		{
			goalPoint_ = RandomLocation.Location;

			//World의 특정 좌표를 목적지로 설정하고 AI에게 이동 명령을 내립니다.
			MoveToLocation(goalPoint_);
		}
	}
}
void AEnemyAIController::MoveToPlayerLocation(void)
{
	if (myPawn_ && playerPawn_)
	{
		goalPoint_ = playerPawn_->GetActorLocation();
		MoveToLocation(goalPoint_);
	}
}
void AEnemyAIController::StopEnemy(void)
{
	StopMovement();
}

void AEnemyAIController::ClearControllerTimer(void)
{
	GetWorldTimerManager().ClearTimer(enemyBehaviorTimer_);
}