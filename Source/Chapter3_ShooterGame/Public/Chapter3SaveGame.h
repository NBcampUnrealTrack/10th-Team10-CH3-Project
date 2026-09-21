#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Chapter3SaveGame.generated.h"

// Only stable identifiers and player progress belong in this file format.
UCLASS()
class CHAPTER3_SHOOTERGAME_API UChapter3SaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame)
    int32 SaveVersion = 1;

    UPROPERTY(SaveGame)
    TSet<FName> ClearedLevelIds;

    UPROPERTY(SaveGame)
    TSet<FName> CollectedItemIds;

    UPROPERTY(SaveGame)
    int64 Money = 0;

    UPROPERTY(SaveGame)
    TSet<FGuid> RewardedMissionRuns;
};
