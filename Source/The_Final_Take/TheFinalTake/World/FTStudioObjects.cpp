#include "TheFinalTake/World/FTStudioObjects.h"

#include "TheFinalTake/Core/FTVisuals.h"
#include "TheFinalTake/Characters/FTCharacter.h"
#include "TheFinalTake/Data/FTFilmDefinition.h"
#include "TheFinalTake/FX/FTChunkyParticles.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Game/FTPlayerController.h"
#include "TheFinalTake/Game/FTSceneManager.h"
#include "TheFinalTake/Interaction/FTInteractableComponent.h"
#include "TheFinalTake/Props/FTProp.h"
#include "TheFinalTake/UI/FTMenus.h"

#include "Components/AudioComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "FinalTakeWorld"

using namespace FTColors;

namespace
{
	const AFTGameState* GSOf(const UObject* Ctx)
	{
		const UWorld* W = Ctx ? Ctx->GetWorld() : nullptr;
		return W ? W->GetGameState<AFTGameState>() : nullptr;
	}
}

// ============================================================================ door

AFTDoor::AFTDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	Panel = FTVis::MakePart(this, Root, TEXT("Panel"), EFTShape::Cube, FVector::ZeroVector, FVector(20.f, 300.f, 380.f), Teal, FRotator::ZeroRotator, 0.f, true);
	PanelStripe = FTVis::MakePart(this, Panel, TEXT("PanelStripe"), EFTShape::Cube, FVector(0.f, 0.f, -0.35f * 100.f), FVector(1.2f, 1.f, 0.08f), Yellow);
	PanelStripe->SetRelativeScale3D(FVector(1.2f, 1.f, 0.08f));
	FrameL = FTVis::MakePart(this, Root, TEXT("FrameL"), EFTShape::Box, FVector::ZeroVector, FVector(40.f, 30.f, 400.f), Cream, FRotator::ZeroRotator, 0.f, true);
	FrameR = FTVis::MakePart(this, Root, TEXT("FrameR"), EFTShape::Box, FVector::ZeroVector, FVector(40.f, 30.f, 400.f), Cream, FRotator::ZeroRotator, 0.f, true);
	FrameTop = FTVis::MakePart(this, Root, TEXT("FrameTop"), EFTShape::Box, FVector::ZeroVector, FVector(40.f, 360.f, 30.f), Cream);
	KeyPanel = FTVis::MakePart(this, Root, TEXT("KeyPanel"), EFTShape::Box, FVector::ZeroVector, FVector(10.f, 30.f, 44.f), Charcoal);
	KeyLamp = FTVis::MakePart(this, Root, TEXT("KeyLamp"), EFTShape::Sphere, FVector::ZeroVector, FVector(9.f), Red, FRotator::ZeroRotator, 8.f);
	SignText = FTVis::MakeText(this, Root, TEXT("SignText"), FText::GetEmpty(), FVector::ZeroVector, FRotator(0.f, 180.f, 0.f), 60.f, FColor(255, 214, 90));
	SignTextBack = FTVis::MakeText(this, Root, TEXT("SignTextBack"), FText::GetEmpty(), FVector::ZeroVector, FRotator::ZeroRotator, 40.f, FColor(255, 214, 90));
	DoorUse = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("DoorUse"));
	DoorUse->SetupAttachment(Root);
	DoorUse->Setup(TEXT("Door"), LOCTEXT("DoorLabel", "Door"), LOCTEXT("Open", "Open"), EFTInteractType::Press, FVector(40.f, 150.f, 190.f));
	DoorUse->MaxDistance = 380.f;
	KeyUse = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("KeyUse"));
	KeyUse->SetupAttachment(Root);
	KeyUse->Setup(TEXT("Keycard"), LOCTEXT("KeyLabel", "Keycard panel"), LOCTEXT("Swipe", "Swipe keycard"), EFTInteractType::Press, FVector(20.f, 26.f, 30.f));
	KeyUse->AddHighlight(KeyPanel);
}

void AFTDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTDoor, bOpen);
}

void AFTDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	const float W = Size.X, H = Size.Y;
	FTVis::ApplyShape(Panel, EFTShape::Cube, FVector(20.f, W, H));
	Panel->SetRelativeLocation(FVector(0.f, 0.f, H * 0.5f));
	FTVis::Paint(Panel, DoorColor, 0.f, true);
	FTVis::ApplyShape(FrameL, EFTShape::Box, FVector(44.f, 30.f, H + 20.f));
	FTVis::ApplyShape(FrameR, EFTShape::Box, FVector(44.f, 30.f, H + 20.f));
	FTVis::ApplyShape(FrameTop, EFTShape::Box, FVector(44.f, W + 60.f, 30.f));
	FrameL->SetRelativeLocation(FVector(0.f, -W * 0.5f - 15.f, (H + 20.f) * 0.5f));
	FrameR->SetRelativeLocation(FVector(0.f, W * 0.5f + 15.f, (H + 20.f) * 0.5f));
	FrameTop->SetRelativeLocation(FVector(0.f, 0.f, H + 15.f));
	const bool bKey = Lock == EFTDoorLock::Keycard;
	KeyPanel->SetVisibility(bKey);
	KeyLamp->SetVisibility(Lock != EFTDoorLock::Free);
	KeyPanel->SetRelativeLocation(FVector(-26.f, -W * 0.5f - 60.f, 130.f));
	KeyLamp->SetRelativeLocation(Lock == EFTDoorLock::Keycard ? FVector(-32.f, -W * 0.5f - 60.f, 148.f) : FVector(-26.f, 0.f, H + 50.f));
	KeyUse->SetRelativeLocation(FVector(-30.f, -W * 0.5f - 60.f, 130.f));
	KeyUse->SetInteractionEnabled(bKey);
	DoorUse->SetBoxExtent(FVector(40.f, W * 0.5f, H * 0.5f));
	DoorUse->SetRelativeLocation(FVector(0.f, 0.f, H * 0.5f));
	SignText->SetText(Sign);
	SignText->SetRelativeLocation(FVector(-26.f, 0.f, H + 75.f));
	SignTextBack->SetText(Sign);
	SignTextBack->SetRelativeLocation(FVector(26.f, 0.f, H + 60.f));
	FTVis::Paint(KeyLamp, Red, 8.f, true);
}

bool AFTDoor::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	const AFTGameState* GS = GSOf(this);
	if (Comp == KeyUse)
	{
		if (bOpen)
		{
			OutReason = LOCTEXT("AlreadyOpen", "Already open");
			return false;
		}
		if (!GS || GS->FilmId.IsNone())
		{
			OutReason = LOCTEXT("NeedScript", "Access denied: greenlight a script in the Director's Office first");
			return false;
		}
		return true;
	}
	switch (Lock)
	{
	case EFTDoorLock::Keycard:
		if (!bOpen)
		{
			OutReason = GS && !GS->FilmId.IsNone() ? LOCTEXT("SwipeFirst", "Locked - swipe the keycard panel") : LOCTEXT("LockedNoScript", "Locked - pick a script in the Director's Office first");
			return false;
		}
		OutReason = LOCTEXT("StaysOpen", "Stage 4 stays open during the shoot");
		return false;
	case EFTDoorLock::Projection:
		if (!GS || !GS->bProjectionUnlocked)
		{
			OutReason = LOCTEXT("ProjLocked", "Projection room - locked until the final scene wraps");
			return false;
		}
		return true;
	default:
		return true;
	}
}

FText AFTDoor::GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const
{
	if (Comp == KeyUse)
	{
		return LOCTEXT("SwipeVerb", "Swipe keycard");
	}
	return bOpen ? LOCTEXT("Close", "Close") : LOCTEXT("OpenVerb", "Open");
}

void AFTDoor::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	if (Comp == KeyUse)
	{
		MulticastSound(EFTSound::Keycard, KeyPanel->GetComponentLocation(), 1.f, 1.f);
		SetOpen(true);
		Announce(LOCTEXT("StageOpen", "STAGE 4 is open - suit up and set the scene!"), EFTAnnounceStyle::Hint, EFTSound::None);
		return;
	}
	SetOpen(!bOpen);
}

void AFTDoor::SetOpen(bool bNewOpen)
{
	if (bOpen == bNewOpen)
	{
		return;
	}
	bOpen = bNewOpen;
	MulticastSound(EFTSound::DoorSlide, GetActorLocation() + FVector(0.f, 0.f, 150.f), 1.f, 1.f);
	ForceNetUpdate();
}

void AFTDoor::ResetForNewShoot()
{
	if (Lock != EFTDoorLock::Free)
	{
		bOpen = false;
	}
}

void AFTDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AFTGameState* GS = GSOf(this);
	if (HasAuthority() && Lock == EFTDoorLock::Projection && GS && GS->bProjectionUnlocked && !bOpen)
	{
		SetOpen(true);
	}
	OpenAlpha = FMath::FInterpTo(OpenAlpha, bOpen ? 1.f : 0.f, DeltaSeconds, 2.5f);
	Panel->SetRelativeLocation(FVector(0.f, OpenAlpha * (Size.X + 20.f), Size.Y * 0.5f));
	const bool bLockedState = Lock != EFTDoorLock::Free && !bOpen;
	const bool bReady = Lock == EFTDoorLock::Keycard && GS && !GS->FilmId.IsNone() && !bOpen;
	FTVis::Paint(KeyLamp, bLockedState ? (bReady ? Yellow : Red) : Green, bReady ? (FMath::Fmod(GetWorld()->GetTimeSeconds(), 0.8f) < 0.4f ? 14.f : 3.f) : 8.f);
	const bool bUsable = Lock == EFTDoorLock::Free || (Lock == EFTDoorLock::Projection && GS && GS->bProjectionUnlocked);
	DoorUse->SetInteractionEnabled(bUsable || !bOpen);
}

// ============================================================================ script book

AFTScriptBook::AFTScriptBook()
{
	PrimaryActorTick.bCanEverTick = true;
	FTVis::MakePart(this, Root, TEXT("Back"), EFTShape::Box, FVector(0.f, 0.f, 3.f), FVector(100.f, 140.f, 6.f), Teal);
	FTVis::MakePart(this, Root, TEXT("Pages"), EFTShape::Box, FVector(0.f, 0.f, 12.f), FVector(92.f, 132.f, 14.f), Cream);
	FTVis::MakePart(this, Root, TEXT("PageEdge"), EFTShape::Box, FVector(0.f, 67.f, 12.f), FVector(92.f, 2.f, 12.f), CreamDark);
	FTVis::MakePart(this, Root, TEXT("RibbonA"), EFTShape::Box, FVector(30.f, 70.f, 8.f), FVector(10.f, 16.f, 2.f), Coral);
	FTVis::MakePart(this, Root, TEXT("RibbonB"), EFTShape::Box, FVector(-5.f, 70.f, 8.f), FVector(10.f, 16.f, 2.f), Yellow);
	FTVis::MakePart(this, Root, TEXT("RibbonC"), EFTShape::Box, FVector(-35.f, 70.f, 8.f), FVector(10.f, 16.f, 2.f), Magenta);
	CoverPivot = CreateDefaultSubobject<USceneComponent>(TEXT("CoverPivot"));
	CoverPivot->SetupAttachment(Root);
	CoverPivot->SetRelativeLocation(FVector(0.f, -70.f, 20.f));
	FTVis::MakePart(this, CoverPivot, TEXT("Cover"), EFTShape::Box, FVector(0.f, 70.f, 0.f), FVector(100.f, 140.f, 6.f), Teal);
	FTVis::MakePart(this, CoverPivot, TEXT("Label"), EFTShape::Box, FVector(0.f, 70.f, 3.5f), FVector(60.f, 100.f, 1.f), Cream);
	CoverTitle = FTVis::MakeText(this, CoverPivot, TEXT("CoverTitle"), LOCTEXT("Scripts", "SCRIPTS"), FVector(0.f, 70.f, 4.5f), FRotator(90.f, 0.f, 90.f), 16.f, FColor(27, 33, 64));
	Open = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("Open"));
	Open->SetupAttachment(Root);
	Open->Setup(TEXT("Open"), LOCTEXT("BookLabel", "The Script Book"), LOCTEXT("OpenBook", "Open"), EFTInteractType::Hold, FVector(60.f, 80.f, 30.f));
	Open->HoldTime = 0.7f;
	Open->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
}

bool AFTScriptBook::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	const AFTGameState* GS = GSOf(this);
	if (GS && GS->ScriptBookUser && User && GS->ScriptBookUser != User->GetPlayerState())
	{
		OutReason = LOCTEXT("SomeoneReading", "Someone else is reading the script");
		return false;
	}
	return true;
}

void AFTScriptBook::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	AFTPlayerController* PC = User ? Cast<AFTPlayerController>(User->GetController()) : nullptr;
	AFTSceneManager* SM = GetSceneManager();
	FText Reason;
	if (PC && SM && SM->RequestOpenScriptBook(PC, Reason))
	{
		MulticastSound(EFTSound::Rustle, GetActorLocation(), 1.f, 0.8f);
		PC->ClientOpenScriptBook();
	}
	else if (User)
	{
		User->ClientRejected(Reason.IsEmpty() ? LOCTEXT("BookBusy", "The script book is busy") : Reason);
	}
}

