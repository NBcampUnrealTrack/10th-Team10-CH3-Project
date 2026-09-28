#include "StairBossCinematicTrigger.h"

#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNodeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Chapter3_ShooterGame_PlayerController.h"
#include "CollisionShape.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "SlowMotionSkillComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogStairBossCinematic, Log, All);

struct FStairBossFinalPoseAnimProxy : public FAnimInstanceProxy
{
    explicit FStairBossFinalPoseAnimProxy(UAnimInstance* animInstance)
        : FAnimInstanceProxy(animInstance)
    {
    }

    void SetFinalPose(const FPoseSnapshot& pose)
    {
        finalPose_ = pose;
    }

protected:
    virtual void Initialize(UAnimInstance* animInstance) override
    {
        FAnimInstanceProxy::Initialize(animInstance);
        if (const UStairBossFinalPoseAnimInstance* instance = Cast<UStairBossFinalPoseAnimInstance>(animInstance))
        {
            finalPose_ = instance->GetFinalPose();
        }
    }

    virtual bool Evaluate(FPoseContext& output) override
    {
        output.ResetToRefPose();
        if (finalPose_.bIsValid)
        {
            const FBoneContainer& bones = output.Pose.GetBoneContainer();
            for (const FCompactPoseBoneIndex boneIndex : output.Pose.ForEachBoneIndex())
            {
                const int32 meshIndex = bones.MakeMeshPoseIndex(boneIndex).GetInt();
                if (finalPose_.LocalTransforms.IsValidIndex(meshIndex))
                {
                    output.Pose[boneIndex] = finalPose_.LocalTransforms[meshIndex];
                }
            }
        }
        return true;
    }

private:
    FPoseSnapshot finalPose_ = {};
};

bool UStairBossFinalPoseAnimInstance::SetFinalPose(const FPoseSnapshot& pose)
{
    const USkeletalMeshComponent* mesh = GetSkelMeshComponent();
    const USkeletalMesh* asset = IsValid(mesh) ? mesh->GetSkeletalMeshAsset() : nullptr;
    if (!IsValid(asset) || !pose.bIsValid || pose.SkeletalMeshName != asset->GetFName())
    {
        return false;
    }
    const FReferenceSkeleton& skeleton = asset->GetRefSkeleton();
    if (pose.LocalTransforms.Num() != skeleton.GetNum() || pose.BoneNames.Num() != skeleton.GetNum())
    {
        return false;
    }
    for (int32 boneIndex = 0; boneIndex < skeleton.GetNum(); ++boneIndex)
    {
        if (pose.BoneNames[boneIndex] != skeleton.GetBoneName(boneIndex)
            || pose.LocalTransforms[boneIndex].ContainsNaN())
        {
            return false;
        }
    }
    finalPose_ = pose;
    GetProxyOnGameThread<FStairBossFinalPoseAnimProxy>().SetFinalPose(finalPose_);
    return true;
}

FAnimInstanceProxy* UStairBossFinalPoseAnimInstance::CreateAnimInstanceProxy()
{
    return new FStairBossFinalPoseAnimProxy(this);
}

void UStairBossFinalPoseAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* proxy)
{
    delete static_cast<FStairBossFinalPoseAnimProxy*>(proxy);
}

AStairBossCinematicTrigger::AStairBossCinematicTrigger()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = triggerCheckInterval_;
    triggerBox_ = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    SetRootComponent(triggerBox_);
    triggerBox_->InitBoxExtent(triggerHalfExtent_);
    triggerBox_->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    triggerBox_->SetCollisionObjectType(ECC_WorldDynamic);
    triggerBox_->SetCollisionResponseToAllChannels(ECR_Ignore);
    triggerBox_->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    triggerBox_->SetGenerateOverlapEvents(true);
    triggerBox_->SetCanEverAffectNavigation(false);
    triggerBox_->SetHiddenInGame(true);
}

void AStairBossCinematicTrigger::OnConstruction(const FTransform& transform)
{
    Super::OnConstruction(transform);
    ApplyTriggerSettings();
}

void AStairBossCinematicTrigger::ApplyTriggerSettings()
{
    if (IsValid(triggerBox_) && !triggerHalfExtent_.ContainsNaN())
    {
        triggerBox_->SetBoxExtent(triggerHalfExtent_.GetAbs().ComponentMax(FVector::OneVector), false);
    }
    const float checkInterval = FMath::IsFinite(triggerCheckInterval_)
        ? FMath::Max(0.01f, triggerCheckInterval_) : 0.05f;
    SetActorTickInterval(checkInterval);
}

