#include "MainGameState.h"

AMainGameState::AMainGameState()
{
	isGameClear_ = false;
	playerKillCount_ = 0;
}

void AMainGameState::SetIsGameClear(bool newIsGameClear)
{
	isGameClear_ = newIsGameClear;
}
void AMainGameState::SetPlayerKillCount(uint8 newPlayerKillCount)
{
	playerKillCount_ = newPlayerKillCount;
}

bool AMainGameState::GetIsGameClear(void) const
{
	return isGameClear_;
}
uint8 AMainGameState::GetPlayerKillCount(void) const
{
	return playerKillCount_;
}