void AFTScriptBook::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AFTGameState* GS = GSOf(this);
	const bool bReading = GS && GS->ScriptBookUser;
	CoverAngle = FMath::FInterpTo(CoverAngle, bReading ? 165.f : 0.f, DeltaSeconds, 5.f);
	CoverPivot->SetRelativeRotation(FRotator(0.f, 0.f, -CoverAngle));
	const UFTFilmDefinition* Film = GS ? GS->GetFilm() : nullptr;
	CoverTitle->SetText(Film ? Film->Title : LOCTEXT("Scripts2", "SCRIPTS"));
}

// ============================================================================ scene board

AFTSceneBoard::AFTSceneBoard()
{
	PrimaryActorTick.bCanEverTick = true;
	FTVis::MakePart(this, Root, TEXT("Frame"), EFTShape::Box, FVector(0.f, 0.f, 0.f), FVector(10.f, 300.f, 200.f), WoodDark);
	FTVis::MakePart(this, Root, TEXT("Cork"), EFTShape::Box, FVector(6.f, 0.f, 0.f), FVector(4.f, 280.f, 180.f), Hex(0xC98B55));
	FTVis::MakePart(this, Root, TEXT("NoteA"), EFTShape::Box, FVector(9.f, -110.f, 60.f), FVector(2.f, 50.f, 40.f), Yellow, FRotator(0.f, 0.f, 6.f));
	FTVis::MakePart(this, Root, TEXT("NoteB"), EFTShape::Box, FVector(9.f, 115.f, -65.f), FVector(2.f, 44.f, 36.f), Hex(0x9FE3D9), FRotator(0.f, 0.f, -8.f));
	FTVis::MakePart(this, Root, TEXT("Paper"), EFTShape::Box, FVector(9.f, 10.f, -5.f), FVector(2.f, 220.f, 150.f), Cream);
	Title = FTVis::MakeText(this, Root, TEXT("Title"), LOCTEXT("NoScript", "NO SCRIPT YET"), FVector(12.f, 10.f, 55.f), FRotator::ZeroRotator, 20.f, FColor(27, 33, 64));
	for (int32 i = 0; i < 3; ++i)
	{
		UTextRenderComponent* L = FTVis::MakeText(this, Root, *FString::Printf(TEXT("Line%d"), i), FText::GetEmpty(), FVector(12.f, -90.f, 22.f - i * 28.f), FRotator::ZeroRotator, 13.f, FColor(27, 33, 64), false);
		Lines.Add(L);
		Pins.Add(FTVis::MakePart(this, Root, *FString::Printf(TEXT("Pin%d"), i), EFTShape::Sphere, FVector(12.f, -100.f, 22.f - i * 28.f), FVector(8.f), Red, FRotator::ZeroRotator, 1.f));
	}
	Clock = FTVis::MakeText(this, Root, TEXT("Clock"), FText::GetEmpty(), FVector(12.f, 10.f, -65.f), FRotator::ZeroRotator, 12.f, FColor(19, 112, 107));
}

void AFTSceneBoard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Timer -= DeltaSeconds;
	if (Timer > 0.f)
	{
		return;
	}
	Timer = 0.5f;
	const AFTGameState* GS = GSOf(this);
	if (!GS)
	{
		return;
	}
	const UFTFilmDefinition* Film = GS->GetFilm();
	Title->SetText(Film ? Film->Title : LOCTEXT("NoScript2", "NO SCRIPT YET - OPEN THE BOOK"));
	for (int32 i = 0; i < Lines.Num(); ++i)
	{
		FString Line;
		FLinearColor PinColor = Grey;
		if (Film && Film->Scenes.IsValidIndex(i))
		{
			const FFTTakeResult* Best = GS->GetBestTake(i);
			if (Best)
			{
				Line = FString::Printf(TEXT("%s   DONE - %d pts"), *Film->Scenes[i].Title.ToString(), Best->Score);
				PinColor = Green;
			}
			else if (GS->ShootPhase == EFTShootPhase::Shooting && GS->SceneIndex == i)
			{
				Line = FString::Printf(TEXT("%s   >> NOW SHOOTING"), *Film->Scenes[i].Title.ToString());
				PinColor = Red;
			}
			else
			{
				Line = FString::Printf(TEXT("%s"), *Film->Scenes[i].Title.ToString());
			}
		}
		Lines[i]->SetText(FText::FromString(Line));
		FTVis::Paint(Pins[i], PinColor, PinColor == Red ? 6.f : 1.f);
	}
	Clock->SetText(FText::Format(LOCTEXT("BoardClock", "NOW {0}   -   PREMIERE AT 06:00   -   STUDIO {1}%"), GS->GetStudioClockText(), FText::AsNumber(FMath::RoundToInt(GS->StudioCondition))));
}

// ============================================================================ projector

