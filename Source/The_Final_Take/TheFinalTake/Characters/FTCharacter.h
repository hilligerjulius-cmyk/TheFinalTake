#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TheFinalTake/Core/FTTypes.h"
#include "TheFinalTake/Core/FTAudio.h"
#include "FTCharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UFTInteractableComponent;
class AFTStudioActor;
class AFTProp;
class AFTFilmCamera;
struct FInputActionValue;

/**
 * First-person crew member. Locally you see stylised arms; everyone else sees a chunky
 * low-poly character built from primitives with a procedural pose layer (walk, carry,
 * emotes, knockdown) and replicated costumes.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFTCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ---------------------------------------------------------------- state (replicated)
	UPROPERTY(ReplicatedUsing = OnRep_Look, BlueprintReadOnly) int32 CrewIndex = 0;
	UPROPERTY(ReplicatedUsing = OnRep_Look, BlueprintReadOnly) EFTCostume Costume = EFTCostume::None;
	UPROPERTY(ReplicatedUsing = OnRep_HeldProp, BlueprintReadOnly) TObjectPtr<AFTProp> HeldProp;
	/** Station or continuous-use actor this character is operating (camera, boat handle...). */
	UPROPERTY(Replicated, BlueprintReadOnly) TObjectPtr<AFTStudioActor> UsingActor;
	UPROPERTY(ReplicatedUsing = OnRep_Emote, BlueprintReadOnly) EFTEmote Emote = EFTEmote::None;
	UPROPERTY(ReplicatedUsing = OnRep_Emote) uint8 EmoteSerial = 0;
	UPROPERTY(ReplicatedUsing = OnRep_Knocked, BlueprintReadOnly) bool bKnockedDown = false;
	UPROPERTY(Replicated) bool bSprinting = false;

	// ---------------------------------------------------------------- server API
	void EquipCostume(EFTCostume NewCostume);
	void TakeProp(AFTProp* Prop);
	/** Detach the held prop. bThrow gives it a short forward arc. */
	void ReleaseProp(bool bThrow);
	void Knockdown(const FVector& FromLocation, float Duration, bool bDropNonCritical);
	void SetUsingActor(AFTStudioActor* Actor);
	void StopUsing();
	void TeleportToSafety(const FText& Reason);
	void PlayEmote(EFTEmote NewEmote);

	// ---------------------------------------------------------------- queries
	bool IsOperatingCamera() const;
	AFTFilmCamera* GetOperatedCamera() const;
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }
	USceneComponent* GetCarryAnchor() const { return CarryAnchor; }
	UFTInteractableComponent* GetFocusedInteractable() const { return Focused.Get(); }
	float GetHoldProgress() const { return HoldProgress; }
	bool IsInWater() const { return bInWater; }
	FLinearColor GetCrewColor() const;
	FString GetCrewName() const;
	/** Where a framed "actor" subject should be aimed at (upper body). */
	FVector GetSubjectPoint() const;
	bool IsEmoting(EFTEmote Which, float Within = 3.f) const;

	/** Last interaction target prompt text, e.g. "E  Switch On". */
	FText GetPromptText(bool& bOutAvailable, FText& OutReason) const;

	// ---------------------------------------------------------------- RPCs
	UFUNCTION(Server, Reliable) void ServerInteract(UFTInteractableComponent* Comp);
	UFUNCTION(Server, Reliable) void ServerBeginUse(UFTInteractableComponent* Comp);
	UFUNCTION(Server, Reliable) void ServerEndUse();
	UFUNCTION(Server, Reliable) void ServerDrop();
	UFUNCTION(Server, Reliable) void ServerPrimary();
	UFUNCTION(Server, Reliable) void ServerCostumeAction();
	UFUNCTION(Server, Reliable) void ServerEmote(EFTEmote NewEmote);
	UFUNCTION(Server, Reliable) void ServerSetSprint(bool bNewSprint);
	UFUNCTION(Server, Unreliable) void ServerCameraInput(float PanDelta, float TiltDelta, float ZoomDelta, float DollyAxis);
	UFUNCTION(Server, Reliable) void ServerCameraRecord();
	UFUNCTION(Server, Reliable) void ServerCameraRecenter();
	UFUNCTION(NetMulticast, Unreliable) void MulticastCharSound(EFTSound Sound);
	UFUNCTION(Client, Reliable) void ClientRejected(const FText& Reason);
	UFUNCTION(Client, Reliable) void ClientFeedback(const FText& Text);

	UFUNCTION() void OnRep_Look();
	UFUNCTION() void OnRep_HeldProp();
	UFUNCTION() void OnRep_Emote();
	UFUNCTION() void OnRep_Knocked();