void AStairBossCinematicTrigger::BeginPlay()
{
    Super::BeginPlay();
    ApplyTriggerSettings();
    HideCinematicActors();
    triggerBox_->OnComponentBeginOverlap.AddDynamic(this, &AStairBossCinematicTrigger::HandleTriggerOverlap);
}

void AStairBossCinematicTrigger::Tick(float deltaSeconds)
{
    Super::Tick(deltaSeconds);
    if (playerStateCaptured_)
    {
        if (!playerController_.IsValid() || !playerPawn_.IsValid()
            || !IsValid(sequencePlayer_) || !sequencePlayer_->IsValid()
            || !IsValid(runtimeSequenceActor_))
        {
            UE_LOG(LogStairBossCinematic, Warning, TEXT("%s: cinematic reference was lost; restoring player state."), *GetName());
            ReleaseCinematic();
        }
        return;
    }
    if (!automaticallyCheckRange_ || (playOnce_ && hasStarted_) || endingPlay_)
    {
        return;
    }
    APawn* pawn = UGameplayStatics::GetPlayerPawn(this, 0);
    const bool pawnInside = IsPawnInTrigger(pawn);
    if (!pawnInside)
    {
        waitingForExit_ = false;
    }
    else if (!waitingForExit_)
    {
        StartCinematic();
    }
}

bool AStairBossCinematicTrigger::IsPawnInTrigger(const APawn* pawn) const
{
    if (!IsValid(pawn) || !IsValid(triggerBox_))
    {
        return false;
    }
    const FVector localPosition = triggerBox_->GetComponentTransform().InverseTransformPosition(pawn->GetActorLocation());
    const FVector halfExtent = triggerBox_->GetUnscaledBoxExtent();
    if (FMath::Abs(localPosition.X) <= halfExtent.X
        && FMath::Abs(localPosition.Y) <= halfExtent.Y
        && FMath::Abs(localPosition.Z) <= halfExtent.Z)
    {
        return true;
    }
    const ACharacter* character = Cast<ACharacter>(pawn);
    const UCapsuleComponent* capsule = character ? character->GetCapsuleComponent() : nullptr;
    if (IsValid(capsule))
    {
        const FCollisionShape capsuleShape = FCollisionShape::MakeCapsule(
            capsule->GetScaledCapsuleRadius(), capsule->GetScaledCapsuleHalfHeight());
        return triggerBox_->OverlapComponent(capsule->GetComponentLocation(),
            capsule->GetComponentQuat(), capsuleShape);
    }
    return false;
}

void AStairBossCinematicTrigger::HandleTriggerOverlap(UPrimitiveComponent* overlappedComponent,
    AActor* otherActor, UPrimitiveComponent* otherComponent, int32 otherBodyIndex,
    bool fromSweep, const FHitResult& sweepResult)
{
    if (automaticallyCheckRange_ && !waitingForExit_
        && otherActor == UGameplayStatics::GetPlayerPawn(this, 0))
    {
        StartCinematic();
    }
}

void AStairBossCinematicTrigger::ReportConfigurationError(const TCHAR* message)
{
    if (!reportedConfigurationError_)
    {
        UE_LOG(LogStairBossCinematic, Warning, TEXT("%s: %s"), *GetName(), message);
        reportedConfigurationError_ = true;
    }
}

