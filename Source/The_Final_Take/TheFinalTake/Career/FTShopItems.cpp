#include "TheFinalTake/Career/FTShopItems.h"

#include "The_Final_Take.h"
#include "TheFinalTake/Career/FTCareerManager.h"
#include "TheFinalTake/Career/FTEconomy.h"
#include "TheFinalTake/Characters/FTCharacter.h"
#include "TheFinalTake/Core/FTVisuals.h"
#include "TheFinalTake/FX/FTChunkyParticles.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Game/FTPlayerController.h"
#include "TheFinalTake/Interaction/FTInteractableComponent.h"
#include "TheFinalTake/Production/FTShark.h"
#include "TheFinalTake/Props/FTSetPieces.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "FinalTakeShop"

using namespace FTColors;

namespace
{
	bool IsPlaceable(EFTShopCategory C)
	{
		return C == EFTShopCategory::Prop || C == EFTShopCategory::Effect || C == EFTShopCategory::SetPiece;
	}

	/** Runtime text (constructor-only FTVis::MakeText cannot be used after BeginPlay). */
	UTextRenderComponent* SpawnText(AActor* Owner, USceneComponent* Parent, const FText& Text, const FVector& Loc, const FRotator& Rot, float Size, const FColor& Color)
	{
		UTextRenderComponent* T = NewObject<UTextRenderComponent>(Owner);
		T->SetupAttachment(Parent);
		T->SetText(Text);
		T->SetRelativeLocation(Loc);
		T->SetRelativeRotation(Rot);
		T->SetWorldSize(Size);
		T->SetTextRenderColor(Color);
		T->SetHorizontalAlignment(EHTA_Center);
		T->SetVerticalAlignment(EVRTA_TextCenter);
		T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		T->SetCastShadow(false);
		if (UMaterialInterface* TM = FTVis::TextMaterial())
		{
			T->SetTextMaterial(TM);
		}
		T->RegisterComponent();
		return T;
	}

	void DestroyParts(TArray<TObjectPtr<UStaticMeshComponent>>& Parts)
	{
		for (UStaticMeshComponent* P : Parts)
		{
			if (P)
			{
				P->DestroyComponent();
			}
		}
		Parts.Reset();
	}

	/** Floor under a point (or the point itself if nothing is found). */
	FVector SnapToFloor(const UWorld* World, const FVector& P, const AActor* Ignore = nullptr)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(FTShopFloor), false, Ignore);
		if (World && World->LineTraceSingleByChannel(Hit, P + FVector(0.f, 0.f, 120.f), P - FVector(0.f, 0.f, 400.f), ECC_Visibility, Params))
		{
			return Hit.ImpactPoint;
		}
		return P;
	}
}

// ============================================================================ helpers

void FTShop::BuildParts(AActor* Owner, USceneComponent* Parent, const FFTShopItemDef& Def, TArray<TObjectPtr<UStaticMeshComponent>>& Out, bool bOwnerNoSee)
{
	for (const FFTItemPart& P : Def.Parts)
	{
		UStaticMeshComponent* C = FTVis::SpawnPart(Owner, Parent, P.Shape, P.Location, P.Size, P.Color, P.Rotation, P.Emissive, false);
		if (bOwnerNoSee)
		{
			C->SetOwnerNoSee(true);
			C->bCastHiddenShadow = true;
		}
		Out.Add(C);
	}
}

FBox FTShop::PartsBounds(const FFTShopItemDef& Def)
{
	FBox B(ForceInit);
	for (const FFTItemPart& P : Def.Parts)
	{
		B += FBox::BuildAABB(FVector::ZeroVector, P.Size * 0.5f).TransformBy(FTransform(P.Rotation, P.Location));
	}
	return B.IsValid ? B : FBox(FVector(-20.f), FVector(20.f));
}

FText FTShop::UsageHint(const FFTShopItemDef& Def)
{
	switch (Def.Category)
	{
	case EFTShopCategory::Costume: return LOCTEXT("UseCostume", "Hangs on the ACCESSORY WALL in the wardrobe. Wear it - or carry the stand-in there and dress it. Counts while the wearer is in frame.");
	case EFTShopCategory::Prop: return LOCTEXT("UseProp", "Delivered to the LOADING BAY in the lobby. Carry it onto the set - counts while it is in frame.");
	case EFTShopCategory::Effect: return LOCTEXT("UseEffect", "Delivered to the LOADING BAY. Put it on the set and flip its switch - counts only while it runs in frame.");
	case EFTShopCategory::SharkUpgrade: return LOCTEXT("UseShark", "Installed on the SHARK RIG right away (KITS switch at the rig desk). Counts while the raised shark is in frame.");
	case EFTShopCategory::SetPiece: return LOCTEXT("UseSet", "Delivered to the LOADING BAY. Dress the set with it - counts while it is in frame.");
	default: return FText::GetEmpty();
	}
}

FName FTShop::WearerKey(const FString& Wearer, EFTAccessorySlot Slot)
{
	static const TCHAR* Names[] = { TEXT("Head"), TEXT("Face"), TEXT("Body") };
	return FName(*FString::Printf(TEXT("%s.%s"), *Wearer, Names[FMath::Clamp((int32)Slot, 0, 2)]));
}

void FTShop::GatherShowcase(const UWorld* World, TArray<FFTShowcaseEntry>& Out)
{
	if (!World)
	{
		return;
	}
	for (TActorIterator<AFTShopItemActor> It(World); It; ++It)
	{
		if (!It->IsShowcased())
		{
			continue;
		}
		FFTShowcaseEntry E;
		E.ItemId = It->GetItemId();
		E.Actor = *It;
		It->GetSubjectBounds(NAME_None, E.Center, E.Radius);
		Out.Add(E);
	}
	for (TActorIterator<AFTCharacter> It(World); It; ++It)
	{
		It->GatherShowcase(Out);
	}
	for (TActorIterator<AFTStandIn> It(World); It; ++It)
	{
		It->GatherShowcase(Out);
	}
	for (TActorIterator<AFTSharkRig> It(World); It; ++It)
	{
		It->GatherShowcase(Out);
	}
}

AFTShopItemActor* FTShop::FindItemActor(const UWorld* World, FName ItemId)
{
	for (TActorIterator<AFTShopItemActor> It(World); It; ++It)
	{
		if (It->GetItemId() == ItemId && !It->IsActorBeingDestroyed())
		{
			return *It;
		}
	}
	return nullptr;
}

// ============================================================================ worn accessories

void FFTWornAccessories::Apply(AActor* Owner, const TArray<USceneComponent*>& Anchors, const TArray<FName>& Items, bool bOwnerNoSee)
{
	const UFTEconomyConfig* Cfg = UFTEconomyConfig::Get();
	for (int32 S = 0; S < NumSlots; ++S)
	{
		const FName Want = Items.IsValidIndex(S) ? Items[S] : NAME_None;
		if (Want == Current[S])
		{
			continue;
		}
		for (const TWeakObjectPtr<UStaticMeshComponent>& P : Parts[S])
		{
			if (P.IsValid())
			{
				P->DestroyComponent();
			}
		}
		Parts[S].Reset();
		Current[S] = Want;
		const FFTShopItemDef* Def = Want.IsNone() ? nullptr : Cfg->FindItem(Want);
		if (!Def || !Owner || !Anchors.IsValidIndex(S) || !Anchors[S])
		{
			continue;
		}
		TArray<TObjectPtr<UStaticMeshComponent>> Spawned;
		FTShop::BuildParts(Owner, Anchors[S], *Def, Spawned, bOwnerNoSee);
		for (UStaticMeshComponent* C : Spawned)
		{
			C->SetVisibility(bVisible[S]);
			Parts[S].Add(C);
		}
	}
}

void FFTWornAccessories::SetSlotVisible(int32 Slot, bool bInVisible)
{
	if (Slot < 0 || Slot >= NumSlots || bVisible[Slot] == bInVisible)
	{
		return;
	}
	bVisible[Slot] = bInVisible;
	for (const TWeakObjectPtr<UStaticMeshComponent>& P : Parts[Slot])
	{
		if (P.IsValid())
		{
			P->SetVisibility(bInVisible);
		}
	}
}

