// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Hangar/PowerUp.h"
#include "DoubleDamage.generated.h"

/**
 * 
 */
UCLASS()
class KINK_API ADoubleDamage : public APowerUp
{
	GENERATED_BODY()

public:
	ADoubleDamage();

	UPROPERTY(EditDefaultsOnly, Category = Attributes)
	int32 Multiplier;

	virtual void ActivatePowerUp() override;

	virtual void DeactivatePowerUp() override;

	
};