AFTProjector::AFTProjector()
{
	PrimaryActorTick.bCanEverTick = true;
	Tags = { TEXT("Device.Projector") };
	FTVis::MakePart(this, Root, TEXT("Stand"), EFTShape::Box, FVector(0.f, 0.f, 45.f), FVector(70.f, 70.f, 90.f), Charcoal, FRotator::ZeroRotator, 0.f, true);
	FTVis::MakePart(this, Root, TEXT("Housing"), EFTShape::Box, FVector(0.f, 0.f, 125.f), FVector(100.f, 56.f, 70.f), GreyDark);
	FTVis::MakePart(this, Root, TEXT("HousingTrim"), EFTShape::Box, FVector(0.f, 0.f, 162.f), FVector(104.f, 60.f, 6.f), Amber);
	FTVis::MakePart(this, Root, TEXT("Barrel"), EFTShape::Cylinder, FVector(68.f, 0.f, 125.f), FVector(34.f, 34.f, 40.f), Charcoal, FRotator(-90.f, 0.f, 0.f));
	LensGlow = FTVis::MakePart(this, Root, TEXT("LensGlow"), EFTShape::Cylinder, FVector(89.f, 0.f, 125.f), FVector(26.f, 26.f, 2.f), Cream, FRotator(-90.f, 0.f, 0.f), 0.3f);
	FTVis::MakePart(this, Root, TEXT("ArmA"), EFTShape::Box, FVector(22.f, 0.f, 185.f), FVector(8.f, 8.f, 50.f), Grey, FRotator(-20.f, 0.f, 0.f));
	FTVis::MakePart(this, Root, TEXT("ArmB"), EFTShape::Box, FVector(-26.f, 0.f, 185.f), FVector(8.f, 8.f, 50.f), Grey, FRotator(20.f, 0.f, 0.f));
	ReelSpinA = CreateDefaultSubobject<USceneComponent>(TEXT("ReelSpinA"));
	ReelSpinA->SetupAttachment(Root);
	ReelSpinA->SetRelativeLocation(FVector(34.f, 0.f, 222.f));
	ReelSpinB = CreateDefaultSubobject<USceneComponent>(TEXT("ReelSpinB"));
	ReelSpinB->SetupAttachment(Root);
	ReelSpinB->SetRelativeLocation(FVector(-38.f, 0.f, 222.f));
	FTVis::MakePart(this, ReelSpinA, TEXT("ReelA"), EFTShape::Torus, FVector::ZeroVector, FVector(70.f, 70.f, 12.f), Grey, FRotator(0.f, 0.f, 90.f));
	FTVis::MakePart(this, ReelSpinA, TEXT("ReelASpoke"), EFTShape::Box, FVector::ZeroVector, FVector(56.f, 6.f, 8.f), Grey);
	FTVis::MakePart(this, ReelSpinB, TEXT("ReelB"), EFTShape::Torus, FVector::ZeroVector, FVector(70.f, 70.f, 12.f), Grey, FRotator(0.f, 0.f, 90.f));
	FTVis::MakePart(this, ReelSpinB, TEXT("ReelBSpoke"), EFTShape::Box, FVector::ZeroVector, FVector(56.f, 6.f, 8.f), Grey);
	for (int32 i = 0; i < 3; ++i)
	{
		FTVis::MakePart(this, Root, *FString::Printf(TEXT("SlotPeg%d"), i), EFTShape::Cylinder, FVector(-10.f + i * 24.f, -36.f, 110.f), FVector(6.f, 6.f, 20.f), Yellow, FRotator(0.f, 0.f, 90.f));
		UStaticMeshComponent* R = FTVis::MakePart(this, Root, *FString::Printf(TEXT("SlotReel%d"), i), EFTShape::Torus, FVector(-10.f + i * 24.f, -44.f, 110.f), FVector(22.f, 22.f, 8.f), Cream, FRotator(0.f, 0.f, 90.f));
		R->SetVisibility(false);
		SlotReels.Add(R);
	}
	FTVis::MakeText(this, Root, TEXT("ReelSign"), LOCTEXT("ReelSlots", "REELS"), FVector(0.f, -32.f, 140.f), FRotator(0.f, -90.f, 0.f), 10.f, FColor(255, 200, 100));
	FTVis::MakePart(this, Root, TEXT("LeverBase"), EFTShape::Box, FVector(-60.f, 30.f, 110.f), FVector(20.f, 20.f, 20.f), Charcoal);
	FTVis::MakePart(this, Root, TEXT("Lever"), EFTShape::Capsule, FVector(-66.f, 30.f, 135.f), FVector(8.f, 8.f, 40.f), Red, FRotator(20.f, 0.f, 0.f));

	BeamRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BeamRoot"));
	BeamRoot->SetupAttachment(Root);
	BeamRoot->SetRelativeLocation(FVector(92.f, 0.f, 125.f));
	Beam = FTVis::MakePart(this, BeamRoot, TEXT("Beam"), EFTShape::Cone, FVector(1000.f, 0.f, 0.f), FVector(400.f, 400.f, 2000.f), Cream, FRotator(90.f, 0.f, 0.f), 0.25f);
	Beam->SetMaterial(0, FTVis::Glow());
	Beam->SetCastShadow(false);
	Beam->SetVisibility(false);
	BeamLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("BeamLight"));
	BeamLight->SetupAttachment(BeamRoot);
	BeamLight->SetIntensity(150000.f);
	BeamLight->SetInnerConeAngle(10.f);
	BeamLight->SetOuterConeAngle(16.f);
	BeamLight->SetAttenuationRadius(5000.f);
	BeamLight->SetCastShadows(false);
	BeamLight->SetVisibility(false);

	PowerPanel = CreateDefaultSubobject<USceneComponent>(TEXT("PowerPanel"));
	PowerPanel->SetupAttachment(Root);
	FTVis::MakePart(this, PowerPanel, TEXT("PowerBox"), EFTShape::Box, FVector(0.f, 0.f, 140.f), FVector(20.f, 70.f, 90.f), Yellow);
	FTVis::MakeText(this, PowerPanel, TEXT("PowerText"), LOCTEXT("ProjPower", "PROJECTOR POWER"), FVector(12.f, 0.f, 196.f), FRotator::ZeroRotator, 9.f, FColor(255, 230, 150));
	PowerLamp = FTVis::MakePart(this, PowerPanel, TEXT("PowerLampPart"), EFTShape::Sphere, FVector(12.f, 22.f, 170.f), FVector(10.f), Red, FRotator::ZeroRotator, 6.f);
	FTVis::MakePart(this, PowerPanel, TEXT("PowerSwitchPart"), EFTShape::Box, FVector(14.f, -6.f, 135.f), FVector(8.f, 16.f, 30.f), Charcoal);

	LoadReel = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("LoadReel"));
	LoadReel->SetupAttachment(Root);
	LoadReel->Setup(TEXT("Load"), LOCTEXT("ProjectorLabel", "Projector"), LOCTEXT("LoadVerb", "Load reel"), EFTInteractType::Press, FVector(55.f, 40.f, 60.f));
	LoadReel->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	StartLever = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("StartLever"));
	StartLever->SetupAttachment(Root);
	StartLever->Setup(TEXT("Start"), LOCTEXT("StartLabel", "Premiere lever"), LOCTEXT("StartVerb", "Start the premiere"), EFTInteractType::Hold, FVector(24.f, 24.f, 34.f));
	StartLever->HoldTime = 1.2f;
	StartLever->SetRelativeLocation(FVector(-64.f, 30.f, 130.f));
	PowerSwitch = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("PowerSwitch"));
	PowerSwitch->SetupAttachment(PowerPanel);
	PowerSwitch->Setup(TEXT("Power"), LOCTEXT("PowerLabel", "Projector power"), LOCTEXT("PowerVerb", "Restore power"), EFTInteractType::Hold, FVector(26.f, 40.f, 50.f));
	PowerSwitch->HoldTime = 0.9f;
	PowerSwitch->SetRelativeLocation(FVector(14.f, 0.f, 140.f));
}

