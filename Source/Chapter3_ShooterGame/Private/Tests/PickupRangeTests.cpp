#if WITH_DEV_AUTOMATION_TESTS

#include "Chapter3GameInstance.h"
#include "Chapter3_ShooterGame_Character.h"
#include "UnlockInventoryComponent.h"
#include "UnlockInventoryPickup.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

namespace Chapter3PickupRangeTests
{
struct FFixture
{
    const FString Slot = TEXT("CH3_Automation_PickupRange_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    TStrongObjectPtr<UChapter3GameInstance> Progress{NewObject<UChapter3GameInstance>()};
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AChapter3_ShooterGame_Character* Character = nullptr;
    UUnlockInventoryComponent* Inventory = nullptr;

    FFixture()
    {
        if (!World) return;
        World->SetGameInstance(Progress.Get());
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Character = World->SpawnActor<AChapter3_ShooterGame_Character>(
            FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
        APlayerController* Controller = World->SpawnActor<APlayerController>();
        if (!Character || !Controller) return;
        Character->GetCapsuleComponent()->SetCapsuleSize(30.0f, 90.0f);
        Character->GetCapsuleComponent()->SetGenerateOverlapEvents(false);
        Controller->Possess(Character);
        Inventory = NewObject<UUnlockInventoryComponent>(Controller);
        Controller->AddInstanceComponent(Inventory);
        Inventory->RegisterComponent();
    }

    ~FFixture()
    {
        if (World) World->DestroyWorld(false);
        UGameplayStatics::DeleteGameInSlot(Slot, 0);
    }

    AUnlockInventoryPickup* AddPickup(FName ItemId, const FVector& Location, float Radius)
    {
        FUnlockInventoryItem Item;
        Item.itemId_ = ItemId;
        Inventory->itemDefinitions_.Add(Item);
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AUnlockInventoryPickup* Pickup = World->SpawnActor<AUnlockInventoryPickup>(
            Location, FRotator::ZeroRotator, SpawnParameters);
        if (Pickup)
        {
            Pickup->itemId_ = ItemId;
            Pickup->pickupRadius_ = Radius;
            Pickup->pickupSphere_->SetSphereRadius(Radius);
            Pickup->pickupSphere_->SetGenerateOverlapEvents(false);
        }
        return Pickup;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter3PickupWithoutOverlapTest,
    "CH3.Progress.Integration.PickupWithoutOverlap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChapter3PickupWithoutOverlapTest::RunTest(const FString& Parameters)
{
    using namespace Chapter3PickupRangeTests;
    FFixture Fixture;
    if (!TestNotNull(TEXT("Create character"), Fixture.Character)
        || !TestNotNull(TEXT("Create player inventory"), Fixture.Inventory)) return false;
    if (!TestTrue(TEXT("Initialize isolated progress"), Fixture.Progress->InitializeProgress(Fixture.Slot))) return false;
    const FName NearId(TEXT("Range_NearLetter"));
    const FName FarId(TEXT("Range_FarPhoto"));
    // Spawn the farther item first so actor iteration order cannot satisfy the nearest-item assertion.
    AUnlockInventoryPickup* Far = Fixture.AddPickup(FarId, FVector(100.0, 0.0, 0.0), 150.0f);
    AUnlockInventoryPickup* Near = Fixture.AddPickup(NearId, FVector(35.0, 0.0, 0.0), 150.0f);
    if (!TestNotNull(TEXT("Spawn farther pickup"), Far) || !TestNotNull(TEXT("Spawn nearer pickup"), Near)) return false;
    TArray<AActor*> Overlaps;
    Fixture.Character->GetOverlappingActors(Overlaps);
    TestEqual(TEXT("The character overlap cache is empty"), Overlaps.Num(), 0);
    TestFalse(TEXT("The nearby sphere has no player overlap"), Near->pickupSphere_->IsOverlappingActor(Fixture.Character));

    Fixture.Character->Interact();

    TestTrue(TEXT("Interact collects the nearest item despite disabled overlap events"), Fixture.Progress->HasCollectedItem(NearId));
    TestTrue(TEXT("Collected pickup is destroyed"), Near->IsActorBeingDestroyed());
    TestFalse(TEXT("One interaction does not collect the second item"), Fixture.Progress->HasCollectedItem(FarId));
    TestFalse(TEXT("The second pickup remains"), Far->IsActorBeingDestroyed());
    TStrongObjectPtr<UChapter3GameInstance> Reloaded(NewObject<UChapter3GameInstance>());
    if (!TestTrue(TEXT("Reload collected progress from disk"), Reloaded->InitializeProgress(Fixture.Slot))) return false;
    TestTrue(TEXT("Actual interaction persists the collected item"), Reloaded->HasCollectedItem(NearId));
    TestFalse(TEXT("Uncollected item remains absent on disk"), Reloaded->HasCollectedItem(FarId));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapter3PickupRangeBoundaryTest,
    "CH3.Progress.Integration.PickupRangeBoundary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChapter3PickupRangeBoundaryTest::RunTest(const FString& Parameters)
{
    using namespace Chapter3PickupRangeTests;
    FFixture Fixture;
    if (!TestNotNull(TEXT("Create character"), Fixture.Character)
        || !TestNotNull(TEXT("Create player inventory"), Fixture.Inventory)) return false;
    if (!TestTrue(TEXT("Initialize isolated progress"), Fixture.Progress->InitializeProgress(Fixture.Slot))) return false;
    const FName ItemId(TEXT("Range_ScaledDossier"));
    AUnlockInventoryPickup* Pickup = Fixture.AddPickup(ItemId, FVector(131.0, 0.0, 0.0), 50.0f);
    if (!TestNotNull(TEXT("Spawn scaled pickup"), Pickup)) return false;
    Pickup->SetActorScale3D(FVector(2.0));
    TestEqual(TEXT("Actor scale doubles the collection sphere radius"), Pickup->pickupSphere_->GetScaledSphereRadius(), 100.0f);
    TestFalse(TEXT("Outside sphere plus capsule radius is rejected"), Pickup->IsCollectorInRange(Fixture.Character));
    TestFalse(TEXT("Out-of-range direct collection is rejected"), Pickup->TryCollect(Fixture.Character));
    TestFalse(TEXT("Rejected pickup is not consumed"), Pickup->IsActorBeingDestroyed());
    TestFalse(TEXT("Rejected collection does not alter progress"), Fixture.Progress->HasCollectedItem(ItemId));

    // Capsule radius is 30: x=129 is reachable only when the pickup scale is applied.
    Pickup->SetActorLocation(FVector(129.0, 0.0, 0.0));
    TestTrue(TEXT("Scaled collection sphere includes the capsule surface"), Pickup->IsCollectorInRange(Fixture.Character));
    // Capsule half-height is 90: its top cap is reachable at z=189 but not z=191.
    Pickup->SetActorLocation(FVector(0.0, 0.0, 189.0));
    TestTrue(TEXT("Vertical reach includes capsule height"), Pickup->IsCollectorInRange(Fixture.Character));
    Pickup->SetActorLocation(FVector(0.0, 0.0, 191.0));
    TestFalse(TEXT("Above the capsule and sphere is outside reach"), Pickup->IsCollectorInRange(Fixture.Character));
    Pickup->SetActorLocation(FVector(129.0, 0.0, 0.0));
    TestTrue(TEXT("Direct collection succeeds inside the scaled range"), Pickup->TryCollect(Fixture.Character));
    TestTrue(TEXT("Successful collection records progress"), Fixture.Progress->HasCollectedItem(ItemId));
    return true;
}

#endif
