// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Chapter3_ShooterGame_Character.generated.h"

UCLASS()
class CHAPTER3_SHOOTERGAME_API AChapter3_ShooterGame_Character : public ACharacter
{
	GENERATED_BODY()

public:
	AChapter3_ShooterGame_Character();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* Mesh1P;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	FORCEINLINE class UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
	FORCEINLINE class USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }

	// PlayerController가 IA_Move 값을 받아 여기로 넘겨줌 (X = 좌우, Y = 전후)
	void Move(FVector2D MovementVector);

	// PlayerController가 IA_Look 값을 받아 여기로 넘겨줌 (X = Yaw, Y = Pitch)
	void Look(FVector2D LookAxisVector);

	// bSprint = true면 달리기 시작, false면 멈춤
	void SetSprinting(bool bSprint);

	// bCrouch = true면 앉기, false면 일어서기
	void SetCrouching(bool bCrouch);

	// 파쿠르 시도 (스페이스바)
	void TryParkour();

	// 상호작용 시도 (F)
	void Interact();

	// 격발 시작/종료 (좌클릭)
	void StartFire();
	void StopFire();

	// bAim = true면 정조준 시작, false면 정조준 해제 (우클릭)
	void SetAiming(bool bAim);

	// 스킬 사용 (1, 2)
	void UseSkill1();
	void UseSkill2();

	// 기울이기(peek) 값 설정. -1(왼쪽, Q) ~ 1(오른쪽, E), 0이면 원위치
	void SetLean(float LeanValue);

protected:
	void UpdateMovementSpeed();
	void TryVaultOrClimb();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float CrouchSpeed = 200.f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour")
	float ParkourTraceDistance = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour")
	float VaultCheckHeight = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	float InteractTraceDistance = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float FireTraceDistance = 5000.f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsAiming = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lean")
	float LeanAngle = 15.f;

	UPROPERTY(BlueprintReadOnly, Category = "Lean")
	float CurrentLeanValue = 0.f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnFire(const FHitResult& HitResult);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnAimStart();

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnAimEnd();

	UFUNCTION(BlueprintImplementableEvent, Category = "Skill")
	void OnSkill1();

	UFUNCTION(BlueprintImplementableEvent, Category = "Skill")
	void OnSkill2();

	UFUNCTION(BlueprintImplementableEvent, Category = "Interact")
	void OnInteract(AActor* InteractedActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Parkour")
	void OnVaultStart();

	UFUNCTION(BlueprintImplementableEvent, Category = "Parkour")
	void OnCrawlStart();
};
