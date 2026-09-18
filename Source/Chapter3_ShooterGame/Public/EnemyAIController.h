#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;

UCLASS()
class CHAPTER3_SHOOTERGAME_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

private:
	APawn* myPawn_;
	APawn* playerPawn_;
	FVector goalPoint_;

	FTimerHandle enemyBehaviorTimer_;
	FTimerHandle reloadDelayTimer_;
	FTimerHandle reloadTimer_;

	UPROPERTY(EditAnyWhere, Category = "AI")
	float sightRadius_ = 2000.0f;
	UPROPERTY(EditAnyWhere, Category = "AI")
	float loseSightRadius_ = 3000.0f;
	UPROPERTY(EditAnyWhere, Category = "AI")
	float moveRadius_ = 1000.0f;

	UPROPERTY(EditAnyWhere, Category = "EnemyShooting", meta = (ClampMin = "0.0", Units = "cm"))
	float fireRange_ = 0.0f;
	UPROPERTY(EditAnyWhere, Category = "EnemyShooting", meta = (ClampMin = "0.0"))
	float damage_ = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "EnemyShooting", meta = (ClampMin = "0.0", Units = "s"))
	float fireInterval_ = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "EnemyShooting|Magazine", meta = (ClampMin = "1", UIMin = "1"))
	int32 magazineCapacity_ = 10;
	UPROPERTY(EditDefaultsOnly, Category = "EnemyShooting|Magazine", meta = (ClampMin = "0.2", Units = "s"))
	float reloadDuration_ = 0.0f;
	UPROPERTY(VisibleInstanceOnly, Category = "EnemyShooting|Magazine", meta = (AllowPrivateAccess = "true"))
	bool reloading_ = false;
	UPROPERTY(EditDefaultsOnly, Category = "EnemyShooting|Magazine", meta = (ClampMin = "0.01", Units = "s"))
	float reloadDelay_ = 0.1f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "EnemyShooting|Magazine", meta = (AllowPrivateAccess = "true"))
	int32 currentAmmo_ = 0;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionComponent* AIPerception;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAISenseConfig_Sight* SightConfig;

	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

public:
	bool isCaptured_ = false;
	float kMinimumReloadDuration = 0.2f;
	float kDefaultReloadDuration = 1.8f;
	float nextFireTime_ = 0.0f;

	float GetSightRadius(void) const;
	float GetLoseSightRadius(void) const;
	float GetMoveRadius(void) const;

	void MoveToRandomLocation(void);
	void MoveToPlayerLocation(void);
	void StopEnemy(void);

	void PauseEnemyBehaviorTimer(float pauseTime);
	void ClearControllerTimer(void);

	void Fire(void);
	void StartReload(void);
	void FinishReload(void);
	void QueueAutomaticReload(void);

	void ApplyShotDamage(const FHitResult& hitResult, const FVector& shotDirection);
};