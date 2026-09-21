#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Chapter3GameInstance.generated.h"

class UChapter3SaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChapter3ProgressChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChapter3SaveFinished, bool, success, const FString&, error);

UCLASS()
class CHAPTER3_SHOOTERGAME_API UChapter3GameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;
    virtual void Shutdown() override;

    // One initialization per instance. Explicit slot selection also isolates automation tests.
    bool InitializeProgress(const FString& slotName);

    // Mutation results report acceptance in memory. Check IsSavePending for disk persistence.
    UFUNCTION(BlueprintCallable, Category = "Progress")
    bool CollectItem(FName itemId);

    // Pickup instance guards duplicate calls; revisiting a map can award money again.
    UFUNCTION(BlueprintCallable, Category = "Progress|Money")
    bool AddMoney(int64 amount);

    UFUNCTION(BlueprintCallable, Category = "Progress")
    bool CompleteMission(FName levelId, FGuid missionRunId, int64 rewardAmount, bool firstClearRewardOnly);

    UFUNCTION(BlueprintCallable, Category = "Progress|Save")
    bool SaveProgress();

    UFUNCTION(BlueprintPure, Category = "Progress")
    bool HasCollectedItem(FName itemId) const;

    UFUNCTION(BlueprintPure, Category = "Progress")
    bool HasClearedLevel(FName levelId) const;

    UFUNCTION(BlueprintPure, Category = "Progress")
    int64 GetMoney() const;

    UFUNCTION(BlueprintPure, Category = "Progress|Save")
    bool IsProgressReady() const { return progressReady_; }

    UFUNCTION(BlueprintPure, Category = "Progress|Save")
    bool IsSavePending() const { return savePending_; }

    UFUNCTION(BlueprintPure, Category = "Progress|Save")
    FString GetLastSaveError() const { return lastSaveError_; }

    UPROPERTY(BlueprintAssignable, Category = "Progress")
    FOnChapter3ProgressChanged OnProgressChanged;

    UPROPERTY(BlueprintAssignable, Category = "Progress|Save")
    FOnChapter3SaveFinished OnSaveFinished;

private:
    bool Fail(const FString& error);

    UPROPERTY(Transient)
    TObjectPtr<UChapter3SaveGame> progress_;

    FString slotName_;
    FString lastSaveError_;
    bool initialized_ = false;
    bool progressReady_ = false;
    bool savePending_ = false;
};
