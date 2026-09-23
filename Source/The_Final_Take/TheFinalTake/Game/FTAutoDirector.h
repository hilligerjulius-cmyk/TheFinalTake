#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "FTAutoDirector.generated.h"

class AFTCharacter;
class AFTPlayerController;

/**
 * Automated server-side playthrough used for validation. Spawned by the game mode only when
 * Saved/FTAutoTest.txt exists. Drives the real gameplay objects through the whole shoot
 * (script -> 3 takes -> flood -> reels -> premiere -> results -> restart) and writes a
 * PASS/FAIL report to Saved/FTAutoTestResult.txt. It bypasses input/UI on purpose.
 */
UCLASS(NotPlaceable)
class THE_FINAL_TAKE_API AFTAutoDirector : public AInfo
{
	GENERATED_BODY()

public:
	AFTAutoDirector();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	static bool IsRequested();

private:
	struct FStep
	{
		FString Name;
		TFunction<bool()> Run;
		float Timeout = 20.f;
	};

	void BuildSteps();
	void Log(const FString& Line, bool bFail = false);
	void Finish();
	bool Use(const FString& ClassName, FName ActionId, FName RequiredTag = NAME_None);
	AActor* FindByClassName(const FString& ClassName) const;
	bool Teleport(const FVector& Where, float Yaw = 0.f);
	bool Carry(const FString& ClassName, int32 ReelIndex = -1);

	TArray<FStep> Steps;
	int32 Current = 0;
	float StepTime = 0.f;
	float Delay = 3.f;
	int32 Failures = 0;
	TArray<FString> Report;
	TWeakObjectPtr<AFTCharacter> Crew;
	bool bDone = false;
};
