// Copyright Epic Games, Inc. All Rights Reserved.


#include "TP_WeaponComponent.h"
#include "demoCharacter.h"
#include "demoProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnemyCharacter.h"
#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
}


void UTP_WeaponComponent::Fire()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	// Try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
			if (!PlayerController || !PlayerController->PlayerCameraManager) return;

			const FVector Start = PlayerController->PlayerCameraManager->GetCameraLocation();
			const FVector Dir = PlayerController->PlayerCameraManager->GetCameraRotation().Vector();
			// 命中与伤害交给服务器计算
			if (AdemoCharacter* C = Character)
			{
				C->Server_Fire(Start, Dir);
			}

			UE_LOG(LogTemp, Warning, TEXT("Fire() called. Local=%d, Owner=%s"),
				Character->IsLocallyControlled(),
				GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"));


			//const FVector End = Start + Dir * 5000.f;

			//FHitResult Hit;
			//FCollisionQueryParams Params(SCENE_QUERY_STAT(TP_Weapon_FireTrace), true);
			//Params.AddIgnoredActor(Character);         // 忽略自己
			//Params.AddIgnoredActor(GetOwner());        // 忽略武器本体

			//const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

			//// 调试画线：绿色=命中，蓝色=未命中
			//DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Blue, false, 1.0f, 0, 1.5f);

			//if (bHit && Hit.GetActor())
			//{
			//	if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Hit.GetActor()))
			//	{
			//		Enemy->ApplyDamageSimple(34.f); // 三枪死
			//	}
			//}
		}
	}
	
	// Try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
	}
	
	// Try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}


void UTP_WeaponComponent::AttachWeapon(AdemoCharacter* TargetCharacter)
{
	Character = TargetCharacter;
	if (Character == nullptr)
	{
		return;
	}

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh1P(), AttachmentRules, FName(TEXT("GripPoint")));

	if (AActor* WeaponActor = GetOwner())
	{
		WeaponActor->SetOwner(TargetCharacter); // 把武器的Owner设为拾取者
	}

	
	// switch bHasRifle so the animation blueprint can switch to another animation set
	Character->SetHasRifle(true);

	// Set up action bindings
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (!Character->IsLocallyControlled())
		{
			// 服务器和远端客户端不做输入绑定
			return;
		}
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			// Fire
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::Fire);
		}
	}
}

void UTP_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Character == nullptr)
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(FireMappingContext);
		}
	}
}