void FFTWornAccessories::Clear()
{
	for (int32 S = 0; S < NumSlots; ++S)
	{
		for (const TWeakObjectPtr<UStaticMeshComponent>& P : Parts[S])
		{
			if (P.IsValid())
			{
				P->DestroyComponent();
			}
		}
		Parts[S].Reset();
		Current[S] = NAME_None;
	}
}

// ============================================================================ placeable item

AFTShopItemActor::AFTShopItemActor()
{
	PropTag = TEXT("Prop.ShopItem");
	PropName = LOCTEXT("ShopItem", "Studio upgrade");
	bFloats = true;
	FloatSink = 10.f;

	Switch = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("Switch"));
	Switch->SetupAttachment(Visual);
	Switch->Setup(TEXT("Switch"), LOCTEXT("EffectSwitch", "Effect switch"), LOCTEXT("SwitchOn", "Switch on"), EFTInteractType::Press, FVector(14.f, 18.f, 18.f));

	Fx = CreateDefaultSubobject<UFTChunkyParticles>(TEXT("Fx"));
	Fx->SetupAttachment(Visual);
	Fx2 = CreateDefaultSubobject<UFTChunkyParticles>(TEXT("Fx2"));
	Fx2->SetupAttachment(Visual);

	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(Visual);
	GlowLight->SetIntensity(2600.f);
	GlowLight->SetAttenuationRadius(380.f);
	GlowLight->SetCastShadows(false);
	GlowLight->SetVisibility(false);
}

void AFTShopItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTShopItemActor, ItemId);
	DOREPLIFETIME(AFTShopItemActor, bFxOn);
}

void AFTShopItemActor::InitItem(FName InItemId)
{
	ItemId = InItemId;
}

const FFTShopItemDef* AFTShopItemActor::GetDef() const
{
	return ItemId.IsNone() ? nullptr : UFTEconomyConfig::Get()->FindItem(ItemId);
}

bool AFTShopItemActor::IsEffect() const
{
	const FFTShopItemDef* Def = GetDef();
	return Def && Def->Category == EFTShopCategory::Effect;
}

bool AFTShopItemActor::IsShowcased() const
{
	if (!GetDef() || IsActorBeingDestroyed())
	{
		return false;
	}
	return !IsEffect() || bFxOn;
}

void AFTShopItemActor::BeginPlay()
{
	Super::BeginPlay();
	BuildLook();
	OnRep_FxOn();
}

void AFTShopItemActor::OnRep_ItemId()
{
	BuildLook();
	OnRep_FxOn();
}

void AFTShopItemActor::BuildLook()
{
	const FFTShopItemDef* Def = GetDef();
	if (bBuilt || !Def)
	{
		return;
	}
	bBuilt = true;
	PropName = Def->Name;
	Tags.AddUnique(Def->ItemId);
	FTShop::BuildParts(this, Visual, *Def, Parts);
	PartGlow.Reset();
	for (int32 i = 0; i < Parts.Num(); ++i)
	{
		PartGlow.Add(Def->Parts.IsValidIndex(i) ? Def->Parts[i].Emissive : 0.f);
	}

	const FBox B = FTShop::PartsBounds(*Def);
	const FVector Ext = B.GetExtent();
	Grab->SetBoxExtent(FVector(FMath::Max(Ext.X, 20.f), FMath::Max(Ext.Y, 20.f), FMath::Max(Ext.Z, 20.f)));
	Grab->SetRelativeLocation(B.GetCenter());
	Grab->Label = Def->Name;
	// big pieces ride at the side like the stand-in, small ones are held in front
	const bool bBig = FMath::Max(Ext.X, Ext.Y) > 48.f || Ext.Z > 70.f;
	CarryOffset = bBig ? FVector(55.f, 60.f + Ext.Y * 0.35f, -110.f) : FVector(16.f, 0.f, -B.GetCenter().Z);
	CarryRotation = bBig ? FRotator(0.f, -70.f, 0.f) : FRotator::ZeroRotator;
	FloatSink = FMath::Clamp(Ext.Z * 0.35f, 4.f, 30.f);

	if (Def->Category == EFTShopCategory::Effect)
	{
		// control box with a status lamp on the back of the unit
		const FVector BoxLoc(B.Min.X - 8.f, 0.f, 20.f);
		Parts.Add(FTVis::SpawnPart(this, Visual, EFTShape::Box, BoxLoc, FVector(12.f, 26.f, 30.f), Charcoal));
		PartGlow.Add(0.f);
		SwitchLamp = FTVis::SpawnPart(this, Visual, EFTShape::Box, BoxLoc + FVector(-6.5f, 0.f, 6.f), FVector(2.f, 14.f, 10.f), Red, FRotator::ZeroRotator, 2.f);
		Switch->SetRelativeLocation(BoxLoc);
		Switch->Label = Def->Name;
		Switch->AddHighlight(SwitchLamp);
		Switch->SetInteractionEnabled(true);
	}
	else
	{
		Switch->SetInteractionEnabled(false);
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		ConfigureFx(*Def, B);
	}
}

void AFTShopItemActor::ConfigureFx(const FFTShopItemDef& Def, const FBox& B)
{
	switch (Def.Fx)
	{
	case EFTItemFx::Sparks:
		for (UFTChunkyParticles* P : { Fx.Get(), Fx2.Get() })
		{
			const bool bMain = P == Fx;
			P->Configure(EFTShape::Sphere, bMain ? Yellow : Orange, bMain ? 16.f : 10.f, false);
			P->SetRelativeLocation(FVector(0.f, 0.f, B.Max.Z));
			P->MaxParticles = 150;
			P->SpawnRate = bMain ? 70.f : 45.f;
			P->Lifetime = FVector2D(0.45f, 0.9f);
			P->SpawnExtent = FVector(3.f);
			P->BaseVelocity = FVector(0.f, 0.f, bMain ? 560.f : 420.f);
			P->VelocityJitter = FVector(120.f, 120.f, 140.f);
			P->Gravity = 950.f;
			P->StartSize = FVector2D(2.5f, 5.f);
			P->EndSizeScale = 0.3f;
			P->bSpin = false;
		}
		GlowLight->SetRelativeLocation(FVector(0.f, 0.f, B.Max.Z + 60.f));
		GlowLight->SetLightColor(FLinearColor(1.f, 0.62f, 0.25f));
		break;
	case EFTItemFx::Fog:
		Fx->Configure(EFTShape::Ball, FLinearColor(0.82f, 0.8f, 0.95f, 0.32f), 0.15f, true);
		Fx->SetRelativeLocation(FVector(B.Max.X, 0.f, 22.f));
		Fx->MaxParticles = 70;
		Fx->SpawnRate = 10.f;
		Fx->Lifetime = FVector2D(5.f, 7.f);
		Fx->SpawnExtent = FVector(8.f, 12.f, 4.f);
		Fx->BaseVelocity = FVector(120.f, 0.f, -3.f);
		Fx->VelocityJitter = FVector(40.f, 70.f, 4.f);
		Fx->Drag = 0.3f;
		Fx->StartSize = FVector2D(36.f, 56.f);
		Fx->EndSizeScale = 3.2f;
		break;
	case EFTItemFx::Confetti:
		for (UFTChunkyParticles* P : { Fx.Get(), Fx2.Get() })
		{
			const bool bMain = P == Fx;
			P->Configure(EFTShape::Cube, bMain ? Magenta : Cyan, 0.4f, false);
			P->SetRelativeLocation(FVector(24.f, 0.f, B.Max.Z));
			P->MaxParticles = 160;
			P->SpawnRate = 28.f;
			P->Lifetime = FVector2D(2.5f, 3.5f);
			P->SpawnExtent = FVector(6.f);
			P->BaseVelocity = FVector(220.f, 0.f, 640.f);
			P->VelocityJitter = FVector(220.f, 240.f, 160.f);
			P->Gravity = 420.f;
			P->Drag = 1.1f;
			P->StartSize = FVector2D(4.f, 6.f);
			P->bRestOnFloor = true;
		}
		break;
	case EFTItemFx::Bubbles:
		Fx->Configure(EFTShape::Ball, FLinearColor(0.78f, 0.95f, 1.f, 0.35f), 0.5f, true);
		Fx->SetRelativeLocation(FVector(12.f, 0.f, B.Max.Z + 8.f));
		Fx->MaxParticles = 70;
		Fx->SpawnRate = 12.f;
		Fx->Lifetime = FVector2D(3.f, 4.5f);
		Fx->SpawnExtent = FVector(6.f, 10.f, 6.f);
		Fx->BaseVelocity = FVector(70.f, 0.f, 45.f);
		Fx->VelocityJitter = FVector(50.f, 80.f, 30.f);
		Fx->Gravity = -12.f;
		Fx->Drag = 0.45f;
		Fx->StartSize = FVector2D(8.f, 16.f);
		Fx->EndSizeScale = 1.2f;
		break;
	case EFTItemFx::Glow:
		// always-on practicals: a warm light that breathes with the emissive parts
		GlowLight->SetRelativeLocation(B.GetCenter() + FVector(30.f, 0.f, B.GetExtent().Z * 0.4f));
		GlowLight->SetLightColor(Def.Swatch);
		GlowLight->SetIntensity(2200.f);
		GlowLight->SetVisibility(true);
		break;
	default:
		break;
	}
}

