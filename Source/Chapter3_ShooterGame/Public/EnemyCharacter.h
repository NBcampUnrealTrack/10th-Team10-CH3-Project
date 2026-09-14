#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	bouncer UMETA(DisplayName = "Bouncer"),
	bodyguard UMETA(DisplayName = "Bodyguard")
};
UENUM(BlueprintType)
enum class EAlertType : uint8
{
	patrol UMETA(DisplayName = "Patrol"),
	caution UMETA(DisplayName = "Caution"),
	detection UMETA(DisplayName = "Detection"),
	attack UMETA(DisplayName = "Attack")
};

UCLASS()
class CHAPTER3_SHOOTERGAME_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

	void SetMovementSpeed(void);
	void AlertCalculation(void);
	void DestroyEnemy(void);

	UPROPERTY(EditAnywhere, Category = "AI")
	float walkSpeed_ = 300.0f;
	UPROPERTY(VisibleAnywhere, Category = "AI")
	float runSpeed_ = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Defense")
	float defense_ = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Health")
	float maxHealth_ = 100.0f;
	UPROPERTY(EditAnywhere, Category = "Health")
	float currentHealth_ = 100.0f;

	UPROPERTY(EditAnywhere)
	EEnemyType enemyType_;
	//UPROPERTY(EditAnywhere)
	//TMap<EEnemyType, USkeletalMesh*> enemyMeshes_;

protected:
	virtual void BeginPlay() override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	void StartFire(void);
	void OnDeath(void);

private:
	FTimerHandle EnemyStateUpdateTimer_;
	FTimerHandle EnemyAttackIntervalTimer_;

	EAlertType alertType_ = EAlertType::patrol;
	float sightRadius_ = 0.0f;

public:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};