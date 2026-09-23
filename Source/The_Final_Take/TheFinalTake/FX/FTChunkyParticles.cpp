#include "TheFinalTake/FX/FTChunkyParticles.h"

#include "TheFinalTake/Core/FTVisuals.h"

UFTChunkyParticles::UFTChunkyParticles()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCanEverAffectNavigation(false);
	SetGenerateOverlapEvents(false);
	CastShadow = false;
	bUseAsOccluder = false;
	SetMobility(EComponentMobility::Movable);
}

void UFTChunkyParticles::BeginPlay()
{
	Super::BeginPlay();
	ClearInstances();
	Particles.Reset();
	bDirty = true;
}

void UFTChunkyParticles::Configure(EFTShape InShape, const FLinearColor& Color, float Emissive, bool bTranslucent, bool bGlow)
{
	SetStaticMesh(FTVis::GetMesh(InShape));
	SetMaterial(0, bGlow ? FTVis::Glow() : (bTranslucent ? FTVis::Translucent() : FTVis::Matte()));
	FTVis::Paint(this, Color, Emissive, true, 0.f, Color.A);
}

void UFTChunkyParticles::SetEmitting(bool bOn)
{
	bEmitting = bOn;
	if (bOn)
	{
		SetComponentTickEnabled(true);
	}
}

void UFTChunkyParticles::ClearParticles()
{
	Particles.Reset();
	bDirty = true;
	SyncInstances();
}

void UFTChunkyParticles::Spawn(const FVector& LocalPos, float SpeedScale)
{
	if (Particles.Num() >= MaxParticles)
	{
		// recycle the oldest
		Particles.RemoveAt(0, 1, EAllowShrinking::No);
	}
	FParticle P;
	P.Pos = LocalPos;
	const FVector Jitter(FMath::FRandRange(-1.f, 1.f) * VelocityJitter.X, FMath::FRandRange(-1.f, 1.f) * VelocityJitter.Y, FMath::FRandRange(-1.f, 1.f) * VelocityJitter.Z);
	P.Vel = (BaseVelocity + Jitter) * SpeedScale;
	P.Life = FMath::FRandRange(Lifetime.X, Lifetime.Y);
	P.Size = FMath::FRandRange(StartSize.X, StartSize.Y);
	P.Spin = bSpin ? FRotator(FMath::FRandRange(0.f, 360.f), FMath::FRandRange(0.f, 360.f), 0.f) : FRotator::ZeroRotator;
	P.SpinRate = bSpin ? FRotator(FMath::FRandRange(-90.f, 90.f), FMath::FRandRange(-90.f, 90.f), 0.f) : FRotator::ZeroRotator;
	Particles.Add(P);
	bDirty = true;
}

void UFTChunkyParticles::Burst(int32 Count, const FVector& LocalOffset, float SpeedScale)
{
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector R(FMath::FRandRange(-SpawnExtent.X, SpawnExtent.X), FMath::FRandRange(-SpawnExtent.Y, SpawnExtent.Y), FMath::FRandRange(-SpawnExtent.Z, SpawnExtent.Z));
		Spawn(LocalOffset + R, SpeedScale);
	}
	SetComponentTickEnabled(true);
}

void UFTChunkyParticles::BurstAt(const FVector& WorldLocation, int32 Count, float SpeedScale)
{
	Burst(Count, GetComponentTransform().InverseTransformPosition(WorldLocation), SpeedScale);
}

