#include "TheFinalTake/Production/FTShark.h"

#include "TheFinalTake/Core/FTVisuals.h"
#include "TheFinalTake/Characters/FTCharacter.h"
#include "TheFinalTake/FX/FTChunkyParticles.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Game/FTSceneManager.h"
#include "TheFinalTake/Interaction/FTInteractableComponent.h"
#include "TheFinalTake/World/FTFloodController.h"
#include "TheFinalTake/World/FTZone.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "FinalTakeShark"

using namespace FTColors;

namespace
{
	float Now(const UObject* Ctx)
	{
		const UWorld* W = Ctx->GetWorld();
		const AFTGameState* GS = W ? W->GetGameState<AFTGameState>() : nullptr;
		return GS ? GS->GetServerWorldTimeSeconds() : (W ? W->GetTimeSeconds() : 0.f);
	}
}

FFTSharkParts FFTSharkParts::Build(AActor* Owner, USceneComponent* Parent, float S)
{
	FFTSharkParts P;
	P.Root = Parent;
	auto Part = [&](const TCHAR* Name, USceneComponent* At, EFTShape Shape, FVector Loc, FVector Size, FLinearColor C, FRotator R = FRotator::ZeroRotator)
	{
		return FTVis::MakePart(Owner, At, Name, Shape, Loc * S, Size * S, C, R);
	};
	const FLinearColor Body = Hex(0x4F7FD9);
	const FLinearColor BodyDark = Hex(0x3A5FB0);
	Part(TEXT("SharkBody"), Parent, EFTShape::Ball, FVector(0.f, 0.f, 0.f), FVector(340.f, 160.f, 150.f), Body);
	Part(TEXT("SharkBelly"), Parent, EFTShape::Ball, FVector(20.f, 0.f, -36.f), FVector(290.f, 128.f, 86.f), White);
	Part(TEXT("SharkSnout"), Parent, EFTShape::Ball, FVector(150.f, 0.f, 16.f), FVector(130.f, 118.f, 96.f), Body);
	Part(TEXT("SharkTailStalk"), Parent, EFTShape::Ball, FVector(-160.f, 0.f, 4.f), FVector(150.f, 76.f, 76.f), Body);
	Part(TEXT("SharkTail"), Parent, EFTShape::Prism, FVector(-250.f, 0.f, 22.f), FVector(14.f, 110.f, 150.f), BodyDark, FRotator(-25.f, 90.f, 0.f));
	Part(TEXT("SharkDorsal"), Parent, EFTShape::Prism, FVector(-24.f, 0.f, 112.f), FVector(14.f, 120.f, 110.f), BodyDark, FRotator(0.f, 90.f, 0.f));
	Part(TEXT("SharkFinL"), Parent, EFTShape::Prism, FVector(50.f, -86.f, -40.f), FVector(10.f, 84.f, 60.f), BodyDark, FRotator(0.f, 70.f, -60.f));
	Part(TEXT("SharkFinR"), Parent, EFTShape::Prism, FVector(50.f, 86.f, -40.f), FVector(10.f, 84.f, 60.f), BodyDark, FRotator(0.f, 110.f, 60.f));
	for (int32 i = 0; i < 5; ++i)
	{
		const float Y = -40.f + i * 20.f;
		Part(*FString::Printf(TEXT("SharkToothU%d"), i), Parent, EFTShape::Cone, FVector(186.f - FMath::Abs(Y) * 0.4f, Y, -18.f), FVector(16.f, 16.f, 22.f), White, FRotator(180.f, 0.f, 0.f));
	}
	P.EyeL = Part(TEXT("SharkEyeL"), Parent, EFTShape::Ball, FVector(150.f, -50.f, 44.f), FVector(28.f, 18.f, 32.f), White);
	P.EyeR = Part(TEXT("SharkEyeR"), Parent, EFTShape::Ball, FVector(150.f, 50.f, 44.f), FVector(28.f, 18.f, 32.f), White);
	Part(TEXT("SharkPupilL"), Parent, EFTShape::Ball, FVector(160.f, -57.f, 46.f), FVector(14.f, 8.f, 18.f), Ink);
	Part(TEXT("SharkPupilR"), Parent, EFTShape::Ball, FVector(160.f, 57.f, 46.f), FVector(14.f, 8.f, 18.f), Ink);
	Part(TEXT("SharkBrowL"), Parent, EFTShape::Box, FVector(158.f, -50.f, 66.f), FVector(8.f, 30.f, 6.f), BodyDark, FRotator(0.f, 0.f, -15.f));
	Part(TEXT("SharkBrowR"), Parent, EFTShape::Box, FVector(158.f, 50.f, 66.f), FVector(8.f, 30.f, 6.f), BodyDark, FRotator(0.f, 0.f, 15.f));

	USceneComponent* Jaw = Owner->CreateDefaultSubobject<USceneComponent>(TEXT("SharkJaw"));
	Jaw->SetupAttachment(Parent);
	Jaw->SetRelativeLocation(FVector(90.f, 0.f, -34.f) * S);
	P.Jaw = Jaw;
	Part(TEXT("SharkJawBone"), Jaw, EFTShape::Ball, FVector(70.f, 0.f, -8.f), FVector(160.f, 104.f, 42.f), White);
	Part(TEXT("SharkMouth"), Jaw, EFTShape::Ball, FVector(64.f, 0.f, 6.f), FVector(128.f, 84.f, 28.f), Hex(0xC9304A));
	for (int32 i = 0; i < 4; ++i)
	{
		const float Y = -30.f + i * 20.f;
		Part(*FString::Printf(TEXT("SharkToothL%d"), i), Jaw, EFTShape::Cone, FVector(118.f - FMath::Abs(Y) * 0.4f, Y, 16.f), FVector(14.f, 14.f, 20.f), White);
	}
	return P;
}

