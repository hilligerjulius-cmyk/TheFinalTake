#include "TheFinalTake/Props/FTProp.h"

#include "TheFinalTake/Characters/FTCharacter.h"
#include "TheFinalTake/Core/FTVisuals.h"
#include "TheFinalTake/Interaction/FTInteractableComponent.h"
#include "TheFinalTake/World/FTFloodController.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Game/FTSceneManager.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "FinalTakeProps"

AFTProp::AFTProp()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicatingMovement(true);
	SetNetUpdateFrequency(30.f);

	Visual = CreateDefaultSubobject<USceneComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Root);

	Grab = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("Grab"));
	Grab->SetupAttachment(Root);
	Grab->Setup(TEXT("Carry"), FText::GetEmpty(), LOCTEXT("Carry", "Carry"), EFTInteractType::Carry, FVector(30.f));
	Grab->SetRelativeLocation(FVector(0.f, 0.f, 25.f));
}

void AFTProp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTProp, Carrier);
	DOREPLIFETIME(AFTProp, bFloating);
	DOREPLIFETIME(AFTProp, ReelSceneIndex);
}

void AFTProp::BeginPlay()
{
	Super::BeginPlay();
	HomeTransform = GetActorTransform();
	FloatPhase = FMath::FRandRange(0.f, 6.28f);
	Grab->Label = PropName;
}

bool AFTProp::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	if (Carrier && Carrier != User)
	{
		OutReason = FText::Format(LOCTEXT("Carried", "{0} is carrying that"), FText::FromString(Carrier->GetCrewName()));
		return false;
	}
	if (User && User->UsingActor)
	{
		OutReason = LOCTEXT("HandsBusy", "Your hands are busy - press Q to let go first");
		return false;
	}
	return true;
}

FText AFTProp::GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const
{
	if (User && User->HeldProp && User->HeldProp != this)
	{
		return LOCTEXT("Swap", "Swap for");
	}
	return LOCTEXT("CarryVerb", "Carry");
}

void AFTProp::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	if (User && !Carrier)
	{
		User->TakeProp(this);
	}
}

