#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TheFinalTake/Core/FTTypes.h"
#include "FTFilmDefinition.generated.h"

/**
 * One film script: identity, script-book presentation and its scene list.
 * Each shipped film has a subclass whose constructor authors sensible defaults,
 * and a data asset instance (Content/TheFinalTake/Data) that designers can edit.
 */
UCLASS(BlueprintType)
class THE_FINAL_TAKE_API UFTFilmDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Film") FName FilmId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Film") FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Film") FText Genre;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Film", meta = (MultiLine = true)) FText Logline;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Film") FText Runtime;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Film") FText SignatureHazard;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Film") FText SignatureEffect;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Film", meta = (ClampMin = 1, ClampMax = 5)) int32 Difficulty = 2;
	/** Locked films are shown in the script book but cannot be greenlit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Film") bool bPlayable = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Film") FText LockedReason;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look") FLinearColor PosterPrimary = FLinearColor::Blue;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look") FLinearColor PosterSecondary = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look") FText PosterTagline;
	/** Soundstage the film is shot on ("Stage4", "Stage5", "Stage6"); stage-tagged actors belong to it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Career") FName StageId = TEXT("Stage4");
	/** Door / sign name of that stage, e.g. "STAGE 4". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Career") FText StageLabel;
	/** Genre appeal applied to the release audience (bigger spectacles draw more people). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Career") float AudienceMultiplier = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scenes") TArray<FFTSceneDefinition> Scenes;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("FTFilm"), FilmId); }

	/** Resolves a film by id: data asset first, then the authored defaults of the matching subclass. */
	static const UFTFilmDefinition* Find(FName FilmId);
	static TArray<const UFTFilmDefinition*> All();
};

UCLASS()
class THE_FINAL_TAKE_API UFTFilm_JawsOfTheStudio : public UFTFilmDefinition
{
	GENERATED_BODY()
public:
	UFTFilm_JawsOfTheStudio();
};

UCLASS()
class THE_FINAL_TAKE_API UFTFilm_MoonfallMotel : public UFTFilmDefinition
{
	GENERATED_BODY()
public:
	UFTFilm_MoonfallMotel();
};

UCLASS()
class THE_FINAL_TAKE_API UFTFilm_CastleOnFire : public UFTFilmDefinition
{
	GENERATED_BODY()
public:
	UFTFilm_CastleOnFire();
};
