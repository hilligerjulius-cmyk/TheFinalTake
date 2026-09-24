#include "TheFinalTake/Props/FTSetPieces.h"

#include "TheFinalTake/Core/FTVisuals.h"
#include "TheFinalTake/Characters/FTCharacter.h"
#include "TheFinalTake/FX/FTChunkyParticles.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Interaction/FTInteractableComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "FinalTakeSetPieces"

using namespace FTColors;

// ============================================================================ rescue boat

AFTRescueBoat::AFTRescueBoat()
{
	PrimaryActorTick.bCanEverTick = true;
	Tags = { FTTags::PropBoat, FTTags::SubjBoat };
	// slipway + trolley stay behind
	FTVis::MakePart(this, Root, TEXT("SlipL"), EFTShape::Box, FVector(120.f, -60.f, 6.f), FVector(420.f, 14.f, 12.f), WoodDark);
	FTVis::MakePart(this, Root, TEXT("SlipR"), EFTShape::Box, FVector(120.f, 60.f, 6.f), FVector(420.f, 14.f, 12.f), WoodDark);
	FTVis::MakePart(this, Root, TEXT("Trolley"), EFTShape::Box, FVector(0.f, 0.f, 18.f), FVector(220.f, 110.f, 12.f), Grey);
	for (int32 i = 0; i < 4; ++i)
	{
		FTVis::MakePart(this, Root, *FString::Printf(TEXT("TrolleyWheel%d"), i), EFTShape::Cylinder, FVector(i < 2 ? -80.f : 80.f, i % 2 ? 55.f : -55.f, 12.f), FVector(22.f, 22.f, 10.f), Charcoal, FRotator(0.f, 0.f, 90.f));
	}
	FTVis::MakeText(this, Root, TEXT("SlipSign"), LOCTEXT("BoatSign", "BOAT BAY  -  PUSH INTO TANK"), FVector(-150.f, 0.f, 20.f), FRotator(90.f, 0.f, 180.f), 18.f, FColor(255, 200, 90));

	BoatRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BoatRoot"));
	BoatRoot->SetupAttachment(Root);
	BoatRoot->SetRelativeLocation(FVector(0.f, 0.f, 24.f));
	const FLinearColor Hull = Hex(0xFF7A2E);
	// hull and deck are solid so the crew can climb in and ride along (the boat is a movable base)
	FTVis::MakePart(this, BoatRoot, TEXT("TubeL"), EFTShape::Capsule, FVector(0.f, -58.f, 28.f), FVector(52.f, 52.f, 290.f), Hull, FRotator(-90.f, 0.f, 0.f), 0.f, true);
	FTVis::MakePart(this, BoatRoot, TEXT("TubeR"), EFTShape::Capsule, FVector(0.f, 58.f, 28.f), FVector(52.f, 52.f, 290.f), Hull, FRotator(-90.f, 0.f, 0.f), 0.f, true);
	FTVis::MakePart(this, BoatRoot, TEXT("Bow"), EFTShape::Ball, FVector(135.f, 0.f, 28.f), FVector(70.f, 150.f, 52.f), Hull, FRotator::ZeroRotator, 0.f, true);
	FTVis::MakePart(this, BoatRoot, TEXT("Floor"), EFTShape::Box, FVector(0.f, 0.f, 10.f), FVector(250.f, 100.f, 12.f), GreyDark, FRotator::ZeroRotator, 0.f, true);
	FTVis::MakePart(this, BoatRoot, TEXT("Seat"), EFTShape::Box, FVector(10.f, 0.f, 36.f), FVector(34.f, 110.f, 8.f), WoodDark);
	FTVis::MakePart(this, BoatRoot, TEXT("Transom"), EFTShape::Box, FVector(-138.f, 0.f, 30.f), FVector(10.f, 110.f, 40.f), GreyDark);
	FTVis::MakePart(this, BoatRoot, TEXT("Motor"), EFTShape::Box, FVector(-160.f, 0.f, 60.f), FVector(36.f, 30.f, 44.f), Charcoal);
	FTVis::MakePart(this, BoatRoot, TEXT("MotorCap"), EFTShape::Box, FVector(-160.f, 0.f, 86.f), FVector(38.f, 32.f, 10.f), Coral);
	FTVis::MakePart(this, BoatRoot, TEXT("MotorLeg"), EFTShape::Box, FVector(-166.f, 0.f, 18.f), FVector(10.f, 10.f, 50.f), Charcoal);
	FTVis::MakePart(this, BoatRoot, TEXT("RopeL"), EFTShape::Box, FVector(0.f, -84.f, 30.f), FVector(200.f, 3.f, 3.f), Cream);
	FTVis::MakePart(this, BoatRoot, TEXT("RopeR"), EFTShape::Box, FVector(0.f, 84.f, 30.f), FVector(200.f, 3.f, 3.f), Cream);
	FTVis::MakeText(this, BoatRoot, TEXT("Name"), LOCTEXT("BoatName", "RESCUE 1"), FVector(20.f, -86.f, 30.f), FRotator(0.f, -90.f, 0.f), 16.f, FColor::White);
	HandleL = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("HandleL"));
	HandleL->SetupAttachment(BoatRoot);
	HandleL->Setup(TEXT("PushL"), LOCTEXT("BoatLabel", "Rescue boat"), LOCTEXT("Push", "Push"), EFTInteractType::Continuous, FVector(50.f, 50.f, 50.f));
	HandleL->SetRelativeLocation(FVector(-150.f, -60.f, 40.f));
	HandleL->MaxDistance = 360.f;
	HandleR = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("HandleR"));
	HandleR->SetupAttachment(BoatRoot);
	HandleR->Setup(TEXT("PushR"), LOCTEXT("BoatLabel2", "Rescue boat"), LOCTEXT("Push2", "Push"), EFTInteractType::Continuous, FVector(50.f, 50.f, 50.f));
	HandleR->SetRelativeLocation(FVector(-150.f, 60.f, 40.f));
	HandleR->MaxDistance = 360.f;
	Splash = CreateDefaultSubobject<UFTChunkyParticles>(TEXT("Splash"));
	Splash->SetupAttachment(BoatRoot);
	Splash->MaxParticles = 90;
	Splash->Lifetime = FVector2D(0.6f, 1.f);
	Splash->SpawnExtent = FVector(120.f, 60.f, 5.f);
	Splash->BaseVelocity = FVector(0.f, 0.f, 380.f);
	Splash->VelocityJitter = FVector(200.f, 200.f, 150.f);
	Splash->Gravity = 1100.f;
	Splash->StartSize = FVector2D(10.f, 22.f);
}

