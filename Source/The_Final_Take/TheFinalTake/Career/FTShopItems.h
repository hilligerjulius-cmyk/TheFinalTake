#pragma once

#include "CoreMinimal.h"
#include "TheFinalTake/Career/FTCareerTypes.h"
#include "TheFinalTake/Props/FTProp.h"
#include "FTShopItems.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UFTInteractableComponent;
class UFTChunkyParticles;
class AFTShopItemActor;

/** A purchased upgrade that could end up in a shot: where it is right now and how big it reads on camera. */
struct FFTShowcaseEntry
{
	FName ItemId;
	FVector Center = FVector::ZeroVector;
	float Radius = 50.f;
	/** Ignored by the occlusion trace (the wearer, the prop itself...). */
	const AActor* Actor = nullptr;
};

namespace FTShop
{
	/** Spawns the runtime parts of an item look (props, effects, set pieces, accessories, shark kits). */
	THE_FINAL_TAKE_API void BuildParts(AActor* Owner, USceneComponent* Parent, const FFTShopItemDef& Def, TArray<TObjectPtr<UStaticMeshComponent>>& Out, bool bOwnerNoSee = false);
	/** Local-space box around the parts of an item. */
	THE_FINAL_TAKE_API FBox PartsBounds(const FFTShopItemDef& Def);
	/** One line that tells the crew how the item gets into a shot. */
	THE_FINAL_TAKE_API FText UsageHint(const FFTShopItemDef& Def);
	/** Save key of a worn accessory: "<Wearer>.<Slot>" (Wearer = "Crew.<index>" or "StandIn.<stage>"). */
	THE_FINAL_TAKE_API FName WearerKey(const FString& Wearer, EFTAccessorySlot Slot);
	/** Every purchased upgrade that is currently active: worn, placed, switched on, or installed on a raised rig. */
	THE_FINAL_TAKE_API void GatherShowcase(const UWorld* World, TArray<FFTShowcaseEntry>& Out);
	/** Spawns (server) or finds the world object of an owned placeable item. */
	THE_FINAL_TAKE_API AFTShopItemActor* FindItemActor(const UWorld* World, FName ItemId);
}

/**
 * The worn-accessory look of a crew member or stand-in: one item per slot (head, face, body),
 * rebuilt from the item data whenever the replicated item ids change.
 */
class THE_FINAL_TAKE_API FFTWornAccessories
{
public:
	static constexpr int32 NumSlots = 3;
	/** Anchors: head top, eye line, chest centre (in that order). */
	void Apply(AActor* Owner, const TArray<USceneComponent*>& Anchors, const TArray<FName>& Items, bool bOwnerNoSee);
	void SetSlotVisible(int32 Slot, bool bVisible);
	bool IsSlotVisible(int32 Slot) const { return Slot >= 0 && Slot < NumSlots && bVisible[Slot]; }
	FName Get(int32 Slot) const { return Slot >= 0 && Slot < NumSlots ? Current[Slot] : NAME_None; }
	void Clear();

private:
	FName Current[NumSlots];
	bool bVisible[NumSlots] = { true, true, true };
	TArray<TWeakObjectPtr<UStaticMeshComponent>> Parts[NumSlots];
};

/**
 * A purchased prop, practical effect or set piece standing in the studio. Carried like any prop
 * (server-authoritative, kinematic), remembers where the crew put it (career save) and - for
 * effects - has its own on/off switch. Only switched-on effects count as "active" for a take.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTShopItemActor : public AFTProp
{
	GENERATED_BODY()

public:
	AFTShopItemActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual FText GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
	virtual void ResetForNewShoot() override;
	virtual bool IsDeviceActive() const override { return bFxOn; }
	virtual bool GetSubjectBounds(FName SubjectTag, FVector& OutCenter, float& OutRadius) const override;

	/** Server, before FinishSpawning. */
	void InitItem(FName InItemId);
	FName GetItemId() const { return ItemId; }
	const FFTShopItemDef* GetDef() const;
	bool IsEffect() const;
	/** Would this item count for a take right now (effects must be switched on, nothing may be carried off-set)? */
	bool IsShowcased() const;
	/** Server: switch an effect unit on/off. */
	void SetFxOn(bool bOn);

