#if WITH_DEV_AUTOMATION_TESTS

#include "WeaponAttachment.h"
#include "WeaponAttachmentComponent.h"
#include "M1911WeaponView.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"

namespace
{
    class FAttachmentTestWorld
    {
    public:
        explicit FAttachmentTestWorld(EWorldType::Type worldType = EWorldType::Game)
        {
            const UWorld::InitializationValues values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
            world_ = UWorld::CreateWorld(worldType, false, NAME_None, nullptr,
                true, ERHIFeatureLevel::Num, &values);
            GEngine->CreateNewWorldContext(worldType).SetCurrentWorld(world_);
            if (world_->IsGameWorld())
            {
                world_->InitializeActorsForPlay(FURL());
                world_->GetWorldSettings()->NotifyBeginPlay();
                world_->BeginPlay();
            }
        }

        ~FAttachmentTestWorld()
        {
            if (world_->HasBegunPlay())
            {
                world_->EndPlay(EEndPlayReason::Quit);
            }
            GEngine->DestroyWorldContext(world_);
            world_->DestroyWorld(false);
        }

        AActor* CreateWeapon() const
        {
            AActor* weapon = world_->SpawnActor<AActor>();
            USceneComponent* root = NewObject<USceneComponent>(weapon);
            weapon->SetRootComponent(root);
            weapon->AddInstanceComponent(root);
            root->RegisterComponent();
            return weapon;
        }

        UWeaponAttachmentComponent* CreateManager(AActor* weapon, bool useDefault = false) const
        {
            UWeaponAttachmentComponent* manager = NewObject<UWeaponAttachmentComponent>(weapon);
            weapon->AddInstanceComponent(manager);
            FWeaponAttachmentSlot muzzleSlot = {};
            muzzleSlot.slotName_ = TEXT("Muzzle");
            muzzleSlot.relativeTransform_.SetTranslation(FVector(25.0f, 0.0f, 0.0f));
            muzzleSlot.defaultAttachmentClass_ = useDefault ? AWeaponAttachment::StaticClass() : nullptr;
            FWeaponAttachmentSlot opticSlot = {};
            opticSlot.slotName_ = TEXT("Optic");
            manager->attachmentSlots_ = { muzzleSlot, opticSlot };
            manager->SetAttachmentTarget(weapon->GetRootComponent());
            manager->RegisterComponent();
            return manager;
        }

