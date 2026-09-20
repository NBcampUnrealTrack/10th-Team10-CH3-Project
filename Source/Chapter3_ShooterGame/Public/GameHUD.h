
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GameHUD.generated.h"

class UUserWidget;

UCLASS()
class CHAPTER3_SHOOTERGAME_API AGameHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> InGameHUDClass;
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> InGameHUDWidget;
};