bool AStairBossCinematicTrigger::PrepareSequencePlayer()
{
    if (IsValid(sequenceActor_))
    {
        if (sequenceActor_->GetWorld() != GetWorld() || !IsValid(sequenceActor_->GetSequence()))
        {
            ReportConfigurationError(TEXT("Assign a Level Sequence Actor with a valid sequence in this level."));
            return false;
        }
        if (IsValid(sequenceAsset_) && sequenceActor_->GetSequence() != sequenceAsset_)
        {
            ReportConfigurationError(TEXT("Sequence Asset must match the assigned Level Sequence Actor, or be empty."));
            return false;
        }
        runtimeSequenceActor_ = sequenceActor_;
        sequencePlayer_ = sequenceActor_->GetSequencePlayer();
        previousPlaybackSettings_ = sequenceActor_->PlaybackSettings;
    }
    else if (IsValid(sequenceAsset_))
    {
        FMovieSceneSequencePlaybackSettings settings = {};
        ALevelSequenceActor* createdActor = nullptr;
        sequencePlayer_ = ULevelSequencePlayer::CreateLevelSequencePlayer(this, sequenceAsset_, settings, createdActor);
        runtimeSequenceActor_ = createdActor;
        createdSequenceActor_ = IsValid(createdActor);
        previousPlaybackSettings_ = settings;
    }
    else
    {
        ReportConfigurationError(TEXT("Assign Sequence Actor or Sequence Asset before starting the cinematic."));
        return false;
    }
    if (!IsValid(sequencePlayer_) || !sequencePlayer_->IsValid()
        || sequencePlayer_->GetDuration().AsSeconds() <= 0.0
        || sequencePlayer_->IsPlaying() || sequencePlayer_->IsPaused())
    {
        ReportConfigurationError(TEXT("The sequence player must be initialized, nonempty, and stopped. Disable Auto Play."));
        if (createdSequenceActor_ && IsValid(runtimeSequenceActor_))
        {
            runtimeSequenceActor_->Destroy();
        }
        sequencePlayer_ = nullptr;
        runtimeSequenceActor_ = nullptr;
        createdSequenceActor_ = false;
        return false;
    }
    FMovieSceneSequencePlaybackSettings settings = previousPlaybackSettings_;
    settings.bAutoPlay = false;
    settings.LoopCount.Value = 0;
    settings.StartTime = 0.0f;
    settings.bRandomStartTime = false;
    settings.bDisableMovementInput = false;
    settings.bDisableLookAtInput = false;
    settings.bHidePlayer = false;
    settings.bHideHud = false;
    settings.bDisableCameraCuts = false;
    settings.bPauseAtEnd = true;
    settings.PlayRate = FMath::IsFinite(settings.PlayRate) && settings.PlayRate > 0.0f
        ? settings.PlayRate : 1.0f;
    settings.FinishCompletionStateOverride = EMovieSceneCompletionModeOverride::ForceRestoreState;
    sequencePlayer_->SetPlaybackSettings(settings);
    sequencePlayer_->OnFinished.AddDynamic(this, &AStairBossCinematicTrigger::HandleSequenceFinished);
    sequencePlayer_->OnStop.AddDynamic(this, &AStairBossCinematicTrigger::HandleSequenceStopped);
    reportedConfigurationError_ = false;
    return true;
}

bool AStairBossCinematicTrigger::StartCinematic()
{
    if (endingPlay_ || playerStateCaptured_ || (playOnce_ && hasStarted_)
        || !GetWorld() || !GetWorld()->IsGameWorld() || GetWorld()->IsPaused())
    {
        return false;
    }
    APlayerController* controller = UGameplayStatics::GetPlayerController(this, 0);
    APawn* pawn = IsValid(controller) ? controller->GetPawn() : nullptr;
    if (!IsValid(controller) || !controller->IsLocalController() || !IsValid(pawn)
        || !IsValid(controller->PlayerCameraManager))
    {
        return false;
    }
    if (!FMath::IsFinite(fadeOutDuration_) || fadeOutDuration_ < 0.0f)
    {
        ReportConfigurationError(TEXT("Fade Out Duration must be a finite nonnegative value."));
        return false;
    }
    if (!FMath::IsFinite(returnFadeInDuration_) || returnFadeInDuration_ < 0.0f)
    {
        ReportConfigurationError(TEXT("Return Fade In Duration must be a finite nonnegative value."));
        return false;
    }
    if (controller->bCinematicMode)
    {
        return false;
    }
    if (!PrepareSequencePlayer())
    {
        return false;
    }
    CaptureAndLockPlayer(controller, pawn);
    hasStarted_ = true;
    hasFinished_ = false;
    waitingForExit_ = true;
    controller->PlayerCameraManager->StartCameraFade(
        previousFadeEnabled_ ? previousFadeAmount_ : 0.0f, 1.0f,
        fadeOutDuration_, FLinearColor::Black, false, true);
    if (fadeOutDuration_ > 0.0f)
    {
        GetWorldTimerManager().SetTimer(fadeTimer_, this,
            &AStairBossCinematicTrigger::BeginSequencePlayback, fadeOutDuration_, false);
    }
    else
    {
        BeginSequencePlayback();
    }
    UE_LOG(LogStairBossCinematic, Log, TEXT("%s: cinematic started; player input locked."), *GetName());
    onStarted_.Broadcast();
    return true;
}

