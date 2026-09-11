#include "EnemySpawner.h"
#include "EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"

AEnemySpawner::AEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> enemys;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyCharacter::StaticClass(), enemys);

	numberOfSet_ = enemys.Num();
	if (numberOfSet_ - numberOfSpawn_ >= 0)
	{
		numberOfDelete_ = numberOfSet_ - numberOfSpawn_;

		for (int32 i = 0; i < numberOfDelete_; i++)
		{
			uint16 randomIndex = FMath::RandRange(0, enemys.Num() - 1);
			if (enemys[randomIndex])
			{
				if (AEnemyCharacter* deleteEnemy = Cast<AEnemyCharacter>(enemys[randomIndex]))
				{
					enemys.RemoveAt(randomIndex);
					deleteEnemy->DestroyEnemy();
				}
			}
		}
	}

}
