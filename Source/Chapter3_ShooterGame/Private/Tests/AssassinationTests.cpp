#if WITH_DEV_AUTOMATION_TESTS

#include "AssassinationTargetComponent.h"
#include "ShootingPlayerController.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"

namespace
{
    class FAssassinationTestWorld
    {
    public:
        FAssassinationTestWorld()
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

        ~FAssassinationTestWorld()
        {
            world_->EndPlay(EEndPlayReason::Quit);
            GEngine->DestroyWorldContext(world_);
            world_->DestroyWorld(false);
        }

        APawn* CreatePlayer() const
        {
            APawn* pawn = world_->SpawnActor<APawn>();
            USceneComponent* root = NewObject<USceneComponent>(pawn);
            pawn->SetRootComponent(root);
            root->RegisterComponent();
            return pawn;
        }

        UAssassinationTargetComponent* CreateTarget(const FVector& location = FVector::ZeroVector) const
        {
            AActor* owner = world_->SpawnActor<AActor>();
            UAssassinationTargetComponent* target = NewObject<UAssassinationTargetComponent>(owner);
            owner->AddInstanceComponent(target);
            owner->SetRootComponent(target);
            target->RegisterComponent();
            owner->SetActorLocation(location);
            return target;
        }

        UWorld* world_ = nullptr;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAssassinationConditionsTest, "CH3.Assassination.ConditionsAndDeath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAssassinationConditionsTest::RunTest(const FString& parameters)
{
    FAssassinationTestWorld testWorld = {};
    APawn* player = testWorld.CreatePlayer();
    UAssassinationTargetComponent* target = testWorld.CreateTarget();
    TestFalse(TEXT("플레이어 참조 누락"), target->CanBeAssassinatedBy(nullptr));
    player->SetActorLocation(FVector(150.0f, 0.0f, 0.0f));
    TestFalse(TEXT("정면 제외"), target->TryAssassinate(player));
    player->SetActorLocation(FVector(0.0f, 150.0f, 0.0f));
    TestFalse(TEXT("측면 제외"), target->CanBeAssassinatedBy(player));
    player->SetActorLocation(FVector(-201.0f, 0.0f, 0.0f));
    TestFalse(TEXT("거리 밖 제외"), target->CanBeAssassinatedBy(player));
    player->SetActorLocation(FVector(-100.0f, 0.0f, 101.0f));
    TestFalse(TEXT("층간 높이 차이 제외"), target->CanBeAssassinatedBy(player));
    player->SetActorLocation(FVector::ZeroVector);
    TestFalse(TEXT("겹친 위치는 후방이 아님"), target->CanBeAssassinatedBy(player));
    player->SetActorLocation(FVector(-150.0f, 0.0f, 0.0f));
    TestTrue(TEXT("가까운 후방 허용"), target->CanBeAssassinatedBy(player));
    target->GetOwner()->SetActorRotation(FRotator(0.0f, 180.0f, 0.0f));
    TestFalse(TEXT("적 회전 후 다시 판정"), target->CanBeAssassinatedBy(player));
    target->GetOwner()->SetActorRotation(FRotator::ZeroRotator);
    target->assassinationDistance_ = 100.0f;
    TestFalse(TEXT("변경한 거리 반영"), target->CanBeAssassinatedBy(player));
    target->assassinationDistance_ = 200.0f;
    target->assassinationEnabled_ = false;
    TestFalse(TEXT("대상 비활성화"), target->CanBeAssassinatedBy(player));
    target->assassinationEnabled_ = true;

    AActor* wall = testWorld.world_->SpawnActor<AActor>();
    UBoxComponent* box = NewObject<UBoxComponent>(wall);
    wall->SetRootComponent(box);
    box->SetBoxExtent(FVector(10.0f, 100.0f, 100.0f));
    box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    box->SetCollisionResponseToAllChannels(ECR_Block);
    box->RegisterComponent();
    wall->SetActorLocation(FVector(-75.0f, 0.0f, 0.0f));
    TestFalse(TEXT("벽 너머 암살 차단"), target->TryAssassinate(player));
    wall->Destroy();
    target->destroyOwnerOnAssassination_ = false;
    TestTrue(TEXT("연출용 액터 보존 상태에서 암살"), target->TryAssassinate(player));
    TestTrue(TEXT("암살 상태 기록"), target->IsAssassinated());
    TestFalse(TEXT("중복 암살 차단"), target->TryAssassinate(player));
    TestFalse(TEXT("사망 대상 충돌 중단"), target->GetOwner()->GetActorEnableCollision());
    TestFalse(TEXT("사망 대상 추가 피해 중단"), target->GetOwner()->CanBeDamaged());

    UAssassinationTargetComponent* disposableTarget = testWorld.CreateTarget();
    AActor* disposableOwner = disposableTarget->GetOwner();
    TestTrue(TEXT("기본 사망 처리 성공"), disposableTarget->TryAssassinate(player));
    TestTrue(TEXT("기본 설정에서 액터 제거"), disposableOwner->IsActorBeingDestroyed());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAssassinationSelectionTest, "CH3.Assassination.TargetSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAssassinationSelectionTest::RunTest(const FString& parameters)
{
    FAssassinationTestWorld testWorld = {};
    AChapter3_ShooterGame_PlayerController* controller = testWorld.world_->SpawnActor<AChapter3_ShooterGame_PlayerController>();
    TestNull(TEXT("빙의 전에는 대상 없음"), controller->FindAssassinationTarget());
    APawn* player = testWorld.CreatePlayer();
    controller->Possess(player);
    UAssassinationTargetComponent* nearTarget = testWorld.CreateTarget(FVector(100.0f, 0.0f, 0.0f));
    UAssassinationTargetComponent* farTarget = testWorld.CreateTarget(FVector(180.0f, 0.0f, 0.0f));
    UAssassinationTargetComponent* frontTarget = testWorld.CreateTarget(FVector(-50.0f, 0.0f, 0.0f));
    TestEqual(TEXT("후방 조건을 만족하는 가장 가까운 대상 선택"), controller->FindAssassinationTarget(), nearTarget);
    nearTarget->destroyOwnerOnAssassination_ = false;
    TestTrue(TEXT("컨트롤러 암살 성공"), controller->TryAssassinate());
    TestTrue(TEXT("가까운 대상만 사망"), nearTarget->IsAssassinated());
    TestFalse(TEXT("먼 대상 생존"), farTarget->IsAssassinated());
    TestFalse(TEXT("정면 대상 생존"), frontTarget->IsAssassinated());
    TestEqual(TEXT("죽은 대상은 검색에서 제외"), controller->FindAssassinationTarget(), farTarget);
    return true;
}

#endif