void AFTRescueBoat::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTRescueBoat, Progress);
	DOREPLIFETIME(AFTRescueBoat, PusherCount);
}

bool AFTRescueBoat::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	if (Progress >= 1.f)
	{
		OutReason = LOCTEXT("InWater", "The boat is already in the water");
		return false;
	}
	if (User && User->HeldProp)
	{
		OutReason = LOCTEXT("HandsFull", "You need both hands - drop what you carry (Q)");
		return false;
	}
	return true;
}

FText AFTRescueBoat::GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const
{
	return PusherCount >= 1 ? LOCTEXT("PushTogether", "Hold to push (2 crew = faster!)") : LOCTEXT("PushHold", "Hold to push");
}

void AFTRescueBoat::OnBeginUse(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	Pushers.AddUnique(User);
	PusherCount = Pushers.Num();
}

void AFTRescueBoat::OnEndUse(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	Pushers.Remove(User);
	PusherCount = Pushers.Num();
}

void AFTRescueBoat::OnUserLeft(AFTCharacter* User)
{
	OnEndUse(nullptr, User);
}

void AFTRescueBoat::ResetForNewShoot()
{
	for (const TWeakObjectPtr<AFTCharacter>& P : Pushers)
	{
		if (AFTCharacter* C = P.Get())
		{
			if (C->UsingActor == this)
			{
				C->SetUsingActor(nullptr);
			}
		}
	}
	Pushers.Reset();
	PusherCount = 0;
	Progress = 0.f;
	bSplashed = false;
}

void AFTRescueBoat::OnSceneEvent(FName Event, AActor* Source)
{
	if (Event == FTTags::EvSharkAttackBoat)
	{
		MulticastRock();
	}
}

void AFTRescueBoat::MulticastRock_Implementation()
{
	RockTime = GetWorld()->GetTimeSeconds();
}

