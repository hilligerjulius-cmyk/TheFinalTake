#pragma once

#include "CoreMinimal.h"
#include "TheFinalTake/Interaction/FTStudioActor.h"
#include "FTFloodController.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UAudioComponent;
class UFTChunkyParticles;

/**
 * Staged flood: a stylised water plane that rises in controlled, replicated stages
 * (Dry -> Leaking -> Flooded). Water height is derived from the replicated stage and
 * its server start time, so every machine animates it smoothly without streaming.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTFloodController : public AFTStudioActor
{
	GENERATED_BODY()

public:
	AFTFloodController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnFloodStageChanged(EFTFloodStage NewStage) override;
	virtual void ResetForNewShoot() override;

	static AFTFloodController* Get(const UObject* WorldContext);

	/** Current water surface height (world Z). */
	float GetWaterZ() const;
	bool IsInFloodArea(const FVector& Location) const;
	/** Water depth at a location (0 when dry or outside). */
	float GetDepthAt(const FVector& FeetLocation) const;

	/** Server: advance the flood. */
	void SetStage(EFTFloodStage Stage);

	UPROPERTY(EditAnywhere, Category = "Flood") float FloorZ = -120.f;
	UPROPERTY(EditAnywhere, Category = "Flood") float LeakDepth = 22.f;
	UPROPERTY(EditAnywhere, Category = "Flood") float FloodDepth = 58.f;
	UPROPERTY(EditAnywhere, Category = "Flood") float LeakRiseTime = 18.f;
	UPROPERTY(EditAnywhere, Category = "Flood") float FloodRiseTime = 14.f;
	/** Where the tank valve bursts (relative). */
	UPROPERTY(EditAnywhere, Category = "Flood", meta = (MakeEditWidget = true)) FVector ValveLocation = FVector(0.f, 0.f, 0.f);

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> FloodArea;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> WaterPlane;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFTChunkyParticles> ValveSpray;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ValvePipe;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ValveWheel;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> LeakAudio;

	float VisualZ = 0.f;
};
