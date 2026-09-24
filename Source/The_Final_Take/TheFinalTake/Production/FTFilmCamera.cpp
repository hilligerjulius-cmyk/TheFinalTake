#include "TheFinalTake/Production/FTFilmCamera.h"

#include "TheFinalTake/Characters/FTCharacter.h"
#include "TheFinalTake/Core/FTVisuals.h"
#include "TheFinalTake/Interaction/FTInteractableComponent.h"
#include "TheFinalTake/Game/FTSceneManager.h"
#include "TheFinalTake/Game/FTPlayerState.h"
#include "TheFinalTake/Props/FTProp.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "FinalTakeCamera"

AFTFilmCamera::AFTFilmCamera()
{
	using namespace FTColors;
	PrimaryActorTick.bCanEverTick = true;
	SetNetUpdateFrequency(30.f);
	Tags = { TEXT("Device.Camera") };

	Track = CreateDefaultSubobject<USceneComponent>(TEXT("Track"));
	Track->SetupAttachment(Root);
	RailL = FTVis::MakePart(this, Track, TEXT("RailL"), EFTShape::Box, FVector(-34.f, 0.f, 4.f), FVector(8.f, 900.f, 8.f), Grey);
	RailR = FTVis::MakePart(this, Track, TEXT("RailR"), EFTShape::Box, FVector(34.f, 0.f, 4.f), FVector(8.f, 900.f, 8.f), Grey);
	for (int32 i = 0; i < NumSleepers; ++i)
	{
		Sleepers.Add(FTVis::MakePart(this, Track, *FString::Printf(TEXT("Sleeper%d"), i), EFTShape::Box, FVector(0.f, 0.f, 2.f), FVector(96.f, 12.f, 4.f), WoodDark));
	}

	Dolly = CreateDefaultSubobject<USceneComponent>(TEXT("Dolly"));
	Dolly->SetupAttachment(Root);
	FTVis::MakePart(this, Dolly, TEXT("Platform"), EFTShape::Box, FVector(0.f, 0.f, 22.f), FVector(96.f, 78.f, 14.f), GreyDark);
	FTVis::MakePart(this, Dolly, TEXT("PlatformTrim"), EFTShape::Box, FVector(0.f, 0.f, 30.f), FVector(90.f, 72.f, 3.f), Grey);
	int32 W = 0;
	for (float X : { -34.f, 34.f })
	{
		for (float Y : { -34.f, 34.f })
		{
			FTVis::MakePart(this, Dolly, *FString::Printf(TEXT("Wheel%d"), W++), EFTShape::Cylinder, FVector(X, Y, 14.f), FVector(22.f, 22.f, 10.f), Charcoal, FRotator(0.f, 0.f, 90.f));
		}
	}
	FTVis::MakePart(this, Dolly, TEXT("PushBar"), EFTShape::Cylinder, FVector(-52.f, 0.f, 70.f), FVector(6.f, 6.f, 70.f), Coral, FRotator(0.f, 0.f, 90.f));
	FTVis::MakePart(this, Dolly, TEXT("PushPostL"), EFTShape::Cylinder, FVector(-48.f, -32.f, 50.f), FVector(5.f, 5.f, 42.f), Coral);
	FTVis::MakePart(this, Dolly, TEXT("PushPostR"), EFTShape::Cylinder, FVector(-48.f, 32.f, 50.f), FVector(5.f, 5.f, 42.f), Coral);
	FTVis::MakePart(this, Dolly, TEXT("Pedestal"), EFTShape::Cylinder, FVector(0.f, 0.f, 72.f), FVector(20.f, 20.f, 84.f), Charcoal);
	FTVis::MakePart(this, Dolly, TEXT("PedestalRing"), EFTShape::Cylinder, FVector(0.f, 0.f, 44.f), FVector(34.f, 34.f, 8.f), Teal);

	PanHead = CreateDefaultSubobject<USceneComponent>(TEXT("PanHead"));
	PanHead->SetupAttachment(Dolly);
	PanHead->SetRelativeLocation(FVector(0.f, 0.f, 118.f));
	FTVis::MakePart(this, PanHead, TEXT("HeadBase"), EFTShape::Cylinder, FVector(0.f, 0.f, 0.f), FVector(28.f, 28.f, 10.f), GreyDark);
	TiltHead = CreateDefaultSubobject<USceneComponent>(TEXT("TiltHead"));
	TiltHead->SetupAttachment(PanHead);
	TiltHead->SetRelativeLocation(FVector(0.f, 0.f, 22.f));

	FTVis::MakePart(this, TiltHead, TEXT("Body"), EFTShape::Box, FVector(0.f, 0.f, 0.f), FVector(56.f, 30.f, 34.f), Teal);
	FTVis::MakePart(this, TiltHead, TEXT("BodyStripe"), EFTShape::Box, FVector(0.f, 0.f, -12.f), FVector(58.f, 32.f, 6.f), TealDark);
	FTVis::MakePart(this, TiltHead, TEXT("Badge"), EFTShape::Box, FVector(10.f, 15.5f, 4.f), FVector(12.f, 2.f, 8.f), Coral);
	FTVis::MakePart(this, TiltHead, TEXT("LensBarrel"), EFTShape::Cylinder, FVector(40.f, 0.f, 0.f), FVector(22.f, 22.f, 28.f), Charcoal, FRotator(-90.f, 0.f, 0.f));
	FTVis::MakePart(this, TiltHead, TEXT("LensRing"), EFTShape::Cylinder, FVector(50.f, 0.f, 0.f), FVector(25.f, 25.f, 5.f), GreyDark, FRotator(-90.f, 0.f, 0.f));
	FTVis::MakePart(this, TiltHead, TEXT("LensGlass"), EFTShape::Cylinder, FVector(55.f, 0.f, 0.f), FVector(16.f, 16.f, 2.f), Cyan, FRotator(-90.f, 0.f, 0.f), 1.5f);
	FTVis::MakePart(this, TiltHead, TEXT("MatteBox"), EFTShape::Box, FVector(62.f, 0.f, 0.f), FVector(8.f, 38.f, 30.f), Charcoal);
	ReelA = FTVis::MakePart(this, TiltHead, TEXT("ReelA"), EFTShape::Cylinder, FVector(-8.f, 0.f, 34.f), FVector(34.f, 34.f, 8.f), Charcoal, FRotator(0.f, 0.f, 90.f));
	ReelB = FTVis::MakePart(this, TiltHead, TEXT("ReelB"), EFTShape::Cylinder, FVector(-34.f, 0.f, 30.f), FVector(30.f, 30.f, 8.f), Charcoal, FRotator(0.f, 0.f, 90.f));
	FTVis::MakePart(this, ReelA, TEXT("ReelAHub"), EFTShape::Cylinder, FVector(0.f, 0.f, 0.f), FVector(60.f, 60.f, 140.f), Grey);
	FTVis::MakePart(this, ReelB, TEXT("ReelBHub"), EFTShape::Cylinder, FVector(0.f, 0.f, 0.f), FVector(60.f, 60.f, 140.f), Grey);
	FTVis::MakePart(this, TiltHead, TEXT("Handle"), EFTShape::Capsule, FVector(-44.f, 22.f, -6.f), FVector(8.f, 8.f, 36.f), Coral, FRotator(-90.f, 0.f, 0.f));
	FTVis::MakePart(this, TiltHead, TEXT("Viewfinder"), EFTShape::Box, FVector(-30.f, -18.f, 8.f), FVector(22.f, 10.f, 12.f), Charcoal);
	RecLamp = FTVis::MakePart(this, TiltHead, TEXT("RecLamp"), EFTShape::Sphere, FVector(22.f, 10.f, 20.f), FVector(8.f, 8.f, 8.f), Red, FRotator::ZeroRotator, 0.2f);

	// monitor for the rest of the crew
	FTVis::MakePart(this, Dolly, TEXT("MonitorArm"), EFTShape::Cylinder, FVector(-30.f, -44.f, 70.f), FVector(4.f, 4.f, 60.f), Charcoal);
	FTVis::MakePart(this, Dolly, TEXT("MonitorBox"), EFTShape::Box, FVector(-32.f, -44.f, 108.f), FVector(8.f, 44.f, 28.f), Charcoal, FRotator(0.f, 180.f, 0.f));
	MonitorScreen = FTVis::MakePart(this, Dolly, TEXT("MonitorScreen"), EFTShape::Plane, FVector(-36.5f, -44.f, 108.f), FVector(38.f, 22.f, 1.f), Navy, FRotator(0.f, 90.f, 90.f), 0.4f);

	Lens = CreateDefaultSubobject<UCameraComponent>(TEXT("Lens"));
	Lens->SetupAttachment(TiltHead);
	Lens->SetRelativeLocation(FVector(66.f, 0.f, 0.f));
	Lens->SetFieldOfView(50.f);
	Lens->bConstrainAspectRatio = false;
	// "film look" for everything seen through the lens: shallow focus, grain, vignette, warm/teal grade
	{
		FPostProcessSettings& PP = Lens->PostProcessSettings;
		PP.bOverride_DepthOfFieldFocalDistance = true;
		PP.DepthOfFieldFocalDistance = 900.f;
		PP.bOverride_DepthOfFieldFstop = true;
		PP.DepthOfFieldFstop = 2.8f;
		PP.bOverride_DepthOfFieldSensorWidth = true;
		PP.DepthOfFieldSensorWidth = 36.f;
		PP.bOverride_VignetteIntensity = true;
		PP.VignetteIntensity = 0.7f;
		PP.bOverride_FilmGrainIntensity = true;
		PP.FilmGrainIntensity = 0.2f;
		PP.bOverride_SceneFringeIntensity = true;
		PP.SceneFringeIntensity = 0.8f;
		PP.bOverride_BloomIntensity = true;
		PP.BloomIntensity = 0.9f;
		PP.bOverride_ColorSaturation = true;
		PP.ColorSaturation = FVector4(1.15f, 1.15f, 1.15f, 1.f);
		PP.bOverride_ColorContrast = true;
		PP.ColorContrast = FVector4(1.12f, 1.12f, 1.12f, 1.f);
		PP.bOverride_ColorGainHighlights = true;
		PP.ColorGainHighlights = FVector4(1.06f, 1.f, 0.9f, 1.f);
		PP.bOverride_ColorGainShadows = true;
		PP.ColorGainShadows = FVector4(0.9f, 1.f, 1.08f, 1.f);
		Lens->PostProcessBlendWeight = 1.f;
	}

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(Lens);
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->FOVAngle = 50.f;
	Capture->PostProcessSettings = Lens->PostProcessSettings;
	Capture->PostProcessBlendWeight = 1.f;
	// the monitor is a small, infrequent render without history: skip the expensive features that need history anyway
	Capture->ShowFlags.SetLumenGlobalIllumination(false);
	Capture->ShowFlags.SetLumenReflections(false);
	Capture->ShowFlags.SetDistanceFieldAO(false);
	Capture->ShowFlags.SetVolumetricFog(false);
	Capture->ShowFlags.SetMotionBlur(false);

	OperateHandle = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("OperateHandle"));
	OperateHandle->SetupAttachment(Dolly);
	OperateHandle->Setup(TEXT("Operate"), LOCTEXT("CameraLabel", "Movie Camera"), LOCTEXT("Operate", "Operate"), EFTInteractType::Press, FVector(55.f, 55.f, 70.f));
	OperateHandle->SetRelativeLocation(FVector(-10.f, 0.f, 110.f));
	OperateHandle->MaxDistance = 360.f;
}

