#if WITH_DEV_AUTOMATION_TESTS

#include "SlowMotionSkillComponent.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

namespace
{
    constexpr float kTestStep = 0.01f;
    constexpr float kTestDuration = 0.3f;
    constexpr float kTestCooldown = 0.4f;
    constexpr float kTestBaseTimeDilation = 0.75f;
    constexpr float kTestSlowTimeDilation = 0.15f;
    constexpr int32 kDurationFrames = 35;
    constexpr int32 kCooldownFrames = 45;

    class FSlowMotionTestWorld
    {
    public:
        FSlowMotionTestWorld()
        {
            const UWorld::InitializationValues values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
            world_ = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
                true, ERHIFeatureLevel::Num, &values);
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world_);
            world_->InitializeActorsForPlay(FURL());
            world_->GetWorldSettings()->NotifyBeginPlay();
            world_->BeginPlay();
        }

        ~FSlowMotionTestWorld()
        {
            world_->EndPlay(EEndPlayReason::Quit);
            GEngine->DestroyWorldContext(world_);
            world_->DestroyWorld(false);
        }

        void AdvanceFrames(int32 frameCount) const
        {
            for (int32 frameIndex = 0; frameIndex < frameCount; ++frameIndex)
            {
                ++GFrameCounter;
                world_->Tick(LEVELTICK_All, kTestStep);
            }
        }

        USlowMotionSkillComponent* CreateSkill() const
        {
            AActor* owner = world_->SpawnActor<AActor>();
            USlowMotionSkillComponent* skill = NewObject<USlowMotionSkillComponent>(owner);
            owner->AddInstanceComponent(skill);
            skill->RegisterComponent();
            FindFProperty<FFloatProperty>(skill->GetClass(), TEXT("duration_"))->SetPropertyValue_InContainer(skill, kTestDuration);
            FindFProperty<FFloatProperty>(skill->GetClass(), TEXT("cooldown_"))->SetPropertyValue_InContainer(skill, kTestCooldown);
            return skill;
        }

        UWorld* world_ = nullptr;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlowMotionLifecycleTest, "CH3.Skills.SlowMotionLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSlowMotionLifecycleTest::RunTest(const FString& parameters)
{
    FSlowMotionTestWorld testWorld = {};
    USlowMotionSkillComponent* skill = testWorld.CreateSkill();
    USlowMotionSkillComponent* otherSkill = testWorld.CreateSkill();
    UGameplayStatics::SetGlobalTimeDilation(testWorld.world_, kTestBaseTimeDilation);
    TestTrue(TEXT("Skill activates"), skill->TryActivateSlowMotion());
    TestEqual(TEXT("Time is slowed relative to previous scale"),
        UGameplayStatics::GetGlobalTimeDilation(testWorld.world_), kTestSlowTimeDilation);
    TestFalse(TEXT("Active skill cannot activate twice"), skill->TryActivateSlowMotion());
    TestFalse(TEXT("Another skill cannot overlap global slow motion"), otherSkill->TryActivateSlowMotion());
    testWorld.AdvanceFrames(kDurationFrames);
    TestEqual(TEXT("Real duration expires even while game time is slowed"), skill->GetRemainingDuration(), 0.0f);
    TestEqual(TEXT("Original time dilation is restored"),
        UGameplayStatics::GetGlobalTimeDilation(testWorld.world_), kTestBaseTimeDilation);
    TestTrue(TEXT("Cooldown starts on expiry"), skill->GetRemainingCooldown() > 0.0f);
    TestFalse(TEXT("Cooldown rejects activation"), skill->TryActivateSlowMotion());
    testWorld.AdvanceFrames(kCooldownFrames);
    TestTrue(TEXT("Cooldown uses real time"), skill->TryActivateSlowMotion());
    skill->CancelSlowMotion();
    TestEqual(TEXT("Cancel restores time"),
        UGameplayStatics::GetGlobalTimeDilation(testWorld.world_), kTestBaseTimeDilation);
    TestTrue(TEXT("Cancel also starts cooldown"), skill->GetRemainingCooldown() > 0.0f);
    TestTrue(TEXT("Other skill can activate after release"), otherSkill->TryActivateSlowMotion());
    otherSkill->GetOwner()->Destroy();
    TestEqual(TEXT("Owner destruction restores time"),
        UGameplayStatics::GetGlobalTimeDilation(testWorld.world_), kTestBaseTimeDilation);
    return true;
}

#endif
