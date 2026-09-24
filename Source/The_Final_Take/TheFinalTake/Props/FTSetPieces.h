#pragma once

#include "CoreMinimal.h"
#include "TheFinalTake/Interaction/FTStudioActor.h"
#include "TheFinalTake/Props/FTProp.h"
#include "TheFinalTake/Career/FTShopItems.h"
#include "FTSetPieces.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UFTInteractableComponent;
class UFTChunkyParticles;

/**
 * Inflatable rescue boat on a slipway. Crew members hold E on the stern handles to shove it
 * into the tank: one pusher works (slowly), two pushers are much faster.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTRescueBoat : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTRescueBoat();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual FText GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const override;
	virtual void OnBeginUse(UFTInteractableComponent* Comp, AFTCharacter* User) override;
	virtual void OnEndUse(UFTInteractableComponent* Comp, AFTCharacter* User) override;
	virtual void OnUserLeft(AFTCharacter* User) override;
	virtual void OnSceneEvent(FName Event, AActor* Source) override;
	virtual void ResetForNewShoot() override;
	virtual bool GetSubjectBounds(FName SubjectTag, FVector& OutCenter, float& OutRadius) const override;

	UPROPERTY(EditAnywhere, Category = "Boat", meta = (MakeEditWidget = true)) FVector PathEnd = FVector(700.f, 0.f, 70.f);
	UPROPERTY(EditAnywhere, Category = "Boat") float ArcHeight = 120.f;
protected:
	UFUNCTION(NetMulticast, Unreliable) void MulticastRock();
	UPROPERTY(Replicated) float Progress = 0.f;
	UPROPERTY(Replicated) int32 PusherCount = 0;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> BoatRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> HandleL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> HandleR;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTChunkyParticles> Splash;
	TArray<TWeakObjectPtr<AFTCharacter>> Pushers;
	float SqueakTimer = 0.f;
	float RockTime = -10.f;
	bool bSplashed = false;
};

/** Costume rack: equip lifeguard / shark / raincoat / foam knight (or dress a carried stand-in). */
UCLASS()
class THE_FINAL_TAKE_API AFTCostumeRack : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTCostumeRack();
	virtual FText GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
protected:
	static EFTCostume CostumeForIndex(int32 Index);
	UPROPERTY() TArray<TObjectPtr<UFTInteractableComponent>> Hangers;
};

/**
 * Painted cardboard stand-in for solo crews: carry it onto a mark, change its costume and
 * hand it a prop. Counts as an actor for costume, framing and hero-mark objectives.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTStandIn : public AFTProp
{
	GENERATED_BODY()
public:
	AFTStandIn();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual FText GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
	virtual void ResetForNewShoot() override;

	void SetCostume(EFTCostume NewCostume);
	void ClearHeld() { HeldItem = nullptr; }
	AFTProp* GetHeldItem() const { return HeldItem; }
	FVector GetActingPoint() const { return GetActorLocation() + FVector(0.f, 0.f, 110.f); }

	/** Server: dress the stand-in with a purchased accessory (NAME_None removes it). Persisted per stand-in. */
	void SetAccessory(EFTAccessorySlot Slot, FName ItemId);
	FName GetAccessory(EFTAccessorySlot Slot) const { return Accessories.IsValidIndex((int32)Slot) ? Accessories[(int32)Slot] : NAME_None; }
	void GatherShowcase(TArray<FFTShowcaseEntry>& Out) const;

	UPROPERTY(ReplicatedUsing = OnRep_Costume, BlueprintReadOnly) EFTCostume Costume = EFTCostume::None;
	/** Head, face, body accessory ids. */
	UPROPERTY(ReplicatedUsing = OnRep_Costume, BlueprintReadOnly) TArray<FName> Accessories;
	/** Save key of this stand-in ("StandIn.<id>"): one stand-in per soundstage. */
	UPROPERTY(EditAnywhere, Category = "Stand-in") FName StandInId = TEXT("Stage4");
protected:
	UFUNCTION() void OnRep_Costume();
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> AccHead;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> AccFace;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> AccBody;
	/** Costume headwear hidden while a purchased hat is worn. */
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> HatParts;
	FFTWornAccessories Worn;
	UPROPERTY(Replicated) TObjectPtr<AFTProp> HeldItem;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> HandAnchor;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> CostumeButton;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> HandButton;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> CostumeLabel;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Lifeguard;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Shark;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Raincoat;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Knight;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Plain;
};

/** "Lost & Found" shelf: recalls every lost critical prop to its home shelf. */
UCLASS()
class THE_FINAL_TAKE_API AFTPropShelf : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTPropShelf();
	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
protected:
	float LastUse = -100.f;
};

/** Where finished film reels appear after each accepted take. */
UCLASS()
class THE_FINAL_TAKE_API AFTReelTray : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTReelTray();
};