void AFTProp::PickUp(AFTCharacter* NewCarrier)
{
	if (!HasAuthority() || !NewCarrier)
	{
		return;
	}
	Carrier = NewCarrier;
	bFalling = false;
	bFloating = false;
	Velocity = FVector::ZeroVector;
	SetCarriedCollision(true);
	AttachToComponent(NewCarrier->GetCarryAnchor(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SetActorRelativeLocation(CarryOffset);
	SetActorRelativeRotation(CarryRotation);
	MulticastSound(EFTSound::Rustle, GetActorLocation(), 0.7f, FMath::FRandRange(0.9f, 1.2f));
	ForceNetUpdate();
}

void AFTProp::Drop(bool bThrow)
{
	if (!HasAuthority())
	{
		return;
	}
	AFTCharacter* Old = Carrier;
	Carrier = nullptr;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetCarriedCollision(false);

	FVector Fwd = FVector::ForwardVector;
	FVector Start = GetActorLocation();
	float Yaw = GetActorRotation().Yaw;
	if (Old)
	{
		const FRotator View = Old->GetControlRotation();
		Fwd = View.Vector();
		Yaw = View.Yaw;
		Start = Old->GetActorLocation() + FVector(Fwd.X, Fwd.Y, 0.f).GetSafeNormal() * 55.f + FVector(0.f, 0.f, 10.f);
	}
	SetActorLocationAndRotation(Start, FRotator(0.f, Yaw, 0.f));
	Velocity = bThrow ? Fwd * 950.f + FVector(0.f, 0.f, 380.f) : FVector(Fwd.X, Fwd.Y, 0.f) * 90.f + FVector(0.f, 0.f, 60.f);
	bFalling = true;
	bFloating = false;
	if (bThrow)
	{
		MulticastSound(EFTSound::Whoosh, Start, 0.8f, 1.1f);
	}
	ForceNetUpdate();
}

void AFTProp::Land(const FVector& Location)
{
	bFalling = false;
	Velocity = FVector::ZeroVector;
	SetActorLocation(Location);
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
	MulticastSound(EFTSound::Thud, Location, 0.6f, FMath::FRandRange(0.9f, 1.2f));
}

void AFTProp::ReturnHome(bool bAnnounce)
{
	if (!HasAuthority())
	{
		return;
	}
	if (Carrier)
	{
		Carrier->HeldProp = nullptr;
		Carrier->ForceNetUpdate();
		Carrier = nullptr;
	}
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetCarriedCollision(false);
	SetActorTransform(HomeTransform);
	bFalling = false;
	bFloating = false;
	Velocity = FVector::ZeroVector;
	if (bAnnounce)
	{
		Announce(FText::Format(LOCTEXT("ReturnedHome", "The {0} was recovered to its shelf."), PropName), EFTAnnounceStyle::Info, EFTSound::UIConfirm);
	}
	ForceNetUpdate();
}

void AFTProp::ResetForNewShoot()
{
	if (bDestroyOnReset)
	{
		if (Carrier)
		{
			Carrier->HeldProp = nullptr;
		}
		Destroy();
		return;
	}
	ReturnHome(false);
}

void AFTProp::OnUserLeft(AFTCharacter* User)
{
	if (Carrier == User)
	{
		Drop(false);
	}
}

void AFTProp::SetCarriedCollision(bool bCarried)
{
	Grab->SetInteractionEnabled(!bCarried);
}

void AFTProp::OnRep_Carrier()
{
	SetCarriedCollision(Carrier != nullptr);
}

bool AFTProp::GetSubjectBounds(FName SubjectTag, FVector& OutCenter, float& OutRadius) const
{
	FVector Extent;
	GetActorBounds(true, OutCenter, Extent, false);
	OutRadius = FMath::Clamp(Extent.Size() * 0.7f, 20.f, 250.f);
	return true;
}

void AFTProp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// cosmetic bob while floating (every machine)
	if (bFloating && !Carrier)
	{
		const float T = GetWorld()->GetTimeSeconds() + FloatPhase;
		Visual->SetRelativeLocation(FVector(0.f, 0.f, FMath::Sin(T * 2.1f) * 3.f));
		Visual->SetRelativeRotation(FRotator(FMath::Sin(T * 1.3f) * 4.f, 0.f, FMath::Cos(T * 1.7f) * 5.f));
	}
	else if (!Visual->GetRelativeLocation().IsNearlyZero())
	{
		Visual->SetRelativeLocation(FVector::ZeroVector);
		Visual->SetRelativeRotation(FRotator::ZeroRotator);
	}

	if (!HasAuthority())
	{
		return;
	}

	if (Carrier)
	{
		if (!IsValid(Carrier))
		{
			Drop(false);
		}
		return;
	}

	const AFTFloodController* Flood = AFTFloodController::Get(this);
	FVector Loc = GetActorLocation();

	if (bFalling)
	{
		Velocity.Z -= 980.f * DeltaSeconds;
		const FVector Next = Loc + Velocity * DeltaSeconds;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(FTPropFall), false, this);
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Loc + FVector(0.f, 0.f, 10.f), Next, ECC_Visibility, Params) && !Cast<APawn>(Hit.GetActor()))
		{
			if (Hit.ImpactNormal.Z > 0.55f)
			{
				Land(Hit.ImpactPoint);
			}
			else
			{
				Velocity = FMath::GetReflectionVector(Velocity, Hit.ImpactNormal) * 0.3f;
				SetActorLocation(Hit.ImpactPoint + Hit.ImpactNormal * 5.f);
			}
		}
		else
		{
			SetActorLocation(Next);
		}
		if (bFloats && Flood && Flood->IsInFloodArea(Next) && Flood->GetWaterZ() > Next.Z + FloatSink)
		{
			bFalling = false;
			bFloating = true;
			MulticastSound(EFTSound::Splash, Next, 0.5f, 1.3f);
		}
	}
	else if (bFloating)
	{
		if (!Flood || !Flood->IsInFloodArea(Loc))
		{
			bFloating = false;
			bFalling = true;
		}
		else
		{
			const float WaterZ = Flood->GetWaterZ();
			// Is there ground above the float height? (water drained or pushed onto a step)
			FHitResult Ground;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(FTPropFloat), false, this);
			const bool bGround = GetWorld()->LineTraceSingleByChannel(Ground, Loc + FVector(0.f, 0.f, 60.f), Loc - FVector(0.f, 0.f, 400.f), ECC_Visibility, Params);
			const float TargetZ = WaterZ - FloatSink;
			if (bGround && Ground.ImpactPoint.Z > TargetZ)
			{
				bFloating = false;
				Land(Ground.ImpactPoint);
			}
			else
			{
				// slow drift towards the prop warehouse, blocked by walls
				const FVector Drift = FVector(0.35f, 1.f, 0.f).GetSafeNormal() * 14.f * DeltaSeconds;
				FVector Next = FVector(Loc.X, Loc.Y, FMath::FInterpTo(Loc.Z, TargetZ, DeltaSeconds, 2.f));
				FHitResult Wall;
				if (!GetWorld()->LineTraceSingleByChannel(Wall, Next, Next + Drift * 20.f, ECC_Visibility, Params) && Flood->IsInFloodArea(Next + Drift * 40.f))
				{
					Next += Drift;
				}
				SetActorLocation(Next);
			}
		}
	}
	else if (bFloats && Flood && Flood->IsInFloodArea(Loc) && Flood->GetWaterZ() - FloatSink > Loc.Z + 4.f)
	{
		bFloating = true;
	}

	// Out of bounds recovery
	if (GetActorLocation().Z < -1500.f)
	{
		ReturnHome(bCritical);
		if (AFTSceneManager* SM = GetSceneManager())
		{
			SM->AdjustCondition(-2.f, LOCTEXT("PropLost", "A prop fell off the set"));
		}
	}
}

