#include "TheFinalTake/Production/FTStations.h"

#include "TheFinalTake/Core/FTVisuals.h"
#include "TheFinalTake/Characters/FTCharacter.h"
#include "TheFinalTake/FX/FTChunkyParticles.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Game/FTSceneManager.h"
#include "TheFinalTake/Interaction/FTInteractableComponent.h"

#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "FinalTakeStations"

using namespace FTColors;

namespace
{
	float ServerNow(const UObject* Ctx)
	{
		const UWorld* W = Ctx ? Ctx->GetWorld() : nullptr;
		const AFTGameState* GS = W ? W->GetGameState<AFTGameState>() : nullptr;
		return GS ? GS->GetServerWorldTimeSeconds() : (W ? W->GetTimeSeconds() : 0.f);
	}

	const AFTGameState* GameStateOf(const UObject* Ctx)
	{
		const UWorld* W = Ctx ? Ctx->GetWorld() : nullptr;
		return W ? W->GetGameState<AFTGameState>() : nullptr;
	}

	UStaticMeshComponent* GlowCone(AActor* Owner, USceneComponent* Parent, FName Name, float Length, float Width, const FLinearColor& Color, float Strength)
	{
		// tip at the parent origin, opening along +X
		UStaticMeshComponent* C = FTVis::MakePart(Owner, Parent, Name, EFTShape::Cone, FVector(Length * 0.5f, 0.f, 0.f), FVector(Width, Width, Length), Color, FRotator(90.f, 0.f, 0.f), Strength);
		C->SetMaterial(0, FTVis::Glow());
		C->SetCastShadow(false);
		return C;
	}
}

// ============================================================================ lighthouse

AFTLighthouse::AFTLighthouse()
{
	PrimaryActorTick.bCanEverTick = true;
	Tags = { FTTags::DevLighthouse, FTTags::SubjLighthouse };
	FTVis::MakePart(this, Root, TEXT("Base"), EFTShape::Box, FVector(0.f, 0.f, 18.f), FVector(150.f, 150.f, 36.f), GreyDark, FRotator::ZeroRotator, 0.f, true);
	const float Diam[4] = { 112.f, 100.f, 90.f, 80.f };
	for (int32 i = 0; i < 4; ++i)
	{
		FTVis::MakePart(this, Root, *FString::Printf(TEXT("Seg%d"), i), EFTShape::Cylinder, FVector(0.f, 0.f, 76.f + i * 80.f), FVector(Diam[i], Diam[i], 80.f), i % 2 ? Red : White, FRotator::ZeroRotator, 0.f, i == 0);
	}
	FTVis::MakePart(this, Root, TEXT("Door"), EFTShape::Box, FVector(54.f, 0.f, 70.f), FVector(8.f, 34.f, 62.f), WoodDark);
	FTVis::MakePart(this, Root, TEXT("Window"), EFTShape::Box, FVector(46.f, 0.f, 240.f), FVector(6.f, 20.f, 28.f), Navy);
	FTVis::MakePart(this, Root, TEXT("Gallery"), EFTShape::Cylinder, FVector(0.f, 0.f, 342.f), FVector(122.f, 122.f, 10.f), Charcoal);
	FTVis::MakePart(this, Root, TEXT("Railing"), EFTShape::Torus, FVector(0.f, 0.f, 364.f), FVector(122.f, 122.f, 6.f), Grey);
	FTVis::MakePart(this, Root, TEXT("LanternGlass"), EFTShape::Cylinder, FVector(0.f, 0.f, 378.f), FVector(70.f, 70.f, 60.f), Cream, FRotator::ZeroRotator, 0.2f);
	Lamp = FTVis::MakePart(this, Root, TEXT("Lamp"), EFTShape::Ball, FVector(0.f, 0.f, 378.f), FVector(44.f), Yellow, FRotator::ZeroRotator, 0.3f);
	FTVis::MakePart(this, Root, TEXT("Roof"), EFTShape::Cone, FVector(0.f, 0.f, 432.f), FVector(96.f, 96.f, 52.f), Red);
	FTVis::MakePart(this, Root, TEXT("Tip"), EFTShape::Sphere, FVector(0.f, 0.f, 462.f), FVector(16.f), Charcoal);

	Beam = CreateDefaultSubobject<USceneComponent>(TEXT("Beam"));
	Beam->SetupAttachment(Root);
	Beam->SetRelativeLocation(FVector(0.f, 0.f, 378.f));
	BeamCone = GlowCone(this, Beam, TEXT("BeamCone"), 1400.f, 220.f, Hex(0xFFE08A), 0.35f);
	BeamLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("BeamLight"));
	BeamLight->SetupAttachment(Beam);
	BeamLight->SetIntensity(40000.f);
	BeamLight->SetInnerConeAngle(6.f);
	BeamLight->SetOuterConeAngle(14.f);
	BeamLight->SetAttenuationRadius(4500.f);
	BeamLight->SetLightColor(FLinearColor(1.f, 0.85f, 0.55f));
	BeamLight->SetCastShadows(false);
	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Beam);
	Glow->SetIntensity(3000.f);
	Glow->SetAttenuationRadius(500.f);
	Glow->SetLightColor(FLinearColor(1.f, 0.8f, 0.45f));
	Glow->SetCastShadows(false);

	FTVis::MakePart(this, Root, TEXT("SwitchBox"), EFTShape::Box, FVector(70.f, 70.f, 50.f), FVector(28.f, 28.f, 40.f), Charcoal);
	FTVis::MakePart(this, Root, TEXT("SwitchPlate"), EFTShape::Box, FVector(85.f, 70.f, 50.f), FVector(3.f, 22.f, 30.f), Yellow);
	Lever = FTVis::MakePart(this, Root, TEXT("Lever"), EFTShape::Cylinder, FVector(90.f, 70.f, 58.f), FVector(6.f, 6.f, 30.f), Red, FRotator(-35.f, 0.f, 0.f));
	FTVis::MakeText(this, Root, TEXT("SwitchLabel"), LOCTEXT("LHLabel", "LIGHTHOUSE"), FVector(88.f, 70.f, 80.f), FRotator::ZeroRotator, 9.f, FColor(255, 230, 150));
	Switch = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("Switch"));
	Switch->SetupAttachment(Root);
	Switch->Setup(TEXT("Toggle"), LOCTEXT("LHSwitch", "Lighthouse"), LOCTEXT("SwitchOn", "Switch on"), EFTInteractType::Press, FVector(30.f, 30.f, 40.f));
	Switch->SetRelativeLocation(FVector(80.f, 70.f, 55.f));
	Switch->AddHighlight(Lever);
}

void AFTLighthouse::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTLighthouse, bOn);
}

FText AFTLighthouse::GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const
{
	return bOn ? LOCTEXT("SwitchOff", "Switch off") : LOCTEXT("SwitchOn2", "Switch on");
}

void AFTLighthouse::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	bOn = !bOn;
	OnRep_On();
	MulticastSound(EFTSound::Lever, Lever->GetComponentLocation(), 0.9f, 1.f);
	if (bOn)
	{
		ReportEvent(TEXT("Light.Lighthouse"), User);
	}
	ForceNetUpdate();
}

void AFTLighthouse::ResetForNewShoot()
{
	bOn = false;
	OnRep_On();
}