void AFTFilmCamera::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTFilmCamera, Operator);
	DOREPLIFETIME(AFTFilmCamera, bRecording);
	DOREPLIFETIME(AFTFilmCamera, Pan);
	DOREPLIFETIME(AFTFilmCamera, Tilt);
	DOREPLIFETIME(AFTFilmCamera, Zoom);
	DOREPLIFETIME(AFTFilmCamera, DollyAlpha);
}

void AFTFilmCamera::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	FTVis::ApplyShape(RailL, EFTShape::Box, FVector(8.f, TrackLength + 60.f, 8.f));
	FTVis::ApplyShape(RailR, EFTShape::Box, FVector(8.f, TrackLength + 60.f, 8.f));
	for (int32 i = 0; i < Sleepers.Num(); ++i)
	{
		const float A = Sleepers.Num() > 1 ? (float)i / (Sleepers.Num() - 1) : 0.5f;
		Sleepers[i]->SetRelativeLocation(FVector(0.f, FMath::Lerp(-TrackLength * 0.5f - 20.f, TrackLength * 0.5f + 20.f, A), 2.f));
	}
	DollyAlpha = DefaultDolly;
	Dolly->SetRelativeLocation(FVector(0.f, FMath::Lerp(-TrackLength * 0.5f, TrackLength * 0.5f, DollyAlpha), 0.f));
}