void AFTShopItemActor::OnRep_FxOn()
{
	const FFTShopItemDef* Def = GetDef();
	if (!Def || !bBuilt)
	{
		return;
	}
	const bool bParticles = Def->Fx != EFTItemFx::None && Def->Fx != EFTItemFx::Glow;
	if (bParticles && GetNetMode() != NM_DedicatedServer)
	{
		const float Floor = GetActorLocation().Z + 1.f;
		Fx->FloorZ = Floor;
		Fx2->FloorZ = Floor;
		Fx->SetEmitting(bFxOn);
		Fx2->SetEmitting(bFxOn && (Def->Fx == EFTItemFx::Sparks || Def->Fx == EFTItemFx::Confetti));
		GlowLight->SetVisibility(bFxOn && Def->Fx == EFTItemFx::Sparks);
	}
	if (SwitchLamp)
	{
		FTVis::Paint(SwitchLamp, bFxOn ? Green : Red, bFxOn ? 4.f : 2.f);
	}
}

void AFTShopItemActor::SetFxOn(bool bOn)
{
	if (!HasAuthority() || !IsEffect() || bFxOn == bOn)
	{
		return;
	}
	bFxOn = bOn;
	OnRep_FxOn();
	const FFTShopItemDef* Def = GetDef();
	EFTSound Sfx = bOn ? EFTSound::PowerUp : EFTSound::PowerDown;
	float Pitch = 1.2f;
	if (bOn && Def)
	{
		switch (Def->Fx)
		{
		case EFTItemFx::Confetti: Sfx = EFTSound::FoamPop; Pitch = 1.3f; break;
		case EFTItemFx::Fog: Sfx = EFTSound::SmokeHiss; Pitch = 0.9f; break;
		case EFTItemFx::Sparks: Sfx = EFTSound::SmokeHiss; Pitch = 1.7f; break;
		case EFTItemFx::Bubbles: Sfx = EFTSound::Boing; Pitch = 1.8f; break;
		default: break;
		}
	}
	MulticastSound(Sfx, GetActorLocation(), 0.7f, Pitch);
	ForceNetUpdate();
}

bool AFTShopItemActor::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	if (Comp == Switch)
	{
		if (Carrier || Holder)
		{
			OutReason = LOCTEXT("SetDownFirst", "Set it down on the set first");
			return false;
		}
		return true;
	}
	return Super::CanInteract(Comp, User, OutReason);
}

FText AFTShopItemActor::GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const
{
	if (Comp == Switch)
	{
		return bFxOn ? LOCTEXT("SwitchOff", "Switch off") : LOCTEXT("SwitchOnVerb", "Switch on");
	}
	return Super::GetPromptVerb(Comp, User);
}

void AFTShopItemActor::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	if (Comp == Switch)
	{
		SetFxOn(!bFxOn);
		return;
	}
	Super::OnInteract(Comp, User);
	if (Carrier)
	{
		SetFxOn(false); // nobody runs a pyro fountain while carrying it
	}
}

void AFTShopItemActor::ResetForNewShoot()
{
	// purchased dressing stays where the crew put it; only running effects are switched off
	SetFxOn(false);
}

void AFTShopItemActor::Land(const FVector& Location)
{
	Super::Land(Location);
	if (HasAuthority())
	{
		HomeTransform = GetActorTransform();
		if (AFTCareerManager* CM = AFTCareerManager::Get(this))
		{
			CM->SetPlacement(ItemId, GetActorTransform());
		}
	}
}

bool AFTShopItemActor::GetSubjectBounds(FName SubjectTag, FVector& OutCenter, float& OutRadius) const
{
	const FFTShopItemDef* Def = GetDef();
	if (!Def)
	{
		return Super::GetSubjectBounds(SubjectTag, OutCenter, OutRadius);
	}
	const FBox B = FTShop::PartsBounds(*Def);
	OutCenter = Visual->GetComponentTransform().TransformPosition(B.GetCenter());
	if (Def->Category == EFTShopCategory::Effect)
	{
		// the effect itself (sparks, fog, confetti) is what the camera sees
		OutCenter += FVector(0.f, 0.f, Def->FrameRadius * 0.4f);
	}
	OutRadius = Def->FrameRadius;
	return true;
}

void AFTShopItemActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FFTShopItemDef* Def = GetDef();
	if (!Def || Def->Fx != EFTItemFx::Glow || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// breathing practicals (torch flames, neon, moon glow)
	GlowPhase += DeltaSeconds;
	const float Flicker = 0.85f + 0.12f * FMath::Sin(GlowPhase * 3.1f) + 0.06f * FMath::Sin(GlowPhase * 11.7f);
	for (int32 i = 0; i < Parts.Num() && i < PartGlow.Num(); ++i)
	{
		if (PartGlow[i] > 0.f && Parts[i])
		{
			FTVis::SetGlow(Parts[i], PartGlow[i] * Flicker);
		}
	}
	GlowLight->SetIntensity(2200.f * Flicker);
}

// ============================================================================ delivery bay

AFTDeliveryBay::AFTDeliveryBay()
{
	Title = LOCTEXT("BayTitle", "LOADING BAY");
	TitleText = FTVis::MakeText(this, Root, TEXT("Title"), Title, FVector(0.f, 0.f, 1.5f), FRotator(90.f, 0.f, 0.f), 30.f, FColor(255, 214, 90));
}

void AFTDeliveryBay::GetSlots(TArray<FTransform>& Out) const
{
	const FTransform T = GetActorTransform();
	for (int32 C = 0; C < Columns; ++C)
	{
		for (int32 R = 0; R < Rows; ++R)
		{
			const FVector Local((C - (Columns - 1) * 0.5f) * Spacing.X, (R - (Rows - 1) * 0.5f) * Spacing.Y, 0.f);
			Out.Add(FTransform(T.GetRotation(), T.TransformPosition(Local)));
		}
	}
}

