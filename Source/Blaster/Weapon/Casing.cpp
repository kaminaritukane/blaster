// Fill out your copyright notice in the Description page of Project Settings.


#include "Casing.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);

	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);

	ShellEjectionImpules = 10.f;
	bToBeDestroyed = false;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();
	
	CasingMesh->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpules);
}

void ACasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bToBeDestroyed) return;
	bToBeDestroyed = true;

	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}

	GetWorld()->GetTimerManager().SetTimer(DestroyDelayTimeHandle, this, &ThisClass::OnDestroyTimer, 3.f, false);
}

void ACasing::OnDestroyTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(DestroyDelayTimeHandle);
	Destroy();
}

