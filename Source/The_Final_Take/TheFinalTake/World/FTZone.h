#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FTZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM()
enum class EFTZoneMark : uint8
{
	None,
	TapeX,		// actor mark on the floor
	Ring,		// circular mark
	Outline		// taped rectangle
};

/**
 * Tagged volume used by objectives (beach, tank, hero mark), shark safe zones and
 * fall-recovery volumes. Optional painted floor mark keeps marks readable in-world.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTZone : public AActor
{
	GENERATED_BODY()

public:
	AFTZone();
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone") FName ZoneTag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone") FVector Extent = FVector(200.f, 200.f, 150.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone") EFTZoneMark Mark = EFTZoneMark::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone") FLinearColor MarkColor = FLinearColor::Yellow;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone") FText MarkLabel;

	bool Contains(const FVector& Location) const;

	static bool IsInZone(const UObject* WorldContext, FName Tag, const FVector& Location);
	static AFTZone* Find(const UObject* WorldContext, FName Tag);

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Volume;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> MarkA;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> MarkB;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> MarkC;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> MarkD;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
};
