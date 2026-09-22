#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Chapter3_ShooterGame_GameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreChangedSignature, int32, new_score);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameOverSignature, const FString&, fail_reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameVictorySignature, const FString&, victory_reason);

UCLASS()
class CHAPTER3_SHOOTERGAME_API AChapter3_ShooterGame_GameMode : public AGameMode
{
    GENERATED_BODY()

public:
    AChapter3_ShooterGame_GameMode();

    virtual void BeginPlay() override;
    virtual void Tick(float delta_seconds) override;

    // --- 수집품 완벽 보유 여부 판별 ---
    UFUNCTION(BlueprintPure, Category = "Ending")
    bool IsAllCollectiblesAcquired() const;

    // --- 스테이지별 적 처치 허용 수 및 점수 차감 연산 ---
    UFUNCTION(BlueprintPure, Category = "Score|UI")
    int32 GetAllowedKillsForCurrentStage() const;

    UFUNCTION(BlueprintPure, Category = "Score|UI")
    int32 GetCurrentPlayerKillCount() const;

    // 허용 수 초과 처치 수
    UFUNCTION(BlueprintPure, Category = "Score|UI")
    int32 GetExcessKillCount() const; 

    // 초과 처치로 인해 차감되는 총 패널티 점수 (초과분 * 350)
    UFUNCTION(BlueprintPure, Category = "Score|UI")
    int32 GetKillPenaltyAmount() const; 

    // 최종 연산 결과 점수 (5000 - 패널티)

    UFUNCTION(BlueprintPure, Category = "Score|UI")
    int32 CalculateFinalScore() const;

    // --- 클리어 보상 재화 세부 연산 (결과 창 UI 노출용) ---
    UFUNCTION(BlueprintPure, Category = "Reward|UI")
    int32 GetBaseRewardCurrency() const { return kBaseRewardCurrency; } // 기본 보상 (3000)

    // 점수 변환 보상 (최종 점수 * 0.5)
    UFUNCTION(BlueprintPure, Category = "Reward|UI")
    int32 GetScoreRewardCurrency() const; 

    // 총 클리어 보상 재화 (기본 3000 + 점수 변환 보상)
    UFUNCTION(BlueprintPure, Category = "Reward|UI")
    int32 CalculateRewardCurrency() const; 

    // --- 기존 Chapter3GameInstance 연동 누적 재화 제어 ---
    UFUNCTION(BlueprintPure, Category = "Currency")
    int64 GetTotalCurrency() const;

    UFUNCTION(BlueprintCallable, Category = "Currency")
    bool AddCurrency(int64 amount);

    // --- 스테이지 & 타이머 제어 ---
    UFUNCTION(BlueprintCallable, Category = "Mission|Control")
    void ResetStageTimer();

    UFUNCTION(BlueprintCallable, Category = "Mission|Control")
    void ResetAllTimers();

    UFUNCTION(BlueprintCallable, Category = "Mission|Control")
    void RestartCurrentStage();

    UFUNCTION(BlueprintCallable, Category = "Mission|Control")
    void TravelToNextStage(FName next_level_name);

    // --- 시간 및 상태 Getter ---
    UFUNCTION(BlueprintPure, Category = "UI|Timer")
    FString GetFormattedStagePlayTime() const;

    UFUNCTION(BlueprintPure, Category = "UI|Timer")
    float GetStagePlayTime() const { return stage_play_time_; }

    UFUNCTION(BlueprintPure, Category = "UI|Timer")
    float GetDetectionRemainingTime() const { return detection_remaining_time_; }

    UFUNCTION(BlueprintPure, Category = "UI|Timer")
    bool IsDetectionTimerActive() const { return is_detection_timer_active_; }

    UFUNCTION(BlueprintPure, Category = "UI|Timer")
    float GetBossMapRemainingTime() const { return boss_map_remaining_time_; }

    UFUNCTION(BlueprintPure, Category = "UI|Timer")
    bool IsBossMapTimerActive() const { return is_boss_map_timer_active_; }

    UFUNCTION(BlueprintPure, Category = "UI|Status")
    bool IsGameOver() const { return is_game_over_; }

    UFUNCTION(BlueprintPure, Category = "UI|Status")
    bool IsGameCleared() const { return is_game_cleared_; }

    UFUNCTION(BlueprintPure, Category = "UI|Status")
    bool WasDetected() const { return was_detected_; }

    // --- 점수 제어 ---
    UFUNCTION(BlueprintCallable, Category = "Score")
    void AddScore(int32 amount);

    UFUNCTION(BlueprintPure, Category = "UI|Score")
    int32 GetCurrentScore() const;

    // --- 실제 암살/구출 목표 연동 ---
    UFUNCTION(BlueprintCallable, Category = "Mission")
    void OnTargetEliminated();

    UFUNCTION(BlueprintCallable, Category = "Mission")
    void OnHeroineRescued();

    UFUNCTION(BlueprintCallable, Category = "Mission")
    void ReportPlayerDetected();

    // --- 이벤트 델리게이트 ---
    UPROPERTY(BlueprintAssignable, Category = "UI|Events")
    FOnScoreChangedSignature on_score_changed_;

    UPROPERTY(BlueprintAssignable, Category = "UI|Events")
    FOnGameOverSignature on_game_over_;

    UPROPERTY(BlueprintAssignable, Category = "UI|Events")
    FOnGameVictorySignature on_game_victory_;

private:
    void ProcessGameOver(const FString& fail_reason);
    void ProcessGameVictory(const FString& victory_reason);

    void CheckPlayerDetectionFromAI();
    void CheckPlayerDeath();

    // 상숫값 설정
    constexpr static int32 kBaseScore = 5000;              // 기본 점수
    constexpr static int32 kKillPenaltyAmount = 350;       // 초과 처치당 패널티 점수
    constexpr static int32 kBaseRewardCurrency = 3000;     // 기본 클리어 재화
    constexpr static float kScoreToCurrencyRatio = 0.5f;   // 점수 -> 재화 환산 비율 (0.5)

    constexpr static float kDetectionTimeLimit = 180.0f;
    constexpr static float kBossMapTimeLimit = 300.0f;

    UPROPERTY(VisibleAnywhere, Category = "Mission")
    bool is_game_over_ = false;

    UPROPERTY(VisibleAnywhere, Category = "Mission")
    bool is_game_cleared_ = false;

    UPROPERTY(VisibleAnywhere, Category = "Score")
    int32 current_score_ = 0;

    UPROPERTY(VisibleAnywhere, Category = "Mission")
    bool is_target_eliminated_ = false;

    UPROPERTY(VisibleAnywhere, Category = "Mission")
    bool is_escaped_ = false;

    UPROPERTY(VisibleAnywhere, Category = "Mission")
    bool was_detected_ = false;

    UPROPERTY(VisibleAnywhere, Category = "Timer")
    float stage_play_time_ = 0.0f;

    float detection_remaining_time_ = 0.0f;
    bool is_detection_timer_active_ = false;

    float boss_map_remaining_time_ = 0.0f;
    bool is_boss_map_timer_active_ = false;
};