#include "TheFinalTake/Characters/FTCharacter.h"

#include "TheFinalTake/Core/FTVisuals.h"
#include "TheFinalTake/Core/FTAudio.h"
#include "TheFinalTake/Core/FTInput.h"
#include "TheFinalTake/Core/FTSettings.h"
#include "TheFinalTake/Interaction/FTInteractableComponent.h"
#include "TheFinalTake/Interaction/FTStudioActor.h"
#include "TheFinalTake/Props/FTProp.h"
#include "TheFinalTake/Production/FTFilmCamera.h"
#include "TheFinalTake/World/FTFloodController.h"
#include "TheFinalTake/World/FTZone.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Game/FTPlayerState.h"
#include "TheFinalTake/Game/FTPlayerController.h"
#include "TheFinalTake/Game/FTSceneManager.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "FinalTakeCharacter"

namespace
{
	struct FCrewLook
	{
		FLinearColor Skin, Hair, Shirt, Pants, Sleeve, Glove, Shoe, Cap, Brim, Bib, Glasses;
		bool bCap, bGlasses, bPhones, bBib;
		int32 HairStyle; // 0 short, 1 bun, 2 curly
	};

	FCrewLook GetCrewLook(int32 Index)
	{
		using namespace FTColors;
		switch (((Index % 4) + 4) % 4)
		{
		// gloves stay bright: in first person they fill the lower corners, dark gloves read as rocks
		case 0: return { SkinA, HairBrown, Teal, Teal, Teal, Cream, Cream, Teal, TealDark, Teal, Coral, true, false, false, false, 0 };
		case 1: return { SkinD, HairBrown, Coral, Coral, Coral, SkinD, Cream, Coral, Coral, Coral, CoralDark, false, true, false, false, 1 };
		case 2: return { SkinC, HairBlack, Yellow, Yellow, Yellow, Coral, Charcoal, Yellow, Amber, Yellow, Charcoal, true, false, true, false, 0 };
		default: return { SkinA, HairGinger, Cream, TealLight, Cream, Teal, TealLight, Cream, Teal, TealLight, Coral, true, false, false, true, 2 };
		}
	}

	FVector Sz(float X, float Y, float Z) { return FVector(X, Y, Z); }
}

AFTCharacter::AFTCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SetNetUpdateFrequency(60.f);

	GetCapsuleComponent()->InitCapsuleSize(36.f, 88.f);
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = 430.f;
	Move->JumpZVelocity = 520.f;
	Move->AirControl = 0.45f;
	Move->BrakingDecelerationWalking = 2200.f;
	Move->GroundFriction = 9.f;
	Move->MaxStepHeight = 42.f;
	Move->SetWalkableFloorAngle(50.f);
	Move->bOrientRotationToMovement = false;
	Move->GetNavAgentPropertiesRef().bCanCrouch = false;

	GetMesh()->SetHiddenInGame(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(4.f, 0.f, 62.f));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetFieldOfView(92.f);

	BuildBody();
}

