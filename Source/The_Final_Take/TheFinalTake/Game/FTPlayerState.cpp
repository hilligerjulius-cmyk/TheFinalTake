#include "TheFinalTake/Game/FTPlayerState.h"

#include "TheFinalTake/Core/FTVisuals.h"
#include "Net/UnrealNetwork.h"

void AFTPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTPlayerState, CrewIndex);
	DOREPLIFETIME(AFTPlayerState, Costume);
	DOREPLIFETIME(AFTPlayerState, bOnCamera);
	DOREPLIFETIME(AFTPlayerState, bRunningEffect);
	DOREPLIFETIME(AFTPlayerState, bCarrying);
}

FLinearColor AFTPlayerState::CrewColor(int32 Index)
{
	switch (((Index % 4) + 4) % 4)
	{
	case 0: return FTColors::Cyan;
	case 1: return FTColors::Coral;
	case 2: return FTColors::Yellow;
	default: return FTColors::Magenta;
	}
}

FString AFTPlayerState::CrewRoleName(int32 Index)
{
	switch (((Index % 4) + 4) % 4)
	{
	case 0: return TEXT("Teal Crew");
	case 1: return TEXT("Coral Crew");
	case 2: return TEXT("Yellow Crew");
	default: return TEXT("Mint Crew");
	}
}

FString AFTPlayerState::GetCrewName() const
{
	const FString N = GetPlayerName();
	return N.IsEmpty() ? CrewRoleName(CrewIndex) : N;
}
