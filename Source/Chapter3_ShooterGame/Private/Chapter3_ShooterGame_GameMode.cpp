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
    total_farming_reward_ = 0; // 새 스테이지 시작 시 추가 파밍 점수 0으로 초기화
    is_target_eliminated_ = false;
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
}

// --- 패널티 연산 ---

int32 AChapter3_ShooterGame_GameMode::GetAllowedKillsForCurrentStage() const {
    FString current_level_name = UGameplayStatics::GetCurrentLevelName(this);

    // 임무 2 (Industrial_UnfinishedBuilding): 허용 처치 수 4마리
    if (current_level_name.Contains(TEXT("Industrial_UnfinishedBuilding"))) {
        return 4;
    }
    // 임무 3 (DemoMap): 허용 처치 수 5마리
    else if (current_level_name.Contains(TEXT("DemoMap"))) {
        return 5;
    }

    // 임무 1 (기본 맵): 허용 처치 수 2마리
    return 2;
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

// 초과 처치 발생 시 마리 수에 관계없이 고정 500점 패널티 부여
int32 AChapter3_ShooterGame_GameMode::GetKillPenaltyAmount() const {
    if (GetExcessKillCount() > 0) {
        return kFixedKillPenalty; // 500점
    }
    return 0;
}

// --- 추가 보상 (파밍) 시스템 ---

void AChapter3_ShooterGame_GameMode::AddFarmingReward(int32 score_amount) {
    if (is_game_over_ || is_game_cleared_ || score_amount <= 0) return;

    // 추가 점수 순수 누적
    total_farming_reward_ += score_amount;

    // UI에 최종 연산된 스코어 전달
    on_score_changed_.Broadcast(CalculateFinalScore());
}

// --- 최종 점수 연산 및 저장 ---

int32 AChapter3_ShooterGame_GameMode::CalculateFinalScore() const {
    // 최종 점수 = 기본(3000) + 추가 파밍 점수 - 패널티(0 또는 500)
    int32 final_score = kBaseScore + total_farming_reward_ - GetKillPenaltyAmount();
    return FMath::Max(0, final_score);
}

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

// --- 기타 유틸리티 및 암살 목표 연동 ---

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

void AChapter3_ShooterGame_GameMode::OnTargetEliminated() {
    if (is_game_over_ || is_game_cleared_) return;
    is_target_eliminated_ = true;
    ProcessGameVictory(TEXT("주요 암살 대상 처치 성공: 미션 클리어!"));
}

void AChapter3_ShooterGame_GameMode::ResetStageTimer() { stage_play_time_ = 0.0f; }

void AChapter3_ShooterGame_GameMode::ResetAllTimers() {
    stage_play_time_ = 0.0f;
    detection_remaining_time_ = 0.0f;
    is_detection_timer_active_ = false;
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

void AChapter3_ShooterGame_GameMode::ProcessGameOver(const FString& fail_reason) {
    is_game_over_ = true;
    is_detection_timer_active_ = false;
    on_game_over_.Broadcast(fail_reason);
}

void AChapter3_ShooterGame_GameMode::ProcessGameVictory(const FString& victory_reason) {
    is_game_cleared_ = true;
    is_detection_timer_active_ = false;

    // 최종 점수를 획득 재화(Money)에 추가
    int32 final_reward = CalculateFinalScore();
    AddCurrency(final_reward);

    on_game_victory_.Broadcast(victory_reason);
}