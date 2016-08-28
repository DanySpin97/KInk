// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Hangar/Projectile.h"
#include "PiercingProjectile.generated.h"

/**
 * 
 */
UCLASS()
class KINK_API APiercingProjectile : public AProjectile
{
	GENERATED_BODY()
public:

	UFUNCTION()
	virtual void OnBeginOverlap(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
	
	
};
