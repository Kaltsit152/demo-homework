// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "demoCharacter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "demoGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
    
    // 初始化的敌人要配置AIController
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = 280.f;
    }

    AttackSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AttackSphere"));
    AttackSphere->SetupAttachment(GetRootComponent());
    AttackSphere->SetSphereRadius(AttackRange); // 直接复用 AttackRange
    AttackSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    AttackSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    AttackSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    // 防止多个敌人在一起互相卡死
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;

    // 定时更新追踪
    GetWorldTimerManager().SetTimer(
        MoveTimerHandle,
        this,
        &AEnemyCharacter::UpdateMoveToPlayer,
        MoveUpdateInterval,
        true
    );
	
}

// 计算最近的玩家
APawn* AEnemyCharacter::FindNearestPlayerPawn() const
{
    APawn* BestPawn = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (!PC) continue;

        APawn* P = PC->GetPawn();
        if (!P) continue;

        if (AdemoCharacter* DC = Cast<AdemoCharacter>(P)) if (DC->IsPlayerDead()) continue;

        float DistSq = FVector::DistSquared(P->GetActorLocation(), GetActorLocation());
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestPawn = P;
        }
    }
    return BestPawn;
}

// 自动追踪玩家逻辑
void AEnemyCharacter::UpdateMoveToPlayer()
{
    if (IsDead()) return;

    //APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    APawn* PlayerPawn = FindNearestPlayerPawn();
    if (!PlayerPawn) return;

    // 太远就不追
    const float Dist = FVector::Dist(PlayerPawn->GetActorLocation(), GetActorLocation());
    if (Dist > MaxChaseDistance) return;

    AAIController* AICon = Cast<AAIController>(GetController());
    if (!AICon) return;

    // MoveToActor 会使用 NavMesh 寻路
    AICon->MoveToActor(PlayerPawn, AcceptableRadius);

    TryStartAttack();
}

void AEnemyCharacter::TryStartAttack()
{
    if (IsDead()) return;


    TArray<AActor*> OverlappingActors;
    AttackSphere->GetOverlappingActors(OverlappingActors, AdemoCharacter::StaticClass());

    if (OverlappingActors.Num() > 0)
    {
        if (!GetWorldTimerManager().IsTimerActive(AttackTimerHandle))
        {
            GetWorldTimerManager().SetTimer(
                AttackTimerHandle,
                this,
                &AEnemyCharacter::DealDamageToPlayer,
                AttackInterval,
                true
            );
        }
    }
    else
    {
        StopAttack();
    }
}

void AEnemyCharacter::StopAttack()
{
    if (GetWorldTimerManager().IsTimerActive(AttackTimerHandle))
    {
        GetWorldTimerManager().ClearTimer(AttackTimerHandle);
    }
}

void AEnemyCharacter::DealDamageToPlayer()
{
    if (IsDead()) { StopAttack(); return; }

    //APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    APawn* PlayerPawn = FindNearestPlayerPawn();
    if (!PlayerPawn) { StopAttack(); return; }

    // 距离检查：防止玩家跑远了 timer 还在打
    const float Dist = FVector::Dist(PlayerPawn->GetActorLocation(), GetActorLocation());
    if (Dist > AttackRange)
    {
        StopAttack();
        return;
    }

    if (AdemoCharacter* Player = Cast<AdemoCharacter>(PlayerPawn))
    {
        Player->ApplyDamageToPlayer(AttackDamage);

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(4, 0.5f, FColor::Orange, TEXT("[Enemy] Attack!"));
        }
    }
}


void AEnemyCharacter::ApplyDamageSimple(float Damage)
{
    if (IsDead()) return;

    Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);

    // 屏幕打印调试
    if (GEngine)
    {
        const FString Msg = FString::Printf(TEXT("[Enemy] Hit! Health=%.1f"), Health);
        GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, Msg);
    }

    if (IsDead())
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("[Enemy] Dead -> Destroy"));
        }

        // 清理所有行为
        StopAttack();                                   // 停止攻击定时器
        GetWorldTimerManager().ClearTimer(MoveTimerHandle); // 停止追踪 MoveTo

        // 通知 GameMode：团队+1分，并触发补怪
        if (AdemoGameMode* GM = Cast<AdemoGameMode>(UGameplayStatics::GetGameMode(this)))
        {
            GM->NotifyEnemyKilled(this);
        }
        Destroy();
    }
}