void AFTCharacter::BuildBody()
{
	using namespace FTColors;
	const FCrewLook L = GetCrewLook(0);

	// ---------------------------------------------------------------- first-person arms (owner only)
	FPArms = CreateDefaultSubobject<USceneComponent>(TEXT("FPArms"));
	FPArms->SetupAttachment(FirstPersonCamera);
	FPArmPivotL = CreateDefaultSubobject<USceneComponent>(TEXT("FPArmPivotL"));
	FPArmPivotL->SetupAttachment(FPArms);
	FPArmPivotL->SetRelativeLocation(FVector(14.f, -24.f, -30.f));
	FPArmPivotR = CreateDefaultSubobject<USceneComponent>(TEXT("FPArmPivotR"));
	FPArmPivotR->SetupAttachment(FPArms);
	FPArmPivotR->SetRelativeLocation(FVector(14.f, 24.f, -30.f));

	auto FP = [this](FName Name, USceneComponent* Pivot, EFTShape Shape, FVector Loc, FVector Size, FLinearColor C, FRotator R)
	{
		UStaticMeshComponent* P = FTVis::MakePart(this, Pivot, Name, Shape, Loc, Size, C, R);
		P->SetOnlyOwnerSee(true);
		P->SetCastShadow(false);
		P->bVisibleInReflectionCaptures = false;
		return P;
	};
	FPSleeveL = FP(TEXT("FPSleeveL"), FPArmPivotL, EFTShape::Capsule, FVector(14.f, 0.f, 0.f), Sz(18.f, 18.f, 40.f), L.Sleeve, FRotator(-90.f, 0.f, 0.f));
	FPSleeveR = FP(TEXT("FPSleeveR"), FPArmPivotR, EFTShape::Capsule, FVector(14.f, 0.f, 0.f), Sz(18.f, 18.f, 40.f), L.Sleeve, FRotator(-90.f, 0.f, 0.f));
	FPCuffL = FP(TEXT("FPCuffL"), FPArmPivotL, EFTShape::Cylinder, FVector(31.f, 0.f, 0.f), Sz(11.f, 11.f, 4.f), Cream, FRotator(-90.f, 0.f, 0.f));
	FPCuffR = FP(TEXT("FPCuffR"), FPArmPivotR, EFTShape::Cylinder, FVector(31.f, 0.f, 0.f), Sz(11.f, 11.f, 4.f), Cream, FRotator(-90.f, 0.f, 0.f));
	FPHandL = FP(TEXT("FPHandL"), FPArmPivotL, EFTShape::Ball, FVector(39.f, 0.f, 0.f), Sz(17.f, 12.f, 10.f), L.Glove, FRotator::ZeroRotator);
	FPHandR = FP(TEXT("FPHandR"), FPArmPivotR, EFTShape::Ball, FVector(39.f, 0.f, 0.f), Sz(17.f, 12.f, 10.f), L.Glove, FRotator::ZeroRotator);
	FPThumbL = FP(TEXT("FPThumbL"), FPArmPivotL, EFTShape::Ball, FVector(36.f, 5.f, -2.f), Sz(10.f, 6.f, 6.f), L.Glove, FRotator(0.f, 32.f, 0.f));
	FPThumbR = FP(TEXT("FPThumbR"), FPArmPivotR, EFTShape::Ball, FVector(36.f, -5.f, -2.f), Sz(10.f, 6.f, 6.f), L.Glove, FRotator(0.f, -32.f, 0.f));
	// Sewn knuckle pads and finger breaks, kept below the centre of the first-person view.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		USceneComponent* Pivot = Side == 0 ? FPArmPivotL.Get() : FPArmPivotR.Get();
		FP(*FString::Printf(TEXT("FPKnucklePad%d"), Side), Pivot, EFTShape::Box,
			FVector(39.f, 0.f, 4.3f), Sz(8.f, 8.f, 1.2f), CreamDark, FRotator::ZeroRotator);
		for (int32 Finger = 0; Finger < 3; ++Finger)
		{
			FP(*FString::Printf(TEXT("FPFingerSeam%d_%d"), Side, Finger), Pivot, EFTShape::Box,
				FVector(44.f, -2.6f + Finger * 2.6f, 2.5f), Sz(3.5f, 0.35f, 0.5f), WoodDark, FRotator::ZeroRotator);
		}
	}

	// ---------------------------------------------------------------- third-person body
	BodyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BodyRoot"));
	BodyRoot->SetupAttachment(GetCapsuleComponent());
	BodyRoot->SetRelativeLocation(FVector(0.f, 0.f, -88.f));

	Hips = CreateDefaultSubobject<USceneComponent>(TEXT("Hips"));
	Hips->SetupAttachment(BodyRoot);
	Hips->SetRelativeLocation(FVector(0.f, 0.f, 46.f));
	Chest = CreateDefaultSubobject<USceneComponent>(TEXT("Chest"));
	Chest->SetupAttachment(Hips);
	Chest->SetRelativeLocation(FVector(0.f, 0.f, 8.f));
	Neck = CreateDefaultSubobject<USceneComponent>(TEXT("Neck"));
	Neck->SetupAttachment(Chest);
	Neck->SetRelativeLocation(FVector(0.f, 0.f, 46.f));
	ShoulderL = CreateDefaultSubobject<USceneComponent>(TEXT("ShoulderL"));
	ShoulderL->SetupAttachment(Chest);
	ShoulderL->SetRelativeLocation(FVector(0.f, -25.f, 38.f));
	ShoulderR = CreateDefaultSubobject<USceneComponent>(TEXT("ShoulderR"));
	ShoulderR->SetupAttachment(Chest);
	ShoulderR->SetRelativeLocation(FVector(0.f, 25.f, 38.f));
	HipL = CreateDefaultSubobject<USceneComponent>(TEXT("HipL"));
	HipL->SetupAttachment(Hips);
	HipL->SetRelativeLocation(FVector(0.f, -10.f, 0.f));
	HipR = CreateDefaultSubobject<USceneComponent>(TEXT("HipR"));
	HipR->SetupAttachment(Hips);
	HipR->SetRelativeLocation(FVector(0.f, 10.f, 0.f));

	auto B = [this](FName Name, USceneComponent* Parent, EFTShape Shape, FVector Loc, FVector Size, FLinearColor C, FRotator R = FRotator::ZeroRotator, bool bOwnerSees = false)
	{
		UStaticMeshComponent* P = FTVis::MakePart(this, Parent, Name, Shape, Loc, Size, C, R);
		if (!bOwnerSees)
		{
			P->SetOwnerNoSee(true);
			P->bCastHiddenShadow = true;
		}
		return P;
	};

	Pelvis = B(TEXT("Pelvis"), Hips, EFTShape::Box, FVector(0.f, 0.f, 2.f), Sz(28.f, 34.f, 16.f), L.Pants);
	LegL = B(TEXT("LegL"), HipL, EFTShape::Capsule, FVector(0.f, 0.f, -21.f), Sz(25.f, 25.f, 44.f), L.Pants, FRotator::ZeroRotator, true);
	LegR = B(TEXT("LegR"), HipR, EFTShape::Capsule, FVector(0.f, 0.f, -21.f), Sz(25.f, 25.f, 44.f), L.Pants, FRotator::ZeroRotator, true);
	CuffL = B(TEXT("CuffL"), HipL, EFTShape::Cylinder, FVector(0.f, 0.f, -33.f), Sz(18.f, 18.f, 7.f), L.Pants, FRotator::ZeroRotator, true);
	CuffR = B(TEXT("CuffR"), HipR, EFTShape::Cylinder, FVector(0.f, 0.f, -33.f), Sz(18.f, 18.f, 7.f), L.Pants, FRotator::ZeroRotator, true);
	ShoeL = B(TEXT("ShoeL"), HipL, EFTShape::Box, FVector(4.f, 0.f, -40.f), Sz(25.f, 15.f, 11.f), L.Shoe, FRotator::ZeroRotator, true);
	ShoeR = B(TEXT("ShoeR"), HipR, EFTShape::Box, FVector(4.f, 0.f, -40.f), Sz(25.f, 15.f, 11.f), L.Shoe, FRotator::ZeroRotator, true);

	Torso = B(TEXT("Torso"), Chest, EFTShape::CrewTorso, FVector(0.f, 0.f, 22.f), Sz(32.f, 46.f, 44.f), L.Shirt);
	Belt = B(TEXT("Belt"), Chest, EFTShape::Box, FVector(0.f, 0.f, 2.f), Sz(32.f, 42.f, 6.f), Charcoal);
	Walkie = B(TEXT("Walkie"), Chest, EFTShape::Box, FVector(4.f, 22.f, 5.f), Sz(6.f, 5.f, 12.f), Charcoal);
	Collar = B(TEXT("Collar"), Chest, EFTShape::Box, FVector(1.f, 0.f, 43.f), Sz(26.f, 30.f, 5.f), Cream);
	Bib = B(TEXT("Bib"), Chest, EFTShape::Box, FVector(15.5f, 0.f, 18.f), Sz(3.f, 26.f, 26.f), L.Bib);
	B(TEXT("CrewBeltBuckle"), Chest, EFTShape::Box, FVector(17.f, 0.f, 2.f), Sz(3.f, 9.f, 6.f), CreamDark);
	B(TEXT("CrewRadioAntenna"), Chest, EFTShape::Capsule, FVector(3.f, 22.f, 16.f), Sz(2.f, 2.f, 14.f), Charcoal);
	B(TEXT("CrewRadioScreen"), Chest, EFTShape::Box, FVector(7.2f, 22.f, 7.f), Sz(1.f, 3.5f, 4.f), TealLight);
	B(TEXT("CrewPocket"), Chest, EFTShape::Box, FVector(16.f, -11.f, 27.f), Sz(4.f, 12.f, 14.f), TealDark);
	B(TEXT("CrewPocketFlap"), Chest, EFTShape::Box, FVector(18.f, -11.f, 32.f), Sz(2.f, 13.f, 4.f), CreamDark);
	B(TEXT("CrewPencil"), Chest, EFTShape::Cylinder, FVector(18.f, -14.f, 37.f), Sz(2.f, 2.f, 11.f), Yellow);
	B(TEXT("CrewBadge"), Chest, EFTShape::Box, FVector(16.f, 10.f, 34.f), Sz(2.f, 11.f, 7.f), Cream);
	B(TEXT("CrewBadgeStripe"), Chest, EFTShape::Box, FVector(17.2f, 10.f, 35.f), Sz(0.8f, 8.f, 2.f), Coral);
	for (int32 Side = 0; Side < 2; ++Side)
	{
		USceneComponent* Hip = Side == 0 ? HipL.Get() : HipR.Get();
		B(*FString::Printf(TEXT("CrewSole%d"), Side), Hip, EFTShape::Box,
			FVector(4.f, 0.f, -44.f), Sz(26.f, 16.f, 3.f), CreamDark, FRotator::ZeroRotator, true);
		for (int32 Lace = 0; Lace < 3; ++Lace)
		{
			B(*FString::Printf(TEXT("CrewLace%d_%d"), Side, Lace), Hip, EFTShape::Box,
				FVector(2.f + Lace * 4.f, 0.f, -34.5f), Sz(2.f, 11.f, 1.5f), Cream, FRotator::ZeroRotator, true);
		}
	}

	ArmL = B(TEXT("ArmL"), ShoulderL, EFTShape::Capsule, FVector(0.f, 0.f, -17.f), Sz(23.f, 23.f, 40.f), L.Sleeve);
	ArmR = B(TEXT("ArmR"), ShoulderR, EFTShape::Capsule, FVector(0.f, 0.f, -17.f), Sz(23.f, 23.f, 40.f), L.Sleeve);
	HandL = B(TEXT("HandL"), ShoulderL, EFTShape::Sphere, FVector(0.f, 0.f, -39.f), Sz(16.f, 15.f, 16.f), L.Glove);
	HandR = B(TEXT("HandR"), ShoulderR, EFTShape::Sphere, FVector(0.f, 0.f, -39.f), Sz(16.f, 15.f, 16.f), L.Glove);

	Head = B(TEXT("Head"), Neck, EFTShape::Ball, FVector(0.f, 0.f, 21.f), Sz(46.f, 44.f, 44.f), L.Skin);
	EyeL = B(TEXT("EyeL"), Neck, EFTShape::Ball, FVector(19.5f, -8.5f, 24.f), Sz(7.f, 8.f, 11.f), White);
	EyeR = B(TEXT("EyeR"), Neck, EFTShape::Ball, FVector(19.5f, 8.5f, 24.f), Sz(7.f, 8.f, 11.f), White);
	PupilL = B(TEXT("PupilL"), Neck, EFTShape::Ball, FVector(22.8f, -8.5f, 23.5f), Sz(3.f, 5.f, 7.f), Ink);
	PupilR = B(TEXT("PupilR"), Neck, EFTShape::Ball, FVector(22.8f, 8.5f, 23.5f), Sz(3.f, 5.f, 7.f), Ink);
	BrowL = B(TEXT("BrowL"), Neck, EFTShape::Box, FVector(20.5f, -8.5f, 31.5f), Sz(3.f, 10.f, 2.5f), L.Hair);
	BrowR = B(TEXT("BrowR"), Neck, EFTShape::Box, FVector(20.5f, 8.5f, 31.5f), Sz(3.f, 10.f, 2.5f), L.Hair);
	Mouth = B(TEXT("Mouth"), Neck, EFTShape::Box, FVector(21.5f, 0.f, 12.f), Sz(3.f, 11.f, 3.f), CoralDark);
	Nose = B(TEXT("Nose"), Neck, EFTShape::Sphere, FVector(22.5f, 0.f, 18.f), Sz(7.f, 6.f, 6.f), L.Skin);
	EarL = B(TEXT("EarL"), Neck, EFTShape::Sphere, FVector(0.f, -22.f, 20.f), Sz(7.f, 4.f, 10.f), L.Skin);
	EarR = B(TEXT("EarR"), Neck, EFTShape::Sphere, FVector(0.f, 22.f, 20.f), Sz(7.f, 4.f, 10.f), L.Skin);
	HairTop = B(TEXT("HairTop"), Neck, EFTShape::Ball, FVector(-2.f, 0.f, 33.f), Sz(47.f, 46.f, 26.f), L.Hair);
	HairBack = B(TEXT("HairBack"), Neck, EFTShape::Ball, FVector(-8.f, 0.f, 22.f), Sz(36.f, 45.f, 34.f), L.Hair);
	HairBun = B(TEXT("HairBun"), Neck, EFTShape::Sphere, FVector(-13.f, 0.f, 43.f), Sz(19.f, 19.f, 19.f), L.Hair);
	CapCrown = B(TEXT("CapCrown"), Neck, EFTShape::Ball, FVector(0.f, 0.f, 35.f), Sz(49.f, 47.f, 25.f), L.Cap);
	CapBrim = B(TEXT("CapBrim"), Neck, EFTShape::Box, FVector(22.f, 0.f, 33.f), Sz(22.f, 32.f, 3.f), L.Brim, FRotator(-8.f, 0.f, 0.f));
	CapBadge = B(TEXT("CapBadge"), Neck, EFTShape::Box, FVector(20.5f, 0.f, 40.f), Sz(2.f, 9.f, 7.f), Cream);
	GlassL = B(TEXT("GlassL"), Neck, EFTShape::Torus, FVector(23.f, -8.5f, 24.f), Sz(15.f, 15.f, 4.f), L.Glasses, FRotator(90.f, 0.f, 0.f));
	GlassR = B(TEXT("GlassR"), Neck, EFTShape::Torus, FVector(23.f, 8.5f, 24.f), Sz(15.f, 15.f, 4.f), L.Glasses, FRotator(90.f, 0.f, 0.f));
	PhoneL = B(TEXT("PhoneL"), Neck, EFTShape::Cylinder, FVector(0.f, -24.f, 21.f), Sz(15.f, 15.f, 8.f), Charcoal, FRotator(0.f, 0.f, 90.f));
	PhoneR = B(TEXT("PhoneR"), Neck, EFTShape::Cylinder, FVector(0.f, 24.f, 21.f), Sz(15.f, 15.f, 8.f), Charcoal, FRotator(0.f, 0.f, 90.f));
	PhoneBand = B(TEXT("PhoneBand"), Neck, EFTShape::Box, FVector(0.f, 0.f, 44.5f), Sz(6.f, 48.f, 4.f), Charcoal);

	// ---------------------------------------------------------------- costumes
	LGHat = B(TEXT("LGHat"), Neck, EFTShape::Cylinder, FVector(0.f, 0.f, 40.f), Sz(40.f, 40.f, 14.f), Red);
	LGBrim = B(TEXT("LGBrim"), Neck, EFTShape::Cylinder, FVector(0.f, 0.f, 34.f), Sz(64.f, 64.f, 3.f), Red);
	LGWhistle = B(TEXT("LGWhistle"), Chest, EFTShape::Box, FVector(16.5f, 0.f, 30.f), Sz(3.f, 4.f, 8.f), CoralDark);
	LGShorts = B(TEXT("LGShorts"), Hips, EFTShape::Box, FVector(0.f, 0.f, -5.f), Sz(31.f, 37.f, 19.f), Red);

	SHHood = B(TEXT("SHHood"), Neck, EFTShape::Ball, FVector(-12.f, 0.f, 26.f), Sz(62.f, 60.f, 62.f), Blue);
	SHJaw = B(TEXT("SHJaw"), Neck, EFTShape::Torus, FVector(17.f, 0.f, 22.f), Sz(52.f, 52.f, 10.f), White, FRotator(90.f, 0.f, 0.f));
	SHFin = B(TEXT("SHFin"), Neck, EFTShape::Prism, FVector(-16.f, 0.f, 62.f), Sz(6.f, 30.f, 28.f), Blue, FRotator(0.f, 90.f, 0.f));
	SHTail = B(TEXT("SHTail"), Hips, EFTShape::Prism, FVector(-24.f, 0.f, 4.f), Sz(6.f, 26.f, 30.f), Blue, FRotator(-35.f, 90.f, 0.f));
	SHBelly = B(TEXT("SHBelly"), Chest, EFTShape::Box, FVector(15.5f, 0.f, 20.f), Sz(3.f, 30.f, 36.f), White);
	for (int32 i = 0; i < 6; ++i)
	{
		const bool bTop = i < 3;
		const float Y = (i % 3 - 1) * 11.f;
		UStaticMeshComponent* T = B(*FString::Printf(TEXT("SHTooth%d"), i), Neck, EFTShape::Cone, FVector(20.f, Y, bTop ? 36.f : 8.f), Sz(6.f, 6.f, 8.f), White, bTop ? FRotator(180.f, 0.f, 0.f) : FRotator::ZeroRotator);
		SHTeeth.Add(T);
	}

	RCCoat = B(TEXT("RCCoat"), Chest, EFTShape::Box, FVector(0.f, 0.f, 8.f), Sz(34.f, 44.f, 64.f), Yellow);
	RCHood = B(TEXT("RCHood"), Neck, EFTShape::Ball, FVector(-9.f, 0.f, 27.f), Sz(50.f, 50.f, 44.f), Yellow);
	for (int32 i = 0; i < 3; ++i)
	{
		RCButtons.Add(B(*FString::Printf(TEXT("RCButton%d"), i), Chest, EFTShape::Sphere, FVector(17.5f, 0.f, 32.f - i * 12.f), Sz(4.f, 4.f, 4.f), Charcoal));
	}

	FKHelmet = B(TEXT("FKHelmet"), Neck, EFTShape::Box, FVector(-5.f, 0.f, 23.f), Sz(46.f, 50.f, 52.f), Grey);
	FKVisor = B(TEXT("FKVisor"), Neck, EFTShape::Box, FVector(20.f, 0.f, 33.f), Sz(6.f, 46.f, 6.f), GreyDark);
	FKPlume = B(TEXT("FKPlume"), Neck, EFTShape::Prism, FVector(-4.f, 0.f, 58.f), Sz(6.f, 30.f, 24.f), Red, FRotator(0.f, 90.f, 0.f));
	FKPadL = B(TEXT("FKPadL"), ShoulderL, EFTShape::Sphere, FVector(0.f, -2.f, 2.f), Sz(22.f, 22.f, 18.f), Grey);
	FKPadR = B(TEXT("FKPadR"), ShoulderR, EFTShape::Sphere, FVector(0.f, 2.f, 2.f), Sz(22.f, 22.f, 18.f), Grey);
	FKShield = B(TEXT("FKShield"), ShoulderL, EFTShape::Box, FVector(10.f, -9.f, -26.f), Sz(34.f, 5.f, 42.f), Cream);
	FKShieldCross = B(TEXT("FKShieldCross"), ShoulderL, EFTShape::Box, FVector(10.f, -12.f, -26.f), Sz(12.f, 2.f, 42.f), Red);

	// ---------------------------------------------------------------- identity marker
	Marker = FTVis::MakePart(this, BodyRoot, TEXT("Marker"), EFTShape::Cone, FVector(0.f, 0.f, 178.f), Sz(16.f, 16.f, 14.f), Cyan, FRotator(180.f, 0.f, 0.f), 2.f);
	Marker->SetOwnerNoSee(true);
	Marker->SetCastShadow(false);
	NameTag = FTVis::MakeText(this, BodyRoot, TEXT("NameTag"), FText::FromString(TEXT("Crew")), FVector(0.f, 0.f, 196.f), FRotator::ZeroRotator, 13.f, FColor::White);
	NameTag->SetOwnerNoSee(true);

	CarryAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("CarryAnchor"));
	CarryAnchor->SetupAttachment(Chest);
	CarryAnchor->SetRelativeLocation(FVector(38.f, 0.f, 12.f));
}

void AFTCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTCharacter, CrewIndex);
	DOREPLIFETIME(AFTCharacter, Costume);
	DOREPLIFETIME(AFTCharacter, HeldProp);
	DOREPLIFETIME(AFTCharacter, UsingActor);
	DOREPLIFETIME(AFTCharacter, Emote);
	DOREPLIFETIME(AFTCharacter, EmoteSerial);
	DOREPLIFETIME(AFTCharacter, bKnockedDown);
	DOREPLIFETIME_CONDITION(AFTCharacter, bSprinting, COND_SkipOwner);
}

void AFTCharacter::BeginPlay()
{
	Super::BeginPlay();
	LastSafeLocation = GetActorLocation();
	BlinkTimer = FMath::FRandRange(1.f, 4.f);
	ApplyLook();
	AttachCarryAnchorForView();
}

void AFTCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (AFTPlayerState* PS = GetPlayerState<AFTPlayerState>())
	{
		CrewIndex = PS->CrewIndex;
		PS->Costume = Costume;
	}
	ApplyLook();
	AttachCarryAnchorForView();
}

void AFTCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	AttachCarryAnchorForView();
	UpdateNameTag();
}

void AFTCharacter::AttachCarryAnchorForView()
{
	if (IsLocallyControlled())
	{
		CarryAnchor->AttachToComponent(FirstPersonCamera, FAttachmentTransformRules::KeepRelativeTransform);
		CarryAnchor->SetRelativeLocation(FVector(44.f, 16.f, -26.f));
		CarryAnchor->SetRelativeRotation(FRotator::ZeroRotator);
	}
	else
	{
		CarryAnchor->AttachToComponent(Chest, FAttachmentTransformRules::KeepRelativeTransform);
		CarryAnchor->SetRelativeLocation(FVector(38.f, 0.f, 12.f));
		CarryAnchor->SetRelativeRotation(FRotator::ZeroRotator);
	}
}

void AFTCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		StopUsing();
		ReleaseProp(false);
		for (TActorIterator<AFTStudioActor> It(GetWorld()); It; ++It)
		{
			It->OnUserLeft(this);
		}
	}
	if (UFTInteractableComponent* F = Focused.Get())
	{
		F->SetHighlighted(false);
	}
	Super::EndPlay(EndPlayReason);
}

// ======================================================================== look / costumes

FLinearColor AFTCharacter::GetCrewColor() const
{
	return AFTPlayerState::CrewColor(CrewIndex);
}

FString AFTCharacter::GetCrewName() const
{
	if (const AFTPlayerState* PS = GetPlayerState<AFTPlayerState>())
	{
		return PS->GetCrewName();
	}
	return AFTPlayerState::CrewRoleName(CrewIndex);
}

void AFTCharacter::OnRep_Look()
{
	ApplyLook();
}