void AFTProjector::BeginPlay()
{
	Super::BeginPlay();
	PowerPanel->SetRelativeLocation(PowerPanelOffset);
	for (TActorIterator<AFTCinemaScreen> It(GetWorld()); It; ++It)
	{
		const FVector From = BeamRoot->GetComponentLocation();
		const FVector To = It->GetActorLocation();
		const float Dist = FVector::Dist(From, To);
		BeamRoot->SetWorldRotation((To - From).Rotation());
		FTVis::ApplyShape(Beam, EFTShape::Cone, FVector(It->ScreenSize.X * 0.85f, It->ScreenSize.X * 0.85f, Dist));
		Beam->SetRelativeLocation(FVector(Dist * 0.5f, 0.f, 0.f));
		BeamLight->SetAttenuationRadius(Dist + 800.f);
		const float Half = FMath::RadiansToDegrees(FMath::Atan2(It->ScreenSize.X * 0.5f, Dist));
		BeamLight->SetOuterConeAngle(Half);
		BeamLight->SetInnerConeAngle(Half * 0.8f);
		bAimed = true;
		break;
	}
}

bool AFTProjector::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	const AFTGameState* GS = GSOf(this);
	if (!GS || (GS->ShootPhase != EFTShootPhase::Finale))
	{
		OutReason = GS && GS->ShootPhase == EFTShootPhase::Premiere ? LOCTEXT("Showing", "Now showing - enjoy!") : LOCTEXT("NotFinale", "Finish shooting the film first");
		return false;
	}
	if (Comp == LoadReel)
	{
		if (!User || !Cast<AFTProp_Reel>(User->HeldProp))
		{
			OutReason = LOCTEXT("NeedReel", "Carry a finished film reel here");
			return false;
		}
		return true;
	}
	if (Comp == PowerSwitch && GS->bProjectorPower)
	{
		OutReason = LOCTEXT("PowerOk", "Projector power is on");
		return false;
	}
	return true;
}

