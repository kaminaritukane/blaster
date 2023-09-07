// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class ABlasterCharacter;

public:	
	UBuffComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void Heal(float HealAmount, float HealingTime);
	void RepenishShield(float ShieldAmount, float RelenishTime);
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime);
	void BuffJump(float BuffJumpVelocity, float BuffTime);
	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);
	void SetInitialJumpVelocity(float Velocity);

protected:
	virtual void BeginPlay() override;
	void HealRampUp(float DeltaTime);
	void ShieldRampUp(float DeltaTime);

private:
	UPROPERTY()
	class ABlasterCharacter* Character;

	/**
	* Heal Buff
	*/
	bool bHealing = false;
	float HealingRate = 0.f;
	float AmountToHeal = 0.f;

	/**
	* Shield Buff
	*/
	bool bReplenishingShield = false;
	float ShieldReplenishRate = 0.f;
	float ShieldReplenishAmount = 0.f;

	/**
	* Spee Buff
	*/
	FTimerHandle SpeedBuffTimer;
	void ResetSpeed();
	float InitialBaseSpeed;
	float InitialCrouchSpeed;

	UFUNCTION(NetMulticast, Reliable)
	void MulticaseSpeedBuff(float BaseSpeed, float CrouchSpeed);

	/**
	* Jump Buff
	*/
	FTimerHandle JumpBuffTimer;
	void ResetJump();
	float InitialJumpVelocity;

	UFUNCTION(NetMulticast, Reliable)
	void MulticaseJumpBuff(float JumpVelocity);


public:	
	
};
