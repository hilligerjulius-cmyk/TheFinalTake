#include "TheFinalTake/World/FTStudioShell.h"

#include "TheFinalTake/Core/FTVisuals.h"
#include "TheFinalTake/Game/FTGameState.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"

using namespace FTColors;

namespace
{
	// studio-specific shades (on the shared palette)
	const FLinearColor Asphalt = Hex(0x3C4262);
	const FLinearColor Sidewalk = Hex(0x6E7391);
	const FLinearColor StageFloor = Hex(0x646B8C);
	const FLinearColor StageWall = Hex(0x232A4F);
	const FLinearColor StageWallLow = Hex(0x1C5C63);
	const FLinearColor LobbyWall = Hex(0xF3DFC0);
	const FLinearColor Ceiling = Hex(0xE2CFA8);
	const FLinearColor Tape = Hex(0xFFC93C);
	const FLinearColor TankBlue = Hex(0x3D7DD8);
	const FLinearColor Backdrop = Hex(0x7EC8F2);
	const FLinearColor Glass = FLinearColor(0.55f, 0.85f, 1.f, 0.22f);
	const FLinearColor Carpet = Hex(0xC9304A);
	const FLinearColor Terracotta = Hex(0xD9784A);
	const FLinearColor Brass = Hex(0xE3B04B);

	UMaterialInterface* IsmMaterial()
	{
		static TWeakObjectPtr<UMaterialInterface> Cache;
		if (!Cache.IsValid())
		{
			UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/TheFinalTake/Materials/M_FT_MatteISM.M_FT_MatteISM"), nullptr, LOAD_NoWarn | LOAD_Quiet);
			if (M && !IsRunningCommandlet() && !M->IsRooted())
			{
				M->AddToRoot();
			}
			Cache = M;
		}
		return Cache.IsValid() ? Cache.Get() : FTVis::Matte();
	}

	enum EZone { ZExterior = 0, ZLobby = 1, ZStage = 2, ZProjection = 3 };
}

AFTStudioShell::AFTStudioShell()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	Root->SetMobility(EComponentMobility::Static);
	static const EFTShape Shapes[NumGroups] = {
		EFTShape::Cube, EFTShape::Cube, EFTShape::Box, EFTShape::Box, EFTShape::Cylinder, EFTShape::Cylinder,
		EFTShape::Sphere, EFTShape::Ball, EFTShape::Cone, EFTShape::Prism, EFTShape::Ramp, EFTShape::Torus,
		EFTShape::Capsule, EFTShape::Cube, EFTShape::Cube, EFTShape::Shoreline };
	static const bool Solid[NumGroups] = { true, false, true, false, true, false, false, false, false, false, true, false, false, true, true, false };
	for (int32 g = 0; g < NumGroups; ++g)
	{
		UInstancedStaticMeshComponent* I = CreateDefaultSubobject<UInstancedStaticMeshComponent>(*FString::Printf(TEXT("Group%02d"), g));
		I->SetupAttachment(Root);
		I->SetStaticMesh(FTVis::GetMesh(Shapes[g]));
		I->SetMobility(EComponentMobility::Static);
		I->NumCustomDataFloats = 5;
		if (Solid[g])
		{
			I->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			I->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		else
		{
			FTVis::NoCollision(I);
		}
		I->bCastDynamicShadow = true;
		Groups.Add(I);
	}
	Groups[GlassSolid]->SetCastShadow(false);
	Groups[Blocker]->SetHiddenInGame(true);
	Groups[Blocker]->SetCastShadow(false);

	for (int32 i = 0; i < 72; ++i)
	{
		UTextRenderComponent* T = CreateDefaultSubobject<UTextRenderComponent>(*FString::Printf(TEXT("Text%02d"), i));
		T->SetupAttachment(Root);
		T->SetHorizontalAlignment(EHTA_Center);
		T->SetVerticalAlignment(EVRTA_TextCenter);
		T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		T->SetCastShadow(false);
		T->SetMobility(EComponentMobility::Static);
		if (UMaterialInterface* TM = FTVis::TextMaterial())
		{
			T->SetTextMaterial(TM);
		}
		Texts.Add(T);
	}
	for (int32 i = 0; i < 36; ++i)
	{
		UPointLightComponent* P = CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("Point%02d"), i));
		P->SetupAttachment(Root);
		P->SetMobility(EComponentMobility::Movable);
		P->SetCastShadows(false);
		Points.Add(P);
	}
	for (int32 i = 0; i < 12; ++i)
	{
		USpotLightComponent* S = CreateDefaultSubobject<USpotLightComponent>(*FString::Printf(TEXT("Spot%02d"), i));
		S->SetupAttachment(Root);
		S->SetMobility(EComponentMobility::Movable);
		S->SetCastShadows(false);
		Spots.Add(S);
	}
	TankWater = FTVis::MakePart(this, Root, TEXT("TankWater"), EFTShape::WaterGrid, FVector(1800.f, 100.f, -50.f), FVector(920.f, 1920.f, 100.f), Hex(0x2E8FE8));
	TankWater->SetCastShadow(false);
	TankWater->SetBoundsScale(2.f);
}

void AFTStudioShell::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
}

void AFTStudioShell::BeginPlay()
{
	Super::BeginPlay();
	ApplyLighting();
}

void AFTStudioShell::ClearAll()
{
	for (int32 g = 0; g < Groups.Num(); ++g)
	{
		UInstancedStaticMeshComponent* I = Groups[g];
		I->ClearInstances();
		I->NumCustomDataFloats = 5;
		I->SetMaterial(0, g == GlassSolid ? FTVis::Translucent() : IsmMaterial());
		FTVis::Paint(I, g == GlassSolid ? Glass : FLinearColor::White, 0.f, true, 0.f, g == GlassSolid ? Glass.A : 1.f);
	}
	for (UTextRenderComponent* T : Texts)
	{
		T->SetVisibility(false);
	}
	for (UPointLightComponent* P : Points)
	{
		P->SetVisibility(false);
	}
	for (USpotLightComponent* S : Spots)
	{
		S->SetVisibility(false);
	}
	NextText = NextPoint = NextSpot = 0;
	LightBase.Reset();
	LightZone.Reset();
	LightBase.SetNumZeroed(Points.Num() + Spots.Num());
	LightZone.SetNumZeroed(Points.Num() + Spots.Num());
}

int32 AFTStudioShell::Add(int32 Group, const FVector& Center, const FVector& Size, const FLinearColor& Color, float Emissive, const FRotator& Rot, float Gloss)
{
	UInstancedStaticMeshComponent* I = Groups[Group];
	const int32 Index = I->AddInstance(FTransform(Rot, Center, Size / 100.f), false);
	I->SetCustomData(Index, { Color.R, Color.G, Color.B, Emissive, Gloss }, false);
	return Index;
}

void AFTStudioShell::Wall(const FVector& Min, const FVector& Max, const FLinearColor& Color, bool bSolid)
{
	Add(bSolid ? CubeSolid : CubeDeco, (Min + Max) * 0.5f, (Max - Min).GetAbs(), Color);
}

void AFTStudioShell::Text(const FString& S, const FVector& Loc, float Yaw, float Size, const FColor& Color, float Pitch)
{
	if (!Texts.IsValidIndex(NextText))
	{
		return;
	}
	UTextRenderComponent* T = Texts[NextText++];
	T->SetText(FText::FromString(S));
	T->SetRelativeLocationAndRotation(Loc, FRotator(Pitch, Yaw, 0.f));
	T->SetWorldSize(Size);
	T->SetTextRenderColor(Color);
	T->SetVisibility(true);
}

void AFTStudioShell::Point(const FVector& Loc, const FLinearColor& Color, float Intensity, float Radius, int32 Zone, bool bShadows)
{
	if (!Points.IsValidIndex(NextPoint))
	{
		return;
	}
	const int32 Slot = NextPoint;
	UPointLightComponent* P = Points[NextPoint++];
	P->SetRelativeLocation(Loc);
	P->SetLightColor(Color);
	P->SetIntensity(Intensity);
	P->SetAttenuationRadius(Radius);
	P->SetSourceRadius(32.f);
	P->SetSoftSourceRadius(48.f);
	P->SetCastShadows(bShadows);
	P->SetVisibility(true);
	LightBase[Slot] = Intensity;
	LightZone[Slot] = Zone;
}

void AFTStudioShell::Spot(const FVector& Loc, const FRotator& Rot, const FLinearColor& Color, float Intensity, float Radius, float ConeAngle, int32 Zone)
{
	if (!Spots.IsValidIndex(NextSpot))
	{
		return;
	}
	const int32 Slot = Points.Num() + NextSpot;
	USpotLightComponent* S = Spots[NextSpot++];
	S->SetRelativeLocationAndRotation(Loc, Rot);
	S->SetLightColor(Color);
	S->SetIntensity(Intensity);
	S->SetAttenuationRadius(Radius);
	S->SetSourceRadius(18.f);
	S->SetOuterConeAngle(ConeAngle);
	S->SetInnerConeAngle(ConeAngle * 0.7f);
	S->SetVisibility(true);
	LightBase[Slot] = Intensity;
	LightZone[Slot] = Zone;
}

void AFTStudioShell::Arrow(const FVector& From, const FVector& To, const FLinearColor& Color, int32 Count)
{
	const FVector D = (To - From);
	const float Yaw = D.Rotation().Yaw;
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector P = From + D * (Count > 1 ? (float)i / (Count - 1) : 0.f);
		// shaft + head, flat on the floor
		Add(CubeDeco, P + FRotator(0.f, Yaw, 0.f).RotateVector(FVector(-35.f, 0.f, 0.f)), FVector(70.f, 26.f, 1.f), Color, 0.35f, FRotator(0.f, Yaw, 0.f));
		Add(PrismDeco, P + FRotator(0.f, Yaw, 0.f).RotateVector(FVector(22.f, 0.f, 0.f)), FVector(1.5f, 70.f, 50.f), Color, 0.35f, FRotator(-90.f, Yaw, 0.f));
	}
}

