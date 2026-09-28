#include "Chapter3GameInstance.h"
#include "Chapter3SaveGame.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogChapter3Progress, Log, All);

void UChapter3GameInstance::Init()
{
    InitializeProgress(TEXT("Chapter3_Progress"));

    Super::Init();
}

void UChapter3GameInstance::Shutdown()
{
    if (progressReady_ && savePending_)
    {
        SaveProgress();
    }
    Super::Shutdown();
}

bool UChapter3GameInstance::Fail(const FString& error)
{
    lastSaveError_ = error;
    UE_LOG(LogChapter3Progress, Warning, TEXT("%s"), *error);
    OnSaveFinished.Broadcast(false, lastSaveError_);
    return false;
}

bool UChapter3GameInstance::InitializeProgress(const FString& slotName)
{
    if (initialized_)
    {
        return progressReady_ && slotName_ == slotName;
    }
    initialized_ = true;
    slotName_ = slotName;
    if (slotName_.IsEmpty() || slotName_.Contains(TEXT("/")) || slotName_.Contains(TEXT("\\")) || slotName_.Contains(TEXT("..")))
    {
        return Fail(TEXT("Invalid progress save slot."));
    }

    if (UGameplayStatics::DoesSaveGameExist(slotName_, 0))
    {
        progress_ = Cast<UChapter3SaveGame>(UGameplayStatics::LoadGameFromSlot(slotName_, 0));
        if (!progress_ || progress_->SaveVersion != 1 || progress_->Money < 0 ||
            progress_->ClearedLevelIds.Contains(NAME_None) || progress_->CollectedItemIds.Contains(NAME_None) ||
            progress_->RewardedMissionRuns.Contains(FGuid()))
        {
            progress_ = nullptr;
            return Fail(TEXT("Progress save is unreadable or unsupported. Existing file was preserved; progression is disabled."));
        }
    }
    else
    {
        progress_ = NewObject<UChapter3SaveGame>(this);
    }

    progressReady_ = true;
    lastSaveError_.Reset();
    return true;
}

bool UChapter3GameInstance::SaveProgress()
{
    if (!progressReady_ || !progress_)
    {
        return Fail(TEXT("Progress is not loaded. Refusing to overwrite the save file."));
    }
    savePending_ = true;
    if (!UGameplayStatics::SaveGameToSlot(progress_, slotName_, 0))
    {
        return Fail(TEXT("Could not save progress. Changes remain in memory; retry SaveProgress."));
    }
    savePending_ = false;
    lastSaveError_.Reset();
    OnSaveFinished.Broadcast(true, lastSaveError_);
    return true;
}

bool UChapter3GameInstance::CollectItem(FName itemId)
{
    if (!progressReady_ || !progress_ || itemId.IsNone() || progress_->CollectedItemIds.Contains(itemId))
    {
        return false;
    }
    progress_->CollectedItemIds.Add(itemId);
    SaveProgress();
    OnProgressChanged.Broadcast();
    return true;
}

bool UChapter3GameInstance::AddMoney(int64 amount)
{
    if (!progressReady_ || !progress_ || amount <= 0 || progress_->Money > MAX_int64 - amount)
    {
        return false;
    }
    progress_->Money += amount;
    SaveProgress();
    OnProgressChanged.Broadcast();
    return true;
}

bool UChapter3GameInstance::SpendMoney(int64 amount)
{
    if (!progressReady_ || !progress_ || amount <= 0 || progress_->Money < amount)
    {
        return false;
    }
    progress_->Money -= amount;
    SaveProgress();
    OnProgressChanged.Broadcast();
    return true;
}

bool UChapter3GameInstance::CompleteMission(FName levelId, FGuid missionRunId, int64 rewardAmount, bool firstClearRewardOnly)
{
    if (!progressReady_ || !progress_ || levelId.IsNone() || !missionRunId.IsValid() || rewardAmount < 0 ||
        progress_->RewardedMissionRuns.Contains(missionRunId))
    {
        return false;
    }
    const int64 grantedReward = firstClearRewardOnly && progress_->ClearedLevelIds.Contains(levelId) ? 0 : rewardAmount;
    if (progress_->Money > MAX_int64 - grantedReward)
    {
        return false;
    }

    progress_->ClearedLevelIds.Add(levelId);
    progress_->Money += grantedReward;
    progress_->RewardedMissionRuns.Add(missionRunId);
    SaveProgress();
    OnProgressChanged.Broadcast();
    return true;
}

bool UChapter3GameInstance::HasCollectedItem(FName itemId) const
{
    return progressReady_ && progress_ && progress_->CollectedItemIds.Contains(itemId);
}

bool UChapter3GameInstance::HasClearedLevel(FName levelId) const
{
    return progressReady_ && progress_ && progress_->ClearedLevelIds.Contains(levelId);
}

int64 UChapter3GameInstance::GetMoney() const
{
    return progressReady_ && progress_ ? progress_->Money : 0;
}
