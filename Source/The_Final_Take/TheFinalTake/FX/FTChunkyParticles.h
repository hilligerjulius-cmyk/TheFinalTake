#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "TheFinalTake/Core/FTTypes.h"
#include "FTChunkyParticles.generated.h"

/**
 * Tiny instanced-mesh particle system for the chunky low-poly look (rain streaks,
 * smoke puffs, foam blobs, splashes, valve spray). Purely cosmetic and local:
 * gameplay only replicates the emitter's on/off state or a burst event.
 */
UCLASS(ClassGroup = (FinalTake), meta = (BlueprintSpawnableComponent))
class THE_FINAL_TAKE_API UFTChunkyParticles : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	UFTChunkyParticles();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Configure(EFTShape InShape, const FLinearColor& Color, float Emissive, bool bTranslucent, bool bGlow = false);
	void SetEmitting(bool bOn);
	bool IsEmitting() const { return bEmitting; }
	/** Spawn a burst at the emitter (component space offset). */
	void Burst(int32 Count, const FVector& LocalOffset = FVector::ZeroVector, float SpeedScale = 1.f);
	/** Spawn a burst at a world location. */
	void BurstAt(const FVector& WorldLocation, int32 Count, float SpeedScale = 1.f);
	void ClearParticles();

	UPROPERTY(EditAnywhere, Category = "Particles") int32 MaxParticles = 120;
	UPROPERTY(EditAnywhere, Category = "Particles") float SpawnRate = 40.f;
	UPROPERTY(EditAnywhere, Category = "Particles") FVector2D Lifetime = FVector2D(1.f, 1.5f);
	/** Half extents of the local spawn box. */
	UPROPERTY(EditAnywhere, Category = "Particles") FVector SpawnExtent = FVector(20.f);
	UPROPERTY(EditAnywhere, Category = "Particles") FVector BaseVelocity = FVector(0.f, 0.f, 100.f);
	UPROPERTY(EditAnywhere, Category = "Particles") FVector VelocityJitter = FVector(20.f);
	UPROPERTY(EditAnywhere, Category = "Particles") float Gravity = 0.f;
	UPROPERTY(EditAnywhere, Category = "Particles") float Drag = 0.f;
	UPROPERTY(EditAnywhere, Category = "Particles") FVector2D StartSize = FVector2D(10.f, 16.f);
	UPROPERTY(EditAnywhere, Category = "Particles") float EndSizeScale = 1.f;
	/** Grow in quickly at birth instead of popping. */
	UPROPERTY(EditAnywhere, Category = "Particles") float GrowInTime = 0.08f;
	/** Stretch along velocity (rain streaks). */
	UPROPERTY(EditAnywhere, Category = "Particles") float Stretch = 0.f;
	UPROPERTY(EditAnywhere, Category = "Particles") bool bSpin = true;
	/** World-space wind added to every particle each second. */
	UPROPERTY(EditAnywhere, Category = "Particles") FVector Wind = FVector::ZeroVector;
	/** Particles die (or rest, see bRestOnFloor) when they fall below this world Z. */
	UPROPERTY(EditAnywhere, Category = "Particles") float FloorZ = -100000.f;
	UPROPERTY(EditAnywhere, Category = "Particles") bool bRestOnFloor = false;
	/** Keep particles inside the spawn box (bounded effect zones). */
	UPROPERTY(EditAnywhere, Category = "Particles") FVector BoundsExtent = FVector::ZeroVector;

private:
	struct FParticle
	{
		FVector Pos;
		FVector Vel;
		float Age = 0.f;
		float Life = 1.f;
		float Size = 10.f;
		FRotator Spin;
		FRotator SpinRate;
		bool bResting = false;
	};

	void Spawn(const FVector& LocalPos, float SpeedScale);
	void SyncInstances();

	TArray<FParticle> Particles;
	TArray<FTransform> Scratch;
	float SpawnAccumulator = 0.f;
	bool bEmitting = false;
	bool bDirty = true;
};