void AFTStudioShell::Stairs(const FVector& Start, float Run, float Width, float Rise, int32 Steps, bool bAlongX, const FLinearColor& Tread, const FLinearColor& Edge)
{
	// Start = floor-level corner centre of the first step; each step rises by Rise and advances by Run
	for (int32 k = 0; k < Steps; ++k)
	{
		const float Top = Start.Z + Rise * (k + 1);
		const float Bottom = (Rise > 0.f ? Start.Z : Start.Z + Rise * (Steps + 1)) - 1.f;
		const float Mid = Run * (k + 0.5f);
		const FVector C = bAlongX ? FVector(Start.X + Mid, Start.Y, (Top + Bottom) * 0.5f) : FVector(Start.X, Start.Y + Mid, (Top + Bottom) * 0.5f);
		const FVector S = bAlongX ? FVector(FMath::Abs(Run), Width, Top - Bottom) : FVector(Width, FMath::Abs(Run), Top - Bottom);
		Add(CubeSolid, C, S, Tread);
		const FVector EdgeC = bAlongX ? FVector(Start.X + Run * k + Run * 0.1f, Start.Y, Top + 0.5f) : FVector(Start.X, Start.Y + Run * k + Run * 0.1f, Top + 0.5f);
		const FVector EdgeS = bAlongX ? FVector(FMath::Abs(Run) * 0.2f, Width, 1.f) : FVector(Width, FMath::Abs(Run) * 0.2f, 1.f);
		Add(CubeDeco, EdgeC, EdgeS, Edge, 0.2f);
	}
}

void AFTStudioShell::Poster(const FVector& Loc, float Yaw, int32 Film, float Scale)
{
	const FRotator R(0.f, Yaw, 0.f);
	auto L = [&](const FVector& Local) { return Loc + R.RotateVector(Local * Scale); };
	FLinearColor Bg = Blue, Accent = Coral;
	FString Title = TEXT("JAWS OF THE STUDIO");
	if (Film == 1) { Bg = Purple; Accent = Cyan; Title = TEXT("MOONFALL MOTEL"); }
	if (Film == 2) { Bg = Coral; Accent = Yellow; Title = TEXT("CASTLE ON FIRE"); }
	Add(BoxDeco, L(FVector(0.f, 0.f, 0.f)), FVector(8.f, 170.f, 250.f) * Scale, Charcoal, 0.f, R);
	Add(CubeDeco, L(FVector(4.5f, 0.f, 0.f)), FVector(2.f, 150.f, 230.f) * Scale, Bg, 0.15f, R);
	switch (Film)
	{
	case 0:
		Add(CubeDeco, L(FVector(6.f, 0.f, -70.f)), FVector(1.f, 150.f, 60.f) * Scale, DeepBlue, 0.1f, R);
		Add(PrismDeco, L(FVector(6.5f, 0.f, -20.f)), FVector(1.f, 70.f, 70.f) * Scale, Navy, 0.f, R + FRotator(0.f, 90.f, 0.f) + FRotator(0.f, -90.f, 0.f));
		break;
	case 1:
		Add(BallDeco, L(FVector(7.f, 0.f, 40.f)), FVector(3.f, 100.f, 26.f) * Scale, Grey, 0.2f, R);
		Add(PrismDeco, L(FVector(6.f, 0.f, -20.f)), FVector(1.f, 90.f, 110.f) * Scale, Accent, 0.6f, R + FRotator(0.f, 0.f, 180.f));
		break;
	default:
		for (int32 i = 0; i < 4; ++i)
		{
			Add(CubeDeco, L(FVector(6.f, -45.f + i * 30.f, -20.f + (i % 2) * 14.f)), FVector(1.f, 22.f, 90.f) * Scale, Cream, 0.1f, R);
		}
		Add(PrismDeco, L(FVector(6.5f, 0.f, 50.f)), FVector(1.f, 60.f, 50.f) * Scale, Accent, 0.8f, R);
		break;
	}
	Text(Title, L(FVector(8.f, 0.f, 95.f)), Yaw, 16.f * Scale, FColor(255, 245, 225));
}

void AFTStudioShell::Plant(const FVector& Loc, float Scale)
{
	Add(CylDeco, Loc + FVector(0.f, 0.f, 25.f * Scale), FVector(50.f, 50.f, 50.f) * Scale, Terracotta, 0.f, FRotator::ZeroRotator, 0.3f);
	Add(TorusDeco, Loc + FVector(0.f, 0.f, 48.f * Scale), FVector(55.f, 55.f, 10.f) * Scale, CreamDark);
	Add(CylDeco, Loc + FVector(0.f, 0.f, 47.f * Scale), FVector(43.f, 43.f, 4.f) * Scale, WoodDark);
	// Broad, individually posed leaves: a studio prop with a readable silhouette.
	for (int32 i = 0; i < 7; ++i)
	{
		const float Angle = i * 137.5f;
		const FRotator R(22.f + (i % 3) * 12.f, Angle, 0.f);
		const FVector Offset = FRotator(0.f, Angle, 0.f).RotateVector(FVector(20.f, 0.f, 73.f + (i % 3) * 16.f));
		Add(BallDeco, Loc + Offset * Scale, FVector(63.f, 22.f, 11.f) * Scale, i % 2 ? Green : TealDark, 0.f, R);
		Add(CapsuleDeco, Loc + FVector(Offset.X * 0.4f, Offset.Y * 0.4f, 62.f + (i % 3) * 8.f) * Scale,
			FVector(5.f, 5.f, 45.f) * Scale, GreenDark, 0.f, FRotator(15.f, Angle, 0.f));
	}
}

void AFTStudioShell::Crate(const FVector& Loc, float Size, float Yaw)
{
	const FRotator R(0.f, Yaw, 0.f);
	Add(BoxSolid, Loc + FVector(0.f, 0.f, Size * 0.5f), FVector(Size), Wood, 0.f, R);
	Add(CubeDeco, Loc + FVector(0.f, 0.f, Size * 0.5f), FVector(Size * 1.02f, Size * 1.02f, Size * 0.14f), WoodDark, 0.f, R);
	for (float Side : { -1.f, 1.f })
	{
		for (float Edge : { -0.36f, 0.36f })
		{
			Add(BoxDeco, Loc + R.RotateVector(FVector(Side * Size * 0.51f, Edge * Size, Size * 0.5f)),
				FVector(Size * 0.055f, Size * 0.12f, Size * 0.92f), CreamDark, 0.f, R);
		}
		Add(BoxDeco, Loc + R.RotateVector(FVector(Side * Size * 0.54f, 0.f, Size * 0.7f)),
			FVector(2.f, Size * 0.28f, Size * 0.1f), WoodDark, 0.f, R);
	}
}

void AFTStudioShell::FlightCase(const FVector& Loc, const FVector& Size, float Yaw)
{
	const FRotator R(0.f, Yaw, 0.f);
	Add(BoxSolid, Loc + FVector(0.f, 0.f, Size.Z * 0.5f), Size, Charcoal, 0.f, R);
	Add(CubeDeco, Loc + FVector(0.f, 0.f, Size.Z * 0.5f), FVector(Size.X * 1.03f, Size.Y * 1.03f, 6.f), Grey, 0.f, R, 0.75f);
	Add(CubeDeco, Loc + FVector(0.f, 0.f, Size.Z - 2.f), FVector(Size.X * 1.03f, Size.Y * 1.03f, 4.f), Grey, 0.f, R, 0.75f);
	auto Detail = [&](FVector P, FVector Extent, FLinearColor Color)
	{
		Add(BoxDeco, Loc + R.RotateVector(P), Extent, Color, 0.f, R, Color == Grey || Color == CreamDark ? 0.7f : 0.12f);
	};
	for (float X : { -1.f, 1.f })
	{
		for (float Y : { -1.f, 1.f })
		{
			Detail(FVector(X * Size.X * 0.47f, Y * Size.Y * 0.47f, Size.Z * 0.5f), FVector(7.f, 7.f, Size.Z * 0.9f), Grey);
			Detail(FVector(X * Size.X * 0.46f, Y * Size.Y * 0.46f, 6.f), FVector(15.f, 15.f, 12.f), GreyDark);
		}
		Detail(FVector(X * Size.X * 0.28f, -Size.Y * 0.515f, Size.Z * 0.58f), FVector(12.f, 4.f, 20.f), CreamDark);
		Detail(FVector(X * Size.X * 0.28f, -Size.Y * 0.54f, Size.Z * 0.59f), FVector(5.f, 3.f, 9.f), Charcoal);
	}
	Detail(FVector(0.f, -Size.Y * 0.52f, Size.Z * 0.28f), FVector(Size.X * 0.32f, 3.f, Size.Z * 0.18f), Teal);
	Detail(FVector(0.f, -Size.Y * 0.55f, Size.Z * 0.8f), FVector(28.f, 6.f, 7.f), GreyDark);
}

void AFTStudioShell::Cone(const FVector& Loc)
{
	Add(CubeDeco, Loc + FVector(0.f, 0.f, 2.f), FVector(44.f, 44.f, 4.f), Orange);
	Add(ConeDeco, Loc + FVector(0.f, 0.f, 36.f), FVector(34.f, 34.f, 64.f), Orange);
	Add(CylDeco, Loc + FVector(0.f, 0.f, 40.f), FVector(24.f, 24.f, 10.f), White);
}