void AFTLighthouse::OnRep_On()
{
	BeamLight->SetVisibility(bOn);
	Glow->SetVisibility(bOn);
	BeamCone->SetVisibility(bOn);
	FTVis::SetGlow(Lamp, bOn ? 18.f : 0.3f);
	Lever->SetRelativeRotation(FRotator(bOn ? 35.f : -35.f, 0.f, 0.f));
}

void AFTLighthouse::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!BeamLight->IsVisible() && bOn)
	{
		OnRep_On();
	}
	if (bOn)
	{
		Beam->AddLocalRotation(FRotator(0.f, 70.f * DeltaSeconds, 0.f));
	}
}

bool AFTLighthouse::GetSubjectBounds(FName SubjectTag, FVector& OutCenter, float& OutRadius) const
{
	OutCenter = GetActorLocation() + FVector(0.f, 0.f, 230.f);
	OutRadius = 200.f;
	return true;
}

// ============================================================================ stage light

AFTStageLight::AFTStageLight()
{
	for (int32 i = 0; i < 3; ++i)
	{
		const float A = i * 120.f;
		FTVis::MakePart(this, Root, *FString::Printf(TEXT("Leg%d"), i), EFTShape::Cylinder, FRotator(0.f, A, 0.f).Vector() * 24.f + FVector(0.f, 0.f, 26.f), FVector(5.f, 5.f, 62.f), Charcoal, FRotator(35.f, A, 0.f));
	}
	Pole = FTVis::MakePart(this, Root, TEXT("Pole"), EFTShape::Cylinder, FVector(0.f, 0.f, 130.f), FVector(6.f, 6.f, 220.f), Charcoal);
	Collar = FTVis::MakePart(this, Root, TEXT("Collar"), EFTShape::Cylinder, FVector(0.f, 0.f, 60.f), FVector(12.f, 12.f, 8.f), Amber);
	Head = CreateDefaultSubobject<USceneComponent>(TEXT("Head"));
	Head->SetupAttachment(Root);
	Head->SetRelativeLocation(FVector(0.f, 0.f, 240.f));
	FTVis::MakePart(this, Head, TEXT("Yoke"), EFTShape::Box, FVector(0.f, 0.f, -24.f), FVector(10.f, 48.f, 8.f), Charcoal);
	FTVis::MakePart(this, Head, TEXT("Body"), EFTShape::Box, FVector(0.f, 0.f, 0.f), FVector(46.f, 40.f, 40.f), Charcoal);
	FTVis::MakePart(this, Head, TEXT("Rim"), EFTShape::Cylinder, FVector(24.f, 0.f, 0.f), FVector(38.f, 38.f, 6.f), Amber, FRotator(-90.f, 0.f, 0.f));
	LensPart = FTVis::MakePart(this, Head, TEXT("LensPart"), EFTShape::Cylinder, FVector(27.f, 0.f, 0.f), FVector(32.f, 32.f, 3.f), Amber, FRotator(-90.f, 0.f, 0.f), 0.5f);
	FTVis::MakePart(this, Head, TEXT("DoorTop"), EFTShape::Box, FVector(36.f, 0.f, 24.f), FVector(22.f, 40.f, 2.f), Charcoal, FRotator(-30.f, 0.f, 0.f));
	FTVis::MakePart(this, Head, TEXT("DoorBottom"), EFTShape::Box, FVector(36.f, 0.f, -24.f), FVector(22.f, 40.f, 2.f), Charcoal, FRotator(30.f, 0.f, 0.f));
	FTVis::MakePart(this, Head, TEXT("DoorLeft"), EFTShape::Box, FVector(36.f, -24.f, 0.f), FVector(22.f, 2.f, 40.f), Charcoal, FRotator(0.f, 30.f, 0.f));
	FTVis::MakePart(this, Head, TEXT("DoorRight"), EFTShape::Box, FVector(36.f, 24.f, 0.f), FVector(22.f, 2.f, 40.f), Charcoal, FRotator(0.f, -30.f, 0.f));
	Spot = CreateDefaultSubobject<USpotLightComponent>(TEXT("Spot"));
	Spot->SetupAttachment(Head);
	Spot->SetRelativeLocation(FVector(30.f, 0.f, 0.f));
	Spot->SetInnerConeAngle(14.f);
	Spot->SetOuterConeAngle(26.f);
	Spot->SetAttenuationRadius(3500.f);
	Spot->SetCastShadows(true);
	Cone = GlowCone(this, Head, TEXT("Cone"), 650.f, 260.f, Amber, 0.12f);
}

void AFTStageLight::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTStageLight, bOn);
	DOREPLIFETIME(AFTStageLight, ColorIndex);
}

void AFTStageLight::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Tags = { *FString::Printf(TEXT("Device.Light.%d"), Circuit) };
	FTVis::ApplyShape(Pole, EFTShape::Cylinder, FVector(6.f, 6.f, StandHeight));
	Pole->SetRelativeLocation(FVector(0.f, 0.f, 20.f + StandHeight * 0.5f));
	Head->SetRelativeLocation(FVector(0.f, 0.f, StandHeight + 40.f));
	ColorIndex = StartColor;
	bOn = bStartOn;
	ApplyLight();
}

void AFTStageLight::BeginPlay()
{
	Super::BeginPlay();
	ApplyLight();
}

FText AFTStageLight::CircuitName(int32 C)
{
	switch (C)
	{
	case 1: return LOCTEXT("Key", "KEY");
	case 2: return LOCTEXT("Fill", "FILL");
	case 3: return LOCTEXT("Hero", "HERO");
	default: return FText::AsNumber(C);
	}
}

FLinearColor AFTStageLight::GetColor() const
{
	switch (ColorIndex % 4)
	{
	case 0: return FLinearColor(1.f, 0.66f, 0.3f);
	case 1: return FLinearColor(0.3f, 0.85f, 1.f);
	case 2: return FLinearColor(1.f, 0.3f, 0.75f);
	default: return FLinearColor(1.f, 0.95f, 0.88f);
	}
}

bool AFTStageLight::IsDeviceActive() const
{
	const AFTGameState* GS = GameStateOf(this);
	return bOn && (!GS || GS->bStagePower);
}

void AFTStageLight::SetOn(bool bNewOn)
{
	bOn = bNewOn;
	ApplyLight();
	ForceNetUpdate();
}

void AFTStageLight::CycleColor()
{
	ColorIndex = (ColorIndex + 1) % 4;
	ApplyLight();
	ForceNetUpdate();
}

void AFTStageLight::ResetForNewShoot()
{
	ColorIndex = StartColor;
	bOn = bStartOn;
	ApplyLight();
}

void AFTStageLight::OnRep_Light()
{
	ApplyLight();
}

void AFTStageLight::OnStagePowerChanged(bool bPowered)
{
	ApplyLight();
}

void AFTStageLight::OnShootPhaseChanged(EFTShootPhase Phase)
{
	ApplyLight();
}