// ============================================================================ rig

AFTSharkRig::AFTSharkRig()
{
	PrimaryActorTick.bCanEverTick = true;
	Tags = { TEXT("Device.SharkRig"), FTTags::SubjShark };
	RailMesh = FTVis::MakePart(this, Root, TEXT("Rail"), EFTShape::Box, FVector(-140.f, 0.f, 70.f), FVector(20.f, 1100.f, 16.f), Grey);
	Carriage = CreateDefaultSubobject<USceneComponent>(TEXT("Carriage"));
	Carriage->SetupAttachment(Root);
	FTVis::MakePart(this, Carriage, TEXT("Trolley"), EFTShape::Box, FVector(-140.f, 0.f, 84.f), FVector(50.f, 60.f, 24.f), Yellow);
	SharkRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SharkRoot"));
	SharkRoot->SetupAttachment(Carriage);
	FTVis::MakePart(this, SharkRoot, TEXT("Arm"), EFTShape::Cylinder, FVector(-110.f, 0.f, 60.f), FVector(14.f, 14.f, 150.f), GreyDark, FRotator(-30.f, 0.f, 0.f));
	const FFTSharkParts P = FFTSharkParts::Build(this, SharkRoot, 1.f);
	Jaw = P.Jaw;
	EyeL = P.EyeL;
	EyeR = P.EyeR;

	Splash = CreateDefaultSubobject<UFTChunkyParticles>(TEXT("Splash"));
	Splash->SetupAttachment(Root);
	Splash->MaxParticles = 120;
	Splash->SpawnRate = 0.f;
	Splash->Lifetime = FVector2D(0.6f, 1.1f);
	Splash->SpawnExtent = FVector(60.f, 60.f, 5.f);
	Splash->BaseVelocity = FVector(0.f, 0.f, 520.f);
	Splash->VelocityJitter = FVector(260.f, 260.f, 200.f);
	Splash->Gravity = 1100.f;
	Splash->StartSize = FVector2D(12.f, 26.f);

	Station = CreateDefaultSubobject<USceneComponent>(TEXT("Station"));
	Station->SetupAttachment(Root);
	FTVis::MakePart(this, Station, TEXT("Desk"), EFTShape::Box, FVector(0.f, 0.f, 45.f), FVector(70.f, 220.f, 90.f), TealDark, FRotator::ZeroRotator, 0.f, true);
	FTVis::MakePart(this, Station, TEXT("DeskTop"), EFTShape::Box, FVector(-4.f, 0.f, 95.f), FVector(66.f, 214.f, 8.f), Charcoal, FRotator(-15.f, 0.f, 0.f));
	FTVis::MakeText(this, Station, TEXT("StationTitle"), LOCTEXT("RigTitle", "SHARK RIG"), FVector(-24.f, 0.f, 130.f), FRotator(0.f, 180.f, 0.f), 16.f, FColor(130, 200, 255));
	const FText Names[5] = { LOCTEXT("Raise", "RAISE"), LOCTEXT("Left", "LEFT"), LOCTEXT("Right", "RIGHT"), LOCTEXT("Lunge", "LUNGE"), LOCTEXT("Reset", "RESET") };
	const FLinearColor Cols[5] = { Teal, Cyan, Cyan, Red, Grey };
	for (int32 i = 0; i < 5; ++i)
	{
		const float Y = -84.f + i * 42.f;
		UStaticMeshComponent* Cap = FTVis::MakePart(this, Station, *FString::Printf(TEXT("RigButton%d"), i), i == 3 ? EFTShape::Cylinder : EFTShape::Box, FVector(-4.f, Y, 104.f), i == 3 ? FVector(34.f, 34.f, 14.f) : FVector(22.f, 30.f, 10.f), Cols[i], FRotator(-15.f, 0.f, 0.f), 0.8f);
		ButtonCaps.Add(Cap);
		FTVis::MakeText(this, Station, *FString::Printf(TEXT("RigLabel%d"), i), Names[i], FVector(-30.f, Y, 112.f), FRotator(0.f, 180.f, 0.f), 8.f, FColor::White);
		UFTInteractableComponent* I = CreateDefaultSubobject<UFTInteractableComponent>(*FString::Printf(TEXT("RigControl%d"), i));
		I->SetupAttachment(Station);
		I->Setup(*FString::Printf(TEXT("Rig%d"), i), FText::Format(LOCTEXT("RigCtl", "Shark rig {0}"), Names[i]), Names[i], EFTInteractType::Press, FVector(16.f, 19.f, 14.f));
		I->SetRelativeLocation(FVector(-4.f, Y, 106.f));
		I->AddHighlight(Cap);
		Buttons.Add(I);
	}
}

