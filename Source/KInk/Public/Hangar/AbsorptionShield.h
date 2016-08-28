// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Hangar/PowerUp.h"
#include "AbsorptionShield.generated.h"

/**
 * 
 */
UCLASS()
class KINK_API AAbsorptionShield : public APowerUp
{
	GENERATED_BODY()
	
public:
	AAbsorptionShield();

	UPROPERTY(EditDefaultsOnly, Category = Attributes)
		int32 AbsorptionShieldValue;

	virtual void ActivatePowerUp() override;

	virtual void DeactivatePowerUp() override;

	
	
};