void AFTStageLight::ApplyLight()
{
	const AFTGameState* GS = GameStateOf(this);
	const bool bPremiere = GS && (GS->ShootPhase == EFTShootPhase::Premiere || GS->ShootPhase == EFTShootPhase::Failed);
	const bool bLit = IsDeviceActive() && !bPremiere;
	const FLinearColor C = GetColor();
	Spot->SetLightColor(C);
	Spot->SetIntensity(Intensity);
	Spot->SetVisibility(bLit);
	Cone->SetVisibility(bLit);
	FTVis::Paint(Cone, C, 0.12f);
	FTVis::Paint(LensPart, C, bLit ? 20.f : 0.2f);
}

// ============================================================================ lighting board

AFTLightingBoard::AFTLightingBoard()
{
	PrimaryActorTick.bCanEverTick = true;
	FTVis::MakePart(this, Root, TEXT("Desk"), EFTShape::Box, FVector(0.f, 0.f, 45.f), FVector(80.f, 190.f, 90.f), Navy, FRotator::ZeroRotator, 0.f, true);
	FTVis::MakePart(this, Root, TEXT("Panel"), EFTShape::Box, FVector(-4.f, 0.f, 96.f), FVector(76.f, 180.f, 10.f), Charcoal, FRotator(-18.f, 0.f, 0.f));
	FTVis::MakePart(this, Root, TEXT("Trim"), EFTShape::Box, FVector(0.f, 0.f, 88.f), FVector(84.f, 194.f, 5.f), Teal);
	FTVis::MakeText(this, Root, TEXT("Title"), LOCTEXT("BoardTitle", "LIGHTING"), FVector(-20.f, 0.f, 125.f), FRotator(0.f, 180.f, 0.f), 16.f, FColor(255, 210, 120));
	PowerLamp = FTVis::MakePart(this, Root, TEXT("PowerLamp"), EFTShape::Sphere, FVector(-20.f, 80.f, 110.f), FVector(10.f), Green, FRotator::ZeroRotator, 6.f);
	for (int32 i = 0; i < 3; ++i)
	{
		const float Y = -55.f + i * 55.f;
		UStaticMeshComponent* T = FTVis::MakePart(this, Root, *FString::Printf(TEXT("ToggleCap%d"), i), EFTShape::Cylinder, FVector(-10.f, Y, 108.f), FVector(30.f, 30.f, 12.f), Grey, FRotator(-18.f, 0.f, 0.f));
		UStaticMeshComponent* C = FTVis::MakePart(this, Root, *FString::Printf(TEXT("ColorCap%d"), i), EFTShape::Box, FVector(12.f, Y, 101.f), FVector(18.f, 30.f, 8.f), Grey, FRotator(-18.f, 0.f, 0.f));
		ToggleCaps.Add(T);
		ColorCaps.Add(C);
		UTextRenderComponent* L = FTVis::MakeText(this, Root, *FString::Printf(TEXT("Label%d"), i), AFTStageLight::CircuitName(i + 1), FVector(-40.f, Y, 112.f), FRotator(0.f, 180.f, 0.f), 10.f, FColor::White);
		(void)L;
		UFTInteractableComponent* TI = CreateDefaultSubobject<UFTInteractableComponent>(*FString::Printf(TEXT("Toggle%d"), i));
		TI->SetupAttachment(Root);
		TI->Setup(*FString::Printf(TEXT("T%d"), i + 1), FText::Format(LOCTEXT("CircuitLabel", "{0} light"), AFTStageLight::CircuitName(i + 1)), LOCTEXT("ToggleVerb", "Toggle"), EFTInteractType::Press, FVector(18.f, 20.f, 14.f));
		TI->SetRelativeLocation(FVector(-10.f, Y, 110.f));
		TI->AddHighlight(T);
		Toggles.Add(TI);
		UFTInteractableComponent* CI = CreateDefaultSubobject<UFTInteractableComponent>(*FString::Printf(TEXT("Color%d"), i));
		CI->SetupAttachment(Root);
		CI->Setup(*FString::Printf(TEXT("C%d"), i + 1), FText::Format(LOCTEXT("CircuitColor", "{0} colour"), AFTStageLight::CircuitName(i + 1)), LOCTEXT("ColorVerb", "Change"), EFTInteractType::Press, FVector(12.f, 20.f, 10.f));
		CI->SetRelativeLocation(FVector(14.f, Y, 102.f));
		CI->AddHighlight(C);
		Colors.Add(CI);
	}
}

void AFTLightingBoard::BeginPlay()
{
	Super::BeginPlay();
	for (int32 i = 1; i <= 3; ++i)
	{
		Lights.Add(FindLight(i));
	}
}

AFTStageLight* AFTLightingBoard::FindLight(int32 Circuit) const
{
	for (TActorIterator<AFTStageLight> It(GetWorld()); It; ++It)
	{
		if (It->Circuit == Circuit)
		{
			return *It;
		}
	}
	return nullptr;
}

bool AFTLightingBoard::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	const AFTGameState* GS = GameStateOf(this);
	if (GS && !GS->bStagePower)
	{
		OutReason = LOCTEXT("NoPower", "No stage power! Restore the breaker first");
		return false;
	}
	return true;
}

FText AFTLightingBoard::GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const
{
	const int32 Index = Toggles.IndexOfByKey(Comp);
	if (Index != INDEX_NONE && Lights.IsValidIndex(Index) && Lights[Index].IsValid())
	{
		return Lights[Index]->IsOn() ? LOCTEXT("TurnOff", "Turn off") : LOCTEXT("TurnOn", "Turn on");
	}
	return Comp ? Comp->Verb : FText::GetEmpty();
}

void AFTLightingBoard::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	int32 Index = Toggles.IndexOfByKey(Comp);
	const bool bToggle = Index != INDEX_NONE;
	if (!bToggle)
	{
		Index = Colors.IndexOfByKey(Comp);
	}
	if (!Lights.IsValidIndex(Index) || !Lights[Index].IsValid())
	{
		if (User)
		{
			User->ClientRejected(LOCTEXT("NoFixture", "That circuit has no fixture"));
		}
		return;
	}
	if (bToggle)
	{
		Lights[Index]->SetOn(!Lights[Index]->IsOn());
	}
	else
	{
		Lights[Index]->CycleColor();
	}
	MulticastSound(EFTSound::Lever, Comp->GetComponentLocation(), 0.6f, 1.5f);
}

void AFTLightingBoard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AFTGameState* GS = GameStateOf(this);
	const bool bPower = !GS || GS->bStagePower;
	FTVis::Paint(PowerLamp, bPower ? Green : Red, bPower ? 6.f : (FMath::Fmod(GetWorld()->GetTimeSeconds(), 0.8f) < 0.4f ? 10.f : 0.5f));
	for (int32 i = 0; i < 3; ++i)
	{
		AFTStageLight* L = Lights.IsValidIndex(i) ? Lights[i].Get() : nullptr;
		const FLinearColor C = L ? L->GetColor() : Grey;
		FTVis::Paint(ToggleCaps[i], L && L->IsOn() ? C : Grey * 0.6f, L && L->IsDeviceActive() ? 6.f : 0.f);
		FTVis::Paint(ColorCaps[i], C, 1.5f);
	}
}

// ============================================================================ breaker

