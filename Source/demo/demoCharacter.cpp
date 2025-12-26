// Copyright Epic Games, Inc. All Rights Reserved.

#include "demoCharacter.h"
#include "demoProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "TP_PickUpComponent.h"
#include "GameFramework/PlayerStart.h"
#include "EnemyCharacter.h"
#include "TP_WeaponComponent.h"
#include "Net/UnrealNetwork.h"

//////////////////////////////////////////////////////////////////////////
// AdemoCharacter

AdemoCharacter::AdemoCharacter()
{
	// Character doesnt have a rifle at start
	bHasRifle = false;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	//Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	bReplicates = true;

}

void AdemoCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	// 初始化清空状态
	SpawnedWeapon = nullptr;
	Health = MaxHealth;
	bHasRifle = false;

	// 本地玩家：输入映射 
	if (IsLocallyControlled())
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
	

	// 服务器生成武器
	if (HasAuthority() && DefaultWeaponClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedWeapon = GetWorld()->SpawnActor<AActor>(
			DefaultWeaponClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			Params
		);

		// v1 服务器绑定武器
		/*if (SpawnedWeapon)
		{
			if (UTP_WeaponComponent* WC = SpawnedWeapon->FindComponentByClass<UTP_WeaponComponent>())
			{
				WC->AttachWeapon(this);
				bHasRifle = true;
			}
		}*/

		// v2 手动 服务器-》OnRep 进行武器绑定
		//OnRep_SpawnedWeapon();

		// v3 统一入口：生成默认武器
		SpawnDefaultWeapon();
	}

	// debug打印
	/*if (GEngine && IsLocallyControlled())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("BeginPlay OK"));
	}*/
}

// OnRep绑定武器
void AdemoCharacter::OnRep_SpawnedWeapon()
{
	if (!SpawnedWeapon) return;

	// 只在本地或Listen Server的玩家身上装备
	if (!IsLocallyControlled() && !HasAuthority())
	{
		return;
	}

	if (UTP_WeaponComponent* WeaponComp =
		SpawnedWeapon->FindComponentByClass<UTP_WeaponComponent>())
	{
		WeaponComp->AttachWeapon(this);   // 绑定输入 + 挂到 Mesh1P
		bHasRifle = true;
	}
}

void AdemoCharacter::SpawnDefaultWeapon()
{
	if (!HasAuthority() || !DefaultWeaponClass)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnedWeapon = GetWorld()->SpawnActor<AActor>(
		DefaultWeaponClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		Params
	);

	// Listen Server 不会自动触发 OnRep，延迟一帧补上
	FTimerHandle TempHandle;
	GetWorld()->GetTimerManager().SetTimerForNextTick(
		this,
		&AdemoCharacter::OnRep_SpawnedWeapon
	);
}



// 注册Replication
void AdemoCharacter::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AdemoCharacter, SpawnedWeapon);
	DOREPLIFETIME(AdemoCharacter, Health);
	DOREPLIFETIME(AdemoCharacter, MaxHealth);
}

// 玩家的死亡判定及处理
void AdemoCharacter::ApplyDamageToPlayer(float Damage)
{
	if (IsPlayerDead()) return;

	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);

	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			2, 1.0f, FColor::Red,
			FString::Printf(TEXT("[Player] HP = %.1f"), Health)
		);
	}*/

	if (IsPlayerDead())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				3, 2.0f, FColor::Red,
				TEXT("[Player] Dead! Respawning...")
			);
		}

		// 缓存 Controller
		AController* SavedController = GetController();
		if (!SavedController)
		{
			UE_LOG(LogTemp, Error, TEXT("Death: Controller is null"));
			return;
		}

		UWorld* World = GetWorld();
		if (!World)
		{
			UE_LOG(LogTemp, Error, TEXT("Death: World is null"));
			return;
		}

		/* 注释掉老的死亡枪重新生成再去捡的逻辑，变为出生自带枪*/
		// // 恢复拾取球
		//TArray<AActor*> AttachedActors;
		//GetAttachedActors(AttachedActors);
		
		//for (AActor* A : AttachedActors)
		//{
		//	if (!A) continue;

		//	// 只处理tag为枪的
		//	if (!A->ActorHasTag(FName("Rifle"))) continue;

		//	// 1) 从玩家身上卸下掉在地上/或移动到出生点
		//	A->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		//	// 2) 恢复拾取球（重新允许 Overlap）
		//	if (UTP_PickUpComponent* Pick = A->FindComponentByClass<UTP_PickUpComponent>())
		//	{
		//		Pick->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		//		Pick->SetGenerateOverlapEvents(true);
		//	}

		//	// 3) 把枪送回出生点，避免掉在死亡点找不到
		//	if (AActor* PS = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()))
		//	{
		//		A->SetActorLocation(PS->GetActorLocation() + FVector(80, 0, 0));
		//		A->SetActorRotation(PS->GetActorRotation());
		//	}

		//	break; // 只处理一把枪
		//}

		// 服务器清理武器
		if (HasAuthority() && SpawnedWeapon)
		{
			SpawnedWeapon->Destroy();
			SpawnedWeapon = nullptr;
		}

		// 先脱离并销毁当前 Pawn
		DetachFromControllerPendingDestroy();
		SetLifeSpan(0.1f);

		// 延迟2s重生
		FTimerHandle RespawnTimer;
		World->GetTimerManager().SetTimer(
			RespawnTimer,
			[World, SavedController]()
			{
				if (!World || !SavedController)
				{
					UE_LOG(LogTemp, Error, TEXT("Respawn: World or Controller invalid"));
					return;
				}

				AGameModeBase* GM = UGameplayStatics::GetGameMode(World);
				if (!GM)
				{
					UE_LOG(LogTemp, Error, TEXT("Respawn failed: GameMode is null"));
					return;
				}

				AActor* PS = GM->FindPlayerStart(SavedController);
				if (!PS)
				{
					UE_LOG(LogTemp, Error, TEXT("Respawn failed: No valid PlayerStart found"));
					return;
				}

				UE_LOG(LogTemp, Warning, TEXT("RestartPlayer executed"));
				GM->RestartPlayer(SavedController);
			},
			2.0f,
			false
		);
	}
}

void AdemoCharacter::Server_Fire_Implementation(
	const FVector_NetQuantize& Start,
	const FVector_NetQuantizeNormal& Dir)
{
	UE_LOG(LogTemp, Warning, TEXT("[Server] Server_Fire received from %s"),
		*GetName());

	UWorld* World = GetWorld();
	if (!World) return;

	FVector End = FVector(Start) + FVector(Dir) * 5000.f;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = World->LineTraceSingleByChannel(
		Hit, FVector(Start), End, ECC_Visibility, Params);

	//DrawDebugLine(World, FVector(Start), End, bHit ? FColor::Green : FColor::Blue, false, 1.0f, 0, 1.5f);

	if (bHit)
	{
		if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Hit.GetActor()))
		{
			Enemy->ApplyDamageSimple(34.f);
		}
	}
}

//////////////////////////////////////////////////////////////////////////// Input

void AdemoCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AdemoCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AdemoCharacter::Look);
	}
}


void AdemoCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AdemoCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AdemoCharacter::SetHasRifle(bool bNewHasRifle)
{
	bHasRifle = bNewHasRifle;
}

bool AdemoCharacter::GetHasRifle()
{
	return bHasRifle;
}