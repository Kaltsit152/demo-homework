// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "demoGameMode.generated.h"

class AEnemyCharacter;

UCLASS(minimalapi)
class AdemoGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AdemoGameMode();

    virtual void BeginPlay() override;

    // Enemy 死亡时调用：加分 + 判胜 + 补怪
    UFUNCTION(BlueprintCallable)
    void NotifyEnemyKilled(AEnemyCharacter* DeadEnemy);

protected:
    // ===== 胜利 =====
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Win")
    int32 WinScore = 5;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Win")
    int32 TeamKillCount = 0;

    void CheckWin();
    void HandleWin();

    // 胜利的UI
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<class UUserWidget> WinWidgetClass;

    // ===== 刷怪 =====
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
    int32 MaxAliveEnemies = 2;

    // 在 BP_demoGameMode 里设置（选 EnemyCharacter 或其蓝图）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
    TSubclassOf<AEnemyCharacter> EnemyClass;

    // 关卡里 Tag=EnemySpawn 的点
    UPROPERTY()
    TArray<AActor*> SpawnPoints;

    // 存活敌人（Destroy 后弱引用会自动失效）
    UPROPERTY()
    TArray<TWeakObjectPtr<AEnemyCharacter>> AliveEnemies;

    void CollectSpawnPoints();
    void CleanupDeadFromAliveList();
    void EnsureEnemies();
    void SpawnOneEnemyRandom();
};



