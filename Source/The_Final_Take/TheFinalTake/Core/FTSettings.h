#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FTSettings.generated.h"

/** Per-machine player settings (saved to the "FTSettings" slot). */
UCLASS()
class THE_FINAL_TAKE_API UFTSettings : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() float MasterVolume = 0.8f;
	UPROPERTY() float MouseSensitivity = 1.0f;
	UPROPERTY() bool bReducedScreenShake = false;
	UPROPERTY() bool bHighContrastUI = false;
	UPROPERTY() FString LastJoinAddress = TEXT("127.0.0.1");

	static UFTSettings* Get();
	void Save();

	DECLARE_MULTICAST_DELEGATE(FOnSettingsChanged);
	static FOnSettingsChanged OnChanged;
};