FText AFTProjector::GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const
{
	if (Comp == LoadReel && User)
	{
		if (const AFTProp_Reel* R = Cast<AFTProp_Reel>(User->HeldProp))
		{
			return FText::Format(LOCTEXT("LoadN", "Load scene {0} reel"), FText::AsNumber(R->ReelSceneIndex + 1));
		}
	}
	return Comp ? Comp->Verb : FText::GetEmpty();
}

void AFTProjector::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	AFTSceneManager* SM = GetSceneManager();
	if (!SM || !User)
	{
		return;
	}
	if (Comp == LoadReel)
	{
		if (AFTProp_Reel* R = Cast<AFTProp_Reel>(User->HeldProp))
		{
			const int32 Index = R->ReelSceneIndex;
			User->HeldProp = nullptr;
			User->ForceNetUpdate();
			R->Destroy();
			SM->OnReelLoaded(Index);
			MulticastSound(EFTSound::Stamp, GetActorLocation() + FVector(0.f, 0.f, 150.f), 1.f, 0.8f);
		}
	}
	else if (Comp == PowerSwitch)
	{
		SM->OnProjectorPowered();
		MulticastSound(EFTSound::PowerUp, PowerPanel->GetComponentLocation(), 1.f, 1.f);
	}
	else if (Comp == StartLever)
	{
		FText Reason;
		if (!SM->RequestStartPremiere(User, Reason))
		{
			User->ClientRejected(Reason);
		}
	}
}

void AFTProjector::OnShootPhaseChanged(EFTShootPhase Phase)
{
	const bool bShow = Phase == EFTShootPhase::Premiere;
	Beam->SetVisibility(bShow && bAimed);
	BeamLight->SetVisibility(bShow);
	FTVis::SetGlow(LensGlow, bShow ? 30.f : 0.3f);
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (bShow && !Loop)
	{
		Loop = FTAudio::Attach(Root, EFTSound::ProjectorLoop, 0.6f);
	}
	else if (!bShow && Loop)
	{
		Loop->Stop();
		Loop->DestroyComponent();
		Loop = nullptr;
	}
}

void AFTProjector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AFTGameState* GS = GSOf(this);
	if (!GS)
	{
		return;
	}
	for (int32 i = 0; i < SlotReels.Num(); ++i)
	{
		SlotReels[i]->SetVisibility(i < GS->ReelsLoaded);
	}
	FTVis::Paint(PowerLamp, GS->bProjectorPower ? Green : Red, 6.f);
	if (GS->ShootPhase == EFTShootPhase::Premiere)
	{
		ReelSpinA->AddLocalRotation(FRotator(0.f, 0.f, 0.f));
		ReelSpinA->AddLocalRotation(FRotator(-180.f * DeltaSeconds, 0.f, 0.f));
		ReelSpinB->AddLocalRotation(FRotator(-180.f * DeltaSeconds, 0.f, 0.f));
		const float Flicker = 0.85f + 0.15f * FMath::Sin(GetWorld()->GetTimeSeconds() * 48.f);
		BeamLight->SetIntensity(150000.f * Flicker);
	}
}

// ============================================================================ cinema screen

