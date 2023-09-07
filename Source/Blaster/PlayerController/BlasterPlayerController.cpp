// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Blaster/HUD/Announcement.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/HUD/ReturnToMainMenu.h"
#include "Blaster/BlasterTypes/Announcement.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Image.h"

void ABlasterPlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim)
{
	ClientElimAnnouncement(Attacker, Victim);
}

void ABlasterPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
	APlayerState* Self = GetPlayerState<APlayerState>();
	if (Attacker && Victim && Self)
	{
		BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
		if (BlasterHUD)
		{
			if (Attacker == Self && Victim != Self)
			{
				BlasterHUD->AddElimAnnouncement("You", Victim->GetPlayerName());
				return;
			}
			if (Victim == Self && Attacker != Self)
			{
				BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "you");
				return;
			}
			if (Attacker == Victim && Attacker == Self)
			{
				BlasterHUD->AddElimAnnouncement("You", "youself");
				return;
			}
			if (Attacker == Victim && Attacker != Self)
			{
				BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "themselves");
				return;
			}
			BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
		}
	}
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	ServerCheckMatchState();
}

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent == nullptr) return;

	InputComponent->BindAction("Quit", IE_Pressed, this, &ThisClass::ShowReturnToMainMenu);
}

void ABlasterPlayerController::OnPossess(APawn* aPawn)
{
	Super::OnPossess(aPawn);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(aPawn);
	if (BlasterCharacter)
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
	}
	
	SetHUDScore(0);
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, MatchState);
	DOREPLIFETIME(ThisClass, bShowTeamScores);
}

void ABlasterPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SetHUDTime();

	CheckTimeSync(DeltaSeconds);

	PollInit();

	CheckPing(DeltaSeconds);
}

void ABlasterPlayerController::CheckPing(float DeltaSeconds)
{
	PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;
	if (PlayerState == nullptr) return;

	SetPingValue(PlayerState->GetPingInMilliseconds());

	HighPingRunningTime += DeltaSeconds;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		// Ping is compressed; it's actualy ping / 4
			//UE_LOG(LogTemp, Warning, TEXT("PindMS: %f"), PlayerState->GetPingInMilliseconds());
		if (PlayerState->GetPingInMilliseconds() > HighPingThreshold)
		{
			HighPingWarning();
			PingAnimationRunningTime = 0.f;
			ServerReportPingStatus(true);
		}
		else
		{
			ServerReportPingStatus(false);
		}
		HighPingRunningTime = 0.f;
	}
	if (BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->HighPingAnimation
		&& BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnimation))
	{
		PingAnimationRunningTime += DeltaSeconds;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

void ABlasterPlayerController::SetPingValue(float PingValue)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->PingValue)
	{
		FString Text = FString::Printf(TEXT("(%dms)"), FMath::CeilToInt(PingValue));
		BlasterHUD->CharacterOverlay->PingValue->SetText(FText::FromString(Text));
	}
}

void ABlasterPlayerController::ShowReturnToMainMenu()
{
	if (ReturnToMainMenuWidget == nullptr) return;
	if (ReturnToMainMenu == nullptr)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
	}

	if (ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if (bReturnToMainMenuOpen)
		{
			ReturnToMainMenu->MenuSetup();
		}
		else
		{
			ReturnToMainMenu->MenuTearDown();
		}
	}

	//for (auto it = GetWorld()->GetPawnIterator(); it; it++)
	//{
	//	auto character = Cast<ABlasterCharacter>(*it);
	//	if (character)
	//	{
	//		auto controllerName = character->GetController() ? character->GetController()->GetFName().ToString() : FString();
	//		UE_LOG(LogTemp, Warning, TEXT("CharaName: %s, Ctrlr: %s, Auth: %d, LocalCtrl: %d"),
	//			*character->GetFName().ToString(),
	//			*controllerName,
	//			character->HasAuthority(),
	//			character->IsLocallyControlled());
	//	}
	//}
}

