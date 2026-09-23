#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "TheFinalTake/Core/FTTypes.h"
#include "FTInteractableComponent.generated.h"

class AFTStudioActor;
class UMeshComponent;

/** Trace channel used for interaction (configured as "Interact" in DefaultEngine.ini). */
#define ECC_FTInteract ECC_GameTraceChannel2

/**
 * A single usable spot on a studio actor (a switch, a handle, a costume hanger...).
 * The box is only hit by the interaction trace; the owning AFTStudioActor decides
 * availability and runs the authoritative logic on the server.
 */
UCLASS(ClassGroup = (FinalTake), meta = (BlueprintSpawnableComponent))
class THE_FINAL_TAKE_API UFTInteractableComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UFTInteractableComponent();

	/** Identifies which action on the owner this is (e.g. "Toggle", "Siren", "HandleBow"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction") FName ActionId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction") FText Label;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction") FText Verb;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction") EFTInteractType Type = EFTInteractType::Press;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction") float HoldTime = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction") float MaxDistance = 320.f;

	/** Meshes that get the highlight overlay while targeted. Empty = all meshes of the owner. */
	UPROPERTY(Transient) TArray<TObjectPtr<UMeshComponent>> HighlightTargets;

	void Setup(FName InActionId, const FText& InLabel, const FText& InVerb, EFTInteractType InType, const FVector& Extent);
	void AddHighlight(UMeshComponent* Mesh) { HighlightTargets.AddUnique(Mesh); }

	AFTStudioActor* GetStudioOwner() const;

	/** Local only: toggles the overlay highlight. */
	void SetHighlighted(bool bOn);
	bool IsHighlighted() const { return bHighlighted; }

	void SetInteractionEnabled(bool bEnabled);

private:
	bool bHighlighted = false;
};