// ============================================================================ prop types

AFTProp_Harpoon::AFTProp_Harpoon()
{
	using namespace FTColors;
	PropTag = FTTags::PropHarpoon;
	PropName = LOCTEXT("Harpoon", "Hero Harpoon");
	Tags = { FTTags::PropHarpoon, FTTags::SubjHarpoon };
	bCritical = true;
	bThrowable = true;
	FloatSink = 3.f;
	CarryOffset = FVector(10.f, -4.f, 4.f);
	CarryRotation = FRotator(18.f, -8.f, 0.f);
	const FRotator Along(-90.f, 0.f, 0.f);
	FTVis::MakePart(this, Visual, TEXT("Shaft"), EFTShape::Cylinder, FVector(0.f, 0.f, 8.f), FVector(6.f, 6.f, 170.f), Wood, Along);
	FTVis::MakePart(this, Visual, TEXT("Tip"), EFTShape::Cone, FVector(98.f, 0.f, 8.f), FVector(16.f, 16.f, 30.f), Grey, Along);
	FTVis::MakePart(this, Visual, TEXT("TipCollar"), EFTShape::Cylinder, FVector(82.f, 0.f, 8.f), FVector(11.f, 11.f, 6.f), GreyDark, Along);
	FTVis::MakePart(this, Visual, TEXT("Foam"), EFTShape::Capsule, FVector(22.f, 0.f, 8.f), FVector(18.f, 18.f, 44.f), Yellow, Along);
	FTVis::MakePart(this, Visual, TEXT("Band"), EFTShape::Cylinder, FVector(22.f, 0.f, 8.f), FVector(19.f, 19.f, 5.f), Coral, Along);
	FTVis::MakePart(this, Visual, TEXT("Grip"), EFTShape::Box, FVector(-40.f, 0.f, 8.f), FVector(30.f, 10.f, 10.f), Teal);
	FTVis::MakePart(this, Visual, TEXT("Trigger"), EFTShape::Box, FVector(-30.f, 0.f, -2.f), FVector(8.f, 6.f, 14.f), Coral, FRotator(-15.f, 0.f, 0.f));
	FTVis::MakePart(this, Visual, TEXT("Butt"), EFTShape::Box, FVector(-78.f, 0.f, 8.f), FVector(16.f, 9.f, 12.f), TealDark);
	Grab->SetBoxExtent(FVector(90.f, 16.f, 16.f));
	Grab->SetRelativeLocation(FVector(10.f, 0.f, 8.f));
}

AFTProp_LifeRing::AFTProp_LifeRing()
{
	using namespace FTColors;
	PropTag = TEXT("Prop.LifeRing");
	PropName = LOCTEXT("LifeRing", "Life Ring");
	Tags = { TEXT("Prop.LifeRing"), FTTags::SubjLifeRing };
	FloatSink = 5.f;
	CarryOffset = FVector(6.f, 0.f, 0.f);
	CarryRotation = FRotator(0.f, 0.f, 80.f);
	FTVis::MakePart(this, Visual, TEXT("Ring"), EFTShape::Torus, FVector(0.f, 0.f, 9.f), FVector(72.f, 72.f, 20.f), Coral);
	for (int32 i = 0; i < 4; ++i)
	{
		const float A = i * 90.f + 45.f;
		const FVector P = FRotator(0.f, A, 0.f).Vector() * 27.f + FVector(0.f, 0.f, 9.f);
		FTVis::MakePart(this, Visual, *FString::Printf(TEXT("Stripe%d"), i), EFTShape::Box, P, FVector(8.f, 17.f, 21.f), White, FRotator(0.f, A, 0.f));
	}
	Grab->SetBoxExtent(FVector(38.f, 38.f, 14.f));
	Grab->SetRelativeLocation(FVector(0.f, 0.f, 9.f));
}

AFTProp_Lamp::AFTProp_Lamp()
{
	using namespace FTColors;
	PropTag = TEXT("Prop.Lamp");
	PropName = LOCTEXT("Lamp", "Practical Lamp");
	Tags = { TEXT("Prop.Lamp"), FTTags::SubjLamp };
	bFloats = false;
	CarryOffset = FVector(6.f, 0.f, -40.f);
	for (int32 i = 0; i < 3; ++i)
	{
		const float A = i * 120.f;
		const FVector Dir = FRotator(0.f, A, 0.f).Vector();
		FTVis::MakePart(this, Visual, *FString::Printf(TEXT("Leg%d"), i), EFTShape::Cylinder, Dir * 14.f + FVector(0.f, 0.f, 16.f), FVector(4.f, 4.f, 36.f), Charcoal, FRotator(25.f, A, 0.f));
	}
	FTVis::MakePart(this, Visual, TEXT("Pole"), EFTShape::Cylinder, FVector(0.f, 0.f, 62.f), FVector(5.f, 5.f, 90.f), Charcoal);
	FTVis::MakePart(this, Visual, TEXT("Collar"), EFTShape::Cylinder, FVector(0.f, 0.f, 36.f), FVector(9.f, 9.f, 6.f), Amber);
	FTVis::MakePart(this, Visual, TEXT("Housing"), EFTShape::Box, FVector(4.f, 0.f, 112.f), FVector(26.f, 26.f, 22.f), Charcoal);
	FTVis::MakePart(this, Visual, TEXT("Lens"), EFTShape::Cylinder, FVector(18.f, 0.f, 112.f), FVector(20.f, 20.f, 3.f), Amber, FRotator(-90.f, 0.f, 0.f), 6.f);
	FTVis::MakePart(this, Visual, TEXT("DoorTop"), EFTShape::Box, FVector(20.f, 0.f, 126.f), FVector(12.f, 26.f, 2.f), Charcoal, FRotator(25.f, 0.f, 0.f));
	Bulb = CreateDefaultSubobject<UPointLightComponent>(TEXT("Bulb"));
	Bulb->SetupAttachment(Visual);
	Bulb->SetRelativeLocation(FVector(30.f, 0.f, 112.f));
	Bulb->SetIntensity(2500.f);
	Bulb->SetAttenuationRadius(420.f);
	Bulb->SetLightColor(FLinearColor(1.f, 0.72f, 0.4f));
	Bulb->SetCastShadows(false);
	Grab->SetBoxExtent(FVector(22.f, 22.f, 65.f));
	Grab->SetRelativeLocation(FVector(0.f, 0.f, 65.f));
}

void AFTProp_Lamp::BeginPlay()
{
	Super::BeginPlay();
}

AFTProp_Crate::AFTProp_Crate()
{
	using namespace FTColors;
	PropTag = TEXT("Prop.Crate");
	PropName = LOCTEXT("Crate", "Prop Crate");
	Tags = { TEXT("Prop.Crate"), FTTags::SubjCrate };
	FloatSink = 26.f;
	CarryOffset = FVector(12.f, -10.f, -18.f);
	FTVis::MakePart(this, Visual, TEXT("Body"), EFTShape::Box, FVector(0.f, 0.f, 25.f), FVector(56.f, 56.f, 50.f), Wood);
	FTVis::MakePart(this, Visual, TEXT("SlatA"), EFTShape::Box, FVector(0.f, 0.f, 25.f), FVector(58.f, 58.f, 8.f), WoodDark);
	FTVis::MakePart(this, Visual, TEXT("SlatB"), EFTShape::Box, FVector(0.f, 0.f, 25.f), FVector(8.f, 58.f, 52.f), WoodDark);
	FTVis::MakePart(this, Visual, TEXT("SlatC"), EFTShape::Box, FVector(0.f, 0.f, 25.f), FVector(58.f, 8.f, 52.f), WoodDark);
	FTVis::MakeText(this, Visual, TEXT("Stencil"), LOCTEXT("CrateStencil", "PROPS"), FVector(29.5f, 0.f, 36.f), FRotator::ZeroRotator, 9.f, FColor(40, 30, 25));
	Grab->SetBoxExtent(FVector(30.f, 30.f, 27.f));
	Grab->SetRelativeLocation(FVector(0.f, 0.f, 25.f));
}