bool AFTRescueBoat::GetSubjectBounds(FName SubjectTag, FVector& OutCenter, float& OutRadius) const
{
	OutCenter = BoatRoot->GetComponentLocation() + FVector(0.f, 0.f, 40.f);
	OutRadius = 170.f;
	return true;
}

void AFTRescueBoat::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		for (int32 i = Pushers.Num() - 1; i >= 0; --i)
		{
			AFTCharacter* C = Pushers[i].Get();
			const float D = C ? FMath::Min(FVector::Dist(C->GetActorLocation(), HandleL->GetComponentLocation()), FVector::Dist(C->GetActorLocation(), HandleR->GetComponentLocation())) : 99999.f;
			if (!C || C->UsingActor != this || D > 420.f || Progress >= 1.f)
			{
				if (C && C->UsingActor == this)
				{
					C->SetUsingActor(nullptr);
				}
				Pushers.RemoveAt(i);
			}
		}
		PusherCount = Pushers.Num();
		if (PusherCount > 0 && Progress < 1.f)
		{
			const float Rate = PusherCount >= 2 ? 0.17f : 0.075f;
			Progress = FMath::Min(1.f, Progress + Rate * DeltaSeconds);
			SqueakTimer -= DeltaSeconds;
			if (SqueakTimer <= 0.f)
			{
				SqueakTimer = 0.9f;
				MulticastSound(EFTSound::Squeak, BoatRoot->GetComponentLocation(), 0.5f, FMath::FRandRange(0.9f, 1.2f));
			}
		}
	}

	const float P = Progress;
	const FVector Base = FMath::Lerp(FVector(0.f, 0.f, 24.f), PathEnd, P);
	const float Arc = FMath::Sin(FMath::Clamp((P - 0.25f) / 0.65f, 0.f, 1.f) * PI) * ArcHeight;
	float Bob = 0.f, Roll = 0.f;
	const float Time = GetWorld()->GetTimeSeconds();
	if (P >= 1.f)
	{
		Bob = FMath::Sin(Time * 1.4f) * 4.f;
		Roll = FMath::Sin(Time * 1.1f) * 3.f;
		const float RockT = Time - RockTime;
		if (RockT < 2.5f)
		{
			Roll += FMath::Sin(RockT * 14.f) * 14.f * (1.f - RockT / 2.5f);
		}
	}
	BoatRoot->SetRelativeLocation(Base + FVector(0.f, 0.f, Arc + Bob));
	BoatRoot->SetRelativeRotation(FRotator(P > 0.2f && P < 0.95f ? -6.f : 0.f, 0.f, Roll));
	if (P >= 0.97f && !bSplashed)
	{
		bSplashed = true;
		if (GetNetMode() != NM_DedicatedServer)
		{
			Splash->Configure(EFTShape::Sphere, Hex(0xBFEFFF), 0.4f, false);
			Splash->FloorZ = BoatRoot->GetComponentLocation().Z - 20.f;
			Splash->Burst(60);
			FTAudio::PlayAt(this, EFTSound::Splash, BoatRoot->GetComponentLocation(), 1.f, 0.9f);
		}
	}
	else if (P < 0.5f)
	{
		bSplashed = false;
	}
}

// ============================================================================ costume rack

EFTCostume AFTCostumeRack::CostumeForIndex(int32 Index)
{
	switch (Index)
	{
	case 0: return EFTCostume::Lifeguard;
	case 1: return EFTCostume::Shark;
	case 2: return EFTCostume::Raincoat;
	case 3: return EFTCostume::FoamKnight;
	default: return EFTCostume::None;
	}
}

