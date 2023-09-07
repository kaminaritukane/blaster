#pragma once

#define TRACE_LENGTH 80000.f

#define CUSTOM_DEPTH_PURPLE 250
#define CUSTOM_DEPTH_BLUE 251
#define CUSTOM_DEPTH_TAN 252

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(Displayname = "Assault Rifle"),
	EWT_RocketLauncher UMETA(Displayname = "Rocket Launcher"),
	EWT_Pistol UMETA(Displayname = "Pistol"),
	EWT_SubmachineGun UMETA(Displayname = "Submachine Gun"),
	EWT_ShotGun UMETA(Displayname = "Shotgun"),
	EWT_SniperRifle UMETA(Displayname = "Sniper Rifle"),
	EWT_GrenadeLauncher UMETA(Displayname = "Grenade Launcher"),
	EWT_Flag UMETA(Displayname = "Flag"),

	EWT_Max UMETA(Displayname = "Default Max"),
};