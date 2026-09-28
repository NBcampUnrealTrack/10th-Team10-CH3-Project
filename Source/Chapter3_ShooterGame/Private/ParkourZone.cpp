#include "ParkourZone.h"

#include "Components/BoxComponent.h"
#include "Chapter3_ShooterGame_Character.h"

AParkourZone::AParkourZone()
{
    PrimaryActorTick.bCanEverTick = false;

    ParkourBox = CreateDefaultSubobject<UBoxComponent>(
        TEXT("ParkourBox")
    );

    SetRootComponent(ParkourBox);

    ParkourBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));

    // 이동을 막지 않고 오버랩만 검사
    ParkourBox->SetCollisionEnabled(
        ECollisionEnabled::QueryOnly
    );

    ParkourBox->SetCollisionObjectType(ECC_WorldDynamic);

    ParkourBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    ParkourBox->SetCollisionResponseToChannel(
        ECC_Pawn,
        ECR_Overlap
    );

    ParkourBox->SetGenerateOverlapEvents(true);
}

void AParkourZone::ActivateParkour(AActor* PlayerActor)
{
    AChapter3_ShooterGame_Character* Player =
        Cast<AChapter3_ShooterGame_Character>(PlayerActor);

    if (!IsValid(Player))
    {
        return;
    }

    // 실제로 이 영역 안에 있는 플레이어인지 재확인
    if (!ParkourBox->IsOverlappingActor(Player))
    {
        return;
    }

    // 이미 구현된 파쿠르 호출
    Player->TryParkour();
}
