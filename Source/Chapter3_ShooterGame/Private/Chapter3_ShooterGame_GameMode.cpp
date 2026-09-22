#include "Chapter3_ShooterGame_GameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Chapter3GameInstance.h"
#include "EngineUtils.h"
#include "EnemyAIController.h"
#include "Chapter3_ShooterGame_Character.h"

AChapter3_ShooterGame_GameMode::AChapter3_ShooterGame_GameMode() {
    PrimaryActorTick.bCanEverTick = true;
}

void AChapter3_ShooterGame_GameMode::BeginPlay() {
    mission_run_id_ = FGuid::NewGuid();
    is_game_over_ = false;
    is_game_cleared_ = false;
    current_score_ = 0;
    is_target_eliminated_ = false;
    is_escaped_ = false;
    is_detection_timer_active_ = false;
    is_boss_map_timer_active_ = false;

    FString current_level_name = UGameplayStatics::GetCurrentLevelName(this);
    if (current_level_name.Contains(TEXT("Boss"))) {
        is_boss_map_timer_active_ = true;
        boss_map_remaining_time_ = kBossMapTimeLimit;
    }
    // Blueprint BeginPlay can call mission APIs, so initialize the run first.
    Super::BeginPlay();
}

void AChapter3_ShooterGame_GameMode::Tick(float delta_seconds) {
    Super::Tick(delta_seconds);

    if (is_game_over_ || is_game_cleared_) {
        return;
    }

    // 사망 여부 검사
    CheckPlayerDeath();

    // 사망 판정 시 ProcessGameOver 실행
    if (is_game_over_) {
        return;
    }

    // 발각 상태 검사 및 발각 후 3분 타이머
    if (!is_detection_timer_active_) {
        CheckPlayerDetectionFromAI();
    }
    else {
        detection_remaining_time_ -= delta_seconds;
        if (detection_remaining_time_ <= 0.0f) {
            detection_remaining_time_ = 0.0f;
            is_detection_timer_active_ = false;
            ProcessGameOver(TEXT("[타임아웃] 발각 후 3분 제한 시간 초과로 패배했습니다."));
        }
    }


    // 보스 맵 5분 제한 시간 타이머
    if (is_boss_map_timer_active_) {
        boss_map_remaining_time_ -= delta_seconds;
        if (boss_map_remaining_time_ <= 0.0f) {
            boss_map_remaining_time_ = 0.0f;
            is_boss_map_timer_active_ = false;
            ProcessGameOver(TEXT("[타임아웃] 보스 맵 제한 시간 초과로 패배했습니다."));
        }
    }
}

    // 플레이어 사망 상태 검사 (Character 클래스의 IsDead() 호출)
    void AChapter3_ShooterGame_GameMode::CheckPlayerDeath() {
        APawn* player_pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
        if (!player_pawn) return;

        if (AChapter3_ShooterGame_Character* player_character = Cast<AChapter3_ShooterGame_Character>(player_pawn)) {
            if (player_character->IsDead()) {
                ProcessGameOver(TEXT("플레이어 체력이 0이 되어 사망했습니다."));
            }
        }
    }

// GameMode가 직접 적 AI의 상태를 호출/확인하는 함수
void AChapter3_ShooterGame_GameMode::CheckPlayerDetectionFromAI() {
    if (!GetWorld()) return;

    // 월드 내의 모든 AEnemyAIController를 탐색
    for (TActorIterator<AEnemyAIController> It(GetWorld()); It; ++It) {
        AEnemyAIController* AIController = *It;

        // 적 AI가 플레이어를 감지(isCaptured_ == true)했는지 직접 확인
        if (AIController && AIController->isCaptured_) {
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] 적 AI 발각 확인 -> 3분 제한시간 타이머 시작"));
            is_detection_timer_active_ = true;
            detection_remaining_time_ = kDetectionTimeLimit;
            break; // 한 명이라도 감지했으면 타이머를 켜고 즉시 탐색 종료
        }
    }
}

void AChapter3_ShooterGame_GameMode::ReportPlayerDetected()
{
    if (is_game_over_ || is_game_cleared_ || is_detection_timer_active_)
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] ReportPlayerDetected() 호출됨 -> 3분 타이머 시작"));
    is_detection_timer_active_ = true;
    detection_remaining_time_ = kDetectionTimeLimit;
}