void AFTDeliveryBay::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// painted pallet squares with numbers, title at the front edge (readable from +X)
	TitleText->SetText(Title);
	TitleText->SetRelativeLocation(FVector(Columns * Spacing.X * 0.5f + 34.f, 0.f, 1.5f));
	int32 N = 1;
	for (int32 C = 0; C < Columns; ++C)
	{
		for (int32 R = 0; R < Rows; ++R)
		{
			const FVector L((C - (Columns - 1) * 0.5f) * Spacing.X, (R - (Rows - 1) * 0.5f) * Spacing.Y, 0.6f);
			const float W = Spacing.X - 24.f, H = Spacing.Y - 24.f;
			FTVis::SpawnPart(this, Root, EFTShape::Cube, L + FVector(W * 0.5f, 0.f, 0.f), FVector(6.f, H, 1.f), Yellow, FRotator::ZeroRotator, 0.4f);
			FTVis::SpawnPart(this, Root, EFTShape::Cube, L - FVector(W * 0.5f, 0.f, 0.f), FVector(6.f, H, 1.f), Yellow, FRotator::ZeroRotator, 0.4f);
			FTVis::SpawnPart(this, Root, EFTShape::Cube, L + FVector(0.f, H * 0.5f, 0.f), FVector(W, 6.f, 1.f), Yellow, FRotator::ZeroRotator, 0.4f);
			FTVis::SpawnPart(this, Root, EFTShape::Cube, L - FVector(0.f, H * 0.5f, 0.f), FVector(W, 6.f, 1.f), Yellow, FRotator::ZeroRotator, 0.4f);
			SpawnText(this, Root, FText::AsNumber(N++), L + FVector(W * 0.5f - 22.f, -H * 0.5f + 22.f, 1.f), FRotator(90.f, 0.f, 0.f), 20.f, FColor(255, 214, 90));
		}
	}
}

// ============================================================================ shop terminal

AFTShopTerminal::AFTShopTerminal()
{
	PrimaryActorTick.bCanEverTick = true;
	auto Part = [this](const TCHAR* Name, EFTShape Shape, FVector Loc, FVector Size, FLinearColor C, FRotator R = FRotator::ZeroRotator, float Glow = 0.f, bool bSolid = false)
	{
		return FTVis::MakePart(this, Root, Name, Shape, Loc, Size, C, R, Glow, bSolid);
	};
	// counter (customers stand on the +X side)
	Part(TEXT("Counter"), EFTShape::Box, FVector(0.f, 0.f, 52.f), FVector(90.f, 300.f, 104.f), Coral, FRotator::ZeroRotator, 0.f, true);
	Part(TEXT("CounterTop"), EFTShape::Box, FVector(4.f, 0.f, 108.f), FVector(104.f, 316.f, 8.f), Cream);
	Part(TEXT("CounterKick"), EFTShape::Box, FVector(40.f, 0.f, 8.f), FVector(12.f, 304.f, 16.f), CoralDark);
	for (int32 i = 0; i < 3; ++i)
	{
		Part(*FString::Printf(TEXT("CounterStripe%d"), i), EFTShape::Box, FVector(46.f, 0.f, 36.f + i * 22.f), FVector(2.f, 290.f, 6.f), i == 1 ? Yellow : Cream, FRotator::ZeroRotator, i == 1 ? 0.6f : 0.f);
	}
	// cash register
	Part(TEXT("Register"), EFTShape::Box, FVector(-4.f, -105.f, 128.f), FVector(44.f, 50.f, 32.f), Charcoal);
	Part(TEXT("RegisterTop"), EFTShape::Box, FVector(-12.f, -105.f, 154.f), FVector(26.f, 46.f, 20.f), GreyDark, FRotator(-20.f, 0.f, 0.f));
	RegisterDisplay = Part(TEXT("RegisterDisplay"), EFTShape::Box, FVector(0.f, -105.f, 158.f), FVector(2.f, 34.f, 10.f), Green, FRotator(-20.f, 0.f, 0.f), 3.f);
	for (int32 k = 0; k < 6; ++k)
	{
		Part(*FString::Printf(TEXT("RegisterKey%d"), k), EFTShape::Box, FVector(12.f + (k / 3) * 8.f, -118.f + (k % 3) * 12.f, 146.f), FVector(6.f, 9.f, 4.f), k == 5 ? Coral : Cream);
	}
	// catalogue book on a little stand
	UStaticMeshComponent* BookParts[] = {
		Part(TEXT("BookStand"), EFTShape::Box, FVector(10.f, 10.f, 118.f), FVector(40.f, 70.f, 8.f), WoodDark, FRotator(-25.f, 0.f, 0.f)),
		Part(TEXT("BookL"), EFTShape::Box, FVector(12.f, -8.f, 126.f), FVector(34.f, 34.f, 3.f), Cream, FRotator(-25.f, 0.f, -8.f)),
		Part(TEXT("BookR"), EFTShape::Box, FVector(12.f, 28.f, 126.f), FVector(34.f, 34.f, 3.f), Cream, FRotator(-25.f, 0.f, 8.f)),
		Part(TEXT("BookCover"), EFTShape::Box, FVector(10.f, 10.f, 123.f), FVector(38.f, 74.f, 2.f), Teal, FRotator(-25.f, 0.f, 0.f)) };
	// turntable with the featured purchase
	Part(TEXT("TurntableBase"), EFTShape::Cylinder, FVector(0.f, 100.f, 116.f), FVector(70.f, 70.f, 8.f), Navy);
	Part(TEXT("TurntableRim"), EFTShape::Torus, FVector(0.f, 100.f, 120.f), FVector(72.f, 72.f, 5.f), Yellow, FRotator::ZeroRotator, 1.f);
	Turntable = CreateDefaultSubobject<USceneComponent>(TEXT("Turntable"));
	Turntable->SetupAttachment(Root);
	Turntable->SetRelativeLocation(FVector(0.f, 100.f, 121.f));
	FeaturedLabel = FTVis::MakeText(this, Root, TEXT("FeaturedLabel"), LOCTEXT("NoFeatured", "NEW STOCK EVERY NIGHT"), FVector(47.5f, 0.f, 80.f), FRotator::ZeroRotator, 13.f, FColor(255, 240, 220));
	Confetti = CreateDefaultSubobject<UFTChunkyParticles>(TEXT("Confetti"));
	Confetti->SetupAttachment(Root);
	Confetti->SetRelativeLocation(FVector(0.f, 100.f, 150.f));
	Confetti->MaxParticles = 120;
	Confetti->SpawnRate = 0.f;
	Confetti->Lifetime = FVector2D(1.6f, 2.4f);
	Confetti->SpawnExtent = FVector(10.f);
	Confetti->BaseVelocity = FVector(60.f, 0.f, 420.f);
	Confetti->VelocityJitter = FVector(260.f, 260.f, 160.f);
	Confetti->Gravity = 520.f;
	Confetti->Drag = 1.f;
	Confetti->StartSize = FVector2D(4.f, 6.f);
	Confetti->bRestOnFloor = true;

	// back wall: shelves full of boxes and the big sign
	Part(TEXT("ShelfBack"), EFTShape::Box, FVector(-80.f, 0.f, 150.f), FVector(12.f, 360.f, 300.f), NavyLight);
	for (int32 s = 0; s < 2; ++s)
	{
		const float Z = 150.f + s * 70.f;
		Part(*FString::Printf(TEXT("Shelf%d"), s), EFTShape::Box, FVector(-62.f, 0.f, Z), FVector(32.f, 340.f, 5.f), Wood);
		for (int32 b = 0; b < 7; ++b)
		{
			const FLinearColor BC = (b + s) % 4 == 0 ? Teal : ((b + s) % 4 == 1 ? Yellow : ((b + s) % 4 == 2 ? Coral : Magenta));
			const float H = 26.f + ((b * 7 + s * 3) % 3) * 8.f;
			Part(*FString::Printf(TEXT("Stock%d_%d"), s, b), EFTShape::Box, FVector(-62.f, -140.f + b * 46.f, Z + 3.f + H * 0.5f), FVector(24.f, 34.f, H), BC);
		}
	}
	Part(TEXT("Sign"), EFTShape::Box, FVector(-70.f, 0.f, 335.f), FVector(14.f, 400.f, 80.f), Navy);
	Part(TEXT("SignNeonTop"), EFTShape::Box, FVector(-62.f, 0.f, 373.f), FVector(4.f, 392.f, 5.f), Magenta, FRotator::ZeroRotator, 4.f);
	Part(TEXT("SignNeonBottom"), EFTShape::Box, FVector(-62.f, 0.f, 297.f), FVector(4.f, 392.f, 5.f), Magenta, FRotator::ZeroRotator, 4.f);
	FTVis::MakeText(this, Root, TEXT("SignText"), LOCTEXT("SupplySign", "STUDIO SUPPLY CO."), FVector(-62.f, 0.f, 342.f), FRotator::ZeroRotator, 44.f, FColor(255, 214, 90));
	FTVis::MakeText(this, Root, TEXT("SignSub"), LOCTEXT("SupplySub", "COSTUMES - PROPS - EFFECTS - SHARK KITS - SET PIECES"), FVector(-62.f, 0.f, 312.f), FRotator::ZeroRotator, 11.f, FColor(255, 240, 220));

	Browse = CreateDefaultSubobject<UFTInteractableComponent>(TEXT("Browse"));
	Browse->SetupAttachment(Root);
	Browse->Setup(TEXT("Browse"), LOCTEXT("ShopLabel", "Studio Supply Co."), LOCTEXT("BrowseVerb", "Browse the catalogue"), EFTInteractType::Press, FVector(56.f, 160.f, 70.f));
	Browse->SetRelativeLocation(FVector(20.f, 0.f, 100.f));
	for (UStaticMeshComponent* B : BookParts)
	{
		Browse->AddHighlight(B);
	}
	Browse->AddHighlight(RegisterDisplay);

	// the catalogue's photo studio: far above the roof, only ever seen by the scene capture
	PreviewRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewRoot"));
	PreviewRoot->SetupAttachment(Root);
	PreviewRoot->SetRelativeLocation(FVector(0.f, 0.f, 3200.f));
	PreviewRoot->SetUsingAbsoluteRotation(true);
	PreviewSpin = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewSpin"));
	PreviewSpin->SetupAttachment(PreviewRoot);
	PreviewCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PreviewCapture"));
	PreviewCapture->SetupAttachment(PreviewRoot);
	PreviewCapture->SetRelativeLocation(FVector(520.f, 0.f, 150.f));
	PreviewCapture->SetRelativeRotation(FRotator(-6.f, 180.f, 0.f));
	PreviewCapture->FOVAngle = 30.f;
	PreviewCapture->bCaptureEveryFrame = false;
	PreviewCapture->bCaptureOnMovement = false;
	PreviewCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	PreviewCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	PreviewCapture->ShowFlags.SetLumenGlobalIllumination(false);
	PreviewCapture->ShowFlags.SetLumenReflections(false);
	PreviewCapture->ShowFlags.SetDistanceFieldAO(false);
	PreviewCapture->ShowFlags.SetVolumetricFog(false);
	PreviewCapture->ShowFlags.SetMotionBlur(false);
	PreviewCapture->ShowFlags.SetFog(false);
	PreviewCapture->ShowFlags.SetAtmosphere(false);
	{
		FPostProcessSettings& PP = PreviewCapture->PostProcessSettings;
		PP.bOverride_AutoExposureMethod = true;
		PP.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		PP.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		PP.AutoExposureApplyPhysicalCameraExposure = 0;
		PP.bOverride_AutoExposureBias = true;
		PP.AutoExposureBias = 0.f;
		PP.bOverride_VignetteIntensity = true;
		PP.VignetteIntensity = 0.25f;
		PP.bOverride_BloomIntensity = true;
		PP.BloomIntensity = 0.3f;
		PreviewCapture->PostProcessBlendWeight = 1.f;
	}
	PreviewKey = CreateDefaultSubobject<UPointLightComponent>(TEXT("PreviewKey"));
	PreviewKey->SetupAttachment(PreviewRoot);
	PreviewKey->SetRelativeLocation(FVector(220.f, -170.f, 260.f));
	PreviewKey->SetIntensity(9000.f);
	PreviewKey->SetAttenuationRadius(900.f);
	PreviewKey->SetLightColor(FLinearColor(1.f, 0.93f, 0.84f));
	PreviewKey->SetCastShadows(true);
	PreviewKey->SetVisibility(false);
	PreviewFill = CreateDefaultSubobject<UPointLightComponent>(TEXT("PreviewFill"));
	PreviewFill->SetupAttachment(PreviewRoot);
	PreviewFill->SetRelativeLocation(FVector(180.f, 220.f, 120.f));
	PreviewFill->SetIntensity(3800.f);
	PreviewFill->SetAttenuationRadius(900.f);
	PreviewFill->SetLightColor(FLinearColor(0.7f, 0.85f, 1.f));
	PreviewFill->SetCastShadows(false);
	PreviewFill->SetVisibility(false);
}