void AFTFilmCamera::BeginPlay()
{
	Super::BeginPlay();
	VisualDolly = LocalDolly = DollyAlpha;
	VisualPan = LocalPan = Pan;
	VisualTilt = LocalTilt = Tilt;
	VisualZoom = LocalZoom = Zoom;
	if (GetNetMode() != NM_DedicatedServer)
	{
		FeedTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, 320, 180, RTF_RGBA8);
		Capture->TextureTarget = FeedTarget;
		if (UMaterialInterface* ScreenMat = FTVis::Screen())
		{
			MonitorMID = UMaterialInstanceDynamic::Create(ScreenMat, this);
			MonitorMID->SetTextureParameterValue(TEXT("Feed"), FeedTarget);
			MonitorScreen->SetMaterial(0, MonitorMID);
		}
	}
}

bool AFTFilmCamera::IsLocallyOperated() const
{
	return Operator && Operator->IsLocallyControlled();
}

bool AFTFilmCamera::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	if (Operator && Operator != User)
	{
		OutReason = FText::Format(LOCTEXT("InUse", "Camera is in use by {0}"), FText::FromString(Operator->GetCrewName()));
		return false;
	}
	if (User && User->HeldProp)
	{
		OutReason = LOCTEXT("PutDown", "Put down what you're carrying first (Q)");
		return false;
	}
	return true;
}

