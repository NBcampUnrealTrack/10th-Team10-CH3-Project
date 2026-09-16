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

	fireRange_ = 10000.0f;
	damage_ = 20.0f;
	fireInterval_ = 0.2f;
	reloadDuration_ = 0.0f;
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
	GetWorldTimerManager().ClearTimer(reloadDelayTimer_);
	GetWorldTimerManager().ClearTimer(reloadTimer_);
}

void AEnemyAIController::Fire(void)
{
	UWorld* world = GetWorld();
	if (GetWorldTimerManager().IsTimerActive(reloadDelayTimer_))
	{
		return;
	}


	if (!world || !myPawn_ || !IsLocalController() || world->IsPaused()
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

	// 카메라 위치에서 조준 방향으로 검사한다. 실제 투사체를 생성하지 않는 방식이다.
	FVector start = FVector::ZeroVector;
	FRotator viewRotation = FRotator::ZeroRotator;
	GetPlayerViewPoint(start, viewRotation);

	const FVector shotDirection = viewRotation.Vector();
	const FVector end = start + shotDirection * fireRange_;

	FCollisionQueryParams queryParams(SCENE_QUERY_STAT(PlayerShot), true);
	queryParams.AddIgnoredActor(this);
	queryParams.AddIgnoredActor(myPawn_);

	// 캐릭터에 부착된 총 등의 액터도 자기 자신에 맞지 않도록 제외한다.
	TArray<AActor*> attachedActors = {};
	myPawn_->GetAttachedActors(attachedActors, true, true);
	queryParams.AddIgnoredActors(attachedActors);

	FHitResult hitResult = {};
	world->LineTraceSingleByChannel(hitResult, start, end, ECC_Visibility, queryParams);

	if (hitResult.bBlockingHit)
	{
		ApplyShotDamage(hitResult, shotDirection);
	}

	if (currentAmmo_ == 0)
	{
		QueueAutomaticReload();
	}
}
void AEnemyAIController::QueueAutomaticReload(void)
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
		&AEnemyAIController::StartReload, delay, false);
}
void AEnemyAIController::StartReload(void)
{
	UWorld* world = GetWorld();
	if (!world || world->IsPaused() || !IsLocalController() || !IsValid(GetPawn())
		|| reloading_ || currentAmmo_ >= magazineCapacity_)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(reloadDelayTimer_);
	const float duration = FMath::IsFinite(reloadDuration_)
		? FMath::Max(kMinimumReloadDuration, reloadDuration_) : kDefaultReloadDuration;
	reloading_ = true;
	GetWorldTimerManager().SetTimer(reloadTimer_, this,
		&AEnemyAIController::FinishReload, duration, false);
}
void AEnemyAIController::FinishReload(void)
{
	// 예비 탄약 제한은 추후 추가한다. 지금은 장전이 끝날 때마다 탄창을 채운다.
	currentAmmo_ = magazineCapacity_;
	reloading_ = false;
}

void AEnemyAIController::ApplyShotDamage(const FHitResult& hitResult, const FVector& shotDirection)
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