void AFTSharkRig::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTSharkRig, State);
	DOREPLIFETIME(AFTSharkRig, StateTime);
	DOREPLIFETIME(AFTSharkRig, RailTarget);
}

void AFTSharkRig::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Station->SetRelativeLocation(StationOffset);
	Station->SetRelativeRotation(FRotator(0.f, StationYaw, 0.f));
	SharkRoot->SetRelativeLocation(FVector(0.f, 0.f, WaterHeight - 150.f));
	// the rail is only as long as the carriage travel, so it never pokes into the island or the dock
	RailMesh->SetRelativeScale3D(FVector(0.2f, (RailHalfLength * 2.f + 100.f) / 100.f, 0.16f));
}

bool AFTSharkRig::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	if (State == EFTSharkState::Telegraph || State == EFTSharkState::Lunging || State == EFTSharkState::Defeated)
	{
		OutReason = LOCTEXT("Busy", "The shark is mid-performance!");
		return false;
	}
	return true;
}

FText AFTSharkRig::GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const
{
	if (Buttons.IsValidIndex(0) && Comp == Buttons[0])
	{
		return State == EFTSharkState::Submerged ? LOCTEXT("RaiseVerb", "Raise shark") : LOCTEXT("LowerVerb", "Lower shark");
	}
	return Comp ? Comp->Verb : FText::GetEmpty();
}

void AFTSharkRig::SetState(EFTSharkState NewState)
{
	State = NewState;
	StateTime = Now(this);
	bLungeResolved = false;
	ForceNetUpdate();
}

void AFTSharkRig::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	const int32 Index = Buttons.IndexOfByKey(Comp);
	LastOperator = User;
	MulticastSound(EFTSound::Lever, Comp->GetComponentLocation(), 0.6f, 1.2f);
	switch (Index)
	{
	case 0:
		SetState(State == EFTSharkState::Submerged ? EFTSharkState::Raised : EFTSharkState::Submerged);
		MulticastSplash(SharkRoot->GetComponentLocation() + FVector(0.f, 0.f, 60.f), 0.6f);
		break;
	case 1: RailTarget = FMath::Clamp(RailTarget - 0.4f, -1.f, 1.f); break;
	case 2: RailTarget = FMath::Clamp(RailTarget + 0.4f, -1.f, 1.f); break;
	case 3: StartLunge(User); break;
	case 4:
		RailTarget = 0.f;
		SetState(EFTSharkState::Submerged);
		break;
	default: break;
	}
	ForceNetUpdate();
}

