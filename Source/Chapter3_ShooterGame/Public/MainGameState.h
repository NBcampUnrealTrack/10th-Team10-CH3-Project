#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MainGameState.generated.h"

UCLASS()
class CHAPTER3_SHOOTERGAME_API AMainGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameClearOption")
	bool isGameClear = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameClearOption")
	uint8 playerKillCount_ = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameClearOption")
	uint8 fieldEnemyCount_ = 0;
};