void AFTCharacter::ApplyLook()
{
	using namespace FTColors;
	const FCrewLook L = GetCrewLook(CrewIndex);
	FLinearColor Shirt = L.Shirt, Pants = L.Pants, Sleeve = L.Sleeve, Glove = L.Glove, Shoe = L.Shoe, Legs = L.Pants;

	switch (Costume)
	{
	case EFTCostume::Lifeguard:
		Shirt = White; Sleeve = L.Skin; Glove = L.Skin; Legs = L.Skin; Pants = Red; Shoe = Red;
		break;
	case EFTCostume::Shark:
		Shirt = Blue; Sleeve = Blue; Glove = Blue; Legs = Blue; Pants = Blue; Shoe = Blue;
		break;
	case EFTCostume::Raincoat:
		Shirt = Yellow; Sleeve = Yellow; Shoe = Yellow;
		break;
	case EFTCostume::FoamKnight:
		Shirt = Grey; Sleeve = Grey; Glove = Grey; Legs = GreyDark; Pants = GreyDark; Shoe = GreyDark;
		break;
	default:
		break;
	}

	FTVis::Paint(Torso, Shirt);
	FTVis::Paint(Pelvis, Pants);
	FTVis::Paint(LegL, Legs);
	FTVis::Paint(LegR, Legs);
	FTVis::Paint(CuffL, Costume == EFTCostume::Lifeguard ? L.Skin : (Costume == EFTCostume::None ? L.Pants * 0.85f : Legs));
	FTVis::Paint(CuffR, Costume == EFTCostume::Lifeguard ? L.Skin : (Costume == EFTCostume::None ? L.Pants * 0.85f : Legs));
	FTVis::Paint(ShoeL, Shoe);
	FTVis::Paint(ShoeR, Shoe);
	FTVis::Paint(ArmL, Sleeve);
	FTVis::Paint(ArmR, Sleeve);
	FTVis::Paint(HandL, Glove);
	FTVis::Paint(HandR, Glove);
	FTVis::Paint(Head, L.Skin);
	FTVis::Paint(Nose, L.Skin * 0.92f);
	FTVis::Paint(EarL, L.Skin);
	FTVis::Paint(EarR, L.Skin);
	FTVis::Paint(HairTop, L.Hair);
	FTVis::Paint(HairBack, L.Hair);
	FTVis::Paint(HairBun, L.Hair);
	FTVis::Paint(BrowL, L.Hair);
	FTVis::Paint(BrowR, L.Hair);
	FTVis::Paint(CapCrown, L.Cap);
	FTVis::Paint(CapBrim, L.Brim);
	FTVis::Paint(GlassL, L.Glasses);
	FTVis::Paint(GlassR, L.Glasses);
	FTVis::Paint(Bib, L.Bib);
	FTVis::Paint(Collar, Costume == EFTCostume::None ? Cream : Shirt);

	// first-person arms mirror the costume sleeves
	FTVis::Paint(FPSleeveL, Sleeve);
	FTVis::Paint(FPSleeveR, Sleeve);
	// the crew accent colour separates sleeve and glove even when both are light
	const FLinearColor Cuff = Costume == EFTCostume::Lifeguard ? L.Skin : (Costume == EFTCostume::None ? L.Glasses : Sleeve * 0.8f);
	FTVis::Paint(FPCuffL, Cuff);
	FTVis::Paint(FPCuffR, Cuff);
	FTVis::Paint(FPHandL, Glove);
	FTVis::Paint(FPHandR, Glove);
	FTVis::Paint(FPThumbL, Glove);
	FTVis::Paint(FPThumbR, Glove);

	FTVis::Paint(Marker, GetCrewColor(), 2.5f);
	SetCostumePartsVisible();
	UpdateNameTag();
}

void AFTCharacter::SetCostumePartsVisible()
{
	const FCrewLook L = GetCrewLook(CrewIndex);
	const bool bNone = Costume == EFTCostume::None;
	const bool bLG = Costume == EFTCostume::Lifeguard;
	const bool bSH = Costume == EFTCostume::Shark;
	const bool bRC = Costume == EFTCostume::Raincoat;
	const bool bFK = Costume == EFTCostume::FoamKnight;
	const bool bHeadCovered = bSH || bFK;

	auto Vis = [](UPrimitiveComponent* C, bool bOn) { if (C) { C->SetVisibility(bOn); } };

	Vis(CapCrown, L.bCap && !bHeadCovered && !bLG && !bRC);
	Vis(CapBrim, L.bCap && !bHeadCovered && !bLG && !bRC);
	Vis(CapBadge, L.bCap && !bHeadCovered && !bLG && !bRC);
	Vis(HairTop, !bHeadCovered && !(L.bCap && !bLG && !bRC) && L.HairStyle != 1);
	Vis(HairBack, !bHeadCovered);
	Vis(HairBun, !bHeadCovered && !bLG && L.HairStyle == 1);
	Vis(GlassL, L.bGlasses && !bFK);
	Vis(GlassR, L.bGlasses && !bFK);
	Vis(PhoneL, L.bPhones && !bHeadCovered);
	Vis(PhoneR, L.bPhones && !bHeadCovered);
	Vis(PhoneBand, L.bPhones && !bHeadCovered && !bLG);
	Vis(Bib, L.bBib && bNone);
	Vis(Walkie, !bSH);
	Vis(Belt, !bSH && !bRC);
	Vis(EarL, !bHeadCovered && !bRC);
	Vis(EarR, !bHeadCovered && !bRC);

	Vis(LGHat, bLG);
	Vis(LGBrim, bLG);
	Vis(LGWhistle, bLG);
	Vis(LGShorts, bLG);
	Vis(SHHood, bSH);
	Vis(SHJaw, bSH);
	Vis(SHFin, bSH);
	Vis(SHTail, bSH);
	Vis(SHBelly, bSH);
	for (UStaticMeshComponent* T : SHTeeth) { Vis(T, bSH); }
	Vis(RCCoat, bRC);
	Vis(RCHood, bRC);
	for (UStaticMeshComponent* Btn : RCButtons) { Vis(Btn, bRC); }
	Vis(FKHelmet, bFK);
	Vis(FKVisor, bFK);
	Vis(FKPlume, bFK);
	Vis(FKPadL, bFK);
	Vis(FKPadR, bFK);
	Vis(FKShield, bFK);
	Vis(FKShieldCross, bFK);
}

void AFTCharacter::UpdateNameTag()
{
	if (NameTag)
	{
		NameTag->SetText(FText::FromString(GetCrewName()));
		const FLinearColor C = GetCrewColor();
		NameTag->SetTextRenderColor(C.ToFColor(true));
	}
}

void AFTCharacter::EquipCostume(EFTCostume NewCostume)
{
	if (!HasAuthority())
	{
		return;
	}
	Costume = NewCostume;
	if (AFTPlayerState* PS = GetPlayerState<AFTPlayerState>())
	{
		PS->Costume = NewCostume;
	}
	ApplyLook();
	ForceNetUpdate();
}

// ======================================================================== props & stations

void AFTCharacter::TakeProp(AFTProp* Prop)
{
	if (!HasAuthority() || !Prop)
	{
		return;
	}
	if (HeldProp && HeldProp != Prop)
	{
		ReleaseProp(false);
	}
	HeldProp = Prop;
	Prop->PickUp(this);
	if (AFTPlayerState* PS = GetPlayerState<AFTPlayerState>())
	{
		PS->bCarrying = true;
	}
	ForceNetUpdate();
}

void AFTCharacter::ReleaseProp(bool bThrow)
{
	if (!HasAuthority() || !HeldProp)
	{
		return;
	}
	AFTProp* P = HeldProp;
	HeldProp = nullptr;
	P->Drop(bThrow);
	if (AFTPlayerState* PS = GetPlayerState<AFTPlayerState>())
	{
		PS->bCarrying = false;
	}
	ForceNetUpdate();
}

void AFTCharacter::OnRep_HeldProp()
{
	PunchAlpha = 0.6f;
}

void AFTCharacter::SetUsingActor(AFTStudioActor* Actor)
{
	UsingActor = Actor;
	if (AFTPlayerState* PS = GetPlayerState<AFTPlayerState>())
	{
		PS->bOnCamera = Cast<AFTFilmCamera>(Actor) != nullptr;
	}
	ForceNetUpdate();
}

void AFTCharacter::StopUsing()
{
	if (!HasAuthority() || !UsingActor)
	{
		return;
	}
	AFTStudioActor* A = UsingActor;
	if (AFTFilmCamera* Cam = Cast<AFTFilmCamera>(A))
	{
		Cam->Release();
	}
	else
	{
		A->OnEndUse(nullptr, this);
	}
	SetUsingActor(nullptr);
}

bool AFTCharacter::IsOperatingCamera() const
{
	return Cast<AFTFilmCamera>(UsingActor) != nullptr;
}

AFTFilmCamera* AFTCharacter::GetOperatedCamera() const
{
	return Cast<AFTFilmCamera>(UsingActor);
}

FVector AFTCharacter::GetSubjectPoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, 20.f);
}

bool AFTCharacter::IsEmoting(EFTEmote Which, float Within) const
{
	const UWorld* W = GetWorld();
	return W && Emote == Which && (W->GetTimeSeconds() - EmoteStartTime) < Within;
}

