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

    // --- 점수 관련 ---
    UFUNCTION(BlueprintCallable, Category = "Score")
    void AddScore(int32 amount);

    UFUNCTION(BlueprintPure, Category = "UI|Score")
    int32 GetCurrentScore() const;

    UPROPERTY(BlueprintAssignable, Category = "UI|Events")
    FOnScoreChangedSignature on_score_changed_;

    UPROPERTY(BlueprintAssignable, Category = "UI|Events")
    FOnGameOverSignature on_game_over_;

    UPROPERTY(BlueprintAssignable, Category = "UI|Events")
    FOnGameVictorySignature on_game_victory_;

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

    // --- 적 AI가 플레이어를 감지하여 타이머 호출 ---
    UFUNCTION(BlueprintCallable, Category = "Mission")
    void ReportPlayerDetected();

    // ==========================================
    // 임시 함수 사용 (접두어 통일: Dummy_)
    // ==========================================

    UFUNCTION(BlueprintCallable, Category = "TeamDummy")
    void Dummy_ReceiveEnemyEliminated();

    UFUNCTION(BlueprintCallable, Category = "TeamDummy")
    void Dummy_ReceivePlayerEscaped();

    UFUNCTION(BlueprintCallable, Category = "TeamDummy")
    void Dummy_ReceiveHeroineRescued();

    // --- 시작 / 종료 ---
    UFUNCTION(BlueprintCallable, Category = "Mission|Control")
    void TriggerGameStart();

    UFUNCTION(BlueprintCallable, Category = "Mission|Control")
    void TriggerGameEnd();

    // None이면 PIE 접두어를 제거한 현재 맵 이름을 저장 ID로 사용합니다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Progress")
    FName progress_level_id_;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Progress", meta = (ClampMin = "0", UIMin = "0"))
    int64 mission_reward_amount_ = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Progress")
    bool first_clear_reward_only_ = false;

private:
    void ProcessGameOver(const FString& fail_reason);
    void ProcessGameVictory(const FString& victory_reason);

    // --- 상태 확인용 검사 함수 ---
    void CheckPlayerDetectionFromAI();
    void CheckPlayerDeath();

    // 맵 실행마다 새 ID를 부여하여 같은 임무 완료의 중복 지급을 막습니다.
    FGuid mission_run_id_;

    // --- 상태 변수 ---
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

    // --- 타이머 변수 ---
    constexpr static float kDetectionTimeLimit = 180.0f;
    float detection_remaining_time_ = 0.0f;
    bool is_detection_timer_active_ = false;

    constexpr static float kBossMapTimeLimit = 300.0f;
    float boss_map_remaining_time_ = 0.0f;
    bool is_boss_map_timer_active_ = false;
};