void AFTSharkRig::StartLunge(AFTCharacter* User)
{
	LastOperator = User;
	if (State == EFTSharkState::Submerged)
	{
		SetState(EFTSharkState::Raised);
		AutoTimer = 0.9f; // lunge right after surfacing
		return;
	}
	SetState(EFTSharkState::Telegraph);
	MulticastSound(EFTSound::SharkChomp, SharkRoot->GetComponentLocation(), 0.7f, 0.8f);
}

void AFTSharkRig::OnSceneEvent(FName Event, AActor* Source)
{
	if (Event == TEXT("Director.Defeat"))
	{
		bDefeatQueued = true;
	}
	else if (Event == TEXT("Director.AutoLunge") && (State == EFTSharkState::Submerged || State == EFTSharkState::Raised))
	{
		StartLunge(nullptr);
	}
}

void AFTSharkRig::ResetForNewShoot()
{
	RailTarget = 0.f;
	bDefeatQueued = false;
	AutoTimer = 0.f;
	SetState(EFTSharkState::Submerged);
}

void AFTSharkRig::MulticastSplash_Implementation(FVector_NetQuantize Where, float Size)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	Splash->Configure(EFTShape::Sphere, Hex(0xBFEFFF), 0.4f, false);
	Splash->FloorZ = GetActorLocation().Z + WaterHeight - 10.f;
	Splash->BurstAt(Where, FMath::RoundToInt(40 * Size), Size);
	FTAudio::PlayAt(this, EFTSound::Splash, Where, FMath::Clamp(Size, 0.4f, 1.f));
}

bool AFTSharkRig::GetSubjectBounds(FName SubjectTag, FVector& OutCenter, float& OutRadius) const
{
	OutCenter = SharkRoot->GetComponentLocation() + FVector(0.f, 0.f, 40.f);
	OutRadius = 200.f;
	return true;
}

void AFTSharkRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float T = Now(this) - StateTime;

	// server state machine
	if (HasAuthority())
	{
		if (AutoTimer > 0.f)
		{
			AutoTimer -= DeltaSeconds;
			if (AutoTimer <= 0.f && State == EFTSharkState::Raised)
			{
				SetState(EFTSharkState::Telegraph);
				MulticastSound(EFTSound::SharkChomp, SharkRoot->GetComponentLocation(), 0.7f, 0.8f);
			}
		}
		if (State == EFTSharkState::Telegraph && T > 0.9f)
		{
			SetState(EFTSharkState::Lunging);
			MulticastSound(EFTSound::Whoosh, SharkRoot->GetComponentLocation(), 1.f, 0.7f);
		}
		else if (State == EFTSharkState::Lunging)
		{
			if (!bLungeResolved && T > 0.45f)
			{
				bLungeResolved = true;
				const FVector Mouth = SharkRoot->GetComponentLocation() + SharkRoot->GetForwardVector() * 180.f;
				MulticastSplash(FVector(Mouth.X, Mouth.Y, GetActorLocation().Z + WaterHeight), 1.2f);
				MulticastSound(EFTSound::SharkChomp, Mouth, 1.f, 1.f);
				for (TActorIterator<AFTCharacter> It(GetWorld()); It; ++It)
				{
					if (*It != LastOperator && FVector::Dist2D(It->GetActorLocation(), Mouth) < 320.f && AFTZone::IsInZone(this, FTTags::ZoneTank, It->GetActorLocation()))
					{
						It->Knockdown(Mouth, 1.2f, true);
					}
				}
				ReportEvent(FTTags::EvRigLunge, LastOperator);
			}
			if (T > 1.6f)
			{
				if (bDefeatQueued)
				{
					bDefeatQueued = false;
					SetState(EFTSharkState::Defeated);
					MulticastSound(EFTSound::Boing, SharkRoot->GetComponentLocation(), 1.f, 0.6f);
				}
				else
				{
					SetState(EFTSharkState::Raised);
				}
			}
		}
		else if (State == EFTSharkState::Defeated && T > DefeatedDuration)
		{
			SetState(EFTSharkState::Submerged);
		}
	}

	// visuals (every machine)
	RailPos = FMath::FInterpTo(RailPos, RailTarget, DeltaSeconds, 1.6f);
	Carriage->SetRelativeLocation(FVector(0.f, RailPos * RailHalfLength, 0.f));
	float TargetZ = WaterHeight - 150.f;
	float Forward = 0.f, JawOpen = 0.f, Roll = 0.f, Pitch = 0.f;
	const float Time = GetWorld()->GetTimeSeconds();
	switch (State)
	{
	case EFTSharkState::Raised:
		TargetZ = WaterHeight + 8.f + FMath::Sin(Time * 1.6f) * 6.f;
		JawOpen = 8.f + FMath::Abs(FMath::Sin(Time * 0.9f)) * 10.f;
		Pitch = -8.f;
		break;
	case EFTSharkState::Telegraph:
		TargetZ = WaterHeight + 20.f;
		JawOpen = 25.f;
		Roll = FMath::Sin(Time * 40.f) * 5.f;
		Forward = -30.f * FMath::Min(T / 0.9f, 1.f);
		Pitch = -14.f;
		break;
	case EFTSharkState::Lunging:
	{
		const float A = FMath::Clamp(T / 0.45f, 0.f, 1.f);
		const float Back = FMath::Clamp((T - 0.75f) / 0.85f, 0.f, 1.f);
		const float K = FMath::Sin(A * PI * 0.5f) * (1.f - Back);
		TargetZ = WaterHeight + 8.f + 95.f * K;
		Forward = LungeReach * K;
		JawOpen = 40.f * (T < 0.5f ? 1.f : 1.f - Back);
		Pitch = -22.f * K;
		break;
	}
	case EFTSharkState::Defeated:
		// slow, dramatic sink that stays in frame long enough for the 3 s hold after the killing blow
		TargetZ = FMath::Lerp(WaterHeight + 40.f, WaterHeight - 150.f, FMath::Clamp(FMath::Square(T / DefeatedDuration), 0.f, 1.f));
		Roll = FMath::Min(T / 0.8f, 1.f) * 165.f;
		JawOpen = 30.f;
		break;
	default:
		break;
	}
	const bool bSnap = State == EFTSharkState::Lunging;
	VisualZ = bSnap ? TargetZ : FMath::FInterpTo(VisualZ, TargetZ, DeltaSeconds, 3.f);
	SharkRoot->SetRelativeLocation(FVector(Forward, 0.f, VisualZ));
	SharkRoot->SetRelativeRotation(FRotator(Pitch, 0.f, Roll));
	Jaw->SetRelativeRotation(FRotator(-JawOpen, 0.f, 0.f));
	const float Squint = State == EFTSharkState::Defeated ? 0.15f : 1.f;
	EyeL->SetRelativeScale3D(FVector(0.28f, 0.18f, 0.32f * Squint));
	EyeR->SetRelativeScale3D(FVector(0.28f, 0.18f, 0.32f * Squint));
	if (State != LastVisualState)
	{
		LastVisualState = State;
		for (int32 i = 0; i < ButtonCaps.Num(); ++i)
		{
			FTVis::SetGlow(ButtonCaps[i], i == 0 && State != EFTSharkState::Submerged ? 6.f : 0.8f);
		}
	}
}

// ============================================================================ flood hazard

AFTSharkHazard::AFTSharkHazard()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicatingMovement(true);
	SetNetUpdateFrequency(20.f);
	Body = CreateDefaultSubobject<USceneComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	const FFTSharkParts P = FFTSharkParts::Build(this, Body, 0.8f);
	Jaw = P.Jaw;
	Warning = FTVis::MakeText(this, Root, TEXT("Warning"), FText::FromString(TEXT("!")), FVector(0.f, 0.f, 220.f), FRotator::ZeroRotator, 140.f, FColor(255, 60, 50));
	Warning->SetVisibility(false);
	Wake = CreateDefaultSubobject<UFTChunkyParticles>(TEXT("Wake"));
	Wake->SetupAttachment(Root);
	Wake->SetRelativeLocation(FVector(-60.f, 0.f, 0.f));
	Wake->SpawnRate = 25.f;
	Wake->MaxParticles = 70;
	Wake->Lifetime = FVector2D(0.7f, 1.2f);
	Wake->BaseVelocity = FVector(-40.f, 0.f, 60.f);
	Wake->VelocityJitter = FVector(40.f, 60.f, 30.f);
	Wake->Gravity = 300.f;
	Wake->StartSize = FVector2D(10.f, 18.f);
	Patrol = { FVector(0.f, -1100.f, 0.f), FVector(500.f, -1100.f, 0.f), FVector(500.f, 1100.f, 0.f), FVector(0.f, 1100.f, 0.f) };
	Body->SetVisibility(false, true);
}