void AStairBossCinematicTrigger::CaptureAndLockPlayer(APlayerController* controller, APawn* pawn)
{
    playerController_ = controller;
    playerPawn_ = pawn;
    previousViewTarget_ = controller->GetViewTarget();
    previousControllerInputEnabled_ = controller->InputEnabled();
    previousPawnInputEnabled_ = pawn->InputEnabled();
    previousCinematicMode_ = controller->bCinematicMode;
    previousHidePawnInCinematicMode_ = controller->bHidePawnInCinematicMode;
    previousPawnHidden_ = pawn->IsHidden();
    previousPawnCanBeDamaged_ = pawn->CanBeDamaged();
    AHUD* hud = controller->GetHUD();
    previousShowHud_ = IsValid(hud) ? hud->bShowHUD : true;
    APlayerCameraManager* camera = controller->PlayerCameraManager;
    previousFadeEnabled_ = camera->bEnableFading;
    previousFadeAmount_ = camera->FadeAmount;
    previousFadeColor_ = camera->FadeColor;
    previousFadeAudio_ = camera->bFadeAudio;
    playerStateCaptured_ = true;
    controller->FlushPressedKeys();
    if (AChapter3_ShooterGame_PlayerController* gameController = Cast<AChapter3_ShooterGame_PlayerController>(controller))
    {
        gameController->StopAiming();
    }
    if (USlowMotionSkillComponent* slowMotion = controller->FindComponentByClass<USlowMotionSkillComponent>())
    {
        slowMotion->CancelSlowMotion();
    }
    controller->DisableInput(controller);
    pawn->DisableInput(controller);
    controller->SetIgnoreMoveInput(true);
    controller->SetIgnoreLookInput(true);
    controller->SetCinematicMode(true, false, true, false, false);
    if (hidePlayerWidgets_)
    {
        TArray<UUserWidget*> widgets = {};
        UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, widgets, UUserWidget::StaticClass(), true);
        for (UUserWidget* widget : widgets)
        {
            if (IsValid(widget) && widget->GetOwningPlayer() == controller)
            {
                previousWidgetVisibility_.Add(widget, static_cast<uint8>(widget->GetVisibility()));
                widget->SetVisibility(ESlateVisibility::Hidden);
            }
        }
    }
    pawn->ConsumeMovementInputVector();
    if (ACharacter* character = Cast<ACharacter>(pawn))
    {
        UCharacterMovementComponent* movement = character->GetCharacterMovement();
        if (IsValid(movement))
        {
            playerMovement_ = movement;
            previousMovementMode_ = static_cast<uint8>(movement->MovementMode);
            previousCustomMovementMode_ = movement->CustomMovementMode;
            movement->StopMovementImmediately();
            movement->DisableMovement();
        }
    }
    if (protectPlayer_)
    {
        pawn->SetCanBeDamaged(false);
    }
}

void AStairBossCinematicTrigger::BeginSequencePlayback()
{
    APlayerController* controller = playerController_.Get();
    APawn* pawn = playerPawn_.Get();
    if (endingPlay_ || !playerStateCaptured_ || !IsValid(controller) || !IsValid(pawn)
        || !IsValid(controller->PlayerCameraManager) || !IsValid(sequencePlayer_)
        || !sequencePlayer_->IsValid() || !IsValid(runtimeSequenceActor_))
    {
        ReleaseCinematic();
        return;
    }
    if (hidePlayerDuringSequence_)
    {
        pawn->SetActorHiddenInGame(true);
    }
    ShowCinematicActors();
    controller->PlayerCameraManager->StopCameraFade();
    sequencePlayer_->SetPlaybackPosition(FMovieSceneSequencePlaybackParams(
        sequencePlayer_->GetStartTime().Time, EUpdatePositionMethod::Jump));
    sequencePlaybackStarted_ = true;
    sequencePlayer_->Play();
    UE_LOG(LogStairBossCinematic, Log, TEXT("%s: sequence playback began."), *GetName());
}

void AStairBossCinematicTrigger::HideCinematicActors()
{
    for (AActor* actor : cinematicActors_)
    {
        if (IsValid(actor) && actor != this && actor->GetWorld() == GetWorld())
        {
            previousCinematicVisibility_.FindOrAdd(actor, actor->IsHidden());
            actor->SetActorHiddenInGame(true);
        }
    }
}

void AStairBossCinematicTrigger::ShowCinematicActors()
{
    for (AActor* actor : cinematicActors_)
    {
        if (IsValid(actor) && actor != this && actor->GetWorld() == GetWorld())
        {
            previousCinematicVisibility_.FindOrAdd(actor, actor->IsHidden());
            actor->SetActorHiddenInGame(false);
        }
    }
}

