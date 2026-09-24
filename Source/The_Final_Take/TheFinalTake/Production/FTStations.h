#pragma once

#include "CoreMinimal.h"
#include "TheFinalTake/Interaction/FTStudioActor.h"
#include "FTStations.generated.h"

class UStaticMeshComponent;
class USpotLightComponent;
class UPointLightComponent;
class UTextRenderComponent;
class UFTInteractableComponent;
class UFTChunkyParticles;
class UAudioComponent;
class AFTStageLight;

/** Miniature lighthouse practical on the beach set with a rotating beam. */
UCLASS()
class THE_FINAL_TAKE_API AFTLighthouse : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTLighthouse();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FText GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
	virtual void ResetForNewShoot() override;
	virtual bool IsDeviceActive() const override { return bOn; }
	virtual bool GetSubjectBounds(FName SubjectTag, FVector& OutCenter, float& OutRadius) const override;
protected:
	UFUNCTION() void OnRep_On();
	UPROPERTY(ReplicatedUsing = OnRep_On) bool bOn = false;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Beam;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpotLightComponent> BeamLight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Glow;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Lamp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BeamCone;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Lever;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> Switch;
};

/** A stage fixture on a stand, driven by the lighting board. Tag it Device.Light.<circuit>. */
UCLASS()
class THE_FINAL_TAKE_API AFTStageLight : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTStageLight();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void ResetForNewShoot() override;
	virtual void OnStagePowerChanged(bool bPowered) override;
	virtual void OnShootPhaseChanged(EFTShootPhase Phase) override;
	virtual bool IsDeviceActive() const override;

	void SetOn(bool bNewOn);
	void CycleColor();
	bool IsOn() const { return bOn; }
	FLinearColor GetColor() const;
	static FText CircuitName(int32 Circuit);

	/** 1 = key, 2 = fill, 3 = hero */
	UPROPERTY(EditAnywhere, Category = "Light") int32 Circuit = 1;
	UPROPERTY(EditAnywhere, Category = "Light") int32 StartColor = 0;
	UPROPERTY(EditAnywhere, Category = "Light") float Intensity = 60000.f;
	UPROPERTY(EditAnywhere, Category = "Light") bool bStartOn = false;
	UPROPERTY(EditAnywhere, Category = "Light") float StandHeight = 220.f;
	/** Downward tilt of the fixture head. */
	UPROPERTY(EditAnywhere, Category = "Light") float AimPitch = -12.f;
protected:
	UFUNCTION() void OnRep_Light();
	void ApplyLight();
	UPROPERTY(ReplicatedUsing = OnRep_Light) bool bOn = false;
	UPROPERTY(ReplicatedUsing = OnRep_Light) int32 ColorIndex = 0;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Head;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpotLightComponent> Spot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> LensPart;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Cone;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Pole;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Collar;
};

/** Lighting desk: toggle + colour for three circuits. Dead while stage power is out. */
UCLASS()
class THE_FINAL_TAKE_API AFTLightingBoard : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTLightingBoard();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual FText GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
protected:
	AFTStageLight* FindLight(int32 Circuit) const;
	UPROPERTY() TArray<TObjectPtr<UFTInteractableComponent>> Toggles;
	UPROPERTY() TArray<TObjectPtr<UFTInteractableComponent>> Colors;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ToggleCaps;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ColorCaps;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> PowerLamp;
	TArray<TWeakObjectPtr<AFTStageLight>> Lights;
};

/** Main stage breaker: hold E to restore power after the flood trips it. */
UCLASS()
class THE_FINAL_TAKE_API AFTBreaker : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTBreaker();
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
	virtual void OnStagePowerChanged(bool bPowered) override;
protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> LeverPivot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> StatusLamp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTChunkyParticles> Sparks;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> Handle;
	float SparkTimer = 0.f;
};

/** Rotating red beacon; wakes up when power is out or the stage floods. */
UCLASS()
class THE_FINAL_TAKE_API AFTEmergencyLight : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTEmergencyLight();
	virtual void Tick(float DeltaSeconds) override;
protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Spinner;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpotLightComponent> Spot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Fill;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Dome;
	bool bWasActive = false;
};

/** Sound console with three cues (siren, storm, monster sting), lamps, meters and cooldowns. */
UCLASS()
class THE_FINAL_TAKE_API AFTSoundConsole : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTSoundConsole();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
	virtual void ResetForNewShoot() override;

	/** Where the stage speakers are (sound plays there too). */
	UPROPERTY(EditAnywhere, Category = "Sound", meta = (MakeEditWidget = true)) FVector SpeakerLocation = FVector(1200.f, 0.f, 400.f);
protected:
	UFUNCTION(NetMulticast, Reliable) void MulticastCue(int32 Cue);
	float CooldownLeft(int32 Cue) const;
	UPROPERTY(Replicated) TArray<float> LastPlayed;
	UPROPERTY() TArray<TObjectPtr<UFTInteractableComponent>> Buttons;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Lamps;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Meters;
};

UENUM()
enum class EFTEffectType : uint8
{
	Wind,
	Rain,
	Smoke,
	Foam
};

/** Practical effects: wind fan, rain rig, smoke machine and foam cannon. */
UCLASS()
class THE_FINAL_TAKE_API AFTEffectMachine : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTEffectMachine();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual FText GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
	virtual void ResetForNewShoot() override;
	virtual bool IsDeviceActive() const override { return bOn; }
	virtual bool GetSubjectBounds(FName SubjectTag, FVector& OutCenter, float& OutRadius) const override;

	UPROPERTY(EditAnywhere, Category = "Effect") EFTEffectType Effect = EFTEffectType::Wind;
	/** Centre of the bounded effect zone (rain area, smoke area, foam target). */
	UPROPERTY(EditAnywhere, Category = "Effect", meta = (MakeEditWidget = true)) FVector EffectCenter = FVector(600.f, 0.f, 350.f);
	UPROPERTY(EditAnywhere, Category = "Effect") FVector EffectExtent = FVector(400.f, 700.f, 50.f);
protected:
	UFUNCTION() void OnRep_On();
	UFUNCTION(NetMulticast, Reliable) void MulticastFoam(int32 Seed);
	void BuildLook();
	UPROPERTY(ReplicatedUsing = OnRep_On) bool bOn = false;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Spinner;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Rig;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTChunkyParticles> Particles;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> Switch;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> StatusLamp;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Loop;
	float LastFoam = -10.f;
};

/** The prop fin that glides across the tank when the siren sounds (or on its crank). */
UCLASS()
class THE_FINAL_TAKE_API AFTFinGlider : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTFinGlider();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual void OnSceneEvent(FName Event, AActor* Source) override;
	virtual bool GetSubjectBounds(FName SubjectTag, FVector& OutCenter, float& OutRadius) const override;
	void StartGlide();

	UPROPERTY(EditAnywhere, Category = "Fin", meta = (MakeEditWidget = true)) FVector PathEnd = FVector(0.f, 1400.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Fin") float GlideTime = 5.5f;
protected:
	UPROPERTY(Replicated) float GlideStart = -100.f;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Fin;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> Crank;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTChunkyParticles> Wake;
};