void AFTSharkHazard::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTSharkHazard, State);
	DOREPLIFETIME(AFTSharkHazard, StateTime);
}

void AFTSharkHazard::BeginPlay()
{
	Super::BeginPlay();
	StartTransform = GetActorTransform();
	Wake->Configure(EFTShape::Sphere, Hex(0xCFF3FF), 0.3f, false);
}

bool AFTSharkHazard::IsSwimmable(const FVector& From, const FVector& To, float WaterZ) const
{
	const UWorld* World = GetWorld();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FTSharkSwim), false, this);
	const FCollisionObjectQueryParams Statics(ECC_WorldStatic);
	const FVector A(From.X, From.Y, WaterZ - 25.f);
	const FVector B(To.X, To.Y, WaterZ - 25.f);
	FHitResult Hit;
	// the fin must not pass through walls, desks, stairs or other set pieces
	if (!A.Equals(B, 1.f) && World->SweepSingleByObjectType(Hit, A, B, FQuat::Identity, Statics, FCollisionShape::MakeSphere(40.f), Params))
	{
		return false;
	}
	// ...and there must be real depth under it (no gliding over the island or the stage landing)
	if (World->LineTraceSingleByObjectType(Hit, B + FVector(0.f, 0.f, 60.f), B - FVector(0.f, 0.f, 400.f), Statics, Params) && Hit.ImpactPoint.Z > WaterZ - 45.f)
	{
		return false;
	}
	return true;
}

void AFTSharkHazard::ResetForNewShoot()
{
	SetActorTransform(StartTransform);
	State = EFTSharkState::Submerged;
	StateTime = Now(this);
	Target.Reset();
	Cooldown = 5.f;
	PatrolIndex = 0;
}