        UWorld* world_ = nullptr;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponAttachmentLifecycleTest, "CH3.WeaponAttachment.Lifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponAttachmentLifecycleTest::RunTest(const FString& parameters)
{
    FAttachmentTestWorld testWorld{};
    AActor* weapon = testWorld.CreateWeapon();
    UWeaponAttachmentComponent* manager = testWorld.CreateManager(weapon);
    const FName muzzleSlot(TEXT("Muzzle"));
    const FName opticSlot(TEXT("Optic"));
    AWeaponAttachment* muzzle = manager->EquipAttachment(muzzleSlot, AWeaponAttachment::StaticClass());
    AWeaponAttachment* optic = manager->EquipAttachment(opticSlot, AWeaponAttachment::StaticClass());
    if (!TestNotNull(TEXT("Muzzle equipped"), muzzle) || !TestNotNull(TEXT("Optic equipped"), optic))
    {
        return false;
    }
    TestTrue(TEXT("Attachment knows weapon and slot"), muzzle->GetAttachedWeapon() == weapon && muzzle->GetAttachmentSlot() == muzzleSlot);
    TestTrue(TEXT("Owner chain supports first person visibility"), muzzle->GetOwner() == weapon && muzzle->attachmentMesh_->bOnlyOwnerSee);
    TestTrue(TEXT("Equipped mesh does not block traces"), muzzle->attachmentMesh_->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
    weapon->SetActorLocationAndRotation(FVector(100.0f, 50.0f, 10.0f), FRotator(0.0f, 90.0f, 0.0f));
    TestTrue(TEXT("Attachment follows translated and rotated weapon"), muzzle->GetActorLocation().Equals(FVector(100.0f, 75.0f, 10.0f)));

    AddExpectedError(TEXT("Cannot equip slot"), EAutomationExpectedErrorFlags::Contains, 3);
    TestNull(TEXT("Unknown slot rejected"), manager->EquipAttachment(TEXT("Unknown"), AWeaponAttachment::StaticClass()));
    TestNull(TEXT("Empty class rejected"), manager->EquipAttachment(muzzleSlot, nullptr));
    manager->attachmentSlots_[0].socketName_ = TEXT("MissingSocket");
    TestNull(TEXT("Missing socket rejected"), manager->EquipAttachment(muzzleSlot, AWeaponAttachment::StaticClass()));
    TestTrue(TEXT("Failed replacement preserves existing attachment"), manager->GetAttachment(muzzleSlot) == muzzle);
    manager->attachmentSlots_[0].socketName_ = NAME_None;

    AWeaponAttachment* replacement = manager->EquipAttachment(muzzleSlot, AWeaponAttachment::StaticClass());
    TestTrue(TEXT("Successful replacement destroys previous actor"), IsValid(replacement) && !IsValid(muzzle));
    TestTrue(TEXT("Other slot unaffected by replacement"), manager->GetAttachment(opticSlot) == optic);
    TestTrue(TEXT("Unequip succeeds"), manager->UnequipAttachment(muzzleSlot));
    TestNull(TEXT("Unequip clears slot"), manager->GetAttachment(muzzleSlot));
    TestFalse(TEXT("Unequipped actor destroyed"), IsValid(replacement));
    TestFalse(TEXT("Repeated unequip is safe"), manager->UnequipAttachment(muzzleSlot));

    optic->Destroy();
    TestNull(TEXT("External destruction cannot leave usable stale entry"), manager->GetAttachment(opticSlot));
    AWeaponAttachment* newOptic = manager->EquipAttachment(opticSlot, AWeaponAttachment::StaticClass());
    TestNotNull(TEXT("Slot reusable after external destruction"), newOptic);
    weapon->Destroy();
    TestFalse(TEXT("Destroying weapon destroys owned attachment"), IsValid(newOptic));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponAttachmentDefaultsTest, "CH3.WeaponAttachment.DefaultsAndTargetSwitch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponAttachmentDefaultsTest::RunTest(const FString& parameters)
{
    FAttachmentTestWorld testWorld{};
    AActor* weapon = testWorld.CreateWeapon();
    UWeaponAttachmentComponent* manager = testWorld.CreateManager(weapon, true);
    AWeaponAttachment* attachment = manager->GetAttachment(TEXT("Muzzle"));
    if (!TestNotNull(TEXT("Configured default equipped at BeginPlay"), attachment))
    {
        return false;
    }
    USceneComponent* reloadTarget = NewObject<USceneComponent>(weapon);
    reloadTarget->SetupAttachment(weapon->GetRootComponent());
    weapon->AddInstanceComponent(reloadTarget);
    reloadTarget->RegisterComponent();
    reloadTarget->SetRelativeLocation(FVector(0.0f, 0.0f, -20.0f));
    TestTrue(TEXT("Switch to reload mesh succeeds"), manager->SetAttachmentTarget(reloadTarget));
    TestTrue(TEXT("Switch preserves attachment instance and local offset"), manager->GetAttachment(TEXT("Muzzle")) == attachment
        && attachment->GetRootComponent()->GetAttachParent() == reloadTarget
        && attachment->GetActorLocation().Equals(FVector(25.0f, 0.0f, -20.0f)));
    TestFalse(TEXT("Null target rejected"), manager->SetAttachmentTarget(nullptr));
    AActor* otherWeapon = testWorld.CreateWeapon();
    TestFalse(TEXT("Target belonging to another weapon rejected"), manager->SetAttachmentTarget(otherWeapon->GetRootComponent()));
    TestTrue(TEXT("Switch back succeeds"), manager->SetAttachmentTarget(weapon->GetRootComponent()));

    const FWeaponAttachmentSlot duplicateSlot = manager->attachmentSlots_[0];
    manager->attachmentSlots_.Add(duplicateSlot);
    AddExpectedError(TEXT("Duplicate attachment slot"), EAutomationExpectedErrorFlags::Contains, 1);
    AddExpectedError(TEXT("Cannot equip slot"), EAutomationExpectedErrorFlags::Contains, 1);
    TestNull(TEXT("Ambiguous duplicate slot rejected"), manager->EquipAttachment(TEXT("Muzzle"), AWeaponAttachment::StaticClass()));
    TestTrue(TEXT("Duplicate configuration does not remove existing attachment"), manager->GetAttachment(TEXT("Muzzle")) == attachment);
    manager->DestroyComponent();
    TestFalse(TEXT("Destroying manager cleans up attachment"), IsValid(attachment));
    return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponAttachmentPreviewTest, "CH3.WeaponAttachment.EditorPreview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponAttachmentPreviewTest::RunTest(const FString& parameters)
{
    FAttachmentTestWorld testWorld(EWorldType::EditorPreview);
    AM1911WeaponView* weapon = testWorld.world_->SpawnActorDeferred<AM1911WeaponView>(
        AM1911WeaponView::StaticClass(), FTransform::Identity);
    UWeaponAttachmentComponent* manager = weapon->attachmentComponent_;
    FWeaponAttachmentSlot slot = {};
    slot.slotName_ = TEXT("Optic");
    slot.defaultAttachmentClass_ = AWeaponAttachment::StaticClass();
    slot.relativeTransform_ = FTransform(FRotator(0.0f, 30.0f, 0.0f), FVector(10.0f, 0.0f, 8.0f), FVector(0.5f));
    manager->attachmentSlots_.Add(slot);
    weapon->FinishSpawning(FTransform::Identity);

    TArray<UChildActorComponent*> previews;
    weapon->GetComponents<UChildActorComponent>(previews);
    if (!TestEqual(TEXT("Construction creates one preview"), previews.Num(), 1))
    {
        return false;
    }
    AWeaponAttachment* previewActor = Cast<AWeaponAttachment>(previews[0]->GetChildActor());
    if (!TestNotNull(TEXT("Preview instantiates attachment class"), previewActor))
    {
        return false;
    }
    TestTrue(TEXT("Preview is transient and editor only"), previews[0]->IsEditorOnly()
        && previews[0]->HasAllFlags(RF_Transient | RF_DuplicateTransient) && previewActor->IsEditorOnly());
    TestTrue(TEXT("Preview uses slot position rotation and scale"), previews[0]->GetRelativeTransform().Equals(slot.relativeTransform_));
    TestNull(TEXT("Preview does not equip gameplay attachment"), manager->GetAttachment(slot.slotName_));
    TestNull(TEXT("Preview does not invoke equipped state"), previewActor->GetAttachedWeapon());
    TestFalse(TEXT("Editor camera can see preview mesh"), previewActor->attachmentMesh_->bOnlyOwnerSee);

    manager->attachmentSlots_[0].relativeTransform_.SetLocation(FVector(0.0f, 0.0f, 20.0f));
    manager->RefreshAttachmentPreview();
    previews.Reset();
    weapon->GetComponents<UChildActorComponent>(previews);
    TestEqual(TEXT("Refresh replaces instead of duplicating preview"), previews.Num(), 1);
    TestFalse(TEXT("Previous preview actor destroyed"), IsValid(previewActor));
    if (previews.Num() == 1)
    {
        TestTrue(TEXT("Transform edit is reflected"), previews[0]->GetRelativeLocation().Equals(FVector(0.0f, 0.0f, 20.0f)));
    }
    weapon->RerunConstructionScripts();
    previews.Reset();
    weapon->GetComponents<UChildActorComponent>(previews);
    TestEqual(TEXT("Construction rerun does not duplicate preview"), previews.Num(), 1);

    manager = weapon->attachmentComponent_;
    manager->attachmentSlots_[0].socketName_ = TEXT("MissingSocket");
    manager->RefreshAttachmentPreview();
    previews.Reset();
    weapon->GetComponents<UChildActorComponent>(previews);
    TestEqual(TEXT("Invalid socket has no misleading preview"), previews.Num(), 0);
    manager->attachmentSlots_[0].socketName_ = NAME_None;
    manager->showAttachmentPreview_ = false;
    manager->RefreshAttachmentPreview();
    previews.Reset();
    weapon->GetComponents<UChildActorComponent>(previews);
    TestEqual(TEXT("Preview toggle disables visuals"), previews.Num(), 0);

    FAttachmentTestWorld gameWorld;
    AActor* gameWeapon = gameWorld.CreateWeapon();
    UWeaponAttachmentComponent* gameManager = gameWorld.CreateManager(gameWeapon, true);
    gameManager->RefreshAttachmentPreview();
    previews.Reset();
    gameWeapon->GetComponents<UChildActorComponent>(previews);
    TestEqual(TEXT("Game world never creates preview components"), previews.Num(), 0);
    TestNotNull(TEXT("Game default attachment still equips"), gameManager->GetAttachment(TEXT("Muzzle")));
    return true;
}
#endif

#endif
