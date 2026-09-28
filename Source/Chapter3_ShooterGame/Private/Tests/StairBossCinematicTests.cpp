#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "StairBossCinematicTrigger.h"
#include "Chapter3_ShooterGame_GameMode.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieScene.h"
#include "UObject/Package.h"

namespace StairBossCinematicTests
{
// No project GameInstance, content map, viewport, or disk-backed save is used.
// Completion tests broadcast the real player's public OnFinished delegate after
// successful StartCinematic. They validate the completion contract, not natural
// sequence timing, camera tracks, or the Blueprint result widget's appearance.
struct FFixture
{
    UWorld* World = nullptr;
    AChapter3_ShooterGame_GameMode* GameMode = nullptr;
    APlayerController* Controller = nullptr;
    APawn* Pawn = nullptr;
    AStairBossCinematicTrigger* Trigger = nullptr;
    ULevelSequencePlayer* Player = nullptr;

    bool Initialize(FAutomationTestBase& Test)
    {
        if (!Test.TestNotNull(TEXT("Engine available"), GEngine)) return false;
        // GameMode enables its boss countdown from the map package name.
        UPackage* Package = CreatePackage(*FString::Printf(TEXT("/Temp/StairBossAutomation_%s"),
            *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
        Package->SetFlags(RF_Transient);
        World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("StairBossAutomation"), Package);
        if (!Test.TestNotNull(TEXT("Transient game world"), World)) return false;
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        World->SetGameInstance(NewObject<UGameInstance>(GEngine));
        World->GetWorldSettings()->DefaultGameMode = AChapter3_ShooterGame_GameMode::StaticClass();
        FURL URL;
        if (!Test.TestTrue(TEXT("Native mission GameMode created"), World->SetGameMode(URL))) return false;
        World->InitializeActorsForPlay(URL);
        GameMode = World->GetAuthGameMode<AChapter3_ShooterGame_GameMode>();
        if (!Test.TestNotNull(TEXT("Mission GameMode"), GameMode)) return false;
        GameMode->DispatchBeginPlay();

        Controller = World->SpawnActor<APlayerController>();
        Pawn = World->SpawnActor<APawn>();
        if (!Test.TestNotNull(TEXT("Player controller"), Controller)
            || !Test.TestNotNull(TEXT("Player pawn"), Pawn)) return false;
        Controller->Possess(Pawn);
        Controller->EnableInput(Controller);
        Pawn->EnableInput(Controller);
        Controller->ClientSetHUD(AHUD::StaticClass());
        if (!Test.TestNotNull(TEXT("Camera manager"), Controller->PlayerCameraManager.Get())
            || !Test.TestNotNull(TEXT("HUD"), Controller->GetHUD())) return false;
        Controller->GetHUD()->bShowHUD = true;

        ULevelSequence* Sequence = NewObject<ULevelSequence>(World);
        Sequence->Initialize();
        Sequence->GetMovieScene()->SetPlaybackRange(0, 24000);
        ALevelSequenceActor* SequenceActor = nullptr;
        Player = ULevelSequencePlayer::CreateLevelSequencePlayer(World, Sequence,
            FMovieSceneSequencePlaybackSettings(), SequenceActor);
        if (!Test.TestNotNull(TEXT("Sequence player"), Player)) return false;
        Trigger = World->SpawnActor<AStairBossCinematicTrigger>();
        if (!Test.TestNotNull(TEXT("Cinematic trigger"), Trigger)) return false;
        Trigger->automaticallyCheckRange_ = false;
        Trigger->fadeOutDuration_ = 0.0f;
        Trigger->sequenceActor_ = SequenceActor;
        return true;
    }

    bool Start(FAutomationTestBase& Test)
    {
        if (!Test.TestTrue(TEXT("StartCinematic succeeds"), Trigger->StartCinematic())) return false;
        Test.TestTrue(TEXT("Player captured"), Trigger->IsCinematicActive());
        Test.TestTrue(TEXT("Cinematic mode enabled"), Controller->bCinematicMode);
        Test.TestTrue(TEXT("Movement locked"), Controller->IsMoveInputIgnored());
        Test.TestTrue(TEXT("Look locked"), Controller->IsLookInputIgnored());
        Test.TestFalse(TEXT("HUD hidden during playback"), Controller->GetHUD()->bShowHUD);
        return true;
    }

    void CheckRestored(FAutomationTestBase& Test)
    {
        Test.TestFalse(TEXT("Player capture released"), Trigger->IsCinematicActive());
        Test.TestFalse(TEXT("Cinematic mode disabled"), Controller->bCinematicMode);
        Test.TestFalse(TEXT("Movement unlocked"), Controller->IsMoveInputIgnored());
        Test.TestFalse(TEXT("Look unlocked"), Controller->IsLookInputIgnored());
        Test.TestTrue(TEXT("Controller input restored"), Controller->InputEnabled());
        Test.TestTrue(TEXT("Pawn input restored"), Pawn->InputEnabled());
        Test.TestTrue(TEXT("HUD restored"), Controller->GetHUD()->bShowHUD);
        Test.TestFalse(TEXT("Black fade removed"), Controller->PlayerCameraManager->bEnableFading);
        Test.TestFalse(TEXT("Pawn visible again"), Pawn->IsHidden());
    }

    ~FFixture()
    {
        if (IsValid(Trigger)) Trigger->ReleaseCinematic();
        if (World)
        {
            World->DestroyWorld(false);
            GEngine->DestroyWorldContext(World);
        }
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStairBossCompletionTest,
    "Chapter3.Cinematic.StairBoss.CompletionRestoresAndClears",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStairBossCompletionTest::RunTest(const FString& Parameters)
{
    StairBossCinematicTests::FFixture Fixture;
    if (!Fixture.Initialize(*this) || !Fixture.Start(*this)) return false;
    Fixture.Player->OnFinished.Broadcast();
    TestTrue(TEXT("Completion recorded"), Fixture.Trigger->HasFinished());
    TestTrue(TEXT("Mission cleared"), Fixture.GameMode->IsGameCleared());
    Fixture.CheckRestored(*this);
    // A stale/repeated completion notification must not capture the player again.
    Fixture.Player->OnFinished.Broadcast();
    Fixture.CheckRestored(*this);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStairBossCancellationTest,
    "Chapter3.Cinematic.StairBoss.CancellationDoesNotClear",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStairBossCancellationTest::RunTest(const FString& Parameters)
{
    for (const bool bStopPlayer : {true, false})
    {
        StairBossCinematicTests::FFixture Fixture;
        if (!Fixture.Initialize(*this) || !Fixture.Start(*this)) return false;
        if (bStopPlayer) Fixture.Player->Stop();
        else Fixture.Trigger->ReleaseCinematic();
        TestFalse(TEXT("Cancellation does not complete mission"), Fixture.GameMode->IsGameCleared());
        TestFalse(TEXT("Cancellation is not natural completion"), Fixture.Trigger->HasFinished());
        Fixture.CheckRestored(*this);
        Fixture.Player->OnFinished.Broadcast();
        TestFalse(TEXT("Late callback after cancellation cannot clear"), Fixture.GameMode->IsGameCleared());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStairBossHoldBlackTest,
    "Chapter3.Cinematic.StairBoss.LegacyHoldBlack",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStairBossHoldBlackTest::RunTest(const FString& Parameters)
{
    StairBossCinematicTests::FFixture Fixture;
    if (!Fixture.Initialize(*this)) return false;
    Fixture.Trigger->completeMissionOnFinish_ = false;
    Fixture.Trigger->holdBlackAtEnd_ = true;
    if (!Fixture.Start(*this)) return false;
    Fixture.Player->OnFinished.Broadcast();
    TestTrue(TEXT("Legacy completion recorded"), Fixture.Trigger->HasFinished());
    TestFalse(TEXT("Opt-out does not clear mission"), Fixture.GameMode->IsGameCleared());
    TestTrue(TEXT("Legacy mode retains player capture"), Fixture.Trigger->IsCinematicActive());
    TestTrue(TEXT("Legacy mode retains black fade"), Fixture.Controller->PlayerCameraManager->bEnableFading);
    TestEqual(TEXT("Legacy fade is fully black"), Fixture.Controller->PlayerCameraManager->FadeAmount, 1.0f);
    Fixture.Trigger->ReleaseCinematic();
    Fixture.CheckRestored(*this);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStairBossTimerTest,
    "Chapter3.Cinematic.StairBoss.MissionTimersSuspendAndResume",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStairBossTimerTest::RunTest(const FString& Parameters)
{
    StairBossCinematicTests::FFixture Fixture;
    if (!Fixture.Initialize(*this) || !Fixture.Start(*this)) return false;
    Fixture.GameMode->ReportPlayerDetected();
    Fixture.GameMode->Tick(10.0f);
    TestFalse(TEXT("Detection ignored during cinematic"), Fixture.GameMode->WasDetected());
    TestFalse(TEXT("Detection timer remains inactive"), Fixture.GameMode->IsDetectionTimerActive());
    TestEqual(TEXT("Stage time suspended"), Fixture.GameMode->GetStagePlayTime(), 0.0f);
    TestEqual(TEXT("Boss countdown suspended"), Fixture.GameMode->GetBossMapRemainingTime(), 300.0f);

    Fixture.Trigger->ReleaseCinematic();
    Fixture.GameMode->ReportPlayerDetected();
    Fixture.GameMode->Tick(1.0f);
    TestTrue(TEXT("Detection resumes after cancellation"), Fixture.GameMode->WasDetected());
    TestEqual(TEXT("Stage time resumes"), Fixture.GameMode->GetStagePlayTime(), 1.0f);
    TestEqual(TEXT("Detection countdown resumes"), Fixture.GameMode->GetDetectionRemainingTime(), 179.0f);
    TestEqual(TEXT("Boss countdown resumes"), Fixture.GameMode->GetBossMapRemainingTime(), 299.0f);

    Fixture.Trigger->playOnce_ = false;
    if (!Fixture.Start(*this)) return false;
    Fixture.GameMode->Tick(400.0f);
    TestEqual(TEXT("Active detection countdown suspended"), Fixture.GameMode->GetDetectionRemainingTime(), 179.0f);
    TestEqual(TEXT("Active boss countdown suspended"), Fixture.GameMode->GetBossMapRemainingTime(), 299.0f);
    TestFalse(TEXT("Cinematic time cannot cause timeout"), Fixture.GameMode->IsGameOver());
    Fixture.Trigger->ReleaseCinematic();
    Fixture.GameMode->Tick(1.0f);
    TestEqual(TEXT("Detection countdown resumes again"), Fixture.GameMode->GetDetectionRemainingTime(), 178.0f);
    TestEqual(TEXT("Boss countdown resumes again"), Fixture.GameMode->GetBossMapRemainingTime(), 298.0f);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
