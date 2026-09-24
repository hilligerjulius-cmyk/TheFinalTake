#pragma once

#include "CoreMinimal.h"
#include "TheFinalTake/World/FTStudioShell.h"
#include "TheFinalTake/Props/FTProp.h"
#include "FTCity.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class USpotLightComponent;
class UFTInteractableComponent;

/**
 * Downtown: the boulevard that leaves the studio street to the west, city blocks with shops,
 * lamps and neon, the cinema plaza with parking, and the GRAND CINEMA itself (foyer with box
 * office and concessions, mezzanine projection booth, hall with stage and screen). Built from the
 * same instanced low-poly blocks as the studio. Place one at the world origin.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTCityShell : public AFTStudioShell
{
	GENERATED_BODY()

public:
	AFTCityShell();

protected:
	virtual void BuildContent() override;
	void BuildBoulevard();
	void BuildBlocks();
	void BuildPlaza();
	void BuildCinema();
	void Building(float X0, float X1, bool bNorth, float Height, const FLinearColor& Color, const FString& Sign, const FLinearColor& SignColor, int32 Seed);
	void StreetLamp(const FVector& Base, float ArmDir, bool bLight);
};

/** Marquee, blade sign, bulbs and sweeping searchlights on the cinema facade; shows tonight's film. */
UCLASS()
class THE_FINAL_TAKE_API AFTCinemaMarquee : public AFTStudioActor
{
	GENERATED_BODY()

public:
	AFTCinemaMarquee();
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Bulbs;
	UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> Lines;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> Searchlights;
	UPROPERTY() TArray<TObjectPtr<USpotLightComponent>> SearchSpots;
	float TextTimer = 0.f;
	int32 ChaseStep = 0;
	float ChaseTimer = 0.f;
};

/** Box-office chart in the foyer: every release of this studio, ranked by revenue. */
UCLASS()
class THE_FINAL_TAKE_API AFTBoxOfficeBoard : public AFTStudioActor
{
	GENERATED_BODY()

public:
	static constexpr int32 NumLines = 8;
	AFTBoxOfficeBoard();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	void Refresh();
	UPROPERTY() TObjectPtr<UTextRenderComponent> Header;
	UPROPERTY() TObjectPtr<UTextRenderComponent> Footer;
	UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> Lines;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Stars;
	FDelegateHandle CareerHandle;
};

/**
 * The cinema's seating block: raked tiers, seats and a low-poly audience (instanced meshes) that
 * fills as many seats as the release drew viewers and reacts to the show - polite for a flop,
 * on their feet for a blockbuster. Rows run along local +X (away from the screen), seats along +Y.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTCinemaSeating : public AFTStudioActor
{
	GENERATED_BODY()

public:
	AFTCinemaSeating();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Seating") int32 Rows = 12;
	UPROPERTY(EditAnywhere, Category = "Seating") int32 Cols = 20;
	UPROPERTY(EditAnywhere, Category = "Seating") float RowSpacing = 100.f;
	UPROPERTY(EditAnywhere, Category = "Seating") float SeatSpacing = 72.f;
	UPROPERTY(EditAnywhere, Category = "Seating") float Riser = 32.f;
	/** Seats kept free for the crew (four seats from CrewFirstSeat in CrewRow). */
	UPROPERTY(EditAnywhere, Category = "Seating") int32 CrewRow = 7;
	UPROPERTY(EditAnywhere, Category = "Seating") int32 CrewFirstSeat = 8;

	static AFTCinemaSeating* Find(const UWorld* World);
	int32 GetAudienceSeats() const { return Rows * Cols - 4; }
	/** Audience members the current release fills the hall with (0 outside the premiere). */
	int32 GetTargetAudience() const { return Target; }
	int32 GetShownAudience() const { return FMath::FloorToInt(Shown); }
	/** Where crew member Index stands when seated for the premiere (faces the screen). */
	FTransform GetCrewSeat(int32 Index) const;
	FVector GetHallCenter() const;

protected:
	FVector SeatLocal(int32 Row, int32 Col) const;
	bool IsCrewSeat(int32 Row, int32 Col) const;
	void BuildCrowd();
	void UpdateCrowd(float DeltaSeconds);
	int32 ComputeTarget() const;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Tiers;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> SeatBases;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> SeatBacks;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Bodies;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Heads;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Hair;

	struct FMember
	{
		FVector Seat;
		float Phase = 0.f;
		float Energy = 1.f;
		float BodyH = 60.f;
	};
	/** Audience in fill order (front-centre first). */
	TArray<FMember> Members;
	int32 Target = 0;
	float Shown = 0.f;
	int32 AppliedShown = -1;
	float Excite = 0.f;
	float AnimTimer = 0.f;
	int32 LastCheerCard = -10;
	TArray<FTransform> BodyScratch;
	TArray<FTransform> HeadScratch;
	TArray<FTransform> HairScratch;
};

/**
 * Padded film-can case: pack every finished reel into it at the reel tray, then carry the whole
 * film to the Grand Cinema in one trip (solo-friendly). The cinema projector unloads it.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTFilmCase : public AFTProp
{
	GENERATED_BODY()

public:
	AFTFilmCase();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual FText GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
	virtual void ResetForNewShoot() override;

	/** Server: store a reel (the reel actor is consumed). */
	bool PackReel(class AFTProp_Reel* Reel);
	/** Server: hand every stored reel over (the case is empty afterwards). */
	TArray<int32> TakeReels();
	int32 NumReels() const { return StoredReels.Num(); }

protected:
	UFUNCTION() void OnRep_Reels();
	void PublishCount();
	UPROPERTY(ReplicatedUsing = OnRep_Reels) TArray<int32> StoredReels;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ReelDiscs;
	UPROPERTY() TObjectPtr<UTextRenderComponent> Label;
};