void AFTShopTerminal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTShopTerminal, Featured);
}

void AFTShopTerminal::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		if (AFTCareerManager* CM = AFTCareerManager::Get(this))
		{
			CM->OnPurchased.AddUObject(this, &AFTShopTerminal::HandlePurchased);
			CM->OnCareerReset.AddUObject(this, &AFTShopTerminal::HandleCareerReset);
			SyncOwnedItems();
		}
	}
	RebuildFeatured();
}

void AFTShopTerminal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AFTCareerManager* CM = AFTCareerManager::Get(this))
	{
		CM->OnPurchased.RemoveAll(this);
		CM->OnCareerReset.RemoveAll(this);
	}
	PreviewUsers = 0;
	Super::EndPlay(EndPlayReason);
}

FText AFTShopTerminal::GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const
{
	return LOCTEXT("BrowseVerb2", "Browse the catalogue");
}

void AFTShopTerminal::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	if (AFTPlayerController* PC = User ? Cast<AFTPlayerController>(User->GetController()) : nullptr)
	{
		PC->ClientOpenShop(this);
		MulticastSound(EFTSound::Rustle, Comp->GetComponentLocation(), 0.8f, 0.9f);
	}
}

// ---------------------------------------------------------------- server: world <-> career

void AFTShopTerminal::SyncOwnedItems()
{
	UWorld* World = GetWorld();
	const AFTGameState* G = World ? World->GetGameState<AFTGameState>() : nullptr;
	if (!HasAuthority() || !G)
	{
		return;
	}
	const UFTEconomyConfig* Cfg = UFTEconomyConfig::Get();
	// remove world objects the career does not own (after a reset)
	TArray<AFTShopItemActor*> Stale;
	for (TActorIterator<AFTShopItemActor> It(World); It; ++It)
	{
		if (!G->IsOwned(It->GetItemId()))
		{
			Stale.Add(*It);
		}
	}
	for (AFTShopItemActor* A : Stale)
	{
		if (AFTCharacter* C = A->GetCarrier())
		{
			C->HeldProp = nullptr;
			C->ForceNetUpdate();
		}
		A->Destroy();
	}
	for (const FName Id : G->OwnedItems)
	{
		const FFTShopItemDef* Def = Cfg->FindItem(Id);
		if (Def && IsPlaceable(Def->Category) && !FTShop::FindItemActor(World, Id))
		{
			SpawnItem(Id, false);
		}
	}
	bSynced = true;
}

bool AFTShopTerminal::FindDeliverySpot(FTransform& Out) const
{
	TArray<AFTDeliveryBay*> Bays;
	for (TActorIterator<AFTDeliveryBay> It(GetWorld()); It; ++It)
	{
		Bays.Add(*It);
	}
	Bays.Sort([](const AFTDeliveryBay& A, const AFTDeliveryBay& B) { return A.Priority < B.Priority; });
	TArray<FVector> Taken;
	for (TActorIterator<AFTShopItemActor> It(GetWorld()); It; ++It)
	{
		if (!It->GetCarrier())
		{
			Taken.Add(It->GetActorLocation());
		}
	}
	for (const AFTDeliveryBay* Bay : Bays)
	{
		TArray<FTransform> Slots;
		Bay->GetSlots(Slots);
		for (const FTransform& S : Slots)
		{
			const bool bFree = !Taken.ContainsByPredicate([&S](const FVector& P) { return FVector::Dist2D(P, S.GetLocation()) < 110.f; });
			if (bFree)
			{
				Out = S;
				return true;
			}
		}
	}
	return false;
}

