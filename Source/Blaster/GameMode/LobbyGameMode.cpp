// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "MultiplayerSessionsSubsystem.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	int32 NumberOfPlayers = GameState->PlayerArray.Num();

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		UMultiplayerSessionsSubsystem* SubSystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		check(SubSystem);

		if (NumberOfPlayers == SubSystem->DesiredNumPublicConnections)
		{
			UWorld* World = GetWorld();
			if (World)
			{
				bUseSeamlessTravel = true;

				FString MatchType = SubSystem->DesiredMatchType;
				if (MatchType == "Ray_FreeForAll")
				{
					World->ServerTravel(FString("/Game/Maps/BlasterMap?listen"));
				}
				else if (MatchType == "Ray_Teams")
				{
					World->ServerTravel(FString("/Game/Maps/Teams?listen"));
				}
				else if (MatchType == "Ray_CaptureTheFlag")
				{
					World->ServerTravel(FString("/Game/Maps/CaptureTheFlag?listen"));
				}
				
			}
		}
	}
}