AFTBreaker::AFTBreaker()
{
	PrimaryActorTick.bCanEverTick = true;
	FTVis::MakePart(this, Root, TEXT("Panel"), EFTShape::Box, FVector(0.f, 0.f, 140.f), FVector(22.f, 100.f, 150.f), Yellow, FRotator::ZeroRotator, 0.f, true);
	for (int32 i = 0; i < 4; ++i)
	{
		FTVis::MakePart(this, Root, *FString::Printf(TEXT("Stripe%d"), i), EFTShape::Box, FVector(12.f, -36.f + i * 24.f, 72.f), FVector(2.f, 10.f, 20.f), Ink, FRotator(0.f, 0.f, 30.f));
	}
	FTVis::MakePart(this, Root, TEXT("Door"), EFTShape::Box, FVector(12.f, 0.f, 150.f), FVector(3.f, 80.f, 100.f), Hex(0xE8B42F));
	FTVis::MakeText(this, Root, TEXT("Sign"), LOCTEXT("BreakerSign", "MAIN BREAKER"), FVector(14.f, 0.f, 228.f), FRotator::ZeroRotator, 12.f, FColor(30, 30, 40));
	StatusLamp = FTVis::MakePart(this, Root, TEXT("StatusLamp"), EFTShape::Sphere, FVector(16.f, 34.f, 200.f), FVector(12.f), Green, FRotator::ZeroRotator, 6.f);
	LeverPivot = CreateDefaultSubobject<USceneComponent>(TEXT("LeverPivot"));
	LeverPivot->SetupAttachment(Root);
	LeverPivot->SetRelativeLocation(FVector(18.f, 0.f, 150.f));
	FTVis::MakePart(this, LeverPivot, TEXT("LeverArm"), EFTShape::Box, FVector(10.f, 0.f, 22.f), FVector(8.f, 10.f, 50.f), Charcoal);
	FTVis::MakePart(this, LeverPivot, TEXT("LeverGrip"), EFTShape::Capsule, FVector(20.f, 0.f, 46.f), FVector(12.f, 12.f, 44.f), Red, FRotator(0.f, 0.f, 90.f));
	Sparks = CreateDefaultSubobject<UFTChunkyParticles>(TEXT("Sparks"));
	Sparks->SetupAttachment(Root);
	Sparks->SetRelativeLocation(FVector(18.f, -30.f, 190.f));
	Sparks->MaxParticles = 40;
	Sparks->Lifetime = FVector2D(0.3f, 0.6f);
	Sparks->BaseVelocity = FVector(150.f, 0.f, 80.f);
	Sparks->VelocityJitter = FVector(120.f, 180.f, 120.f);
	Sparks->Gravity = 900.f;
	Sparks->StartSize = FVector2D(3.f, 6.f);
	Handle = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("Handle"));
	Handle->SetupAttachment(Root);
	Handle->Setup(TEXT("Restore"), LOCTEXT("BreakerLabel", "Main breaker"), LOCTEXT("Restore", "Restore stage power"), EFTInteractType::Hold, FVector(30.f, 55.f, 70.f));
	Handle->HoldTime = 1.2f;
	Handle->SetRelativeLocation(FVector(20.f, 0.f, 150.f));
}

bool AFTBreaker::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	const AFTGameState* GS = GameStateOf(this);
	if (!GS || GS->bStagePower)
	{
		OutReason = LOCTEXT("PowerOn", "Power is already on");
		return false;
	}
	if (GS->ShootPhase == EFTShootPhase::Failed)
	{
		OutReason = LOCTEXT("Dead", "The studio is dark for good... retry the shoot");
		return false;
	}
	return true;
}

void AFTBreaker::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	if (AFTSceneManager* SM = GetSceneManager())
	{
		SM->OnBreakerRestored();
	}
	MulticastSound(EFTSound::Lever, GetActorLocation() + FVector(0.f, 0.f, 150.f), 1.f, 0.8f);
}

void AFTBreaker::OnStagePowerChanged(bool bPowered)
{
	LeverPivot->SetRelativeRotation(FRotator(bPowered ? 0.f : -70.f, 0.f, 0.f));
	FTVis::Paint(StatusLamp, bPowered ? Green : Red, 8.f);
}

void AFTBreaker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AFTGameState* GS = GameStateOf(this);
	if (GS && !GS->bStagePower && GetNetMode() != NM_DedicatedServer)
	{
		SparkTimer -= DeltaSeconds;
		if (SparkTimer <= 0.f)
		{
			SparkTimer = FMath::FRandRange(0.8f, 2.f);
			Sparks->Configure(EFTShape::Sphere, Yellow, 12.f, false);
			Sparks->Burst(10);
		}
	}
}

// ============================================================================ emergency light

AFTEmergencyLight::AFTEmergencyLight()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	FTVis::MakePart(this, Root, TEXT("Base"), EFTShape::Box, FVector(0.f, 0.f, 6.f), FVector(34.f, 34.f, 12.f), Charcoal);
	Dome = FTVis::MakePart(this, Root, TEXT("Dome"), EFTShape::Ball, FVector(0.f, 0.f, 22.f), FVector(28.f, 28.f, 32.f), Red, FRotator::ZeroRotator, 0.3f);
	Spinner = CreateDefaultSubobject<USceneComponent>(TEXT("Spinner"));
	Spinner->SetupAttachment(Root);
	Spinner->SetRelativeLocation(FVector(0.f, 0.f, 22.f));
	Spot = CreateDefaultSubobject<USpotLightComponent>(TEXT("Spot"));
	Spot->SetupAttachment(Spinner);
	Spot->SetRelativeRotation(FRotator(-25.f, 0.f, 0.f));
	Spot->SetLightColor(FLinearColor(1.f, 0.08f, 0.05f));
	Spot->SetIntensity(30000.f);
	Spot->SetInnerConeAngle(18.f);
	Spot->SetOuterConeAngle(34.f);
	Spot->SetAttenuationRadius(3200.f);
	Spot->SetCastShadows(false);
	Spot->SetVisibility(false);
	Fill = CreateDefaultSubobject<UPointLightComponent>(TEXT("Fill"));
	Fill->SetupAttachment(Root);
	Fill->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	Fill->SetLightColor(FLinearColor(1.f, 0.1f, 0.06f));
	Fill->SetIntensity(6000.f);
	Fill->SetAttenuationRadius(1400.f);
	Fill->SetCastShadows(false);
	Fill->SetVisibility(false);
}

void AFTEmergencyLight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AFTGameState* GS = GameStateOf(this);
	bool bActive = false;
	if (GS)
	{
		const bool bRunning = GS->ShootPhase != EFTShootPhase::Title && GS->ShootPhase != EFTShootPhase::Lobby && GS->ShootPhase != EFTShootPhase::Premiere && GS->ShootPhase != EFTShootPhase::Results;
		bActive = bRunning && (!GS->bStagePower || GS->FloodStage != EFTFloodStage::Dry);
	}
	if (bActive != bWasActive)
	{
		bWasActive = bActive;
		Spot->SetVisibility(bActive);
		Fill->SetVisibility(bActive);
		FTVis::SetGlow(Dome, bActive ? 14.f : 0.3f);
	}
	if (bActive)
	{
		Spinner->AddLocalRotation(FRotator(0.f, 240.f * DeltaSeconds, 0.f));
	}
}

// ============================================================================ sound console