AFTProp_FinMarker::AFTProp_FinMarker()
{
	using namespace FTColors;
	PropTag = TEXT("Prop.FinMarker");
	PropName = LOCTEXT("FinMarker", "Fin Marker");
	Tags = { TEXT("Prop.FinMarker"), FTTags::SubjFinMarker };
	FloatSink = 14.f;
	CarryOffset = FVector(10.f, 0.f, -30.f);
	FTVis::MakePart(this, Visual, TEXT("Float"), EFTShape::Cylinder, FVector(0.f, 0.f, 6.f), FVector(40.f, 40.f, 12.f), Coral);
	FTVis::MakePart(this, Visual, TEXT("Pole"), EFTShape::Cylinder, FVector(0.f, 0.f, 22.f), FVector(4.f, 4.f, 26.f), GreyDark);
	FTVis::MakePart(this, Visual, TEXT("Fin"), EFTShape::Prism, FVector(0.f, 0.f, 54.f), FVector(7.f, 44.f, 48.f), Blue, FRotator(0.f, 90.f, 0.f));
	FTVis::MakePart(this, Visual, TEXT("FinEdge"), EFTShape::Prism, FVector(-1.f, 0.f, 54.f), FVector(8.f, 30.f, 34.f), DeepBlue, FRotator(0.f, 90.f, 0.f));
	Grab->SetBoxExtent(FVector(25.f, 25.f, 40.f));
	Grab->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
}

AFTProp_Reel::AFTProp_Reel()
{
	using namespace FTColors;
	PropTag = FTTags::PropReel;
	PropName = LOCTEXT("Reel", "Film Reel");
	Tags = { FTTags::PropReel };
	bCritical = true;
	bDestroyOnReset = true;
	FloatSink = 4.f;
	CarryOffset = FVector(8.f, 0.f, -8.f);
	CarryRotation = FRotator(0.f, 90.f, 0.f);
	const FRotator Upright(0.f, 0.f, 90.f);
	FTVis::MakePart(this, Visual, TEXT("Rim"), EFTShape::Torus, FVector(0.f, 0.f, 24.f), FVector(48.f, 48.f, 12.f), Grey, Upright);
	FTVis::MakePart(this, Visual, TEXT("Disc"), EFTShape::Cylinder, FVector(0.f, 0.f, 24.f), FVector(40.f, 40.f, 6.f), Charcoal, Upright);
	FTVis::MakePart(this, Visual, TEXT("Hub"), EFTShape::Cylinder, FVector(0.f, 0.f, 24.f), FVector(14.f, 14.f, 12.f), Cream, Upright);
	for (int32 i = 0; i < 3; ++i)
	{
		const float A = i * 120.f;
		const FVector P = FVector(FMath::Cos(FMath::DegreesToRadians(A)) * 12.f, 0.f, 24.f + FMath::Sin(FMath::DegreesToRadians(A)) * 12.f);
		FTVis::MakePart(this, Visual, *FString::Printf(TEXT("Hole%d"), i), EFTShape::Cylinder, P + FVector(0.f, 4.f, 0.f), FVector(9.f, 9.f, 2.f), Grey, Upright);
	}
	Label = FTVis::MakeText(this, Visual, TEXT("Label"), LOCTEXT("ReelLabel", "REEL"), FVector(0.f, 7.f, 24.f), FRotator(0.f, 90.f, 0.f), 7.f, FColor(255, 220, 120));
	Grab->SetBoxExtent(FVector(24.f, 14.f, 24.f));
	Grab->SetRelativeLocation(FVector(0.f, 0.f, 24.f));
}

void AFTProp_Reel::SetReelScene(int32 InSceneIndex)
{
	ReelSceneIndex = InSceneIndex;
	OnRep_ReelIndex();
}

void AFTProp_Reel::OnRep_ReelIndex()
{
	const FText T = FText::Format(LOCTEXT("ReelN", "SC {0}"), FText::AsNumber(ReelSceneIndex + 1));
	if (Label)
	{
		Label->SetText(T);
	}
	PropName = FText::Format(LOCTEXT("ReelName", "Scene {0} Reel"), FText::AsNumber(ReelSceneIndex + 1));
	Grab->Label = PropName;
}

#undef LOCTEXT_NAMESPACE
