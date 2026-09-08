#if WITH_DEV_AUTOMATION_TESTS

#include "DistractionCoin.h"

#include "AIController.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "UObject/UnrealType.h"

namespace
{
    constexpr float kTestStep = 1.0f / 60.0f;
    constexpr int32 kFlightTestFrames = 180;
    constexpr float kTestHearingRange = 5000.0f;
    constexpr float kTestFloorHalfSize = 5000.0f;
    constexpr float kTestFloorHalfHeight = 10.0f;

    class FCoinTestWorld
    {
    public:
        FCoinTestWorld()
        {
            const UWorld::InitializationValues values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
                .CreateAISystem(true).ShouldSimulatePhysics(true).SetTransactional(false);
            world_ = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
                true, ERHIFeatureLevel::Num, &values);
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world_);
            world_->InitializeActorsForPlay(FURL());
            world_->GetWorldSettings()->NotifyBeginPlay();
            world_->BeginPlay();
        }

        ~FCoinTestWorld()
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
                if (UAIPerceptionSystem* perception = UAIPerceptionSystem::GetCurrent(world_))
                {
                    perception->Tick(kTestStep);
                }
            }
        }

        UWorld* world_ = nullptr;
    };

    UAIPerceptionComponent* CreateListener(UWorld* world, const FVector& location)
    {
        AAIController* controller = world->SpawnActor<AAIController>();
        APawn* pawn = world->SpawnActor<APawn>();
        USceneComponent* root = NewObject<USceneComponent>(pawn);
        pawn->SetRootComponent(root);
        pawn->AddInstanceComponent(root);
        root->RegisterComponent();
        pawn->SetActorLocation(location);
        controller->Possess(pawn);
        UAIPerceptionComponent* perception = NewObject<UAIPerceptionComponent>(controller);
        UAISenseConfig_Hearing* hearing = NewObject<UAISenseConfig_Hearing>(perception);
        hearing->HearingRange = kTestHearingRange;
        hearing->DetectionByAffiliation.bDetectEnemies = true;
        hearing->DetectionByAffiliation.bDetectFriendlies = true;
        hearing->DetectionByAffiliation.bDetectNeutrals = true;
        perception->ConfigureSense(*hearing);
        controller->AddInstanceComponent(perception);
        perception->RegisterComponent();
        return perception;
    }

    void CreateFloor(UWorld* world)
    {
        AActor* floor = world->SpawnActor<AActor>();
        UBoxComponent* box = NewObject<UBoxComponent>(floor);
        floor->SetRootComponent(box);
        floor->AddInstanceComponent(box);
        box->SetBoxExtent(FVector(kTestFloorHalfSize, kTestFloorHalfSize, kTestFloorHalfHeight));
        box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        box->SetCollisionObjectType(ECC_WorldStatic);
        box->SetCollisionResponseToAllChannels(ECR_Block);
        box->RegisterComponent();
        floor->SetActorLocation(FVector(0.0f, 0.0f, -kTestFloorHalfHeight));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoinLandingTest, "CH3.Coin.PhysicsAndHearing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoinLandingTest::RunTest(const FString& parameters)
{
    FCoinTestWorld testWorld = {};
    UWorld* world = testWorld.world_;
    UClass* coinClass = LoadClass<ADistractionCoin>(nullptr,
        TEXT("/Game/Shooting/Blueprints/BP_DistractionCoin.BP_DistractionCoin_C"));
    if (!TestNotNull(TEXT("Configured coin Blueprint loads"), coinClass))
    {
        return false;
    }
    CreateFloor(world);
    UAIPerceptionComponent* nearListener = CreateListener(world, FVector(400.0f, 0.0f, 100.0f));
    UAIPerceptionComponent* farListener = CreateListener(world, FVector(4000.0f, 0.0f, 100.0f));
    testWorld.AdvanceFrames(2);
    if (!TestNotNull(TEXT("AI perception system exists"), UAIPerceptionSystem::GetCurrent(world)))
    {
        return false;
    }

    ADistractionCoin* coin = world->SpawnActor<ADistractionCoin>(
        coinClass, FVector(0.0f, 0.0f, 160.0f), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Coin spawned"), coin))
    {
        return false;
    }
    TestNotNull(TEXT("Downloaded coin mesh loads"), coin->FindComponentByClass<UStaticMeshComponent>()->GetStaticMesh().Get());
    UProjectileMovementComponent* movement = coin->FindComponentByClass<UProjectileMovementComponent>();
    coin->LaunchCoin(FVector(1000.0f, 0.0f, 300.0f));
    testWorld.AdvanceFrames(10);
    AddInfo(FString::Printf(TEXT("Flight: location=%s velocity=%s active=%d begunPlay=%d"),
        *coin->GetActorLocation().ToString(), *movement->Velocity.ToString(), movement->IsActive(), coin->HasActorBegunPlay()));
    TestTrue(TEXT("Coin moves forward and initially rises"),
        coin->GetActorLocation().X > 50.0f && coin->GetActorLocation().Z > 160.0f);
    FActorPerceptionBlueprintInfo info = {};
    TestFalse(TEXT("No distraction during flight"), nearListener->GetActorsPerception(coin, info));

    testWorld.AdvanceFrames(kFlightTestFrames);
    AddInfo(FString::Printf(TEXT("Landing: location=%s velocity=%s"),
        *coin->GetActorLocation().ToString(), *movement->Velocity.ToString()));
    TestTrue(TEXT("Coin stops at floor"), movement->Velocity.IsNearlyZero()
        && coin->GetActorLocation().Z < 10.0f && coin->GetActorLocation().Z > 0.0f);
    if (TestTrue(TEXT("Nearby AI receives landing noise"), nearListener->GetActorsPerception(coin, info)))
    {
        TestEqual(TEXT("Exactly one sense recorded"), info.LastSensedStimuli.Num(), 1);
        if (!info.LastSensedStimuli.IsEmpty())
        {
            const FAIStimulus& stimulus = info.LastSensedStimuli[0];
            TestEqual(TEXT("Noise tag"), stimulus.Tag, ADistractionCoin::kNoiseTag);
            TestTrue(TEXT("Noise location is floor impact, not throw origin"),
                FMath::Abs(stimulus.StimulusLocation.Z) < 1.0f && stimulus.StimulusLocation.X > 100.0f);
            TestTrue(TEXT("Stimulus successfully sensed"), stimulus.WasSuccessfullySensed());
        }
    }
    TestFalse(TEXT("AI outside coin range receives no noise"), farListener->GetActorsPerception(coin, info));
    const FVector landedLocation = coin->GetActorLocation();
    coin->LaunchCoin(FVector(1000.0f, 0.0f, 300.0f));
    testWorld.AdvanceFrames(10);
    TestTrue(TEXT("Landed coin cannot be relaunched"), coin->GetActorLocation().Equals(landedLocation));

    ADistractionCoin* silentCoin = world->SpawnActor<ADistractionCoin>(
        coinClass, FVector(0.0f, 100.0f, 160.0f), FRotator::ZeroRotator);
    FFloatProperty* rangeProperty = FindFProperty<FFloatProperty>(ADistractionCoin::StaticClass(), TEXT("noiseRange_"));
    rangeProperty->SetPropertyValue_InContainer(silentCoin, 0.0f);
    silentCoin->LaunchCoin(FVector(1000.0f, 0.0f, 300.0f));
    testWorld.AdvanceFrames(kFlightTestFrames);
    TestFalse(TEXT("Zero noise range disables hearing instead of making it unlimited"),
        nearListener->GetActorsPerception(silentCoin, info));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoinMeshSelectionTest, "CH3.Coin.ComponentMeshSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoinMeshSelectionTest::RunTest(const FString& parameters)
{
    FCoinTestWorld testWorld = {};
    UClass* coinClass = LoadClass<ADistractionCoin>(nullptr,
        TEXT("/Game/Shooting/Blueprints/BP_DistractionCoin.BP_DistractionCoin_C"));
    UStaticMesh* selectedMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!TestNotNull(TEXT("Coin Blueprint loads"), coinClass)
        || !TestNotNull(TEXT("Alternative mesh loads"), selectedMesh))
    {
        return false;
    }

    const FTransform spawnTransform(FVector(0.0f, 0.0f, 160.0f));
    ADistractionCoin* coin = testWorld.world_->SpawnActorDeferred<ADistractionCoin>(coinClass, spawnTransform);
    if (!TestNotNull(TEXT("Deferred coin spawned"), coin))
    {
        return false;
    }
    UStaticMeshComponent* meshComponent = coin->FindComponentByClass<UStaticMeshComponent>();
    meshComponent->SetStaticMesh(selectedMesh);
    const float expectedRadius = coin->GetCollisionRadius();
    coin->FinishSpawning(spawnTransform);
    TestTrue(TEXT("BeginPlay ran"), coin->HasActorBegunPlay());
    TestTrue(TEXT("Construction and BeginPlay preserve the selected component mesh"),
        meshComponent->GetStaticMesh() == selectedMesh);
    TestTrue(TEXT("Spawn clearance matches fitted collision"),
        FMath::IsNearlyEqual(expectedRadius, coin->GetCollisionRadius()));

    meshComponent->SetStaticMesh(nullptr);
    coin->OnConstruction(coin->GetActorTransform());
    TestNull(TEXT("Selecting None keeps the coin mesh empty"), meshComponent->GetStaticMesh().Get());
    return true;
}

#endif