void AFTSharkHazard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AFTGameState* GS = GetWorld()->GetGameState<AFTGameState>();
	AFTFloodController* Flood = AFTFloodController::Get(this);
	const bool bActive = GS && Flood && GS->FloodStage == EFTFloodStage::Flooded
		&& GS->ShootPhase != EFTShootPhase::Results && GS->ShootPhase != EFTShootPhase::Premiere && GS->ShootPhase != EFTShootPhase::Failed;
	if (Body->IsVisible() != bActive)
	{
		Body->SetVisibility(bActive, true);
		Wake->SetEmitting(bActive);
	}
	if (!bActive)
	{
		return;
	}
	const float WaterZ = Flood->GetWaterZ();
	const float T = Now(this) - StateTime;

	if (HasAuthority())
	{
		FVector Loc = GetActorLocation();
		Loc.Z = WaterZ - 48.f;
		switch (State)
		{
		case EFTSharkState::Submerged:
		{
			const FVector Goal = StartTransform.TransformPosition(Patrol.IsValidIndex(PatrolIndex) ? Patrol[PatrolIndex] : FVector::ZeroVector);
			FVector To = Goal - Loc;
			To.Z = 0.f;
			if (To.Size() < 80.f)
			{
				PatrolIndex = (PatrolIndex + 1) % FMath::Max(1, Patrol.Num());
			}
			const FVector Dir = To.GetSafeNormal();
			const FVector Next = Loc + Dir * Speed * DeltaSeconds;
			if (IsSwimmable(Loc, Next, WaterZ))
			{
				Loc = Next;
			}
			else
			{
				// blocked by a set piece or shallow ground: head for the next waypoint instead of pushing through
				PatrolIndex = (PatrolIndex + 1) % FMath::Max(1, Patrol.Num());
			}
			SetActorLocation(Loc);
			if (!Dir.IsNearlyZero())
			{
				SetActorRotation(FMath::RInterpTo(GetActorRotation(), Dir.Rotation(), DeltaSeconds, 3.f));
			}
			Cooldown -= DeltaSeconds;
			if (Cooldown <= 0.f)
			{
				AFTCharacter* Best = nullptr;
				float BestDist = 900.f;
				for (TActorIterator<AFTCharacter> It(GetWorld()); It; ++It)
				{
					const float D = FVector::Dist2D(It->GetActorLocation(), Loc);
					if (It->IsInWater() && !It->bKnockedDown && D < BestDist && !AFTZone::IsInZone(this, FTTags::ZoneSafe, It->GetActorLocation())
						&& IsSwimmable(Loc, It->GetActorLocation(), WaterZ))
					{
						Best = *It;
						BestDist = D;
					}
				}
				if (Best)
				{
					Target = Best;
					State = EFTSharkState::Telegraph;
					StateTime = Now(this);
					MulticastSound(EFTSound::MonsterSting, Loc, 0.6f, 1.3f);
				}
				else
				{
					Cooldown = 1.f;
				}
			}
			break;
		}
		case EFTSharkState::Telegraph:
		{
			AFTCharacter* C = Target.Get();
			if (!C)
			{
				State = EFTSharkState::Submerged;
				Cooldown = 3.f;
				break;
			}
			const FVector To = (C->GetActorLocation() - Loc).GetSafeNormal2D();
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), To.Rotation(), DeltaSeconds, 6.f));
			SetActorLocation(Loc);
			if (T > 1.3f)
			{
				LungeFrom = Loc;
				LungeTo = FVector(C->GetActorLocation().X, C->GetActorLocation().Y, Loc.Z);
				State = EFTSharkState::Lunging;
				StateTime = Now(this);
				bHit = false;
				MulticastSound(EFTSound::Whoosh, Loc, 1.f, 0.6f);
			}
			break;
		}
		case EFTSharkState::Lunging:
		{
			const float A = FMath::Clamp(T / 0.7f, 0.f, 1.f);
			FVector P = FMath::Lerp(LungeFrom, LungeTo, FMath::SmoothStep(0.f, 1.f, A));
			P.Z = WaterZ - 48.f;
			if (!IsSwimmable(Loc, P, WaterZ))
			{
				// the target moved behind something solid: stop at the obstacle instead of clipping through it
				LungeFrom = Loc;
				LungeTo = Loc;
				P = Loc;
			}
			SetActorLocation(P);
			if (!bHit)
			{
				for (TActorIterator<AFTCharacter> It(GetWorld()); It; ++It)
				{
					if (FVector::Dist2D(It->GetActorLocation(), P) < 170.f && It->IsInWater() && !AFTZone::IsInZone(this, FTTags::ZoneSafe, It->GetActorLocation()))
					{
						bHit = true;
						It->Knockdown(P, 1.4f, true);
						MulticastSound(EFTSound::SharkChomp, P, 1.f, 1.f);
						if (AFTSceneManager* SM = GetSceneManager())
						{
							SM->AdjustCondition(-2.f, LOCTEXT("Bitten", "The flood shark bumped the crew"));
						}
					}
				}
			}
			if (A >= 1.f)
			{
				State = EFTSharkState::Raised;
				StateTime = Now(this);
			}
			break;
		}
		case EFTSharkState::Raised:
			SetActorLocation(Loc);
			if (T > 1.2f)
			{
				State = EFTSharkState::Submerged;
				StateTime = Now(this);
				Cooldown = FMath::FRandRange(6.f, 9.f);
			}
			break;
		default:
			State = EFTSharkState::Submerged;
			break;
		}
	}

	// visuals
	float Rise = 0.f, JawOpen = 0.f;
	if (State == EFTSharkState::Lunging)
	{
		const float A = FMath::Clamp(T / 0.7f, 0.f, 1.f);
		Rise = FMath::Sin(A * PI) * 70.f;
		JawOpen = 35.f;
	}
	else if (State == EFTSharkState::Telegraph)
	{
		Rise = 12.f;
		JawOpen = 12.f;
	}
	Body->SetRelativeLocation(FVector(0.f, 0.f, Rise));
	Body->SetRelativeRotation(FRotator(-Rise * 0.2f, 0.f, FMath::Sin(GetWorld()->GetTimeSeconds() * 2.f) * 4.f));
	Jaw->SetRelativeRotation(FRotator(-JawOpen, 0.f, 0.f));
	const bool bWarn = State == EFTSharkState::Telegraph;
	Warning->SetVisibility(bWarn);
	if (bWarn)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (PC->PlayerCameraManager)
			{
				const FVector ToCam = PC->PlayerCameraManager->GetCameraLocation() - Warning->GetComponentLocation();
				Warning->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
