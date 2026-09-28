#include "TeleportElevator.h"

#include "KeyCard.h"

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

    const bool bSuccess = Player->TeleportTo(
        TeleportDestination->GetActorLocation(),
        Player->GetActorRotation(),
        false,
        false);

    if (bSuccess)
    {
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
            TEXT("Teleport failed. Check destination collision."));
    }
}

void ATeleportElevator::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(LocalController))
    {
        DisableInput(LocalController);
    }

    Super::EndPlay(EndPlayReason);
}