FText AFTFilmCamera::GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const
{
	return Operator == User ? LOCTEXT("Release", "Step off") : LOCTEXT("OperateVerb", "Operate");
}

void AFTFilmCamera::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	if (!User)
	{
		return;
	}
	if (Operator == User)
	{
		Release();
	}
	else if (!Operator)
	{
		Claim(User);
	}
}

void AFTFilmCamera::Claim(AFTCharacter* User)
{
	if (!HasAuthority() || !User)
	{
		return;
	}
	if (User->UsingActor && User->UsingActor != this)
	{
		User->StopUsing();
	}
	Operator = User;
	User->SetUsingActor(this);
	MulticastSound(EFTSound::Lever, GetActorLocation() + FVector(0.f, 0.f, 120.f), 0.6f, 1.3f);
	OnRep_Operator();
	ForceNetUpdate();
}

void AFTFilmCamera::Release(const FText& Reason)
{
	if (!HasAuthority() || !Operator)
	{
		return;
	}
	// A camera left while rolling keeps recording as a locked-off shot (solo crews rely on this).
	AFTCharacter* Old = Operator;
	Operator = nullptr;
	if (bRecording && Reason.IsEmpty())
	{
		Old->ClientFeedback(NSLOCTEXT("FinalTakeCamera", "LockedOff", "Locked-off shot: the camera keeps rolling. Go do the action!"));
	}
	if (Old->UsingActor == this)
	{
		Old->SetUsingActor(nullptr);
	}
	if (!Reason.IsEmpty())
	{
		Old->ClientFeedback(Reason);
	}
	OnRep_Operator();
	ForceNetUpdate();
}

void AFTFilmCamera::OnUserLeft(AFTCharacter* User)
{
	if (Operator == User)
	{
		Release();
	}
}

void AFTFilmCamera::ResetForNewShoot()
{
	Release();
	SetRecording(false);
	Pan = 0.f;
	Tilt = -4.f;
	Zoom = 0.35f;
	DollyAlpha = DefaultDolly;
	ForceNetUpdate();
}

void AFTFilmCamera::ApplyOperatorInput(float PanDelta, float TiltDelta, float ZoomDelta, float DollyAxis)
{
	const float DollyStep = DollyAxis * (240.f / FMath::Max(TrackLength, 1.f)) / 30.f;
	if (HasAuthority())
	{
		Pan = FMath::Clamp(Pan + PanDelta, -PanLimit, PanLimit);
		Tilt = FMath::Clamp(Tilt + TiltDelta, -TiltLimit, TiltLimit);
		Zoom = FMath::Clamp(Zoom + ZoomDelta, 0.f, 1.f);
		DollyAlpha = FMath::Clamp(DollyAlpha + DollyStep, 0.f, 1.f);
	}
	else
	{
		LocalPan = FMath::Clamp(LocalPan + PanDelta, -PanLimit, PanLimit);
		LocalTilt = FMath::Clamp(LocalTilt + TiltDelta, -TiltLimit, TiltLimit);
		LocalZoom = FMath::Clamp(LocalZoom + ZoomDelta, 0.f, 1.f);
		LocalDolly = FMath::Clamp(LocalDolly + DollyStep, 0.f, 1.f);
	}
}

void AFTFilmCamera::Recenter()
{
	if (HasAuthority())
	{
		Pan = 0.f;
		Tilt = -4.f;
		Zoom = 0.35f;
	}
	LocalPan = 0.f;
	LocalTilt = -4.f;
	LocalZoom = 0.35f;
}