AFTShopItemActor* AFTShopTerminal::SpawnItem(FName Id, bool bAnnounceDelivery)
{
	UWorld* World = GetWorld();
	AFTCareerManager* CM = AFTCareerManager::Get(this);
	FTransform Where;
	bool bSaved = CM && CM->GetPlacement(Id, Where);
	if (bSaved)
	{
		// a saved spot must still have floor under it (never spawn into the void)
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(FTShopPlace), false);
		bSaved = Where.GetLocation().Z > -1000.f && World->LineTraceSingleByChannel(Hit, Where.GetLocation() + FVector(0.f, 0.f, 60.f), Where.GetLocation() - FVector(0.f, 0.f, 200.f), ECC_Visibility, Params);
	}
	if (!bSaved)
	{
		if (!FindDeliverySpot(Where))
		{
			Where = FTransform(GetActorRotation(), GetActorTransform().TransformPosition(FVector(220.f, 0.f, 0.f)));
		}
		Where.SetLocation(SnapToFloor(World, Where.GetLocation()));
	}
	Where.SetScale3D(FVector::OneVector);
	AFTShopItemActor* A = World->SpawnActorDeferred<AFTShopItemActor>(AFTShopItemActor::StaticClass(), Where, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!A)
	{
		return nullptr;
	}
	A->InitItem(Id);
	A->FinishSpawning(Where);
	if (CM && !bSaved)
	{
		CM->SetPlacement(Id, Where);
	}
	if (bAnnounceDelivery)
	{
		if (AFTGameState* G = World->GetGameState<AFTGameState>())
		{
			G->MulticastPing(Where.GetLocation() + FVector(0.f, 0.f, 60.f), 1, TEXT("DELIVERY"));
		}
	}
	UE_LOG(LogFinalTake, Display, TEXT("[Shop] %s placed at %s (%s)"), *Id.ToString(), *Where.GetLocation().ToCompactString(), bSaved ? TEXT("saved spot") : TEXT("loading bay"));
	return A;
}

void AFTShopTerminal::HandlePurchased(FName Id)
{
	const FFTShopItemDef* Def = UFTEconomyConfig::Get()->FindItem(Id);
	if (!Def)
	{
		return; // cars and stages are handled by the dealer / stage doors
	}
	if (IsPlaceable(Def->Category) && !FTShop::FindItemActor(GetWorld(), Id))
	{
		SpawnItem(Id, true);
	}
	Featured = Id;
	OnRep_Featured();
	MulticastCelebrate(Id);
	ForceNetUpdate();
}

void AFTShopTerminal::HandleCareerReset()
{
	Featured = NAME_None;
	OnRep_Featured();
	SyncOwnedItems();
}

// ---------------------------------------------------------------- featured purchase on the counter

void AFTShopTerminal::OnRep_Featured()
{
	RebuildFeatured();
}

void AFTShopTerminal::RebuildFeatured()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	DestroyParts(FeaturedParts);
	const FFTShopItemDef* Def = Featured.IsNone() ? nullptr : UFTEconomyConfig::Get()->FindItem(Featured);
	if (!Def)
	{
		FeaturedLabel->SetText(LOCTEXT("NoFeatured2", "NEW STOCK EVERY NIGHT"));
		return;
	}
	const FBox B = FTShop::PartsBounds(*Def);
	const float Size = FMath::Max(B.GetSize().GetMax(), 1.f);
	const float Scale = FMath::Clamp(62.f / Size, 0.2f, 2.2f);
	Turntable->SetRelativeScale3D(FVector(Scale));
	FTShop::BuildParts(this, Turntable, *Def, FeaturedParts);
	for (UStaticMeshComponent* P : FeaturedParts)
	{
		// sit the scaled item on the plate
		P->SetRelativeLocation(P->GetRelativeLocation() - FVector(B.GetCenter().X, B.GetCenter().Y, B.Min.Z));
	}
	FeaturedLabel->SetText(FText::Format(LOCTEXT("FeaturedFmt", "JUST BOUGHT: {0}"), Def->Name));
}

void AFTShopTerminal::MulticastCelebrate_Implementation(FName Id)
{
	CelebrateTime = GetWorld()->GetTimeSeconds();
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	Confetti->Configure(EFTShape::Cube, Yellow, 0.6f, false);
	Confetti->FloorZ = GetActorLocation().Z + 1.f;
	Confetti->Burst(60, FVector::ZeroVector, 1.f);
	FTAudio::PlayAt(this, EFTSound::Stamp, Turntable->GetComponentLocation(), 1.f, 1.3f);
	FTAudio::PlayAt(this, EFTSound::ApplauseSmall, Turntable->GetComponentLocation(), 0.6f, 1.1f);
}

// ---------------------------------------------------------------- local catalogue preview

UTextureRenderTarget2D* AFTShopTerminal::BeginPreview()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}
	if (!PreviewTarget)
	{
		PreviewTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, 640, 400, RTF_RGBA8);
		PreviewCapture->TextureTarget = PreviewTarget;
	}
	if (PreviewBackdrop.Num() == 0)
	{
		// cyclorama: floor, curved back wall, a warm halo disc behind the product
		const FLinearColor Wall = Hex(0x2F5F80);
		PreviewBackdrop.Add(FTVis::SpawnPart(this, PreviewRoot, EFTShape::Cylinder, FVector(0.f, 0.f, -3.f), FVector(900.f, 900.f, 6.f), Hex(0xE9D8B8), FRotator::ZeroRotator, 0.08f));
		for (int32 i = 0; i < 9; ++i)
		{
			const float A = -80.f + i * 20.f;
			const FVector Dir = FRotator(0.f, 180.f + A, 0.f).Vector();
			PreviewBackdrop.Add(FTVis::SpawnPart(this, PreviewRoot, EFTShape::Box, Dir * 300.f + FVector(0.f, 0.f, 200.f), FVector(12.f, 112.f, 420.f), Wall, FRotator(0.f, A, 0.f), 0.18f));
		}
		PreviewBackdrop.Add(FTVis::SpawnPart(this, PreviewRoot, EFTShape::Cylinder, FVector(-280.f, 0.f, 110.f), FVector(260.f, 260.f, 4.f), Hex(0xFFB35C), FRotator(90.f, 0.f, 0.f), 0.7f));
		PreviewPlinth = FTVis::SpawnPart(this, PreviewRoot, EFTShape::Cylinder, FVector(0.f, 0.f, 20.f), FVector(110.f, 110.f, 40.f), Hex(0xF2E6D0));
		PreviewBackdrop.Add(PreviewPlinth);
		PreviewBackdrop.Add(FTVis::SpawnPart(this, PreviewRoot, EFTShape::Torus, FVector(0.f, 0.f, 2.f), FVector(118.f, 118.f, 6.f), Hex(0xE8B04A), FRotator::ZeroRotator, 0.4f));
		for (UStaticMeshComponent* P : PreviewBackdrop)
		{
			P->SetVisibleInSceneCaptureOnly(true);
		}
	}
	++PreviewUsers;
	PreviewKey->SetVisibility(true);
	PreviewFill->SetVisibility(true);
	PreviewCapture->bCaptureEveryFrame = true;
	return PreviewTarget;
}

void AFTShopTerminal::SetPreviewItem(FName ItemId)
{
	if (GetNetMode() == NM_DedicatedServer || ItemId == PreviewItem)
	{
		return;
	}
	PreviewItem = ItemId;
	DestroyParts(PreviewParts);
	PreviewCapture->ClearShowOnlyComponents();
	for (UStaticMeshComponent* P : PreviewBackdrop)
	{
		PreviewCapture->ShowOnlyComponent(P);
	}
	const FFTShopItemDef* Def = ItemId.IsNone() ? nullptr : UFTEconomyConfig::Get()->FindItem(ItemId);
	if (!Def)
	{
		return;
	}
	const FBox B = FTShop::PartsBounds(*Def);
	const float Size = FMath::Max(B.GetSize().GetMax(), 1.f);
	// every product is shown at about the same size, standing on a plinth at lens height
	// (shark kits are offsets on the shark body, so everything is recentred on the plinth)
	const float Scale = FMath::Clamp(80.f / Size, 0.2f, 3.f);
	const float Bottom = FMath::Max(8.f, 100.f - B.GetExtent().Z * Scale);
	PreviewSpin->SetRelativeScale3D(FVector(Scale));
	PreviewSpin->SetRelativeLocation(FVector(0.f, 0.f, Bottom));
	if (PreviewPlinth)
	{
		FTVis::ApplyShape(PreviewPlinth, EFTShape::Cylinder, FVector(110.f, 110.f, Bottom));
		PreviewPlinth->SetRelativeLocation(FVector(0.f, 0.f, Bottom * 0.5f));
	}
	FTShop::BuildParts(this, PreviewSpin, *Def, PreviewParts);
	for (UStaticMeshComponent* P : PreviewParts)
	{
		P->SetRelativeLocation(P->GetRelativeLocation() - FVector(B.GetCenter().X, B.GetCenter().Y, B.Min.Z));
		P->SetVisibleInSceneCaptureOnly(true);
		PreviewCapture->ShowOnlyComponent(P);
	}
}

