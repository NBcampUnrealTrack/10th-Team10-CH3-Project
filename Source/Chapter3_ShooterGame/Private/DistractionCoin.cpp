#include "DistractionCoin.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"

namespace
{
    constexpr float kDefaultCoinDiameter = 4.0f;
    constexpr float kDefaultCollisionRadius = 2.0f;
    constexpr float kDefaultNoiseRange = 1500.0f;
    constexpr float kDefaultFlightLifeSpan = 15.0f;
    constexpr float kDefaultLandedLifeSpan = 8.0f;
    constexpr float kNoiseLoudness = 1.0f;
    constexpr float kDefaultGravityScale = 1.0f;
}

const FName ADistractionCoin::kNoiseTag(TEXT("CoinDistraction"));

ADistractionCoin::ADistractionCoin()
{
    PrimaryActorTick.bCanEverTick = false;
    coinDiameter_ = kDefaultCoinDiameter;
    noiseRange_ = kDefaultNoiseRange;
    flightLifeSpan_ = kDefaultFlightLifeSpan;
    landedLifeSpan_ = kDefaultLandedLifeSpan;

    collision_ = CreateDefaultSubobject<USphereComponent>(TEXT("CoinCollision"));
    SetRootComponent(collision_);
    collision_->InitSphereRadius(kDefaultCollisionRadius);
    collision_->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    collision_->SetCollisionObjectType(ECC_WorldDynamic);
    collision_->SetCollisionResponseToAllChannels(ECR_Block);
    collision_->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    collision_->SetGenerateOverlapEvents(false);
    collision_->SetCanEverAffectNavigation(false);

    coinMesh_ = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoinMesh"));
    coinMesh_->SetupAttachment(collision_);
    coinMesh_->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    coinMesh_->SetGenerateOverlapEvents(false);
    coinMesh_->SetCanEverAffectNavigation(false);

    projectileMovement_ = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("CoinMovement"));
    projectileMovement_->SetUpdatedComponent(collision_);
    projectileMovement_->bAutoActivate = false;
    projectileMovement_->bShouldBounce = false;
    projectileMovement_->bRotationFollowsVelocity = true;
    projectileMovement_->bForceSubStepping = true;
    projectileMovement_->ProjectileGravityScale = kDefaultGravityScale;
    projectileMovement_->OnProjectileStop.AddDynamic(this, &ADistractionCoin::HandleProjectileStop);
}

void ADistractionCoin::OnConstruction(const FTransform& transform)
{
    Super::OnConstruction(transform);
    FitCoinMesh();
}

void ADistractionCoin::BeginPlay()
{
    Super::BeginPlay();
    FitCoinMesh();
    IgnoreThrower();
    SetLifeSpan(FMath::Max(UE_SMALL_NUMBER, flightLifeSpan_));
}

void ADistractionCoin::FitCoinMesh()
{
    coinMesh_->SetStaticMesh(coinMeshAsset_);
    const UStaticMesh* mesh = coinMeshAsset_;
    if (!mesh)
    {
        coinMesh_->SetRelativeTransform(FTransform::Identity);
        collision_->SetSphereRadius(kDefaultCollisionRadius);
        return;
    }

    // 원본 메시의 단위와 피벗에 관계없이 표시를 충돌 구 중심에 맞춘다.
    const FBoxSphereBounds bounds = mesh->GetBounds();
    const float sourceDiameter = bounds.BoxExtent.GetMax() * 2.0f;
    if (sourceDiameter <= UE_SMALL_NUMBER)
    {
        return;
    }

    const float meshScale = FMath::Max(UE_SMALL_NUMBER, coinDiameter_) / sourceDiameter;
    coinMesh_->SetRelativeScale3D(FVector(meshScale));
    coinMesh_->SetRelativeLocation(-bounds.Origin * meshScale);
    collision_->SetSphereRadius(FMath::Max(kDefaultCollisionRadius, bounds.SphereRadius * meshScale));
}

void ADistractionCoin::IgnoreThrower()
{
    APawn* thrower = GetInstigator();
    collision_->IgnoreActorWhenMoving(GetOwner(), true);
    if (!thrower)
    {
        return;
    }

    collision_->IgnoreActorWhenMoving(thrower, true);
    TArray<AActor*> attachedActors = {};
    thrower->GetAttachedActors(attachedActors, true, true);
    for (AActor* attachedActor : attachedActors)
    {
        collision_->IgnoreActorWhenMoving(attachedActor, true);
    }
}

float ADistractionCoin::GetCollisionRadius() const
{
    const UStaticMesh* mesh = coinMeshAsset_;
    if (mesh)
    {
        const FBoxSphereBounds bounds = mesh->GetBounds();
        const float sourceDiameter = bounds.BoxExtent.GetMax() * 2.0f;
        if (sourceDiameter > UE_SMALL_NUMBER)
        {
            return FMath::Max(kDefaultCollisionRadius, bounds.SphereRadius * coinDiameter_ / sourceDiameter);
        }
    }
    return collision_->GetScaledSphereRadius();
}

void ADistractionCoin::LaunchCoin(const FVector& launchVelocity)
{
    if (hasLaunched_ || hasLanded_ || launchVelocity.IsNearlyZero() || launchVelocity.ContainsNaN())
    {
        return;
    }

    hasLaunched_ = true;
    IgnoreThrower();
    projectileMovement_->Velocity = launchVelocity;
    projectileMovement_->Activate(true);
}

void ADistractionCoin::HandleProjectileStop(const FHitResult& hitResult)
{
    if (hasLanded_ || !hitResult.bBlockingHit)
    {
        return;
    }

    hasLanded_ = true;
    collision_->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    const FVector landingLocation = hitResult.ImpactPoint;
    const float effectiveNoiseRange = FMath::Max(0.0f, noiseRange_);

    // 자극의 위치는 착지점, 소스는 코인이다. 투척자 위치로 AI가 향하지 않게 한다.
    // MaxRange=0은 엔진에서 무제한이므로, 0으로 설정했을 때는 보고 자체를 생략한다.
    if (HasAuthority() && effectiveNoiseRange > 0.0f)
    {
        UAISense_Hearing::ReportNoiseEvent(this, landingLocation, kNoiseLoudness,
            this, effectiveNoiseRange, kNoiseTag);
    }

    if (landingSound_)
    {
        UGameplayStatics::PlaySoundAtLocation(this, landingSound_, landingLocation);
    }

    SetLifeSpan(FMath::Max(UE_SMALL_NUMBER, landedLifeSpan_));
    onCoinLanded_.Broadcast(landingLocation, GetInstigator(), effectiveNoiseRange);
}
