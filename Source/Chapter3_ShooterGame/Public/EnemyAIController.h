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

	UPROPERTY(EditAnyWhere, Category = "AI")
	float sightRadius_ = 2000.0f;
	UPROPERTY(EditAnyWhere, Category = "AI")
	float loseSightRadius_ = 3000.0f;
	UPROPERTY(EditAnyWhere, Category = "AI")
	float moveRadius_ = 1000.0f;

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

	float GetSightRadius(void) const;
	float GetLoseSightRadius(void) const;
	float GetMoveRadius(void) const;

	void MoveToRandomLocation(void);
	void MoveToPlayerLocation(void);
	void StopEnemy(void);

	void ClearControllerTimer(void);
};