void AFTFilmCamera::FrameTargets(const TArray<FVector>& Targets, float SubjectRadius)
{
	if (!HasAuthority() || Targets.Num() == 0)
	{
		return;
	}
	const FVector Eye = Lens->GetComponentLocation();
	float YawMin = 180.f, YawMax = -180.f, PitchMin = 90.f, PitchMax = -90.f;
	for (const FVector& T : Targets)
	{
		const FVector Local = GetActorTransform().InverseTransformVectorNoScale(T - Eye);
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X));
		const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(Local.Z, FVector2D(Local.X, Local.Y).Size()));
		// the subject's own size, as seen from the lens
		const float Pad = FMath::RadiansToDegrees(FMath::Atan2(SubjectRadius, FMath::Max(Local.Size(), 1.f)));
		YawMin = FMath::Min(YawMin, Yaw - Pad);
		YawMax = FMath::Max(YawMax, Yaw + Pad);
		PitchMin = FMath::Min(PitchMin, Pitch - Pad);
		PitchMax = FMath::Max(PitchMax, Pitch + Pad);
	}
	Pan = FMath::Clamp((YawMin + YawMax) * 0.5f, -PanLimit, PanLimit);
	Tilt = FMath::Clamp((PitchMin + PitchMax) * 0.5f, -TiltLimit, TiltLimit);
	// fit the padded spread into ~75% of the horizontal FOV (16:9 lens), never tighter than a medium shot
	const float Spread = FMath::Max(YawMax - YawMin, (PitchMax - PitchMin) * 16.f / 9.f);
	const float WantFOV = FMath::Clamp(Spread / 0.75f, FMath::Max(FOVRange.X, 40.f), FOVRange.Y);
	Zoom = FMath::Clamp((FOVRange.Y - WantFOV) / FMath::Max(FOVRange.Y - FOVRange.X, 1.f), 0.f, 1.f);
	LocalPan = Pan;
	LocalTilt = Tilt;
	LocalZoom = Zoom;
	ForceNetUpdate();
}

void AFTFilmCamera::SetRecording(bool bNewRecording)
{
	if (!HasAuthority() || bRecording == bNewRecording)
	{
		return;
	}
	bRecording = bNewRecording;
	OnRep_Recording();
	ForceNetUpdate();
}

void AFTFilmCamera::OnRep_Operator()
{
	if (Operator && !IsLocallyOperated())
	{
		// other machines: keep the predicted copy in sync for a clean hand-over later
		LocalPan = Pan;
		LocalTilt = Tilt;
		LocalZoom = Zoom;
		LocalDolly = DollyAlpha;
	}
	else if (Operator)
	{
		LocalPan = Pan;
		LocalTilt = Tilt;
		LocalZoom = Zoom;
		LocalDolly = DollyAlpha;
	}
	if (AFTPlayerState* PS = Operator ? Operator->GetPlayerState<AFTPlayerState>() : nullptr)
	{
		if (HasAuthority())
		{
			PS->bOnCamera = true;
		}
	}
	UpdateLocalView();
}

void AFTFilmCamera::OnRep_Recording()
{
	FTVis::SetGlow(RecLamp, bRecording ? 12.f : 0.2f);
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	FTAudio::PlayAt(this, EFTSound::RecBeep, RecLamp->GetComponentLocation(), 0.8f, bRecording ? 1.f : 0.8f);
	if (bRecording && !MotorAudio)
	{
		MotorAudio = FTAudio::Attach(TiltHead, EFTSound::CameraMotor, 0.35f);
	}
	else if (!bRecording && MotorAudio)
	{
		MotorAudio->Stop();
		MotorAudio->DestroyComponent();
		MotorAudio = nullptr;
	}
}

void AFTFilmCamera::UpdateLocalView()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}
	const bool bShould = IsLocallyOperated();
	if (bShould && !bLocalViewActive)
	{
		bLocalViewActive = true;
		PC->SetViewTargetWithBlend(this, 0.35f, VTBlend_EaseInOut, 2.f);
	}
	else if (!bShould && bLocalViewActive)
	{
		bLocalViewActive = false;
		if (APawn* P = PC->GetPawn())
		{
			PC->SetViewTargetWithBlend(P, 0.3f, VTBlend_EaseInOut, 2.f);
		}
	}
}

FTransform AFTFilmCamera::GetLensTransform() const
{
	return Lens->GetComponentTransform();
}

float AFTFilmCamera::GetFOV() const
{
	return Lens->FieldOfView;
}