void AFTStudioShell::Palm(const FVector& Loc, float Height)
{
	const int32 Segs = 5;
	for (int32 i = 0; i < Segs; ++i)
	{
		const float T = (float)i / Segs;
		Add(CylDeco, Loc + FVector(T * 30.f, 0.f, Height * (T + 0.5f / Segs)), FVector(26.f - i * 2.f, 26.f - i * 2.f, Height / Segs + 4.f), i % 2 ? Wood : WoodDark, 0.f, FRotator(-6.f, 0.f, 0.f));
	}
	const FVector Top = Loc + FVector(30.f, 0.f, Height);
	for (int32 i = 0; i < 6; ++i)
	{
		const float Yaw = i * 60.f;
		const FRotator Around(0.f, Yaw, 0.f);
		const FLinearColor Leaf = i % 2 ? Green : TealDark;
		Add(BallDeco, Top + Around.RotateVector(FVector(49.f, 0.f, 10.f)), FVector(115.f, 38.f, 12.f), Leaf, 0.f, FRotator(10.f, Yaw, 0.f));
		Add(BallDeco, Top + Around.RotateVector(FVector(111.f, 0.f, -3.f)), FVector(65.f, 26.f, 8.f), Leaf, 0.f, FRotator(-27.f, Yaw, 0.f));
		Add(CapsuleDeco, Top + Around.RotateVector(FVector(50.f, 0.f, 15.f)), FVector(3.f, 3.f, 96.f), GreenDark, 0.f, FRotator(-80.f, Yaw, 0.f));
	}
	for (int32 Nut = 0; Nut < 3; ++Nut)
	{
		Add(SphereDeco, Top + FRotator(0.f, Nut * 120.f, 0.f).RotateVector(FVector(13.f, 0.f, -12.f)), FVector(25.f, 25.f, 31.f), Wood);
	}
}

void AFTStudioShell::Truss(float Y, float Z)
{
	const float X0 = -500.f, X1 = 2300.f;
	for (float Dy : { -30.f, 30.f })
	{
		for (float Dz : { -25.f, 25.f })
		{
			Add(CubeDeco, FVector((X0 + X1) * 0.5f, Y + Dy, Z + Dz), FVector(X1 - X0, 8.f, 8.f), GreyDark);
		}
	}
	for (float X = X0; X <= X1; X += 140.f)
	{
		Add(CubeDeco, FVector(X, Y, Z), FVector(6.f, 60.f, 6.f), GreyDark, 0.f, FRotator(0.f, 0.f, 0.f));
		Add(CubeDeco, FVector(X + 35.f, Y - 30.f, Z), FVector(6.f, 6.f, 70.f), GreyDark, 0.f, FRotator(35.f, 0.f, 0.f));
		Add(CubeDeco, FVector(X + 35.f, Y + 30.f, Z), FVector(6.f, 6.f, 70.f), GreyDark, 0.f, FRotator(-35.f, 0.f, 0.f));
	}
}

// ============================================================================ build

void AFTStudioShell::Build()
{
	ClearAll();
	BuildExterior();
	BuildLobby();
	BuildOffice();
	BuildWardrobe();
	BuildStage();
	BuildTankSet();
	BuildUpperLevel();
	BuildWarehouse();
	for (UInstancedStaticMeshComponent* I : Groups)
	{
		I->MarkRenderStateDirty();
	}
	ApplyLighting();
}

void AFTStudioShell::BuildExterior()
{
	// street, sidewalk, road paint
	Wall(FVector(-3700.f, -2200.f, -30.f), FVector(-1800.f, 2600.f, 0.f), Asphalt);
	Wall(FVector(-2250.f, -2200.f, 0.f), FVector(-1800.f, 2600.f, 4.f), Sidewalk);
	for (float Y = -1900.f; Y < 2500.f; Y += 420.f)
	{
		Add(CubeDeco, FVector(-3000.f, Y, 0.6f), FVector(26.f, 220.f, 1.f), Tape, 0.2f);
	}
	Add(CubeDeco, FVector(-2255.f, 200.f, 2.f), FVector(12.f, 4800.f, 6.f), Cream);
	// puddles
	const FVector Puddles[] = { FVector(-2080.f, -600.f, 4.6f), FVector(-2150.f, 420.f, 4.6f), FVector(-2600.f, -150.f, 0.8f), FVector(-2900.f, 900.f, 0.8f) };
	for (const FVector& P : Puddles)
	{
		Add(CylDeco, P, FVector(180.f, 110.f, 1.f), Hex(0x3456A8), 0.25f);
	}
	// facade (south wall) with the entrance opening
	const FLinearColor Facade = Teal;
	Wall(FVector(-1820.f, -1620.f, 0.f), FVector(-1780.f, -260.f, 820.f), Facade);
	Wall(FVector(-1820.f, 260.f, 0.f), FVector(-1780.f, 2020.f, 820.f), Facade);
	Wall(FVector(-1820.f, -260.f, 360.f), FVector(-1780.f, 260.f, 820.f), Facade);
	Add(BoxDeco, FVector(-1826.f, 200.f, 820.f), FVector(70.f, 3700.f, 40.f), Cream);
	Add(CubeDeco, FVector(-1823.f, 200.f, 560.f), FVector(6.f, 3640.f, 22.f), Cream);
	Add(CubeDeco, FVector(-1823.f, 200.f, 60.f), FVector(6.f, 3640.f, 120.f), TealDark);
	// lit windows
	for (float Y : { -1250.f, -700.f, 700.f, 1250.f, 1750.f })
	{
		Add(CubeDeco, FVector(-1824.f, Y, 250.f), FVector(4.f, 300.f, 200.f), Hex(0xFFD58A), 1.6f);
		Add(CubeDeco, FVector(-1826.f, Y, 250.f), FVector(4.f, 12.f, 200.f), Cream);
		Add(CubeDeco, FVector(-1826.f, Y, 250.f), FVector(4.f, 300.f, 12.f), Cream);
		Add(CubeDeco, FVector(-1827.f, Y, 145.f), FVector(10.f, 320.f, 14.f), Cream);
		Add(CubeDeco, FVector(-1824.f, Y, 420.f), FVector(4.f, 300.f, 110.f), Hex(0xFFD58A), 0.9f);
	}
	// art-deco marquee and canopy
	Add(BoxDeco, FVector(-1850.f, 0.f, 680.f), FVector(30.f, 980.f, 190.f), Coral, 0.3f);
	Add(CubeDeco, FVector(-1866.f, 0.f, 680.f), FVector(2.f, 940.f, 150.f), Navy);
	Text(TEXT("THE FINAL TAKE STUDIOS"), FVector(-1868.f, 0.f, 690.f), 180.f, 78.f, FColor(255, 222, 140));
	for (int32 i = 0; i < 16; ++i)
	{
		const float Y = -460.f + i * (920.f / 15.f);
		Add(SphereDeco, FVector(-1868.f, Y, 592.f), FVector(12.f), Yellow, 12.f);
		Add(SphereDeco, FVector(-1868.f, Y, 768.f), FVector(12.f), Yellow, 12.f);
	}
	for (float Y : { -560.f, 560.f })
	{
		Add(BoxDeco, FVector(-1850.f, Y, 700.f), FVector(40.f, 40.f, 420.f), Cream);
		Add(PrismDeco, FVector(-1850.f, Y, 930.f), FVector(40.f, 60.f, 60.f), Yellow, 1.f);
	}
	Add(BoxSolid, FVector(-1960.f, 0.f, 380.f), FVector(320.f, 760.f, 22.f), TealDark);
	for (int32 i = 0; i < 9; ++i)
	{
		Add(SphereDeco, FVector(-2118.f, -360.f + i * 90.f, 368.f), FVector(10.f), Amber, 10.f);
	}
	// entrance doors (glass slid open) + frame
	Add(GlassSolid, FVector(-1790.f, -330.f, 170.f), FVector(12.f, 140.f, 340.f), Glass);
	Add(GlassSolid, FVector(-1790.f, 330.f, 170.f), FVector(12.f, 140.f, 340.f), Glass);
	Add(BoxDeco, FVector(-1800.f, -262.f, 180.f), FVector(50.f, 16.f, 360.f), Brass);
	Add(BoxDeco, FVector(-1800.f, 262.f, 180.f), FVector(50.f, 16.f, 360.f), Brass);
	// red carpet, ropes
	Add(CubeDeco, FVector(-2030.f, 0.f, 5.f), FVector(460.f, 300.f, 2.f), Carpet);
	for (float X : { -2200.f, -2050.f, -1900.f })
	{
		for (float Y : { -190.f, 190.f })
		{
			Add(CylSolid, FVector(X, Y, 50.f), FVector(10.f, 10.f, 92.f), Brass);
			Add(SphereDeco, FVector(X, Y, 98.f), FVector(16.f), Brass, 0.5f);
		}
	}
	Add(CapsuleDeco, FVector(-2125.f, -190.f, 80.f), FVector(6.f, 6.f, 150.f), Carpet, 0.f, FRotator(-90.f, 0.f, 0.f));
	Add(CapsuleDeco, FVector(-2125.f, 190.f, 80.f), FVector(6.f, 6.f, 150.f), Carpet, 0.f, FRotator(-90.f, 0.f, 0.f));
	// "STAGE 4" sign on its stand
	Add(BoxSolid, FVector(-2080.f, -820.f, 90.f), FVector(20.f, 20.f, 180.f), Charcoal);
	Add(BoxSolid, FVector(-2080.f, -820.f, 230.f), FVector(20.f, 300.f, 140.f), Navy);
	Text(TEXT("STAGE 4"), FVector(-2092.f, -820.f, 256.f), 180.f, 58.f, FColor(255, 240, 220));
	Add(CubeDeco, FVector(-2092.f, -850.f, 196.f), FVector(2.f, 130.f, 18.f), Yellow, 0.8f);
	// head at the +Y end of the shaft, pointing towards the entrance (roll +90 turns the prism apex to +Y)
	Add(PrismDeco, FVector(-2092.f, -760.f, 196.f), FVector(2.f, 44.f, 40.f), Yellow, 0.8f, FRotator(0.f, 0.f, 90.f));
	Spot(FVector(-2250.f, -820.f, 380.f), FRotator(-35.f, 0.f, 0.f), FLinearColor(1.f, 0.8f, 0.55f), 12000.f, 800.f, 30.f, ZExterior);
	// equipment clutter outside
	FlightCase(FVector(-2120.f, -1200.f, 4.f), FVector(90.f, 70.f, 70.f), 10.f);
	FlightCase(FVector(-2120.f, -1200.f, 74.f), FVector(80.f, 60.f, 50.f), 25.f);
	FlightCase(FVector(-2050.f, 900.f, 4.f), FVector(110.f, 70.f, 80.f), -8.f);
	Cone(FVector(-2180.f, -500.f, 4.f));
	Cone(FVector(-2170.f, 560.f, 4.f));
	Add(TorusDeco, FVector(-2060.f, -1060.f, 12.f), FVector(80.f, 80.f, 24.f), Ink);
	Add(TorusDeco, FVector(-2060.f, -1060.f, 30.f), FVector(70.f, 70.f, 20.f), Ink);
	Plant(FVector(-1900.f, -420.f, 4.f), 1.2f);
	Plant(FVector(-1900.f, 420.f, 4.f), 1.2f);
	// street lamps
	for (float Y : { -1500.f, 500.f, 2100.f })
	{
		Add(CylSolid, FVector(-2200.f, Y, 210.f), FVector(14.f, 14.f, 420.f), Navy);
		Add(BoxDeco, FVector(-2240.f, Y, 420.f), FVector(90.f, 20.f, 12.f), Navy);
		Add(BoxDeco, FVector(-2280.f, Y, 405.f), FVector(50.f, 40.f, 26.f), Charcoal);
		Add(CubeDeco, FVector(-2280.f, Y, 390.f), FVector(40.f, 30.f, 4.f), Amber, 20.f);
		Point(FVector(-2280.f, Y, 360.f), FLinearColor(1.f, 0.72f, 0.4f), 9000.f, 1100.f, ZExterior, Y == 500.f);
	}
	// buildings across the street
	struct FB { float Y, W, H; FLinearColor C; };
	const FB Blocks[] = { { -1700.f, 700.f, 900.f, NavyLight }, { -900.f, 600.f, 1300.f, Hex(0x3A3F6E) }, { -150.f, 700.f, 800.f, Hex(0x4A3F72) },
		{ 700.f, 800.f, 1150.f, NavyLight }, { 1600.f, 700.f, 950.f, Hex(0x3A3F6E) }, { 2300.f, 600.f, 1400.f, Hex(0x2F355E) } };
	for (const FB& B : Blocks)
	{
		Wall(FVector(-3900.f, B.Y - B.W * 0.5f, -30.f), FVector(-3600.f, B.Y + B.W * 0.5f, B.H), B.C);
		for (float Z = 200.f; Z < B.H - 100.f; Z += 180.f)
		{
			for (float Wy = B.Y - B.W * 0.5f + 90.f; Wy < B.Y + B.W * 0.5f - 60.f; Wy += 140.f)
			{
				const bool bLit = FMath::Frac(FMath::Sin(Wy * 0.013f + Z * 0.021f) * 43758.5f) > 0.45f;
				Add(CubeDeco, FVector(-3598.f, Wy, Z), FVector(4.f, 70.f, 90.f), bLit ? Hex(0xFFD58A) : Hex(0x1B2140), bLit ? 1.2f : 0.f);
			}
		}
	}
	// invisible street bounds
	Add(Blocker, FVector(-3620.f, 200.f, 300.f), FVector(40.f, 4800.f, 600.f), White);
	Add(Blocker, FVector(-2700.f, -2210.f, 300.f), FVector(1900.f, 40.f, 600.f), White);
	Add(Blocker, FVector(-2700.f, 2610.f, 300.f), FVector(1900.f, 40.f, 600.f), White);
	// coral arrows from the kerb into the lobby
	Arrow(FVector(-2400.f, 0.f, 6.f), FVector(-1950.f, 0.f, 6.f), Coral, 3);
}

