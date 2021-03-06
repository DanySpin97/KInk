// Copyright (c) 2016 WiseDragonStd

#include "KInk.h"
#include "Pooling.h"
#include "Projectile.h"
#include "Kraken.h"
#include "SplineActor.h"
#include "BaseBotController.h"
#include "Utility/EffectsActor.h"
#include "World/MessageHandler.h"
#include "World/ArcadeGameMode.h"
#include "Kismet/KismetMathLibrary.h"
#include "PaperFlipbookComponent.h"
#include "World/ArcadeGameState.h"

/* AI Module includes */
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
/* This contains includes all key types like UBlackboardKeyType_Vector used below. */
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"

#include "Enemy.h"

#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5, FColor::White,text)

AEnemy::AEnemy(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("CapsuleComponent")))
{
	RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("RootComponent"));

	
	CapsuleHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight() + 2;

	AudioComponent = ObjectInitializer.CreateDefaultSubobject<UAudioComponent>(this, TEXT("AudioComponent"));
	AudioComponent->VolumeMultiplier = 0;

	bAlwaysFire = true;
	bInPool = false;
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();
	GameState = GetWorld()->GetGameState<AArcadeGameState>();
	PlayerShip = Cast<AKraken>(GetWorld()->GetFirstPlayerController()->GetPawn());
	FTimerHandle SetReferenceTimer;
	GetWorldTimerManager().SetTimer(SetReferenceTimer, this, &AEnemy::InitReference, 0.2);
}

void AEnemy::InitReference()
{
	auto GameMode = Cast<AArcadeGameMode>(GetWorld()->GetAuthGameMode());
	MessageHandler = GameMode->MessageHandler;
	PoolRef = GameMode->Pool;
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const FVector& CapsuleLocation = CapsuleComponent->GetComponentLocation();
	if (CapsuleLocation.Z != 0)
	{
		GetCapsuleComponent()->SetWorldLocation(FVector(CapsuleLocation.X, CapsuleLocation.Y, 0));
	}

	if (bAlwaysFire && !bInPool)
	{
		FireTime += DeltaTime;
	}

	if (bDamaged)
	{
		ColorValue = FMath::FInterpTo(ColorValue, 1, DeltaTime, 0.9);
		Sprite->SetSpriteColor(FLinearColor(1, ColorValue, ColorValue));
		if (ColorValue == 0)
		{
			bDamaged = false;
		}
	}
}

void AEnemy::Fire()
{
	FireTime -= FireRate;
}

bool AEnemy::CanFire()
{
	if (FireTime >= FireRate)
	{
		return true;
	}
	return false;
}

void AEnemy::Fire(const FVector& Location)
{
	if (!CanFire())
	{
		return;
	}

	for (int32 i = 0; i < NumberOfBullet; ++i)
	{
		FString ProjectileSocketName = FString(TEXT("Projectile"));
		ProjectileSocketName.AppendInt(i);
		const FVector& SocketLocation = GetSprite()->GetSocketLocation(FName(*ProjectileSocketName));
		const FRotator& Rotation = UKismetMathLibrary::FindLookAtRotation(SocketLocation, Location);
		const FVector& Direction = FRotationMatrix(FRotator(Rotation.Pitch, Rotation.Yaw - 60, Rotation.Roll)).GetUnitAxis(EAxis::X);
		PoolRef->PoolProjectile(GetWorld(), Projectile, this->GetClass(), SocketLocation, Rotation, Direction, (float)BulletDamage * GameState->Multiplier);
	}

	FireTime -= FireRate;
}

float AEnemy::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	if (DamageAmount > 0)
	{
		// Decrease health by damage dealt
		CurrentHealth -= DamageAmount;
		MessageData DestroyedData;
		// If the health is equal or less than 0
		if (CurrentHealth <= 0)
		{
			CurrentHealth = 0;
			if (DamageCauser)
			{
				// Did a projectile dealt the damage?
				if (DamageCauser->IsA(AProjectile::StaticClass()))
				{
					DestroyedData.CauseDestroyed = ECauseDestroyed::Projectile;
				}
				// Did the player dealt the damage by impacting?
				else if (DamageCauser->IsA(AKraken::StaticClass()))
				{
					DestroyedData.CauseDestroyed = ECauseDestroyed::ImpactWithShip;
				}
				else
				{
					DestroyedData.CauseDestroyed = ECauseDestroyed::Projectile;
				}
			}
			// Tell the message handler that this enemy has been destroyed and who destroyed it
			MessageHandler->ReceiveMessage(FMessage(DamageCauser, (AActor*)this, EMessageData::Destroyed, DestroyedData));
		}
		else if (CurrentHealth > MaxHealth)
		{
			CurrentHealth = MaxHealth;
		}
		ColorValue = 0;
		bDamaged = true;
	}
	return DamageAmount;
}

void AEnemy::OnMessage(struct FMessage Message)
{
	const FDamageEvent& DamageEvent = FDamageEvent();
	switch (Message.TypeData)
	{
	case EMessageData::Damage:
		UGameplayStatics::ApplyDamage(this, Message.DataValue.Damage, nullptr, Message.Sender, UDamageType::StaticClass());
		break;
	case EMessageData::Destroyed:
		switch (Message.DataValue.CauseDestroyed)
		{
		case ECauseDestroyed::Projectile:
		case ECauseDestroyed::ImpactWithShip:
			AudioComponent->SetSound(SoundDestroyed);
			AudioComponent->Play();
			UStaticLibrary::SpawnBP<AEffectsActor>(GetWorld(), DestroyedEffect, CapsuleComponent->GetComponentLocation(), FRotator());
			PoolRef->DeactivateEnemy(this);
			break;
		case ECauseDestroyed::ExitGame:
			// Save
			// Main Menu
			break;
		default:
			break;
		}
	default:
		break;
	}
}

void AEnemy::Pool()
{
	TActorIterator<AKraken> Ship(GetWorld());
	if (GetBotController())
	{
		GetBotController()->GetBlackboardComp()->SetValue<UBlackboardKeyType_Object>(GetBotController()->GetBlackboardComp()->GetKeyID(TEXT("PlayerActor")), *Ship);
	}
	auto GameState = GetWorld()->GetGameState<AArcadeGameState>();
	CurrentHealth = MaxHealth * GameState->Multiplier;
	FireTime = 0;
	// This variable determines the rate of the fire, increase the value the rate is divided by and the rate will have a smooth curve
	FireRate = 1.0f / ((float)GetRate() * GameState->Multiplier / 50.f);
	AudioComponent->VolumeMultiplier = 1;
	ColorValue = 1;
	bDamaged = false;
	Sprite->SetSpriteColor(FLinearColor(1, ColorValue, ColorValue));
	bInPool = false;
}

void AEnemy::Deactivate()
{
	bInPool = true;
}