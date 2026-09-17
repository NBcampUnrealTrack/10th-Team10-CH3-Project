#include "GameHUD.h"
#include "Blueprint/UserWidget.h"

void AGameHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!InGameHUDClass)
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayerController();

	if (!PlayerController)
	{
		return;
	}

	InGameHUDWidget = CreateWidget<UUserWidget>(
		PlayerController,
		InGameHUDClass
	);

	if (InGameHUDWidget)
	{
		InGameHUDWidget->AddToViewport();
	}
}

