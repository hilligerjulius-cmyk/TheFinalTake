#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "TheFinalTake/Core/FTTypes.h"
#include "FTSceneManager.generated.h"

class AFTGameState;
class AFTCharacter;
class AFTFilmCamera;
class AFTStudioActor;
class APlayerController;
class UFTFilmDefinition;

/**
 * Server-only scene state machine:
 * Preparation -> Ready to Roll -> Recording -> Take Complete -> Transition -> Next Scene
 * -> Finale -> Premiere -> Results. Evaluates objectives from world state, scores takes
 * (transparent 100-point model), runs the disaster timeline, dawn clock and studio condition.
 */
UCLASS(NotPlaceable)
class THE_FINAL_TAKE_API AFTSceneManager : public AInfo
{
	GENERATED_BODY()

public:
	AFTSceneManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	static AFTSceneManager* Get(const UObject* WorldContext);

	// ---- requests (validated here)
	bool RequestOpenScriptBook(APlayerController* PC, FText& OutReason);
	void CloseScriptBook(APlayerController* PC);
	bool RequestSelectFilm(APlayerController* PC, FName FilmId, FText& OutReason);
	void RequestRecordToggle(AFTFilmCamera* Camera, AFTCharacter* User);
	void RequestRestart(APlayerController* PC);
	bool RequestStartPremiere(AFTCharacter* User, FText& OutReason);
	void OnReelLoaded(int32 ReelSceneIndex);
	void OnProjectorPowered();
	void OnBreakerRestored();

	// ---- world feedback
	void ReportEvent(FName Event, AActor* Source, AFTCharacter* EventInstigator);
	void AdjustCondition(float Delta, const FText& Reason);
	bool WasEventDuringRecording(FName Event) const;
	float GetLastEventTime(FName Event) const;

	/** Framing evaluation for a camera (server + used for operator feedback). */
	FFTFrameReport EvaluateFrame(const AFTFilmCamera* Camera, const FFTSceneDefinition& Scene) const;
	static bool FindSubject(const UWorld* World, FName SubjectTag, FVector& OutCenter, float& OutRadius, AActor*& OutActor);

	void ResetShoot();

	UPROPERTY(EditAnywhere, Category = "Shoot") float DawnSeconds = 1800.f;

private:
	AFTGameState* GS() const;
	void StartFilm(FName FilmId);
	void EnterScene(int32 Index);
	void SetSceneState(EFTSceneState NewState);
	void EvaluateObjectives();
	bool EvaluateObjective(const FFTObjectiveDefinition& Def, float& OutProgress) const;
	bool AreSetupObjectivesComplete() const;
	void TickRecording(float DeltaSeconds);
	void FinishTake(bool bManualStop);
	FFTTakeResult ScoreTake() const;
	void ApplyDisaster(EFTSceneDisaster Disaster);
	void TickFlood(float DeltaSeconds);
	void TickFinale(float DeltaSeconds);
	void TickClockAndCondition(float DeltaSeconds);
	void Fail(const FText& Reason);
	void UpdateCallout();
	void SpawnReel(int32 SceneIdx);
	AFTFilmCamera* FindCamera() const;
	bool IsCostumeInZone(EFTCostume Costume, FName Zone) const;
	bool IsDeviceActive(FName DeviceTag) const;

	TMap<FName, float> EventTimes;
	float RecordStartTime = 0.f;
	float StateTime = 0.f;
	float EvalTimer = 0.f;
	float FrameAccum = 0.f;
	int32 FrameSamples = 0;
	TMap<FName, float> SubjectBest;
	float SmokeAccum = 0.f;
	float LeakWarningTime = -1.f;
	float FloodTimer = 0.f;
	float CalloutTimer = 0.f;
	float DefaultRigTimer = 0.f;
	float PremiereTimer = 0.f;
	bool bKnockedDuringTake = false;
	TSet<FName> AnnouncedObjectives;
	TWeakObjectPtr<APlayerController> BookUser;
};