void AFTCharacter::Knockdown(const FVector& FromLocation, float Duration, bool bDropNonCritical)
{
	if (!HasAuthority() || bKnockedDown)
	{
		return;
	}
	bKnockedDown = true;
	KnockEndTime = GetWorld()->GetTimeSeconds() + Duration;
	FVector Away = GetActorLocation() - FromLocation;
	Away.Z = 0.f;
	Away = Away.GetSafeNormal();
	if (Away.IsNearlyZero())
	{
		Away = -GetActorForwardVector();
	}
	LaunchCharacter(Away * 420.f + FVector(0.f, 0.f, 320.f), true, true);
	if (HeldProp && bDropNonCritical && !HeldProp->IsCritical())
	{
		ReleaseProp(false);
	}
	if (AFTSceneManager* SM = AFTSceneManager::Get(this))
	{
		SM->ReportEvent(FTTags::EvKnockdown, this, this);
	}
	OnRep_Knocked();
	ForceNetUpdate();
}

void AFTCharacter::OnRep_Knocked()
{
	if (bKnockedDown)
	{
		FTAudio::PlayAt(this, EFTSound::Boing, GetActorLocation(), 0.9f, FMath::FRandRange(0.9f, 1.1f));
	}
}

void AFTCharacter::TeleportToSafety(const FText& Reason)
{
	if (!HasAuthority())
	{
		return;
	}
	FVector Target = LastSafeLocation;
	if (Target.IsNearlyZero())
	{
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			Target = It->GetActorLocation();
			break;
		}
	}
	TeleportTo(Target + FVector(0.f, 0.f, 20.f), GetActorRotation(), false, true);
	GetCharacterMovement()->StopMovementImmediately();
	if (!Reason.IsEmpty())
	{
		ClientFeedback(Reason);
	}
}

void AFTCharacter::PlayEmote(EFTEmote NewEmote)
{
	if (!HasAuthority())
	{
		return;
	}
	Emote = NewEmote;
	++EmoteSerial;
	OnRep_Emote();
	ForceNetUpdate();
}

void AFTCharacter::OnRep_Emote()
{
	EmoteStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
}

// ======================================================================== tick

void AFTCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateMovementSpeed();
	UpdateBodyPose(DeltaSeconds);
	UpdateFace(DeltaSeconds);

	if (IsLocallyControlled())
	{
		UpdateInteractionTarget(DeltaSeconds);
		UpdateFirstPersonArms(DeltaSeconds);
		FlushCameraInput(DeltaSeconds);
	}
	else if (NameTag)
	{
		if (APlayerController* LocalPC = GetWorld()->GetFirstPlayerController())
		{
			if (LocalPC->PlayerCameraManager)
			{
				const FVector CamLoc = LocalPC->PlayerCameraManager->GetCameraLocation();
				const FVector ToCam = CamLoc - NameTag->GetComponentLocation();
				NameTag->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
				const float Dist = ToCam.Size();
				NameTag->SetVisibility(Dist < 2500.f);
				Marker->SetVisibility(Dist > 250.f);
			}
		}
	}

	if (HasAuthority())
	{
		if (bKnockedDown && GetWorld()->GetTimeSeconds() >= KnockEndTime)
		{
			bKnockedDown = false;
			ForceNetUpdate();
		}
		UpdateRecovery();
		// props are handed over / loaded / reset from many places; derive the roster flag instead of tracking it
		if (AFTPlayerState* PS = GetPlayerState<AFTPlayerState>())
		{
			const bool bCarryingNow = HeldProp != nullptr;
			if (PS->bCarrying != bCarryingNow)
			{
				PS->bCarrying = bCarryingNow;
				PS->ForceNetUpdate();
			}
		}
	}

	// water splashes (cosmetic, local)
	if (bInWater && GetVelocity().Size2D() > 120.f)
	{
		WaterSplashTimer -= DeltaSeconds;
		if (WaterSplashTimer <= 0.f)
		{
			WaterSplashTimer = FMath::FRandRange(0.45f, 0.7f);
			FTAudio::PlayAt(this, EFTSound::Splash, GetActorLocation() - FVector(0.f, 0.f, 70.f), 0.25f, FMath::FRandRange(1.1f, 1.4f));
		}
	}
}

void AFTCharacter::UpdateMovementSpeed()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	float Speed = 430.f;
	if (bSprinting && !HeldProp)
	{
		Speed = 660.f;
	}
	else if (bSprinting)
	{
		Speed = 560.f;
	}
	bInWater = false;
	if (AFTFloodController* Flood = AFTFloodController::Get(this))
	{
		const float Depth = Flood->GetDepthAt(GetActorLocation() - FVector(0.f, 0.f, 88.f));
		bInWater = Depth > 10.f;
		if (bInWater)
		{
			Speed *= FMath::Lerp(0.85f, 0.5f, FMath::Clamp(Depth / 55.f, 0.f, 1.f));
		}
	}
	if (IsOperatingCamera() || bKnockedDown)
	{
		Speed = 0.f;
	}
	Move->MaxWalkSpeed = Speed;
}

void AFTCharacter::UpdateRecovery()
{
	const FVector Loc = GetActorLocation();
	const float Now = GetWorld()->GetTimeSeconds();
	if (Loc.Z < -2000.f || AFTZone::IsInZone(this, FTTags::ZoneRecovery, Loc))
	{
		TeleportToSafety(LOCTEXT("Recovered", "Whoops! Back on your mark."));
		return;
	}
	if (Now - LastSafeCheck > 1.f)
	{
		LastSafeCheck = Now;
		if (GetCharacterMovement()->IsMovingOnGround() && !bInWater && !bKnockedDown)
		{
			LastSafeLocation = Loc;
		}
	}
}

EFTExpression AFTCharacter::ComputeExpression() const
{
	if (bKnockedDown)
	{
		return EFTExpression::Panic;
	}
	const float Since = GetWorld()->GetTimeSeconds() - EmoteStartTime;
	if (Since < 2.5f)
	{
		switch (Emote)
		{
		case EFTEmote::Wave:
		case EFTEmote::Cheer: return EFTExpression::Happy;
		case EFTEmote::Panic: return EFTExpression::Panic;
		case EFTEmote::HeroPose: return EFTExpression::Confident;
		case EFTEmote::Point: return EFTExpression::Confident;
		default: break;
		}
	}
	if (const AFTGameState* GSt = GetWorld()->GetGameState<AFTGameState>())
	{
		if (GSt->ShootPhase == EFTShootPhase::Failed)
		{
			return EFTExpression::Exhausted;
		}
		if (GSt->ShootPhase == EFTShootPhase::Results || GSt->ShootPhase == EFTShootPhase::Premiere)
		{
			return EFTExpression::Happy;
		}
		if (bInWater && GSt->FloodStage == EFTFloodStage::Flooded)
		{
			return EFTExpression::Panic;
		}
	}
	if (HeldProp && HeldProp->PropTag == FTTags::PropHarpoon)
	{
		return EFTExpression::Confident;
	}
	if (UsingActor)
	{
		return EFTExpression::Confident;
	}
	return EFTExpression::Neutral;
}

void AFTCharacter::UpdateFace(float DeltaSeconds)
{
	if (IsLocallyControlled() && !IsOperatingCamera())
	{
		return; // own face is never visible
	}
	BlinkTimer -= DeltaSeconds;
	if (BlinkTimer <= 0.f)
	{
		BlinkAlpha = 1.f;
		BlinkTimer = FMath::FRandRange(2.f, 5.f);
	}
	BlinkAlpha = FMath::Max(0.f, BlinkAlpha - DeltaSeconds * 8.f);

	const EFTExpression Ex = ComputeExpression();
	float EyeZ = 1.f, PupilScale = 1.f;
	FVector MouthScale(3.f, 11.f, 3.f);
	float MouthZ = 12.f, MouthRoll = 0.f, BrowZ = 31.5f, BrowRollL = 0.f, BrowRollR = 0.f;
	switch (Ex)
	{
	case EFTExpression::Happy:
		MouthScale = FVector(3.f, 14.f, 5.f); BrowZ = 33.f; break;
	case EFTExpression::Panic:
		MouthScale = FVector(4.f, 9.f, 10.f); MouthZ = 10.5f; BrowZ = 34.f; BrowRollL = -18.f; BrowRollR = 18.f; PupilScale = 0.65f; EyeZ = 1.15f; break;
	case EFTExpression::Confident:
		MouthScale = FVector(3.f, 12.f, 3.f); MouthRoll = 8.f; BrowRollL = 14.f; BrowRollR = -14.f; BrowZ = 30.5f; break;
	case EFTExpression::Confused:
		MouthScale = FVector(3.f, 8.f, 3.f); MouthRoll = -12.f; BrowRollL = -10.f; break;
	case EFTExpression::Exhausted:
		EyeZ = 0.45f; MouthScale = FVector(3.f, 12.f, 2.f); MouthZ = 11.f; BrowZ = 29.5f; BrowRollL = -8.f; BrowRollR = 8.f; break;
	default: break;
	}
	EyeZ *= FMath::Lerp(1.f, 0.1f, BlinkAlpha);
	EyeL->SetRelativeScale3D(FVector(0.07f, 0.08f, 0.11f * EyeZ));
	EyeR->SetRelativeScale3D(FVector(0.07f, 0.08f, 0.11f * EyeZ));
	PupilL->SetRelativeScale3D(FVector(0.03f, 0.05f * PupilScale, 0.07f * PupilScale * EyeZ));
	PupilR->SetRelativeScale3D(FVector(0.03f, 0.05f * PupilScale, 0.07f * PupilScale * EyeZ));
	Mouth->SetRelativeScale3D(MouthScale / 100.f);
	Mouth->SetRelativeLocation(FVector(21.5f, 0.f, MouthZ));
	Mouth->SetRelativeRotation(FRotator(0.f, 0.f, MouthRoll));
	BrowL->SetRelativeLocation(FVector(20.5f, -8.5f, BrowZ));
	BrowR->SetRelativeLocation(FVector(20.5f, 8.5f, BrowZ));
	BrowL->SetRelativeRotation(FRotator(0.f, 0.f, BrowRollL));
	BrowR->SetRelativeRotation(FRotator(0.f, 0.f, BrowRollR));
}

