// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	UWorld* World = GetWorld();
	if (MuzzleFlashSocket && World)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();

		FActorSpawnParameters SpawnParames;
		SpawnParames.Owner = GetOwner();
		SpawnParames.Instigator = InstigatorPawn;

		AProjectile* SpawnedProjectile = nullptr;
		if (bUseServerSideRewind)
		{
			if (InstigatorPawn->HasAuthority()) // server
			{
				if (InstigatorPawn->IsLocallyControlled()) // server, host - use replicated projectile
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParames);
					SpawnedProjectile->bUseServerSideRewind = false;
					SpawnedProjectile->Damage = Damage;
					SpawnedProjectile->HeadShotDamage = HeadShotDamage;
				}
				else // server, not locally controlled - spawn non-replicated projectile, use SSR
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParames);
					SpawnedProjectile->bUseServerSideRewind = true;
				}
			}
			else // client, using SSR
			{
				if (InstigatorPawn->IsLocallyControlled()) // client, locally controlled - spawn non-replicated projectile, use SSR
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParames);
					SpawnedProjectile->bUseServerSideRewind = true;
					SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
				}
				else // client, not locally controlled - spawn non-replicated projectile, no SSR
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParames);
					SpawnedProjectile->bUseServerSideRewind = false;
				}
			}
		}
		else // weapon not use SSR
		{
			if (InstigatorPawn->HasAuthority()) // server
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParames);
				SpawnedProjectile->bUseServerSideRewind = false;
				SpawnedProjectile->Damage = Damage;
				SpawnedProjectile->HeadShotDamage = HeadShotDamage;
			}
		}
	}
}