void AFTStudioShell::BuildLobby()
{
	// floor tiles + carpet
	Wall(FVector(-1800.f, -1000.f, -20.f), FVector(-600.f, 1000.f, 0.f), Cream);
	for (int32 ix = 0; ix < 6; ++ix)
	{
		for (int32 iy = 0; iy < 10; ++iy)
		{
			if ((ix + iy) % 2 == 0)
			{
				Add(CubeDeco, FVector(-1700.f + ix * 200.f, -900.f + iy * 200.f, 0.5f), FVector(198.f, 198.f, 1.f), CreamDark);
			}
		}
	}
	Add(CubeDeco, FVector(-1200.f, 0.f, 1.6f), FVector(1200.f, 300.f, 2.f), Carpet);
	Add(CubeDeco, FVector(-1200.f, -155.f, 1.8f), FVector(1200.f, 10.f, 2.f), Brass, 0.3f);
	Add(CubeDeco, FVector(-1200.f, 155.f, 1.8f), FVector(1200.f, 10.f, 2.f), Brass, 0.3f);
	// walls
	Wall(FVector(-1800.f, -1020.f, 0.f), FVector(-600.f, -980.f, 560.f), LobbyWall);
	Wall(FVector(-1800.f, 980.f, 0.f), FVector(-1350.f, 1020.f, 560.f), LobbyWall);
	Wall(FVector(-1050.f, 980.f, 0.f), FVector(-600.f, 1020.f, 560.f), LobbyWall);
	Wall(FVector(-1350.f, 980.f, 300.f), FVector(-1050.f, 1020.f, 560.f), LobbyWall);
	Add(CubeDeco, FVector(-1200.f, -977.f, 60.f), FVector(1200.f, 4.f, 120.f), Teal);
	Add(CubeDeco, FVector(-1575.f, 977.f, 60.f), FVector(450.f, 4.f, 120.f), Teal);
	Add(CubeDeco, FVector(-825.f, 977.f, 60.f), FVector(450.f, 4.f, 120.f), Teal);
	Wall(FVector(-1800.f, -1000.f, 560.f), FVector(-600.f, 1000.f, 580.f), Ceiling);
	// office door frame + sign
	Add(BoxDeco, FVector(-1365.f, 1000.f, 150.f), FVector(30.f, 60.f, 300.f), Coral);
	Add(BoxDeco, FVector(-1035.f, 1000.f, 150.f), FVector(30.f, 60.f, 300.f), Coral);
	Add(BoxDeco, FVector(-1200.f, 1000.f, 312.f), FVector(360.f, 60.f, 30.f), Coral);
	Add(BoxDeco, FVector(-1200.f, 960.f, 380.f), FVector(300.f, 10.f, 70.f), Navy);
	Text(TEXT("DIRECTOR'S OFFICE"), FVector(-1200.f, 953.f, 380.f), -90.f, 28.f, FColor(255, 222, 140));
	// reception desk
	Add(BoxSolid, FVector(-1300.f, -700.f, 55.f), FVector(300.f, 110.f, 110.f), Coral);
	Add(BoxDeco, FVector(-1300.f, -700.f, 114.f), FVector(320.f, 130.f, 10.f), Cream);
	Add(BoxDeco, FVector(-1260.f, -720.f, 135.f), FVector(40.f, 30.f, 30.f), Charcoal);
	Add(CubeDeco, FVector(-1300.f, -968.f, 300.f), FVector(4.f, 400.f, 90.f), Teal);
	Text(TEXT("RECEPTION"), FVector(-1300.f, -965.f, 300.f), 90.f, 40.f, FColor(255, 240, 220));
	// benches, plants, posters
	// waiting bench (the north-west corner is the loading bay, the north-east corner the Studio Supply counter)
	Add(BoxSolid, FVector(-1650.f, -470.f, 25.f), FVector(80.f, 260.f, 50.f), Teal);
	Add(BoxDeco, FVector(-1650.f, -470.f, 55.f), FVector(90.f, 270.f, 10.f), Coral);
	Plant(FVector(-1720.f, -900.f, 0.f), 1.1f);
	Plant(FVector(-700.f, -900.f, 0.f), 1.1f);
	Poster(FVector(-1100.f, -975.f, 260.f), 90.f, 0, 1.f);
	Poster(FVector(-800.f, -975.f, 260.f), 90.f, 1, 1.f);
	Poster(FVector(-1600.f, -975.f, 260.f), 90.f, 2, 1.f);
	// velvet ropes along the carpet
	for (float X : { -1500.f, -1150.f, -800.f })
	{
		for (float Y : { -200.f, 200.f })
		{
			Add(CylDeco, FVector(X, Y, 46.f), FVector(10.f, 10.f, 92.f), Brass);
			Add(SphereDeco, FVector(X, Y, 96.f), FVector(15.f), Brass, 0.4f);
		}
	}
	// STAGE 4 sign above the big door
	Add(BoxDeco, FVector(-625.f, 0.f, 470.f), FVector(12.f, 720.f, 130.f), Navy);
	Text(TEXT("STAGE 4"), FVector(-634.f, 0.f, 474.f), 180.f, 96.f, FColor(255, 214, 90));
	for (int32 i = 0; i < 12; ++i)
	{
		Add(SphereDeco, FVector(-634.f, -330.f + i * 60.f, 410.f), FVector(11.f), Yellow, 12.f);
		Add(SphereDeco, FVector(-634.f, -330.f + i * 60.f, 530.f), FVector(11.f), Yellow, 12.f);
	}
	// floor arrows: entrance -> office, carpet -> stage 4
	Arrow(FVector(-1650.f, 320.f, 2.2f), FVector(-1230.f, 880.f, 2.2f), Coral, 4);
	Arrow(FVector(-1450.f, 0.f, 3.f), FVector(-760.f, 0.f, 3.f), Yellow, 4);
	// lights
	Point(FVector(-1450.f, -450.f, 480.f), FLinearColor(1.f, 0.91f, 0.77f), 7400.f, 1300.f, ZLobby, true);
	Point(FVector(-950.f, 450.f, 480.f), FLinearColor(0.69f, 0.88f, 1.f), 6400.f, 1300.f, ZLobby);
	Point(FVector(-700.f, 0.f, 380.f), FLinearColor(1.f, 0.86f, 0.65f), 3600.f, 700.f, ZLobby);
	for (const FVector& L : { FVector(-1450.f, -450.f, 540.f), FVector(-950.f, 450.f, 540.f), FVector(-1450.f, 450.f, 540.f), FVector(-950.f, -450.f, 540.f) })
	{
		Add(CylDeco, L + FVector(0.f, 0.f, 10.f), FVector(4.f, 4.f, 30.f), Charcoal);
		Add(ConeDeco, L - FVector(0.f, 0.f, 16.f), FVector(70.f, 70.f, 36.f), Teal, 0.35f);
		Add(SphereDeco, L - FVector(0.f, 0.f, 36.f), FVector(26.f), Cream, 14.f);
	}
}

