#include "TheFinalTake/Core/FTAudio.h"

#include "TheFinalTake/Core/FTSettings.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "UObject/StrongObjectPtr.h"

static const TCHAR* SoundName(EFTSound S)
{
	switch (S)
	{
	case EFTSound::Clapper: return TEXT("S_Clapper");
	case EFTSound::UIClick: return TEXT("S_UIClick");
	case EFTSound::UIConfirm: return TEXT("S_UIConfirm");
	case EFTSound::UIError: return TEXT("S_UIError");
	case EFTSound::Stamp: return TEXT("S_Stamp");
	case EFTSound::RecBeep: return TEXT("S_RecBeep");
	case EFTSound::Siren: return TEXT("S_Siren");
	case EFTSound::Storm: return TEXT("S_Storm");
	case EFTSound::MonsterSting: return TEXT("S_MonsterSting");
	case EFTSound::ProjectorLoop: return TEXT("S_ProjectorLoop");
	case EFTSound::RainLoop: return TEXT("S_RainLoop");
	case EFTSound::WindLoop: return TEXT("S_WindLoop");
	case EFTSound::LeakLoop: return TEXT("S_LeakLoop");
	case EFTSound::Splash: return TEXT("S_Splash");
	case EFTSound::Alarm: return TEXT("S_Alarm");
	case EFTSound::PowerDown: return TEXT("S_PowerDown");
	case EFTSound::PowerUp: return TEXT("S_PowerUp");
	case EFTSound::DoorSlide: return TEXT("S_DoorSlide");
	case EFTSound::Keycard: return TEXT("S_Keycard");
	case EFTSound::Lever: return TEXT("S_Lever");
	case EFTSound::Whoosh: return TEXT("S_Whoosh");
	case EFTSound::Boing: return TEXT("S_Boing");
	case EFTSound::Thud: return TEXT("S_Thud");
	case EFTSound::Rustle: return TEXT("S_Rustle");
	case EFTSound::Squeak: return TEXT("S_Squeak");
	case EFTSound::FoamPop: return TEXT("S_FoamPop");
	case EFTSound::CameraMotor: return TEXT("S_CameraMotor");
	case EFTSound::SharkChomp: return TEXT("S_SharkChomp");
	case EFTSound::SmokeHiss: return TEXT("S_SmokeHiss");
	case EFTSound::ApplauseBig: return TEXT("S_ApplauseBig");
	case EFTSound::ApplauseSmall: return TEXT("S_ApplauseSmall");
	case EFTSound::Groan: return TEXT("S_Groan");
	case EFTSound::Laugh: return TEXT("S_Laugh");
	case EFTSound::MusicTitleLoop: return TEXT("S_MusicTitleLoop");
	case EFTSound::Fanfare: return TEXT("S_Fanfare");
	default: return nullptr;
	}
}

USoundBase* FTAudio::Get(EFTSound Sound)
{
	static TWeakObjectPtr<USoundBase> Cache[static_cast<int32>(EFTSound::Count)];
	const int32 Index = static_cast<int32>(Sound);
	if (Index <= 0 || Index >= static_cast<int32>(EFTSound::Count))
	{
		return nullptr;
	}
	if (!Cache[Index].IsValid())
	{
		const TCHAR* Name = SoundName(Sound);
		const FString Path = FString::Printf(TEXT("/Game/TheFinalTake/Audio/%s.%s"), Name, Name);
		Cache[Index] = LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	return Cache[Index].Get();
}

USoundAttenuation* FTAudio::Attenuation()
{
	static TStrongObjectPtr<USoundAttenuation> Att;
	if (!Att.IsValid())
	{
		USoundAttenuation* A = NewObject<USoundAttenuation>(GetTransientPackage(), TEXT("FTDefaultAttenuation"));
		A->Attenuation.bAttenuate = true;
		A->Attenuation.bSpatialize = true;
		A->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
		A->Attenuation.AttenuationShapeExtents = FVector(500.f, 0.f, 0.f);
		A->Attenuation.FalloffDistance = 3500.f;
		Att.Reset(A);
	}
	return Att.Get();
}

float FTAudio::Master()
{
	return UFTSettings::Get()->MasterVolume;
}

void FTAudio::Play2D(const UObject* WorldContext, EFTSound Sound, float Volume, float Pitch)
{
	if (USoundBase* S = Get(Sound))
	{
		UGameplayStatics::PlaySound2D(WorldContext, S, Volume * Master(), Pitch);
	}
}

void FTAudio::PlayAt(const UObject* WorldContext, EFTSound Sound, const FVector& Location, float Volume, float Pitch)
{
	if (USoundBase* S = Get(Sound))
	{
		UGameplayStatics::PlaySoundAtLocation(WorldContext, S, Location, Volume * Master(), Pitch, 0.f, Attenuation());
	}
}

UAudioComponent* FTAudio::Attach(USceneComponent* Parent, EFTSound Sound, float Volume, bool bAutoPlay)
{
	USoundBase* S = Get(Sound);
	if (!S || !Parent)
	{
		return nullptr;
	}
	UAudioComponent* C = UGameplayStatics::SpawnSoundAttached(S, Parent, NAME_None, FVector::ZeroVector,
		EAttachLocation::KeepRelativeOffset, false, Volume * Master(), 1.f, 0.f, Attenuation(), nullptr, false);
	if (C && !bAutoPlay)
	{
		C->Stop();
	}
	return C;
}

void FTAudio::SetVolume(UAudioComponent* Comp, float Volume)
{
	if (Comp)
	{
		Comp->SetVolumeMultiplier(Volume * Master());
	}
}
