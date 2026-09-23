#include "TheFinalTake/Game/FTGameState.h"

#include "TheFinalTake/Data/FTFilmDefinition.h"
#include "TheFinalTake/Interaction/FTStudioActor.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "FinalTakeState"

AFTGameState::AFTGameState()
{
	SetNetUpdateFrequency(15.f);
}

void AFTGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTGameState, ShootPhase);
	DOREPLIFETIME(AFTGameState, FilmId);
	DOREPLIFETIME(AFTGameState, SceneIndex);
	DOREPLIFETIME(AFTGameState, SceneState);
	DOREPLIFETIME(AFTGameState, Objectives);
	DOREPLIFETIME(AFTGameState, TakeNumber);
	DOREPLIFETIME(AFTGameState, DawnDuration);
	DOREPLIFETIME(AFTGameState, DawnServerTime);
	DOREPLIFETIME(AFTGameState, FrozenRemaining);
	DOREPLIFETIME(AFTGameState, StudioCondition);
	DOREPLIFETIME(AFTGameState, TeamScore);
	DOREPLIFETIME(AFTGameState, StyleBonus);
	DOREPLIFETIME(AFTGameState, DisasterBonus);
	DOREPLIFETIME(AFTGameState, RecordingTime);
	DOREPLIFETIME(AFTGameState, CaptureProgress);
	DOREPLIFETIME(AFTGameState, bKeyActionDone);
	DOREPLIFETIME(AFTGameState, LiveFrame);
	DOREPLIFETIME(AFTGameState, TakeResults);
	DOREPLIFETIME(AFTGameState, FloodStage);
	DOREPLIFETIME(AFTGameState, FloodStageTime);
	DOREPLIFETIME(AFTGameState, bStagePower);
	DOREPLIFETIME(AFTGameState, bProjectionUnlocked);
	DOREPLIFETIME(AFTGameState, bProjectorPower);
	DOREPLIFETIME(AFTGameState, ReelsLoaded);
	DOREPLIFETIME(AFTGameState, PremiereStartTime);
	DOREPLIFETIME(AFTGameState, FailReason);
	DOREPLIFETIME(AFTGameState, Callout);
	DOREPLIFETIME(AFTGameState, bTitleMode);
	DOREPLIFETIME(AFTGameState, ScriptBookUser);
}

const UFTFilmDefinition* AFTGameState::GetFilm() const
{
	return UFTFilmDefinition::Find(FilmId);
}

const FFTSceneDefinition* AFTGameState::GetCurrentScene() const
{
	const UFTFilmDefinition* Film = GetFilm();
	if (Film && Film->Scenes.IsValidIndex(SceneIndex))
	{
		return &Film->Scenes[SceneIndex];
	}
	return nullptr;
}

float AFTGameState::GetDawnRemaining() const
{
	if (DawnServerTime <= 0.f)
	{
		return FrozenRemaining;
	}
	return FMath::Max(0.f, DawnServerTime - GetServerWorldTimeSeconds());
}

FText AFTGameState::GetStudioClockText() const
{
	// The night runs from midnight to the 06:00 premiere.
	const float Elapsed01 = 1.f - FMath::Clamp(GetDawnRemaining() / FMath::Max(DawnDuration, 1.f), 0.f, 1.f);
	const int32 TotalMinutes = FMath::FloorToInt(Elapsed01 * 360.f);
	return FText::FromString(FString::Printf(TEXT("%02d:%02d AM"), TotalMinutes / 60, TotalMinutes % 60));
}

int32 AFTGameState::GetCompletedTakeCount() const
{
	TSet<int32> Done;
	for (const FFTTakeResult& R : TakeResults)
	{
		if (R.bAccepted)
		{
			Done.Add(R.SceneIndex);
		}
	}
	return Done.Num();
}

const FFTTakeResult* AFTGameState::GetBestTake(int32 InSceneIndex) const
{
	const FFTTakeResult* Best = nullptr;
	for (const FFTTakeResult& R : TakeResults)
	{
		if (R.SceneIndex == InSceneIndex && R.bAccepted && (!Best || R.Score > Best->Score))
		{
			Best = &R;
		}
	}
	return Best;
}

void AFTGameState::MulticastAnnounce_Implementation(const FText& Text, EFTAnnounceStyle Style, EFTSound Sound)
{
	OnAnnounce.Broadcast(Text, Style, Sound);
	if (Sound != EFTSound::None && GetNetMode() != NM_DedicatedServer)
	{
		FTAudio::Play2D(this, Sound, 0.8f);
	}
}

void AFTGameState::MulticastTakeResult_Implementation(const FFTTakeResult& Result)
{
	OnTakeResult.Broadcast(Result);
}

void AFTGameState::MulticastPing_Implementation(FVector_NetQuantize Location, int32 ColorIndex, const FString& Name)
{
	OnPing.Broadcast(Location, ColorIndex, Name);
}

void AFTGameState::MulticastSound2D_Implementation(EFTSound Sound, float Volume)
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		FTAudio::Play2D(this, Sound, Volume);
	}
}

void AFTGameState::OnRep_State()
{
	OnStateChanged.Broadcast();
}

void AFTGameState::OnRep_Flood()
{
	for (TActorIterator<AFTStudioActor> It(GetWorld()); It; ++It)
	{
		It->OnFloodStageChanged(FloodStage);
	}
	OnStateChanged.Broadcast();
}

void AFTGameState::OnRep_Power()
{
	for (TActorIterator<AFTStudioActor> It(GetWorld()); It; ++It)
	{
		It->OnStagePowerChanged(bStagePower);
	}
	OnStateChanged.Broadcast();
}

void AFTGameState::NotifyStateChanged()
{
	ForceNetUpdate();
	OnRep_State();
}

void AFTGameState::NotifyFloodChanged()
{
	ForceNetUpdate();
	OnRep_Flood();
}

void AFTGameState::NotifyPowerChanged()
{
	ForceNetUpdate();
	OnRep_Power();
}

#undef LOCTEXT_NAMESPACE