AFTCostumeRack::AFTCostumeRack()
{
	FTVis::MakePart(this, Root, TEXT("PostL"), EFTShape::Cylinder, FVector(0.f, -230.f, 95.f), FVector(8.f, 8.f, 190.f), Grey, FRotator::ZeroRotator, 0.f, true);
	FTVis::MakePart(this, Root, TEXT("PostR"), EFTShape::Cylinder, FVector(0.f, 230.f, 95.f), FVector(8.f, 8.f, 190.f), Grey, FRotator::ZeroRotator, 0.f, true);
	FTVis::MakePart(this, Root, TEXT("Bar"), EFTShape::Cylinder, FVector(0.f, 0.f, 188.f), FVector(8.f, 8.f, 470.f), Grey, FRotator(0.f, 0.f, 90.f));
	FTVis::MakePart(this, Root, TEXT("FootL"), EFTShape::Box, FVector(0.f, -230.f, 4.f), FVector(70.f, 10.f, 8.f), GreyDark);
	FTVis::MakePart(this, Root, TEXT("FootR"), EFTShape::Box, FVector(0.f, 230.f, 4.f), FVector(70.f, 10.f, 8.f), GreyDark);
	FTVis::MakePart(this, Root, TEXT("Header"), EFTShape::Box, FVector(0.f, 0.f, 222.f), FVector(8.f, 260.f, 40.f), Magenta);
	FTVis::MakeText(this, Root, TEXT("HeaderText"), LOCTEXT("Wardrobe", "WARDROBE"), FVector(5.f, 0.f, 222.f), FRotator::ZeroRotator, 24.f, FColor::White);

	const FText Names[5] = { LOCTEXT("Lifeguard", "Lifeguard"), LOCTEXT("Shark", "Shark suit"), LOCTEXT("Raincoat", "Raincoat"), LOCTEXT("Knight", "Foam knight"), LOCTEXT("Work", "Work clothes") };
	for (int32 i = 0; i < 5; ++i)
	{
		const float Y = -180.f + i * 90.f;
		const FString P = FString::Printf(TEXT("H%d"), i);
		FTVis::MakePart(this, Root, *(P + TEXT("Hook")), EFTShape::Torus, FVector(0.f, Y, 178.f), FVector(12.f, 12.f, 3.f), Grey, FRotator(0.f, 0.f, 90.f));
		FTVis::MakePart(this, Root, *(P + TEXT("Hanger")), EFTShape::Prism, FVector(0.f, Y, 162.f), FVector(4.f, 56.f, 16.f), Wood, FRotator(0.f, 90.f, 0.f));
		switch (i)
		{
		case 0:
			FTVis::MakePart(this, Root, *(P + TEXT("A")), EFTShape::Box, FVector(0.f, Y, 128.f), FVector(10.f, 50.f, 56.f), White);
			FTVis::MakePart(this, Root, *(P + TEXT("B")), EFTShape::Box, FVector(0.f, Y, 88.f), FVector(12.f, 54.f, 30.f), Red);
			FTVis::MakePart(this, Root, *(P + TEXT("C")), EFTShape::Torus, FVector(8.f, Y, 120.f), FVector(40.f, 40.f, 10.f), Coral, FRotator(0.f, 0.f, 90.f));
			break;
		case 1:
			FTVis::MakePart(this, Root, *(P + TEXT("A")), EFTShape::Box, FVector(0.f, Y, 110.f), FVector(16.f, 60.f, 96.f), Blue);
			FTVis::MakePart(this, Root, *(P + TEXT("B")), EFTShape::Box, FVector(8.f, Y, 110.f), FVector(4.f, 34.f, 70.f), White);
			FTVis::MakePart(this, Root, *(P + TEXT("C")), EFTShape::Prism, FVector(-6.f, Y, 172.f), FVector(6.f, 30.f, 28.f), Blue, FRotator(0.f, 90.f, 0.f));
			break;
		case 2:
			FTVis::MakePart(this, Root, *(P + TEXT("A")), EFTShape::Box, FVector(0.f, Y, 105.f), FVector(14.f, 56.f, 104.f), Yellow);
			FTVis::MakePart(this, Root, *(P + TEXT("B")), EFTShape::Box, FVector(8.f, Y, 120.f), FVector(4.f, 4.f, 60.f), Charcoal);
			break;
		case 3:
			FTVis::MakePart(this, Root, *(P + TEXT("A")), EFTShape::Box, FVector(0.f, Y, 118.f), FVector(18.f, 56.f, 80.f), Grey);
			FTVis::MakePart(this, Root, *(P + TEXT("B")), EFTShape::Box, FVector(10.f, Y + 18.f, 104.f), FVector(6.f, 30.f, 40.f), Red);
			FTVis::MakePart(this, Root, *(P + TEXT("C")), EFTShape::Prism, FVector(0.f, Y, 172.f), FVector(6.f, 26.f, 22.f), Red, FRotator(0.f, 90.f, 0.f));
			break;
		default:
			FTVis::MakePart(this, Root, *(P + TEXT("A")), EFTShape::Box, FVector(0.f, Y, 120.f), FVector(12.f, 52.f, 70.f), Teal);
			FTVis::MakePart(this, Root, *(P + TEXT("B")), EFTShape::Box, FVector(7.f, Y, 138.f), FVector(4.f, 30.f, 6.f), Cream);
			break;
		}
		FTVis::MakeText(this, Root, *(P + TEXT("Label")), Names[i], FVector(14.f, Y, 48.f), FRotator::ZeroRotator, 12.f, FColor(255, 240, 210));
		UFTInteractableComponent* I = CreateDefaultSubobject<UFTInteractableComponent>(*(P + TEXT("Use")));
		I->SetupAttachment(Root);
		I->Setup(*P, Names[i], LOCTEXT("Wear", "Wear"), EFTInteractType::Press, FVector(28.f, 38.f, 70.f));
		I->SetRelativeLocation(FVector(10.f, Y, 115.f));
		Hangers.Add(I);
	}
}

