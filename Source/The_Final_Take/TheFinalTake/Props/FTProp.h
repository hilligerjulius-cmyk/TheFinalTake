#pragma once

#include "CoreMinimal.h"
#include "TheFinalTake/Interaction/FTStudioActor.h"
#include "FTProp.generated.h"

class UFTInteractableComponent;
class AFTCharacter;

/**
 * Carryable prop. Server-authoritative and kinematic (no unconstrained physics):
 * drops fall along a traced arc, props float on flood water, and critical props
 * return to their home shelf when lost or out of bounds.
 */
UCLASS(Abstract)
class THE_FINAL_TAKE_API AFTProp : public AFTStudioActor
{
	GENERATED_BODY()

public:
	AFTProp();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual FText GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
	virtual void ResetForNewShoot() override;
	virtual void OnUserLeft(AFTCharacter* User) override;
	virtual bool GetSubjectBounds(FName SubjectTag, FVector& OutCenter, float& OutRadius) const override;

	/** Server: attach to a carrier. */
	void PickUp(AFTCharacter* NewCarrier);
	/** Server: detach from the carrier, optionally with a short throw. */
	void Drop(bool bThrow);
	/** Server: snap back to the home shelf. */
	void ReturnHome(bool bAnnounce);
	/** Server: a critical prop dropped by a knocked-down carrier stays in hand. */
	bool IsCritical() const { return bCritical; }
	AFTCharacter* GetCarrier() const { return Carrier; }
	bool IsFloating() const { return bFloating; }
	/** Server: hand the prop to a non-player holder (e.g. a stand-in). */
	void AttachToHolder(USceneComponent* Anchor, AActor* NewHolder);
	AActor* GetHolder() const { return Holder; }
	/** Distance from the home shelf. */
	float DistanceFromHome() const { return FVector::Dist(GetActorLocation(), HomeTransform.GetLocation()); }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop") FName PropTag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop") FText PropName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop") bool bCritical = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop") bool bFloats = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop") bool bThrowable = false;
	/** How the prop sits relative to the carry anchor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop") FVector CarryOffset = FVector(0.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop") FRotator CarryRotation = FRotator::ZeroRotator;
	/** How deep the prop sits in flood water (actor origin is the prop's bottom). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop") float FloatSink = 8.f;
	/** Reel props: which scene this reel belongs to. */
	UPROPERTY(ReplicatedUsing = OnRep_ReelIndex, BlueprintReadOnly, Category = "Prop") int32 ReelSceneIndex = -1;
	/** Spawned props (reels) destroy themselves on reset instead of going home. */
	UPROPERTY() bool bDestroyOnReset = false;

	UFUNCTION() virtual void OnRep_ReelIndex() {}
	UFUNCTION() void OnRep_Carrier();

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Visual;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> Grab;
	UPROPERTY(ReplicatedUsing = OnRep_Carrier) TObjectPtr<AFTCharacter> Carrier;
	UPROPERTY(Replicated) TObjectPtr<AActor> Holder;
	UPROPERTY(Replicated) bool bFloating = false;
	void ReleaseFromHolder();

	FTransform HomeTransform;
	FVector Velocity = FVector::ZeroVector;
	bool bFalling = false;
	float FloatPhase = 0.f;
	float LostTimer = 0.f;
	float OutOfWaterTime = 0.f;

	void Land(const FVector& Location);
	void SetCarriedCollision(bool bCarried);
};

UCLASS()
class THE_FINAL_TAKE_API AFTProp_Harpoon : public AFTProp
{
	GENERATED_BODY()
public:
	AFTProp_Harpoon();
};

UCLASS()
class THE_FINAL_TAKE_API AFTProp_LifeRing : public AFTProp
{
	GENERATED_BODY()
public:
	AFTProp_LifeRing();
};

UCLASS()
class THE_FINAL_TAKE_API AFTProp_Lamp : public AFTProp
{
	GENERATED_BODY()
public:
	AFTProp_Lamp();
	virtual void BeginPlay() override;
protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UPointLightComponent> Bulb;
};

UCLASS()
class THE_FINAL_TAKE_API AFTProp_Crate : public AFTProp
{
	GENERATED_BODY()
public:
	AFTProp_Crate();
};

UCLASS()
class THE_FINAL_TAKE_API AFTProp_FinMarker : public AFTProp
{
	GENERATED_BODY()
public:
	AFTProp_FinMarker();
};

UCLASS()
class THE_FINAL_TAKE_API AFTProp_Reel : public AFTProp
{
	GENERATED_BODY()
public:
	AFTProp_Reel();
	void SetReelScene(int32 InSceneIndex);
	virtual void OnRep_ReelIndex() override;
protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UTextRenderComponent> Label;
};
