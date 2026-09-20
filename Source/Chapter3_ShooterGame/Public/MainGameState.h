#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MainGameState.generated.h"

UCLASS()
class CHAPTER3_SHOOTERGAME_API AMainGameState : public AGameState
{
	GENERATED_BODY()

private:
	UPROPERTY(EditDefaultsOnly, Category = "GameClearOption")
	bool isGameClear_ = false;

	UPROPERTY(EditDefaultsOnly, Category = "GameClearOption")
	uint8 playerKillCount_ = 0;

public:
	AMainGameState();

	void SetIsGameClear(bool newIsGameClear);
	void SetPlayerKillCount(uint8 newPlayerKillCount);

	bool GetIsGameClear(void) const;
	uint8 GetPlayerKillCount(void) const;
};