FText AFTCostumeRack::GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const
{
	const EFTCostume C = CostumeForIndex(Hangers.IndexOfByKey(Comp));
	if (User && Cast<AFTStandIn>(User->HeldProp))
	{
		return LOCTEXT("Dress", "Dress stand-in as");
	}
	if (C == EFTCostume::None)
	{
		return LOCTEXT("ChangeBack", "Change into");
	}
	return User && User->Costume == C ? LOCTEXT("TakeOff", "Take off") : LOCTEXT("WearVerb", "Wear");
}

void AFTCostumeRack::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	if (!User)
	{
		return;
	}
	const EFTCostume C = CostumeForIndex(Hangers.IndexOfByKey(Comp));
	if (AFTStandIn* S = Cast<AFTStandIn>(User->HeldProp))
	{
		S->SetCostume(C);
	}
	else
	{
		User->EquipCostume(User->Costume == C ? EFTCostume::None : C);
	}
	MulticastSound(EFTSound::Rustle, Comp->GetComponentLocation(), 1.f, 1.f);
}

// ============================================================================ stand-in

AFTStandIn::AFTStandIn()
{
	PropTag = TEXT("Prop.StandIn");
	PropName = LOCTEXT("StandIn", "Cardboard Stand-in");
	Tags = { TEXT("Prop.StandIn") };
	bFloats = true;
	FloatSink = 30.f;
	CarryOffset = FVector(60.f, 70.f, -130.f);
	CarryRotation = FRotator(0.f, -70.f, 0.f);
	auto Part = [this](const TCHAR* Name, EFTShape Shape, FVector Loc, FVector Size, FLinearColor C, FRotator R = FRotator::ZeroRotator)
	{
		return FTVis::MakePart(this, Visual, Name, Shape, Loc, Size, C, R);
	};
	// stand
	Part(TEXT("Base"), EFTShape::Box, FVector(-10.f, 0.f, 3.f), FVector(60.f, 60.f, 6.f), WoodDark);
	Part(TEXT("Brace"), EFTShape::Box, FVector(-24.f, 0.f, 70.f), FVector(6.f, 10.f, 140.f), Wood, FRotator(-18.f, 0.f, 0.f));
	// cardboard figure (faces +X)
	Plain.Add(Part(TEXT("Body"), EFTShape::Box, FVector(0.f, 0.f, 95.f), FVector(6.f, 50.f, 110.f), Coral));
	Part(TEXT("Legs"), EFTShape::Box, FVector(0.f, 0.f, 30.f), FVector(6.f, 40.f, 40.f), CoralDark);
	Part(TEXT("Face"), EFTShape::Cylinder, FVector(1.f, 0.f, 172.f), FVector(52.f, 52.f, 6.f), SkinA, FRotator(90.f, 0.f, 0.f));
	Part(TEXT("EyeL"), EFTShape::Cylinder, FVector(5.f, -9.f, 178.f), FVector(9.f, 9.f, 2.f), Ink, FRotator(90.f, 0.f, 0.f));
	Part(TEXT("EyeR"), EFTShape::Cylinder, FVector(5.f, 9.f, 178.f), FVector(9.f, 9.f, 2.f), Ink, FRotator(90.f, 0.f, 0.f));
	Part(TEXT("Smile"), EFTShape::Box, FVector(5.f, 0.f, 160.f), FVector(2.f, 16.f, 3.f), CoralDark);
	Plain.Add(Part(TEXT("Hair"), EFTShape::Box, FVector(0.f, 0.f, 196.f), FVector(7.f, 50.f, 12.f), HairBrown));
	Part(TEXT("ArmL"), EFTShape::Box, FVector(0.f, -32.f, 100.f), FVector(6.f, 12.f, 70.f), SkinA, FRotator(0.f, 0.f, 12.f));
	Part(TEXT("ArmR"), EFTShape::Box, FVector(0.f, 32.f, 110.f), FVector(6.f, 12.f, 70.f), SkinA, FRotator(0.f, 0.f, -40.f));
	// costume overlays
	Lifeguard.Add(Part(TEXT("LGTop"), EFTShape::Box, FVector(4.f, 0.f, 110.f), FVector(4.f, 48.f, 60.f), White));
	Lifeguard.Add(Part(TEXT("LGShorts"), EFTShape::Box, FVector(4.f, 0.f, 58.f), FVector(4.f, 50.f, 30.f), Red));
	Lifeguard.Add(Part(TEXT("LGHat"), EFTShape::Box, FVector(4.f, 0.f, 202.f), FVector(4.f, 70.f, 12.f), Red));
	Lifeguard.Add(Part(TEXT("LGCrown"), EFTShape::Box, FVector(4.f, 0.f, 214.f), FVector(4.f, 40.f, 16.f), Red));
	Shark.Add(Part(TEXT("SHHood"), EFTShape::Cylinder, FVector(-3.f, 0.f, 176.f), FVector(80.f, 80.f, 4.f), Blue, FRotator(90.f, 0.f, 0.f)));
	Shark.Add(Part(TEXT("SHFin"), EFTShape::Prism, FVector(-3.f, 0.f, 226.f), FVector(4.f, 40.f, 34.f), Blue, FRotator(0.f, 90.f, 0.f)));
	Shark.Add(Part(TEXT("SHBody"), EFTShape::Box, FVector(4.f, 0.f, 95.f), FVector(4.f, 54.f, 112.f), Blue));
	Shark.Add(Part(TEXT("SHBelly"), EFTShape::Box, FVector(6.f, 0.f, 95.f), FVector(3.f, 30.f, 80.f), White));
	Shark.Add(Part(TEXT("SHJaw"), EFTShape::Torus, FVector(6.f, 0.f, 172.f), FVector(62.f, 62.f, 6.f), White, FRotator(90.f, 0.f, 0.f)));
	Raincoat.Add(Part(TEXT("RCCoat"), EFTShape::Box, FVector(4.f, 0.f, 85.f), FVector(4.f, 56.f, 130.f), Yellow));
	Raincoat.Add(Part(TEXT("RCHood"), EFTShape::Cylinder, FVector(-3.f, 0.f, 180.f), FVector(70.f, 70.f, 4.f), Yellow, FRotator(90.f, 0.f, 0.f)));
	Knight.Add(Part(TEXT("FKBody"), EFTShape::Box, FVector(4.f, 0.f, 100.f), FVector(4.f, 54.f, 100.f), Grey));
	Knight.Add(Part(TEXT("FKHelm"), EFTShape::Box, FVector(-2.f, 0.f, 180.f), FVector(4.f, 64.f, 66.f), Grey));
	Knight.Add(Part(TEXT("FKVisor"), EFTShape::Box, FVector(4.f, 0.f, 184.f), FVector(3.f, 44.f, 8.f), Ink));
	Knight.Add(Part(TEXT("FKPlume"), EFTShape::Prism, FVector(-2.f, 0.f, 226.f), FVector(4.f, 30.f, 26.f), Red, FRotator(0.f, 90.f, 0.f)));
	Knight.Add(Part(TEXT("FKShield"), EFTShape::Box, FVector(8.f, -30.f, 90.f), FVector(4.f, 34.f, 44.f), Red));

	HandAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("HandAnchor"));
	HandAnchor->SetupAttachment(Visual);
	HandAnchor->SetRelativeLocation(FVector(14.f, 50.f, 130.f));

	Grab->SetBoxExtent(FVector(20.f, 30.f, 60.f));
	Grab->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	CostumeButton = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("CostumeButton"));
	CostumeButton->SetupAttachment(Root);
	CostumeButton->Setup(TEXT("Costume"), LOCTEXT("StandInCostume", "Stand-in costume"), LOCTEXT("NextCostume", "Change"), EFTInteractType::Press, FVector(16.f, 34.f, 16.f));
	CostumeButton->SetRelativeLocation(FVector(22.f, 0.f, 16.f));
	HandButton = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("HandButton"));
	HandButton->SetupAttachment(Root);
	HandButton->Setup(TEXT("Hand"), LOCTEXT("StandInHand", "Stand-in's hand"), LOCTEXT("HandOver", "Hand over"), EFTInteractType::Press, FVector(18.f, 18.f, 22.f));
	HandButton->SetRelativeLocation(FVector(10.f, 50.f, 130.f));
	CostumeLabel = FTVis::MakeText(this, Visual, TEXT("CostumeLabel"), LOCTEXT("StandInSign", "STAND-IN"), FVector(28.f, 0.f, 12.f), FRotator::ZeroRotator, 9.f, FColor(255, 230, 150));
}

