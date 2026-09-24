#pragma once

#include "CoreMinimal.h"
#include "TheFinalTake/Interaction/FTStudioActor.h"
#include "FTStudioObjects.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UFTInteractableComponent;
class UFTChunkyParticles;
class UWidgetComponent;
class USpotLightComponent;
class URectLightComponent;
class UAudioComponent;

UENUM()
enum class EFTDoorLock : uint8
{
	Free,			// opens on interaction
	Keycard,		// keycard panel works once a script is greenlit
	Projection		// opens automatically when the projection room unlocks
};

/** Sliding studio door with an optional keycard panel. */
UCLASS()
class THE_FINAL_TAKE_API AFTDoor : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTDoor();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual FText GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
	virtual void ResetForNewShoot() override;

	UPROPERTY(EditAnywhere, Category = "Door") EFTDoorLock Lock = EFTDoorLock::Free;
	UPROPERTY(EditAnywhere, Category = "Door") FVector2D Size = FVector2D(300.f, 380.f);
	UPROPERTY(EditAnywhere, Category = "Door") FText Sign;
	UPROPERTY(EditAnywhere, Category = "Door") FLinearColor DoorColor = FLinearColor(0.03f, 0.25f, 0.24f);
protected:
	void SetOpen(bool bNewOpen);
	UPROPERTY(Replicated) bool bOpen = false;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Panel;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PanelStripe;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FrameL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FrameR;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FrameTop;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> KeyPanel;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> KeyLamp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> SignText;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> SignTextBack;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> DoorUse;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> KeyUse;
	float OpenAlpha = 0.f;
};

/** The oversized script book on the director's desk (hold E to open the script picker). */
UCLASS()
class THE_FINAL_TAKE_API AFTScriptBook : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTScriptBook();
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> CoverTitle;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> CoverPivot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> Open;
	float CoverAngle = 0.f;
};

/** Cork board with the film, scene progress, studio clock and condition. */
UCLASS()
class THE_FINAL_TAKE_API AFTSceneBoard : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTSceneBoard();
	virtual void Tick(float DeltaSeconds) override;
protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Title;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Clock;
	UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> Lines;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Pins;
	float Timer = 0.f;
};

UENUM()
enum class EFTProjectorRole : uint8
{
	TestScreening,	// studio projection room: optional preview on the studio screen (needs projector power)
	Premiere		// Grand Cinema booth: load the reels, start the real premiere
};

/** Projector: test screening in the studio, or the real premiere in the Grand Cinema booth. */
UCLASS()
class THE_FINAL_TAKE_API AFTProjector : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTProjector();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual FText GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
	virtual void OnShootPhaseChanged(EFTShootPhase Phase) override;

	/** Where the power panel hangs (relative). */
	UPROPERTY(EditAnywhere, Category = "Projector", meta = (MakeEditWidget = true)) FVector PowerPanelOffset = FVector(-150.f, 200.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Projector") EFTProjectorRole ProjectorRole = EFTProjectorRole::TestScreening;
protected:
	void SetBeam(bool bShow);
	bool bBeamOn = false;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> ReelSpinA;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> ReelSpinB;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> BeamRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Beam;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpotLightComponent> BeamLight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> LensGlow;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> PowerPanel;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PowerLamp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> LoadReel;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> StartLever;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> PowerSwitch;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> SlotReels;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Loop;
	bool bAimed = false;
};

/** The studio's roll-up cinema screen: a slim roller during the shoot, unrolls for the premiere montage (UMG in world space). */
UCLASS()
class THE_FINAL_TAKE_API AFTCinemaScreen : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTCinemaScreen();
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	UPROPERTY(EditAnywhere, Category = "Screen") FVector2D ScreenSize = FVector2D(1600.f, 900.f);
	/** The Grand Cinema's screen shows the premiere; the studio screen only the test screening. */
	UPROPERTY(EditAnywhere, Category = "Screen") bool bGrandCinema = false;
protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> Screen;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Sheet;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BottomBar;
	UPROPERTY(VisibleAnywhere) TObjectPtr<URectLightComponent> Spill;
	void ApplyOpen();
	float Open = 0.f;
};

/** Outdoor rain + puddle shimmer for the rainy studio entrance. */
UCLASS()
class THE_FINAL_TAKE_API AFTAmbientRain : public AFTStudioActor
{
	GENERATED_BODY()
public:
	AFTAmbientRain();
	virtual void BeginPlay() override;
	UPROPERTY(EditAnywhere, Category = "Rain") FVector Extent = FVector(700.f, 1600.f, 10.f);
protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTChunkyParticles> Rain;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Loop;
};