AFTSoundConsole::AFTSoundConsole()
{
	PrimaryActorTick.bCanEverTick = true;
	Tags = { TEXT("Device.SoundConsole") };
	LastPlayed = { -100.f, -100.f, -100.f };
	FTVis::MakePart(this, Root, TEXT("Desk"), EFTShape::Box, FVector(0.f, 0.f, 45.f), FVector(80.f, 200.f, 90.f), Charcoal, FRotator::ZeroRotator, 0.f, true);
	FTVis::MakePart(this, Root, TEXT("Top"), EFTShape::Box, FVector(-4.f, 0.f, 96.f), FVector(76.f, 190.f, 10.f), GreyDark, FRotator(-16.f, 0.f, 0.f));
	FTVis::MakePart(this, Root, TEXT("Trim"), EFTShape::Box, FVector(0.f, 0.f, 88.f), FVector(84.f, 204.f, 5.f), Magenta);
	FTVis::MakeText(this, Root, TEXT("Title"), LOCTEXT("SoundTitle", "SOUND"), FVector(-20.f, 0.f, 128.f), FRotator(0.f, 180.f, 0.f), 16.f, FColor(255, 150, 210));
	const FLinearColor Cols[3] = { Red, Blue, Purple };
	const FText Names[3] = { LOCTEXT("Siren", "SIREN"), LOCTEXT("Storm", "STORM"), LOCTEXT("Sting", "STING") };
	for (int32 i = 0; i < 3; ++i)
	{
		const float Y = -60.f + i * 60.f;
		UStaticMeshComponent* Btn = FTVis::MakePart(this, Root, *FString::Printf(TEXT("Button%d"), i), EFTShape::Cylinder, FVector(-6.f, Y, 107.f), FVector(34.f, 34.f, 14.f), Cols[i], FRotator(-16.f, 0.f, 0.f), 0.6f);
		UStaticMeshComponent* L = FTVis::MakePart(this, Root, *FString::Printf(TEXT("Lamp%d"), i), EFTShape::Sphere, FVector(-30.f, Y, 124.f), FVector(10.f), Cols[i], FRotator::ZeroRotator, 0.3f);
		Lamps.Add(L);
		FTVis::MakeText(this, Root, *FString::Printf(TEXT("Name%d"), i), Names[i], FVector(-38.f, Y, 110.f), FRotator(0.f, 180.f, 0.f), 9.f, FColor::White);
		UFTInteractableComponent* I = CreateDefaultSubobject<UFTInteractableComponent>(*FString::Printf(TEXT("Cue%d"), i));
		I->SetupAttachment(Root);
		I->Setup(*FString::Printf(TEXT("Cue%d"), i), FText::Format(LOCTEXT("CueLabel", "{0} cue"), Names[i]), LOCTEXT("Play", "Play"), EFTInteractType::Press, FVector(22.f, 24.f, 16.f));
		I->SetRelativeLocation(FVector(-6.f, Y, 110.f));
		I->AddHighlight(Btn);
		Buttons.Add(I);
	}
	for (int32 i = 0; i < 6; ++i)
	{
		Meters.Add(FTVis::MakePart(this, Root, *FString::Printf(TEXT("Meter%d"), i), EFTShape::Box, FVector(26.f, -50.f + i * 20.f, 100.f), FVector(6.f, 12.f, 10.f), Green, FRotator::ZeroRotator, 3.f));
	}
	// speaker stacks and boom mic
	for (int32 s = 0; s < 2; ++s)
	{
		const float Y = s ? 150.f : -150.f;
		FTVis::MakePart(this, Root, *FString::Printf(TEXT("Speaker%d"), s), EFTShape::Box, FVector(0.f, Y, 70.f), FVector(60.f, 60.f, 140.f), Charcoal, FRotator::ZeroRotator, 0.f, true);
		FTVis::MakePart(this, Root, *FString::Printf(TEXT("Woofer%d"), s), EFTShape::Cylinder, FVector(-31.f, Y, 55.f), FVector(40.f, 40.f, 4.f), GreyDark, FRotator(-90.f, 0.f, 0.f));
		FTVis::MakePart(this, Root, *FString::Printf(TEXT("Tweeter%d"), s), EFTShape::Cylinder, FVector(-31.f, Y, 110.f), FVector(20.f, 20.f, 4.f), GreyDark, FRotator(-90.f, 0.f, 0.f));
	}
	FTVis::MakePart(this, Root, TEXT("BoomStand"), EFTShape::Cylinder, FVector(60.f, -120.f, 90.f), FVector(5.f, 5.f, 180.f), Charcoal);
	FTVis::MakePart(this, Root, TEXT("BoomArm"), EFTShape::Cylinder, FVector(120.f, -120.f, 185.f), FVector(4.f, 4.f, 140.f), Charcoal, FRotator(-70.f, 0.f, 0.f));
	FTVis::MakePart(this, Root, TEXT("BoomMic"), EFTShape::Capsule, FVector(185.f, -120.f, 205.f), FVector(16.f, 16.f, 50.f), Grey, FRotator(-70.f, 0.f, 0.f));
}

void AFTSoundConsole::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTSoundConsole, LastPlayed);
}

float AFTSoundConsole::CooldownLeft(int32 Cue) const
{
	static const float Cooldowns[3] = { 7.f, 6.f, 5.f };
	if (!LastPlayed.IsValidIndex(Cue))
	{
		return 0.f;
	}
	return FMath::Max(0.f, LastPlayed[Cue] + Cooldowns[Cue] - ServerNow(this));
}

bool AFTSoundConsole::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	const int32 Cue = Buttons.IndexOfByKey(Comp);
	const float Left = CooldownLeft(Cue);
	if (Left > 0.f)
	{
		OutReason = FText::Format(LOCTEXT("Cooldown", "Cue is still playing ({0}s)"), FText::AsNumber(FMath::CeilToInt(Left)));
		return false;
	}
	return true;
}

void AFTSoundConsole::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	const int32 Cue = Buttons.IndexOfByKey(Comp);
	if (!LastPlayed.IsValidIndex(Cue))
	{
		return;
	}
	LastPlayed[Cue] = ServerNow(this);
	MulticastCue(Cue);
	static const FName Events[3] = { FTTags::EvSiren, FTTags::EvStorm, FTTags::EvSting };
	ReportEvent(Events[Cue], User);
	ForceNetUpdate();
}

void AFTSoundConsole::MulticastCue_Implementation(int32 Cue)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	static const EFTSound Sounds[3] = { EFTSound::Siren, EFTSound::Storm, EFTSound::MonsterSting };
	FTAudio::PlayAt(this, Sounds[Cue], GetActorLocation() + FVector(0.f, 0.f, 120.f), 0.55f);
	FTAudio::PlayAt(this, Sounds[Cue], GetActorTransform().TransformPosition(SpeakerLocation), 1.f);
}

void AFTSoundConsole::ResetForNewShoot()
{
	LastPlayed = { -100.f, -100.f, -100.f };
}

