#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TheFinalTake/Career/FTCareerTypes.h"
#include "FTCareerSave.generated.h"

class UFTEconomyConfig;

/**
 * One studio career ("world"): shared money, purchases, placed set dressing, parked cars,
 * released films. Only the host / standalone game writes it (see AFTCareerManager).
 *
 * On disk: Saved/SaveGames/FTCareer_<slot>.ftsave = "FTCR" magic, format version, payload size,
 * CRC32 and the SaveGame payload. Writes go to a temp file and are moved into place; the previous
 * file is kept as .bak. A file that fails validation is moved aside (.corrupt) and the backup or a
 * fresh career is used instead, so a broken save never blocks the game.
 */
UCLASS()
class THE_FINAL_TAKE_API UFTCareerSave : public USaveGame
{
	GENERATED_BODY()

public:
	/** Bump when fields change meaning; Migrate() upgrades older data. */
	static constexpr int32 CurrentVersion = 1;
	/** Player-visible career slots (1..NumSlots). Higher numbers are reserved for automated tests. */
	static constexpr int32 NumSlots = 3;

	UPROPERTY() int32 Version = 0;
	UPROPERTY() FString StudioName;
	UPROPERTY() int32 Money = 0;
	UPROPERTY() TArray<FName> OwnedItems;
	UPROPERTY() TArray<FName> OwnedVehicles;
	UPROPERTY() TArray<FName> UnlockedStages;
	UPROPERTY() TArray<FFTPlacedItem> Placements;
	UPROPERTY() TArray<FFTParkedVehicle> ParkedVehicles;
	UPROPERTY() TArray<FFTEquippedAccessory> Accessories;
	/** Shark kits currently switched on at the rig desk. */
	UPROPERTY() TArray<FName> ActiveRigKits;
	UPROPERTY() TArray<FFTReleasedFilm> Films;
	/** Last booked release number (guards against double booking). */
	UPROPERTY() int32 ReleaseCounter = 0;
	UPROPERTY() int64 TotalRevenue = 0;
	UPROPERTY() int64 TotalSpent = 0;
	UPROPERTY() FDateTime Created;
	UPROPERTY() FDateTime LastSaved;

	/** A new career with the configured starting money, Stage 4 and the studio van. */
	static UFTCareerSave* CreateFresh(int32 Slot, const UFTEconomyConfig& Cfg);

	/**
	 * Loads a slot. Never returns null: a missing slot yields a fresh career, a damaged one is
	 * moved aside and replaced by its backup (or a fresh career). OutNotice explains what happened.
	 */
	static UFTCareerSave* LoadOrCreate(int32 Slot, const UFTEconomyConfig& Cfg, FText& OutNotice);

	/** Validated write (temp file + move, previous file kept as .bak). */
	bool SaveToSlot(int32 Slot, FString& OutError);

	static bool DeleteSlot(int32 Slot);
	static FFTCareerSlotInfo Peek(int32 Slot);
	static FString SlotPath(int32 Slot);

	/** Upgrades older versions and repairs impossible values (negative money, unknown ids...). */
	void Sanitize(const UFTEconomyConfig& Cfg);

	int32 TimesReleased(FName FilmId) const;
	int32 BestRevenue() const;

	/** Raw file helpers (exposed for the automation tests). */
	static bool ReadValidated(const FString& Path, TArray<uint8>& OutPayload, FString& OutError);
	static bool WriteValidated(const FString& Path, const TArray<uint8>& Payload, FString& OutError);
};
