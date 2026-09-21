#if WITH_DEV_AUTOMATION_TESTS

#include "Chapter3GameInstance.h"
#include "Chapter3_ShooterGame_GameMode.h"
#include "UnlockInventoryComponent.h"
#include "UnlockInventoryPickup.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

namespace Chapter3ProgressIntegrationTests
{
struct FFixture
{
    const FString Slot = TEXT("CH3_Automation_Integration_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    TStrongObjectPtr<UChapter3GameInstance> Progress{NewObject<UChapter3GameInstance>()};
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);

    FFixture()
    {
        if (World)
        {
            World->SetGameInstance(Progress.Get());
        }
    }

    ~FFixture()
    {
        if (World)
        {
            World->DestroyWorld(false);
        }
        UGameplayStatics::DeleteGameInSlot(Slot, 0);
    }

    UUnlockInventoryComponent* CreateInventory(FName ItemId)
    {
        APlayerController* Controller = World->SpawnActor<APlayerController>();
        if (!Controller)
        {
            return nullptr;
        }
        UUnlockInventoryComponent* Inventory = NewObject<UUnlockInventoryComponent>(Controller);
        FUnlockInventoryItem Item;
        Item.itemId_ = ItemId;
        Inventory->itemDefinitions_.Add(Item);
        Controller->AddInstanceComponent(Inventory);
        Inventory->RegisterComponent();
        return Inventory;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter3InventoryIntegrationTest,
    "CH3.Progress.Integration.InventoryAndPickup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChapter3InventoryIntegrationTest::RunTest(const FString& Parameters)
{
    using namespace Chapter3ProgressIntegrationTests;
    FFixture Fixture;
    if (!TestNotNull(TEXT("Create an isolated game world"), Fixture.World)) return false;
    const FName ItemId(TEXT("Integration_Collectible"));
    UUnlockInventoryComponent* First = Fixture.CreateInventory(ItemId);
    if (!TestNotNull(TEXT("Create controller inventory"), First)) return false;
    TestTrue(TEXT("Collection is blocked before progress initialization"),
        First->UnlockItem(ItemId) == EUnlockInventoryResult::ProgressUnavailable);
    TestFalse(TEXT("Blocked collection leaves the slot locked"), First->IsItemUnlocked(ItemId));
    if (!TestTrue(TEXT("Initialize an isolated save slot"), Fixture.Progress->InitializeProgress(Fixture.Slot))) return false;
    TestTrue(TEXT("Registered item unlocks through the real component"),
        First->UnlockItem(ItemId) == EUnlockInventoryResult::Unlocked);
    TestTrue(TEXT("Component forwards collection to persistent progress"), Fixture.Progress->HasCollectedItem(ItemId));
    TestTrue(TEXT("Unknown item is rejected"),
        First->UnlockItem(FName(TEXT("Unknown_Item"))) == EUnlockInventoryResult::InvalidItem);

    UUnlockInventoryComponent* Restored = Fixture.CreateInventory(ItemId);
    if (!TestNotNull(TEXT("Create replacement controller inventory"), Restored)) return false;
    TestTrue(TEXT("New inventory restores the saved unlock"), Restored->IsItemUnlocked(ItemId));
    TestTrue(TEXT("Restored item cannot unlock twice"),
        Restored->UnlockItem(ItemId) == EUnlockInventoryResult::AlreadyUnlocked);

    AUnlockInventoryPickup* Pickup = Fixture.World->SpawnActor<AUnlockInventoryPickup>();
    if (!TestNotNull(TEXT("Spawn previously collected pickup"), Pickup)) return false;
    Pickup->itemId_ = ItemId;
    Pickup->DispatchBeginPlay();
    TestTrue(TEXT("Previously collected pickup destroys itself during BeginPlay"), Pickup->IsActorBeingDestroyed());
    TestFalse(TEXT("Restored pickup cannot collide"), Pickup->GetActorEnableCollision());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter3MissionIntegrationTest,
    "CH3.Progress.Integration.MissionCompletion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChapter3MissionIntegrationTest::RunTest(const FString& Parameters)
{
    using namespace Chapter3ProgressIntegrationTests;
    FFixture Fixture;
    if (!TestNotNull(TEXT("Create an isolated game world"), Fixture.World)) return false;
    if (!TestTrue(TEXT("Initialize an isolated save slot"), Fixture.Progress->InitializeProgress(Fixture.Slot))) return false;
    const FName LevelId(TEXT("Integration_Level"));
    for (int32 Run = 0; Run < 2; ++Run)
    {
        AChapter3_ShooterGame_GameMode* Mode = Fixture.World->SpawnActor<AChapter3_ShooterGame_GameMode>();
        if (!TestNotNull(TEXT("Spawn a new mission GameMode"), Mode)) return false;
        Mode->progress_level_id_ = LevelId;
        Mode->mission_reward_amount_ = 250;
        Mode->first_clear_reward_only_ = false;
        Mode->DispatchBeginPlay();
        Mode->TriggerGameEnd();
        Mode->TriggerGameEnd();
        TestTrue(TEXT("Ending marks the GameMode cleared"), Mode->IsGameCleared());
        TestTrue(TEXT("Ending records the persistent level ID"), Fixture.Progress->HasClearedLevel(LevelId));
        TestEqual(TEXT("Each run pays once despite duplicate ending calls"), Fixture.Progress->GetMoney(), int64((Run + 1) * 250));
        Mode->Destroy();
    }
    TStrongObjectPtr<UChapter3GameInstance> Reloaded(NewObject<UChapter3GameInstance>());
    if (!TestTrue(TEXT("Reload completion data from disk"), Reloaded->InitializeProgress(Fixture.Slot))) return false;
    TestTrue(TEXT("Integrated completion survives reload"), Reloaded->HasClearedLevel(LevelId));
    TestEqual(TEXT("Integrated payouts survive reload"), Reloaded->GetMoney(), int64(500));
    return true;
}

#endif
