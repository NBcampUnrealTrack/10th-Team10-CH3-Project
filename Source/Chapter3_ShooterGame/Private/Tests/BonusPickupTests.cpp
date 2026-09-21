#if WITH_DEV_AUTOMATION_TESTS

#include "BonusPickup.h"
#include "Chapter3GameInstance.h"
#include "Chapter3_ShooterGame_Character.h"
#include "Chapter3_ShooterGame_GameMode.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Engine/EngineBaseTypes.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

namespace Chapter3BonusPickupTests
{
struct FFixture
{
    const FString Slot = TEXT("CH3_Automation_Bonus_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    TStrongObjectPtr<UChapter3GameInstance> Progress{NewObject<UChapter3GameInstance>()};
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AChapter3_ShooterGame_Character* Character = nullptr;
    AChapter3_ShooterGame_GameMode* Mode = nullptr;

    FFixture()
    {
        if (!World) return;
        World->SetGameInstance(Progress.Get());
        World->GetWorldSettings()->DefaultGameMode = AChapter3_ShooterGame_GameMode::StaticClass();
        World->SetGameMode(FURL());
        Mode = World->GetAuthGameMode<AChapter3_ShooterGame_GameMode>();
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Character = World->SpawnActor<AChapter3_ShooterGame_Character>(
            FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
        APlayerController* Controller = World->SpawnActor<APlayerController>();
        if (!Character || !Controller) return;
        Character->GetCapsuleComponent()->SetCapsuleSize(30.0f, 90.0f);
        Character->GetCapsuleComponent()->SetGenerateOverlapEvents(false);
        Controller->Possess(Character);
    }

    ~FFixture()
    {
        if (World) World->DestroyWorld(false);
        UGameplayStatics::DeleteGameInSlot(Slot, 0);
    }

    ABonusPickup* AddPickup(int64 Money, int32 Score, const FVector& Location = FVector(35.0, 0.0, 0.0))
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ABonusPickup* Pickup = World->SpawnActor<ABonusPickup>(Location, FRotator::ZeroRotator, SpawnParameters);
        if (Pickup)
        {
            Pickup->moneyReward_ = Money;
            Pickup->scoreReward_ = Score;
            Pickup->pickupRadius_ = 150.0f;
            Pickup->pickupSphere_->SetSphereRadius(150.0f);
            Pickup->pickupSphere_->SetGenerateOverlapEvents(false);
        }
        return Pickup;
    }

    bool Validate(FAutomationTestBase& Test) const
    {
        return Test.TestNotNull(TEXT("Create isolated world"), World)
            && Test.TestNotNull(TEXT("Create controlled character"), Character)
            && Test.TestNotNull(TEXT("Install authoritative score GameMode"), Mode);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter3BonusMoneyTest,
    "CH3.Progress.Integration.BonusMoneyAndReplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChapter3BonusMoneyTest::RunTest(const FString& Parameters)
{
    using namespace Chapter3BonusPickupTests;
    FFixture Fixture;
    if (!Fixture.Validate(*this)
        || !TestTrue(TEXT("Initialize isolated save"), Fixture.Progress->InitializeProgress(Fixture.Slot))) return false;
    ABonusPickup* Cash = Fixture.AddPickup(125, 0);
    if (!TestNotNull(TEXT("Spawn cash"), Cash)) return false;
    TestTrue(TEXT("Cash is collected"), Cash->TryCollect(Fixture.Character));
    TestEqual(TEXT("Cash increases persistent money"), Fixture.Progress->GetMoney(), int64(125));
    TestEqual(TEXT("Money-only pickup leaves score unchanged"), Fixture.Mode->GetCurrentScore(), 0);
    TestTrue(TEXT("Collected cash is consumed"), Cash->IsActorBeingDestroyed());
    TestFalse(TEXT("Repeated collection of the same actor is rejected"), Cash->TryCollect(Fixture.Character));
    TestEqual(TEXT("Repeated collection does not pay twice"), Fixture.Progress->GetMoney(), int64(125));

    // A replay creates fresh placed actors; bonus pickups have no permanent collection ID.
    ABonusPickup* ReplayCash = Fixture.AddPickup(125, 0);
    if (!TestNotNull(TEXT("Spawn a fresh cash instance"), ReplayCash)) return false;
    TestTrue(TEXT("A new instance can award money again"), ReplayCash->TryCollect(Fixture.Character));
    TestEqual(TEXT("New instance adds another reward"), Fixture.Progress->GetMoney(), int64(250));
    TStrongObjectPtr<UChapter3GameInstance> Reloaded(NewObject<UChapter3GameInstance>());
    if (!TestTrue(TEXT("Read isolated save back from disk"), Reloaded->InitializeProgress(Fixture.Slot))) return false;
    TestEqual(TEXT("Both cash rewards survive reload"), Reloaded->GetMoney(), int64(250));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter3BonusScoreTest,
    "CH3.Progress.Integration.BonusScoreAndCombinedReward",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChapter3BonusScoreTest::RunTest(const FString& Parameters)
{
    using namespace Chapter3BonusPickupTests;
    FFixture Fixture;
    if (!Fixture.Validate(*this)
        || !TestTrue(TEXT("Initialize isolated save"), Fixture.Progress->InitializeProgress(Fixture.Slot))) return false;
    ABonusPickup* Gem = Fixture.AddPickup(0, 200);
    ABonusPickup* Combined = Fixture.AddPickup(50, 300);
    if (!TestNotNull(TEXT("Spawn score-only gem"), Gem)
        || !TestNotNull(TEXT("Spawn combined reward"), Combined)) return false;
    TestTrue(TEXT("Score-only gem collects"), Gem->TryCollect(Fixture.Character));
    TestEqual(TEXT("Gem adds score through GameMode"), Fixture.Mode->GetCurrentScore(), 200);
    TestEqual(TEXT("Score-only gem leaves money unchanged"), Fixture.Progress->GetMoney(), int64(0));
    TestTrue(TEXT("Combined reward collects"), Combined->TryCollect(Fixture.Character));
    TestEqual(TEXT("Combined pickup adds its score once"), Fixture.Mode->GetCurrentScore(), 500);
    TestEqual(TEXT("Combined pickup also adds money"), Fixture.Progress->GetMoney(), int64(50));
    TestFalse(TEXT("Combined pickup cannot award a second time"), Combined->TryCollect(Fixture.Character));
    TestEqual(TEXT("Duplicate attempt leaves score unchanged"), Fixture.Mode->GetCurrentScore(), 500);
    TestEqual(TEXT("Duplicate attempt leaves money unchanged"), Fixture.Progress->GetMoney(), int64(50));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter3BonusInteractionTest,
    "CH3.Progress.Integration.BonusRangeAndInteraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChapter3BonusInteractionTest::RunTest(const FString& Parameters)
{
    using namespace Chapter3BonusPickupTests;
    FFixture Fixture;
    if (!Fixture.Validate(*this)
        || !TestTrue(TEXT("Initialize isolated save"), Fixture.Progress->InitializeProgress(Fixture.Slot))) return false;
    ABonusPickup* Outside = Fixture.AddPickup(500, 0, FVector(181.0, 0.0, 0.0));
    ABonusPickup* Far = Fixture.AddPickup(70, 0, FVector(100.0, 0.0, 0.0));
    ABonusPickup* Near = Fixture.AddPickup(30, 0);
    if (!TestNotNull(TEXT("Spawn outside pickup"), Outside)
        || !TestNotNull(TEXT("Spawn farther pickup first"), Far)
        || !TestNotNull(TEXT("Spawn nearest pickup last"), Near)) return false;
    TestFalse(TEXT("Beyond sphere plus player capsule is out of range"), Outside->IsCollectorInRange(Fixture.Character));
    TestFalse(TEXT("Out-of-range direct collection is rejected"), Outside->TryCollect(Fixture.Character));
    TestFalse(TEXT("Out-of-range pickup remains available"), Outside->IsActorBeingDestroyed());
    TestEqual(TEXT("Out-of-range pickup pays nothing"), Fixture.Progress->GetMoney(), int64(0));
    TestFalse(TEXT("Nearby pickup has no cached overlap"), Near->pickupSphere_->IsOverlappingActor(Fixture.Character));
    Fixture.Character->Interact();
    TestTrue(TEXT("Interact collects the nearest bonus without overlap events"), Near->IsActorBeingDestroyed());
    TestFalse(TEXT("One interaction leaves the farther bonus"), Far->IsActorBeingDestroyed());
    TestEqual(TEXT("One interaction awards only the nearest bonus"), Fixture.Progress->GetMoney(), int64(30));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter3BonusInvalidRewardTest,
    "CH3.Progress.Integration.BonusInvalidRewardProtection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChapter3BonusInvalidRewardTest::RunTest(const FString& Parameters)
{
    using namespace Chapter3BonusPickupTests;
    FFixture Fixture;
    if (!Fixture.Validate(*this)) return false;
    ABonusPickup* Cash = Fixture.AddPickup(10, 5);
    if (!TestNotNull(TEXT("Spawn cash before progress initialization"), Cash)) return false;
    TestFalse(TEXT("Uninitialized progress rejects money mutation"), Fixture.Progress->AddMoney(10));
    TestFalse(TEXT("Uninitialized progress rejects combined pickup"), Cash->TryCollect(Fixture.Character));
    TestFalse(TEXT("Rejected pickup remains available"), Cash->IsActorBeingDestroyed());
    TestEqual(TEXT("Rejected money does not partially award score"), Fixture.Mode->GetCurrentScore(), 0);
    if (!TestTrue(TEXT("Initialize isolated save"), Fixture.Progress->InitializeProgress(Fixture.Slot))) return false;
    TestFalse(TEXT("Negative money addition is rejected"), Fixture.Progress->AddMoney(-1));
    TestFalse(TEXT("Zero money addition is rejected"), Fixture.Progress->AddMoney(0));
    const TPair<int64, int32> InvalidRewards[] = {{-1, 5}, {10, -1}, {0, 0}};
    for (const TPair<int64, int32>& Reward : InvalidRewards)
    {
        ABonusPickup* Invalid = Fixture.AddPickup(Reward.Key, Reward.Value);
        if (!TestNotNull(TEXT("Spawn invalid reward pickup"), Invalid)) return false;
        TestFalse(TEXT("Invalid reward rejects collection"), Invalid->TryCollect(Fixture.Character));
        TestFalse(TEXT("Invalid reward is not consumed"), Invalid->IsActorBeingDestroyed());
    }
    TestEqual(TEXT("Invalid rewards do not change money"), Fixture.Progress->GetMoney(), int64(0));
    TestEqual(TEXT("Invalid rewards do not change score"), Fixture.Mode->GetCurrentScore(), 0);

    Fixture.Mode->AddScore(MAX_int32);
    TestFalse(TEXT("Score overflow rejects combined reward"), Cash->TryCollect(Fixture.Character));
    TestFalse(TEXT("Score overflow preserves pickup"), Cash->IsActorBeingDestroyed());
    TestEqual(TEXT("Score overflow does not partially award money"), Fixture.Progress->GetMoney(), int64(0));
    TestEqual(TEXT("Score stays at its valid maximum"), Fixture.Mode->GetCurrentScore(), MAX_int32);

    if (!TestTrue(TEXT("Fill persistent money to maximum"), Fixture.Progress->AddMoney(MAX_int64))) return false;
    TestFalse(TEXT("Money overflow is rejected"), Fixture.Progress->AddMoney(1));
    ABonusPickup* OverflowCash = Fixture.AddPickup(1, 0);
    if (!TestNotNull(TEXT("Spawn overflowing cash"), OverflowCash)) return false;
    TestFalse(TEXT("Money overflow rejects pickup"), OverflowCash->TryCollect(Fixture.Character));
    TestFalse(TEXT("Money overflow preserves pickup"), OverflowCash->IsActorBeingDestroyed());
    TestEqual(TEXT("Money remains at its valid maximum"), Fixture.Progress->GetMoney(), MAX_int64);
    return true;
}

#endif