void AFTShopTerminal::EndPreview()
{
	PreviewUsers = FMath::Max(0, PreviewUsers - 1);
	if (PreviewUsers == 0)
	{
		PreviewCapture->bCaptureEveryFrame = false;
		PreviewKey->SetVisibility(false);
		PreviewFill->SetVisibility(false);
		DestroyParts(PreviewParts);
		PreviewItem = NAME_None;
	}
}

void AFTShopTerminal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	const float Since = Now - CelebrateTime;
	const float Speed = Since < 2.f ? 360.f : 40.f;
	Turntable->AddLocalRotation(FRotator(0.f, Speed * DeltaSeconds, 0.f));
	FTVis::SetGlow(RegisterDisplay, Since < 2.f ? 3.f + 5.f * FMath::Abs(FMath::Sin(Since * 12.f)) : 3.f);
	if (PreviewUsers > 0)
	{
		PreviewSpin->AddLocalRotation(FRotator(0.f, 30.f * DeltaSeconds, 0.f));
	}
}

// ============================================================================ accessory wall

namespace
{
	constexpr float HookSpacing = 70.f;
	FVector HookBase(int32 Hook, int32 Count = AFTAccessoryStand::MaxHooks) { return FVector(0.f, (Hook - (Count - 1) * 0.5f) * HookSpacing, 0.f); }
}

AFTAccessoryStand::AFTAccessoryStand()
{
	auto Part = [this](const FString& Name, EFTShape Shape, FVector Loc, FVector Size, FLinearColor C, FRotator R = FRotator::ZeroRotator, float Glow = 0.f, bool bSolid = false)
	{
		return FTVis::MakePart(this, Root, *Name, Shape, Loc, Size, C, R, Glow, bSolid);
	};
	const float W = MaxHooks * HookSpacing + 30.f;
	Part(TEXT("Back"), EFTShape::Box, FVector(-14.f, 0.f, 160.f), FVector(12.f, W, 320.f), Hex(0x2B2F5E), FRotator::ZeroRotator, 0.f, true);
	Part(TEXT("FrameTop"), EFTShape::Box, FVector(-8.f, 0.f, 322.f), FVector(20.f, W + 16.f, 12.f), Cream);
	Part(TEXT("FrameL"), EFTShape::Box, FVector(-8.f, -W * 0.5f - 4.f, 160.f), FVector(20.f, 10.f, 326.f), Cream);
	Part(TEXT("FrameR"), EFTShape::Box, FVector(-8.f, W * 0.5f + 4.f, 160.f), FVector(20.f, 10.f, 326.f), Cream);
	// counter shelf the busts stand on
	Part(TEXT("Shelf"), EFTShape::Box, FVector(12.f, 0.f, 104.f), FVector(48.f, W, 10.f), Wood);
	Part(TEXT("ShelfBody"), EFTShape::Box, FVector(8.f, 0.f, 50.f), FVector(40.f, W - 10.f, 100.f), Magenta, FRotator::ZeroRotator, 0.f, true);
	Part(TEXT("ShelfTrim"), EFTShape::Box, FVector(29.f, 0.f, 92.f), FVector(2.f, W - 14.f, 4.f), Yellow, FRotator::ZeroRotator, 0.8f);
	// makeup mirror with bulbs above the busts
	Part(TEXT("Mirror"), EFTShape::Box, FVector(-6.f, 0.f, 262.f), FVector(4.f, 300.f, 84.f), SkyBlue, FRotator::ZeroRotator, 0.35f);
	Part(TEXT("MirrorFrame"), EFTShape::Box, FVector(-7.f, 0.f, 262.f), FVector(4.f, 316.f, 98.f), Cream);
	for (int32 i = 0; i < 9; ++i)
	{
		Part(FString::Printf(TEXT("Bulb%d"), i), EFTShape::Sphere, FVector(0.f, -144.f + i * 36.f, 308.f), FVector(12.f), Cream, FRotator::ZeroRotator, 10.f);
	}
	FTVis::MakeText(this, Root, TEXT("Title"), LOCTEXT("AccTitle", "ACCESSORIES"), FVector(-3.f, 0.f, 262.f), FRotator::ZeroRotator, 36.f, FColor(255, 214, 90));
	FTVis::MakeText(this, Root, TEXT("Sub"), LOCTEXT("AccSub", "E: wear / take off  -  carry the stand-in here to dress it"), FVector(-3.f, 0.f, 232.f), FRotator::ZeroRotator, 10.f, FColor(255, 240, 220));

	for (int32 h = 0; h < MaxHooks; ++h)
	{
		const FVector B = HookBase(h);
		USceneComponent* Anchor = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("HookAnchor%d"), h));
		Anchor->SetupAttachment(Root);
		Anchor->SetRelativeLocation(B + FVector(12.f, 0.f, 162.f));
		HookAnchors.Add(Anchor);
		HookBusts.Add(Part(FString::Printf(TEXT("Bust%d"), h), EFTShape::Ball, B + FVector(12.f, 0.f, 144.f), FVector(38.f, 34.f, 38.f), Hex(0xD9D2C5)));
		HookNecks.Add(Part(FString::Printf(TEXT("BustNeck%d"), h), EFTShape::Cylinder, B + FVector(12.f, 0.f, 117.f), FVector(10.f, 10.f, 18.f), Hex(0xBDB5A8)));
		HookLabels.Add(FTVis::MakeText(this, Root, *FString::Printf(TEXT("HookLabel%d"), h), FText::GetEmpty(), B + FVector(29.5f, 0.f, 82.f), FRotator::ZeroRotator, 6.f, FColor(255, 240, 220)));
		HookPrices.Add(FTVis::MakeText(this, Root, *FString::Printf(TEXT("HookPrice%d"), h), FText::GetEmpty(), B + FVector(29.5f, 0.f, 72.f), FRotator::ZeroRotator, 5.5f, FColor(255, 214, 90)));
		UFTInteractableComponent* I = CreateDefaultSubobject<UFTInteractableComponent>(*FString::Printf(TEXT("Hook%d"), h));
		I->SetupAttachment(Root);
		I->Setup(*FString::Printf(TEXT("Hook%d"), h), FText::GetEmpty(), LOCTEXT("WearAcc", "Wear"), EFTInteractType::Press, FVector(26.f, 26.f, 44.f));
		I->SetRelativeLocation(B + FVector(20.f, 0.f, 150.f));
		Hooks.Add(I);
	}
}

void AFTAccessoryStand::LayoutHooks()
{
	const int32 Count = FMath::Max(1, HookItems.Num());
	for (int32 h = 0; h < MaxHooks; ++h)
	{
		const bool bUsed = h < HookItems.Num();
		const FVector B = HookBase(h, Count);
		HookBusts[h]->SetRelativeLocation(B + FVector(12.f, 0.f, 144.f));
		HookNecks[h]->SetRelativeLocation(B + FVector(12.f, 0.f, 117.f));
		HookLabels[h]->SetRelativeLocation(B + FVector(29.5f, 0.f, 82.f));
		HookPrices[h]->SetRelativeLocation(B + FVector(29.5f, 0.f, 72.f));
		HookAnchors[h]->SetRelativeLocation(B + FVector(12.f, 0.f, 159.f));
		Hooks[h]->SetRelativeLocation(B + FVector(20.f, 0.f, 150.f));
		HookBusts[h]->SetVisibility(bUsed);
		HookNecks[h]->SetVisibility(bUsed);
		Hooks[h]->SetInteractionEnabled(bUsed);
		// only the bust and what hangs on it light up when targeted
		Hooks[h]->HighlightTargets.Reset();
		Hooks[h]->AddHighlight(HookBusts[h]);
	}
}

