#include "KeyCard.h"

#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

AKeyCard::AKeyCard()
{
    PrimaryActorTick.bCanEverTick = false;

    // 카드 획득 범위
    InteractionBox =
        CreateDefaultSubobject<UBoxComponent>(
            TEXT("InteractionBox"));

    SetRootComponent(InteractionBox);

    InteractionBox->SetBoxExtent(
        FVector(100.0f, 100.0f, 100.0f));

    InteractionBox->SetCollisionEnabled(
        ECollisionEnabled::QueryOnly);

    InteractionBox->SetCollisionResponseToAllChannels(
        ECR_Ignore);

    InteractionBox->SetCollisionResponseToChannel(
        ECC_Pawn,
        ECR_Overlap);

    InteractionBox->SetGenerateOverlapEvents(true);

  
    CardMesh =
        CreateDefaultSubobject<UStaticMeshComponent>(
            TEXT("CardMesh"));

    CardMesh->SetupAttachment(InteractionBox);

  
    CardMesh->SetCollisionEnabled(
        ECollisionEnabled::NoCollision);
}

void AKeyCard::BeginPlay()
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
                &AKeyCard::TryCollect);

        // 다른 액터와 기존 플레이어의 F 입력을 막지 않는다.
        Binding.bConsumeInput = false;
    }
}

void AKeyCard::TryCollect()
{
    UE_LOG(LogTemp, Warning, TEXT("Card: F received"));

    APawn* DebugPlayer =
        IsValid(LocalController) ? LocalController->GetPawn() : nullptr;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Player=%s, Overlap=%d, Collected=%d"),
        *GetNameSafe(DebugPlayer),
        DebugPlayer && InteractionBox->IsOverlappingActor(DebugPlayer),
        bCollected);

    if (bCollected || !IsValid(LocalController))
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

   
    const FVector Extent = InteractionBox->GetUnscaledBoxExtent();

    const bool bInsideBox =
        FMath::Abs(LocalPosition.X) <= Extent.X &&
        FMath::Abs(LocalPosition.Y) <= Extent.Y &&
        FMath::Abs(LocalPosition.Z) <= Extent.Z;

    if (!bInsideBox)
    {
        return;
    }

    bCollected = true;

   
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);

    
    DisableInput(LocalController);

    UE_LOG(LogTemp, Log, TEXT("Card collected."));

    
}

void AKeyCard::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(LocalController))
    {
        DisableInput(LocalController);
    }

    Super::EndPlay(EndPlayReason);
}
