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
	FString DescribeHands() const;
	/** Switches a toggle-style device on (never off again): uses it only while nothing tagged DeviceTag is active. */
	bool EnsureActive(const FString& ClassName, FName ActionId, FName DeviceTag, FName RequiredTag = NAME_None);
	FString DescribeObjectives() const;
	/** -FTAutoShots: adds a step that saves a screenshot (with UI) to Saved/AutoShots/<Name>.png */
	void AddShot(const FString& Name);
	float LastToggleTime = -100.f;
	bool bShots = false;
	/** Real (wall-clock) frame times after warm-up, for the performance summary. */
	TArray<float> FrameTimes;
	double LastFrameStamp = 0.0;
	// career bookkeeping checks
	int32 StartMoney = 0;
	int32 StartFilms = 0;
	int32 SpentMoney = 0;

	TArray<FStep> Steps;
	FString LastUseFailure;
	int32 Current = 0;
	float StepTime = 0.f;
	float Delay = 3.f;
	int32 Failures = 0;
	TArray<FString> Report;
	TWeakObjectPtr<AFTCharacter> Crew;
	bool bDone = false;
};
