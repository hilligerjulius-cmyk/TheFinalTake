#pragma once

#include "CoreMinimal.h"
#include "TheFinalTake/Core/FTTypes.h"
#include "FTCareerTypes.generated.h"

/** What a shop entry is and where it lives once bought. */
UENUM(BlueprintType)
enum class EFTShopCategory : uint8
{
	Costume,		// wearable accessory, equipped at the wardrobe mirror (crew or stand-in)
	Prop,			// carryable hand/floor prop, placed on a set
	Effect,			// placeable practical effect unit, must be switched on
	SharkUpgrade,	// kit installed on the shark rig, toggled at the rig desk
	SetPiece,		// larger placeable decoration
	Vehicle,		// car in the studio parking lot
	Stage			// studio expansion (unlocks a soundstage and its film)
};

/** Box-office classification of a release. */
UENUM(BlueprintType)
enum class EFTBoxOfficeTier : uint8
{
	Flop,
	CultClassic,
	Modest,
	Solid,
	WeekendHit,
	Blockbuster
};

/** Where a wearable accessory sits on a crew member / stand-in. */
UENUM(BlueprintType)
enum class EFTAccessorySlot : uint8
{
	Head,
	Face,
	Body
};

/** Visual effect a placeable effect unit emits while switched on. */
UENUM(BlueprintType)
enum class EFTItemFx : uint8
{
	None,
	Sparks,		// stylised pyro fountain (cold sparks, no real fire)
	Fog,		// low rolling fog
	Confetti,
	Bubbles,
	Glow		// pulsing emissive parts only
};

/** One primitive of a data-driven item look (same conventions as FTVis::MakePart). */
USTRUCT(BlueprintType)
struct FFTItemPart
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) EFTShape Shape = EFTShape::Box;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector Location = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector Size = FVector(20.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FRotator Rotation = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Color = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Emissive = 0.f;
};

/** A purchasable production upgrade (costume accessory, prop, effect, shark kit, set piece). */
USTRUCT(BlueprintType)
struct FFTShopItemDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ItemId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Name;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true)) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EFTShopCategory Category = EFTShopCategory::Prop;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0)) int32 Price = 1000;
	/** Production value (style points) added to a release when the item was visibly in a take. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0)) int32 StyleValue = 3;
	/** Film this fits best (shown in the shop; NAME_None = any film). Not a restriction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SuggestedFilm;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EFTAccessorySlot AccessorySlot = EFTAccessorySlot::Head;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EFTItemFx Fx = EFTItemFx::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Swatch = FLinearColor::White;
	/** Radius used for "is it in frame" checks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float FrameRadius = 60.f;
	/** Look: world object (props/effects/set pieces) or accessory relative to the attach point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FFTItemPart> Parts;
};

UENUM(BlueprintType)
enum class EFTVehicleStyle : uint8
{
	Van,
	Taxi,
	Muscle,
	Limo
};

/** Arcade car sold at the dealer. */
USTRUCT(BlueprintType)
struct FFTVehicleDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName VehicleId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Name;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true)) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0)) int32 Price = 10000;
	/** cm/s */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float TopSpeed = 1400.f;
	/** cm/s^2 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Acceleration = 700.f;
	/** Max turn rate at speed, deg/s */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Handling = 70.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 1, ClampMax = 4)) int32 Seats = 4;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EFTVehicleStyle Style = EFTVehicleStyle::Van;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Body = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Trim = FLinearColor::Black;
};

/** Studio expansion: a soundstage that unlocks a film. */
USTRUCT(BlueprintType)
struct FFTStageDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName StageId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Name;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true)) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0)) int32 Price = 30000;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName FilmId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Swatch = FLinearColor::White;
};

/** One transparent line of the audience calculation. */
USTRUCT(BlueprintType)
struct FFTReleaseLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FText Label;
	/** Signed audience contribution of this factor. */
	UPROPERTY(BlueprintReadOnly) int32 Audience = 0;
};

/** Everything the release formula needs; plain data so it can be unit-tested. */
USTRUCT()
struct FFTReleaseInput
{
	GENERATED_BODY()

	UPROPERTY() FName FilmId;
	UPROPERTY() FText FilmTitle;
	/** Best accepted take score per scene (0 = scene never accepted). */
	UPROPERTY() TArray<int32> SceneScores;
	UPROPERTY() TArray<FText> SceneTitles;
	/** Purchased items that were visibly active in at least one accepted take (unique). */
	UPROPERTY() TArray<FName> VisibleItems;
	/** Sum of their style values (before the cap). */
	UPROPERTY() int32 ProductionPoints = 0;
	/** Night left when the premiere started, 0..1 of the dawn timer. */
	UPROPERTY() float RemainingFraction = 0.f;
	UPROPERTY() float StudioCondition = 100.f;
	/** The film's own disaster happened and the crew kept shooting. */
	UPROPERTY() bool bSurvivedDisaster = false;
	/** How often this film was already released by this studio. */
	UPROPERTY() int32 TimesReleasedBefore = 0;
	UPROPERTY() float GenreMultiplier = 1.f;
	/** Seed for the small cosmetic variation (deterministic per studio + release). */
	UPROPERTY() int32 Seed = 0;
};