void ABlasterPlayerController::OnRep_ShowTeamScores()
{
	if (bShowTeamScores)
	{
		InitTeamScores();
	}
	else
	{
		HideTeamScores();
	}
}

void ABlasterPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

void ABlasterPlayerController::CheckTimeSync(float DeltaSeconds)
{
	TimeSyncRunningTime += DeltaSeconds;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ABlasterPlayerController::HighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->HighPingImage
		&& BlasterHUD->CharacterOverlay->HighPingAnimation)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		BlasterHUD->CharacterOverlay->PlayAnimation(
			BlasterHUD->CharacterOverlay->HighPingAnimation,
			0.f,
			5);
	}
}



void ABlasterPlayerController::StopHighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->HighPingImage
		&& BlasterHUD->CharacterOverlay->HighPingAnimation)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnimation))
		{
			BlasterHUD->CharacterOverlay->StopAnimation(BlasterHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch,
	float Warmup, 
	float Match,
	float Cooldown,
	float StartingTime)
{
	MatchState = StateOfMatch;
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;

	OnMatchStateSet(MatchState);

	if (BlasterHUD && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
	}
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		MatchState = GameMode->GetMatchState();
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;

		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);

		//if (IsLocalController()
		//	&& BlasterHUD && MatchState == MatchState::WaitingToStart)
		//{
		//	BlasterHUD->AddAnnouncement();
		//}
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::Cooldown)
	{
		TimeLeft = WarmupTime + MatchTime + CooldownTime - GetServerTime() + LevelStartingTime;
	}
	uint32 SecondesLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority())
	{
		BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
		if (BlasterGameMode)
		{
			SecondesLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime());
		}
	}
	
	if (CountdownInt != SecondesLeft)
	{
		if (MatchState == MatchState::WaitingToStart 
			|| MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}
	CountdownInt = SecondesLeft;
}

void ABlasterPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (BlasterHUD && BlasterHUD->CharacterOverlay)
		{
			CharacterOverlay = BlasterHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				if (bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore) SetHUDScore(HUDScore);
				if (bInitializeDefeats) SetHUDDefeat(HUDDefeats);
				if (bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);

				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
				if (BlasterCharacter && BlasterCharacter->GetCombat()
					&& bInitializeGrenades)
				{
					SetHUDGrenades(BlasterCharacter->GetCombat()->GetGrenades());
				}
			}
		}
	}
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServetTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServetTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = RoundTripTime * 0.5f;
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ABlasterPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();

	return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABlasterPlayerController::OnMatchStateSet(FName State, bool bTeamMatch)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted(bTeamMatch);
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::HandleMatchHasStarted(bool bTeamMatch)
{
	if (HasAuthority()) bShowTeamScores = bTeamMatch;

	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		if (BlasterHUD->CharacterOverlay == nullptr) BlasterHUD->AddCharacterOverlay();
		if (BlasterHUD->Announcement)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}

		if (!HasAuthority()) return;

		if (bTeamMatch)
		{
			InitTeamScores();
		}
		else
		{
			HideTeamScores();
		}
	}
}

FString ABlasterPlayerController::GetInfoText(const TArray<class ABlasterPlayerState*>& TopPlayers)
{
	ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
	if (BlasterPlayerState == nullptr) return FString();

	FString InfoTextString;

	if (TopPlayers.Num() == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)
	{
		InfoTextString = Announcement::YouAreTheWinner;
	}
	else if (TopPlayers.Num() == 1)
	{
		InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
	}
	else if (TopPlayers.Num() > 1)
	{
		InfoTextString = Announcement::PlayersTiedForTheWin;
		for (auto TiedPlayer : TopPlayers)
		{
			InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
		}
	}

	return InfoTextString;
}

