// Copyright Epic Games, Inc. All Rights Reserved.

#include "demoGameMode.h"
#include "demoCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "demoPlayerController.h"

AdemoGameMode::AdemoGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}

void AdemoGameMode::BeginPlay()
{
    Super::BeginPlay();
    // debug
    /*if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("[GM] BeginPlay"));
    }*/

    CollectSpawnPoints();

    //debug
    /*if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 5.f, FColor::Cyan,
            FString::Printf(TEXT("[GM] SpawnPoints=%d EnemyClass=%s"),
                SpawnPoints.Num(),
                EnemyClass ? *EnemyClass->GetName() : TEXT("NULL"))
        );
    }*/

    EnsureEnemies();
}

void AdemoGameMode::CollectSpawnPoints()
{
    SpawnPoints.Empty();

    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("EnemySpawn"), Found);
    SpawnPoints = Found;

    /*if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 2.0f, FColor::Cyan,
            FString::Printf(TEXT("[GM] SpawnPoints=%d"), SpawnPoints.Num())
        );
    }*/
}

void AdemoGameMode::NotifyEnemyKilled(AEnemyCharacter* DeadEnemy)
{
    TeamKillCount++;

    // 立刻从存活列表移除当前死亡敌人
    AliveEnemies.RemoveAll([DeadEnemy](const TWeakObjectPtr<AEnemyCharacter>& Ptr)
        {
            return !Ptr.IsValid() || Ptr.Get() == DeadEnemy;
        });

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 2.0f, FColor::Green,
            FString::Printf(TEXT("[GM] Kill %d/%d | Alive=%d"),
                TeamKillCount, WinScore, AliveEnemies.Num())
        );
    }

    CheckWin();

    if (TeamKillCount < WinScore)
    {
        EnsureEnemies();
    }
}

void AdemoGameMode::CleanupDeadFromAliveList()
{
    AliveEnemies.RemoveAll([](const TWeakObjectPtr<AEnemyCharacter>& Ptr)
        {
            return !Ptr.IsValid();
        });
}

void AdemoGameMode::EnsureEnemies()
{
    CleanupDeadFromAliveList();

    while (AliveEnemies.Num() < MaxAliveEnemies)
    {
        SpawnOneEnemyRandom();
        CleanupDeadFromAliveList();

        if (!EnemyClass || SpawnPoints.Num() == 0) break; // 防死循环
    }
}

void AdemoGameMode::SpawnOneEnemyRandom()
{
    if (!EnemyClass)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("[GM] EnemyClass not set (set in BP_demoGameMode)"));
        return;
    }
    if (SpawnPoints.Num() == 0)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("[GM] No SpawnPoints (Tag=EnemySpawn)"));
        return;
    }

    const int32 Idx = FMath::RandRange(0, SpawnPoints.Num() - 1);
    AActor* Point = SpawnPoints[Idx];

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AEnemyCharacter* Enemy = GetWorld()->SpawnActor<AEnemyCharacter>(EnemyClass, Point->GetActorTransform(), Params);

    if (Enemy)
    {
        AliveEnemies.Add(Enemy);

        //debug 画一个绿球：证明敌人真的刷出来了
        //DrawDebugSphere(GetWorld(), Enemy->GetActorLocation(), 80.f, 12, FColor::Green, false, 5.f);

        // 打印位置 + 是否有Controller
        /*if (GEngine)
        {
            const FString CtrlName = Enemy->GetController() ? Enemy->GetController()->GetName() : TEXT("NULL");
            GEngine->AddOnScreenDebugMessage(
                -1, 3.f, FColor::Cyan,
                FString::Printf(TEXT("[GM] Spawned Enemy at %s | Controller=%s"),
                    *Enemy->GetActorLocation().ToString(),
                    *CtrlName)
            );
        }*/

        // 立刻生成默认AI Controller
        Enemy->SpawnDefaultController();
    }
    else
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("[GM] SpawnActor FAILED"));
    }
}

void AdemoGameMode::CheckWin()
{
    if (TeamKillCount >= WinScore)
    {
        HandleWin();
    }
}

void AdemoGameMode::HandleWin()
{
    // 给所有玩家显示胜利UI
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        //if (APlayerController* PC = It->Get())
        //{
            //if (WinWidgetClass)
            //{
            //    UUserWidget* W = CreateWidget<UUserWidget>(PC, WinWidgetClass);
            //    if (W)
            //    {
            //        W->AddToViewport(999);
            //    }
            //}

            //// 禁用输入 + 显示鼠标，而不是立刻 Pause
            //PC->SetIgnoreMoveInput(true);
            //PC->SetIgnoreLookInput(true);
            //PC->bShowMouseCursor = true;

            //FInputModeUIOnly Mode;
            //PC->SetInputMode(Mode);
        //}

         /* 改为Client RPC调用*/
        if (AdemoPlayerController* PC = Cast<AdemoPlayerController>(It->Get()))
        {
            PC->Client_ShowWinUI();
        }
    }
    
    // 结束暂停游戏
    //UGameplayStatics::SetGamePaused(GetWorld(), true);
}
