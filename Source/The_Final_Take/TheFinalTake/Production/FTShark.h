#pragma once

#include "CoreMinimal.h"
#include "TheFinalTake/Interaction/FTStudioActor.h"
#include "FTShark.generated.h"

class UStaticMeshComponent;
class UFTInteractableComponent;
class UFTChunkyParticles;
class UTextRenderComponent;

UENUM()
enum class EFTSharkState : uint8
{
	Submerged,
	Raised,
	Telegraph,
	Lunging,
	Defeated
};

/** Builds the chunky cartoon shark (body, belly, jaw, teeth, eyes, fins). */
struct FFTSharkParts
{
	USceneComponent* Root = nullptr;
	USceneComponent* Jaw = nullptr;
	UStaticMeshComponent* EyeL = nullptr;
	UStaticMeshComponent* EyeR = nullptr;
	static FFTSharkParts Build(AActor* Owner, USceneComponent* Parent, float Scale);
};

/**
 * Mechanical shark puppet on a rail inside the water tank, with its own control station:
 * Raise, Move Left, Move Right, Lunge, Reset. The lunge is telegraphed, splashes and can
 * knock people in the tank back, but never harms or soft-locks anyone.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTSharkRig : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTSharkRig();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual FText GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
	virtual void OnSceneEvent(FName Event, AActor* Source) override;
	virtual void ResetForNewShoot() override;
	virtual bool IsDeviceActive() const override { return State != EFTSharkState::Submerged; }
	virtual bool GetSubjectBounds(FName SubjectTag, FVector& OutCenter, float& OutRadius) const override;

	/** Rail runs along local Y. */
	UPROPERTY(EditAnywhere, Category = "Shark") float RailHalfLength = 500.f;
	/** Water surface height relative to the actor. */
	UPROPERTY(EditAnywhere, Category = "Shark") float WaterHeight = 70.f;
	/** How long the defeated shark floats belly-up before it sinks back into its pit. */
	UPROPERTY(EditAnywhere, Category = "Shark") float DefeatedDuration = 6.f;
	/** How far the lunge throws the shark forward; keep it inside the tank rim. */
	UPROPERTY(EditAnywhere, Category = "Shark") float LungeReach = 200.f;
	/** Station placement (relative). */
	UPROPERTY(EditAnywhere, Category = "Shark", meta = (MakeEditWidget = true)) FVector StationOffset = FVector(-300.f, -900.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Shark") float StationYaw = 90.f;
protected:
	void SetState(EFTSharkState NewState);
	void StartLunge(AFTCharacter* User);
	UFUNCTION(NetMulticast, Unreliable) void MulticastSplash(FVector_NetQuantize Where, float Size);

	UPROPERTY(Replicated) EFTSharkState State = EFTSharkState::Submerged;
	UPROPERTY(Replicated) float StateTime = 0.f;
	UPROPERTY(Replicated) float RailTarget = 0.f;
	UPROPERTY() TObjectPtr<AFTCharacter> LastOperator;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Carriage;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RailMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SharkRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Jaw;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> EyeL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> EyeR;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Station;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTChunkyParticles> Splash;
	UPROPERTY() TArray<TObjectPtr<UFTInteractableComponent>> Buttons;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ButtonCaps;

	float RailPos = 0.f;
	float VisualZ = -120.f;
	bool bDefeatQueued = false;
	bool bLungeResolved = false;
	float AutoTimer = 0.f;
	EFTSharkState LastVisualState = EFTSharkState::Submerged;
};

/**
 * The shark that swims through the flooded soundstage. Patrols, picks a wading crew member,
 * telegraphs, lunges and knocks them down briefly. Ignores marked safe zones.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTSharkHazard : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTSharkHazard();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ResetForNewShoot() override;

	UPROPERTY(EditAnywhere, Category = "Hazard", meta = (MakeEditWidget = true)) TArray<FVector> Patrol;
	UPROPERTY(EditAnywhere, Category = "Hazard") float Speed = 190.f;
protected:
	UPROPERTY(Replicated) EFTSharkState State = EFTSharkState::Submerged;
	UPROPERTY(Replicated) float StateTime = 0.f;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Jaw;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Warning;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTChunkyParticles> Wake;
	TWeakObjectPtr<AFTCharacter> Target;
	/** True if the fin can travel From->To at water level without clipping statics and with real depth below To. */
	bool IsSwimmable(const FVector& From, const FVector& To, float WaterZ) const;
	FVector LungeFrom = FVector::ZeroVector;
	FVector LungeTo = FVector::ZeroVector;
	int32 PatrolIndex = 0;
	float Cooldown = 5.f;
	bool bHit = false;
	FTransform StartTransform;
};
