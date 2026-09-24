#pragma once

#include "CoreMinimal.h"
#include "TheFinalTake/Interaction/FTStudioActor.h"
#include "FTFilmCamera.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UFTInteractableComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UAudioComponent;
class AFTCharacter;

/**
 * The physical rolling movie camera on its dolly track. One operator claims it,
 * sees the lens view, pans/tilts/zooms, rolls the dolly and starts/stops recording.
 * Other players see the REC lamp, the moving head and a live monitor feed.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTFilmCamera : public AFTStudioActor
{
	GENERATED_BODY()

public:
	AFTFilmCamera();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual bool CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const override;
	virtual FText GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const override;
	virtual void OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User) override;
	virtual void OnUserLeft(AFTCharacter* User) override;
	virtual void ResetForNewShoot() override;
	virtual bool IsDeviceActive() const override { return Operator != nullptr; }

	// server
	void Claim(AFTCharacter* User);
	void Release(const FText& Reason = FText::GetEmpty());
	void ApplyOperatorInput(float PanDelta, float TiltDelta, float ZoomDelta, float DollyAxis);
	void Recenter();
	/** Server: pans, tilts and zooms so that all points fit in the lens (used by the auto director). */
	void FrameTargets(const TArray<FVector>& Targets, float SubjectRadius = 150.f);
	void SetRecording(bool bNewRecording);

	// all machines
	AFTCharacter* GetOperator() const { return Operator; }
	bool IsRecording() const { return bRecording; }
	UCameraComponent* GetLens() const { return Lens; }
	FTransform GetLensTransform() const;
	float GetFOV() const;
	float GetZoom01() const;
	/** Captures a still of the current lens view for the premiere montage (local). */
	UTextureRenderTarget2D* CaptureStill();

	UPROPERTY(EditAnywhere, Category = "Camera") float TrackLength = 900.f;
	UPROPERTY(EditAnywhere, Category = "Camera") float PanLimit = 70.f;
	UPROPERTY(EditAnywhere, Category = "Camera") float TiltLimit = 30.f;
	UPROPERTY(EditAnywhere, Category = "Camera") FVector2D FOVRange = FVector2D(28.f, 75.f);
	UPROPERTY(EditAnywhere, Category = "Camera") float DefaultDolly = 0.5f;

protected:
	UFUNCTION() void OnRep_Operator();
	UFUNCTION() void OnRep_Recording();
	void UpdateLocalView();
	void UpdateVisualState();

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Track;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Dolly;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> PanHead;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> TiltHead;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Lens;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RailL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RailR;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RecLamp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> MonitorScreen;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ReelA;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ReelB;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTInteractableComponent> OperateHandle;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneCaptureComponent2D> Capture;
	UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> FeedTarget;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> MotorAudio;
	UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> Sleepers;

	UPROPERTY(ReplicatedUsing = OnRep_Operator) TObjectPtr<AFTCharacter> Operator;
	UPROPERTY(ReplicatedUsing = OnRep_Recording) bool bRecording = false;
	UPROPERTY(Replicated) float Pan = 0.f;
	UPROPERTY(Replicated) float Tilt = -4.f;
	UPROPERTY(Replicated) float Zoom = 0.35f;
	UPROPERTY(Replicated) float DollyAlpha = 0.5f;

	// operator-side prediction (clients only)
	float LocalPan = 0.f;
	float LocalTilt = -4.f;
	float LocalZoom = 0.35f;
	float LocalDolly = 0.5f;

	float VisualPan = 0.f;
	float VisualTilt = 0.f;
	float VisualZoom = 0.35f;
	float VisualDolly = 0.5f;
	float FeedTimer = 0.f;
	float ReelSpin = 0.f;
	bool bLocalViewActive = false;
	bool IsLocallyOperated() const;

	UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> MonitorMID;
	static constexpr int32 NumSleepers = 10;
};
