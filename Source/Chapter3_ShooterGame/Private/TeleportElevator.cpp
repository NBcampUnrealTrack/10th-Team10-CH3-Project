#include "TeleportElevator.h"

#include "KeyCard.h"

#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

ATeleportElevator::ATeleportElevator()
{
    PrimaryActorTick.bCanEverTick = false;

    InteractionBox =
        CreateDefaultSubobject<UBoxComponent>(
            TEXT("InteractionBox"));

    SetRootComponent(InteractionBox);

    InteractionBox->SetBoxExtent(
        FVector(150.0f, 150.0f, 150.0f));

    InteractionBox->SetCollisionEnabled(
        ECollisionEnabled::QueryOnly);

    InteractionBox->SetCollisionResponseToAllChannels(
        ECR_Ignore);

    InteractionBox->SetCollisionResponseToChannel(
        ECC_Pawn,
        ECR_Overlap);

    InteractionBox->SetGenerateOverlapEvents(true);
}

void ATeleportElevator::BeginPlay()
{
    Super::BeginPlay();

    LocalController =
        UGameplayStatics::GetPlayerController(this, 0);

    if (!IsValid(LocalController))
    {
        return;
    }

    EnableInput(LocalController);

    if (InputComponent)
    {
        FInputKeyBinding& Binding =
            InputComponent->BindKey(
                EKeys::F,
                IE_Pressed,
                this,
                &ATeleportElevator::TryTeleport);

        Binding.bConsumeInput = false;
    }
}

void ATeleportElevator::TryTeleport()
{
    if (bIsTeleporting)
    {
        return;
    }

    if (!IsValid(LocalController) || !IsValid(InteractionBox))
    {
        return;
    }

    APawn* Player = LocalController->GetPawn();

    if (!IsValid(Player))
    {
        return;
    }


    const FVector LocalPosition =
        InteractionBox->GetComponentTransform()
        .InverseTransformPosition(Player->GetActorLocation());

    const FVector Extent =
        InteractionBox->GetUnscaledBoxExtent();

    const bool bInsideBox =
        FMath::Abs(LocalPosition.X) <= Extent.X &&
        FMath::Abs(LocalPosition.Y) <= Extent.Y &&
        FMath::Abs(LocalPosition.Z) <= Extent.Z;

 
    if (!bInsideBox)
    {
        return;
    }


    if (!IsValid(RequiredCard))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("RequiredCard is not assigned."));

        return;
    }

   
    if (!RequiredCard->IsCollected())
    {
        UE_LOG(
            LogTemp,
            Log,
            TEXT("A card is required."));

        return;
    }


    if (!IsValid(TeleportDestination))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("TeleportDestination is not assigned."));

        return;
    }
    APlayerCameraManager* CameraManager =
        LocalController->PlayerCameraManager;

    if (!IsValid(CameraManager))
    {
        return;
    }

    
    if (CameraManager->bEnableFading ||
        LocalController->IsMoveInputIgnored() ||
        LocalController->IsLookInputIgnored())
    {
        return;
    }

    bIsTeleporting = true;

    LocalController->SetIgnoreMoveInput(true);
    LocalController->SetIgnoreLookInput(true);

    if (UPawnMovementComponent* Movement =
        Player->GetMovementComponent())
    {
        Movement->StopMovementImmediately();
    }

    const float Duration = FMath::Max(FadeDuration, 0.01f);

   
    CameraManager->StartCameraFade(
        0.0f,
        1.0f,
        Duration,
        FLinearColor::Black,
        false,
        true
    );

  
    GetWorldTimerManager().SetTimer(
        TeleportTimerHandle,
        this,
        &ATeleportElevator::ExecuteTeleport,
        Duration,
        false
    );


}

void ATeleportElevator::ExecuteTeleport()
{
    if (!IsValid(LocalController))
    {
        FinishTeleport();
        return;
    }

    APawn* Player = LocalController->GetPawn();

    if (IsValid(Player) && IsValid(TeleportDestination))
    {
        const FRotator ArrivalRotation(
            0.0f,
            TeleportDestination->GetActorRotation().Yaw,
            0.0f);

        const bool bSuccess = Player->TeleportTo(
            TeleportDestination->GetActorLocation(),
            ArrivalRotation,
            false,
            false
        );

        if (bSuccess)
        {
            LocalController->SetControlRotation(ArrivalRotation);

            if (UPawnMovementComponent* Movement =
                Player->GetMovementComponent())
            {
                Movement->StopMovementImmediately();
            }

            UE_LOG(LogTemp, Log, TEXT("Teleport succeeded."));
        }
        else
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("Teleport failed. Check destination collision.")
            );
        }
    }

    APlayerCameraManager* CameraManager =
        LocalController->PlayerCameraManager;

    if (!IsValid(CameraManager))
    {
        FinishTeleport();
        return;
    }

    const float Duration = FMath::Max(FadeDuration, 0.01f);

    CameraManager->StartCameraFade(
        1.0f,
        0.0f,
        Duration,
        FLinearColor::Black,
        false,
        false
    );

    GetWorldTimerManager().SetTimer(
        FadeFinishTimerHandle,
        this,
        &ATeleportElevator::FinishTeleport,
        Duration,
        false
    );
}
void ATeleportElevator::FinishTeleport()
{
    if (!bIsTeleporting)
    {
        return;
    }

    if (IsValid(LocalController))
    {
        LocalController->SetIgnoreMoveInput(false);
        LocalController->SetIgnoreLookInput(false);
    }

    bIsTeleporting = false;
}
void ATeleportElevator::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(TeleportTimerHandle);
    GetWorldTimerManager().ClearTimer(FadeFinishTimerHandle);

    if (bIsTeleporting && IsValid(LocalController))
    {
        if (IsValid(LocalController->PlayerCameraManager))
        {
            LocalController->PlayerCameraManager->StopCameraFade();
        }
    }

    FinishTeleport();

    if (IsValid(LocalController))
    {
        DisableInput(LocalController);
    }

    Super::EndPlay(EndPlayReason);
}
