#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "FTContentCommandlet.generated.h"

/**
 * Reproducible content setup (safe to re-run, never duplicates assets):
 *  - generates the faceted low-poly shape meshes into /Game/TheFinalTake/Meshes
 *  - imports RawAudio/*.wav into /Game/TheFinalTake/Audio (loops flagged)
 *  - creates the film data assets in /Game/TheFinalTake/Data
 * Run: UnrealEditor-Cmd.exe The_Final_Take.uproject -run=FTContent [-force]
 */
UCLASS()
class THE_FINAL_TAKE_API UFTContentCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UFTContentCommandlet();
	virtual int32 Main(const FString& Params) override;
};
