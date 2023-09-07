// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDefeat(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);
	void SetHUDGrenades(int32 Grenades);

	void HideTeamScores();
	void InitTeamScores();
	void SetHUDRedTeamScore(int32 Score);
	void SetHUDBlueTeamScore(int32 Score);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float GetServerTime();// Synced with server world clock
	virtual void ReceivedPlayer() override;// Sync with server clock as soon as possible

	void OnMatchStateSet(FName State, bool bTeamMatch = false);

	float SingleTripTime = 0.f;

	FHighPingDelegate HighPingDelegate;

	void BroadcastElim(APlayerState* Attacker, APlayerState* Victim);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* aPawn) override;
	void SetHUDTime();
	void PollInit();

	/*
	* Sync time between client and server
	*/

	// Requests the current server time, passing in the client's time when the request was sent
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Reports the current server time to the client in response to ServerRequestServerTime
	UFUNCTION(Client, Reliable)
	void ClientReportServetTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.f;// Difference between client and server time

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;
	float TimeSyncRunningTime = 0.f;
	void CheckTimeSync(float DeltaSeconds);
	
	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName StateOfMatch,
		float Warmup, 
		float Match, 
		float Cooldown,
		float StartingTime);

	void HighPingWarning();
	void StopHighPingWarning();
	void CheckPing(float DeltaSeconds);
	void SetPingValue(float PingValue);

	void ShowReturnToMainMenu();

	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(APlayerState* Attacker, APlayerState* Victim);

	UPROPERTY(ReplicatedUsing = OnRep_ShowTeamScores)
	bool bShowTeamScores = false;

	UFUNCTION()
	void OnRep_ShowTeamScores();

	FString GetInfoText(const TArray<class ABlasterPlayerState*>& TopPlayers);
	FString GetTeamInfoText(class ABlasterGameState* BGameState);

private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	/**
	* Return to main menu
	*/
	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<class UUserWidget> ReturnToMainMenuWidget;

	UPROPERTY()
	class UReturnToMainMenu* ReturnToMainMenu;

	bool bReturnToMainMenuOpen = false;

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;

	float LevelStartingTime = 0.f;
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	bool bInitializeHealth = false;
	float HUDHealth;
	float HUDMaxHealth;

	bool bInitializeShield = false;
	float HUDShield;
	float HUDMaxShield;

	bool bInitializeScore = false;
	float HUDScore;

	bool bInitializeDefeats = false;
	int32 HUDDefeats;

	bool bInitializeGrenades = false;
	int32 HUDGrenades;

	bool bInitializeCarriedAmmo = false;
	float HUDCarriedAmmo;

	bool bInitializeWeaponAmmo = false;
	float HUDWeaponAmmo;

	void HandleMatchHasStarted(bool bTeamMatch = false);
	void HandleCooldown();

	float HighPingRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;

	float PingAnimationRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 20.f;

	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);

	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 50.f;
};
