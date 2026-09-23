#pragma once

#include "CoreMinimal.h"
#include "TheFinalTake/Core/FTTypes.h"

class UStaticMesh;
class UMaterialInterface;
class UStaticMeshComponent;
class UPrimitiveComponent;
class UTextRenderComponent;
class USceneComponent;
class AActor;

/** Studio palette (sRGB hex converted to linear). Keep every asset on this palette. */
namespace FTColors
{
	FLinearColor Hex(uint32 RGB);

	extern const FLinearColor Teal, TealDark, TealLight, Cyan, Coral, CoralDark, Yellow, Amber, Cream, CreamDark;
	extern const FLinearColor Magenta, DeepBlue, Blue, SkyBlue, Navy, NavyLight, Red, Wood, WoodDark, Sand;
	extern const FLinearColor Grey, GreyDark, Charcoal, White, Green, GreenDark, Purple, Orange, Ink;
	extern const FLinearColor SkinA, SkinB, SkinC, SkinD, HairBrown, HairDark, HairGinger, HairBlack;
}

struct FFTPartSpec
{
	EFTShape Shape = EFTShape::Box;
	FVector Location = FVector::ZeroVector;
	FVector Size = FVector(100.f);
	FRotator Rotation = FRotator::ZeroRotator;
	FLinearColor Color = FLinearColor::White;
	float Emissive = 0.f;
	float Gloss = 0.f;
	bool bCollision = false;
};

/** Shared helpers for building chunky low-poly visuals out of the generated shape set. */
class THE_FINAL_TAKE_API FTVis
{
public:
	static UStaticMesh* GetMesh(EFTShape Shape);
	static UMaterialInterface* Matte();
	static UMaterialInterface* Glow();
	static UMaterialInterface* Translucent();
	static UMaterialInterface* Water();
	static UMaterialInterface* Highlight();
	static UMaterialInterface* Screen();

	/** Constructor-time part (CreateDefaultSubobject). Size is in cm. */
	static UStaticMeshComponent* MakePart(AActor* Owner, USceneComponent* Parent, FName Name, EFTShape Shape,
		const FVector& Location, const FVector& Size, const FLinearColor& Color,
		const FRotator& Rotation = FRotator::ZeroRotator, float Emissive = 0.f, bool bCollision = false);

	/** Runtime part (NewObject + RegisterComponent). */
	static UStaticMeshComponent* SpawnPart(AActor* Owner, USceneComponent* Parent, EFTShape Shape,
		const FVector& Location, const FVector& Size, const FLinearColor& Color,
		const FRotator& Rotation = FRotator::ZeroRotator, float Emissive = 0.f, bool bCollision = false);

	/** Applies mesh, material and scale for a shape to an existing component. */
	static void ApplyShape(UStaticMeshComponent* Comp, EFTShape Shape, const FVector& Size);

	/** Colour a primitive. bDefaults writes the serialized defaults (constructor/editor), otherwise runtime data. */
	static void Paint(UPrimitiveComponent* Comp, const FLinearColor& Color, float Emissive = 0.f, bool bDefaults = false, float Gloss = -1.f, float Opacity = 1.f);
	static void SetGlow(UPrimitiveComponent* Comp, float Emissive);

	static UTextRenderComponent* MakeText(AActor* Owner, USceneComponent* Parent, FName Name, const FText& Text,
		const FVector& Location, const FRotator& Rotation, float WorldSize, const FColor& Color, bool bCenter = true);

	/** Standard collision setup for visual-only parts. */
	static void NoCollision(UPrimitiveComponent* Comp);
};
