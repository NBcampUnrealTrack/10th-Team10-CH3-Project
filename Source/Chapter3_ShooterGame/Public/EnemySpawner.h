#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

UCLASS()
class CHAPTER3_SHOOTERGAME_API AEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	AEnemySpawner();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawner")
	uint8 numberOfSpawn_ = 0;

	uint16 numberOfSet_ = 0;
	uint16 numberOfDelete_ = 0;

	void SetNumberOfSpawn(uint8 newValue);
	uint8 GetNumberOfSpawn(void);

protected:
	virtual void BeginPlay() override;
};
