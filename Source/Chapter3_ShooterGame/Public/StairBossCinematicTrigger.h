#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/PoseSnapshot.h"
#include "GameFramework/Actor.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "TimerManager.h"
#include "StairBossCinematicTrigger.generated.h"

class ALevelSequenceActor;
class APawn;
class APlayerController;
class UBoxComponent;
class UCharacterMovementComponent;
class ULevelSequence;
class ULevelSequencePlayer;
class UPrimitiveComponent;
class UUserWidget;

UCLASS(Transient, NotBlueprintable)
class CHAPTER3_SHOOTERGAME_API UStairBossFinalPoseAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    bool SetFinalPose(const FPoseSnapshot& pose);
    const FPoseSnapshot& GetFinalPose() const { return finalPose_; }

protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* proxy) override;

private:
    UPROPERTY(Transient)
    FPoseSnapshot finalPose_ = {};
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStairBossCinematicEvent);

UCLASS(Blueprintable)
class CHAPTER3_SHOOTERGAME_API AStairBossCinematicTrigger : public AActor
{
    GENERATED_BODY()

public:
    AStairBossCinematicTrigger();

    virtual void OnConstruction(const FTransform& transform) override;
    virtual void Tick(float deltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "Cinematic")
    bool StartCinematic();

    UFUNCTION(BlueprintCallable, Category = "Cinematic")
    void ReleaseCinematic();

    UFUNCTION(BlueprintPure, Category = "Cinematic")
    bool HasStarted() const { return hasStarted_; }

    UFUNCTION(BlueprintPure, Category = "Cinematic")
    bool HasFinished() const { return hasFinished_; }

    UFUNCTION(BlueprintPure, Category = "Cinematic")
    bool IsCinematicActive() const { return playerStateCaptured_; }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinematic|Trigger")
    TObjectPtr<UBoxComponent> triggerBox_ = nullptr;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Cinematic|Sequence")
    TObjectPtr<ALevelSequenceActor> sequenceActor_ = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Sequence")
    TObjectPtr<ULevelSequence> sequenceAsset_ = nullptr;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Cinematic|Sequence")
    TArray<TObjectPtr<AActor>> cinematicActors_ = {};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Trigger", meta = (ClampMin = "1.0", Units = "cm"))
    FVector triggerHalfExtent_ = FVector(140.0, 110.0, 130.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Trigger")
    bool automaticallyCheckRange_ = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Trigger", meta = (ClampMin = "0.01", Units = "s"))
    float triggerCheckInterval_ = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Trigger")
    bool playOnce_ = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Playback", meta = (ClampMin = "0.0", Units = "s"))
    float fadeOutDuration_ = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Playback")
    bool holdBlackAtEnd_ = true;

    // Restore the player/HUD before the GameMode broadcasts victory to the result widget.
    // When enabled, this takes precedence over Hold Black At End.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Completion",
        meta = (DisplayName = "Complete Mission On Finish", ToolTip = "On natural sequence completion, restore player/UI state and complete the mission through the current GameMode. Overrides Hold Black At End. Stopping or cancelling does not complete the mission."))
    bool completeMissionOnFinish_ = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Playback")
    bool preserveSequenceStateOnFinish_ = false;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Cinematic|Playback")
    TArray<TObjectPtr<AActor>> finalPoseActors_ = {};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Playback", meta = (ClampMin = "0.0", Units = "s"))
    float returnFadeInDuration_ = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Playback")
    bool protectPlayer_ = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Playback")
    bool hidePlayerDuringSequence_ = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic|Playback")
    bool hidePlayerWidgets_ = true;

    UPROPERTY(BlueprintAssignable, Category = "Cinematic")
    FOnStairBossCinematicEvent onStarted_;

    UPROPERTY(BlueprintAssignable, Category = "Cinematic")
    FOnStairBossCinematicEvent onFinished_;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

private:
    void ApplyTriggerSettings();
    bool IsPawnInTrigger(const APawn* pawn) const;
    bool PrepareSequencePlayer();
    void CaptureAndLockPlayer(APlayerController* controller, APawn* pawn);
    void BeginSequencePlayback();
    void HideCinematicActors();
    void ShowCinematicActors();
    void RestoreCinematicActors();
    void RestorePlayerState();
    void ClearSequencePlayer();
    void ReportConfigurationError(const TCHAR* message);

    UFUNCTION()
    void HandleTriggerOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor,
        UPrimitiveComponent* otherComponent, int32 otherBodyIndex, bool fromSweep,
        const FHitResult& sweepResult);

    UFUNCTION()
    void HandleSequenceFinished();

    UFUNCTION()
    void HandleSequenceStopped();

    UPROPERTY(Transient)
    TObjectPtr<ULevelSequencePlayer> sequencePlayer_ = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<ALevelSequenceActor> runtimeSequenceActor_ = nullptr;

    UPROPERTY(Transient)
    TWeakObjectPtr<APlayerController> playerController_ = nullptr;

    UPROPERTY(Transient)
    TWeakObjectPtr<APawn> playerPawn_ = nullptr;

    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> previousViewTarget_ = nullptr;

    UPROPERTY(Transient)
    TWeakObjectPtr<UCharacterMovementComponent> playerMovement_ = nullptr;

    FMovieSceneSequencePlaybackSettings previousPlaybackSettings_ = {};
    TMap<TWeakObjectPtr<AActor>, bool> previousCinematicVisibility_ = {};
    TMap<TWeakObjectPtr<UUserWidget>, uint8> previousWidgetVisibility_ = {};
    FTimerHandle fadeTimer_ = {};
    FLinearColor previousFadeColor_ = FLinearColor::Black;
    float previousFadeAmount_ = 0.0f;
    uint8 previousMovementMode_ = 0;
    uint8 previousCustomMovementMode_ = 0;
    bool previousControllerInputEnabled_ = false;
    bool previousPawnInputEnabled_ = false;
    bool previousCinematicMode_ = false;
    bool previousHidePawnInCinematicMode_ = false;
    bool previousPawnHidden_ = false;
    bool previousPawnCanBeDamaged_ = false;
    bool previousShowHud_ = true;
    bool previousFadeEnabled_ = false;
    bool previousFadeAudio_ = false;
    bool playerStateCaptured_ = false;
    bool createdSequenceActor_ = false;
    bool sequencePlaybackStarted_ = false;
    bool hasStarted_ = false;
    bool hasFinished_ = false;
    bool waitingForExit_ = false;
    bool reportedConfigurationError_ = false;
    bool endingPlay_ = false;
};