void AFTStandIn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTStandIn, Costume);
	DOREPLIFETIME(AFTStandIn, HeldItem);
}

void AFTStandIn::BeginPlay()
{
	Super::BeginPlay();
	OnRep_Costume();
}

bool AFTStandIn::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	if (Comp == HandButton)
	{
		if (!HeldItem && (!User || !User->HeldProp || User->HeldProp == this))
		{
			OutReason = LOCTEXT("NeedProp", "Bring a prop to hand over");
			return false;
		}
		return true;
	}
	if (Comp == CostumeButton)
	{
		return true;
	}
	return Super::CanInteract(Comp, User, OutReason);
}

FText AFTStandIn::GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const
{
	if (Comp == HandButton)
	{
		return HeldItem ? FText::Format(LOCTEXT("TakeBack", "Take back the {0}"), HeldItem->PropName) : LOCTEXT("GiveProp", "Hand over your prop");
	}
	if (Comp == CostumeButton)
	{
		return LOCTEXT("ChangeCostume", "Change costume");
	}
	return Super::GetPromptVerb(Comp, User);
}

void AFTStandIn::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	if (Comp == CostumeButton)
	{
		SetCostume(static_cast<EFTCostume>((static_cast<uint8>(Costume) + 1) % 5));
		MulticastSound(EFTSound::Rustle, GetActorLocation(), 1.f, 1.1f);
		return;
	}
	if (Comp == HandButton && User)
	{
		if (HeldItem)
		{
			AFTProp* P = HeldItem;
			User->TakeProp(P);
		}
		else if (User->HeldProp && User->HeldProp != this)
		{
			AFTProp* P = User->HeldProp;
			User->HeldProp = nullptr;
			User->ForceNetUpdate();
			P->AttachToHolder(HandAnchor, this);
			HeldItem = P;
			ForceNetUpdate();
		}
		return;
	}
	Super::OnInteract(Comp, User);
}

