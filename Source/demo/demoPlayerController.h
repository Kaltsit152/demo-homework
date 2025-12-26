// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "demoPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class DEMO_API AdemoPlayerController : public APlayerController
{
	GENERATED_BODY()
public:

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> WinWidgetClass;

	UFUNCTION(Client, Reliable)
	void Client_ShowWinUI();

	// ×¼ÐÇ
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> CrosshairWidgetClass;

	UPROPERTY()
	UUserWidget* CrosshairWidget;

	// ÑªÁ¿Ìõ
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY()
	UUserWidget* HUDWidget;

protected:
	virtual void BeginPlay() override;

	
};
