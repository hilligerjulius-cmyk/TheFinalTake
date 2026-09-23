#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TheFinalTake/Core/FTTypes.h"
#include "TheFinalTake/Core/FTAudio.h"
#include "FTStudioActor.generated.h"

class UFTInteractableComponent;
class AFTCharacter;
class AFTGameState;
class AFTSceneManager;

/**
 * Base for every interactive studio object. Implements the shared interactable contract:
 * availability + prompt (evaluated on clients for display and on the server for authority),
 * server-side interaction handlers, reset for a new shoot and cleanup when a user leaves.
 *
 * Use actor Tags for scene lookups: "Device.*" (objective devices), "Subject.*" (framing subjects).
 */
UCLASS(Abstract)
class THE_FINAL_TAKE_API AFTStudioActor : public AActor
{
	GENERATED_BODY()

public:
	AFTStudioActor();

	/** Availability check. Runs on clients for prompts and on the server for validation. */
	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const { return true; }
	/** Prompt verb shown as "E - <Verb>". */
	virtual FText GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const;
	virtual FText GetPromptLabel(const UFTInteractableComponent* Comp) const;

	/** Server: press/hold completed. */
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) {}
	/** Server: continuous use started / stopped. */
	virtual void OnBeginUse(UFTInteractableComponent* Comp, AFTCharacter* User) {}
	virtual void OnEndUse(UFTInteractableComponent* Comp, AFTCharacter* User) {}

	/** Server: restore the initial state for a fresh shoot. */
	virtual void ResetForNewShoot() {}
	/** Server: a user disconnected, died or walked away while using this actor. */
	virtual void OnUserLeft(AFTCharacter* User) {}
	/** All machines: flood stage changed. */
	virtual void OnFloodStageChanged(EFTFloodStage NewStage) {}
	/** All machines: main power for stage lighting changed. */
	virtual void OnStagePowerChanged(bool bPowered) {}
	/** Server: a gameplay event happened somewhere (siren, lunge, "Director.Defeat"...). */
	virtual void OnSceneEvent(FName Event, AActor* Source) {}
	/** All machines: the shoot phase changed (premiere dims lights, blackout...). */
	virtual void OnShootPhaseChanged(EFTShootPhase Phase) {}

	/** Used by DeviceActive objectives and cue scoring. */
	virtual bool IsDeviceActive() const { return false; }
	/** Framing: where the subject is and roughly how big it is. */
	virtual bool GetSubjectBounds(FName SubjectTag, FVector& OutCenter, float& OutRadius) const;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSound(EFTSound Sound, FVector_NetQuantize Location, float Volume = 1.f, float Pitch = 1.f);

protected:
	AFTGameState* GetFTGameState() const;
	AFTSceneManager* GetSceneManager() const;
	/** Server: report a gameplay event to the scene manager (scoring / key actions). */
	void ReportEvent(FName Event, AFTCharacter* EventInstigator = nullptr);
	void Announce(const FText& Text, EFTAnnounceStyle Style, EFTSound Sound = EFTSound::None);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Studio")
	TObjectPtr<USceneComponent> Root;
};
