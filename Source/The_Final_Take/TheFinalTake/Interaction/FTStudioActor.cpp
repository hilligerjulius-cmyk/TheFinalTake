#include "TheFinalTake/Interaction/FTStudioActor.h"

#include "TheFinalTake/Interaction/FTInteractableComponent.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Game/FTSceneManager.h"
#include "Engine/World.h"

AFTStudioActor::AFTStudioActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(20.f);
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

FText AFTStudioActor::GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const
{
	return Comp ? Comp->Verb : FText::GetEmpty();
}

FText AFTStudioActor::GetPromptLabel(const UFTInteractableComponent* Comp) const
{
	return Comp ? Comp->Label : FText::GetEmpty();
}

bool AFTStudioActor::GetSubjectBounds(FName SubjectTag, FVector& OutCenter, float& OutRadius) const
{
	FVector Extent;
	GetActorBounds(true, OutCenter, Extent, false);
	OutRadius = FMath::Max(Extent.Size() * 0.6f, 30.f);
	return true;
}

void AFTStudioActor::MulticastSound_Implementation(EFTSound Sound, FVector_NetQuantize Location, float Volume, float Pitch)
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		FTAudio::PlayAt(this, Sound, Location, Volume, Pitch);
	}
}

AFTGameState* AFTStudioActor::GetFTGameState() const
{
	return GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
}

AFTSceneManager* AFTStudioActor::GetSceneManager() const
{
	return AFTSceneManager::Get(this);
}

void AFTStudioActor::ReportEvent(FName Event, AFTCharacter* EventInstigator)
{
	if (AFTSceneManager* SM = GetSceneManager())
	{
		SM->ReportEvent(Event, this, EventInstigator);
	}
}

void AFTStudioActor::Announce(const FText& Text, EFTAnnounceStyle Style, EFTSound Sound)
{
	if (AFTGameState* GS = GetFTGameState())
	{
		GS->MulticastAnnounce(Text, Style, Sound);
	}
}