void AFTStandIn::SetCostume(EFTCostume NewCostume)
{
	Costume = NewCostume;
	OnRep_Costume();
	ForceNetUpdate();
}

void AFTStandIn::OnRep_Costume()
{
	auto Show = [](const TArray<TObjectPtr<UStaticMeshComponent>>& Set, bool bOn)
	{
		for (UStaticMeshComponent* C : Set)
		{
			if (C)
			{
				C->SetVisibility(bOn);
			}
		}
	};
	Show(Lifeguard, Costume == EFTCostume::Lifeguard);
	Show(Shark, Costume == EFTCostume::Shark);
	Show(Raincoat, Costume == EFTCostume::Raincoat);
	Show(Knight, Costume == EFTCostume::FoamKnight);
	static const TCHAR* Names[] = { TEXT("STAND-IN"), TEXT("STAND-IN: LIFEGUARD"), TEXT("STAND-IN: SHARK"), TEXT("STAND-IN: RAINCOAT"), TEXT("STAND-IN: KNIGHT") };
	CostumeLabel->SetText(FText::FromString(Names[FMath::Clamp((int32)Costume, 0, 4)]));
}

void AFTStandIn::ResetForNewShoot()
{
	SetCostume(EFTCostume::None);
	HeldItem = nullptr;
	Super::ResetForNewShoot();
}

// ============================================================================ lost & found