void AFTStudioShell::BuildOffice()
{
	Wall(FVector(-1800.f, 1000.f, -20.f), FVector(-600.f, 2000.f, 0.f), Wood);
	for (int32 i = 0; i < 10; ++i)
	{
		Add(CubeDeco, FVector(-1200.f, 1050.f + i * 100.f, 0.5f), FVector(1200.f, 3.f, 1.f), WoodDark);
	}
	Add(CubeDeco, FVector(-1150.f, 1500.f, 1.2f), FVector(500.f, 600.f, 1.f), Magenta);
	Add(CubeDeco, FVector(-1150.f, 1500.f, 1.4f), FVector(460.f, 560.f, 1.f), Coral);
	Wall(FVector(-1800.f, 1980.f, 0.f), FVector(-600.f, 2020.f, 560.f), Teal);
	Wall(FVector(-1800.f, 1000.f, 460.f), FVector(-600.f, 2000.f, 480.f), Ceiling);
	Add(CubeDeco, FVector(-1200.f, 1977.f, 50.f), FVector(1200.f, 4.f, 100.f), TealDark);
	// desk + chair
	Add(BoxSolid, FVector(-1050.f, 1500.f, 40.f), FVector(150.f, 300.f, 80.f), Wood);
	Add(BoxDeco, FVector(-1050.f, 1500.f, 83.f), FVector(166.f, 316.f, 6.f), WoodDark);
	Add(CylDeco, FVector(-1080.f, 1390.f, 100.f), FVector(20.f, 20.f, 28.f), Cream);
	Text(TEXT("GOOD IDEAS LATER"), FVector(-1080.f, 1379.f, 100.f), -90.f, 3.f, FColor(40, 40, 60));
	Add(BoxDeco, FVector(-1020.f, 1640.f, 110.f), FVector(50.f, 16.f, 40.f), Ink, 0.f, FRotator(0.f, 20.f, 0.f));
	Add(CubeDeco, FVector(-1020.f, 1640.f, 132.f), FVector(52.f, 18.f, 8.f), White, 0.f, FRotator(0.f, 20.f, 8.f));
	// lamp
	Add(CylDeco, FVector(-1080.f, 1620.f, 90.f), FVector(22.f, 22.f, 6.f), Yellow);
	Add(CylDeco, FVector(-1080.f, 1620.f, 120.f), FVector(5.f, 5.f, 60.f), Yellow, 0.f, FRotator(15.f, 0.f, 0.f));
	Add(ConeDeco, FVector(-1070.f, 1610.f, 150.f), FVector(34.f, 34.f, 26.f), Yellow, 1.f, FRotator(160.f, 0.f, 0.f));
	Point(FVector(-1060.f, 1600.f, 130.f), FLinearColor(1.f, 0.8f, 0.5f), 1800.f, 400.f, ZLobby);
	// director's chair
	for (float Y : { 1680.f, 1760.f })
	{
		Add(BoxDeco, FVector(-1050.f, Y, 45.f), FVector(8.f, 8.f, 90.f), Wood, 0.f, FRotator(0.f, 0.f, Y < 1700.f ? 15.f : -15.f));
	}
	Add(CubeDeco, FVector(-1050.f, 1720.f, 60.f), FVector(60.f, 70.f, 4.f), Navy);
	Add(CubeDeco, FVector(-1050.f, 1760.f, 110.f), FVector(60.f, 4.f, 40.f), Navy);
	Text(TEXT("DIRECTOR"), FVector(-1050.f, 1763.f, 110.f), 90.f, 10.f, FColor(245, 235, 210));
	// bookshelf, couch, plants, posters, window frame
	Add(BoxSolid, FVector(-1600.f, 1950.f, 110.f), FVector(240.f, 50.f, 220.f), WoodDark);
	for (int32 r = 0; r < 3; ++r)
	{
		for (int32 b = 0; b < 7; ++b)
		{
			const FLinearColor BC = b % 3 == 0 ? Coral : (b % 3 == 1 ? Teal : Yellow);
			Add(CubeDeco, FVector(-1700.f + b * 30.f, 1925.f, 60.f + r * 70.f), FVector(24.f, 6.f, 50.f), BC);
		}
	}
	Add(BoxSolid, FVector(-1650.f, 1300.f, 35.f), FVector(100.f, 260.f, 70.f), Coral);
	Add(BoxDeco, FVector(-1700.f, 1300.f, 80.f), FVector(20.f, 260.f, 70.f), CoralDark);
	Add(BoxDeco, FVector(-1640.f, 1240.f, 80.f), FVector(20.f, 50.f, 40.f), Cream, 0.f, FRotator(0.f, 0.f, 10.f));
	Plant(FVector(-750.f, 1900.f, 0.f), 1.f);
	Plant(FVector(-1740.f, 1060.f, 0.f), 0.9f);
	Poster(FVector(-1775.f, 1450.f, 250.f), 0.f, 0, 0.9f);
	Poster(FVector(-1775.f, 1700.f, 250.f), 0.f, 1, 0.9f);
	Poster(FVector(-1775.f, 1200.f, 250.f), 0.f, 2, 0.9f);
	Add(BoxDeco, FVector(-620.f, 1500.f, 225.f), FVector(30.f, 640.f, 20.f), Cream);
	Add(BoxDeco, FVector(-620.f, 1500.f, 355.f), FVector(30.f, 640.f, 20.f), Cream);
	Point(FVector(-1200.f, 1500.f, 400.f), FLinearColor(0.8f, 0.91f, 1.f), 6000.f, 1100.f, ZLobby, true);
}

void AFTStudioShell::BuildWardrobe()
{
	Wall(FVector(-1800.f, -1600.f, -20.f), FVector(-600.f, -1000.f, 0.f), Cream);
	Add(CubeDeco, FVector(-1200.f, -1300.f, 1.f), FVector(900.f, 360.f, 1.f), Magenta);
	Wall(FVector(-1800.f, -1620.f, 0.f), FVector(-600.f, -1580.f, 560.f), Hex(0xF2C6DD));
	Wall(FVector(-1800.f, -1600.f, 460.f), FVector(-600.f, -1000.f, 480.f), Ceiling);
	Add(CubeDeco, FVector(-1200.f, -1577.f, 60.f), FVector(1200.f, 4.f, 120.f), Magenta);
	Add(CubeDeco, FVector(-1200.f, -1023.f, 60.f), FVector(1200.f, 4.f, 120.f), Magenta);
	// the makeup mirror + accessory wall is an actor (AFTAccessoryStand); keep its warm vanity light
	Point(FVector(-1190.f, -1420.f, 250.f), FLinearColor(1.f, 0.85f, 0.7f), 3400.f, 650.f, ZLobby);
	// lockers
	for (int32 i = 0; i < 5; ++i)
	{
		Add(BoxSolid, FVector(-900.f + i * 60.f, -1560.f, 100.f), FVector(56.f, 60.f, 200.f), i % 2 ? Teal : Coral);
		Add(CubeDeco, FVector(-900.f + i * 60.f, -1529.f, 150.f), FVector(20.f, 2.f, 4.f), Charcoal);
		Add(BoxDeco, FVector(-917.f + i * 60.f, -1527.f, 93.f), FVector(5.f, 5.f, 18.f), CreamDark, 0.f, FRotator::ZeroRotator, 0.6f);
		for (int32 Vent = 0; Vent < 3; ++Vent)
		{
			Add(CubeDeco, FVector(-900.f + i * 60.f, -1529.f, 163.f + Vent * 7.f), FVector(23.f, 2.f, 2.f), TealDark);
		}
	}
	Add(BoxSolid, FVector(-1400.f, -1150.f, 25.f), FVector(240.f, 60.f, 50.f), Wood);
	Point(FVector(-1300.f, -1250.f, 400.f), FLinearColor(1.f, 0.92f, 0.84f), 6000.f, 1100.f, ZLobby, true);
	Plant(FVector(-700.f, -1100.f, 0.f), 0.9f);
}

