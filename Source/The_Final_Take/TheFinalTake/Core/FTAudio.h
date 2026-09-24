#pragma once

#include "CoreMinimal.h"
#include "FTAudio.generated.h"

class USoundBase;
class UAudioComponent;
class USceneComponent;
class USoundAttenuation;

UENUM(BlueprintType)
enum class EFTSound : uint8
{
	None,
	Clapper, UIClick, UIConfirm, UIError, Stamp, RecBeep,
	Siren, Storm, MonsterSting, ProjectorLoop, RainLoop, WindLoop, LeakLoop, Splash, Alarm,
	PowerDown, PowerUp, DoorSlide, Keycard, Lever, Whoosh, Boing, Thud, Rustle, Squeak, FoamPop,
	CameraMotor, SharkChomp, SmokeHiss, ApplauseBig, ApplauseSmall, Groan, Laugh, MusicTitleLoop, Fanfare,
	Count UMETA(Hidden)
};

/** Central playback so every sound respects the master volume setting. */
class THE_FINAL_TAKE_API FTAudio
{
public:
	static USoundBase* Get(EFTSound Sound);
	/** Loads every sound up front so the first play of a cue never stalls the game thread. */
	static void PreloadAll();
	static USoundAttenuation* Attenuation();
	static float Master();

	static void Play2D(const UObject* WorldContext, EFTSound Sound, float Volume = 1.f, float Pitch = 1.f);
	static void PlayAt(const UObject* WorldContext, EFTSound Sound, const FVector& Location, float Volume = 1.f, float Pitch = 1.f);
	/** Looping or long sound attached to a component. Caller owns the returned component. */
	static UAudioComponent* Attach(USceneComponent* Parent, EFTSound Sound, float Volume = 1.f, bool bAutoPlay = true);
	static void SetVolume(UAudioComponent* Comp, float Volume);
};
