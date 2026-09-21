#if WITH_DEV_AUTOMATION_TESTS

#include "Chapter3GameInstance.h"
#include "Chapter3SaveGame.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"

namespace Chapter3ProgressTests
{
// Never read, replace, or delete a player's slot, even when a test fails early.
struct FTemporarySlot
{
    const FString Name = TEXT("CH3_Automation_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);

    ~FTemporarySlot()
    {
        UGameplayStatics::DeleteGameInSlot(Name, 0);
    }
};

// The desktop save system writes Saved/SaveGames/<slot>.sav. An empty directory
// at that exact unique path blocks writes without locking or changing real saves.
struct FScopedSaveWriteBlocker
{
    explicit FScopedSaveWriteBlocker(const FString& SlotName)
        : Path(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), SlotName + TEXT(".sav")))
    {
        bCreated = !IFileManager::Get().DirectoryExists(*Path) &&
            IFileManager::Get().MakeDirectory(*Path, true);
    }

    ~FScopedSaveWriteBlocker()
    {
        Remove();
    }

    bool Remove()
    {
        if (bCreated)
        {
            // Never recurse: this fixture owns only its empty directory.
            if (!IFileManager::Get().DeleteDirectory(*Path, false, false))
            {
                return false;
            }
            bCreated = false;
        }
        return true;
    }