float AFTFilmCamera::GetZoom01() const
{
	return IsLocallyOperated() && !HasAuthority() ? LocalZoom : Zoom;
}

UTextureRenderTarget2D* AFTFilmCamera::CaptureStill()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}
	UTextureRenderTarget2D* Still = UKismetRenderingLibrary::CreateRenderTarget2D(this, 480, 270, RTF_RGBA8);
	UTextureRenderTarget2D* Prev = Capture->TextureTarget;
	Capture->TextureTarget = Still;
	Capture->FOVAngle = Lens->FieldOfView;
	Capture->CaptureScene();
	Capture->TextureTarget = Prev;
	return Still;
}

void AFTFilmCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	float TPan = Pan, TTilt = Tilt, TZoom = Zoom, TDolly = DollyAlpha;
	if (IsLocallyOperated() && !HasAuthority())
	{
		TPan = LocalPan;
		TTilt = LocalTilt;
		TZoom = LocalZoom;
		TDolly = LocalDolly;
	}
	// fluid-head damping: the lens eases after the operator's aim, which reads as smooth, filmic moves
	const float HeadSpeed = IsLocallyOperated() ? 7.f : 6.f;
	VisualPan = FMath::FInterpTo(VisualPan, TPan, DeltaSeconds, HeadSpeed);
	VisualTilt = FMath::FInterpTo(VisualTilt, TTilt, DeltaSeconds, HeadSpeed);
	VisualZoom = FMath::FInterpTo(VisualZoom, TZoom, DeltaSeconds, 4.f);
	VisualDolly = FMath::FInterpTo(VisualDolly, TDolly, DeltaSeconds, 3.f);

	Dolly->SetRelativeLocation(FVector(0.f, FMath::Lerp(-TrackLength * 0.5f, TrackLength * 0.5f, VisualDolly), 0.f));
	PanHead->SetRelativeRotation(FRotator(0.f, VisualPan, 0.f));
	TiltHead->SetRelativeRotation(FRotator(VisualTilt, 0.f, 0.f));
	const float FOV = FMath::Lerp(FOVRange.Y, FOVRange.X, VisualZoom);
	Lens->SetFieldOfView(FOV);

	// autofocus pulls focus to whatever sits in the centre of frame (local view and monitor only)
	if (GetNetMode() != NM_DedicatedServer && (Operator || bRecording))
	{
		FocusTimer -= DeltaSeconds;
		if (FocusTimer <= 0.f)
		{
			FocusTimer = 0.1f;
			const FVector From = Lens->GetComponentLocation();
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(FTCameraFocus), false, this);
			FocusTarget = GetWorld()->LineTraceSingleByChannel(Hit, From, From + Lens->GetForwardVector() * 6000.f, ECC_Visibility, Params)
				? FMath::Clamp(Hit.Distance, 150.f, 6000.f) : 3000.f;
		}
		FocusDistance = FMath::FInterpTo(FocusDistance, FocusTarget, DeltaSeconds, 3.f);
		Lens->PostProcessSettings.DepthOfFieldFocalDistance = FocusDistance;
		Capture->PostProcessSettings.DepthOfFieldFocalDistance = FocusDistance;
	}

	if (bRecording)
	{
		ReelSpin += DeltaSeconds * 240.f;
		ReelA->SetRelativeRotation(FRotator(ReelSpin, 0.f, 90.f));
		ReelB->SetRelativeRotation(FRotator(-ReelSpin * 1.2f, 0.f, 90.f));
		const bool bBlink = FMath::Fmod(GetWorld()->GetTimeSeconds(), 1.f) < 0.6f;
		FTVis::SetGlow(RecLamp, bBlink ? 14.f : 1.f);
	}

	if (GetNetMode() != NM_DedicatedServer && FeedTarget && (Operator || bRecording))
	{
		FeedTimer -= DeltaSeconds;
		if (FeedTimer <= 0.f)
		{
			FeedTimer = 1.f / 8.f;
			Capture->FOVAngle = FOV;
			Capture->CaptureScene();
		}
	}

	if (HasAuthority() && Operator)
	{
		// operator wandered off (knocked, teleported...) -> free the camera
		if (!IsValid(Operator) || Operator->UsingActor != this || FVector::Dist(Operator->GetActorLocation(), Dolly->GetComponentLocation()) > 600.f)
		{
			Release(LOCTEXT("TooFarRelease", "You stepped away from the camera."));
		}
	}
}

#undef LOCTEXT_NAMESPACE