void AFTAccessoryStand::BeginPlay()
{
	Super::BeginPlay();
	HookItems.Reset();
	for (const FFTShopItemDef& D : UFTEconomyConfig::Get()->Items)
	{
		if (D.Category == EFTShopCategory::Costume && HookItems.Num() < MaxHooks)
		{
			HookItems.Add(D.ItemId);
		}
	}
	LayoutHooks();
	if (AFTGameState* G = GetWorld()->GetGameState<AFTGameState>())
	{
		CareerHandle = G->OnCareerChanged.AddUObject(this, &AFTAccessoryStand::RefreshDisplays);
	}
	ShownOwned.Reset();
	RefreshDisplays();
}

void AFTAccessoryStand::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AFTGameState* G = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr)
	{
		G->OnCareerChanged.Remove(CareerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

FName AFTAccessoryStand::ItemForHook(int32 Hook) const
{
	return HookItems.IsValidIndex(Hook) ? HookItems[Hook] : NAME_None;
}

void AFTAccessoryStand::RefreshDisplays()
{
	const AFTGameState* G = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
	TArray<FName> Owned;
	for (const FName Id : HookItems)
	{
		if (G && G->IsOwned(Id))
		{
			Owned.Add(Id);
		}
	}
	// server: nobody keeps wearing something the career no longer owns (career reset)
	if (HasAuthority() && G)
	{
		for (TActorIterator<AFTCharacter> It(GetWorld()); It; ++It)
		{
			for (int32 S = 0; S < FFTWornAccessories::NumSlots; ++S)
			{
				const FName Id = It->GetAccessory((EFTAccessorySlot)S);
				if (!Id.IsNone() && !G->IsOwned(Id))
				{
					It->SetAccessory((EFTAccessorySlot)S, NAME_None);
				}
			}
		}
		for (TActorIterator<AFTStandIn> It(GetWorld()); It; ++It)
		{
			for (int32 S = 0; S < FFTWornAccessories::NumSlots; ++S)
			{
				const FName Id = It->GetAccessory((EFTAccessorySlot)S);
				if (!Id.IsNone() && !G->IsOwned(Id))
				{
					It->SetAccessory((EFTAccessorySlot)S, NAME_None);
				}
			}
		}
	}
	if (Owned == ShownOwned && DisplayParts.Num() > 0)
	{
		return;
	}
	ShownOwned = Owned;
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	DestroyParts(DisplayParts);
	const UFTEconomyConfig* Cfg = UFTEconomyConfig::Get();
	for (int32 h = 0; h < MaxHooks; ++h)
	{
		const FFTShopItemDef* Def = Cfg->FindItem(ItemForHook(h));
		const bool bOwned = Def && Owned.Contains(Def->ItemId);
		Hooks[h]->HighlightTargets.Reset();
		Hooks[h]->AddHighlight(HookBusts[h]);
		if (!Def)
		{
			HookLabels[h]->SetText(FText::GetEmpty());
			HookPrices[h]->SetText(FText::GetEmpty());
			continue;
		}
		// the accessory sits where it would sit on a head (head top / eye line / shoulders)
		USceneComponent* Anchor = HookAnchors[h];
		const FVector B = HookBase(h, FMath::Max(1, HookItems.Num()));
		switch (Def->AccessorySlot)
		{
		case EFTAccessorySlot::Face: Anchor->SetRelativeLocation(B + FVector(29.f, 0.f, 147.f)); break;
		case EFTAccessorySlot::Body: Anchor->SetRelativeLocation(B + FVector(12.f, 0.f, 150.f)); break;
		default: Anchor->SetRelativeLocation(B + FVector(12.f, 0.f, 159.f)); break;
		}
		TArray<TObjectPtr<UStaticMeshComponent>> Spawned;
		FTShop::BuildParts(this, Anchor, *Def, Spawned);
		for (UStaticMeshComponent* P : Spawned)
		{
			if (Def->AccessorySlot == EFTAccessorySlot::Body)
			{
				// capes are shown folded small on the bust
				P->SetRelativeScale3D(P->GetRelativeScale3D() * 0.7f);
				P->SetRelativeLocation(P->GetRelativeLocation() * 0.7f);
			}
			if (!bOwned)
			{
				// not bought yet: a dark "sold at the counter" silhouette
				FTVis::Paint(P, Hex(0x3A3F55), 0.f);
			}
			Hooks[h]->AddHighlight(P);
			DisplayParts.Add(P);
		}
		HookLabels[h]->SetText(Def->Name);
		HookLabels[h]->SetTextRenderColor(bOwned ? FColor(255, 240, 220) : FColor(170, 170, 190));
		HookPrices[h]->SetText(bOwned ? LOCTEXT("HookOwned", "OWNED - PRESS E") : FText::Format(LOCTEXT("HookPrice", "{0} AT THE COUNTER"), FText::FromString(FFTEconomy::MoneyString(Def->Price))));
		HookPrices[h]->SetTextRenderColor(bOwned ? FColor(120, 230, 200) : FColor(255, 214, 90));
	}
}

bool AFTAccessoryStand::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	const int32 Hook = Hooks.IndexOfByKey(Comp);
	const FFTShopItemDef* Def = UFTEconomyConfig::Get()->FindItem(ItemForHook(Hook));
	const AFTGameState* G = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
	if (!Def)
	{
		return false;
	}
	if (!G || !G->IsOwned(Def->ItemId))
	{
		OutReason = FText::Format(LOCTEXT("NotOwnedAcc", "Buy the {0} at the Studio Supply counter ({1})"), Def->Name, FText::FromString(FFTEconomy::MoneyString(Def->Price)));
		return false;
	}
	return true;
}

FText AFTAccessoryStand::GetPromptLabel(const UFTInteractableComponent* Comp) const
{
	const FFTShopItemDef* Def = UFTEconomyConfig::Get()->FindItem(ItemForHook(Hooks.IndexOfByKey(Comp)));
	return Def ? Def->Name : FText::GetEmpty();
}

FText AFTAccessoryStand::GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const
{
	const FFTShopItemDef* Def = UFTEconomyConfig::Get()->FindItem(ItemForHook(Hooks.IndexOfByKey(Comp)));
	if (!Def || !User)
	{
		return LOCTEXT("WearAcc2", "Wear");
	}
	if (const AFTStandIn* S = Cast<AFTStandIn>(User->HeldProp))
	{
		return S->GetAccessory(Def->AccessorySlot) == Def->ItemId ? LOCTEXT("UndressStandIn", "Take off the stand-in's") : LOCTEXT("DressStandIn", "Dress the stand-in with");
	}
	return User->GetAccessory(Def->AccessorySlot) == Def->ItemId ? LOCTEXT("TakeOffAcc", "Take off") : LOCTEXT("WearAcc3", "Wear");
}

void AFTAccessoryStand::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	const FFTShopItemDef* Def = UFTEconomyConfig::Get()->FindItem(ItemForHook(Hooks.IndexOfByKey(Comp)));
	if (!Def || !User)
	{
		return;
	}
	if (AFTStandIn* S = Cast<AFTStandIn>(User->HeldProp))
	{
		S->SetAccessory(Def->AccessorySlot, S->GetAccessory(Def->AccessorySlot) == Def->ItemId ? NAME_None : Def->ItemId);
	}
	else
	{
		User->SetAccessory(Def->AccessorySlot, User->GetAccessory(Def->AccessorySlot) == Def->ItemId ? NAME_None : Def->ItemId);
	}
	MulticastSound(EFTSound::Rustle, Comp->GetComponentLocation(), 1.f, 1.15f);
}

#undef LOCTEXT_NAMESPACE