void AFTSoundConsole::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	const float Now = ServerNow(this);
	static const float Lengths[3] = { 3.6f, 4.5f, 2.4f };
	bool bAny = false;
	for (int32 i = 0; i < 3 && i < LastPlayed.Num(); ++i)
	{
		const bool bPlaying = Now - LastPlayed[i] < Lengths[i];
		bAny |= bPlaying;
		FTVis::SetGlow(Lamps[i], bPlaying ? (FMath::Fmod(Now, 0.4f) < 0.2f ? 16.f : 4.f) : 0.3f);
	}
	const float T = GetWorld()->GetTimeSeconds();
	for (int32 i = 0; i < Meters.Num(); ++i)
	{
		const float Level = bAny ? 0.3f + 0.7f * FMath::Abs(FMath::Sin(T * (7.f + i * 1.7f))) : 0.08f;
		Meters[i]->SetRelativeScale3D(FVector(0.06f, 0.12f, 0.1f + Level * 0.5f));
		Meters[i]->SetRelativeLocation(FVector(26.f, -50.f + i * 20.f, 96.f + Level * 25.f));
		FTVis::Paint(Meters[i], Level > 0.8f ? Red : (Level > 0.55f ? Yellow : Green), 3.f);
	}
}

// ============================================================================ effect machines

AFTEffectMachine::AFTEffectMachine()
{
	PrimaryActorTick.bCanEverTick = true;
	Body = CreateDefaultSubobject<USceneComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Spinner = CreateDefaultSubobject<USceneComponent>(TEXT("Spinner"));
	Spinner->SetupAttachment(Body);
	Rig = CreateDefaultSubobject<USceneComponent>(TEXT("Rig"));
	Rig->SetupAttachment(Root);
	for (int32 i = 0; i < 12; ++i)
	{
		Parts.Add(FTVis::MakePart(this, Body, *FString::Printf(TEXT("Part%d"), i), EFTShape::Box, FVector::ZeroVector, FVector(10.f), Grey));
	}
	for (int32 i = 0; i < 4; ++i)
	{
		Parts.Add(FTVis::MakePart(this, Spinner, *FString::Printf(TEXT("Blade%d"), i), EFTShape::Box, FVector::ZeroVector, FVector(10.f), Yellow));
	}
	for (int32 i = 0; i < 6; ++i)
	{
		Parts.Add(FTVis::MakePart(this, Rig, *FString::Printf(TEXT("RigPart%d"), i), EFTShape::Box, FVector::ZeroVector, FVector(10.f), Grey));
	}
	StatusLamp = FTVis::MakePart(this, Body, TEXT("StatusLamp"), EFTShape::Sphere, FVector(0.f, 0.f, 90.f), FVector(10.f), Red, FRotator::ZeroRotator, 2.f);
	Particles = CreateDefaultSubobject<UFTChunkyParticles>(TEXT("Particles"));
	Particles->SetupAttachment(Root);
	Switch = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("Switch"));
	Switch->SetupAttachment(Body);
	Switch->Setup(TEXT("Toggle"), FText::GetEmpty(), LOCTEXT("Start", "Start"), EFTInteractType::Press, FVector(50.f, 50.f, 60.f));
	Switch->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
}

void AFTEffectMachine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTEffectMachine, bOn);
}

void AFTEffectMachine::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildLook();
}

