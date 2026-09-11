#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreChangedSignature, int32, new_score);

UCLASS()
class CHAPTER3_SHOOTERGAME_API AShooterGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AShooterGameMode();

    virtual void BeginPlay() override;
    virtual void Tick(float delta_seconds) override;

    // --- 점수 관련 ---
    UFUNCTION(BlueprintCallable, Category = "Score")
    void AddScore(int32 amount);

    UFUNCTION(BlueprintPure, Category = "Score")
    int32 GetCurrentScore() const;

    UPROPERTY(BlueprintAssignable, Category = "Score")
    FOnScoreChangedSignature on_score_changed_;

    // ==========================================
    // 임시 함수 사용 (접두어 통일: Dummy_)
    // ==========================================

    UFUNCTION(BlueprintCallable, Category = "TeamDummy")
    void Dummy_ReceivePlayerHealth(float current_health);

    UFUNCTION(BlueprintCallable, Category = "TeamDummy")
    void Dummy_ReceiveEnemyEliminated();

    UFUNCTION(BlueprintCallable, Category = "TeamDummy")
    void Dummy_ReceivePlayerDetected();

    UFUNCTION(BlueprintCallable, Category = "TeamDummy")
    void Dummy_ReceivePlayerEscaped();

    UFUNCTION(BlueprintCallable, Category = "TeamDummy")
    void Dummy_ReceiveHeroineRescued();

    // --- 시작 / 종료 ---
    UFUNCTION(BlueprintCallable, Category = "Mission|Control")
    void TriggerGameStart();

    UFUNCTION(BlueprintCallable, Category = "Mission|Control")
    void TriggerGameEnd();

private:
    void ProcessGameOver(const FString& fail_reason);
    void ProcessGameVictory(const FString& victory_reason);

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