void AFTCharacter::UpdateBodyPose(float DeltaSeconds)
{
	const UWorld* W = GetWorld();
	const float Time = W->GetTimeSeconds();
	const float Speed2D = GetVelocity().Size2D();
	SmoothedSpeed = FMath::FInterpTo(SmoothedSpeed, Speed2D, DeltaSeconds, 10.f);
	const float SpeedN = FMath::Clamp(SmoothedSpeed / 430.f, 0.f, 1.6f);
	const bool bAir = GetCharacterMovement()->IsFalling();
	if (SpeedN > 0.05f && !bAir)
	{
		WalkPhase += DeltaSeconds * (5.f + 5.5f * SpeedN);
	}

	KnockAlpha = FMath::FInterpTo(KnockAlpha, bKnockedDown ? 1.f : 0.f, DeltaSeconds, bKnockedDown ? 9.f : 4.f);

	const float Swing = FMath::Sin(WalkPhase) * FMath::Min(SpeedN, 1.3f) * 38.f;
	float LegAngleL = Swing, LegAngleR = -Swing;
	float ArmPitchL = -Swing * 0.8f, ArmPitchR = Swing * 0.8f;
	float ArmRollL = 8.f, ArmRollR = -8.f;
	float ChestPitch = -SpeedN * 7.f;
	float HipsZ = 46.f + FMath::Abs(FMath::Sin(WalkPhase)) * 3.f * FMath::Min(SpeedN, 1.f) + FMath::Sin(Time * 2.f) * 0.5f;
	float BodyYawShake = 0.f;

	if (bAir)
	{
		LegAngleL = 30.f; LegAngleR = -12.f;
		ArmRollL = 55.f; ArmRollR = -55.f;
		ArmPitchL = ArmPitchR = 10.f;
	}

	const bool bCarrying = HeldProp != nullptr;
	const bool bOperating = UsingActor != nullptr;
	if (bCarrying || bOperating)
	{
		ArmPitchL = ArmPitchR = bOperating && !bCarrying ? 82.f : 72.f;
		ArmRollL = 6.f; ArmRollR = -6.f;
		if (bOperating && !IsOperatingCamera())
		{
			ChestPitch = -16.f; // leaning into a push
		}
	}

	const float EmoteT = Time - EmoteStartTime;
	if (EmoteT < 2.6f && Emote != EFTEmote::None)
	{
		switch (Emote)
		{
		case EFTEmote::Wave:
			ArmPitchR = 0.f; ArmRollR = -150.f + FMath::Sin(EmoteT * 14.f) * 22.f; break;
		case EFTEmote::Point:
			ArmPitchR = 92.f; ArmRollR = -4.f; ArmPitchL = -10.f; ArmRollL = 35.f; ChestPitch = -4.f; break;
		case EFTEmote::Cheer:
			ArmPitchL = ArmPitchR = 0.f; ArmRollL = 150.f + FMath::Sin(EmoteT * 16.f) * 10.f; ArmRollR = -ArmRollL;
			HipsZ += FMath::Abs(FMath::Sin(EmoteT * 9.f)) * 9.f; break;
		case EFTEmote::Panic:
			ArmRollL = 120.f + FMath::Sin(EmoteT * 21.f) * 40.f; ArmRollR = -120.f - FMath::Sin(EmoteT * 19.f + 1.f) * 40.f;
			ArmPitchL = FMath::Sin(EmoteT * 17.f) * 35.f; ArmPitchR = FMath::Cos(EmoteT * 15.f) * 35.f;
			BodyYawShake = FMath::Sin(EmoteT * 35.f) * 6.f; break;
		case EFTEmote::HeroPose:
			ArmPitchR = 20.f; ArmRollR = -160.f; ArmPitchL = -15.f; ArmRollL = 38.f; ChestPitch = 6.f; break;
		default: break;
		}
	}

	if (KnockAlpha > 0.01f)
	{
		ArmRollL = FMath::Lerp(ArmRollL, 110.f, KnockAlpha);
		ArmRollR = FMath::Lerp(ArmRollR, -110.f, KnockAlpha);
		LegAngleL = FMath::Lerp(LegAngleL, 40.f, KnockAlpha);
		LegAngleR = FMath::Lerp(LegAngleR, 25.f, KnockAlpha);
	}

	HipL->SetRelativeRotation(FRotator(LegAngleL, 0.f, 0.f));
	HipR->SetRelativeRotation(FRotator(LegAngleR, 0.f, 0.f));
	ShoulderL->SetRelativeRotation(FRotator(ArmPitchL, 0.f, ArmRollL));
	ShoulderR->SetRelativeRotation(FRotator(ArmPitchR, 0.f, ArmRollR));
	Chest->SetRelativeRotation(FRotator(ChestPitch, 0.f, FMath::Sin(WalkPhase) * 3.f * FMath::Min(SpeedN, 1.f)));
	Hips->SetRelativeLocation(FVector(0.f, 0.f, HipsZ));
	BodyRoot->SetRelativeRotation(FRotator(KnockAlpha * 78.f, BodyYawShake, 0.f));
	BodyRoot->SetRelativeLocation(FVector(-KnockAlpha * 20.f, 0.f, -88.f));

	// head follows the view pitch
	const float AimPitch = FRotator::NormalizeAxis(GetBaseAimRotation().Pitch);
	const float Tilt = ComputeExpression() == EFTExpression::Confused ? 12.f : 0.f;
	Neck->SetRelativeRotation(FRotator(FMath::Clamp(AimPitch * 0.45f, -30.f, 30.f), 0.f, Tilt));
}

void AFTCharacter::UpdateFirstPersonArms(float DeltaSeconds)
{
	const bool bShow = !IsOperatingCamera();
	FPArms->SetVisibility(bShow, true);
	if (!bShow)
	{
		return;
	}
	PunchAlpha = FMath::Max(0.f, PunchAlpha - DeltaSeconds * 4.f);
	const float SpeedN = FMath::Clamp(SmoothedSpeed / 430.f, 0.f, 1.6f);
	const float BobZ = FMath::Sin(WalkPhase * 2.f) * 1.3f * FMath::Min(SpeedN, 1.2f);
	const float BobY = FMath::Cos(WalkPhase) * 1.2f * FMath::Min(SpeedN, 1.2f);
	const float Reduced = UFTSettings::Get()->bReducedScreenShake ? 0.3f : 1.f;

	FVector LBase(14.f, -24.f, -30.f), RBase(14.f, 24.f, -30.f);
	FRotator LRot(8.f, 12.f, 0.f), RRot(8.f, -12.f, 0.f);
	if (HeldProp)
	{
		LBase = FVector(22.f, -14.f, -27.f); RBase = FVector(22.f, 12.f, -27.f);
		LRot = FRotator(12.f, 28.f, 0.f); RRot = FRotator(12.f, -20.f, 0.f);
	}
	else if (UsingActor)
	{
		LBase = FVector(24.f, -16.f, -24.f); RBase = FVector(24.f, 16.f, -24.f);
		LRot = FRotator(4.f, 18.f, 0.f); RRot = FRotator(4.f, -18.f, 0.f);
	}
	if (bKnockedDown)
	{
		LBase.Z += 14.f; RBase.Z += 14.f;
		LRot.Pitch += 40.f; RRot.Pitch += 40.f;
	}
	if (HoldProgress > 0.f)
	{
		RBase.X += 6.f * HoldProgress;
	}
	RBase.X += PunchAlpha * 14.f;
	const FVector Bob(0.f, BobY * Reduced, BobZ * Reduced);
	FPArmPivotL->SetRelativeLocation(FMath::VInterpTo(FPArmPivotL->GetRelativeLocation(), LBase + Bob, DeltaSeconds, 14.f));
	FPArmPivotR->SetRelativeLocation(FMath::VInterpTo(FPArmPivotR->GetRelativeLocation(), RBase + Bob, DeltaSeconds, 14.f));
	FPArmPivotL->SetRelativeRotation(FMath::RInterpTo(FPArmPivotL->GetRelativeRotation(), LRot, DeltaSeconds, 12.f));
	FPArmPivotR->SetRelativeRotation(FMath::RInterpTo(FPArmPivotR->GetRelativeRotation(), RRot, DeltaSeconds, 12.f));

	// Local knockdown roll (gentle, respects reduced screen shake)
	const float Roll = KnockAlpha * 18.f * Reduced;
	FirstPersonCamera->SetRelativeRotation(FRotator(0.f, 0.f, Roll));
}

// ======================================================================== interaction

bool AFTCharacter::IsInputBlocked() const
{
	const AFTPlayerController* PC = Cast<AFTPlayerController>(GetController());
	return PC && PC->IsUIBlockingGameplay();
}