void AFTStudioShell::BuildStage()
{
	// floor, walls, ceiling of Stage 4 (floor at -120)
	Wall(FVector(-600.f, -1600.f, -160.f), FVector(2400.f, 2000.f, -120.f), StageFloor);
	Wall(FVector(-600.f, -1640.f, -120.f), FVector(2400.f, -1600.f, 1320.f), StageWall);
	Wall(FVector(-600.f, 2000.f, -120.f), FVector(2400.f, 2040.f, 1320.f), StageWall);
	Wall(FVector(2400.f, -1640.f, -120.f), FVector(2440.f, 2040.f, 1320.f), StageWall);
	Wall(FVector(-600.f, -1600.f, 1320.f), FVector(2400.f, 2000.f, 1360.f), Hex(0x161A33));
	for (float Y : { -1596.f, 1996.f })
	{
		Add(CubeDeco, FVector(900.f, Y, -20.f), FVector(3000.f, 4.f, 200.f), StageWallLow);
		Add(CubeDeco, FVector(900.f, Y, 82.f), FVector(3000.f, 4.f, 8.f), Tape, 0.3f);
	}
	// south wall (shared with lobby/office/wardrobe) with Stage 4 door, wardrobe door and office window
	const float WX0 = -620.f, WX1 = -580.f;
	Wall(FVector(WX0, -1600.f, -120.f), FVector(WX1, -1450.f, 1320.f), StageWall);
	Wall(FVector(WX0, -1450.f, -120.f), FVector(WX1, -1200.f, 0.f), StageWall);
	Wall(FVector(WX0, -1450.f, 300.f), FVector(WX1, -1200.f, 1320.f), StageWall);
	Wall(FVector(WX0, -1200.f, -120.f), FVector(WX1, -300.f, 1320.f), StageWall);
	Wall(FVector(WX0, -300.f, -120.f), FVector(WX1, 300.f, 0.f), StageWall);
	Wall(FVector(WX0, -300.f, 400.f), FVector(WX1, 300.f, 1320.f), StageWall);
	Wall(FVector(WX0, 300.f, -120.f), FVector(WX1, 1200.f, 1320.f), StageWall);
	Wall(FVector(WX0, 1200.f, -120.f), FVector(WX1, 1800.f, 100.f), StageWall);
	Wall(FVector(WX0, 1200.f, 350.f), FVector(WX1, 1800.f, 1320.f), StageWall);
	Wall(FVector(WX0, 1800.f, -120.f), FVector(WX1, 2000.f, 1320.f), StageWall);
	Add(GlassSolid, FVector(-600.f, 1500.f, 225.f), FVector(16.f, 600.f, 250.f), Glass);
	// front-of-house cladding: the navy stage wall must not show from the lobby, office or wardrobe
	auto Clad = [this](float Y0, float Y1, float Z0, float Z1, const FLinearColor& C)
	{
		Add(CubeDeco, FVector(-621.5f, (Y0 + Y1) * 0.5f, (Z0 + Z1) * 0.5f), FVector(3.f, Y1 - Y0, Z1 - Z0), C);
	};
	const FLinearColor WardrobeWall = Hex(0xF2C6DD);
	// lobby (Stage 4 door opening Y -300..300, Z 0..400)
	Clad(-980.f, -300.f, 0.f, 560.f, LobbyWall);
	Clad(300.f, 980.f, 0.f, 560.f, LobbyWall);
	Clad(-300.f, 300.f, 400.f, 560.f, LobbyWall);
	for (float Y : { -640.f, 640.f })
	{
		Add(CubeDeco, FVector(-624.f, Y, 60.f), FVector(4.f, 680.f, 120.f), Teal);
	}
	// office (window Y 1200..1800, Z 100..350)
	Clad(1020.f, 1980.f, 0.f, 100.f, TealDark);
	Clad(1020.f, 1980.f, 350.f, 460.f, LobbyWall);
	Clad(1020.f, 1200.f, 100.f, 350.f, LobbyWall);
	Clad(1800.f, 1980.f, 100.f, 350.f, LobbyWall);
	// wardrobe (door opening Y -1450..-1200, Z 0..300)
	Clad(-1580.f, -1450.f, 0.f, 460.f, WardrobeWall);
	Clad(-1200.f, -1020.f, 0.f, 460.f, WardrobeWall);
	Clad(-1450.f, -1200.f, 300.f, 460.f, WardrobeWall);
	Add(CubeDeco, FVector(-624.f, -1515.f, 60.f), FVector(4.f, 130.f, 120.f), Magenta);
	Add(CubeDeco, FVector(-624.f, -1110.f, 60.f), FVector(4.f, 180.f, 120.f), Magenta);
	// stage-4 landing + stairs down, wardrobe landing + stairs
	Wall(FVector(-580.f, -400.f, -120.f), FVector(-300.f, 800.f, 0.f), TealDark);
	Add(CubeDeco, FVector(-302.f, 200.f, 0.5f), FVector(6.f, 1200.f, 1.f), Tape, 0.3f);
	Stairs(FVector(-300.f, 0.f, 0.f), 60.f, 600.f, -20.f, 5, true, TealDark, Tape);
	Wall(FVector(-580.f, -1450.f, -120.f), FVector(-450.f, -1200.f, 0.f), Hex(0x6B3F7A));
	Stairs(FVector(-450.f, -1325.f, 0.f), 60.f, 250.f, -20.f, 5, true, Hex(0x6B3F7A), Tape);
	// landing railings
	for (const FVector& P : { FVector(-300.f, -380.f, 50.f), FVector(-300.f, -320.f, 50.f), FVector(-300.f, 320.f, 50.f), FVector(-300.f, 780.f, 50.f) })
	{
		Add(CylDeco, P, FVector(6.f, 6.f, 100.f), Yellow);
	}
	Add(CylDeco, FVector(-300.f, 550.f, 100.f), FVector(6.f, 6.f, 460.f), Yellow, 0.f, FRotator(0.f, 0.f, 90.f));
	// SOUNDSTAGE 4 lettering + banners
	Add(BoxDeco, FVector(-560.f, 700.f, 700.f), FVector(20.f, 900.f, 180.f), Yellow);
	Text(TEXT("SOUNDSTAGE 4"), FVector(-548.f, 700.f, 700.f), 0.f, 110.f, FColor(27, 33, 64));
	Add(CubeDeco, FVector(1250.f, -1592.f, 380.f), FVector(360.f, 4.f, 260.f), Cream);
	Add(CubeDeco, FVector(1250.f, -1590.f, 280.f), FVector(360.f, 4.f, 60.f), Blue);
	Text(TEXT("HIGHER TO"), FVector(1250.f, -1586.f, 440.f), 90.f, 48.f, FColor(27, 33, 64));
	Text(TEXT("SAFETY!"), FVector(1250.f, -1586.f, 385.f), 90.f, 48.f, FColor(27, 33, 64));
	Add(PrismDeco, FVector(1250.f, -1587.f, 330.f), FVector(2.f, 70.f, 40.f), Navy, 0.f, FRotator(0.f, 90.f, 0.f) + FRotator(0.f, -90.f, 0.f));
	// floor tape: safe walkways
	Add(CubeDeco, FVector(900.f, -1350.f, -119.4f), FVector(2800.f, 10.f, 1.f), Tape, 0.3f);
	Add(CubeDeco, FVector(900.f, 1750.f, -119.4f), FVector(2800.f, 10.f, 1.f), Tape, 0.3f);
	Add(CubeDeco, FVector(1150.f, 200.f, -119.4f), FVector(10.f, 2800.f, 1.f), Tape, 0.3f);
	// camera deck + steps + labels
	Wall(FVector(300.f, -800.f, -120.f), FVector(600.f, 800.f, -60.f), Hex(0x2B5F66));
	Add(CubeDeco, FVector(450.f, -800.f, -59.4f), FVector(300.f, 8.f, 1.f), Tape, 0.3f);
	Add(CubeDeco, FVector(450.f, 800.f, -59.4f), FVector(300.f, 8.f, 1.f), Tape, 0.3f);
	Add(CubeDeco, FVector(600.f, 0.f, -59.4f), FVector(8.f, 1600.f, 1.f), Tape, 0.3f);
	Stairs(FVector(180.f, 0.f, -120.f), 60.f, 400.f, 20.f, 2, true, Hex(0x2B5F66), Tape);
	Add(BoxDeco, FVector(305.f, -500.f, -20.f), FVector(10.f, 200.f, 60.f), Teal);
	Text(TEXT("CAMERA"), FVector(299.f, -500.f, -20.f), 180.f, 30.f, FColor::White);
	// arrows on the stage floor
	Arrow(FVector(80.f, 0.f, -119.f), FVector(150.f, 0.f, -119.f), Cyan, 2);
	Arrow(FVector(0.f, -600.f, -119.f), FVector(-250.f, -1100.f, -119.f), Magenta, 3);
	Arrow(FVector(700.f, -1480.f, -119.f), FVector(1200.f, -1480.f, -119.f), Magenta, 3);
	Arrow(FVector(1200.f, 1250.f, -119.f), FVector(1400.f, 1600.f, -119.f), Yellow, 3);
	// sound booth
	Add(CubeDeco, FVector(150.f, -1325.f, -119.3f), FVector(480.f, 480.f, 1.f), Purple);
	Wall(FVector(390.f, -1560.f, -120.f), FVector(410.f, -1150.f, -10.f), Hex(0x5B3F8C));
	Wall(FVector(-100.f, -1060.f, -120.f), FVector(260.f, -1040.f, -10.f), Hex(0x5B3F8C));
	// booth front frame: posts up to the balcony slab, header beam with neon strip and the sign mounted on it
	const FLinearColor BoothFrame = Hex(0x5B3F8C);
	Add(BoxSolid, FVector(400.f, -1552.f, 135.f), FVector(26.f, 26.f, 510.f), BoothFrame);
	Add(BoxSolid, FVector(400.f, -1158.f, 135.f), FVector(26.f, 26.f, 510.f), BoothFrame);
	Add(BoxDeco, FVector(400.f, -1355.f, 355.f), FVector(30.f, 420.f, 70.f), BoothFrame);
	Add(BoxDeco, FVector(416.f, -1355.f, 326.f), FVector(4.f, 400.f, 8.f), Magenta, 0.8f);
	Add(BoxDeco, FVector(418.f, -1355.f, 360.f), FVector(6.f, 200.f, 46.f), Navy);
	Text(TEXT("SOUND"), FVector(422.f, -1355.f, 360.f), 0.f, 38.f, FColor(255, 120, 200));
	Point(FVector(150.f, -1300.f, 250.f), FLinearColor(0.9f, 0.5f, 1.f), 4000.f, 700.f, ZStage);
	// effects corner signage
	Add(CubeDeco, FVector(900.f, 1994.f, 260.f), FVector(500.f, 4.f, 90.f), Coral);
	Text(TEXT("PRACTICAL EFFECTS"), FVector(900.f, 1990.f, 260.f), -90.f, 44.f, FColor::White);
	for (int32 i = 0; i < 4; ++i)
	{
		Add(TorusDeco, FVector(700.f + i * 30.f, 1600.f, -114.f), FVector(80.f, 80.f, 12.f), Ink);
	}
	// lighting grid and work lights
	for (float Y : { -900.f, 200.f, 1300.f })
	{
		Truss(Y, 1040.f);
	}
	const FVector Work[] = { FVector(100.f, -900.f, 980.f), FVector(900.f, -900.f, 980.f), FVector(1700.f, -900.f, 980.f),
		FVector(100.f, 1300.f, 980.f), FVector(900.f, 1300.f, 980.f), FVector(1700.f, 1300.f, 980.f),
		FVector(500.f, 200.f, 980.f), FVector(1300.f, 200.f, 980.f), FVector(2000.f, 200.f, 980.f) };
	int32 w = 0;
	for (const FVector& L : Work)
	{
		Add(BoxDeco, L + FVector(0.f, 0.f, 20.f), FVector(50.f, 50.f, 40.f), Charcoal);
		Add(CylDeco, L, FVector(40.f, 40.f, 6.f), Cream, 10.f);
		Point(L - FVector(0.f, 0.f, 40.f), (w % 3 == 1) ? FLinearColor(0.75f, 0.85f, 1.f) : FLinearColor(1.f, 0.86f, 0.7f), 26000.f, 2600.f, ZStage, w == 7);
		++w;
	}
	// clutter
	// (the floor between the landing and the lighting desk is the PROP DEPOT delivery bay)
	FlightCase(FVector(-480.f, 1080.f, -120.f), FVector(100.f, 70.f, 80.f), 12.f);
	FlightCase(FVector(-480.f, 1080.f, -40.f), FVector(90.f, 60.f, 50.f), -5.f);
	FlightCase(FVector(-490.f, 1640.f, -120.f), FVector(120.f, 70.f, 90.f), 80.f);
	Cone(FVector(250.f, -950.f, -120.f));
	Cone(FVector(1180.f, -1000.f, -120.f));
	Cone(FVector(1180.f, 1150.f, -120.f));
	Crate(FVector(-200.f, -500.f, -120.f), 80.f, 15.f);
	Crate(FVector(-120.f, -560.f, -120.f), 60.f, 40.f);
	Plant(FVector(-450.f, 1850.f, -120.f), 1.2f);
	// ladder on the west wall
	for (int32 i = 0; i < 12; ++i)
	{
		Add(CubeDeco, FVector(2150.f, -1580.f, -80.f + i * 35.f), FVector(50.f, 6.f, 6.f), Yellow);
	}
	Add(CubeDeco, FVector(2125.f, -1580.f, 110.f), FVector(6.f, 6.f, 420.f), Yellow);
	Add(CubeDeco, FVector(2175.f, -1580.f, 110.f), FVector(6.f, 6.f, 420.f), Yellow);
}

