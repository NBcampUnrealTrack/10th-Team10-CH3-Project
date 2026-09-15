#include "Chapter3_ShooterGame_GameMode.h"
#include "Kismet/GameplayStatics.h"

AChapter3_ShooterGame_GameMode::AChapter3_ShooterGame_GameMode() {
    PrimaryActorTick.bCanEverTick = true;
}

void AChapter3_ShooterGame_GameMode::BeginPlay() {
    Super::BeginPlay();
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
}

void AChapter3_ShooterGame_GameMode::Tick(float delta_seconds) {
    Super::Tick(delta_seconds);

    if (is_game_over_ || is_game_cleared_) {
        return;
    }

    if (is_detection_timer_active_) {
        detection_remaining_time_ -= delta_seconds;
        if (detection_remaining_time_ <= 0.0f) {
            detection_remaining_time_ = 0.0f;
            is_detection_timer_active_ = false;
            ProcessGameOver(TEXT("[타임아웃] 발각 후 3분 제한 시간 초과로 패배했습니다."));
        }
    }

    if (is_boss_map_timer_active_) {
        boss_map_remaining_time_ -= delta_seconds;
        if (boss_map_remaining_time_ <= 0.0f) {
            boss_map_remaining_time_ = 0.0f;
            is_boss_map_timer_active_ = false;
            ProcessGameOver(TEXT("[타임아웃] 보스 맵 제한 시간 초과로 패배했습니다."));
        }
    }
}

// --- 점수 로직 ---
void AChapter3_ShooterGame_GameMode::AddScore(int32 amount) {
    if (is_game_over_ || is_game_cleared_) return;
    current_score_ += amount;
    on_score_changed_.Broadcast(current_score_);
}

int32 AChapter3_ShooterGame_GameMode::GetCurrentScore() const {
    return current_score_;
}

// ==========================================
// 임시
// ==========================================

void AChapter3_ShooterGame_GameMode::Dummy_ReceivePlayerHealth(float current_health) {
    if (is_game_over_ || is_game_cleared_) return;

    UE_LOG(LogTemp, Warning, TEXT("[DUMMY] 플레이어 체력 수신: %.1f"), current_health);
    if (current_health <= 0.0f) {
        ProcessGameOver(TEXT("플레이어 체력이 0이 되어 사망했습니다."));
    }
}

void AChapter3_ShooterGame_GameMode::Dummy_ReceiveEnemyEliminated() {
    if (is_game_over_ || is_game_cleared_) return;

    UE_LOG(LogTemp, Warning, TEXT("[DUMMY] 적 처치 신호 수신됨"));
    is_target_eliminated_ = true;
    AddScore(100);
}

void AChapter3_ShooterGame_GameMode::Dummy_ReceivePlayerDetected() {
    if (is_game_over_ || is_game_cleared_ || is_detection_timer_active_) return;

    UE_LOG(LogTemp, Warning, TEXT("[DUMMY] 플레이어 발각 신호 수신됨 -> 3분 타이머 시작"));
    is_detection_timer_active_ = true;
    detection_remaining_time_ = kDetectionTimeLimit;
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
    is_game_cleared_ = true;
}

// --- 결과 처리 ---
void AChapter3_ShooterGame_GameMode::ProcessGameOver(const FString& fail_reason) {
    is_game_over_ = true;
    is_detection_timer_active_ = false;
    is_boss_map_timer_active_ = false;
    UE_LOG(LogTemp, Error, TEXT("GAME OVER: %s"), *fail_reason);
}

void AChapter3_ShooterGame_GameMode::ProcessGameVictory(const FString& victory_reason) {
    is_game_cleared_ = true;
    is_detection_timer_active_ = false;
    is_boss_map_timer_active_ = false;
    UE_LOG(LogTemp, Log, TEXT("VICTORY: %s"), *victory_reason);
}