void AFTCharacter::UpdateInteractionTarget(float DeltaSeconds)
{
	UFTInteractableComponent* NewFocus = nullptr;
	if (!IsInputBlocked() && !IsOperatingCamera() && !bKnockedDown)
	{
		const FVector Start = FirstPersonCamera->GetComponentLocation();
		const FVector End = Start + FirstPersonCamera->GetForwardVector() * 340.f;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(FTInteract), false, this);
		if (HeldProp)
		{
			Params.AddIgnoredActor(HeldProp);
		}
		FHitResult Hit;
		if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_FTInteract, FCollisionShape::MakeSphere(7.f), Params))
		{
			UFTInteractableComponent* C = Cast<UFTInteractableComponent>(Hit.GetComponent());
			if (C && C->GetStudioOwner() && Hit.Distance <= C->MaxDistance)
			{
				FCollisionQueryParams LOS(SCENE_QUERY_STAT(FTInteractLOS), false, this);
				LOS.AddIgnoredActor(C->GetOwner());
				if (HeldProp)
				{
					LOS.AddIgnoredActor(HeldProp);
				}
				FHitResult Block;
				const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Block, Start, Hit.ImpactPoint, ECC_Visibility, LOS)
					&& !Cast<APawn>(Block.GetActor());
				if (!bBlocked)
				{
					NewFocus = C;
				}
			}
		}
	}

	if (NewFocus != Focused.Get())
	{
		if (UFTInteractableComponent* Old = Focused.Get())
		{
			Old->SetHighlighted(false);
		}
		if (NewFocus)
		{
			NewFocus->SetHighlighted(true);
		}
		Focused = NewFocus;
		if (HoldTarget.IsValid() && HoldTarget.Get() != NewFocus)
		{
			HoldTarget.Reset();
			HoldProgress = 0.f;
		}
		if (ContinuousTarget.IsValid() && ContinuousTarget.Get() != NewFocus)
		{
			ContinuousTarget.Reset();
			ServerEndUse();
		}
	}

	if (UFTInteractableComponent* H = HoldTarget.Get())
	{
		if (bInteractHeld)
		{
			HoldProgress += DeltaSeconds / FMath::Max(H->HoldTime, 0.05f);
			if (HoldProgress >= 1.f)
			{
				HoldProgress = 0.f;
				HoldTarget.Reset();
				PunchAlpha = 1.f;
				ServerInteract(H);
			}
		}
	}
}

FText AFTCharacter::GetPromptText(bool& bOutAvailable, FText& OutReason) const
{
	bOutAvailable = false;
	const UFTInteractableComponent* C = Focused.Get();
	if (!C || !C->GetStudioOwner())
	{
		return FText::GetEmpty();
	}
	const AFTStudioActor* TargetOwner = C->GetStudioOwner();
	bOutAvailable = TargetOwner->CanInteract(C, this, OutReason);
	const FText Verb = TargetOwner->GetPromptVerb(C, this);
	const FText Label = TargetOwner->GetPromptLabel(C);
	if (Label.IsEmpty())
	{
		return Verb;
	}
	return FText::Format(LOCTEXT("PromptFmt", "{0}  {1}"), Verb, Label);
}

bool AFTCharacter::ValidateInteraction(UFTInteractableComponent* Comp, FText& OutReason) const
{
	if (!Comp || !Comp->GetStudioOwner())
	{
		OutReason = LOCTEXT("NoTarget", "Nothing to use here");
		return false;
	}
	if (bKnockedDown)
	{
		OutReason = LOCTEXT("Dizzy", "Still seeing stars...");
		return false;
	}
	if (Comp->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
	{
		OutReason = LOCTEXT("Unavailable", "Not available right now");
		return false;
	}
	const float Dist = FVector::Dist(FirstPersonCamera->GetComponentLocation(), Comp->GetComponentLocation()) - Comp->GetScaledBoxExtent().GetMax();
	if (Dist > Comp->MaxDistance + 120.f)
	{
		OutReason = LOCTEXT("TooFar", "Too far away");
		return false;
	}
	return Comp->GetStudioOwner()->CanInteract(Comp, this, OutReason);
}

void AFTCharacter::ServerInteract_Implementation(UFTInteractableComponent* Comp)
{
	FText Reason;
	if (!ValidateInteraction(Comp, Reason))
	{
		ClientRejected(Reason);
		return;
	}
	Comp->GetStudioOwner()->OnInteract(Comp, this);
}

void AFTCharacter::ServerBeginUse_Implementation(UFTInteractableComponent* Comp)
{
	FText Reason;
	if (!ValidateInteraction(Comp, Reason))
	{
		ClientRejected(Reason);
		return;
	}
	if (UsingActor && UsingActor != Comp->GetStudioOwner())
	{
		StopUsing();
	}
	SetUsingActor(Comp->GetStudioOwner());
	Comp->GetStudioOwner()->OnBeginUse(Comp, this);
}

void AFTCharacter::ServerEndUse_Implementation()
{
	StopUsing();
}

void AFTCharacter::ServerDrop_Implementation()
{
	if (UsingActor)
	{
		StopUsing();
	}
	else if (HeldProp)
	{
		ReleaseProp(false);
	}
}

void AFTCharacter::ServerPrimary_Implementation()
{
	if (!HeldProp || bKnockedDown)
	{
		return;
	}
	if (HeldProp->PropTag == FTTags::PropHarpoon && AFTZone::IsInZone(this, FTTags::ZoneHeroMark, GetActorLocation()))
	{
		PlayEmote(EFTEmote::HeroPose);
		if (AFTSceneManager* SM = AFTSceneManager::Get(this))
		{
			SM->ReportEvent(FTTags::EvHeroThrust, this, this);
		}
		return;
	}
	if (HeldProp->bThrowable)
	{
		ReleaseProp(true);
	}
}

void AFTCharacter::ServerCostumeAction_Implementation()
{
	if (bKnockedDown)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - EmoteStartTime < 1.2f)
	{
		return;
	}
	AFTSceneManager* SM = AFTSceneManager::Get(this);
	switch (Costume)
	{
	case EFTCostume::Shark:
	{
		const FVector Fwd = GetActorForwardVector();
		LaunchCharacter(Fwd * 750.f + FVector(0.f, 0.f, 330.f), true, true);
		PlayEmote(EFTEmote::Panic);
		MulticastCharSound(EFTSound::SharkChomp);
		if (SM)
		{
			SM->ReportEvent(FTTags::EvCostumeLunge, this, this);
		}
		break;
	}
	case EFTCostume::Lifeguard:
		PlayEmote(EFTEmote::Point);
		MulticastCharSound(EFTSound::Squeak);
		if (SM)
		{
			SM->ReportEvent(FTTags::EvEmotePoint, this, this);
		}
		break;
	case EFTCostume::FoamKnight:
		PlayEmote(EFTEmote::HeroPose);
		break;
	case EFTCostume::Raincoat:
		LaunchCharacter(FVector(0.f, 0.f, 420.f), false, true);
		PlayEmote(EFTEmote::Cheer);
		break;
	default:
		ClientRejected(LOCTEXT("NoCostumeMove", "Your work clothes have no special move. Try the costume rack!"));
		break;
	}
}

void AFTCharacter::ServerEmote_Implementation(EFTEmote NewEmote)
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - EmoteStartTime < 0.6f || bKnockedDown)
	{
		return;
	}
	PlayEmote(NewEmote);
	if (AFTSceneManager* SM = AFTSceneManager::Get(this))
	{
		switch (NewEmote)
		{
		case EFTEmote::Point: SM->ReportEvent(FTTags::EvEmotePoint, this, this); break;
		case EFTEmote::Cheer: SM->ReportEvent(FTTags::EvEmoteCheer, this, this); break;
		case EFTEmote::Panic: SM->ReportEvent(FTTags::EvEmotePanic, this, this); break;
		default: break;
		}
	}
}

void AFTCharacter::ServerSetSprint_Implementation(bool bNewSprint)
{
	bSprinting = bNewSprint;
}

void AFTCharacter::ServerCameraInput_Implementation(float PanDelta, float TiltDelta, float ZoomDelta, float DollyAxis)
{
	if (AFTFilmCamera* Cam = GetOperatedCamera())
	{
		Cam->ApplyOperatorInput(FMath::Clamp(PanDelta, -45.f, 45.f), FMath::Clamp(TiltDelta, -45.f, 45.f),
			FMath::Clamp(ZoomDelta, -2.f, 2.f), FMath::Clamp(DollyAxis, -1.f, 1.f));
	}
}

void AFTCharacter::ServerCameraRecord_Implementation()
{
	if (AFTFilmCamera* Cam = GetOperatedCamera())
	{
		if (AFTSceneManager* SM = AFTSceneManager::Get(this))
		{
			SM->RequestRecordToggle(Cam, this);
		}
	}
}

void AFTCharacter::ServerCameraRecenter_Implementation()
{
	if (AFTFilmCamera* Cam = GetOperatedCamera())
	{
		Cam->Recenter();
	}
}

void AFTCharacter::MulticastCharSound_Implementation(EFTSound Sound)
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		FTAudio::PlayAt(this, Sound, GetActorLocation(), 1.f, FMath::FRandRange(0.95f, 1.05f));
	}
}

void AFTCharacter::ClientRejected_Implementation(const FText& Reason)
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastRejectTime < 0.25f)
	{
		return;
	}
	LastRejectTime = Now;
	if (AFTPlayerController* PC = Cast<AFTPlayerController>(GetController()))
	{
		PC->ShowToast(Reason, true);
	}
	FTAudio::Play2D(this, EFTSound::UIError, 0.7f);
}