    FString Path;
    bool bCreated = false;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter3ProgressRoundTripTest,
    "CH3.Progress.FileRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChapter3ProgressRoundTripTest::RunTest(const FString& Parameters)
{
    using namespace Chapter3ProgressTests;
    FTemporarySlot Slot;
    const FName Level(TEXT("Test_Level_01"));
    const FName Collectible(TEXT("Test_Collectible_01"));
    const FGuid RunId = FGuid::NewGuid();
    TStrongObjectPtr<UChapter3GameInstance> First(NewObject<UChapter3GameInstance>());
    if (!TestTrue(TEXT("A missing slot starts a new profile"), First->InitializeProgress(Slot.Name)))
    {
        return false;
    }
    TestTrue(TEXT("The new profile is ready"), First->IsProgressReady());
    TestEqual(TEXT("A new profile has no money"), First->GetMoney(), int64(0));
    TestFalse(TEXT("A new profile has no clear record"), First->HasClearedLevel(Level));
    TestFalse(TEXT("A new profile has no collectible"), First->HasCollectedItem(Collectible));
    TestTrue(TEXT("Collecting an item succeeds"), First->CollectItem(Collectible));
    TestTrue(TEXT("Completing a mission succeeds"), First->CompleteMission(Level, RunId, 1500, false));
    TestFalse(TEXT("Successful synchronous writes leave no pending save"), First->IsSavePending());
    TestTrue(TEXT("The profile exists on disk"), UGameplayStatics::DoesSaveGameExist(Slot.Name, 0));

    // A separate instance must recover from the file, not from shared live state.
    TStrongObjectPtr<UChapter3GameInstance> Reloaded(NewObject<UChapter3GameInstance>());
    if (!TestTrue(TEXT("A separate instance loads the slot"), Reloaded->InitializeProgress(Slot.Name)))
    {
        return false;
    }
    TestTrue(TEXT("Clear record survives reload"), Reloaded->HasClearedLevel(Level));
    TestTrue(TEXT("Collectible survives reload"), Reloaded->HasCollectedItem(Collectible));
    TestEqual(TEXT("Money survives reload"), Reloaded->GetMoney(), int64(1500));
    TestFalse(TEXT("Duplicate collection is rejected after reload"), Reloaded->CollectItem(Collectible));
    TestFalse(TEXT("Duplicate payout is rejected after reload"), Reloaded->CompleteMission(Level, RunId, 1500, false));
    TestEqual(TEXT("Duplicate payout leaves money unchanged"), Reloaded->GetMoney(), int64(1500));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter3ProgressRewardPolicyTest,
    "CH3.Progress.RewardPolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChapter3ProgressRewardPolicyTest::RunTest(const FString& Parameters)
{
    using namespace Chapter3ProgressTests;
    FTemporarySlot Slot;
    TStrongObjectPtr<UChapter3GameInstance> Progress(NewObject<UChapter3GameInstance>());
    if (!TestTrue(TEXT("Initialize isolated profile"), Progress->InitializeProgress(Slot.Name)))
    {
        return false;
    }
    const FName Level(TEXT("Test_Repeatable"));
    const FGuid FirstRun = FGuid::NewGuid();
    TestTrue(TEXT("First clear grants its reward"), Progress->CompleteMission(Level, FirstRun, 100, false));
    TestFalse(TEXT("Repeated completion event is rejected"), Progress->CompleteMission(Level, FirstRun, 100, false));
    TestEqual(TEXT("Repeated event does not double the reward"), Progress->GetMoney(), int64(100));
    TestFalse(TEXT("A run ID cannot pay a second level"), Progress->CompleteMission(FName(TEXT("Test_Other")), FirstRun, 100, false));
    TestFalse(TEXT("Rejected reused run does not clear another level"), Progress->HasClearedLevel(FName(TEXT("Test_Other"))));
    TestTrue(TEXT("A new run can receive a repeatable reward"), Progress->CompleteMission(Level, FGuid::NewGuid(), 50, false));
    TestEqual(TEXT("Repeatable rewards accumulate"), Progress->GetMoney(), int64(150));

    const FName FirstClearLevel(TEXT("Test_FirstClearOnly"));
    TestTrue(TEXT("First-clear policy pays a new level"), Progress->CompleteMission(FirstClearLevel, FGuid::NewGuid(), 200, true));
    const FGuid UnpaidRun = FGuid::NewGuid();
    TestTrue(TEXT("First-clear policy accepts a later completion"), Progress->CompleteMission(FirstClearLevel, UnpaidRun, 200, true));
    TestEqual(TEXT("A later first-clear-only run grants no money"), Progress->GetMoney(), int64(350));
    TestFalse(TEXT("An unpaid run is still recorded against replay"), Progress->CompleteMission(FirstClearLevel, UnpaidRun, 200, false));
    TestEqual(TEXT("Changing policy cannot replay a processed run"), Progress->GetMoney(), int64(350));
    TestTrue(TEXT("Zero-reward missions can still be cleared"), Progress->CompleteMission(FName(TEXT("Test_ZeroReward")), FGuid::NewGuid(), 0, false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter3ProgressValidationTest,
    "CH3.Progress.InvalidInputAndOverflow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChapter3ProgressValidationTest::RunTest(const FString& Parameters)
{
    using namespace Chapter3ProgressTests;
    FTemporarySlot Slot;
    TStrongObjectPtr<UChapter3GameInstance> Progress(NewObject<UChapter3GameInstance>());
    if (!TestTrue(TEXT("Initialize isolated profile"), Progress->InitializeProgress(Slot.Name)))
    {
        return false;
    }
    const FName Level(TEXT("Test_Validation"));
    TestFalse(TEXT("Empty collectible ID is rejected"), Progress->CollectItem(NAME_None));
    TestFalse(TEXT("Empty level ID is rejected"), Progress->CompleteMission(NAME_None, FGuid::NewGuid(), 10, false));
    TestFalse(TEXT("Invalid mission run is rejected"), Progress->CompleteMission(Level, FGuid(), 10, false));
    TestFalse(TEXT("Negative reward is rejected"), Progress->CompleteMission(Level, FGuid::NewGuid(), -1, false));
    TestFalse(TEXT("Invalid requests do not clear a level"), Progress->HasClearedLevel(Level));
    TestEqual(TEXT("Invalid requests do not change money"), Progress->GetMoney(), int64(0));

    TestTrue(TEXT("Maximum representable balance is accepted"), Progress->CompleteMission(Level, FGuid::NewGuid(), MAX_int64, false));
    const FName OverflowLevel(TEXT("Test_Overflow"));
    const FGuid OverflowRun = FGuid::NewGuid();
    TestFalse(TEXT("An overflowing reward is rejected"), Progress->CompleteMission(OverflowLevel, OverflowRun, 1, false));
    TestEqual(TEXT("Overflow does not wrap the balance"), Progress->GetMoney(), int64(MAX_int64));
    TestFalse(TEXT("Overflow rejection does not partially clear the level"), Progress->HasClearedLevel(OverflowLevel));
    TestTrue(TEXT("Rejected run can later complete with a valid zero reward"), Progress->CompleteMission(OverflowLevel, OverflowRun, 0, false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter3ProgressProtectedFileTest,
    "CH3.Progress.UnreadableFileProtection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChapter3ProgressProtectedFileTest::RunTest(const FString& Parameters)
{
    using namespace Chapter3ProgressTests;
    for (const bool bUnsupportedVersion : { false, true })
    {
        FTemporarySlot Slot;
        if (bUnsupportedVersion)
        {
            TStrongObjectPtr<UChapter3SaveGame> FutureSave(NewObject<UChapter3SaveGame>());
            FutureSave->SaveVersion = MAX_int32;
            FutureSave->Money = 1234;
            if (!TestTrue(TEXT("Write an unsupported-version fixture"), UGameplayStatics::SaveGameToSlot(FutureSave.Get(), Slot.Name, 0)))
            {
                return false;
            }
        }
        else
        {
            TArray<uint8> CorruptBytes;
            CorruptBytes.Init(0, 64);
            if (!TestTrue(TEXT("Write a corrupt fixture"), UGameplayStatics::SaveDataToSlot(CorruptBytes, Slot.Name, 0)))
            {
                return false;
            }
        }
        TArray<uint8> Before;
        if (!TestTrue(TEXT("Read original fixture bytes"), UGameplayStatics::LoadDataFromSlot(Before, Slot.Name, 0)))
        {
            return false;
        }
        TStrongObjectPtr<UChapter3GameInstance> Progress(NewObject<UChapter3GameInstance>());
        TestFalse(TEXT("Unreadable or unsupported profile is rejected"), Progress->InitializeProgress(Slot.Name));
        TestFalse(TEXT("Rejected profile is not ready"), Progress->IsProgressReady());
        TestFalse(TEXT("Explicit save cannot overwrite a rejected profile"), Progress->SaveProgress());
        TestFalse(TEXT("Collecting cannot overwrite a rejected profile"), Progress->CollectItem(FName(TEXT("Test_Item"))));
        TestFalse(TEXT("Mission completion cannot overwrite a rejected profile"), Progress->CompleteMission(FName(TEXT("Test_Level")), FGuid::NewGuid(), 100, false));
        TArray<uint8> After;
        TestTrue(TEXT("Protected fixture is still readable as bytes"), UGameplayStatics::LoadDataFromSlot(After, Slot.Name, 0));
        TestTrue(TEXT("Rejected profile remains byte-for-byte unchanged"), Before == After);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter3ProgressSaveRetryTest,
    "CH3.Progress.SaveFailureAndRetry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChapter3ProgressSaveRetryTest::RunTest(const FString& Parameters)
{
    using namespace Chapter3ProgressTests;
    FTemporarySlot Slot;
    TStrongObjectPtr<UChapter3GameInstance> Progress(NewObject<UChapter3GameInstance>());
    if (!TestTrue(TEXT("Initialize isolated retry profile"), Progress->InitializeProgress(Slot.Name)))
    {
        return false;
    }
    FScopedSaveWriteBlocker Blocker(Slot.Name);
    if (!TestTrue(TEXT("Block writes only to the unique test slot"), Blocker.bCreated))
    {
        return false;
    }
    const FName Level(TEXT("Test_RetryLevel"));
    const FName Item(TEXT("Test_RetryItem"));
    const FGuid RunId = FGuid::NewGuid();
    TestTrue(TEXT("Mission remains accepted in memory when disk write fails"), Progress->CompleteMission(Level, RunId, 250, false));
    TestTrue(TEXT("Failed write marks progress pending"), Progress->IsSavePending());
    TestTrue(TEXT("Failed write retains clear record in memory"), Progress->HasClearedLevel(Level));
    TestEqual(TEXT("Failed write retains money in memory"), Progress->GetMoney(), int64(250));
    TestTrue(TEXT("Collection remains accepted in memory when disk write fails"), Progress->CollectItem(Item));
    TestTrue(TEXT("Failed write retains collection in memory"), Progress->HasCollectedItem(Item));
    TestFalse(TEXT("Pending reward cannot be applied twice"), Progress->CompleteMission(Level, RunId, 250, false));
    TestFalse(TEXT("Retry reports failure while the slot is blocked"), Progress->SaveProgress());
    TestTrue(TEXT("Failed retry remains pending"), Progress->IsSavePending());

    if (!TestTrue(TEXT("Remove the temporary write blocker"), Blocker.Remove()))
    {
        return false;
    }
    TestTrue(TEXT("Retry succeeds after writes are restored"), Progress->SaveProgress());
    TestFalse(TEXT("Successful retry clears the pending flag"), Progress->IsSavePending());
    TStrongObjectPtr<UChapter3GameInstance> Reloaded(NewObject<UChapter3GameInstance>());
    if (!TestTrue(TEXT("Reload the recovered save from disk"), Reloaded->InitializeProgress(Slot.Name)))
    {
        return false;
    }
    TestTrue(TEXT("Retried clear record survives reload"), Reloaded->HasClearedLevel(Level));
    TestTrue(TEXT("Retried collection survives reload"), Reloaded->HasCollectedItem(Item));
    TestEqual(TEXT("Retried reward is saved exactly once"), Reloaded->GetMoney(), int64(250));
    TestFalse(TEXT("Retried payout remains protected from replay after reload"), Reloaded->CompleteMission(Level, RunId, 250, false));
    TestEqual(TEXT("Replay does not increase the recovered balance"), Reloaded->GetMoney(), int64(250));
    return true;
}

#endif