AFTCinemaScreen::AFTCinemaScreen()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	FTVis::MakePart(this, Root, TEXT("FrameTop"), EFTShape::Box, FVector(-10.f, 0.f, 480.f), FVector(40.f, 1720.f, 60.f), Charcoal);
	FTVis::MakePart(this, Root, TEXT("FrameBottom"), EFTShape::Box, FVector(-10.f, 0.f, -480.f), FVector(40.f, 1720.f, 60.f), Charcoal);
	FTVis::MakePart(this, Root, TEXT("FrameL"), EFTShape::Box, FVector(-10.f, -830.f, 0.f), FVector(40.f, 60.f, 1000.f), Charcoal);
	FTVis::MakePart(this, Root, TEXT("FrameR"), EFTShape::Box, FVector(-10.f, 830.f, 0.f), FVector(40.f, 60.f, 1000.f), Charcoal);
	FTVis::MakePart(this, Root, TEXT("Backing"), EFTShape::Cube, FVector(-24.f, 0.f, 0.f), FVector(10.f, 1640.f, 940.f), Hex(0x151A33));
	FTVis::MakePart(this, Root, TEXT("Valance"), EFTShape::Box, FVector(10.f, 0.f, 540.f), FVector(40.f, 1900.f, 90.f), Hex(0xB02A3A));
	CurtainL = FTVis::MakePart(this, Root, TEXT("CurtainL"), EFTShape::Box, FVector(20.f, -420.f, 0.f), FVector(20.f, 860.f, 1000.f), Hex(0xC9304A));
	CurtainR = FTVis::MakePart(this, Root, TEXT("CurtainR"), EFTShape::Box, FVector(20.f, 420.f, 0.f), FVector(20.f, 860.f, 1000.f), Hex(0xC9304A));
	Screen = CreateDefaultSubobject<UWidgetComponent>(TEXT("Screen"));
	Screen->SetupAttachment(Root);
	Screen->SetRelativeLocation(FVector(-8.f, 0.f, 0.f));
	Screen->SetWidgetSpace(EWidgetSpace::World);
	Screen->SetDrawSize(FVector2D(1920.f, 1080.f));
	Screen->SetWidgetClass(UFTPremiereWidget::StaticClass());
	Screen->SetRelativeScale3D(FVector(1.f, 1600.f / 1920.f, 900.f / 1080.f));
	Screen->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Screen->SetBlendMode(EWidgetBlendMode::Opaque);
	Screen->SetTwoSided(false);
	Spill = CreateDefaultSubobject<URectLightComponent>(TEXT("Spill"));
	Spill->SetupAttachment(Root);
	Spill->SetRelativeLocation(FVector(40.f, 0.f, 0.f));
	Spill->SetSourceWidth(1400.f);
	Spill->SetSourceHeight(800.f);
	Spill->SetIntensity(0.f);
	Spill->SetAttenuationRadius(3000.f);
	Spill->SetCastShadows(false);
}

void AFTCinemaScreen::BeginPlay()
{
	Super::BeginPlay();
	Screen->SetRelativeScale3D(FVector(1.f, ScreenSize.X / 1920.f, ScreenSize.Y / 1080.f));
}

void AFTCinemaScreen::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AFTGameState* GS = GSOf(this);
	const bool bShow = GS && (GS->ShootPhase == EFTShootPhase::Premiere || GS->ShootPhase == EFTShootPhase::Results);
	Open = FMath::FInterpTo(Open, bShow ? 1.f : 0.f, DeltaSeconds, 1.2f);
	CurtainL->SetRelativeLocation(FVector(20.f, -420.f - Open * 520.f, 0.f));
	CurtainR->SetRelativeLocation(FVector(20.f, 420.f + Open * 520.f, 0.f));
	Spill->SetIntensity(Open * 20.f);
}

// ============================================================================ ambient rain

AFTAmbientRain::AFTAmbientRain()
{
	bReplicates = false;
	Rain = CreateDefaultSubobject<UFTChunkyParticles>(TEXT("Rain"));
	Rain->SetupAttachment(Root);
	Rain->SetRelativeLocation(FVector(0.f, 0.f, 900.f));
	Rain->MaxParticles = 500;
	Rain->SpawnRate = 320.f;
	Rain->Lifetime = FVector2D(1.f, 1.2f);
	Rain->BaseVelocity = FVector(60.f, 30.f, -950.f);
	Rain->VelocityJitter = FVector(10.f, 10.f, 80.f);
	Rain->StartSize = FVector2D(1.4f, 2.f);
	Rain->Stretch = 0.03f;
	Rain->bSpin = false;
	Rain->GrowInTime = 0.f;
}

void AFTAmbientRain::BeginPlay()
{
	Super::BeginPlay();
	Rain->SpawnExtent = Extent;
	Rain->FloorZ = GetActorLocation().Z;
	Rain->Configure(EFTShape::Cube, Hex(0x9FD8FF), 0.8f, false);
	if (GetNetMode() != NM_DedicatedServer)
	{
		Rain->SetEmitting(true);
		Loop = FTAudio::Attach(Root, EFTSound::RainLoop, 0.45f);
	}
}

#undef LOCTEXT_NAMESPACE