protected:
	// ---------------------------------------------------------------- components
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> FirstPersonCamera;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> FPArms;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> FPArmPivotL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> FPArmPivotR;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPSleeveL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPSleeveR;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPCuffL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPCuffR;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPHandL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPHandR;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> CarryAnchor;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> BodyRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Hips;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Chest;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Neck;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> ShoulderL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> ShoulderR;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> HipL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> HipR;

	// body
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Torso;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Pelvis;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Belt;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Walkie;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Bib;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Collar;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> ArmL;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> ArmR;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> HandL;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> HandR;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LegL;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LegR;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> CuffL;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> CuffR;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> ShoeL;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> ShoeR;
	// head
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Head;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> EyeL;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> EyeR;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> PupilL;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> PupilR;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> BrowL;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> BrowR;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mouth;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Nose;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> EarL;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> EarR;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> HairTop;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> HairBack;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> HairBun;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> CapCrown;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> CapBrim;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> CapBadge;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> GlassL;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> GlassR;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> PhoneL;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> PhoneR;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> PhoneBand;
	// costumes
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LGHat;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LGBrim;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LGWhistle;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LGShorts;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SHHood;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SHJaw;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SHFin;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SHTail;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SHBelly;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> SHTeeth;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> RCCoat;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> RCHood;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> RCButtons;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> FKHelmet;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> FKVisor;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> FKPlume;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> FKPadL;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> FKPadR;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> FKShield;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> FKShieldCross;
	// identity
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Marker;
	UPROPERTY() TObjectPtr<UTextRenderComponent> NameTag;

private:
	void BuildBody();
	void ApplyLook();
	void UpdateInteractionTarget(float DeltaSeconds);
	void UpdateMovementSpeed();
	void UpdateBodyPose(float DeltaSeconds);
	void UpdateFirstPersonArms(float DeltaSeconds);
	void UpdateFace(float DeltaSeconds);
	void UpdateNameTag();
	void UpdateRecovery();
	EFTExpression ComputeExpression() const;
	void SetCostumePartsVisible();
	void AttachCarryAnchorForView();

	// input
	void InputMove(const FInputActionValue& Value);
	void InputLook(const FInputActionValue& Value);
	void InputJump();
	void InputJumpStop();
	void InputSprintStart();
	void InputSprintStop();
	void InputInteractStart();
	void InputInteractStop();
	void InputDrop();
	void InputPrimary();
	void InputZoom(const FInputActionValue& Value);
	void InputRecenter();
	void InputCostumeAction();
	void InputEmote1();
	void InputEmote2();
	void InputEmote3();
	void InputEmote4();
	void InputPing();
	void InputHelp();
	void InputPause();
	void SendEmote(EFTEmote E);
	bool IsInputBlocked() const;
	void FlushCameraInput(float DeltaSeconds);

	bool ValidateInteraction(UFTInteractableComponent* Comp, FText& OutReason) const;

	TWeakObjectPtr<UFTInteractableComponent> Focused;
	TWeakObjectPtr<UFTInteractableComponent> HoldTarget;
	TWeakObjectPtr<UFTInteractableComponent> ContinuousTarget;
	float HoldProgress = 0.f;
	bool bInteractHeld = false;

	float WalkPhase = 0.f;
	float SmoothedSpeed = 0.f;
	float KnockAlpha = 0.f;
	float EmoteStartTime = -100.f;
	float BlinkTimer = 2.f;
	float BlinkAlpha = 0.f;
	float PunchAlpha = 0.f;
	float LandBob = 0.f;
	float KnockEndTime = 0.f;
	float LastSafeCheck = 0.f;
	FVector LastSafeLocation = FVector::ZeroVector;
	bool bInWater = false;
	float WaterSplashTimer = 0.f;
	float LastRejectTime = -10.f;

	// accumulated camera-operator input, flushed to the server at a fixed rate
	float PendingPan = 0.f;
	float PendingTilt = 0.f;
	float PendingZoom = 0.f;
	float PendingDolly = 0.f;
	float CameraSendTimer = 0.f;
};
