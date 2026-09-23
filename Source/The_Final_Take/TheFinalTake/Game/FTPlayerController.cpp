#include "TheFinalTake/Game/FTPlayerController.h"

#include "TheFinalTake/Core/FTAudio.h"
#include "TheFinalTake/Core/FTInput.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Game/FTPlayerState.h"
#include "TheFinalTake/Game/FTSceneManager.h"
#include "TheFinalTake/Characters/FTCharacter.h"
#include "TheFinalTake/Production/FTFilmCamera.h"
#include "TheFinalTake/UI/FTHUDWidget.h"
#include "TheFinalTake/UI/FTMenus.h"

#include "Camera/CameraActor.h"
#include "Components/AudioComponent.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "FinalTakePC"

AFTPlayerController::AFTPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void AFTPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		SetupInputContext();
		EnsureWidgets();
		ApplyInputMode();
	}
}

void AFTPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AFTGameState* GS = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr)
	{
		GS->OnStateChanged.Remove(StateHandle);
		GS->OnAnnounce.RemoveAll(this);
		GS->OnTakeResult.RemoveAll(this);
		GS->OnPing.RemoveAll(this);
	}
	if (Music)
	{
		Music->Stop();
	}
	Super::EndPlay(EndPlayReason);
}

void AFTPlayerController::SetupInputContext()
{
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Sub->ClearAllMappings();
			Sub->AddMappingContext(UFTInput::Get()->Context, 10);
		}
	}
}

void AFTPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void AFTPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);
	if (IsLocalController())
	{
		SetupInputContext();
		ApplyInputMode();
	}
}

void AFTPlayerController::EnsureWidgets()
{
	if (!IsLocalController())
	{
		return;
	}
	if (!HUD)
	{
		HUD = CreateWidget<UFTHUDWidget>(this, UFTHUDWidget::StaticClass());
		HUD->AddToViewport(0);
	}
	AFTGameState* GS = GetWorld()->GetGameState<AFTGameState>();
	if (!GS || StateHandle.IsValid())
	{
		return;
	}
	StateHandle = GS->OnStateChanged.AddUObject(this, &AFTPlayerController::OnStateChanged);
	GS->OnAnnounce.AddWeakLambda(this, [this](const FText& Text, EFTAnnounceStyle Style, EFTSound)
	{
		if (HUD)
		{
			HUD->ShowAnnouncement(Text, Style);
		}
	});
	GS->OnTakeResult.AddWeakLambda(this, [this](const FFTTakeResult& R)
	{
		if (HUD)
		{
			HUD->ShowTakeResult(R);
		}
		AFTGameState* G = GetWorld()->GetGameState<AFTGameState>();
		if (R.bAccepted && G)
		{
			for (TActorIterator<AFTFilmCamera> It(GetWorld()); It; ++It)
			{
				if (UTextureRenderTarget2D* Still = It->CaptureStill())
				{
					G->LocalStills.Add(R.SceneIndex, Still);
				}
				break;
			}
		}
	});
	GS->OnPing.AddWeakLambda(this, [this](const FVector& Loc, int32 Color, const FString& Name)
	{
		if (HUD)
		{
			HUD->AddPing(Loc, Color, Name);
		}
		FTAudio::Play2D(this, EFTSound::UIClick, 0.6f, 1.4f);
	});
	OnStateChanged();
}

void AFTPlayerController::OnStateChanged()
{
	AFTGameState* GS = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
	if (!GS || !IsLocalController())
	{
		return;
	}
	// title screen
	if (GS->bTitleMode && !bTitleShown)
	{
		bTitleShown = true;
		TitleMenu = CreateWidget<UFTTitleMenuWidget>(this, UFTTitleMenuWidget::StaticClass());
		TitleMenu->AddToViewport(50);
		if (HUD)
		{
			HUD->SetVisibility(ESlateVisibility::Collapsed);
		}
		UpdateTitleCamera();
	}

	// results / blackout
	const bool bResults = GS->ShootPhase == EFTShootPhase::Results || GS->ShootPhase == EFTShootPhase::Failed;
	if (bResults)
	{
		if (!Results)
		{
			Results = CreateWidget<UFTResultsWidget>(this, UFTResultsWidget::StaticClass());
		}
		if (!Results->IsInViewport())
		{
			Results->AddToViewport(40);
		}
		Results->Refresh();
	}
	else if (Results && Results->IsInViewport())
	{
		Results->RemoveFromParent();
	}

	// book was closed/taken over on the server
	if (bBookOpen && GS->ScriptBookUser != PlayerState)
	{
		LocalCloseScriptBook(false);
	}
	if (bBookOpen && ScriptBook)
	{
		ScriptBook->Refresh();
	}

	// music: title & lobby only
	const bool bWantMusic = GS->ShootPhase == EFTShootPhase::Title || GS->ShootPhase == EFTShootPhase::Lobby;
	if (bWantMusic && !Music)
	{
		if (USoundBase* S = FTAudio::Get(EFTSound::MusicTitleLoop))
		{
			Music = UGameplayStatics::CreateSound2D(this, S, (GS->bTitleMode ? 0.55f : 0.3f) * FTAudio::Master(), 1.f, 0.f, nullptr, true, false);
			if (Music)
			{
				Music->FadeIn(1.5f, (GS->bTitleMode ? 0.55f : 0.3f) * FTAudio::Master());
			}
		}
	}
	else if (!bWantMusic && Music)
	{
		Music->FadeOut(2.f, 0.f);
		Music = nullptr;
	}
	ApplyInputMode();
}

void AFTPlayerController::UpdateTitleCamera()
{
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("TitleCamera")))
		{
			SetViewTargetWithBlend(*It, 0.f);
			return;
		}
	}
}

void AFTPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsLocalController() && !StateHandle.IsValid())
	{
		EnsureWidgets();
	}
	if (Music && GetWorld())
	{
		const AFTGameState* GS = GetWorld()->GetGameState<AFTGameState>();
		Music->SetVolumeMultiplier((GS && GS->bTitleMode ? 0.55f : 0.3f) * FTAudio::Master());
	}
}

bool AFTPlayerController::IsUIBlockingGameplay() const
{
	return bTitleShown || bBookOpen || bPaused || (Results && Results->IsInViewport());
}

void AFTPlayerController::ApplyInputMode()
{
	if (!IsLocalController())
	{
		return;
	}
	if (IsUIBlockingGameplay())
	{
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		SetInputMode(Mode);
		SetShowMouseCursor(true);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}
}

void AFTPlayerController::ShowToast(const FText& Text, bool bError)
{
	if (HUD && !Text.IsEmpty())
	{
		HUD->ShowToast(Text, bError);
	}
}

void AFTPlayerController::ClientToast_Implementation(const FText& Text, bool bError)
{
	ShowToast(Text, bError);
	if (bError)
	{
		FTAudio::Play2D(this, EFTSound::UIError, 0.7f);
	}
}

void AFTPlayerController::ToggleHelp()
{
	if (HUD)
	{
		HUD->SetHelpVisible(!HUD->IsHelpVisible());
	}
}

void AFTPlayerController::TogglePause()
{
	if (bTitleShown)
	{
		return;
	}
	if (bBookOpen)
	{
		LocalCloseScriptBook(true);
		return;
	}
	if (bPaused)
	{
		ClosePause();
		return;
	}
	if (!PauseMenu)
	{
		PauseMenu = CreateWidget<UFTPauseMenuWidget>(this, UFTPauseMenuWidget::StaticClass());
	}
	PauseMenu->AddToViewport(60);
	PauseMenu->Refresh();
	bPaused = true;
	FTAudio::Play2D(this, EFTSound::UIClick, 0.8f);
	ApplyInputMode();
}

void AFTPlayerController::ClosePause()
{
	if (PauseMenu)
	{
		PauseMenu->RemoveFromParent();
	}
	bPaused = false;
	ApplyInputMode();
}

// ============================================================================ script book

void AFTPlayerController::ClientOpenScriptBook_Implementation()
{
	if (!ScriptBook)
	{
		ScriptBook = CreateWidget<UFTScriptBookWidget>(this, UFTScriptBookWidget::StaticClass());
	}
	if (!ScriptBook->IsInViewport())
	{
		ScriptBook->AddToViewport(30);
	}
	ScriptBook->Refresh();
	bBookOpen = true;
	FTAudio::Play2D(this, EFTSound::Rustle, 1.f, 0.8f);
	ApplyInputMode();
}

void AFTPlayerController::ClientCloseScriptBook_Implementation()
{
	LocalCloseScriptBook(false);
}

void AFTPlayerController::LocalCloseScriptBook(bool bNotifyServer)
{
	if (ScriptBook)
	{
		ScriptBook->RemoveFromParent();
	}
	const bool bWasOpen = bBookOpen;
	bBookOpen = false;
	if (bNotifyServer && bWasOpen)
	{
		ServerCloseScriptBook();
	}
	ApplyInputMode();
}

void AFTPlayerController::ServerSelectFilm_Implementation(FName FilmId)
{
	AFTSceneManager* SM = AFTSceneManager::Get(this);
	FText Reason;
	if (!SM || !SM->RequestSelectFilm(this, FilmId, Reason))
	{
		ClientToast(Reason.IsEmpty() ? LOCTEXT("CantSelect", "Can't greenlight that right now") : Reason, true);
		return;
	}
	ClientCloseScriptBook();
}

void AFTPlayerController::ServerCloseScriptBook_Implementation()
{
	if (AFTSceneManager* SM = AFTSceneManager::Get(this))
	{
		SM->CloseScriptBook(this);
	}
}

// ============================================================================ shoot control

void AFTPlayerController::ServerRequestRestart_Implementation()
{
	if (AFTSceneManager* SM = AFTSceneManager::Get(this))
	{
		SM->RequestRestart(this);
	}
}

void AFTPlayerController::ServerPing_Implementation(FVector_NetQuantize Location)
{
	static TMap<TWeakObjectPtr<APlayerController>, float> LastPing;
	const float Now = GetWorld()->GetTimeSeconds();
	float& Last = LastPing.FindOrAdd(this);
	if (Now - Last < 0.75f)
	{
		return;
	}
	Last = Now;
	if (AFTGameState* GS = GetWorld()->GetGameState<AFTGameState>())
	{
		const AFTPlayerState* PS = GetPlayerState<AFTPlayerState>();
		GS->MulticastPing(Location, PS ? PS->CrewIndex : 0, PS ? PS->GetCrewName() : FString(TEXT("Crew")));
	}
}

void AFTPlayerController::StartDemo(bool bHost)
{
	const FString Map = UGameplayStatics::GetCurrentLevelName(this, true);
	UGameplayStatics::OpenLevel(this, FName(*Map), true, TEXT("listen?ftplay=1"));
}

void AFTPlayerController::JoinStudio(const FString& Address)
{
	ClientTravel(Address, TRAVEL_Absolute);
}

void AFTPlayerController::LeaveToTitle()
{
	const FString Map = UGameplayStatics::GetCurrentLevelName(this, true);
	UGameplayStatics::OpenLevel(this, FName(*Map), true, TEXT(""));
}

void AFTPlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE
