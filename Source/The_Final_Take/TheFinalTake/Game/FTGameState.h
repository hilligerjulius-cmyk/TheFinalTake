#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TheFinalTake/Core/FTTypes.h"
#include "TheFinalTake/Core/FTAudio.h"
#include "FTGameState.generated.h"

class UFTFilmDefinition;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FFTOnAnnounce, const FText&, EFTAnnounceStyle, EFTSound);
DECLARE_MULTICAST_DELEGATE_OneParam(FFTOnTakeResult, const FFTTakeResult&);
DECLARE_MULTICAST_DELEGATE(FFTOnStateChanged);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FFTOnPing, const FVector&, int32, const FString&);

/**
 * Canonical replicated shoot state. The server-side AFTSceneManager writes it;
 * HUD, boards and world visuals only read it.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AFTGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ---- shoot flow
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly) EFTShootPhase ShootPhase = EFTShootPhase::Lobby;
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly) FName FilmId;
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly) int32 SceneIndex = 0;
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly) EFTSceneState SceneState = EFTSceneState::Preparation;
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly) TArray<FFTObjectiveStatus> Objectives;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 TakeNumber = 1;

	// ---- clock & pressure
	UPROPERTY(Replicated, BlueprintReadOnly) float DawnDuration = 1800.f;
	/** Server world time when dawn arrives; 0 while the clock is stopped. */
	UPROPERTY(Replicated, BlueprintReadOnly) float DawnServerTime = 0.f;
	UPROPERTY(Replicated, BlueprintReadOnly) float FrozenRemaining = 1800.f;
	UPROPERTY(Replicated, BlueprintReadOnly) float StudioCondition = 100.f;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 TeamScore = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 StyleBonus = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 DisasterBonus = 0;

	// ---- take capture (live)
	UPROPERTY(Replicated, BlueprintReadOnly) float RecordingTime = 0.f;
	UPROPERTY(Replicated, BlueprintReadOnly) float CaptureProgress = 0.f;
	UPROPERTY(Replicated, BlueprintReadOnly) bool bKeyActionDone = false;
	UPROPERTY(Replicated, BlueprintReadOnly) FFTFrameReport LiveFrame;
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly) TArray<FFTTakeResult> TakeResults;

	// ---- world
	UPROPERTY(ReplicatedUsing = OnRep_Flood, BlueprintReadOnly) EFTFloodStage FloodStage = EFTFloodStage::Dry;
	UPROPERTY(Replicated, BlueprintReadOnly) float FloodStageTime = 0.f;
	UPROPERTY(ReplicatedUsing = OnRep_Power, BlueprintReadOnly) bool bStagePower = true;
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly) bool bProjectionUnlocked = false;
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly) bool bProjectorPower = false;
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly) int32 ReelsLoaded = 0;
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly) float PremiereStartTime = 0.f;
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly) FText FailReason;
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly) FText Callout;
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly) bool bTitleMode = false;
	/** Whoever currently has the script book open (only one at a time). */
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly) TObjectPtr<APlayerState> ScriptBookUser;

	/** Local only: still frames grabbed from the film camera when a take is accepted (premiere montage). */
	UPROPERTY(Transient) TMap<int32, TObjectPtr<class UTextureRenderTarget2D>> LocalStills;

	// ---- helpers (all machines)
	const UFTFilmDefinition* GetFilm() const;
	const FFTSceneDefinition* GetCurrentScene() const;
	float GetDawnRemaining() const;
	/** Studio clock text, e.g. "03:42 AM". */
	FText GetStudioClockText() const;
	int32 GetCompletedTakeCount() const;
	const FFTTakeResult* GetBestTake(int32 InSceneIndex) const;
	bool IsShootActive() const { return ShootPhase == EFTShootPhase::Shooting || ShootPhase == EFTShootPhase::Finale; }

	// ---- events to all machines
	UFUNCTION(NetMulticast, Reliable)
	void MulticastAnnounce(const FText& Text, EFTAnnounceStyle Style, EFTSound Sound);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastTakeResult(const FFTTakeResult& Result);
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPing(FVector_NetQuantize Location, int32 ColorIndex, const FString& Name);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSound2D(EFTSound Sound, float Volume = 1.f);

	FFTOnAnnounce OnAnnounce;
	FFTOnTakeResult OnTakeResult;
	FFTOnStateChanged OnStateChanged;
	FFTOnPing OnPing;

	UFUNCTION() void OnRep_State();
	UFUNCTION() void OnRep_Flood();
	UFUNCTION() void OnRep_Power();

	/** Server: call after changing replicated state so listen-server UI updates too. */
	void NotifyStateChanged();
	void NotifyFloodChanged();
	void NotifyPowerChanged();

private:
	EFTShootPhase LastSeenPhase = EFTShootPhase::Lobby;
};