protected:
	virtual void Land(const FVector& Location) override;
	UFUNCTION() void OnRep_ItemId();
	UFUNCTION() void OnRep_FxOn();
	void BuildLook();
	void ConfigureFx(const FFTShopItemDef& Def, const FBox& Bounds);

	UPROPERTY(ReplicatedUsing = OnRep_ItemId) FName ItemId;
	UPROPERTY(ReplicatedUsing = OnRep_FxOn) bool bFxOn = false;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> Switch;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTChunkyParticles> Fx;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTChunkyParticles> Fx2;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> GlowLight;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SwitchLamp;
	TArray<float> PartGlow;
	bool bBuilt = false;
	float GlowPhase = 0.f;
};

/** Painted loading bay: where freshly bought props, effects and set pieces are delivered. */
UCLASS()
class THE_FINAL_TAKE_API AFTDeliveryBay : public AFTStudioActor
{
	GENERATED_BODY()

public:
	AFTDeliveryBay();
	virtual void BeginPlay() override;

	/** Lower = filled first. */
	UPROPERTY(EditAnywhere, Category = "Delivery") int32 Priority = 0;
	UPROPERTY(EditAnywhere, Category = "Delivery") int32 Columns = 1;
	UPROPERTY(EditAnywhere, Category = "Delivery") int32 Rows = 3;
	UPROPERTY(EditAnywhere, Category = "Delivery") FVector2D Spacing = FVector2D(200.f, 210.f);
	UPROPERTY(EditAnywhere, Category = "Delivery") FText Title;

	/** World transforms of every pallet slot. */
	void GetSlots(TArray<FTransform>& Out) const;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> TitleText;
};

/**
 * STUDIO SUPPLY CO. counter in the lobby: opens the shop catalogue, shows the latest purchase on a
 * turntable (with a little celebration) and owns the local 3D preview used by the catalogue.
 * On the server it keeps the world in sync with the career: it spawns every owned placeable item
 * (at its saved spot or on a free delivery pallet) and removes items a career reset took away.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTShopTerminal : public AFTStudioActor
{
	GENERATED_BODY()

public:
	AFTShopTerminal();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FText GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;

	/** Local preview for the catalogue (client side, never replicated). */
	UTextureRenderTarget2D* BeginPreview();
	void SetPreviewItem(FName ItemId);
	void EndPreview();

	/** Server: make sure every owned placeable exists in the world. */
	void SyncOwnedItems();

protected:
	void HandlePurchased(FName Id);
	void HandleCareerReset();
	AFTShopItemActor* SpawnItem(FName Id, bool bAnnounceDelivery);
	bool FindDeliverySpot(FTransform& Out) const;
	UFUNCTION() void OnRep_Featured();
	UFUNCTION(NetMulticast, Reliable) void MulticastCelebrate(FName Id);
	void RebuildFeatured();

	/** Most recent purchase, shown spinning on the counter. */
	UPROPERTY(ReplicatedUsing = OnRep_Featured) FName Featured;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> Browse;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Turntable;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> FeaturedLabel;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTChunkyParticles> Confetti;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RegisterDisplay;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> FeaturedParts;

	// ---- local catalogue preview (a tiny photo studio far above the roof)
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> PreviewRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> PreviewSpin;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneCaptureComponent2D> PreviewCapture;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> PreviewKey;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> PreviewFill;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> PreviewBackdrop;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> PreviewPlinth;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> PreviewParts;
	UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> PreviewTarget;
	FName PreviewItem;
	int32 PreviewUsers = 0;
	float CelebrateTime = -100.f;
	bool bSynced = false;
};

/**
 * Wardrobe accessory wall with a makeup mirror: every bought hat, pair of glasses or cape hangs on
 * its own hook. Press E to wear it (again to take it off) - or, while carrying the stand-in, to
 * dress the stand-in instead, so solo crews can use costumes too.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTAccessoryStand : public AFTStudioActor
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxHooks = 8;

	AFTAccessoryStand();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual FText GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const override;
	virtual FText GetPromptLabel(const UFTInteractableComponent* Comp) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;

protected:
	void RefreshDisplays();
	FName ItemForHook(int32 Hook) const;

	UPROPERTY() TArray<TObjectPtr<UFTInteractableComponent>> Hooks;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> HookAnchors;
	UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> HookLabels;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> HookBusts;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> HookNecks;
	UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> HookPrices;
	/** Lays the used hooks out evenly across the wall (catalogue size decides). */
	void LayoutHooks();
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> DisplayParts;
	/** Costume items in catalogue order (one per hook). */
	TArray<FName> HookItems;
	TArray<FName> ShownOwned;
	FDelegateHandle CareerHandle;
};