void AFTCharacter::ClientFeedback_Implementation(const FText& Text)
{
	if (AFTPlayerController* PC = Cast<AFTPlayerController>(GetController()))
	{
		PC->ShowToast(Text, false);
	}
}

// ======================================================================== input

void AFTCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		return;
	}
	UFTInput* In = UFTInput::Get();
	EIC->BindAction(In->Move, ETriggerEvent::Triggered, this, &AFTCharacter::InputMove);
	EIC->BindAction(In->Look, ETriggerEvent::Triggered, this, &AFTCharacter::InputLook);
	EIC->BindAction(In->Jump, ETriggerEvent::Started, this, &AFTCharacter::InputJump);
	EIC->BindAction(In->Jump, ETriggerEvent::Completed, this, &AFTCharacter::InputJumpStop);
	EIC->BindAction(In->Sprint, ETriggerEvent::Started, this, &AFTCharacter::InputSprintStart);
	EIC->BindAction(In->Sprint, ETriggerEvent::Completed, this, &AFTCharacter::InputSprintStop);
	EIC->BindAction(In->Interact, ETriggerEvent::Started, this, &AFTCharacter::InputInteractStart);
	EIC->BindAction(In->Interact, ETriggerEvent::Completed, this, &AFTCharacter::InputInteractStop);
	EIC->BindAction(In->Drop, ETriggerEvent::Started, this, &AFTCharacter::InputDrop);
	EIC->BindAction(In->Primary, ETriggerEvent::Started, this, &AFTCharacter::InputPrimary);
	EIC->BindAction(In->Zoom, ETriggerEvent::Triggered, this, &AFTCharacter::InputZoom);
	EIC->BindAction(In->Recenter, ETriggerEvent::Started, this, &AFTCharacter::InputRecenter);
	EIC->BindAction(In->CostumeAction, ETriggerEvent::Started, this, &AFTCharacter::InputCostumeAction);
	EIC->BindAction(In->Emote1, ETriggerEvent::Started, this, &AFTCharacter::InputEmote1);
	EIC->BindAction(In->Emote2, ETriggerEvent::Started, this, &AFTCharacter::InputEmote2);
	EIC->BindAction(In->Emote3, ETriggerEvent::Started, this, &AFTCharacter::InputEmote3);
	EIC->BindAction(In->Emote4, ETriggerEvent::Started, this, &AFTCharacter::InputEmote4);
	EIC->BindAction(In->Ping, ETriggerEvent::Started, this, &AFTCharacter::InputPing);
	EIC->BindAction(In->Help, ETriggerEvent::Started, this, &AFTCharacter::InputHelp);
	EIC->BindAction(In->Pause, ETriggerEvent::Started, this, &AFTCharacter::InputPause);
}

void AFTCharacter::InputMove(const FInputActionValue& Value)
{
	if (IsInputBlocked() || bKnockedDown)
	{
		return;
	}
	const FVector2D V = Value.Get<FVector2D>();
	if (IsOperatingCamera())
	{
		PendingDolly = V.X;
		return;
	}
	if (Controller)
	{
		const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), V.Y);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), V.X);
	}
}

void AFTCharacter::InputLook(const FInputActionValue& Value)
{
	if (IsInputBlocked())
	{
		return;
	}
	const FVector2D V = Value.Get<FVector2D>() * UFTSettings::Get()->MouseSensitivity;
	if (AFTFilmCamera* Cam = GetOperatedCamera())
	{
		// the look axis is pre-negated for AddControllerPitchInput; the camera head wants mouse-up = tilt up
		PendingPan += V.X * 0.3f;
		PendingTilt -= V.Y * 0.3f;
		return;
	}
	AddControllerYawInput(V.X);
	AddControllerPitchInput(V.Y);
}

void AFTCharacter::FlushCameraInput(float DeltaSeconds)
{
	AFTFilmCamera* Cam = GetOperatedCamera();
	if (!Cam)
	{
		PendingPan = PendingTilt = PendingZoom = PendingDolly = 0.f;
		return;
	}
	CameraSendTimer -= DeltaSeconds;
	if (CameraSendTimer > 0.f)
	{
		return;
	}
	CameraSendTimer = 1.f / 30.f;
	const float Dolly = PendingDolly;
	if (!FMath::IsNearlyZero(PendingPan) || !FMath::IsNearlyZero(PendingTilt) || !FMath::IsNearlyZero(PendingZoom) || !FMath::IsNearlyZero(Dolly))
	{
		if (!HasAuthority())
		{
			Cam->ApplyOperatorInput(PendingPan, PendingTilt, PendingZoom, Dolly); // local prediction
		}
		ServerCameraInput(PendingPan, PendingTilt, PendingZoom, Dolly);
	}
	PendingPan = PendingTilt = PendingZoom = 0.f;
	PendingDolly = 0.f;
}

void AFTCharacter::InputJump()
{
	if (!IsInputBlocked() && !IsOperatingCamera() && !bKnockedDown)
	{
		Jump();
	}
}

void AFTCharacter::InputJumpStop()
{
	StopJumping();
}

void AFTCharacter::InputSprintStart()
{
	bSprinting = true;
	ServerSetSprint(true);
}

void AFTCharacter::InputSprintStop()
{
	bSprinting = false;
	ServerSetSprint(false);
}

void AFTCharacter::InputInteractStart()
{
	if (IsInputBlocked())
	{
		return;
	}
	bInteractHeld = true;
	if (IsOperatingCamera())
	{
		ServerEndUse();
		return;
	}
	UFTInteractableComponent* C = Focused.Get();
	if (!C)
	{
		return;
	}
	FText Reason;
	if (!C->GetStudioOwner()->CanInteract(C, this, Reason))
	{
		ClientRejected_Implementation(Reason);
		return;
	}
	switch (C->Type)
	{
	case EFTInteractType::Hold:
		HoldTarget = C;
		HoldProgress = 0.f;
		break;
	case EFTInteractType::Continuous:
		ContinuousTarget = C;
		PunchAlpha = 0.5f;
		ServerBeginUse(C);
		break;
	default:
		PunchAlpha = 1.f;
		ServerInteract(C);
		break;
	}
}

void AFTCharacter::InputInteractStop()
{
	bInteractHeld = false;
	HoldTarget.Reset();
	HoldProgress = 0.f;
	if (ContinuousTarget.IsValid())
	{
		ContinuousTarget.Reset();
		ServerEndUse();
	}
}

void AFTCharacter::InputDrop()
{
	if (!IsInputBlocked())
	{
		ServerDrop();
	}
}

void AFTCharacter::InputPrimary()
{
	if (IsInputBlocked())
	{
		return;
	}
	if (IsOperatingCamera())
	{
		ServerCameraRecord();
		return;
	}
	if (HeldProp)
	{
		PunchAlpha = 1.f;
		ServerPrimary();
	}
}

void AFTCharacter::InputZoom(const FInputActionValue& Value)
{
	if (IsOperatingCamera())
	{
		PendingZoom += Value.Get<float>() * 0.12f;
	}
}

void AFTCharacter::InputRecenter()
{
	if (IsOperatingCamera())
	{
		ServerCameraRecenter();
	}
}

void AFTCharacter::InputCostumeAction()
{
	if (!IsInputBlocked() && !IsOperatingCamera())
	{
		ServerCostumeAction();
	}
}

void AFTCharacter::SendEmote(EFTEmote E)
{
	if (IsInputBlocked() || IsOperatingCamera())
	{
		return;
	}
	ServerEmote(E);
	if (AFTPlayerController* PC = Cast<AFTPlayerController>(GetController()))
	{
		static const FText Names[] = {
			FText::GetEmpty(), LOCTEXT("EWave", "You wave!"), LOCTEXT("EPoint", "You point dramatically!"),
			LOCTEXT("ECheer", "You cheer!"), LOCTEXT("EPanic", "You panic!") };
		PC->ShowToast(Names[FMath::Clamp((int32)E, 0, 4)], false);
	}
}

void AFTCharacter::InputEmote1() { SendEmote(EFTEmote::Wave); }
void AFTCharacter::InputEmote2() { SendEmote(EFTEmote::Point); }
void AFTCharacter::InputEmote3() { SendEmote(EFTEmote::Cheer); }
void AFTCharacter::InputEmote4() { SendEmote(EFTEmote::Panic); }

void AFTCharacter::InputPing()
{
	if (IsInputBlocked())
	{
		return;
	}
	const FVector Start = FirstPersonCamera->GetComponentLocation();
	const FVector End = Start + FirstPersonCamera->GetForwardVector() * 4000.f;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FTPing), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		if (AFTPlayerController* PC = Cast<AFTPlayerController>(GetController()))
		{
			PC->ServerPing(Hit.ImpactPoint);
		}
	}
}

void AFTCharacter::InputHelp()
{
	if (AFTPlayerController* PC = Cast<AFTPlayerController>(GetController()))
	{
		PC->ToggleHelp();
	}
}

void AFTCharacter::InputPause()
{
	if (AFTPlayerController* PC = Cast<AFTPlayerController>(GetController()))
	{
		PC->TogglePause();
	}
}

#undef LOCTEXT_NAMESPACE