void UFTChunkyParticles::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	DeltaTime = FMath::Min(DeltaTime, 0.05f);

	if (bEmitting && SpawnRate > 0.f)
	{
		SpawnAccumulator += SpawnRate * DeltaTime;
		while (SpawnAccumulator >= 1.f)
		{
			SpawnAccumulator -= 1.f;
			const FVector R(FMath::FRandRange(-SpawnExtent.X, SpawnExtent.X), FMath::FRandRange(-SpawnExtent.Y, SpawnExtent.Y), FMath::FRandRange(-SpawnExtent.Z, SpawnExtent.Z));
			Spawn(R, 1.f);
		}
	}

	if (Particles.Num() == 0)
	{
		if (bDirty)
		{
			SyncInstances();
		}
		if (!bEmitting)
		{
			SetComponentTickEnabled(false);
		}
		return;
	}

	const FTransform& CT = GetComponentTransform();
	const FVector LocalWind = CT.InverseTransformVectorNoScale(Wind);
	const FVector LocalGravity = CT.InverseTransformVectorNoScale(FVector(0.f, 0.f, -Gravity));
	for (int32 i = Particles.Num() - 1; i >= 0; --i)
	{
		FParticle& P = Particles[i];
		P.Age += DeltaTime;
		if (P.Age >= P.Life)
		{
			Particles.RemoveAtSwap(i, 1, EAllowShrinking::No);
			continue;
		}
		if (!P.bResting)
		{
			P.Vel += (LocalGravity + LocalWind) * DeltaTime;
			if (Drag > 0.f)
			{
				P.Vel *= FMath::Max(0.f, 1.f - Drag * DeltaTime);
			}
			P.Pos += P.Vel * DeltaTime;
			P.Spin += P.SpinRate * DeltaTime;
			if (!BoundsExtent.IsNearlyZero())
			{
				P.Pos.X = FMath::Clamp(P.Pos.X, -BoundsExtent.X, BoundsExtent.X);
				P.Pos.Y = FMath::Clamp(P.Pos.Y, -BoundsExtent.Y, BoundsExtent.Y);
			}
			const float WorldZ = CT.TransformPosition(P.Pos).Z;
			if (WorldZ < FloorZ)
			{
				if (bRestOnFloor)
				{
					P.bResting = true;
					P.Vel = FVector::ZeroVector;
					FVector W = CT.TransformPosition(P.Pos);
					W.Z = FloorZ;
					P.Pos = CT.InverseTransformPosition(W);
				}
				else
				{
					Particles.RemoveAtSwap(i, 1, EAllowShrinking::No);
					continue;
				}
			}
		}
	}
	bDirty = true;
	SyncInstances();
}

void UFTChunkyParticles::SyncInstances()
{
	if (!bDirty)
	{
		return;
	}
	bDirty = false;
	const int32 Needed = FMath::Max(Particles.Num(), 0);
	const int32 Have = GetInstanceCount();
	if (Have < Needed)
	{
		for (int32 i = Have; i < Needed; ++i)
		{
			AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector), false);
		}
	}
	Scratch.SetNum(GetInstanceCount(), EAllowShrinking::No);
	const FVector ScaleToMesh(0.01f);
	for (int32 i = 0; i < Scratch.Num(); ++i)
	{
		if (i < Particles.Num())
		{
			const FParticle& P = Particles[i];
			const float T = P.Age / FMath::Max(P.Life, 0.01f);
			float S = P.Size * FMath::Lerp(1.f, EndSizeScale, T);
			if (GrowInTime > 0.f)
			{
				S *= FMath::Clamp(P.Age / GrowInTime, 0.f, 1.f);
			}
			// shrink out over the last 15% of life
			S *= FMath::Clamp((1.f - T) / 0.15f, 0.f, 1.f);
			FVector Scale3(S);
			FQuat Rot = P.Spin.Quaternion();
			if (Stretch > 0.f && !P.Vel.IsNearlyZero())
			{
				Rot = FRotationMatrix::MakeFromZ(P.Vel.GetSafeNormal()).ToQuat();
				Scale3 = FVector(S, S, S + P.Vel.Size() * Stretch);
			}
			Scratch[i] = FTransform(Rot, P.Pos, Scale3 * ScaleToMesh);
		}
		else
		{
			Scratch[i] = FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector);
		}
	}
	if (Scratch.Num() > 0)
	{
		BatchUpdateInstancesTransforms(0, Scratch, false, true, true);
	}
}
