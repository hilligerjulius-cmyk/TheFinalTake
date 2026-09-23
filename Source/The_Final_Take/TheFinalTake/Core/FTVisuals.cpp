#include "TheFinalTake/Core/FTVisuals.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Actor.h"

namespace FTColors
{
	FLinearColor Hex(uint32 RGB)
	{
		return FLinearColor(FColor((RGB >> 16) & 0xFF, (RGB >> 8) & 0xFF, RGB & 0xFF, 255));
	}

	const FLinearColor Teal = Hex(0x1F9E96);
	const FLinearColor TealDark = Hex(0x13706B);
	const FLinearColor TealLight = Hex(0x5CCFC3);
	const FLinearColor Cyan = Hex(0x33D6E8);
	const FLinearColor Coral = Hex(0xFF6F59);
	const FLinearColor CoralDark = Hex(0xD9483A);
	const FLinearColor Yellow = Hex(0xFFC93C);
	const FLinearColor Amber = Hex(0xFFA62B);
	const FLinearColor Cream = Hex(0xF6E7C8);
	const FLinearColor CreamDark = Hex(0xE0CBA0);
	const FLinearColor Magenta = Hex(0xE0409A);
	const FLinearColor DeepBlue = Hex(0x2346A8);
	const FLinearColor Blue = Hex(0x3D7DD8);
	const FLinearColor SkyBlue = Hex(0x7EC8F2);
	const FLinearColor Navy = Hex(0x1B2140);
	const FLinearColor NavyLight = Hex(0x2E3766);
	const FLinearColor Red = Hex(0xE8322E);
	const FLinearColor Wood = Hex(0xB06A3B);
	const FLinearColor WoodDark = Hex(0x7A4526);
	const FLinearColor Sand = Hex(0xF2C77B);
	const FLinearColor Grey = Hex(0x8C93A6);
	const FLinearColor GreyDark = Hex(0x4B5163);
	const FLinearColor Charcoal = Hex(0x2B2F3A);
	const FLinearColor White = Hex(0xF7F7F2);
	const FLinearColor Green = Hex(0x4DB35E);
	const FLinearColor GreenDark = Hex(0x2F7D45);
	const FLinearColor Purple = Hex(0x7B5BD6);
	const FLinearColor Orange = Hex(0xFF8A2B);
	const FLinearColor Ink = Hex(0x14131F);
	const FLinearColor SkinA = Hex(0xF2C29B);
	const FLinearColor SkinB = Hex(0xE3A77C);
	const FLinearColor SkinC = Hex(0x8D5A3B);
	const FLinearColor SkinD = Hex(0xF7D4BC);
	const FLinearColor HairBrown = Hex(0x5A3522);
	const FLinearColor HairDark = Hex(0x2A1B14);
	const FLinearColor HairGinger = Hex(0xD9642B);
	const FLinearColor HairBlack = Hex(0x1C1C22);
}

