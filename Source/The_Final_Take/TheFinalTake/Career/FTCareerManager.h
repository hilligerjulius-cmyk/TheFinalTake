#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "TheFinalTake/Career/FTCareerTypes.h"
#include "FTCareerManager.generated.h"

class UFTCareerSave;
class UFTEconomyConfig;
class AFTGameState;
class APlayerController;

/**
 * Server-only owner of the studio career. Exactly one exists on the host (listen server) or in a
 * standalone game; clients never have one, never write saves and only see the replicated copy in
 * AFTGameState. Every change of money or ownership goes through this class and the shared
 * FFTCareerLedger rules, then the career is saved (debounced) and re-replicated.
 */
UCLASS(NotPlaceable)
class THE_FINAL_TAKE_API AFTCareerManager : public AInfo
{
	GENERATED_BODY()

public:
	AFTCareerManager();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	static AFTCareerManager* Get(const UObject* WorldContext);

	/** Loads (or creates) the career of a slot and publishes it to the game state. */
	void InitCareer(int32 InSlot);

	/** Server: validated purchase for the whole team. OutReason is always filled for the buyer. */
	bool TryPurchase(APlayerController* Buyer, FName Id, FText& OutReason);

	/** Server: fills rank/number/balance, books the revenue once and saves immediately. */
	bool RecordRelease(FFTReleaseReport& Report);

	/** Server (host only): wipe this slot and start over. */
	void ResetCareer();

	/** Persist placements / parked cars / accessories right away (e.g. before travel). */
	void SaveNow(const TCHAR* Why);
	/** Request a save soon (coalesces bursts of changes). */
	void MarkDirty();

	int32 GetSlot() const { return Slot; }
	UFTCareerSave* GetSave() const { return Save; }
	bool Owns(FName Id) const;
	int32 TimesReleased(FName FilmId) const;
	bool IsStageUnlocked(FName StageId) const;

	/** World-state persistence hooks used by placeables, cars, accessories and rig kits. */
	void SetPlacement(FName ItemId, const FTransform& Where);
	bool GetPlacement(FName ItemId, FTransform& OutWhere) const;
	void SetParkedVehicle(FName VehicleId, const FTransform& Where);
	bool GetParkedVehicle(FName VehicleId, FTransform& OutWhere) const;
	void SetAccessory(FName Wearer, FName ItemId);
	FName GetAccessory(FName Wearer) const;
	void SetRigKitActive(FName ItemId, bool bActive);
	bool IsRigKitActive(FName ItemId) const;

	/** Multicast delegate for systems that spawn world objects for owned content. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FFTOnPurchased, FName /*Id*/);
	FFTOnPurchased OnPurchased;
	DECLARE_MULTICAST_DELEGATE(FFTOnCareerReset);
	FFTOnCareerReset OnCareerReset;

private:
	AFTGameState* GS() const;
	void Publish();
	bool WriteSave(const TCHAR* Why);

	UPROPERTY() TObjectPtr<UFTCareerSave> Save;
	int32 Slot = 1;
	bool bDirty = false;
	float DirtyAge = 0.f;
	/** In-flight guard: one purchase is resolved completely before the next is looked at. */
	bool bInTransaction = false;
};
