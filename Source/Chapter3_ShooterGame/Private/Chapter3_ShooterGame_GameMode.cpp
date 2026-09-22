#include "Chapter3_ShooterGame_GameMode.h"
#include "MainGameState.h" 
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "EnemyAIController.h"
#include "Chapter3_ShooterGame_Character.h"
#include "Chapter3GameInstance.h"
#include "UnlockInventoryComponent.h"

AChapter3_ShooterGame_GameMode::AChapter3_ShooterGame_GameMode() {
    PrimaryActorTick.bCanEverTick = true;
}

void AChapter3_ShooterGame_GameMode::BeginPlay() {
    Super::BeginPlay();

    ResetAllTimers();

    is_game_over_ = false;
    is_game_cleared_ = false;
    current_score_ = kBaseScore;
    is_target_eliminated_ = false;
    is_escaped_ = false;

    FString current_level_name = UGameplayStatics::GetCurrentLevelName(this);
    if (current_level_name.Contains(TEXT("Boss"))) {
        is_boss_map_timer_active_ = true;
        boss_map_remaining_time_ = kBossMapTimeLimit;
    }
}

void AChapter3_ShooterGame_GameMode::Tick(float delta_seconds) {
    Super::Tick(delta_seconds);

    if (is_game_over_ || is_game_cleared_) return;

    stage_play_time_ += delta_seconds;

    CheckPlayerDeath();
    if (is_game_over_) return;

    if (!is_detection_timer_active_) {
        CheckPlayerDetectionFromAI();
    }
    else {
        detection_remaining_time_ -= delta_seconds;
        if (detection_remaining_time_ <= 0.0f) {
            detection_remaining_time_ = 0.0f;
            is_detection_timer_active_ = false;
            ProcessGameOver(TEXT("[타임아웃] 발각 후 제한 시간 초과로 패배했습니다."));
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

// --- 1. 처치 허용 수 및 점수 차감 연산 ---

int32 AChapter3_ShooterGame_GameMode::GetAllowedKillsForCurrentStage() const {
    FString current_level_name = UGameplayStatics::GetCurrentLevelName(this);

    if (current_level_name.Contains(TEXT("Level2")) || current_level_name.Contains(TEXT("Stage2"))) {
        return 4; // Stage 2: 4명 허용
    }
    else if (current_level_name.Contains(TEXT("Level3")) || current_level_name.Contains(TEXT("Stage3")) || current_level_name.Contains(TEXT("Boss"))) {
        return 5; // Stage 3 / Boss: 5명 허용
    }

    return 2; // Stage 1: 2명 허용
}

int32 AChapter3_ShooterGame_GameMode::GetCurrentPlayerKillCount() const {
    if (UWorld* world = GetWorld()) {
        if (AMainGameState* gs = world->GetGameState<AMainGameState>()) {
            return gs->GetPlayerKillCount();
        }
    }
    return 0;
}

int32 AChapter3_ShooterGame_GameMode::GetExcessKillCount() const {
    int32 current_kills = GetCurrentPlayerKillCount();
    int32 allowed_kills = GetAllowedKillsForCurrentStage();
    return FMath::Max(0, current_kills - allowed_kills);
}

int32 AChapter3_ShooterGame_GameMode::GetKillPenaltyAmount() const {
    return GetExcessKillCount() * kKillPenaltyAmount;
}

int32 AChapter3_ShooterGame_GameMode::CalculateFinalScore() const {
    int32 final_score = current_score_ - GetKillPenaltyAmount();
    return FMath::Max(0, final_score);
}

// --- 2. 클리어 보상 재화 세부 연산 ---

int32 AChapter3_ShooterGame_GameMode::GetScoreRewardCurrency() const {
    int32 final_score = CalculateFinalScore();
    return FMath::FloorToInt(final_score * kScoreToCurrencyRatio); // 최종 점수 * 0.5
}

int32 AChapter3_ShooterGame_GameMode::CalculateRewardCurrency() const {
    return kBaseRewardCurrency + GetScoreRewardCurrency(); // 기본 3000 + (점수 * 0.5)
}

// --- 3. 기존 Chapter3GameInstance 연동 누적 재화 제어 ---

int64 AChapter3_ShooterGame_GameMode::GetTotalCurrency() const {
    if (const UChapter3GameInstance* GI = Cast<UChapter3GameInstance>(GetGameInstance())) {
        return GI->GetMoney();
    }
    return 0;
}

bool AChapter3_ShooterGame_GameMode::AddCurrency(int64 amount) {
    if (UChapter3GameInstance* GI = Cast<UChapter3GameInstance>(GetGameInstance())) {
        return GI->AddMoney(amount);
    }
    return false;
}

// --- 4. 팀원 수집품 완벽 보유 판별 ---

bool AChapter3_ShooterGame_GameMode::IsAllCollectiblesAcquired() const {
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!IsValid(PC)) return false;

    UUnlockInventoryComponent* inv_comp = PC->FindComponentByClass<UUnlockInventoryComponent>();
    if (!IsValid(inv_comp)) return false;

    TArray<FUnlockInventorySlot> slots = inv_comp->GetSlots();
    if (slots.Num() == 0) return false;

    for (const FUnlockInventorySlot& slot : slots) {
        if (!slot.isUnlocked_) return false;
    }
    return true;
}

// --- 5. 실제 암살/구출 목표 연동 ---

void AChapter3_ShooterGame_GameMode::OnTargetEliminated() {
    if (is_game_over_ || is_game_cleared_) return;

    is_target_eliminated_ = true;
    ProcessGameVictory(TEXT("주요 암살 대상 처치 성공!"));
}

void AChapter3_ShooterGame_GameMode::OnHeroineRescued() {
    if (is_game_over_ || is_game_cleared_) return;

    FString current_level_name = UGameplayStatics::GetCurrentLevelName(this);
    if (current_level_name.Contains(TEXT("Boss"))) {
        ProcessGameVictory(TEXT("히로인 구출 성공!"));
    }
}

// --- 타이머, 스코어, 이동 관련 구현 ---

void AChapter3_ShooterGame_GameMode::ResetStageTimer() { stage_play_time_ = 0.0f; }

void AChapter3_ShooterGame_GameMode::ResetAllTimers() {
    stage_play_time_ = 0.0f;
    detection_remaining_time_ = 0.0f;
    boss_map_remaining_time_ = 0.0f;
    is_detection_timer_active_ = false;
    is_boss_map_timer_active_ = false;
    was_detected_ = false;
}

void AChapter3_ShooterGame_GameMode::RestartCurrentStage() {
    ResetAllTimers();
    UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}

void AChapter3_ShooterGame_GameMode::TravelToNextStage(FName next_level_name) {
    if (!next_level_name.IsNone()) {
        ResetAllTimers();
        UGameplayStatics::OpenLevel(this, next_level_name);
    }
}

FString AChapter3_ShooterGame_GameMode::GetFormattedStagePlayTime() const {
    int32 total_seconds = FMath::FloorToInt(stage_play_time_);
    return FString::Printf(TEXT("%02d:%02d"), total_seconds / 60, total_seconds % 60);
}

void AChapter3_ShooterGame_GameMode::CheckPlayerDetectionFromAI() {
    if (!GetWorld()) return;
    for (TActorIterator<AEnemyAIController> It(GetWorld()); It; ++It) {
        if (It && It->isCaptured_) {
            ReportPlayerDetected();
            break;
        }
    }
}

void AChapter3_ShooterGame_GameMode::ReportPlayerDetected() {
    if (is_game_over_ || is_game_cleared_ || is_detection_timer_active_) return;
    is_detection_timer_active_ = true;
    was_detected_ = true;
    detection_remaining_time_ = kDetectionTimeLimit;
}

void AChapter3_ShooterGame_GameMode::CheckPlayerDeath() {
    APawn* player_pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (AChapter3_ShooterGame_Character* player_character = Cast<AChapter3_ShooterGame_Character>(player_pawn)) {
        if (player_character->IsDead()) {
            ProcessGameOver(TEXT("플레이어 체력이 0이 되어 사망했습니다."));
        }
    }
}

void AChapter3_ShooterGame_GameMode::AddScore(int32 amount) {
    if (is_game_over_ || is_game_cleared_) return;
    current_score_ += amount;
    on_score_changed_.Broadcast(current_score_);
}

int32 AChapter3_ShooterGame_GameMode::GetCurrentScore() const { return current_score_; }

void AChapter3_ShooterGame_GameMode::ProcessGameOver(const FString& fail_reason) {
    is_game_over_ = true;
    is_detection_timer_active_ = false;
    is_boss_map_timer_active_ = false;
    on_game_over_.Broadcast(fail_reason);
}

void AChapter3_ShooterGame_GameMode::ProcessGameVictory(const FString& victory_reason) {
    is_game_cleared_ = true;
    is_detection_timer_active_ = false;
    is_boss_map_timer_active_ = false;

    // 클리어 보상 계산 및 게임 인스턴스 저장소 반영
    int32 round_reward = CalculateRewardCurrency();
    AddCurrency(round_reward);

    on_game_victory_.Broadcast(victory_reason);
}