#include "TheFinalTake/World/FTZone.h"

#include "TheFinalTake/Core/FTVisuals.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

AFTZone::AFTZone()
{
	PrimaryActorTick.bCanEverTick = false;
	Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("Volume"));
	RootComponent = Volume;
	Volume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Volume->SetHiddenInGame(true);
	Volume->ShapeColor = FColor(80, 220, 200);
	Volume->SetBoxExtent(Extent);
	Volume->SetMobility(EComponentMobility::Static);

	MarkA = FTVis::MakePart(this, Volume, TEXT("MarkA"), EFTShape::Cube, FVector::ZeroVector, FVector(10.f), FLinearColor::Yellow);
	MarkB = FTVis::MakePart(this, Volume, TEXT("MarkB"), EFTShape::Cube, FVector::ZeroVector, FVector(10.f), FLinearColor::Yellow);
	MarkC = FTVis::MakePart(this, Volume, TEXT("MarkC"), EFTShape::Cube, FVector::ZeroVector, FVector(10.f), FLinearColor::Yellow);
	MarkD = FTVis::MakePart(this, Volume, TEXT("MarkD"), EFTShape::Cube, FVector::ZeroVector, FVector(10.f), FLinearColor::Yellow);
	for (UStaticMeshComponent* M : { MarkA.Get(), MarkB.Get(), MarkC.Get(), MarkD.Get() })
	{
		M->SetCastShadow(false);
		M->SetMobility(EComponentMobility::Static);
	}
	Label = FTVis::MakeText(this, Volume, TEXT("Label"), FText::GetEmpty(), FVector::ZeroVector, FRotator(90.f, 0.f, 180.f), 28.f, FColor::White);
	Label->SetMobility(EComponentMobility::Static);
}

void AFTZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Volume->SetBoxExtent(Extent);
	const float FloorZ = -Extent.Z + 0.6f;
	TArray<UStaticMeshComponent*> Marks = { MarkA, MarkB, MarkC, MarkD };
	for (UStaticMeshComponent* M : Marks)
	{
		M->SetVisibility(false);
	}
	const FLinearColor C = MarkColor;
	switch (Mark)
	{
	case EFTZoneMark::TapeX:
	{
		const float Len = FMath::Min(Extent.X, Extent.Y) * 1.8f;
		FTVis::ApplyShape(MarkA, EFTShape::Cube, FVector(Len, 12.f, 1.f));
		FTVis::ApplyShape(MarkB, EFTShape::Cube, FVector(Len, 12.f, 1.f));
		MarkA->SetRelativeLocationAndRotation(FVector(0.f, 0.f, FloorZ), FRotator(0.f, 45.f, 0.f));
		MarkB->SetRelativeLocationAndRotation(FVector(0.f, 0.f, FloorZ), FRotator(0.f, -45.f, 0.f));
		MarkA->SetVisibility(true);
		MarkB->SetVisibility(true);
		break;
	}
	case EFTZoneMark::Ring:
		FTVis::ApplyShape(MarkA, EFTShape::Torus, FVector(Extent.X * 2.f, Extent.Y * 2.f, 3.f));
		MarkA->SetRelativeLocationAndRotation(FVector(0.f, 0.f, FloorZ), FRotator::ZeroRotator);
		MarkA->SetVisibility(true);
		break;
	case EFTZoneMark::Outline:
		FTVis::ApplyShape(MarkA, EFTShape::Cube, FVector(Extent.X * 2.f, 10.f, 1.f));
		FTVis::ApplyShape(MarkB, EFTShape::Cube, FVector(Extent.X * 2.f, 10.f, 1.f));
		FTVis::ApplyShape(MarkC, EFTShape::Cube, FVector(10.f, Extent.Y * 2.f, 1.f));
		FTVis::ApplyShape(MarkD, EFTShape::Cube, FVector(10.f, Extent.Y * 2.f, 1.f));
		MarkA->SetRelativeLocationAndRotation(FVector(0.f, Extent.Y, FloorZ), FRotator::ZeroRotator);
		MarkB->SetRelativeLocationAndRotation(FVector(0.f, -Extent.Y, FloorZ), FRotator::ZeroRotator);
		MarkC->SetRelativeLocationAndRotation(FVector(Extent.X, 0.f, FloorZ), FRotator::ZeroRotator);
		MarkD->SetRelativeLocationAndRotation(FVector(-Extent.X, 0.f, FloorZ), FRotator::ZeroRotator);
		for (UStaticMeshComponent* M : Marks)
		{
			M->SetVisibility(true);
		}
		break;
	default:
		break;
	}
	for (UStaticMeshComponent* M : Marks)
	{
		FTVis::Paint(M, C, 0.6f, true);
	}
	Label->SetText(MarkLabel);
	Label->SetRelativeLocation(FVector(-Extent.X * 0.55f, 0.f, FloorZ + 1.f));
	Label->SetTextRenderColor(C.ToFColor(true));
	Label->SetVisibility(!MarkLabel.IsEmpty());
}

bool AFTZone::Contains(const FVector& Location) const
{
	const FVector Local = GetActorTransform().InverseTransformPositionNoScale(Location);
	return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}

bool AFTZone::IsInZone(const UObject* WorldContext, FName Tag, const FVector& Location)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}
	for (TActorIterator<AFTZone> It(World); It; ++It)
	{
		if (It->ZoneTag == Tag && It->Contains(Location))
		{
			return true;
		}
	}
	return false;
}

AFTZone* AFTZone::Find(const UObject* WorldContext, FName Tag)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AFTZone> It(World); It; ++It)
	{
		if (It->ZoneTag == Tag)
		{
			return *It;
		}
	}
	return nullptr;
}
