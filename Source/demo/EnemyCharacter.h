// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class USphereComponent;

UCLASS()
class DEMO_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyCharacter();
	// µÐÈËÊôÐÔ
	UPROPERTY(EditAnywhere, Category = "Enemy|Attack")
	float AttackRange = 150.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Attack")
	float AttackInterval = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Attack")
	float AttackDamage = 10.f;

	UPROPERTY(VisibleAnywhere, Category = "Enemy|Attack")
	USphereComponent* AttackSphere;

	FTimerHandle AttackTimerHandle;

	void TryStartAttack();
	void StopAttack();
	void DealDamageToPlayer();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// ×·×Ù²ÎÊý
	UPROPERTY(EditAnywhere, Category = "Enemy|AI")
	float MoveUpdateInterval = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Enemy|AI")
	float AcceptableRadius = 120.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|AI")
	float MaxChaseDistance = 20000.f; // ·ÀÖ¹×·Ì«Ô¶

	APawn* FindNearestPlayerPawn() const;

	FTimerHandle MoveTimerHandle;

	void UpdateMoveToPlayer();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Stats")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Stats")
	float Health = 100.f;

public:	
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	void ApplyDamageSimple(float Damage);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Stats")
	bool IsDead() const { return Health <= 0.f; }

};