void AStairBossCinematicTrigger::RestoreCinematicActors()
{
    for (const TPair<TWeakObjectPtr<AActor>, bool>& entry : previousCinematicVisibility_)
    {
        if (AActor* actor = entry.Key.Get())
        {
            actor->SetActorHiddenInGame(entry.Value);
        }
    }
    previousCinematicVisibility_.Reset();
}

void AStairBossCinematicTrigger::HandleSequenceFinished()
{
    if (!playerStateCaptured_ || hasFinished_ || endingPlay_)
    {
        return;
    }
    hasFinished_ = true;
    if (holdBlackAtEnd_)
    {
        APlayerController* controller = playerController_.Get();
        if (IsValid(controller) && IsValid(controller->PlayerCameraManager))
        {
            controller->PlayerCameraManager->SetManualCameraFade(1.0f, FLinearColor::Black, false);
        }
    }
    else
    {
        APlayerController* controller = playerController_.Get();
        ReleaseCinematic();
        if (returnFadeInDuration_ > 0.0f && IsValid(controller)
            && IsValid(controller->PlayerCameraManager))
        {
            controller->PlayerCameraManager->StartCameraFade(
                1.0f, 0.0f, returnFadeInDuration_, FLinearColor::Black, false, false);
        }
    }
    UE_LOG(LogStairBossCinematic, Log, TEXT("%s: cinematic finished; hold black=%s."),
        *GetName(), holdBlackAtEnd_ ? TEXT("true") : TEXT("false"));
    onFinished_.Broadcast();
}

void AStairBossCinematicTrigger::HandleSequenceStopped()
{
    if (playerStateCaptured_ && !hasFinished_ && !endingPlay_)
    {
        UE_LOG(LogStairBossCinematic, Warning, TEXT("%s: sequence stopped early; restoring player state."), *GetName());
        ReleaseCinematic();
    }
}

void AStairBossCinematicTrigger::ClearSequencePlayer()
{
    if (IsValid(sequencePlayer_))
    {
        sequencePlayer_->OnFinished.RemoveDynamic(this, &AStairBossCinematicTrigger::HandleSequenceFinished);
        sequencePlayer_->OnStop.RemoveDynamic(this, &AStairBossCinematicTrigger::HandleSequenceStopped);
        if (sequencePlaybackStarted_ && sequencePlayer_->IsValid())
        {
            const bool preserveFinishedState = preserveSequenceStateOnFinish_ && hasFinished_;
            struct FFinalPose
            {
                TWeakObjectPtr<USkeletalMeshComponent> mesh = nullptr;
                FPoseSnapshot pose = {};
                FTransform transform = FTransform::Identity;
            };
            TArray<FFinalPose> finalPoses = {};
            if (preserveFinishedState)
            {
                for (AActor* actor : finalPoseActors_)
                {
                    if (!IsValid(actor) || actor->GetWorld() != GetWorld() || actor == playerPawn_.Get())
                    {
                        continue;
                    }
                    TArray<USkeletalMeshComponent*> meshes = {};
                    actor->GetComponents<USkeletalMeshComponent>(meshes);
                    for (USkeletalMeshComponent* mesh : meshes)
                    {
                        if (!IsValid(mesh) || !IsValid(mesh->GetSkeletalMeshAsset()))
                        {
                            continue;
                        }
                        mesh->HandleExistingParallelEvaluationTask(true, true);
                        if (mesh->GetNumComponentSpaceTransforms() != mesh->GetNumBones()
                            || mesh->GetNumBones() == 0)
                        {
                            UE_LOG(LogStairBossCinematic, Warning, TEXT("%s: cannot capture final pose for %s; bone transforms are incomplete."), *GetName(), *mesh->GetName());
                            continue;
                        }
                        FFinalPose finalPose = {};
                        finalPose.mesh = mesh;
                        finalPose.transform = mesh->GetComponentTransform();
                        mesh->SnapshotPose(finalPose.pose);
                        if (finalPose.pose.bIsValid)
                        {
                            finalPoses.Add(MoveTemp(finalPose));
                        }
                    }
                }
                sequencePlayer_->SetCompletionModeOverride(EMovieSceneCompletionModeOverride::None);
            }
            sequencePlayer_->Stop();
            if (preserveFinishedState)
            {
                sequencePlayer_->DiscardPreAnimatedState();
            }
            else
            {
                sequencePlayer_->RestoreState();
            }
            for (const FFinalPose& finalPose : finalPoses)
            {
                USkeletalMeshComponent* mesh = finalPose.mesh.Get();
                if (!IsValid(mesh) || !mesh->IsRegistered())
                {
                    continue;
                }
                mesh->SetWorldTransform(finalPose.transform, false, nullptr, ETeleportType::TeleportPhysics);
                mesh->SetDisablePostProcessBlueprint(true);
                mesh->SetAnimInstanceClass(UStairBossFinalPoseAnimInstance::StaticClass());
                UStairBossFinalPoseAnimInstance* instance = Cast<UStairBossFinalPoseAnimInstance>(mesh->GetAnimInstance());
                if (!IsValid(instance) || !instance->SetFinalPose(finalPose.pose))
                {
                    UE_LOG(LogStairBossCinematic, Warning, TEXT("%s: cannot apply final pose for %s; snapshot does not match its skeleton."), *GetName(), *mesh->GetName());
                    continue;
                }
                mesh->TickAnimation(0.0f, false);
                mesh->RefreshBoneTransforms();
                mesh->UpdateBounds();
                mesh->MarkRenderTransformDirty();
                mesh->MarkRenderDynamicDataDirty();
            }
        }
        sequencePlayer_->SetPlaybackSettings(previousPlaybackSettings_);
    }
    if (createdSequenceActor_ && IsValid(runtimeSequenceActor_))
    {
        runtimeSequenceActor_->Destroy();
    }
    sequencePlayer_ = nullptr;
    runtimeSequenceActor_ = nullptr;
    createdSequenceActor_ = false;
    sequencePlaybackStarted_ = false;
}

