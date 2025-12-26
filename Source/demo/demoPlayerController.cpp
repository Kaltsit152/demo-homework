// Fill out your copyright notice in the Description page of Project Settings.


#include "demoPlayerController.h"
#include "Blueprint/UserWidget.h"

void AdemoPlayerController::Client_ShowWinUI_Implementation()
{
    if (!IsLocalController()) return;

    if (WinWidgetClass)
    {
        UUserWidget* W = CreateWidget<UUserWidget>(this, WinWidgetClass);
        if (W)
        {
            W->AddToViewport(999);
        }
    }

    if (CrosshairWidget)
    {
        CrosshairWidget->RemoveFromParent();
    }


    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    bShowMouseCursor = true;

    FInputModeUIOnly Mode;
    SetInputMode(Mode);
}

void AdemoPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController()) return;

    // 1) Crosshair
    if (CrosshairWidgetClass)
    {
        CrosshairWidget = CreateWidget<UUserWidget>(this, CrosshairWidgetClass);
        if (CrosshairWidget)
        {
            CrosshairWidget->AddToViewport(10); // 层级低于胜利UI
        }
    }

    // 2) HUD
    if (HUDWidgetClass)
    {
        HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
        if (HUDWidget)
        {
            HUDWidget->AddToViewport(0); // 比准星更底层
        }
    }
}
