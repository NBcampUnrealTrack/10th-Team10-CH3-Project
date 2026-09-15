#if WITH_DEV_AUTOMATION_TESTS

#include "ShootingPlayerController.h"
#include "M1911WeaponView.h"
#include "WeaponAttachment.h"
#include "WeaponAttachmentComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

namespace
{
    class FWeaponSwitchTestWorld
    {
    public:
        FWeaponSwitchTestWorld()
        {
            const UWorld::InitializationValues values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
            world_ = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
                true, ERHIFeatureLevel::Num, &values);
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world_);
            world_->InitializeActorsForPlay(FURL());
            world_->GetWorldSettings()->NotifyBeginPlay();
            world_->BeginPlay();
        }

        ~FWeaponSwitchTestWorld()
        {
            world_->EndPlay(EEndPlayReason::Quit);
            GEngine->DestroyWorldContext(world_);
            world_->DestroyWorld(false);
        }

        AChapter3_ShooterGame_PlayerController* CreateController(int32 weaponCount, bool useLegacy = false)
        {
            AChapter3_ShooterGame_PlayerController* controller = world_->SpawnActorDeferred<AChapter3_ShooterGame_PlayerController>(
                AChapter3_ShooterGame_PlayerController::StaticClass(), FTransform::Identity);
            if (useLegacy)
            {
                FClassProperty* property = FindFProperty<FClassProperty>(controller->GetClass(), TEXT("weaponViewClass_"));
                property->SetObjectPropertyValue_InContainer(controller, AM1911WeaponView::StaticClass());
            }
            else
            {
                FArrayProperty* property = FindFProperty<FArrayProperty>(controller->GetClass(), TEXT("weaponViewClasses_"));
                auto* classes = property->ContainerPtrToValuePtr<TArray<TSubclassOf<AM1911WeaponView>>>(controller);
                // None 항목은 인벤토리 초기화에서 제외되어야 한다.
                classes->Add(nullptr);
                for (int32 index = 0; index < weaponCount; ++index)
                {
                    classes->Add(AM1911WeaponView::StaticClass());
                }
            }
            controller->FinishSpawning(FTransform::Identity);
            APawn* pawn = world_->SpawnActor<APawn>();
            controller->Possess(pawn);
            return controller;
        }

        void Advance(float seconds)
        {
            for (float elapsed = 0.0f; elapsed < seconds; elapsed += 0.05f)
            {
                world_->Tick(LEVELTICK_All, 0.05f);
            }
        }

        UWorld* world_ = nullptr;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponSwitchLifecycleTest, "CH3.WeaponSwitch.LifecycleAndAmmo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponSwitchLifecycleTest::RunTest(const FString& parameters)
{
    FWeaponSwitchTestWorld testWorld = {};
    AChapter3_ShooterGame_PlayerController* controller = testWorld.CreateController(3);
    TestTrue(TEXT("Initial weapon equips"), controller->EquipWeaponAtIndex(0));
    AM1911WeaponView* first = controller->GetCurrentWeapon();
    if (!TestNotNull(TEXT("First weapon exists"), first))
    {
        return false;
    }

    FWeaponAttachmentSlot attachmentSlot = {};
    attachmentSlot.slotName_ = TEXT("TestOptic");
    first->attachmentComponent_->attachmentSlots_.Add(attachmentSlot);
    AWeaponAttachment* attachment = first->attachmentComponent_->EquipAttachment(TEXT("TestOptic"), AWeaponAttachment::StaticClass());
    if (!TestNotNull(TEXT("Attachment equipped"), attachment))
    {
        return false;
    }
    controller->Fire();
    TestEqual(TEXT("First weapon spends one round"), controller->GetCurrentAmmo(), 9);
    controller->StartReload();
    TestTrue(TEXT("Reload starts"), controller->IsReloading());
    controller->NextWeapon();
    AM1911WeaponView* second = controller->GetCurrentWeapon();
    TestEqual(TEXT("Forward selects next slot"), controller->GetEquippedWeaponIndex(), 1);
    TestTrue(TEXT("Old gun and attachment hidden"), first->IsHidden() && attachment->IsHidden());
    TestFalse(TEXT("Switch cancels reload"), controller->IsReloading());
    TestEqual(TEXT("Second weapon starts full"), controller->GetCurrentAmmo(), 10);
    testWorld.Advance(0.25f);
    controller->Fire();
    TestEqual(TEXT("Second weapon spends its own round"), controller->GetCurrentAmmo(), 9);
    testWorld.Advance(2.0f);
    TestEqual(TEXT("Old reload timer cannot refill second weapon"), controller->GetCurrentAmmo(), 9);
    controller->PreviousWeapon();
    TestTrue(TEXT("Weapon instance and attachment preserved"), controller->GetCurrentWeapon() == first
        && first->attachmentComponent_->GetAttachment(TEXT("TestOptic")) == attachment);
    TestFalse(TEXT("Attachment visible again"), attachment->IsHidden());
    TestEqual(TEXT("First weapon ammo preserved"), controller->GetCurrentAmmo(), 9);
    TestFalse(TEXT("Invalid index rejected"), controller->EquipWeaponAtIndex(99));
    TestTrue(TEXT("Invalid switch preserves weapon"), controller->GetCurrentWeapon() == first);
    controller->PreviousWeapon();
    AM1911WeaponView* third = controller->GetCurrentWeapon();
    TestEqual(TEXT("Reverse wraps to final slot"), controller->GetEquippedWeaponIndex(), 2);
    controller->NextWeapon();
    TestEqual(TEXT("Forward wraps to first slot"), controller->GetEquippedWeaponIndex(), 0);
    controller->UnPossess();
    controller->NextWeapon();
    TestEqual(TEXT("Unpossessed controller cannot switch"), controller->GetEquippedWeaponIndex(), 0);
    controller->Destroy();
    TestFalse(TEXT("Controller destruction cleans up all guns and attachments"),
        IsValid(first) || IsValid(second) || IsValid(third) || IsValid(attachment));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponSwitchLegacyTest, "CH3.WeaponSwitch.LegacyAndEmptyLoadout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponSwitchLegacyTest::RunTest(const FString& parameters)
{
    FWeaponSwitchTestWorld testWorld = {};
    AChapter3_ShooterGame_PlayerController* legacy = testWorld.CreateController(0, true);
    TestTrue(TEXT("Old single-class setting still equips"), legacy->EquipWeaponAtIndex(0));
    AM1911WeaponView* original = legacy->GetCurrentWeapon();
    legacy->Fire();
    legacy->NextWeapon();
    legacy->PreviousWeapon();
    TestTrue(TEXT("One-weapon scrolling does not respawn"), legacy->GetCurrentWeapon() == original);
    TestEqual(TEXT("One-weapon scrolling does not refill"), legacy->GetCurrentAmmo(), 9);

    AChapter3_ShooterGame_PlayerController* empty = testWorld.CreateController(0);
    empty->NextWeapon();
    TestFalse(TEXT("Empty loadout cannot equip"), empty->EquipWeaponAtIndex(0));
    TestNull(TEXT("Empty loadout has no actor"), empty->GetCurrentWeapon());
    return true;
}

#endif
