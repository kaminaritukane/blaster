// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	UE_LOG(LogTemp, Warning, TEXT("AShotgun::FireShotgun HasAuth:%d"), GetOwner()->HasAuthority());

	AWeapon::Fire(FVector());

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		// Maps hit character to number of times hit
		TMap<ABlasterCharacter*, uint32> HitMap;
		TMap<ABlasterCharacter*, uint32> HeadShotHitMap;
		for (const FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
			if (BlasterCharacter)
			{
				const bool bHeadShot = FireHit.BoneName.ToString() == FString("head");
				if (bHeadShot)
				{
					if (HeadShotHitMap.Contains(BlasterCharacter))
					{
						HeadShotHitMap[BlasterCharacter]++;
					}
					else
					{
						HeadShotHitMap.Emplace(BlasterCharacter, 1);
					}
				}
				else
				{
					if (HitMap.Contains(BlasterCharacter))
					{
						HitMap[BlasterCharacter]++;
					}
					else
					{
						HitMap.Emplace(BlasterCharacter, 1);
					}
				}

				if (ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						GetWorld(),
						ImpactParticles,
						FireHit.ImpactPoint,
						FireHit.ImpactNormal.Rotation()
					);
				}

				if (HitSound)
				{
					UGameplayStatics::PlaySoundAtLocation(
						this,
						HitSound,
						FireHit.ImpactPoint,
						0.5f,
						FMath::FRandRange(-0.5f, 0.5f)
					);
				}
			}
		}

		AController* InstigatorController = OwnerPawn->GetController();
		if (InstigatorController)
		{
			// Maps Character hit to total damage
			TArray<ABlasterCharacter*> HitCharacters;
			TMap<ABlasterCharacter*, float> DamageMap;

			// Calculate body damage by multiplying hit x Damage
			for (auto HitPair : HitMap)
			{
				if (HitPair.Key)
				{
					DamageMap.Emplace(HitPair.Key, HitPair.Value * Damage);

					HitCharacters.AddUnique(HitPair.Key);
				}
			}

			// Calculate head shot damage by multiplying hit x head shot Damage
			for (auto HitPair : HeadShotHitMap)
			{
				if (HitPair.Key)
				{
					if (DamageMap.Contains(HitPair.Key))
					{
						DamageMap[HitPair.Key] += HitPair.Value * HeadShotDamage; 
					}
					else
					{
						DamageMap.Emplace(HitPair.Key, HitPair.Value * HeadShotDamage);
					}

					HitCharacters.AddUnique(HitPair.Key);
				}
			}

			// Loop through DamageMap to get total damage for each character
			bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
			for (auto DamagePair : DamageMap)
			{
				if (DamagePair.Key && HasAuthority() && bCauseAuthDamage)
				{
					UGameplayStatics::ApplyDamage(
						DamagePair.Key,
						DamagePair.Value,
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
			}
			
			if (!HasAuthority() && bUseServerSideRewind)
			{
				BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(OwnerPawn) : BlasterOwnerCharacter;
				BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(InstigatorController) : BlasterOwnerController;
				if (BlasterOwnerController
					&& BlasterOwnerCharacter
					&& BlasterOwnerCharacter->GetLagCompensation()
					&& BlasterOwnerCharacter->IsLocallyControlled())
				{
					BlasterOwnerCharacter->GetLagCompensation()->ShotgunServerScoreRequest(
						HitCharacters,
						Start,
						HitTargets,
						BlasterOwnerController->GetServerTime() - BlasterOwnerController->SingleTripTime
					);
				}
			}
		}
	}
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket == nullptr) return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	for (uint32 i = 0; i < NumberOfPellets; ++i)
	{
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec;
		FVector ToEndLoc = EndLoc - TraceStart;
		ToEndLoc = FVector(TraceStart + ToEndLoc.GetSafeNormal() * TRACE_LENGTH);

		HitTargets.Add(ToEndLoc);
	}
}
