#include "TheFinalTake/Interaction/FTInteractableComponent.h"

#include "TheFinalTake/Interaction/FTStudioActor.h"
#include "TheFinalTake/Core/FTVisuals.h"
#include "Components/MeshComponent.h"

UFTInteractableComponent::UFTInteractableComponent()
{
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_FTInteract, ECR_Block);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	SetHiddenInGame(true);
	ShapeColor = FColor(255, 200, 60);
	InitBoxExtent(FVector(20.f));
	Verb = NSLOCTEXT("FinalTake", "Use", "Use");
}

void UFTInteractableComponent::Setup(FName InActionId, const FText& InLabel, const FText& InVerb, EFTInteractType InType, const FVector& Extent)
{
	ActionId = InActionId;
	Label = InLabel;
	Verb = InVerb;
	Type = InType;
	InitBoxExtent(Extent);
}

AFTStudioActor* UFTInteractableComponent::GetStudioOwner() const
{
	return Cast<AFTStudioActor>(GetOwner());
}

void UFTInteractableComponent::SetHighlighted(bool bOn)
{
	if (bHighlighted == bOn)
	{
		return;
	}
	bHighlighted = bOn;
	UMaterialInterface* Mat = bOn ? FTVis::Highlight() : nullptr;
	TArray<UMeshComponent*> Targets;
	for (UMeshComponent* M : HighlightTargets)
	{
		if (M)
		{
			Targets.Add(M);
		}
	}
	if (Targets.Num() == 0 && GetOwner())
	{
		GetOwner()->GetComponents<UMeshComponent>(Targets);
	}
	for (UMeshComponent* M : Targets)
	{
		if (M && M->IsVisible())
		{
			M->SetOverlayMaterial(Mat);
		}
	}
}

void UFTInteractableComponent::SetInteractionEnabled(bool bEnabled)
{
	SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	if (!bEnabled)
	{
		SetHighlighted(false);
	}
}