void AFTEffectMachine::BuildLook()
{
	for (UStaticMeshComponent* P : Parts)
	{
		P->SetVisibility(false);
		P->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	}
	int32 Next = 0;
	int32 NextBlade = 12;
	int32 NextRig = 16;
	auto Set = [this](int32 Index, EFTShape Shape, const FVector& Loc, const FVector& Size, const FLinearColor& Color, const FRotator& Rot = FRotator::ZeroRotator)
	{
		UStaticMeshComponent* P = Parts[Index];
		FTVis::ApplyShape(P, Shape, Size);
		P->SetRelativeLocationAndRotation(Loc, Rot);
		FTVis::Paint(P, Color, 0.f, true);
		P->SetVisibility(true);
	};
	auto Wheels = [&](float HalfX, float HalfY)
	{
		for (float X : { -HalfX, HalfX })
		{
			for (float Y : { -HalfY, HalfY })
			{
				Set(Next++, EFTShape::Cylinder, FVector(X, Y, 9.f), FVector(18.f, 18.f, 8.f), Charcoal, FRotator(0.f, 0.f, 90.f));
			}
		}
	};
	Rig->SetRelativeLocation(EffectCenter);
	Spinner->SetRelativeLocation(FVector::ZeroVector);
	FText Label;
	switch (Effect)
	{
	case EFTEffectType::Wind:
		Tags = { FTTags::DevWind };
		Label = LOCTEXT("WindLabel", "Wind machine");
		Set(Next++, EFTShape::Box, FVector(0.f, 0.f, 20.f), FVector(90.f, 70.f, 16.f), GreyDark);
		Wheels(34.f, 28.f);
		Set(Next++, EFTShape::Box, FVector(0.f, 0.f, 60.f), FVector(20.f, 60.f, 70.f), Charcoal);
		Set(Next++, EFTShape::Torus, FVector(0.f, 0.f, 120.f), FVector(150.f, 150.f, 30.f), Teal, FRotator(90.f, 0.f, 0.f));
		Set(Next++, EFTShape::Cylinder, FVector(-26.f, 0.f, 120.f), FVector(50.f, 50.f, 40.f), TealDark, FRotator(90.f, 0.f, 0.f));
		Set(Next++, EFTShape::Box, FVector(14.f, 0.f, 120.f), FVector(4.f, 130.f, 6.f), Charcoal);
		Set(Next++, EFTShape::Box, FVector(14.f, 0.f, 120.f), FVector(4.f, 6.f, 130.f), Charcoal);
		Spinner->SetRelativeLocation(FVector(4.f, 0.f, 120.f));
		for (int32 b = 0; b < 4; ++b)
		{
			Set(NextBlade++, EFTShape::Box, FRotator(0.f, 0.f, b * 90.f).RotateVector(FVector(0.f, 0.f, 30.f)), FVector(6.f, 22.f, 56.f), Yellow, FRotator(0.f, 0.f, b * 90.f) + FRotator(0.f, 20.f, 0.f));
		}
		Particles->Configure(EFTShape::Cube, White, 1.5f, true);
		Particles->SetRelativeLocation(FVector(60.f, 0.f, 120.f));
		Particles->SpawnRate = 40.f;
		Particles->Lifetime = FVector2D(0.6f, 1.1f);
		Particles->SpawnExtent = FVector(10.f, 60.f, 60.f);
		Particles->BaseVelocity = FVector(900.f, 0.f, 0.f);
		Particles->VelocityJitter = FVector(150.f, 60.f, 40.f);
		Particles->StartSize = FVector2D(2.f, 3.f);
		Particles->Stretch = 0.05f;
		Particles->bSpin = false;
		Particles->MaxParticles = 80;
		break;
	case EFTEffectType::Rain:
		Tags = { FTTags::DevRain, FTTags::SubjStorm };
		Label = LOCTEXT("RainLabel", "Rain machine");
		Set(Next++, EFTShape::Box, FVector(0.f, 0.f, 22.f), FVector(90.f, 60.f, 16.f), GreyDark);
		Wheels(32.f, 24.f);
		Set(Next++, EFTShape::Box, FVector(-12.f, 0.f, 55.f), FVector(50.f, 54.f, 50.f), Teal);
		Set(Next++, EFTShape::Capsule, FVector(24.f, 0.f, 58.f), FVector(34.f, 34.f, 60.f), Yellow, FRotator(0.f, 0.f, 90.f));
		Set(Next++, EFTShape::Cylinder, FVector(-12.f, 0.f, 84.f), FVector(18.f, 18.f, 10.f), Coral);
		Set(NextRig++, EFTShape::Box, FVector::ZeroVector, FVector(18.f, EffectExtent.Y * 2.f, 14.f), Charcoal);
		Set(NextRig++, EFTShape::Box, FVector::ZeroVector, FVector(EffectExtent.X * 1.6f, 14.f, 12.f), Charcoal);
		Set(NextRig++, EFTShape::Cylinder, FVector(0.f, -EffectExtent.Y * 0.8f, 250.f), FVector(4.f, 4.f, 500.f), Grey);
		Set(NextRig++, EFTShape::Cylinder, FVector(0.f, EffectExtent.Y * 0.8f, 250.f), FVector(4.f, 4.f, 500.f), Grey);
		Set(NextRig++, EFTShape::Cone, FVector(0.f, -EffectExtent.Y * 0.5f, -14.f), FVector(20.f, 20.f, 18.f), Cyan, FRotator(180.f, 0.f, 0.f));
		Set(NextRig++, EFTShape::Cone, FVector(0.f, EffectExtent.Y * 0.5f, -14.f), FVector(20.f, 20.f, 18.f), Cyan, FRotator(180.f, 0.f, 0.f));
		Particles->Configure(EFTShape::Cube, Hex(0xA9E6FF), 0.6f, false);
		Particles->SetRelativeLocation(EffectCenter);
		Particles->SpawnRate = 260.f;
		Particles->MaxParticles = 420;
		Particles->Lifetime = FVector2D(0.8f, 1.1f);
		Particles->SpawnExtent = FVector(EffectExtent.X, EffectExtent.Y, 10.f);
		Particles->BaseVelocity = FVector(0.f, 0.f, -900.f);
		Particles->VelocityJitter = FVector(10.f, 10.f, 100.f);
		Particles->StartSize = FVector2D(1.5f, 2.2f);
		Particles->Stretch = 0.035f;
		Particles->bSpin = false;
		Particles->GrowInTime = 0.f;
		break;
	case EFTEffectType::Smoke:
		Tags = { FTTags::DevSmoke };
		Label = LOCTEXT("SmokeLabel", "Smoke machine");
		Set(Next++, EFTShape::Box, FVector(0.f, 0.f, 45.f), FVector(70.f, 50.f, 50.f), Charcoal);
		Wheels(26.f, 20.f);
		Set(Next++, EFTShape::Box, FVector(0.f, 0.f, 76.f), FVector(30.f, 10.f, 8.f), GreyDark);
		Set(Next++, EFTShape::Cylinder, FVector(42.f, 0.f, 45.f), FVector(22.f, 22.f, 20.f), GreyDark, FRotator(-90.f, 0.f, 0.f));
		Set(Next++, EFTShape::Box, FVector(-36.f, 0.f, 45.f), FVector(2.f, 30.f, 20.f), Coral);
		Particles->Configure(EFTShape::Ball, FLinearColor(0.85f, 0.87f, 0.95f, 0.35f), 0.1f, true);
		Particles->SetRelativeLocation(FVector(60.f, 0.f, 45.f));
		Particles->SpawnRate = 7.f;
		Particles->MaxParticles = 45;
		Particles->Lifetime = FVector2D(5.f, 7.f);
		Particles->SpawnExtent = FVector(6.f);
		Particles->BaseVelocity = FVector(110.f, 0.f, 18.f);
		Particles->VelocityJitter = FVector(40.f, 40.f, 12.f);
		Particles->Drag = 0.25f;
		Particles->StartSize = FVector2D(30.f, 50.f);
		Particles->EndSizeScale = 4.5f;
		Particles->GrowInTime = 0.4f;
		break;
	case EFTEffectType::Foam:
		Tags = { FTTags::DevFoam };
		Label = LOCTEXT("FoamLabel", "Foam cannon");
		Set(Next++, EFTShape::Box, FVector(0.f, 0.f, 22.f), FVector(90.f, 60.f, 16.f), GreyDark);
		Wheels(32.f, 24.f);
		Set(Next++, EFTShape::Capsule, FVector(-10.f, 0.f, 55.f), FVector(40.f, 40.f, 70.f), Yellow, FRotator(0.f, 0.f, 90.f));
		Set(Next++, EFTShape::Cylinder, FVector(34.f, 0.f, 92.f), FVector(28.f, 28.f, 110.f), Teal, FRotator(-65.f, 0.f, 0.f));
		Set(Next++, EFTShape::Cylinder, FVector(80.f, 0.f, 112.f), FVector(34.f, 34.f, 10.f), Coral, FRotator(-65.f, 0.f, 0.f));
		Set(Next++, EFTShape::Box, FVector(-30.f, 0.f, 90.f), FVector(10.f, 8.f, 24.f), Coral);
		Particles->Configure(EFTShape::Ball, White, 0.15f, false);
		Particles->SetRelativeLocation(FVector(86.f, 0.f, 116.f));
		Particles->SpawnRate = 0.f;
		Particles->MaxParticles = 160;
		Particles->Lifetime = FVector2D(12.f, 16.f);
		Particles->SpawnExtent = FVector(8.f);
		Particles->BaseVelocity = FVector(820.f, 0.f, 420.f);
		Particles->VelocityJitter = FVector(200.f, 180.f, 150.f);
		Particles->Gravity = 800.f;
		Particles->bRestOnFloor = true;
		Particles->StartSize = FVector2D(14.f, 30.f);
		Particles->EndSizeScale = 0.8f;
		break;
	}
	Switch->Label = Label;
	StatusLamp->SetRelativeLocation(FVector(-20.f, 30.f, Effect == EFTEffectType::Wind ? 50.f : 90.f));
}

void AFTEffectMachine::BeginPlay()
{
	Super::BeginPlay();
	BuildLook();
	if (Effect == EFTEffectType::Foam)
	{
		Particles->FloorZ = GetActorLocation().Z + 2.f;
	}
	else if (Effect == EFTEffectType::Rain)
	{
		Particles->FloorZ = GetActorLocation().Z + 70.f;
	}
	OnRep_On();
}

bool AFTEffectMachine::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	if (Effect == EFTEffectType::Foam && GetWorld() && GetWorld()->GetTimeSeconds() - LastFoam < 1.2f)
	{
		OutReason = LOCTEXT("FoamReload", "Reloading foam...");
		return false;
	}
	return true;
}

FText AFTEffectMachine::GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const
{
	if (Effect == EFTEffectType::Foam)
	{
		return LOCTEXT("Fire", "Fire");
	}
	return bOn ? LOCTEXT("Stop", "Stop") : LOCTEXT("StartVerb", "Start");
}

void AFTEffectMachine::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	if (Effect == EFTEffectType::Foam)
	{
		LastFoam = GetWorld()->GetTimeSeconds();
		MulticastFoam(FMath::Rand());
		ReportEvent(FTTags::EvFoam, User);
		return;
	}
	bOn = !bOn;
	OnRep_On();
	MulticastSound(EFTSound::Lever, GetActorLocation() + FVector(0.f, 0.f, 80.f), 0.8f, 1.1f);
	ForceNetUpdate();
}