FString ABlasterPlayerController::GetTeamInfoText(ABlasterGameState* BGameState)
{
	if (BGameState == nullptr) return FString();

	FString InfoTextString;

	const int32 RedTeamScore = BGameState->RedTeamScore;
	const int32 BlueTeamScore = BGameState->BlueTeamScore;
	if (RedTeamScore == 0 && BlueTeamScore == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (RedTeamScore == BlueTeamScore)
	{
		InfoTextString = Announcement::TeamsTiedForTheWin;
	}
	else if (RedTeamScore > BlueTeamScore)
	{
		InfoTextString = FString::Printf(TEXT("%s\n"), *Announcement::RedTeamWins);
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::RedTeam, RedTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::BlueTeam, BlueTeamScore));
	}
	else if (RedTeamScore < BlueTeamScore)
	{
		InfoTextString = FString::Printf(TEXT("%s\n"), *Announcement::BlueTeamWins);
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::BlueTeam, BlueTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::RedTeam, RedTeamScore));
	}

	return InfoTextString;;
}

void ABlasterPlayerController::HandleCooldown()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		BlasterHUD->CharacterOverlay->RemoveFromParent();
		if (BlasterHUD->Announcement
			&& BlasterHUD->Announcement->AnnouncementText
			&& BlasterHUD->Announcement->InfoText)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(Announcement::NewMatchStartsIn));

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			if (BlasterGameState)
			{
				TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
				FString InfoTextString = bShowTeamScores ? GetTeamInfoText(BlasterGameState) : GetInfoText(TopPlayers);
				BlasterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
			
		}
	}

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter && BlasterCharacter->GetCombat())
	{
		BlasterCharacter->bDisableGameplay = true;
		BlasterCharacter->GetCombat()->FireButtonPressed(false);
	}
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD 
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->HealthBar
		&& BlasterHUD->CharacterOverlay->HealthText)
	{
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString Text = FString::Printf(TEXT("%d/%d"),
			FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(Text));
	}
	else
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->ShieldBar
		&& BlasterHUD->CharacterOverlay->ShieldText)
	{
		const float HealthPercent = Shield / MaxShield;
		BlasterHUD->CharacterOverlay->ShieldBar->SetPercent(HealthPercent);
		FString Text = FString::Printf(TEXT("%d/%d"),
			FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		BlasterHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(Text));
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->ScoreAmount)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeScore = true;
		HUDScore = Score;
	}
}

void ABlasterPlayerController::SetHUDDefeat(int32 Defeats)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->DefeatsAmount)
	{
		FString DefeatText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatText));
	}
	else
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeats;
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->WeaponAmmoAmount)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->CarriedAmmoAmount)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->MatchCountdownText)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60.f;

		FString Text = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(Text));
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD
		&& BlasterHUD->Announcement
		&& BlasterHUD->Announcement->WarmupTime)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60.f;

		FString Text = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(Text));
	}
}

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->GrenadeText)
	{
		FString Text = FString::Printf(TEXT("%d"), Grenades);
		BlasterHUD->CharacterOverlay->GrenadeText->SetText(FText::FromString(Text));
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void ABlasterPlayerController::HideTeamScores()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->RedTeamScore
		&& BlasterHUD->CharacterOverlay->BlueTeamScore
		&& BlasterHUD->CharacterOverlay->ScoreSpacerText)
	{
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText());
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText());
		BlasterHUD->CharacterOverlay->ScoreSpacerText->SetText(FText());
	}
}

void ABlasterPlayerController::InitTeamScores()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->RedTeamScore
		&& BlasterHUD->CharacterOverlay->BlueTeamScore
		&& BlasterHUD->CharacterOverlay->ScoreSpacerText)
	{
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(FString("0")));
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(FString("0")));
		BlasterHUD->CharacterOverlay->ScoreSpacerText->SetText(FText::FromString(FString("|")));
	}
}

void ABlasterPlayerController::SetHUDRedTeamScore(int32 Score)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->RedTeamScore)
	{
		FString Text = FString::Printf(TEXT("%d"), Score);
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(Text));
	}
}

void ABlasterPlayerController::SetHUDBlueTeamScore(int32 Score)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD
		&& BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->BlueTeamScore)
	{
		FString Text = FString::Printf(TEXT("%d"), Score);
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(Text));
	}
}