// --- 점수 로직 ---
void AChapter3_ShooterGame_GameMode::AddScore(int32 amount) {
    if (is_game_over_ || is_game_cleared_) return;
    const int64 nextScore = static_cast<int64>(current_score_) + amount;
    if (nextScore > MAX_int32 || nextScore < MIN_int32) return;
    current_score_ = static_cast<int32>(nextScore);
    on_score_changed_.Broadcast(current_score_);
}

int32 AChapter3_ShooterGame_GameMode::GetCurrentScore() const {
    return current_score_;
}

// ==========================================
// 임시
// ==========================================

void AChapter3_ShooterGame_GameMode::Dummy_ReceiveEnemyEliminated() {
    if (is_game_over_ || is_game_cleared_) return;

    UE_LOG(LogTemp, Warning, TEXT("[DUMMY] 적 처치 신호 수신됨"));
    is_target_eliminated_ = true;
    AddScore(100);
}

void AChapter3_ShooterGame_GameMode::Dummy_ReceivePlayerEscaped() {
    if (is_game_over_ || is_game_cleared_) return;

    UE_LOG(LogTemp, Warning, TEXT("[DUMMY] 탈출 지점 도달 신호 수신됨"));
    is_escaped_ = true;

    if (is_target_eliminated_ && is_escaped_) {
        ProcessGameVictory(TEXT("대상 암살 및 탈출 성공: 기초 의뢰 클리어!"));
    }
}

void AChapter3_ShooterGame_GameMode::Dummy_ReceiveHeroineRescued() {
    if (is_game_over_ || is_game_cleared_) return;

    UE_LOG(LogTemp, Warning, TEXT("[DUMMY] 히로인 구출 신호 수신됨"));
    FString current_level_name = UGameplayStatics::GetCurrentLevelName(this);
    if (current_level_name.Contains(TEXT("Boss"))) {
        ProcessGameVictory(TEXT("히로인 구출 성공: 보스 맵 클리어!"));
    }
}

// --- 시작 / 종료 ---
void AChapter3_ShooterGame_GameMode::TriggerGameStart() {
    UE_LOG(LogTemp, Log, TEXT("[GameMode] 게임 시작 트리거 실행 (상호작용 + 컷신 완료)"));
}

void AChapter3_ShooterGame_GameMode::TriggerGameEnd() {
    UE_LOG(LogTemp, Log, TEXT("[GameMode] 게임 종료 트리거 실행 (최종 엔딩)"));
    ProcessGameVictory(TEXT("최종 엔딩 완료"));
}

// --- 결과 처리 ---
void AChapter3_ShooterGame_GameMode::ProcessGameOver(const FString& fail_reason) {
    is_game_over_ = true;
    is_detection_timer_active_ = false;
    is_boss_map_timer_active_ = false;
    UE_LOG(LogTemp, Error, TEXT("GAME OVER: %s"), *fail_reason);

    on_game_over_.Broadcast(fail_reason);
}

void AChapter3_ShooterGame_GameMode::ProcessGameVictory(const FString& victory_reason) {
    if (is_game_over_ || is_game_cleared_) return;

    is_game_cleared_ = true;
    is_detection_timer_active_ = false;
    is_boss_map_timer_active_ = false;
    UE_LOG(LogTemp, Log, TEXT("VICTORY: %s"), *victory_reason);

    const FName level_id = progress_level_id_.IsNone()
        ? FName(*UGameplayStatics::GetCurrentLevelName(this, true))
        : progress_level_id_;
    if (UChapter3GameInstance* game_instance = Cast<UChapter3GameInstance>(GetGameInstance())) {
        const int64 reward_amount = FMath::Max<int64>(0, mission_reward_amount_);
        if (!game_instance->CompleteMission(level_id, mission_run_id_, reward_amount, first_clear_reward_only_)) {
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] Mission progress was not accepted: %s"), *level_id.ToString());
        }
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] Chapter3GameInstance is not configured; mission progress cannot be saved."));
    }

    on_game_victory_.Broadcast(victory_reason);
}