AFTPropShelf::AFTPropShelf()
{
	FTVis::MakePart(this, Root, TEXT("Frame"), EFTShape::Box, FVector(0.f, 0.f, 100.f), FVector(50.f, 180.f, 200.f), Wood, FRotator::ZeroRotator, 0.f, true);
	for (int32 i = 0; i < 3; ++i)
	{
		FTVis::MakePart(this, Root, *FString::Printf(TEXT("Shelf%d"), i), EFTShape::Box, FVector(10.f, 0.f, 40.f + i * 60.f), FVector(40.f, 170.f, 6.f), WoodDark);
		FTVis::MakePart(this, Root, *FString::Printf(TEXT("Box%d"), i), EFTShape::Box, FVector(10.f, -50.f + i * 45.f, 60.f + i * 60.f), FVector(30.f, 34.f, 30.f), i == 1 ? Teal : Cream);
	}
	FTVis::MakePart(this, Root, TEXT("Sign"), EFTShape::Box, FVector(26.f, 0.f, 212.f), FVector(4.f, 160.f, 34.f), Yellow);
	FTVis::MakeText(this, Root, TEXT("SignText"), LOCTEXT("LostFound", "LOST & FOUND"), FVector(29.f, 0.f, 212.f), FRotator::ZeroRotator, 20.f, FColor(30, 30, 50));
	UFTInteractableComponent* I = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("Recall"));
	I->SetupAttachment(Root);
	I->Setup(TEXT("Recall"), LOCTEXT("ShelfLabel", "Lost & Found"), LOCTEXT("RecallVerb", "Recall lost props"), EFTInteractType::Hold, FVector(34.f, 90.f, 100.f));
	I->HoldTime = 0.8f;
	I->SetRelativeLocation(FVector(20.f, 0.f, 100.f));
}

bool AFTPropShelf::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	if (GetWorld() && GetWorld()->GetTimeSeconds() - LastUse < 5.f)
	{
		OutReason = LOCTEXT("ShelfCooldown", "Give the props a second to arrive...");
		return false;
	}
	return true;
}

void AFTPropShelf::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	LastUse = GetWorld()->GetTimeSeconds();
	int32 Count = 0;
	for (TActorIterator<AFTProp> It(GetWorld()); It; ++It)
	{
		AFTProp* P = *It;
		if (P->IsCritical() && !P->GetCarrier() && !P->GetHolder() && (P->IsFloating() || P->DistanceFromHome() > 400.f) && !P->bDestroyOnReset)
		{
			P->ReturnHome(false);
			++Count;
		}
	}
	Announce(Count > 0 ? FText::Format(LOCTEXT("Recalled", "Lost & Found returned {0} prop(s) to their shelves."), FText::AsNumber(Count))
		: LOCTEXT("NothingLost", "Nothing is lost right now."), EFTAnnounceStyle::Info, EFTSound::UIConfirm);
}

// ============================================================================ reel tray

AFTReelTray::AFTReelTray()
{
	Tags = { TEXT("Spawn.ReelTray") };
	FTVis::MakePart(this, Root, TEXT("Table"), EFTShape::Box, FVector(0.f, 0.f, 40.f), FVector(60.f, 190.f, 10.f), TealDark, FRotator::ZeroRotator, 0.f, true);
	FTVis::MakePart(this, Root, TEXT("LegA"), EFTShape::Box, FVector(0.f, -85.f, 18.f), FVector(50.f, 8.f, 36.f), Charcoal);
	FTVis::MakePart(this, Root, TEXT("LegB"), EFTShape::Box, FVector(0.f, 85.f, 18.f), FVector(50.f, 8.f, 36.f), Charcoal);
	for (int32 i = 0; i < 3; ++i)
	{
		FTVis::MakePart(this, Root, *FString::Printf(TEXT("Slot%d"), i), EFTShape::Box, FVector(0.f, (i - 1) * 55.f, 46.f), FVector(40.f, 12.f, 3.f), Yellow);
	}
	FTVis::MakePart(this, Root, TEXT("Sign"), EFTShape::Box, FVector(-28.f, 0.f, 80.f), FVector(4.f, 120.f, 36.f), Cream);
	FTVis::MakeText(this, Root, TEXT("SignText"), LOCTEXT("Reels", "FINISHED REELS"), FVector(-25.f, 0.f, 80.f), FRotator::ZeroRotator, 12.f, FColor(30, 30, 50));
}

#undef LOCTEXT_NAMESPACE