void AFTStudioShell::BuildTankSet()
{
	// tank walls (top at -20) + white coping
	Wall(FVector(1300.f, -900.f, -120.f), FVector(1340.f, 1100.f, -20.f), TankBlue);
	Wall(FVector(2260.f, -900.f, -120.f), FVector(2300.f, 1100.f, -20.f), TankBlue);
	Wall(FVector(1340.f, -900.f, -120.f), FVector(2260.f, -860.f, -20.f), TankBlue);
	Wall(FVector(1340.f, 1060.f, -120.f), FVector(2260.f, 1100.f, -20.f), TankBlue);
	Add(BoxDeco, FVector(1320.f, 100.f, -18.f), FVector(52.f, 2012.f, 8.f), White);
	Add(BoxDeco, FVector(2280.f, 100.f, -18.f), FVector(52.f, 2012.f, 8.f), White);
	Add(BoxDeco, FVector(1800.f, -880.f, -18.f), FVector(1000.f, 52.f, 8.f), White);
	Add(BoxDeco, FVector(1800.f, 1080.f, -18.f), FVector(1000.f, 52.f, 8.f), White);
	for (float Y = -800.f; Y < 1100.f; Y += 200.f)
	{
		Add(CubeDeco, FVector(1298.f, Y, -70.f), FVector(4.f, 8.f, 96.f), Hex(0x2F66BF));
	}
	Text(TEXT("TANK A  -  NO DIVING"), FVector(1297.f, 100.f, -70.f), 180.f, 22.f, FColor::White);
	// island (beach) with sand, rocks and palms
	Wall(FVector(1860.f, -870.f, -120.f), FVector(2260.f, 30.f, -10.f), Sand);
	Add(RampSolid, FVector(1800.f, -420.f, -65.f), FVector(900.f, 120.f, 110.f), Sand, 0.f, FRotator(0.f, 90.f, 0.f));
	Add(BallDeco, FVector(1990.f, 40.f, -30.f), FVector(160.f, 120.f, 60.f), Sand);
	// A single authored surface blends into the existing ramp; no new collision or navigation.
	Add(ShorelineDeco, FVector(1840.f, -420.f, -9.5f), FVector(100.f, 900.f, 100.f), Sand);
	Add(BoxSolid, FVector(2180.f, -800.f, 20.f), FVector(110.f, 90.f, 70.f), Hex(0x8E8AA3), 0.f, FRotator(0.f, 20.f, 8.f));
	Add(SphereDeco, FVector(2230.f, -200.f, 10.f), FVector(90.f, 110.f, 90.f), Hex(0xA29B8B), 0.f, FRotator(10.f, 35.f, 0.f));
	Add(SphereDeco, FVector(1920.f, -650.f, -2.f), FVector(60.f, 60.f, 30.f), Hex(0xA29B8B), 0.f, FRotator(0.f, 50.f, 12.f));
	Add(SphereDeco, FVector(1560.f, 600.f, -46.f), FVector(80.f, 70.f, 40.f), Hex(0x8E8AA3), 0.f, FRotator(8.f, 20.f, 0.f));
	Palm(FVector(2200.f, -40.f, -10.f), 300.f);
	Palm(FVector(2120.f, -560.f, -10.f), 240.f);
	Add(CubeDeco, FVector(1960.f, -300.f, -9.3f), FVector(160.f, 90.f, 1.f), Coral);
	Add(CubeDeco, FVector(1960.f, -300.f, -9.1f), FVector(160.f, 20.f, 1.f), White);
	Add(CylDeco, FVector(1900.f, -150.f, 20.f), FVector(4.f, 4.f, 60.f), Cream);
	Add(ConeDeco, FVector(1900.f, -150.f, 60.f), FVector(90.f, 90.f, 30.f), Magenta);
	// ramp up to the island over the west tank wall
	Add(RampSolid, FVector(1970.f, -1125.f, -65.f), FVector(160.f, 450.f, 110.f), WoodDark, 0.f, FRotator(0.f, 180.f, 0.f));
	for (int32 i = 0; i < 6; ++i)
	{
		Add(CubeDeco, FVector(1970.f, -1330.f + i * 75.f, -112.f + i * 18.5f), FVector(164.f, 8.f, 3.f), Wood);
	}
	// dock inside the tank + ramp from the east
	Wall(FVector(1500.f, 720.f, -50.f), FVector(2150.f, 1060.f, -30.f), Wood);
	for (float X = 1520.f; X < 2150.f; X += 60.f)
	{
		Add(CubeDeco, FVector(X, 890.f, -29.5f), FVector(4.f, 340.f, 1.f), WoodDark);
	}
	for (const FVector& P : { FVector(1510.f, 730.f, -70.f), FVector(1510.f, 1050.f, -70.f), FVector(1830.f, 730.f, -70.f), FVector(2140.f, 730.f, -70.f) })
	{
		Add(CylSolid, P, FVector(22.f, 22.f, 120.f), WoodDark);
	}
	Add(RampSolid, FVector(1825.f, 1300.f, -75.f), FVector(220.f, 400.f, 90.f), Wood);
	Add(CubeDeco, FVector(1560.f, 880.f, -29.f), FVector(90.f, 90.f, 1.f), Tape, 0.4f, FRotator(0.f, 45.f, 0.f));
	Add(CubeDeco, FVector(1560.f, 880.f, -28.9f), FVector(90.f, 90.f, 1.f), Tape, 0.4f, FRotator(0.f, -45.f, 0.f));
	Text(TEXT("HERO MARK"), FVector(1640.f, 880.f, -28.f), 180.f, 22.f, FColor(255, 214, 90), 90.f);
	// painted sky backdrop, clouds, sun, sea
	Add(CubeDeco, FVector(2390.f, 200.f, 260.f), FVector(10.f, 3560.f, 760.f), Backdrop, 0.25f);
	Add(CubeDeco, FVector(2384.f, 200.f, -40.f), FVector(4.f, 3560.f, 140.f), Blue, 0.2f);
	for (int32 i = 0; i < 12; ++i)
	{
		Add(CubeDeco, FVector(2382.f, -1600.f + i * 310.f, 30.f + (i % 2) * 8.f), FVector(4.f, 160.f, 16.f), White, 0.3f, FRotator(0.f, 0.f, i % 2 ? 8.f : -8.f));
	}
	const FVector Clouds[] = { FVector(2380.f, -1100.f, 460.f), FVector(2380.f, -300.f, 540.f), FVector(2380.f, 700.f, 430.f), FVector(2380.f, 1500.f, 520.f) };
	for (const FVector& C : Clouds)
	{
		Add(BallDeco, C, FVector(6.f, 260.f, 90.f), White, 0.4f);
		Add(BallDeco, C + FVector(-1.f, 90.f, 30.f), FVector(6.f, 170.f, 100.f), White, 0.4f);
		Add(BallDeco, C + FVector(-1.f, -80.f, 20.f), FVector(6.f, 150.f, 80.f), White, 0.4f);
	}
	Add(CylDeco, FVector(2382.f, 1100.f, 300.f), FVector(160.f, 160.f, 4.f), Yellow, 1.2f, FRotator(90.f, 0.f, 0.f));
	for (const FVector& I : { FVector(2382.f, -700.f, 50.f), FVector(2382.f, 1500.f, 40.f) })
	{
		Add(PrismDeco, I, FVector(2.f, 600.f, 120.f), TealDark, 0.15f, FRotator(0.f, 0.f, 0.f));
	}
	// SHARK! banner
	Add(CubeDeco, FVector(2375.f, -1250.f, 560.f), FVector(4.f, 500.f, 150.f), Coral, 0.4f);
	Text(TEXT("SHARK!"), FVector(2371.f, -1250.f, 560.f), 180.f, 110.f, FColor::White);
	// spot on the island, practical sun bounce
	Spot(FVector(1700.f, -500.f, 900.f), FRotator(-60.f, 0.f, 0.f), FLinearColor(1.f, 0.85f, 0.6f), 30000.f, 2000.f, 30.f, ZStage);
	Point(FVector(1800.f, 100.f, 150.f), FLinearColor(0.5f, 0.8f, 1.f), 8000.f, 1300.f, ZStage);
}

