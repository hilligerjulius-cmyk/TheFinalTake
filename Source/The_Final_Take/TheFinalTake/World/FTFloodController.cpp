#include "TheFinalTake/World/FTFloodController.h"

#include "TheFinalTake/Core/FTVisuals.h"
#include "TheFinalTake/FX/FTChunkyParticles.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"

AFTFloodController::AFTFloodController()
{
	PrimaryActorTick.bCanEverTick = true;

	FloodArea = CreateDefaultSubobject<UBoxComponent>(TEXT("FloodArea"));
	FloodArea->SetupAttachment(Root);
	FloodArea->SetBoxExtent(FVector(1500.f, 1500.f, 150.f));
	FloodArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FloodArea->SetHiddenInGame(true);
	FloodArea->ShapeColor = FColor(40, 140, 255);

	WaterPlane = FTVis::MakePart(this, Root, TEXT("WaterPlane"), EFTShape::WaterGrid, FVector::ZeroVector, FVector(3000.f, 3000.f, 100.f), FTColors::Hex(0x1E8FE0));
	WaterPlane->SetCastShadow(false);
	WaterPlane->SetBoundsScale(3.f);
	WaterPlane->SetVisibility(false);

	ValvePipe = FTVis::MakePart(this, Root, TEXT("ValvePipe"), EFTShape::Cylinder, FVector::ZeroVector, FVector(26.f, 26.f, 80.f), FTColors::GreyDark, FRotator(90.f, 0.f, 0.f));
	ValveWheel = FTVis::MakePart(this, Root, TEXT("ValveWheel"), EFTShape::Torus, FVector::ZeroVector, FVector(44.f, 44.f, 8.f), FTColors::Red, FRotator(90.f, 0.f, 0.f));

	ValveSpray = CreateDefaultSubobject<UFTChunkyParticles>(TEXT("ValveSpray"));
	ValveSpray->SetupAttachment(Root);
	ValveSpray->MaxParticles = 160;
	ValveSpray->SpawnRate = 70.f;
	ValveSpray->Lifetime = FVector2D(0.7f, 1.2f);
	ValveSpray->SpawnExtent = FVector(6.f);
	ValveSpray->BaseVelocity = FVector(420.f, 0.f, 160.f);
	ValveSpray->VelocityJitter = FVector(90.f, 120.f, 90.f);
	ValveSpray->Gravity = 980.f;
	ValveSpray->StartSize = FVector2D(8.f, 16.f);
	ValveSpray->EndSizeScale = 1.6f;
}

AFTFloodController* AFTFloodController::Get(const UObject* WorldContext)
{
	static TWeakObjectPtr<AFTFloodController> Cached;
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	if (Cached.IsValid() && Cached->GetWorld() == World)
	{
		return Cached.Get();
	}
	for (TActorIterator<AFTFloodController> It(World); It; ++It)
	{
		Cached = *It;
		return *It;
	}
	return nullptr;
}

void AFTFloodController::BeginPlay()
{
	Super::BeginPlay();
	ValveSpray->Configure(EFTShape::Sphere, FTColors::Hex(0x9FE3FF), 0.4f, false);
	ValvePipe->SetRelativeLocation(ValveLocation);
	ValveWheel->SetRelativeLocation(ValveLocation + FVector(30.f, 0.f, 0.f));
	ValveSpray->SetRelativeLocation(ValveLocation + FVector(40.f, 0.f, 0.f));
	const FVector Ext = FloodArea->GetScaledBoxExtent();
	WaterPlane->SetWorldScale3D(FVector(Ext.X * 2.f / 100.f, Ext.Y * 2.f / 100.f, 1.f));
	VisualZ = FloorZ - 40.f;
	OnFloodStageChanged(EFTFloodStage::Dry);
}

float AFTFloodController::GetWaterZ() const
{
	const AFTGameState* GS = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
	if (!GS)
	{
		return FloorZ - 40.f;
	}
	const float T = FMath::Max(0.f, GS->GetServerWorldTimeSeconds() - GS->FloodStageTime);
	switch (GS->FloodStage)
	{
	case EFTFloodStage::Leaking:
		return FMath::Lerp(FloorZ - 4.f, FloorZ + LeakDepth, FMath::SmoothStep(0.f, 1.f, T / LeakRiseTime));
	case EFTFloodStage::Flooded:
		return FMath::Lerp(FloorZ + LeakDepth, FloorZ + FloodDepth, FMath::SmoothStep(0.f, 1.f, T / FloodRiseTime));
	default:
		return FloorZ - 40.f;
	}
}

bool AFTFloodController::IsInFloodArea(const FVector& Location) const
{
	const FVector Local = FloodArea->GetComponentTransform().InverseTransformPositionNoScale(Location);
	const FVector Ext = FloodArea->GetScaledBoxExtent();
	return FMath::Abs(Local.X) <= Ext.X && FMath::Abs(Local.Y) <= Ext.Y && Location.Z < FloorZ + 260.f;
}

float AFTFloodController::GetDepthAt(const FVector& FeetLocation) const
{
	if (!IsInFloodArea(FeetLocation))
	{
		return 0.f;
	}
	return FMath::Max(0.f, GetWaterZ() - FeetLocation.Z);
}

void AFTFloodController::SetStage(EFTFloodStage Stage)
{
	if (!HasAuthority())
	{
		return;
	}
	if (AFTGameState* GS = GetFTGameState())
	{
		GS->FloodStage = Stage;
		GS->FloodStageTime = GS->GetServerWorldTimeSeconds();
		GS->NotifyFloodChanged();
	}
}

void AFTFloodController::OnFloodStageChanged(EFTFloodStage NewStage)
{
	const bool bActive = NewStage != EFTFloodStage::Dry;
	ValveSpray->SetEmitting(bActive);
	ValveSpray->SpawnRate = NewStage == EFTFloodStage::Flooded ? 110.f : 70.f;
	if (bActive)
	{
		ValveSpray->Burst(40, FVector::ZeroVector, 1.4f);
		FTAudio::PlayAt(this, EFTSound::Splash, ValveSpray->GetComponentLocation(), 1.f, 0.7f);
		if (!LeakAudio && GetNetMode() != NM_DedicatedServer)
		{
			LeakAudio = FTAudio::Attach(ValveSpray, EFTSound::LeakLoop, 0.7f);
		}
		ValveWheel->SetRelativeRotation(FRotator(90.f, 0.f, 35.f));
	}
	else
	{
		if (LeakAudio)
		{
			LeakAudio->Stop();
			LeakAudio->DestroyComponent();
			LeakAudio = nullptr;
		}
		ValveSpray->ClearParticles();
		ValveWheel->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	}
}

void AFTFloodController::ResetForNewShoot()
{
	SetStage(EFTFloodStage::Dry);
}

void AFTFloodController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Target = GetWaterZ();
	VisualZ = FMath::FInterpTo(VisualZ, Target, DeltaSeconds, 3.f);
	const bool bShow = VisualZ > FloorZ - 6.f;
	if (WaterPlane->IsVisible() != bShow)
	{
		WaterPlane->SetVisibility(bShow);
	}
	if (bShow)
	{
		FVector L = WaterPlane->GetComponentLocation();
		L.Z = VisualZ;
		WaterPlane->SetWorldLocation(L);
	}
	ValveSpray->FloorZ = FMath::Max(VisualZ, FloorZ);
	if (ValveSpray->IsEmitting())
	{
		const float Spin = GetWorld()->GetTimeSeconds() * 200.f;
		ValveWheel->SetRelativeRotation(FRotator(90.f, 0.f, Spin));
	}
}