/** Result of a release, shown at the premiere and on the box-office card. */
USTRUCT(BlueprintType)
struct FFTReleaseReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) bool bValid = false;
	UPROPERTY(BlueprintReadOnly) FName FilmId;
	UPROPERTY(BlueprintReadOnly) FText FilmTitle;
	UPROPERTY(BlueprintReadOnly) TArray<int32> SceneScores;
	/** 0..100 */
	UPROPERTY(BlueprintReadOnly) int32 Quality = 0;
	/** Capped production value points actually used. */
	UPROPERTY(BlueprintReadOnly) int32 ProductionPoints = 0;
	UPROPERTY(BlueprintReadOnly) TArray<FName> VisibleItems;
	UPROPERTY(BlueprintReadOnly) int32 ArrivalBonusPct = 0;
	UPROPERTY(BlueprintReadOnly) int32 Audience = 0;
	UPROPERTY(BlueprintReadOnly) int32 TicketPrice = 10;
	/** Studio share, credited to the team account. */
	UPROPERTY(BlueprintReadOnly) int32 Revenue = 0;
	UPROPERTY(BlueprintReadOnly) TArray<FFTReleaseLine> Lines;
	UPROPERTY(BlueprintReadOnly) TArray<FText> Reviews;
	UPROPERTY(BlueprintReadOnly) TArray<FText> ReviewSources;
	/** 1..5 */
	UPROPERTY(BlueprintReadOnly) int32 Stars = 1;
	UPROPERTY(BlueprintReadOnly) EFTBoxOfficeTier Tier = EFTBoxOfficeTier::Flop;
	/** 1 = best revenue among all releases of this studio (including this one). */
	UPROPERTY(BlueprintReadOnly) int32 Rank = 1;
	UPROPERTY(BlueprintReadOnly) int32 RankOf = 1;
	UPROPERTY(BlueprintReadOnly) int32 BalanceAfter = 0;
	UPROPERTY(BlueprintReadOnly) int32 ReleaseNumber = 0;
};

/** A released film as stored in the career save. */
USTRUCT(BlueprintType)
struct FFTReleasedFilm
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName FilmId;
	UPROPERTY(BlueprintReadOnly) FString Title;
	UPROPERTY(BlueprintReadOnly) TArray<int32> SceneScores;
	UPROPERTY(BlueprintReadOnly) int32 Quality = 0;
	UPROPERTY(BlueprintReadOnly) int32 ProductionPoints = 0;
	UPROPERTY(BlueprintReadOnly) int32 Audience = 0;
	UPROPERTY(BlueprintReadOnly) int32 Revenue = 0;
	UPROPERTY(BlueprintReadOnly) int32 Stars = 1;
	UPROPERTY(BlueprintReadOnly) FString Headline;
	UPROPERTY(BlueprintReadOnly) EFTBoxOfficeTier Tier = EFTBoxOfficeTier::Flop;
	UPROPERTY(BlueprintReadOnly) int32 RankAtRelease = 1;
	UPROPERTY(BlueprintReadOnly) int32 ReleaseNumber = 0;
	UPROPERTY(BlueprintReadOnly) FDateTime ReleasedAt;
};

/** A purchased placeable (prop / effect / set piece) where the crew left it. */
USTRUCT(BlueprintType)
struct FFTPlacedItem
{
	GENERATED_BODY()

	UPROPERTY() FName ItemId;
	UPROPERTY() FTransform Transform;
};

/** A purchased car where it was last parked. */
USTRUCT(BlueprintType)
struct FFTParkedVehicle
{
	GENERATED_BODY()

	UPROPERTY() FName VehicleId;
	UPROPERTY() FTransform Transform;
};

/** Equipped accessory of a crew seat or stand-in (persisted so dressed stand-ins stay dressed). */
USTRUCT(BlueprintType)
struct FFTEquippedAccessory
{
	GENERATED_BODY()

	/** "Crew.<index>" or "StandIn.<stage>" */
	UPROPERTY() FName Wearer;
	UPROPERTY() FName ItemId;
};

/** Summary of a save slot for the title screen (read without loading a world). */
USTRUCT(BlueprintType)
struct FFTCareerSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 Slot = 0;
	UPROPERTY(BlueprintReadOnly) bool bExists = false;
	UPROPERTY(BlueprintReadOnly) bool bCorrupt = false;
	UPROPERTY(BlueprintReadOnly) FString StudioName;
	UPROPERTY(BlueprintReadOnly) int32 Money = 0;
	UPROPERTY(BlueprintReadOnly) int32 FilmsReleased = 0;
	UPROPERTY(BlueprintReadOnly) int32 StagesUnlocked = 0;
	UPROPERTY(BlueprintReadOnly) int32 BestRevenue = 0;
	UPROPERTY(BlueprintReadOnly) FDateTime LastSaved;
};

namespace FTCareerIds
{
	static const FName Stage4(TEXT("Stage4"));
	static const FName Stage5(TEXT("Stage5"));
	static const FName Stage6(TEXT("Stage6"));
	static const FName StarterVehicle(TEXT("Car.StudioVan"));
}