void AStairBossCinematicTrigger::RestorePlayerState()
{
    if (!playerStateCaptured_)
    {
        return;
    }
    APlayerController* controller = playerController_.Get();
    APawn* pawn = playerPawn_.Get();
    if (IsValid(controller))
    {
        controller->SetCinematicMode(previousCinematicMode_, previousHidePawnInCinematicMode_, false, false, false);
        controller->SetIgnoreMoveInput(false);
        controller->SetIgnoreLookInput(false);
        controller->FlushPressedKeys();
        if (previousControllerInputEnabled_)
        {
            controller->EnableInput(controller);
        }
        if (AHUD* hud = controller->GetHUD())
        {
            hud->bShowHUD = previousShowHud_;
        }
        if (previousViewTarget_.IsValid())
        {
            controller->SetViewTarget(previousViewTarget_.Get());
        }
        if (APlayerCameraManager* camera = controller->PlayerCameraManager)
        {
            camera->StopCameraFade();
            if (previousFadeEnabled_)
            {
                camera->SetManualCameraFade(previousFadeAmount_, previousFadeColor_, previousFadeAudio_);
            }
        }
    }
    if (IsValid(pawn))
    {
        pawn->SetActorHiddenInGame(previousPawnHidden_);
        pawn->SetCanBeDamaged(previousPawnCanBeDamaged_);
        if (previousPawnInputEnabled_ && IsValid(controller) && pawn->GetController() == controller)
        {
            pawn->EnableInput(controller);
        }
        pawn->ConsumeMovementInputVector();
    }
    if (UCharacterMovementComponent* movement = playerMovement_.Get())
    {
        movement->SetMovementMode(static_cast<EMovementMode>(previousMovementMode_), previousCustomMovementMode_);
    }
    for (const TPair<TWeakObjectPtr<UUserWidget>, uint8>& entry : previousWidgetVisibility_)
    {
        if (UUserWidget* widget = entry.Key.Get())
        {
            widget->SetVisibility(static_cast<ESlateVisibility>(entry.Value));
        }
    }
    previousWidgetVisibility_.Reset();
    playerStateCaptured_ = false;
    playerController_.Reset();
    playerPawn_.Reset();
    previousViewTarget_.Reset();
    playerMovement_.Reset();
}

void AStairBossCinematicTrigger::ReleaseCinematic()
{
    if (GetWorld())
    {
        GetWorldTimerManager().ClearTimer(fadeTimer_);
    }
    ClearSequencePlayer();
    RestorePlayerState();
}

void AStairBossCinematicTrigger::EndPlay(const EEndPlayReason::Type endPlayReason)
{
    endingPlay_ = true;
    ReleaseCinematic();
    RestoreCinematicActors();
    Super::EndPlay(endPlayReason);
}
