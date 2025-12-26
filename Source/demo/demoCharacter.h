// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "demoCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;

UCLASS(config=Game)
class AdemoCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	
public:
	AdemoCharacter();

	// Player Status 
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Player|Stats")
	float MaxHealth = 100.f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Player|Stats")
	float Health = 100.f;

	UFUNCTION(BlueprintCallable, Category = "Player|Combat")
	void ApplyDamageToPlayer(float Damage);

	UFUNCTION(BlueprintCallable, Category = "Player|Stats")
	bool IsPlayerDead() const { return Health <= 0.f; }

	UFUNCTION(Server, Reliable)
	void Server_Fire(const FVector_NetQuantize& Start, const FVector_NetQuantizeNormal& Dir);

	// 修改为出生自带武器
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AActor> DefaultWeaponClass;

	// 服务器生成、客户端同步当前的“我的武器”
	UPROPERTY(ReplicatedUsing = OnRep_SpawnedWeapon)
	AActor* SpawnedWeapon;

	// 当 SpawnedWeapon 同步到客户端时调用装备武器
	UFUNCTION()
	void OnRep_SpawnedWeapon();

	// 服务器生成默认武器
	void SpawnDefaultWeapon();

	// 服务器同步客户端生命
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


protected:
	virtual void BeginPlay();

public:
		
	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	/** Bool for AnimBP to switch to another animation set */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	bool bHasRifle;

	/** Setter to set the bool */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	void SetHasRifle(bool bNewHasRifle);

	/** Getter for the bool */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	bool GetHasRifle();

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }


};