void AFTStudioShell::BuildUpperLevel()
{
	// long stair along the west wall up to the projection level (Z 420)
	Stairs(FVector(1300.f, -1480.f, -120.f), -33.3f, 230.f, 20.f, 27, true, Hex(0x3A4570), Tape);
	Add(CubeDeco, FVector(850.f, -1360.f, 170.f), FVector(900.f, 6.f, 6.f), Yellow, 0.f, FRotator(-31.f, 0.f, 0.f));
	for (int32 i = 0; i < 6; ++i)
	{
		const float X = 1250.f - i * 170.f;
		const float Z = -120.f + (1300.f - X) * 0.6f;
		Add(CylDeco, FVector(X, -1360.f, Z + 50.f), FVector(6.f, 6.f, 100.f), Yellow);
	}
	Add(Blocker, FVector(850.f, -1358.f, 200.f), FVector(900.f, 6.f, 60.f), White, 0.f, FRotator(-31.f, 0.f, 0.f));
	// landing at the top + projection room
	Wall(FVector(250.f, -1600.f, 390.f), FVector(420.f, -1360.f, 420.f), Hex(0x3A4570));
	Wall(FVector(-600.f, -1600.f, 390.f), FVector(250.f, -900.f, 420.f), Hex(0x4A3A2E));
	Add(CubeDeco, FVector(-175.f, -1250.f, 420.6f), FVector(820.f, 660.f, 1.f), Carpet);
	Wall(FVector(-600.f, -1600.f, 740.f), FVector(270.f, -900.f, 760.f), Hex(0x2B2140));
	Wall(FVector(-600.f, -920.f, 420.f), FVector(250.f, -900.f, 740.f), Hex(0x3B2E5E));
	Add(GlassSolid, FVector(-100.f, -905.f, 600.f), FVector(400.f, 10.f, 180.f), Glass);
	Wall(FVector(240.f, -1600.f, 420.f), FVector(260.f, -1560.f, 740.f), Hex(0x3B2E5E));
	Wall(FVector(240.f, -1360.f, 420.f), FVector(260.f, -1330.f, 740.f), Hex(0x3B2E5E));
	Wall(FVector(240.f, -1560.f, 640.f), FVector(260.f, -1360.f, 740.f), Hex(0x3B2E5E));
	// balcony railing towards the screen
	Wall(FVector(245.f, -1330.f, 420.f), FVector(255.f, -900.f, 520.f), Hex(0x3B2E5E));
	Add(BoxDeco, FVector(250.f, -1115.f, 525.f), FVector(20.f, 430.f, 10.f), Brass);
	// cinema seats
	for (int32 r = 0; r < 2; ++r)
	{
		for (int32 s = 0; s < 4; ++s)
		{
			const FVector Seat(-300.f - r * 150.f, -1450.f + s * 120.f, 420.f);
			Add(BoxSolid, Seat + FVector(0.f, 0.f, 25.f), FVector(70.f, 90.f, 50.f), Carpet);
			Add(BoxDeco, Seat + FVector(-38.f, 0.f, 70.f), FVector(16.f, 90.f, 70.f), CoralDark);
			Add(BoxDeco, Seat + FVector(0.f, 0.f, 52.f), FVector(65.f, 76.f, 12.f), Coral);
			Add(BoxDeco, Seat + FVector(-27.f, 0.f, 80.f), FVector(12.f, 74.f, 42.f), Carpet);
			for (float Side : { -1.f, 1.f })
			{
				Add(BoxDeco, Seat + FVector(0.f, Side * 45.f, 61.f), FVector(72.f, 10.f, 10.f), Navy);
				Add(TorusDeco, Seat + FVector(23.f, Side * 45.f, 67.f), FVector(10.f, 10.f, 3.f), Brass, 0.f, FRotator::ZeroRotator, 0.6f);
			}
		}
	}
	Point(FVector(-200.f, -1250.f, 680.f), FLinearColor(1.f, 0.78f, 0.5f), 4000.f, 900.f, ZProjection);
	Poster(FVector(-590.f, -1250.f, 580.f), 0.f, 0, 0.8f);
	// "PROJECTION" hint at the foot of the stairs
	Add(BoxDeco, FVector(1350.f, -1580.f, 120.f), FVector(10.f, 200.f, 70.f), Navy);
	Text(TEXT("PROJECTION  ^"), FVector(1350.f, -1573.f, 120.f), 90.f, 30.f, FColor(255, 214, 90));
}

void AFTStudioShell::BuildWarehouse()
{
	Add(CubeDeco, FVector(1880.f, 1750.f, -119.3f), FVector(960.f, 480.f, 1.f), Hex(0x4B5163));
	for (int32 u = 0; u < 3; ++u)
	{
		const float X = 1600.f + u * 260.f;
		Add(BoxSolid, FVector(X, 1945.f, 80.f), FVector(220.f, 60.f, 400.f), WoodDark);
		for (int32 s = 0; s < 3; ++s)
		{
			Add(BoxDeco, FVector(X, 1910.f, -40.f + s * 110.f), FVector(210.f, 30.f, 8.f), Wood);
			const FVector Prop(X - 50.f + s * 40.f, 1888.f, -10.f + s * 110.f);
			switch ((u + s) % 3)
			{
			case 0: // compact lens case with latches
				Add(BoxDeco, Prop, FVector(66.f, 44.f, 44.f), TealDark);
				Add(BoxDeco, Prop + FVector(0.f, 0.f, 9.f), FVector(68.f, 46.f, 4.f), Grey, 0.f, FRotator::ZeroRotator, 0.7f);
				for (float Side : { -1.f, 1.f })
				{
					Add(BoxDeco, Prop + FVector(Side * 21.f, -24.f, 9.f), FVector(8.f, 4.f, 13.f), CreamDark, 0.f, FRotator::ZeroRotator, 0.7f);
				}
				break;
			case 1: // film can and reel on a small stand
				Add(BoxDeco, Prop + FVector(0.f, 0.f, -20.f), FVector(64.f, 40.f, 7.f), Navy);
				Add(TorusDeco, Prop + FVector(0.f, -5.f, 7.f), FVector(55.f, 55.f, 12.f), CreamDark, 0.f, FRotator(0.f, 0.f, 90.f), 0.7f);
				Add(CylDeco, Prop + FVector(0.f, -5.f, 7.f), FVector(13.f, 13.f, 16.f), Teal, 0.f, FRotator(0.f, 0.f, 90.f), 0.4f);
				for (int32 Spoke = 0; Spoke < 3; ++Spoke)
				{
					Add(BoxDeco, Prop + FVector(0.f, -5.f, 7.f), FVector(43.f, 5.f, 5.f), Grey, 0.f, FRotator(Spoke * 60.f, 0.f, 0.f), 0.65f);
				}
				break;
			default: // folded sound blankets and a coiled cable
				for (int32 Fold = 0; Fold < 3; ++Fold)
				{
					Add(BoxDeco, Prop + FVector(Fold * 2.f, 0.f, -17.f + Fold * 9.f), FVector(65.f, 45.f, 10.f), Fold % 2 ? Coral : NavyLight);
				}
				Add(TorusDeco, Prop + FVector(0.f, 0.f, 11.f), FVector(34.f, 28.f, 7.f), Charcoal);
				break;
			}
		}
	}
	Crate(FVector(1450.f, 1500.f, -120.f), 90.f, 10.f);
	Crate(FVector(1450.f, 1500.f, -30.f), 60.f, 30.f);
	Crate(FVector(1560.f, 1560.f, -120.f), 70.f, -12.f);
	Add(BoxDeco, FVector(1700.f, 1580.f, -80.f), FVector(90.f, 110.f, 80.f), Hex(0xA29B8B), 0.f, FRotator(0.f, 25.f, 6.f));
	Add(BallDeco, FVector(1960.f, 1540.f, -90.f), FVector(80.f), Coral);
	// harpoon rack (the harpoon prop sits on top)
	Add(BoxSolid, FVector(2250.f, 1700.f, -80.f), FVector(70.f, 220.f, 80.f), Charcoal);
	Add(BoxDeco, FVector(2250.f, 1700.f, -38.f), FVector(76.f, 226.f, 4.f), Yellow);
	Add(CubeDeco, FVector(2290.f, 1700.f, 60.f), FVector(4.f, 220.f, 60.f), Red);
	Text(TEXT("HERO HARPOON"), FVector(2286.f, 1700.f, 60.f), 180.f, 26.f, FColor::White);
	// spare lights
	for (int32 i = 0; i < 3; ++i)
	{
		const FVector L(2100.f + i * 70.f, 1480.f, -120.f);
		Add(CylDeco, L + FVector(0.f, 0.f, 60.f), FVector(6.f, 6.f, 120.f), Charcoal);
		Add(BoxDeco, L + FVector(0.f, 0.f, 130.f), FVector(40.f, 34.f, 34.f), i == 1 ? Coral : Teal);
	}
	Add(CubeDeco, FVector(1880.f, 1994.f, 420.f), FVector(600.f, 4.f, 100.f), Yellow);
	Text(TEXT("PROP WAREHOUSE"), FVector(1880.f, 1990.f, 420.f), -90.f, 56.f, FColor(27, 33, 64));
	Point(FVector(1880.f, 1700.f, 350.f), FLinearColor(1.f, 0.85f, 0.6f), 9000.f, 1200.f, ZStage);
}

// ============================================================================ lighting states

void AFTStudioShell::ApplyLighting()
{
	const int32 Total = Points.Num() + Spots.Num();
	for (int32 i = 0; i < Total && i < LightBase.Num(); ++i)
	{
		const int32 Zone = LightZone[i];
		float F = 1.f;
		if (Zone == ZStage)
		{
			F = PhaseDim * PowerDim;
		}
		else if (Zone == ZProjection)
		{
			F = PhaseDim < 1.f ? 0.5f : 1.f;
		}
		else if (Zone == ZLobby)
		{
			F = PhaseDim < 0.2f ? 0.4f : 1.f;
		}
		const float I = LightBase[i] * F;
		if (i < Points.Num())
		{
			Points[i]->SetIntensity(I);
		}
		else
		{
			Spots[i - Points.Num()]->SetIntensity(I);
		}
	}
}

void AFTStudioShell::OnShootPhaseChanged(EFTShootPhase Phase)
{
	PhaseDim = Phase == EFTShootPhase::Premiere ? 0.12f : (Phase == EFTShootPhase::Failed ? 0.05f : 1.f);
	ApplyLighting();
}

void AFTStudioShell::OnStagePowerChanged(bool bPowered)
{
	PowerDim = bPowered ? 1.f : 0.3f;
	ApplyLighting();
}