namespace
{
	template <typename T>
	T* LoadCached(TWeakObjectPtr<T>& Cache, const TCHAR* Path)
	{
		if (!Cache.IsValid())
		{
			Cache = LoadObject<T>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
		return Cache.Get();
	}

	struct FShapeAsset
	{
		const TCHAR* Custom;
		const TCHAR* Fallback;
	};

	FShapeAsset ShapeAsset(EFTShape Shape)
	{
		switch (Shape)
		{
		case EFTShape::Cube: return { nullptr, TEXT("/Engine/BasicShapes/Cube.Cube") };
		case EFTShape::Box: return { TEXT("/Game/TheFinalTake/Meshes/SM_FT_Box.SM_FT_Box"), TEXT("/Engine/BasicShapes/Cube.Cube") };
		case EFTShape::Sphere: return { TEXT("/Game/TheFinalTake/Meshes/SM_FT_Sphere.SM_FT_Sphere"), TEXT("/Engine/BasicShapes/Sphere.Sphere") };
		case EFTShape::Ball: return { TEXT("/Game/TheFinalTake/Meshes/SM_FT_Ball.SM_FT_Ball"), TEXT("/Engine/BasicShapes/Sphere.Sphere") };
		case EFTShape::Cylinder: return { TEXT("/Game/TheFinalTake/Meshes/SM_FT_Cylinder.SM_FT_Cylinder"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder") };
		case EFTShape::Cone: return { TEXT("/Game/TheFinalTake/Meshes/SM_FT_Cone.SM_FT_Cone"), TEXT("/Engine/BasicShapes/Cone.Cone") };
		case EFTShape::Prism: return { TEXT("/Game/TheFinalTake/Meshes/SM_FT_Prism.SM_FT_Prism"), TEXT("/Engine/BasicShapes/Cube.Cube") };
		case EFTShape::Ramp: return { TEXT("/Game/TheFinalTake/Meshes/SM_FT_Ramp.SM_FT_Ramp"), TEXT("/Engine/BasicShapes/Cube.Cube") };
		case EFTShape::Torus: return { TEXT("/Game/TheFinalTake/Meshes/SM_FT_Torus.SM_FT_Torus"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder") };
		case EFTShape::Capsule: return { TEXT("/Game/TheFinalTake/Meshes/SM_FT_Capsule.SM_FT_Capsule"), TEXT("/Engine/BasicShapes/Sphere.Sphere") };
		case EFTShape::Plane: return { nullptr, TEXT("/Engine/BasicShapes/Plane.Plane") };
		case EFTShape::WaterGrid: return { TEXT("/Game/TheFinalTake/Meshes/SM_FT_WaterGrid.SM_FT_WaterGrid"), TEXT("/Engine/BasicShapes/Plane.Plane") };
		}
		return { nullptr, TEXT("/Engine/BasicShapes/Cube.Cube") };
	}
}

UStaticMesh* FTVis::GetMesh(EFTShape Shape)
{
	static TWeakObjectPtr<UStaticMesh> Cache[16];
	static TWeakObjectPtr<UStaticMesh> FallbackCache[16];
	const int32 Index = static_cast<int32>(Shape);
	const FShapeAsset Asset = ShapeAsset(Shape);
	if (Asset.Custom)
	{
		if (UStaticMesh* Mesh = LoadCached(Cache[Index], Asset.Custom))
		{
			return Mesh;
		}
	}
	return LoadCached(FallbackCache[Index], Asset.Fallback);
}

UMaterialInterface* FTVis::Matte()
{
	static TWeakObjectPtr<UMaterialInterface> Cache;
	static TWeakObjectPtr<UMaterialInterface> Fallback;
	if (UMaterialInterface* M = LoadCached(Cache, TEXT("/Game/TheFinalTake/Materials/M_FT_Matte.M_FT_Matte")))
	{
		return M;
	}
	return LoadCached(Fallback, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
}

UMaterialInterface* FTVis::Glow()
{
	static TWeakObjectPtr<UMaterialInterface> Cache;
	UMaterialInterface* M = LoadCached(Cache, TEXT("/Game/TheFinalTake/Materials/M_FT_Glow.M_FT_Glow"));
	return M ? M : Matte();
}

UMaterialInterface* FTVis::Translucent()
{
	static TWeakObjectPtr<UMaterialInterface> Cache;
	UMaterialInterface* M = LoadCached(Cache, TEXT("/Game/TheFinalTake/Materials/M_FT_Translucent.M_FT_Translucent"));
	return M ? M : Matte();
}

UMaterialInterface* FTVis::Water()
{
	static TWeakObjectPtr<UMaterialInterface> Cache;
	UMaterialInterface* M = LoadCached(Cache, TEXT("/Game/TheFinalTake/Materials/M_FT_Water.M_FT_Water"));
	return M ? M : Matte();
}

UMaterialInterface* FTVis::Highlight()
{
	static TWeakObjectPtr<UMaterialInterface> Cache;
	return LoadCached(Cache, TEXT("/Game/TheFinalTake/Materials/M_FT_Highlight.M_FT_Highlight"));
}

UMaterialInterface* FTVis::Screen()
{
	static TWeakObjectPtr<UMaterialInterface> Cache;
	return LoadCached(Cache, TEXT("/Game/TheFinalTake/Materials/M_FT_Screen.M_FT_Screen"));
}

void FTVis::NoCollision(UPrimitiveComponent* Comp)
{
	if (!Comp)
	{
		return;
	}
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Comp->SetGenerateOverlapEvents(false);
	Comp->SetCanEverAffectNavigation(false);
	Comp->CanCharacterStepUpOn = ECB_No;
}

void FTVis::ApplyShape(UStaticMeshComponent* Comp, EFTShape Shape, const FVector& Size)
{
	if (!Comp)
	{
		return;
	}
	Comp->SetStaticMesh(GetMesh(Shape));
	UMaterialInterface* Mat = Shape == EFTShape::WaterGrid ? Water() : Matte();
	Comp->SetMaterial(0, Mat);
	Comp->SetRelativeScale3D(Size / 100.f);
}

void FTVis::Paint(UPrimitiveComponent* Comp, const FLinearColor& Color, float Emissive, bool bDefaults, float Gloss, float Opacity)
{
	if (!Comp)
	{
		return;
	}
	const FVector4 C(Color.R, Color.G, Color.B, Opacity);
	if (bDefaults)
	{
		Comp->SetDefaultCustomPrimitiveDataVector4(0, C);
		Comp->SetDefaultCustomPrimitiveDataFloat(4, Emissive);
		Comp->SetDefaultCustomPrimitiveDataFloat(5, FMath::Max(Gloss, 0.f));
	}
	else
	{
		Comp->SetCustomPrimitiveDataVector4(0, C);
		Comp->SetCustomPrimitiveDataFloat(4, Emissive);
		if (Gloss >= 0.f)
		{
			Comp->SetCustomPrimitiveDataFloat(5, Gloss);
		}
	}
}

void FTVis::SetGlow(UPrimitiveComponent* Comp, float Emissive)
{
	if (Comp)
	{
		Comp->SetCustomPrimitiveDataFloat(4, Emissive);
	}
}

static void ConfigurePart(UStaticMeshComponent* Part, EFTShape Shape, const FVector& Location, const FVector& Size,
	const FLinearColor& Color, const FRotator& Rotation, float Emissive, bool bCollision, bool bDefaults)
{
	FTVis::ApplyShape(Part, Shape, Size);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeRotation(Rotation);
	if (bCollision)
	{
		Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	else
	{
		FTVis::NoCollision(Part);
	}
	Part->bCastDynamicShadow = true;
	Part->SetCastShadow(Size.GetMin() > 3.f);
	Part->bReceivesDecals = false;
	FTVis::Paint(Part, Color, Emissive, bDefaults);
}

UStaticMeshComponent* FTVis::MakePart(AActor* Owner, USceneComponent* Parent, FName Name, EFTShape Shape,
	const FVector& Location, const FVector& Size, const FLinearColor& Color, const FRotator& Rotation, float Emissive, bool bCollision)
{
	UStaticMeshComponent* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
	if (Parent)
	{
		Part->SetupAttachment(Parent);
	}
	ConfigurePart(Part, Shape, Location, Size, Color, Rotation, Emissive, bCollision, true);
	return Part;
}

UStaticMeshComponent* FTVis::SpawnPart(AActor* Owner, USceneComponent* Parent, EFTShape Shape,
	const FVector& Location, const FVector& Size, const FLinearColor& Color, const FRotator& Rotation, float Emissive, bool bCollision)
{
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Owner);
	ConfigurePart(Part, Shape, Location, Size, Color, Rotation, Emissive, bCollision, true);
	if (Parent)
	{
		Part->SetupAttachment(Parent);
	}
	Part->RegisterComponent();
	return Part;
}

UTextRenderComponent* FTVis::MakeText(AActor* Owner, USceneComponent* Parent, FName Name, const FText& Text,
	const FVector& Location, const FRotator& Rotation, float WorldSize, const FColor& Color, bool bCenter)
{
	UTextRenderComponent* T = Owner->CreateDefaultSubobject<UTextRenderComponent>(Name);
	if (Parent)
	{
		T->SetupAttachment(Parent);
	}
	T->SetText(Text);
	T->SetRelativeLocation(Location);
	T->SetRelativeRotation(Rotation);
	T->SetWorldSize(WorldSize);
	T->SetTextRenderColor(Color);
	T->SetHorizontalAlignment(bCenter ? EHTA_Center : EHTA_Left);
	T->SetVerticalAlignment(EVRTA_TextCenter);
	T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	T->SetCastShadow(false);
	return T;
}