void AFTEffectMachine::MulticastFoam_Implementation(int32 Seed)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	FMath::RandInit(Seed);
	Particles->Burst(26);
	FTAudio::PlayAt(this, EFTSound::FoamPop, Particles->GetComponentLocation(), 1.f);
}

void AFTEffectMachine::ResetForNewShoot()
{
	bOn = false;
	OnRep_On();
	Particles->ClearParticles();
}

void AFTEffectMachine::OnRep_On()
{
	FTVis::Paint(StatusLamp, bOn ? Green : Red, bOn ? 8.f : 2.f);
	if (Effect != EFTEffectType::Foam)
	{
		Particles->SetEmitting(bOn);
	}
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (bOn && !Loop)
	{
		const EFTSound S = Effect == EFTEffectType::Wind ? EFTSound::WindLoop : (Effect == EFTEffectType::Rain ? EFTSound::RainLoop : EFTSound::SmokeHiss);
		if (Effect != EFTEffectType::Foam)
		{
			Loop = FTAudio::Attach(Effect == EFTEffectType::Rain ? Particles.Get() : Body.Get(), S, Effect == EFTEffectType::Smoke ? 0.5f : 0.8f);
		}
	}
	else if (!bOn && Loop)
	{
		Loop->Stop();
		Loop->DestroyComponent();
		Loop = nullptr;
	}
}

void AFTEffectMachine::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Effect == EFTEffectType::Wind && bOn)
	{
		Spinner->AddLocalRotation(FRotator(0.f, 0.f, 900.f * DeltaSeconds));
	}
	// wind pushes rain streaks and smoke puffs
	if (Effect == EFTEffectType::Rain || Effect == EFTEffectType::Smoke)
	{
		FVector Wind = FVector::ZeroVector;
		for (TActorIterator<AFTEffectMachine> It(GetWorld()); It; ++It)
		{
			if (It->Effect == EFTEffectType::Wind && It->bOn)
			{
				Wind += It->GetActorForwardVector() * (Effect == EFTEffectType::Rain ? 900.f : 120.f);
			}
		}
		Particles->Wind = Wind;
	}
}

bool AFTEffectMachine::GetSubjectBounds(FName SubjectTag, FVector& OutCenter, float& OutRadius) const
{
	OutCenter = GetActorTransform().TransformPosition(EffectCenter - FVector(0.f, 0.f, 250.f));
	OutRadius = FMath::Max(EffectExtent.X, EffectExtent.Y) * 0.6f;
	return true;
}

// ============================================================================ fin glider

AFTFinGlider::AFTFinGlider()
{
	PrimaryActorTick.bCanEverTick = true;
	Tags = { FTTags::SubjFin };
	Fin = CreateDefaultSubobject<USceneComponent>(TEXT("Fin"));
	Fin->SetupAttachment(Root);
	Fin->SetRelativeLocation(FVector(0.f, 0.f, -80.f));
	FTVis::MakePart(this, Fin, TEXT("FinBody"), EFTShape::Prism, FVector(0.f, 0.f, 36.f), FVector(10.f, 80.f, 72.f), Blue, FRotator(0.f, 0.f, 0.f));
	FTVis::MakePart(this, Fin, TEXT("FinEdge"), EFTShape::Prism, FVector(-1.f, 6.f, 30.f), FVector(12.f, 50.f, 52.f), DeepBlue);
	Wake = CreateDefaultSubobject<UFTChunkyParticles>(TEXT("Wake"));
	Wake->SetupAttachment(Fin);
	Wake->SetRelativeLocation(FVector(0.f, -30.f, 2.f));
	Wake->SpawnRate = 30.f;
	Wake->MaxParticles = 80;
	Wake->Lifetime = FVector2D(0.8f, 1.4f);
	Wake->BaseVelocity = FVector(0.f, -60.f, 40.f);
	Wake->VelocityJitter = FVector(60.f, 30.f, 30.f);
	Wake->Gravity = 250.f;
	Wake->StartSize = FVector2D(8.f, 16.f);
	FTVis::MakePart(this, Root, TEXT("CrankPost"), EFTShape::Cylinder, FVector(0.f, -60.f, 40.f), FVector(10.f, 10.f, 80.f), Charcoal);
	FTVis::MakePart(this, Root, TEXT("CrankWheel"), EFTShape::Torus, FVector(0.f, -60.f, 84.f), FVector(40.f, 40.f, 8.f), Coral, FRotator(0.f, 0.f, 90.f));
	Crank = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("Crank"));
	Crank->SetupAttachment(Root);
	Crank->Setup(TEXT("Glide"), LOCTEXT("FinCrank", "Fin rail"), LOCTEXT("Run", "Run the fin"), EFTInteractType::Press, FVector(30.f, 30.f, 40.f));
	Crank->SetRelativeLocation(FVector(0.f, -60.f, 70.f));
}

void AFTFinGlider::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTFinGlider, GlideStart);
}

bool AFTFinGlider::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	if (ServerNow(this) - GlideStart < GlideTime)
	{
		OutReason = LOCTEXT("Gliding", "The fin is already swimming");
		return false;
	}
	return true;
}

void AFTFinGlider::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	StartGlide();
}

void AFTFinGlider::OnSceneEvent(FName Event, AActor* Source)
{
	if (Event == FTTags::EvSiren)
	{
		StartGlide();
	}
}

void AFTFinGlider::StartGlide()
{
	if (!HasAuthority() || ServerNow(this) - GlideStart < GlideTime)
	{
		return;
	}
	GlideStart = ServerNow(this);
	MulticastSound(EFTSound::Splash, GetActorLocation(), 0.8f, 0.9f);
	ForceNetUpdate();
}

void AFTFinGlider::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float T = (ServerNow(this) - GlideStart) / FMath::Max(GlideTime, 0.1f);
	const bool bGliding = T >= 0.f && T <= 1.f;
	if (bGliding)
	{
		const float Rise = FMath::Clamp(FMath::Min(T / 0.12f, (1.f - T) / 0.12f), 0.f, 1.f);
		const FVector P = FMath::Lerp(FVector::ZeroVector, PathEnd, FMath::SmoothStep(0.f, 1.f, T));
		Fin->SetRelativeLocation(P + FVector(FMath::Sin(T * 18.f) * 8.f, 0.f, FMath::Lerp(-80.f, -8.f, Rise)));
		Fin->SetRelativeRotation(FRotator(0.f, 0.f, FMath::Sin(T * 12.f) * 6.f));
	}
	else
	{
		Fin->SetRelativeLocation(FVector(0.f, 0.f, -80.f));
	}
	if (GetNetMode() != NM_DedicatedServer && Wake->IsEmitting() != bGliding)
	{
		Wake->Configure(EFTShape::Sphere, White, 0.3f, false);
		Wake->SetEmitting(bGliding);
	}
}

bool AFTFinGlider::GetSubjectBounds(FName SubjectTag, FVector& OutCenter, float& OutRadius) const
{
	OutCenter = Fin->GetComponentLocation() + FVector(0.f, 0.f, 30.f);
	OutRadius = 60.f;
	return true;
}

#undef LOCTEXT_NAMESPACE
