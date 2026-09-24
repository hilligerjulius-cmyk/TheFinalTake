#include "TheFinalTake/Game/FTGameMode.h"
#include "TheFinalTake/Career/FTEconomy.h"

#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Game/FTPlayerState.h"
#include "TheFinalTake/Game/FTPlayerController.h"
#include "TheFinalTake/Game/FTSceneManager.h"
#include "TheFinalTake/Game/FTAutoDirector.h"
#include "TheFinalTake/Career/FTCareerManager.h"
#include "TheFinalTake/Career/FTCareerSave.h"
#include "TheFinalTake/Characters/FTCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

AFTGameMode::AFTGameMode()
{
	DefaultPawnClass = AFTCharacter::StaticClass();
	PlayerControllerClass = AFTPlayerController::StaticClass();
	PlayerStateClass = AFTPlayerState::StaticClass();
	GameStateClass = AFTGameState::StaticClass();
	bUseSeamlessTravel = false;
}

void AFTGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	// Standalone start without the play flag shows the title screen.
	bTitleMode = GetNetMode() == NM_Standalone && !UGameplayStatics::HasOption(Options, TEXT("ftplay")) && !AFTAutoDirector::IsRequested();
	// Automated runs never touch the player's careers: they use slot 9 unless told otherwise.
	int32 TestSlot = 9;
	FParse::Value(FCommandLine::Get(), TEXT("FTCareerSlot="), TestSlot);
	CareerSlot = AFTAutoDirector::IsRequested() ? TestSlot : 1;
	if (UGameplayStatics::HasOption(Options, TEXT("career")))
	{
		CareerSlot = FMath::Max(1, UGameplayStatics::GetIntOption(Options, TEXT("career"), CareerSlot));
	}
	// night length (the dawn deadline): balance data, then ?night=N / -FTNight=N
	DawnSeconds = UFTEconomyConfig::Get()->NightSeconds;
	if (UGameplayStatics::HasOption(Options, TEXT("night")))
	{
		DawnSeconds = (float)UGameplayStatics::GetIntOption(Options, TEXT("night"), (int32)DawnSeconds);
	}
	float NightOverride = 0.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("FTNight="), NightOverride))
	{
		DawnSeconds = NightOverride;
	}
	DawnSeconds = FMath::Clamp(DawnSeconds, 20.f, 7200.f);
}

void AFTGameMode::InitGameState()
{
	Super::InitGameState();
	if (AFTGameState* GS = GetGameState<AFTGameState>())
	{
		GS->bTitleMode = bTitleMode;
		GS->DawnDuration = DawnSeconds;
		GS->FrozenRemaining = DawnSeconds;
		GS->ShootPhase = bTitleMode ? EFTShootPhase::Title : EFTShootPhase::Lobby;
	}
}

void AFTGameMode::StartPlay()
{
	FActorSpawnParameters Params;
	Params.Name = TEXT("FTSceneManager");
	SceneManager = GetWorld()->SpawnActor<AFTSceneManager>(AFTSceneManager::StaticClass(), FTransform::Identity, Params);
	if (SceneManager)
	{
		SceneManager->DawnSeconds = DawnSeconds;
	}
	// The career lives on the host only; the title screen has no career loaded.
	if (!bTitleMode)
	{
		if (AFTAutoDirector::IsRequested() && FParse::Param(FCommandLine::Get(), TEXT("FTFreshCareer")))
		{
			UFTCareerSave::DeleteSlot(CareerSlot);
		}
		FActorSpawnParameters CareerParams;
		CareerParams.Name = TEXT("FTCareerManager");
		CareerManager = GetWorld()->SpawnActor<AFTCareerManager>(AFTCareerManager::StaticClass(), FTransform::Identity, CareerParams);
		if (CareerManager)
		{
			CareerManager->InitCareer(CareerSlot);
		}
	}
	if (AFTAutoDirector::IsRequested())
	{
		GetWorld()->SpawnActor<AFTAutoDirector>(AFTAutoDirector::StaticClass(), FTransform::Identity);
	}
	Super::StartPlay();
}

void AFTGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty())
	{
		return;
	}
	const AFTGameState* GS = GetGameState<AFTGameState>();
	if (GS && (GS->IsShootActive() || GS->ShootPhase == EFTShootPhase::Premiere))
	{
		ErrorMessage = TEXT("A shoot is already in progress. Join before the script is greenlit.");
	}
	else if (GetNumPlayers() >= 4)
	{
		ErrorMessage = TEXT("The crew is full (4 players).");
	}
}

void AFTGameMode::PostLogin(APlayerController* NewPlayer)
{
	if (AFTPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<AFTPlayerState>() : nullptr)
	{
		TSet<int32> Used;
		if (const AGameStateBase* GS = GameState)
		{
			for (APlayerState* Other : GS->PlayerArray)
			{
				if (const AFTPlayerState* O = Cast<AFTPlayerState>(Other); O && O != PS)
				{
					Used.Add(O->CrewIndex);
				}
			}
		}
		int32 Index = 0;
		while (Used.Contains(Index) && Index < 8)
		{
			++Index;
		}
		PS->CrewIndex = Index % 4;
		const FString Name = PS->GetPlayerName();
		if (Name.IsEmpty() || Name.StartsWith(TEXT("DESKTOP")) || Name.Len() > 16)
		{
			PS->SetPlayerName(AFTPlayerState::CrewRoleName(PS->CrewIndex));
		}
	}
	Super::PostLogin(NewPlayer);
}

void AFTGameMode::Logout(AController* Exiting)
{
	if (APlayerController* PC = Cast<APlayerController>(Exiting))
	{
		if (SceneManager)
		{
			SceneManager->CloseScriptBook(PC);
		}
	}
	Super::Logout(Exiting);
}

void AFTGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (bTitleMode)
	{
		return; // title screen: no pawn, the controller shows the menu
	}
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

AActor* AFTGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> Starts;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		Starts.Add(*It);
	}
	if (Starts.Num() == 0)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}
	Starts.Sort([](const APlayerStart& A, const APlayerStart& B) { return A.GetName() < B.GetName(); });
	int32 Index = 0;
	if (const AFTPlayerState* PS = Player ? Player->GetPlayerState<AFTPlayerState>() : nullptr)
	{
		Index = PS->CrewIndex;
	}
	return Starts[Index % Starts.Num()];
}
