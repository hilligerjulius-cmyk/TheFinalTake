#include "TheFinalTake/World/FTCity.h"

#include "TheFinalTake/Career/FTEconomy.h"
#include "TheFinalTake/Characters/FTCharacter.h"
#include "TheFinalTake/Core/FTAudio.h"
#include "TheFinalTake/Core/FTVisuals.h"
#include "TheFinalTake/Data/FTFilmDefinition.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Interaction/FTInteractableComponent.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "FinalTakeCity"

using namespace FTColors;

namespace
{
	const FLinearColor Asphalt = Hex(0x3C4262);
	const FLinearColor Sidewalk = Hex(0x6E7391);
	const FLinearColor Ground = Hex(0x30355A);
	const FLinearColor Tape = Hex(0xFFC93C);
	const FLinearColor Brass = Hex(0xE3B04B);
	const FLinearColor Carpet = Hex(0xC9304A);
	const FLinearColor Glass = FLinearColor(0.55f, 0.85f, 1.f, 0.22f);
	const FLinearColor WarmWindow = Hex(0xFFD58A);
	const FLinearColor DarkWindow = Hex(0x1B2140);

	float Hash01(int32 A, int32 B)
	{
		return FMath::Frac(FMath::Sin(A * 12.9898f + B * 78.233f) * 43758.5453f);
	}

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

	UInstancedStaticMeshComponent* MakeIsm(AActor* Owner, USceneComponent* Parent, FName Name, EFTShape Shape, bool bSolid)
	{
		UInstancedStaticMeshComponent* I = Owner->CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		I->SetupAttachment(Parent);
		I->SetStaticMesh(FTVis::GetMesh(Shape));
		I->NumCustomDataFloats = 5;
		if (bSolid)
		{
			I->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			I->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			I->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
		}
		else
		{
			FTVis::NoCollision(I);
		}
		return I;
	}

	void AddInst(UInstancedStaticMeshComponent* I, const FVector& Center, const FVector& Size, const FLinearColor& Color, float Emissive = 0.f, const FRotator& Rot = FRotator::ZeroRotator)
	{
		const int32 Index = I->AddInstance(FTransform(Rot, Center, Size / 100.f), false);
		I->SetCustomData(Index, { Color.R, Color.G, Color.B, Emissive, 0.f }, false);
	}
}

// ============================================================================ city shell

AFTCityShell::AFTCityShell()
{
	// the city needs more signs than the studio pool provides
	AddPools(40, 0, 0);
	for (int32 g : { CubeDeco, BoxDeco, SphereDeco, BallDeco, ConeDeco, PrismDeco, TorusDeco, CapsuleDeco, CylDeco })
	{
		Groups[g]->SetCastShadow(false);
	}
	if (TankWater)
	{
		TankWater->SetVisibility(false);
		TankWater->SetHiddenInGame(true);
	}
}

void AFTCityShell::BuildContent()
{
	BuildBoulevard();
	BuildBlocks();
	BuildPlaza();
	BuildCinema();
}

void AFTCityShell::StreetLamp(const FVector& Base, float ArmDir, bool bLight)
{
	Add(CylDeco, Base + FVector(0.f, 0.f, 8.f), FVector(30.f, 30.f, 16.f), NavyLight);
	Add(CylSolid, Base + FVector(0.f, 0.f, 210.f), FVector(14.f, 14.f, 420.f), Navy);
	Add(BoxDeco, Base + FVector(0.f, ArmDir * 45.f, 420.f), FVector(20.f, 90.f, 12.f), Navy);
	Add(BoxDeco, Base + FVector(0.f, ArmDir * 85.f, 405.f), FVector(40.f, 50.f, 26.f), Charcoal);
	Add(CubeDeco, Base + FVector(0.f, ArmDir * 85.f, 390.f), FVector(30.f, 40.f, 4.f), Amber, 20.f);
	if (bLight)
	{
		Point(Base + FVector(0.f, ArmDir * 85.f, 360.f), FLinearColor(1.f, 0.72f, 0.4f), 9000.f, 1150.f, ZoneExterior);
	}
}

void AFTCityShell::BuildBoulevard()
{
	// ground slab for the whole district (walkable), roadway, lane paint, sidewalks with curbs
	Wall(FVector(-15000.f, -3000.f, -30.f), FVector(-3700.f, 3000.f, 0.f), Ground);
	Add(CubeDeco, FVector(-7050.f, -150.f, 0.3f), FVector(6700.f, 500.f, 0.6f), Asphalt);
	for (float X = -3950.f; X > -10300.f; X -= 500.f)
	{
		Add(CubeDeco, FVector(X, -150.f, 0.7f), FVector(220.f, 14.f, 0.4f), Tape, 0.25f);
	}
	Wall(FVector(-10400.f, -600.f, 0.f), FVector(-3700.f, -400.f, 4.f), Sidewalk);
	Wall(FVector(-10400.f, 100.f, 0.f), FVector(-3700.f, 300.f, 4.f), Sidewalk);
	Add(CubeDeco, FVector(-7050.f, -402.f, 3.f), FVector(6700.f, 8.f, 7.f), Cream);
	Add(CubeDeco, FVector(-7050.f, 102.f, 3.f), FVector(6700.f, 8.f, 7.f), Cream);
	for (float X = -4000.f; X > -10300.f; X -= 240.f)
	{
		// paving joints
		Add(CubeDeco, FVector(X, -500.f, 4.3f), FVector(3.f, 196.f, 0.5f), NavyLight);
		Add(CubeDeco, FVector(X, 200.f, 4.3f), FVector(3.f, 196.f, 0.5f), NavyLight);
	}

	// zebra crossing with traffic lights halfway down the boulevard
	for (int32 i = 0; i < 8; ++i)
	{
		Add(CubeDeco, FVector(-7000.f, -370.f + i * 62.f, 0.8f), FVector(210.f, 32.f, 0.4f), White, 0.15f);
	}
	for (const FVector& P : { FVector(-6860.f, -520.f, 4.f), FVector(-7140.f, 220.f, 4.f) })
	{
		Add(CylSolid, P + FVector(0.f, 0.f, 180.f), FVector(12.f, 12.f, 360.f), Charcoal);
		Add(BoxDeco, P + FVector(0.f, 0.f, 400.f), FVector(30.f, 30.f, 90.f), Charcoal);
		Add(SphereDeco, P + FVector(16.f, 0.f, 428.f), FVector(16.f), Red, 2.f);
		Add(SphereDeco, P + FVector(16.f, 0.f, 400.f), FVector(16.f), Amber, 0.3f);
		Add(SphereDeco, P + FVector(16.f, 0.f, 372.f), FVector(16.f), Green, 6.f);
	}

	// lamps on both sidewalks (every lamp lit; no shadows)
	for (float X = -4300.f; X > -10100.f; X -= 950.f)
	{
		StreetLamp(FVector(X, 250.f, 4.f), -1.f, true);
		StreetLamp(FVector(X - 475.f, -550.f, 4.f), 1.f, true);
	}
	// planters, hydrants, bins, benches
	for (float X = -4750.f; X > -10000.f; X -= 1900.f)
	{
		Plant(FVector(X, 210.f, 4.f), 1.2f);
		Plant(FVector(X - 950.f, -510.f, 4.f), 1.2f);
		Add(CylSolid, FVector(X - 300.f, -470.f, 34.f), FVector(22.f, 22.f, 60.f), Red);
		Add(SphereDeco, FVector(X - 300.f, -470.f, 66.f), FVector(22.f), Red);
		Add(CylSolid, FVector(X + 300.f, 250.f, 42.f), FVector(40.f, 40.f, 76.f), TealDark);
		Add(CylDeco, FVector(X + 300.f, 250.f, 82.f), FVector(44.f, 44.f, 6.f), Grey);
	}
	for (float X : { -5600.f, -8600.f })
	{
		Add(BoxSolid, FVector(X, 255.f, 26.f), FVector(160.f, 44.f, 44.f), Wood);
		Add(BoxDeco, FVector(X, 280.f, 70.f), FVector(160.f, 8.f, 50.f), Wood);
	}

	// wayfinding: signpost at the studio end + coral arrows down the road
	Add(CylSolid, FVector(-3800.f, 220.f, 154.f), FVector(12.f, 12.f, 300.f), Charcoal);
	Add(BoxDeco, FVector(-3800.f, 220.f, 300.f), FVector(12.f, 300.f, 76.f), Coral);
	Text(TEXT("<< GRAND CINEMA"), FVector(-3793.f, 220.f, 304.f), 0.f, 34.f, FColor(255, 245, 225));
	Add(BoxDeco, FVector(-3800.f, 220.f, 236.f), FVector(12.f, 240.f, 46.f), Navy);
	Text(TEXT("<< DREAM CARS"), FVector(-3793.f, 220.f, 238.f), 0.f, 24.f, FColor(255, 214, 90));
	for (float X : { -4400.f, -6000.f, -7700.f, -9300.f })
	{
		Arrow(FVector(X + 120.f, -150.f, 1.f), FVector(X - 120.f, -150.f, 1.f), Coral, 2);
	}
}

void AFTCityShell::Building(float X0, float X1, bool bNorth, float Height, const FLinearColor& Color, const FString& Sign, const FLinearColor& SignColor, int32 Seed)
{
	const float Front = bNorth ? 300.f : -600.f;
	const float Back = bNorth ? 2300.f : -2600.f;
	const float Dir = bNorth ? -1.f : 1.f; // towards the road
	const float Yaw = bNorth ? -90.f : 90.f;
	const float Mid = (X0 + X1) * 0.5f;
	const float W = X1 - X0;
	auto F = [&](float X, float Z, float Out) { return FVector(X, Front + Dir * Out, Z); };
	const FRotator Face(0.f, bNorth ? 0.f : 180.f, 0.f);

	Wall(FVector(X0, FMath::Min(Front, Back), -30.f), FVector(X1, FMath::Max(Front, Back), Height), Color);
	// cornice, plinth
	Add(CubeDeco, F(Mid, Height - 22.f, 6.f), FVector(W + 24.f, 12.f, 44.f), Color * 1.4f);
	Add(CubeDeco, F(Mid, 22.f, 3.f), FVector(W, 6.f, 44.f), Charcoal);

	// shopfront: lit display window with mullions, door, striped awning, neon sign board
	const float ShopW = W * 0.58f;
	const float ShopX = X0 + W * 0.36f;
	Add(CubeDeco, F(ShopX, 170.f, 2.f), FVector(ShopW, 4.f, 210.f), WarmWindow, 1.1f);
	for (int32 m = 0; m <= 3; ++m)
	{
		Add(CubeDeco, F(ShopX - ShopW * 0.5f + ShopW * m / 3.f, 170.f, 5.f), FVector(8.f, 4.f, 214.f), Cream);
	}
	Add(CubeDeco, F(ShopX, 62.f, 5.f), FVector(ShopW + 8.f, 4.f, 8.f), Cream);
	const float DoorX = X0 + W * 0.8f;
	Add(CubeDeco, F(DoorX, 134.f, 2.f), FVector(140.f, 4.f, 260.f), Charcoal);
	Add(CubeDeco, F(DoorX, 134.f, 4.f), FVector(100.f, 4.f, 200.f), WarmWindow * 0.6f, 0.6f);
	Add(CubeDeco, F(DoorX - 40.f, 130.f, 6.f), FVector(6.f, 4.f, 30.f), Brass);
	for (int32 s = 0; s < 6; ++s)
	{
		const FLinearColor Stripe = s % 2 ? Cream : SignColor;
		Add(BoxDeco, F(ShopX - ShopW * 0.5f + ShopW * (s + 0.5f) / 6.f, 300.f, 58.f), FVector(ShopW / 6.f + 1.f, 110.f, 12.f), Stripe, 0.f, FRotator(0.f, 0.f, bNorth ? 14.f : -14.f));
	}
	Add(BoxDeco, F(Mid, 392.f, 9.f), FVector(ShopW + 40.f, 16.f, 74.f), Navy);
	Add(CubeDeco, F(Mid, 432.f, 18.f), FVector(ShopW + 50.f, 3.f, 5.f), SignColor, 4.f);
	Add(CubeDeco, F(Mid, 352.f, 18.f), FVector(ShopW + 50.f, 3.f, 5.f), SignColor, 4.f);
	Text(Sign, F(Mid, 394.f, 18.f), Yaw, 46.f, (SignColor * 1.2f).ToFColor(true));

	// upper floors: window grid, a few lit
	for (float Z = 560.f; Z < Height - 180.f; Z += 230.f)
	{
		int32 Col = 0;
		for (float X = X0 + 120.f; X < X1 - 90.f; X += 190.f, ++Col)
		{
			const bool bLit = Hash01(Seed * 31 + Col, FMath::RoundToInt(Z)) > 0.52f;
			Add(CubeDeco, F(X, Z, 2.f), FVector(100.f, 4.f, 130.f), bLit ? WarmWindow : DarkWindow, bLit ? 1.2f : 0.f);
			Add(CubeDeco, F(X, Z - 72.f, 6.f), FVector(118.f, 12.f, 8.f), Cream);
		}
	}
	// roof furniture
	const FVector Roof(Mid + W * 0.2f, Front - Dir * 500.f, Height);
	if (Seed % 3 == 0)
	{
		for (float Lx : { -60.f, 60.f })
		{
			for (float Ly : { -60.f, 60.f })
			{
				Add(CylDeco, Roof + FVector(Lx, Ly, 70.f), FVector(8.f, 8.f, 140.f), WoodDark);
			}
		}
		Add(CylDeco, Roof + FVector(0.f, 0.f, 200.f), FVector(190.f, 190.f, 150.f), Wood);
		Add(ConeDeco, Roof + FVector(0.f, 0.f, 300.f), FVector(200.f, 200.f, 60.f), WoodDark);
	}
	else
	{
		Add(CylDeco, Roof + FVector(0.f, 0.f, 200.f), FVector(8.f, 8.f, 400.f), Grey);
		Add(SphereDeco, Roof + FVector(0.f, 0.f, 404.f), FVector(16.f), Red, 12.f);
		Add(BoxDeco, Roof + FVector(-200.f, 0.f, 60.f), FVector(160.f, 160.f, 120.f), GreyDark);
	}
}

void AFTCityShell::BuildBlocks()
{
	struct FBlock { float X0, X1, H; FLinearColor C; const TCHAR* Sign; FLinearColor SignC; };
	const FBlock North[] = {
		{ -6800.f, -5600.f, 1600.f, NavyLight, TEXT("DINER"), Magenta },
		{ -8200.f, -7100.f, 2200.f, Hex(0x3A3F6E), TEXT("HOTEL CONTINENTAL"), Yellow },
		{ -9800.f, -8400.f, 1300.f, Hex(0x4A3F72), TEXT("CAMERA SUPPLY"), Cyan } };
	const FBlock South[] = {
		{ -5200.f, -3900.f, 1200.f, Hex(0x2F355E), TEXT("PIZZA"), Coral },
		{ -6700.f, -5400.f, 1800.f, Hex(0x3E335E), TEXT("RECORDS"), Cyan },
		{ -8300.f, -7100.f, 1100.f, NavyLight, TEXT("LAUNDROMAT"), Green },
		{ -9400.f, -8500.f, 1500.f, Hex(0x3A3F6E), TEXT("PAWN SHOP"), Amber } };
	int32 Seed = 1;
	for (const FBlock& B : North)
	{
		Building(B.X0, B.X1, true, B.H, B.C, B.Sign, B.SignC, Seed++);
	}
	for (const FBlock& B : South)
	{
		Building(B.X0, B.X1, false, B.H, B.C, B.Sign, B.SignC, Seed++);
	}
	// alleys between the blocks: closed off by a back wall with a dumpster in front
	struct FAlley { float X0, X1; bool bNorth; };
	const FAlley Alleys[] = { { -5600.f, -5400.f, true }, { -7100.f, -6800.f, true }, { -8400.f, -8200.f, true },
		{ -5400.f, -5200.f, false }, { -7100.f, -6700.f, false }, { -8500.f, -8300.f, false } };
	for (const FAlley& A : Alleys)
	{
		const float WallY = A.bNorth ? 560.f : -860.f;
		Wall(FVector(A.X0, WallY - 20.f, -30.f), FVector(A.X1, WallY + 20.f, 700.f), Hex(0x262B4A));
		const float Mid = (A.X0 + A.X1) * 0.5f;
		Add(BoxSolid, FVector(Mid, WallY + (A.bNorth ? -60.f : 60.f), 60.f), FVector(FMath::Min(150.f, A.X1 - A.X0 - 30.f), 80.f, 120.f), TealDark);
		Add(BoxDeco, FVector(Mid, WallY + (A.bNorth ? -60.f : 60.f), 124.f), FVector(FMath::Min(156.f, A.X1 - A.X0 - 24.f), 86.f, 8.f), GreyDark);
	}

	// north-east lot: Dream Cars dealership (showroom + sales desk are placed in work package 4)
	Wall(FVector(-5600.f, 300.f, 0.f), FVector(-3900.f, 1500.f, 4.f), Hex(0x8C91AE));
	for (float X = -5450.f; X < -3950.f; X += 300.f)
	{
		Add(CubeDeco, FVector(X, 900.f, 4.4f), FVector(8.f, 900.f, 0.6f), White, 0.2f);
	}
	Wall(FVector(-5600.f, 1500.f, 0.f), FVector(-3900.f, 1540.f, 260.f), Hex(0x262B4A));
	Wall(FVector(-5640.f, 300.f, 0.f), FVector(-5600.f, 1540.f, 260.f), Hex(0x262B4A));
	Add(Blocker, FVector(-4750.f, 1520.f, 500.f), FVector(1700.f, 40.f, 1000.f), White);
	Add(BoxDeco, FVector(-4750.f, 1480.f, 520.f), FVector(20.f, 1200.f, 240.f), Coral, 0.2f);
	Text(TEXT("DREAM CARS"), FVector(-4750.f, 1468.f, 560.f), -90.f, 130.f, FColor(255, 245, 225));
	Text(TEXT("FASTER CARS - FASTER PREMIERES"), FVector(-4750.f, 1468.f, 450.f), -90.f, 36.f, FColor(40, 30, 60));
	for (int32 i = 0; i < 10; ++i)
	{
		Add(SphereDeco, FVector(-5320.f + i * 128.f, 1466.f, 660.f), FVector(14.f), Yellow, 10.f);
	}
	Add(CylSolid, FVector(-5300.f, 1480.f, 200.f), FVector(16.f, 16.f, 400.f), Grey);
	Add(CylSolid, FVector(-4200.f, 1480.f, 200.f), FVector(16.f, 16.f, 400.f), Grey);
	Point(FVector(-4750.f, 1200.f, 520.f), FLinearColor(1.f, 0.85f, 0.7f), 7000.f, 1300.f, ZoneExterior);

	// north-west pocket park, south-west parking lot
	Wall(FVector(-10400.f, 300.f, 0.f), FVector(-9800.f, 1200.f, 6.f), Hex(0x3E8E5B));
	Palm(FVector(-10200.f, 700.f, 6.f), 320.f);
	Palm(FVector(-9950.f, 1000.f, 6.f), 260.f);
	Plant(FVector(-10000.f, 500.f, 6.f), 1.3f);
	Add(BoxSolid, FVector(-10250.f, 420.f, 28.f), FVector(160.f, 44.f, 44.f), Wood);
	Wall(FVector(-10400.f, 1200.f, 0.f), FVector(-9800.f, 1240.f, 200.f), Hex(0x2B6B47));
	Add(Blocker, FVector(-10100.f, 1220.f, 500.f), FVector(600.f, 40.f, 1000.f), White);

	Add(CubeDeco, FVector(-9900.f, -1150.f, 0.4f), FVector(1000.f, 1100.f, 0.8f), Asphalt);
	for (int32 b = 0; b <= 4; ++b)
	{
		Add(CubeDeco, FVector(-10350.f + b * 225.f, -1250.f, 0.9f), FVector(8.f, 500.f, 0.4f), White, 0.2f);
	}
	Add(CylSolid, FVector(-9500.f, -1600.f, 180.f), FVector(12.f, 12.f, 360.f), Grey);
	Add(BoxDeco, FVector(-9500.f, -1600.f, 380.f), FVector(12.f, 90.f, 90.f), Blue);
	Text(TEXT("P"), FVector(-9500.f, -1592.f, 380.f), 90.f, 70.f, FColor::White);
	Wall(FVector(-10400.f, -1740.f, 0.f), FVector(-9400.f, -1700.f, 120.f), Hex(0x262B4A));
	Wall(FVector(-10440.f, -1740.f, 0.f), FVector(-10400.f, -1500.f, 120.f), Hex(0x262B4A));
	Add(Blocker, FVector(-9900.f, -1720.f, 500.f), FVector(1000.f, 40.f, 1000.f), White);
	Add(Blocker, FVector(-10420.f, -1620.f, 500.f), FVector(40.f, 240.f, 1000.f), White);
	Point(FVector(-9900.f, -1150.f, 500.f), FLinearColor(0.8f, 0.9f, 1.f), 6000.f, 1200.f, ZoneExterior);

	// rooftop billboard facing the studio street
	Add(BoxDeco, FVector(-6100.f, 1300.f, 1900.f), FVector(20.f, 900.f, 420.f), Navy);
	Add(CubeDeco, FVector(-6088.f, 1300.f, 1900.f), FVector(4.f, 860.f, 380.f), Coral, 0.35f);
	Text(TEXT("TONIGHT AT THE GRAND"), FVector(-6084.f, 1300.f, 1990.f), 0.f, 70.f, FColor(255, 245, 225));
	Text(TEXT("A FINAL TAKE PRODUCTION"), FVector(-6084.f, 1300.f, 1850.f), 0.f, 40.f, FColor(40, 30, 60));
}

void AFTCityShell::BuildPlaza()
{
	// raised plaza in front of the cinema with red carpet, rope line and a star emblem
	Wall(FVector(-11200.f, -1500.f, 0.f), FVector(-10400.f, 1200.f, 4.f), Hex(0xCFC2A8));
	for (float Y = -1350.f; Y < 1150.f; Y += 150.f)
	{
		Add(CubeDeco, FVector(-10800.f, Y, 4.3f), FVector(800.f, 3.f, 0.5f), Hex(0xAFA38B));
	}
	Add(CubeDeco, FVector(-10800.f, -150.f, 4.6f), FVector(800.f, 260.f, 0.6f), Carpet);
	Add(CubeDeco, FVector(-10800.f, -285.f, 4.7f), FVector(800.f, 10.f, 0.6f), Brass, 0.3f);
	Add(CubeDeco, FVector(-10800.f, -15.f, 4.7f), FVector(800.f, 10.f, 0.6f), Brass, 0.3f);
	for (float X = -10500.f; X > -11150.f; X -= 150.f)
	{
		for (float Y : { -330.f, 30.f })
		{
			Add(CylSolid, FVector(X, Y, 50.f), FVector(10.f, 10.f, 92.f), Brass);
			Add(SphereDeco, FVector(X, Y, 98.f), FVector(15.f), Brass, 0.4f);
		}
	}
	for (float Y : { -330.f, 30.f })
	{
		Add(CapsuleDeco, FVector(-10800.f, Y, 80.f), FVector(6.f, 6.f, 600.f), Carpet, 0.f, FRotator(-90.f, 0.f, 0.f));
	}
	for (const FVector& S : { FVector(-10620.f, -800.f, 4.5f), FVector(-10620.f, 500.f, 4.5f) })
	{
		Add(CylDeco, S, FVector(220.f, 220.f, 0.6f), Yellow, 0.4f);
		Add(TorusDeco, S + FVector(0.f, 0.f, 0.3f), FVector(240.f, 240.f, 4.f), Brass, 0.3f);
	}
	// edges: planters + invisible bounds
	for (float Y : { -1480.f, 1180.f })
	{
		Add(BoxSolid, FVector(-10800.f, Y, 34.f), FVector(800.f, 50.f, 60.f), Hex(0xD9784A));
		Add(Blocker, FVector(-10800.f, Y, 500.f), FVector(800.f, 40.f, 1000.f), White);
		for (float X = -10550.f; X > -11150.f; X -= 200.f)
		{
			Add(BallDeco, FVector(X, Y, 80.f), FVector(90.f, 50.f, 60.f), Green);
		}
	}
	Point(FVector(-10700.f, -800.f, 450.f), FLinearColor(1.f, 0.85f, 0.65f), 8000.f, 1200.f, ZoneExterior);
	Point(FVector(-10700.f, 500.f, 450.f), FLinearColor(1.f, 0.85f, 0.65f), 8000.f, 1200.f, ZoneExterior);
	Spot(FVector(-10500.f, -1200.f, 20.f), FRotator(55.f, 160.f, 0.f), FLinearColor(1.f, 0.8f, 0.55f), 40000.f, 2600.f, 28.f, ZoneExterior);
	Spot(FVector(-10500.f, 900.f, 20.f), FRotator(55.f, -160.f, 0.f), FLinearColor(1.f, 0.8f, 0.55f), 40000.f, 2600.f, 28.f, ZoneExterior);
}

void AFTCityShell::BuildCinema()
{
	const FLinearColor Outer = Hex(0x2B2F5E);
	const FLinearColor FoyerWall = Hex(0x7A2E4A);
	const FLinearColor HallWall = Hex(0x4A1E36);
	const FLinearColor Trim = Brass;

	// ------------------------------------------------ shell: floor, outer walls, roof
	Wall(FVector(-14800.f, -1400.f, 0.f), FVector(-11200.f, 1100.f, 4.f), Hex(0x3B2233));
	Wall(FVector(-11240.f, -1400.f, 0.f), FVector(-11200.f, -300.f, 1540.f), Outer);
	Wall(FVector(-11240.f, 300.f, 0.f), FVector(-11200.f, 1100.f, 1540.f), Outer);
	Wall(FVector(-11240.f, -300.f, 420.f), FVector(-11200.f, 300.f, 1540.f), Outer);
	Wall(FVector(-14800.f, 1060.f, 0.f), FVector(-11240.f, 1100.f, 1540.f), Outer);
	Wall(FVector(-14800.f, -1400.f, 0.f), FVector(-11240.f, -1360.f, 1540.f), Outer);
	Wall(FVector(-14800.f, -1360.f, 0.f), FVector(-14760.f, 1060.f, 1540.f), Outer);
	Wall(FVector(-14800.f, -1400.f, 1540.f), FVector(-11200.f, 1100.f, 1580.f), Hex(0x1E2140));
	// facade dressing: cream art-deco pilasters, gold bands, glass entrance frame
	for (float Y : { -1340.f, -840.f, 540.f, 1040.f })
	{
		Add(BoxDeco, FVector(-11190.f, Y, 770.f), FVector(24.f, 70.f, 1540.f), Cream);
		Add(PrismDeco, FVector(-11190.f, Y, 1580.f), FVector(24.f, 70.f, 80.f), Brass, 0.4f);
	}
	for (float Z : { 40.f, 1300.f, 1500.f })
	{
		Add(CubeDeco, FVector(-11193.f, -150.f, Z), FVector(8.f, 2500.f, 16.f), Trim, 0.4f);
	}
	for (float Y : { -310.f, 310.f })
	{
		Add(BoxDeco, FVector(-11195.f, Y, 210.f), FVector(30.f, 24.f, 420.f), Trim, 0.3f);
		Add(GlassSolid, FVector(-11225.f, Y + (Y < 0.f ? -120.f : 120.f), 200.f), FVector(10.f, 220.f, 380.f), Glass);
	}
	Add(BoxDeco, FVector(-11195.f, 0.f, 430.f), FVector(30.f, 640.f, 24.f), Trim, 0.3f);
	for (float Z = 520.f; Z < 1250.f; Z += 180.f)
	{
		for (float Y : { -620.f, -420.f, 120.f, 320.f })
		{
			Add(CubeDeco, FVector(-11197.f, Y, Z), FVector(4.f, 120.f, 120.f), WarmWindow * 0.8f, 0.9f);
		}
	}

	// ------------------------------------------------ foyer
	Add(CubeDeco, FVector(-11920.f, -150.f, 4.5f), FVector(1340.f, 2400.f, 1.f), Hex(0x8E2340));
	for (float X = -11400.f; X > -12500.f; X -= 220.f)
	{
		Add(CubeDeco, FVector(X, -150.f, 4.9f), FVector(12.f, 2380.f, 0.4f), Trim, 0.25f);
	}
	Add(CubeDeco, FVector(-11920.f, 1057.f, 700.f), FVector(1350.f, 4.f, 1380.f), FoyerWall);
	Add(CubeDeco, FVector(-11920.f, -1357.f, 700.f), FVector(1350.f, 4.f, 1380.f), FoyerWall);
	for (float X = -11400.f; X > -12600.f; X -= 300.f)
	{
		Add(CubeDeco, FVector(X, 1054.f, 700.f), FVector(10.f, 4.f, 1380.f), Trim, 0.3f);
		Add(CubeDeco, FVector(X, -1354.f, 700.f), FVector(10.f, 4.f, 1380.f), Trim, 0.3f);
	}
	// chandeliers
	for (const FVector& C : { FVector(-11650.f, -150.f, 820.f), FVector(-12250.f, 150.f, 820.f) })
	{
		Add(CylDeco, C + FVector(0.f, 0.f, 360.f), FVector(4.f, 4.f, 720.f), Brass);
		Add(TorusDeco, C, FVector(160.f, 160.f, 14.f), Brass, 0.4f);
		Add(SphereDeco, C + FVector(0.f, 0.f, -30.f), FVector(60.f), Cream, 8.f);
		for (int32 k = 0; k < 8; ++k)
		{
			Add(SphereDeco, C + FRotator(0.f, k * 45.f, 0.f).RotateVector(FVector(78.f, 0.f, 12.f)), FVector(14.f), WarmWindow, 10.f);
		}
		Point(C + FVector(0.f, 0.f, -80.f), FLinearColor(1.f, 0.84f, 0.62f), 22000.f, 1900.f, ZoneLobby, C.X > -12000.f);
	}
	// box office (north-east) and concessions (north-west)
	Add(BoxSolid, FVector(-11650.f, 930.f, 55.f), FVector(460.f, 120.f, 110.f), Coral);
	Add(BoxDeco, FVector(-11650.f, 930.f, 114.f), FVector(476.f, 130.f, 10.f), Cream);
	Add(GlassSolid, FVector(-11650.f, 960.f, 200.f), FVector(460.f, 6.f, 160.f), Glass);
	Add(BoxDeco, FVector(-11650.f, 1040.f, 350.f), FVector(460.f, 12.f, 76.f), Navy);
	Text(TEXT("BOX OFFICE"), FVector(-11650.f, 1032.f, 352.f), -90.f, 44.f, FColor(255, 214, 90));
	Add(BoxSolid, FVector(-12260.f, 930.f, 55.f), FVector(520.f, 120.f, 110.f), TealDark);
	Add(BoxDeco, FVector(-12260.f, 930.f, 114.f), FVector(536.f, 130.f, 10.f), Cream);
	Add(GlassSolid, FVector(-12400.f, 940.f, 175.f), FVector(80.f, 70.f, 110.f), Glass);
	for (int32 k = 0; k < 9; ++k)
	{
		Add(SphereDeco, FVector(-12420.f + (k % 3) * 20.f, 925.f + (k / 3) * 15.f, 135.f + (k % 2) * 12.f), FVector(12.f), Yellow, 0.6f);
	}
	Add(CylDeco, FVector(-12150.f, 910.f, 150.f), FVector(40.f, 40.f, 60.f), Red);
	Add(CylDeco, FVector(-12050.f, 910.f, 150.f), FVector(40.f, 40.f, 60.f), Blue);
	Add(BoxDeco, FVector(-12260.f, 1040.f, 350.f), FVector(520.f, 12.f, 76.f), Navy);
	Text(TEXT("POPCORN - CANDY - SODA"), FVector(-12260.f, 1032.f, 352.f), -90.f, 34.f, FColor(255, 120, 200));
	// film posters on the inside of the facade
	Poster(FVector(-11246.f, -900.f, 300.f), 180.f, 0, 1.1f);
	Poster(FVector(-11246.f, 750.f, 300.f), 180.f, 1, 1.1f);

	// ------------------------------------------------ mezzanine projection booth (stairs along the south wall)
	Stairs(FVector(-11400.f, -1250.f, 4.f), -30.f, 220.f, 20.f, 21, true, Hex(0x8E2340), Trim);
	Wall(FVector(-12600.f, -1360.f, 404.f), FVector(-12030.f, -500.f, 424.f), Hex(0x5A2A3E));
	Add(CubeDeco, FVector(-12315.f, -930.f, 424.6f), FVector(560.f, 850.f, 1.f), Hex(0x2B2F5E));
	for (float X = -12580.f; X <= -12040.f; X += 90.f)
	{
		Add(CylDeco, FVector(X, -505.f, 474.f), FVector(6.f, 6.f, 100.f), Trim);
	}
	for (float Y = -1130.f; Y <= -510.f; Y += 90.f)
	{
		Add(CylDeco, FVector(-12035.f, Y, 474.f), FVector(6.f, 6.f, 100.f), Trim);
	}
	Add(CubeDeco, FVector(-12315.f, -505.f, 526.f), FVector(570.f, 10.f, 8.f), Trim, 0.3f);
	Add(CubeDeco, FVector(-12035.f, -820.f, 526.f), FVector(10.f, 640.f, 8.f), Trim, 0.3f);
	Add(Blocker, FVector(-12315.f, -505.f, 520.f), FVector(570.f, 10.f, 200.f), White);
	Add(Blocker, FVector(-12035.f, -820.f, 520.f), FVector(10.f, 640.f, 200.f), White);
	Add(CubeDeco, FVector(-11700.f, -1357.f, 170.f), FVector(700.f, 5.f, 30.f), Trim, 0.3f);
	Text(TEXT("PROJECTION BOOTH  ^"), FVector(-11500.f, -1352.f, 220.f), 90.f, 26.f, FColor(255, 214, 90));
	Point(FVector(-12300.f, -900.f, 780.f), FLinearColor(1.f, 0.8f, 0.6f), 6000.f, 900.f, ZoneLobby);
	// warm wall washers along the foyer
	Point(FVector(-11500.f, 900.f, 400.f), FLinearColor(1.f, 0.7f, 0.5f), 7000.f, 900.f, ZoneLobby);
	Point(FVector(-12250.f, 900.f, 400.f), FLinearColor(1.f, 0.7f, 0.5f), 7000.f, 900.f, ZoneLobby);

	// ------------------------------------------------ partition foyer | hall: double door + booth window
	Wall(FVector(-12640.f, -1360.f, 0.f), FVector(-12600.f, -1100.f, 1540.f), FoyerWall);
	Wall(FVector(-12640.f, -1100.f, 0.f), FVector(-12600.f, -700.f, 520.f), FoyerWall);
	Wall(FVector(-12640.f, -1100.f, 720.f), FVector(-12600.f, -700.f, 1540.f), FoyerWall);
	Wall(FVector(-12640.f, -700.f, 0.f), FVector(-12600.f, -250.f, 1540.f), FoyerWall);
	Wall(FVector(-12640.f, 250.f, 0.f), FVector(-12600.f, 1060.f, 1540.f), FoyerWall);
	Wall(FVector(-12640.f, -250.f, 380.f), FVector(-12600.f, 250.f, 1540.f), FoyerWall);
	Add(GlassSolid, FVector(-12620.f, -900.f, 620.f), FVector(8.f, 400.f, 200.f), Glass);
	for (float Y : { -262.f, 262.f })
	{
		Add(BoxDeco, FVector(-12596.f, Y, 190.f), FVector(12.f, 24.f, 380.f), Trim, 0.3f);
	}
	Add(BoxDeco, FVector(-12596.f, 0.f, 392.f), FVector(12.f, 548.f, 24.f), Trim, 0.3f);
	Add(BoxDeco, FVector(-12594.f, 0.f, 470.f), FVector(8.f, 560.f, 90.f), Navy);
	Text(TEXT("HALL 1  -  WORLD PREMIERE"), FVector(-12589.f, 0.f, 472.f), 0.f, 34.f, FColor(255, 214, 90));

	// ------------------------------------------------ hall: carpet, stage, curtains, sconces, exit
	Add(CubeDeco, FVector(-13700.f, -150.f, 4.5f), FVector(2110.f, 2410.f, 1.f), Hex(0x3A1A2C));
	Add(CubeDeco, FVector(-13700.f, -150.f, 1537.f), FVector(2110.f, 2410.f, 4.f), Hex(0x221430));
	Add(CubeDeco, FVector(-13700.f, 1057.f, 760.f), FVector(2110.f, 4.f, 1500.f), HallWall);
	Add(CubeDeco, FVector(-13700.f, -1357.f, 760.f), FVector(2110.f, 4.f, 1500.f), HallWall);
	for (float X = -12900.f; X > -14700.f; X -= 350.f)
	{
		for (float Y : { 1054.f, -1354.f })
		{
			Add(CubeDeco, FVector(X, Y, 760.f), FVector(14.f, 4.f, 1500.f), Trim, 0.25f);
			Add(PrismDeco, FVector(X - 175.f, Y, 1300.f), FVector(4.f, 120.f, 90.f), Trim, 0.35f, FRotator(0.f, 90.f, 0.f));
		}
	}
	Wall(FVector(-14760.f, -1300.f, 0.f), FVector(-14330.f, 1000.f, 64.f), Hex(0x5A3A2A));
	Add(CubeDeco, FVector(-14328.f, -150.f, 40.f), FVector(4.f, 2300.f, 40.f), Trim, 0.35f);
	for (float Y : { -1180.f, 880.f })
	{
		Add(BoxDeco, FVector(-14680.f, Y, 760.f), FVector(70.f, 240.f, 1500.f), Carpet);
		for (int32 f = 0; f < 4; ++f)
		{
			Add(CubeDeco, FVector(-14640.f, Y - 90.f + f * 60.f, 760.f), FVector(6.f, 8.f, 1490.f), Hex(0x9C2038));
		}
	}
	Add(BoxDeco, FVector(-14680.f, -150.f, 1440.f), FVector(80.f, 2360.f, 150.f), Carpet);
	Add(CubeDeco, FVector(-14638.f, -150.f, 1370.f), FVector(4.f, 2360.f, 10.f), Trim, 0.5f);
	for (float X : { -14200.f, -13500.f, -12900.f })
	{
		for (float Y : { 1040.f, -1340.f })
		{
			Add(ConeDeco, FVector(X, Y, 420.f), FVector(40.f, 40.f, 50.f), Trim, 0.3f, FRotator(180.f, 0.f, 0.f));
			Add(SphereDeco, FVector(X, Y, 450.f), FVector(22.f), WarmWindow, 9.f);
		}
		Point(FVector(X, 900.f, 470.f), FLinearColor(1.f, 0.78f, 0.55f), 2600.f, 700.f, ZoneHall);
		Point(FVector(X, -1200.f, 470.f), FLinearColor(1.f, 0.78f, 0.55f), 2600.f, 700.f, ZoneHall);
	}
	Point(FVector(-13400.f, -150.f, 1300.f), FLinearColor(1.f, 0.88f, 0.75f), 9000.f, 1800.f, ZoneHall, true);
	Point(FVector(-14200.f, -150.f, 1300.f), FLinearColor(1.f, 0.88f, 0.75f), 7000.f, 1800.f, ZoneHall);
	Add(BoxDeco, FVector(-12650.f, 0.f, 430.f), FVector(8.f, 120.f, 40.f), Charcoal);
	Text(TEXT("EXIT"), FVector(-12655.f, 0.f, 431.f), 180.f, 26.f, FColor(80, 255, 140));
	// aisle guide lights
	for (float X = -12750.f; X > -14300.f; X -= 150.f)
	{
		Add(CubeDeco, FVector(X, -880.f, 6.f), FVector(20.f, 8.f, 3.f), Amber, 4.f);
		Add(CubeDeco, FVector(X, 600.f, 6.f), FVector(20.f, 8.f, 3.f), Amber, 4.f);
	}
}

// ============================================================================ marquee

AFTCinemaMarquee::AFTCinemaMarquee()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	auto Part = [this](const FString& Name, EFTShape Shape, FVector Loc, FVector Size, FLinearColor C, float Glow = 0.f, FRotator R = FRotator::ZeroRotator)
	{
		return FTVis::MakePart(this, Root, *Name, Shape, Loc, Size, C, R, Glow);
	};
	// canopy over the entrance with a bulb-lit underside
	Part(TEXT("Canopy"), EFTShape::Box, FVector(170.f, 0.f, 440.f), FVector(340.f, 960.f, 26.f), Navy);
	Part(TEXT("CanopyTrim"), EFTShape::Box, FVector(342.f, 0.f, 440.f), FVector(6.f, 964.f, 30.f), Brass, 0.5f);
	for (float Y : { -440.f, 440.f })
	{
		Part(FString::Printf(TEXT("CanopyRod%d"), (int32)Y), EFTShape::Cylinder, FVector(220.f, Y, 700.f), FVector(6.f, 6.f, 560.f), Brass, 0.f, FRotator(-38.f, 0.f, 0.f));
	}
	// the marquee letter board
	Part(TEXT("Marquee"), EFTShape::Box, FVector(190.f, 0.f, 580.f), FVector(150.f, 1160.f, 230.f), Coral);
	Part(TEXT("MarqueeBoard"), EFTShape::Box, FVector(266.f, 0.f, 580.f), FVector(2.f, 1090.f, 180.f), Cream, 0.7f);
	Part(TEXT("MarqueeTop"), EFTShape::Box, FVector(190.f, 0.f, 702.f), FVector(160.f, 1180.f, 14.f), Brass, 0.4f);
	const float LineZ[] = { 635.f, 578.f, 520.f };
	const float LineSize[] = { 30.f, 50.f, 26.f };
	for (int32 i = 0; i < 3; ++i)
	{
		Lines.Add(FTVis::MakeText(this, Root, *FString::Printf(TEXT("Line%d"), i), FText::GetEmpty(), FVector(268.f, 0.f, LineZ[i]), FRotator::ZeroRotator, LineSize[i], FColor(35, 25, 55)));
	}
	for (int32 i = 0; i < 18; ++i)
	{
		const float Y = -540.f + i * (1080.f / 17.f);
		Bulbs.Add(Part(FString::Printf(TEXT("BulbTop%d"), i), EFTShape::Sphere, FVector(268.f, Y, 684.f), FVector(13.f), WarmWindow, 6.f));
		Bulbs.Add(Part(FString::Printf(TEXT("BulbLow%d"), i), EFTShape::Sphere, FVector(268.f, Y, 476.f), FVector(13.f), WarmWindow, 6.f));
	}
	// vertical GRAND blade on the facade + poster box for tonight's film
	Part(TEXT("Blade"), EFTShape::Box, FVector(22.f, 1030.f, 1000.f), FVector(34.f, 190.f, 860.f), Navy);
	Part(TEXT("BladeTrim"), EFTShape::Box, FVector(40.f, 1030.f, 1000.f), FVector(4.f, 200.f, 870.f), Brass, 0.6f);
	const TCHAR* Letters[] = { TEXT("G"), TEXT("R"), TEXT("A"), TEXT("N"), TEXT("D") };
	for (int32 i = 0; i < 5; ++i)
	{
		Lines.Add(FTVis::MakeText(this, Root, *FString::Printf(TEXT("Letter%d"), i), FText::FromString(Letters[i]), FVector(43.f, 1030.f, 1330.f - i * 160.f), FRotator::ZeroRotator, 150.f, FColor(255, 214, 90)));
	}
	Part(TEXT("PosterBox"), EFTShape::Box, FVector(20.f, -1030.f, 900.f), FVector(30.f, 200.f, 520.f), Charcoal);
	Part(TEXT("PosterGlow"), EFTShape::Box, FVector(36.f, -1030.f, 900.f), FVector(2.f, 176.f, 490.f), Magenta, 0.8f);
	Lines.Add(FTVis::MakeText(this, Root, TEXT("PosterTitle"), FText::GetEmpty(), FVector(38.f, -1030.f, 1060.f), FRotator::ZeroRotator, 24.f, FColor(255, 245, 225)));
	Lines.Add(FTVis::MakeText(this, Root, TEXT("PosterStars"), FText::GetEmpty(), FVector(38.f, -1030.f, 760.f), FRotator::ZeroRotator, 40.f, FColor(255, 214, 90)));
	FTVis::MakeText(this, Root, TEXT("Crown"), LOCTEXT("GrandCinema", "GRAND CINEMA"), FVector(14.f, 0.f, 1420.f), FRotator::ZeroRotator, 130.f, FColor(255, 240, 215));

	// two searchlights on the roof corners sweeping the night sky
	for (int32 i = 0; i < 2; ++i)
	{
		const float Y = i == 0 ? -1150.f : 1150.f;
		Part(FString::Printf(TEXT("SearchBase%d"), i), EFTShape::Cylinder, FVector(-200.f, Y, 1610.f), FVector(90.f, 90.f, 60.f), GreyDark);
		USceneComponent* Head = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("SearchHead%d"), i));
		Head->SetupAttachment(Root);
		Head->SetRelativeLocation(FVector(-200.f, Y, 1680.f));
		FTVis::MakePart(this, Head, *FString::Printf(TEXT("SearchDrum%d"), i), EFTShape::Cylinder, FVector(0.f, 0.f, 0.f), FVector(80.f, 80.f, 90.f), Grey, FRotator(-90.f, 0.f, 0.f));
		FTVis::MakePart(this, Head, *FString::Printf(TEXT("SearchLens%d"), i), EFTShape::Cylinder, FVector(46.f, 0.f, 0.f), FVector(70.f, 70.f, 4.f), Cream, FRotator(-90.f, 0.f, 0.f), 20.f);
		UStaticMeshComponent* Beam = FTVis::MakePart(this, Head, *FString::Printf(TEXT("SearchBeam%d"), i), EFTShape::Cone, FVector(2600.f, 0.f, 0.f), FVector(420.f, 420.f, 5200.f), Cream, FRotator(90.f, 0.f, 0.f), 0.08f);
		Beam->SetMaterial(0, FTVis::Glow());
		Beam->SetCastShadow(false);
		USpotLightComponent* S = CreateDefaultSubobject<USpotLightComponent>(*FString::Printf(TEXT("SearchSpot%d"), i));
		S->SetupAttachment(Head);
		S->SetRelativeLocation(FVector(50.f, 0.f, 0.f));
		S->SetIntensity(60000.f);
		S->SetAttenuationRadius(6000.f);
		S->SetOuterConeAngle(5.f);
		S->SetInnerConeAngle(3.f);
		S->SetCastShadows(false);
		Searchlights.Add(Head);
		SearchSpots.Add(S);
	}
}

void AFTCinemaMarquee::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	// searchlights: slow figure-eight sweeps, out of phase
	for (int32 i = 0; i < Searchlights.Num(); ++i)
	{
		const float P = Now * 0.35f + i * 2.1f;
		Searchlights[i]->SetRelativeRotation(FRotator(62.f + 10.f * FMath::Sin(P * 1.7f), (i == 0 ? 150.f : 210.f) + 35.f * FMath::Sin(P), 0.f));
	}
	// chase lights
	ChaseTimer += DeltaSeconds;
	if (ChaseTimer > 0.12f)
	{
		ChaseTimer = 0.f;
		ChaseStep = (ChaseStep + 1) % 3;
		for (int32 i = 0; i < Bulbs.Num(); ++i)
		{
			FTVis::SetGlow(Bulbs[i], (i / 2 + ChaseStep) % 3 == 0 ? 14.f : 2.5f);
		}
	}
	// marquee copy follows the shoot
	TextTimer -= DeltaSeconds;
	if (TextTimer > 0.f)
	{
		return;
	}
	TextTimer = 1.f;
	const AFTGameState* GS = GetWorld()->GetGameState<AFTGameState>();
	const UFTFilmDefinition* Film = GS ? GS->GetFilm() : nullptr;
	FText L0, L1, L2, PosterTitle, PosterStars;
	if (GS && Film && (GS->ShootPhase == EFTShootPhase::Premiere || GS->ShootPhase == EFTShootPhase::Results) && GS->LastRelease.bValid)
	{
		L0 = LOCTEXT("NowShowing", "NOW SHOWING");
		L1 = Film->Title;
		L2 = FText::Format(LOCTEXT("MarqueeVerdict", "{0}  -  {1} VIEWERS"), FFTEconomy::TierName(GS->LastRelease.Tier), FText::FromString(FFTEconomy::NumberString(GS->LastRelease.Audience)));
		PosterStars = FText::FromString(FString::ChrN(FMath::Clamp(GS->LastRelease.Stars, 1, 5), TEXT('*')));
	}
	else if (GS && Film && GS->ShootPhase != EFTShootPhase::Lobby && GS->ShootPhase != EFTShootPhase::Title)
	{
		L0 = LOCTEXT("PremiereTonight", "WORLD PREMIERE TONIGHT 06:00");
		L1 = Film->Title;
		L2 = LOCTEXT("MarqueeCrew", "A FINAL TAKE PRODUCTION  -  BRING THE REELS!");
	}
	else
	{
		L0 = LOCTEXT("ComingSoon", "COMING SOON");
		L1 = GS && !GS->StudioName.IsEmpty() ? FText::FromString(GS->StudioName.ToUpper()) : LOCTEXT("YourFilm", "YOUR FILM HERE");
		L2 = GS ? FText::Format(LOCTEXT("MarqueeCareer", "{0} FILMS RELEASED  -  BEST {1}"), FText::AsNumber(GS->CareerFilmsTotal), FText::FromString(FFTEconomy::MoneyString(GS->CareerBestRevenue))) : FText::GetEmpty();
	}
	PosterTitle = Film ? Film->Title : LOCTEXT("PosterDefault", "PREMIERE NIGHT");
	Lines[0]->SetText(L0);
	Lines[1]->SetText(L1);
	Lines[2]->SetText(L2);
	Lines[8]->SetText(FText::FromString(PosterTitle.ToString().Replace(TEXT(" OF THE "), TEXT("\nOF THE\n")).Replace(TEXT(" "), TEXT("\n"))));
	Lines[9]->SetText(PosterStars);
}

// ============================================================================ box office board

AFTBoxOfficeBoard::AFTBoxOfficeBoard()
{
	bReplicates = false;
	FTVis::MakePart(this, Root, TEXT("Frame"), EFTShape::Box, FVector(-6.f, 0.f, 300.f), FVector(12.f, 600.f, 340.f), Brass, FRotator::ZeroRotator, 0.3f);
	FTVis::MakePart(this, Root, TEXT("Panel"), EFTShape::Box, FVector(1.f, 0.f, 300.f), FVector(4.f, 570.f, 312.f), Hex(0x151A33), FRotator::ZeroRotator, 0.2f);
	Header = FTVis::MakeText(this, Root, TEXT("Header"), LOCTEXT("ChartHeader", "BOX OFFICE CHART"), FVector(4.f, 0.f, 432.f), FRotator::ZeroRotator, 30.f, FColor(255, 214, 90));
	for (int32 i = 0; i < NumLines; ++i)
	{
		Lines.Add(FTVis::MakeText(this, Root, *FString::Printf(TEXT("Line%d"), i), FText::GetEmpty(), FVector(4.f, 0.f, 392.f - i * 32.f), FRotator::ZeroRotator, 16.f, FColor(245, 238, 225)));
	}
	Footer = FTVis::MakeText(this, Root, TEXT("Footer"), FText::GetEmpty(), FVector(4.f, 0.f, 158.f), FRotator::ZeroRotator, 14.f, FColor(120, 230, 200));
}

void AFTBoxOfficeBoard::BeginPlay()
{
	Super::BeginPlay();
	if (AFTGameState* G = GetWorld()->GetGameState<AFTGameState>())
	{
		CareerHandle = G->OnCareerChanged.AddUObject(this, &AFTBoxOfficeBoard::Refresh);
	}
	Refresh();
}

void AFTBoxOfficeBoard::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AFTGameState* G = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr)
	{
		G->OnCareerChanged.Remove(CareerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void AFTBoxOfficeBoard::Refresh()
{
	const AFTGameState* G = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
	if (!G)
	{
		return;
	}
	TArray<FFTReleasedFilm> Films = G->ReleasedFilms;
	Films.Sort([](const FFTReleasedFilm& A, const FFTReleasedFilm& B) { return A.Revenue > B.Revenue; });
	Header->SetText(G->StudioName.IsEmpty() ? LOCTEXT("ChartHeader2", "BOX OFFICE CHART")
		: FText::Format(LOCTEXT("ChartHeaderStudio", "{0}  -  BOX OFFICE"), FText::FromString(G->StudioName.ToUpper())));
	for (int32 i = 0; i < NumLines; ++i)
	{
		if (!Films.IsValidIndex(i))
		{
			Lines[i]->SetText(i == 0 ? LOCTEXT("NoReleases", "No releases yet - tonight could be the first!") : FText::GetEmpty());
			continue;
		}
		const FFTReleasedFilm& F = Films[i];
		Lines[i]->SetText(FText::FromString(FString::Printf(TEXT("#%d  %s  (release %d)   %s   %s   %s"), i + 1, *F.Title.ToUpper(), F.ReleaseNumber,
			*FFTEconomy::MoneyString(F.Revenue), *FString::ChrN(FMath::Clamp(F.Stars, 1, 5), TEXT('*')), *FFTEconomy::TierName(F.Tier).ToString())));
	}
	Footer->SetText(FText::Format(LOCTEXT("ChartFooter", "{0} film(s) released  -  best night {1}  -  team account {2}"),
		FText::AsNumber(G->CareerFilmsTotal), FText::FromString(FFTEconomy::MoneyString(G->CareerBestRevenue)), FText::FromString(FFTEconomy::MoneyString(G->StudioMoney))));
}

// ============================================================================ seating + audience

AFTCinemaSeating::AFTCinemaSeating()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	Tiers = MakeIsm(this, Root, TEXT("Tiers"), EFTShape::Cube, true);
	SeatBases = MakeIsm(this, Root, TEXT("SeatBases"), EFTShape::Box, false);
	SeatBacks = MakeIsm(this, Root, TEXT("SeatBacks"), EFTShape::Box, false);
	Bodies = MakeIsm(this, Root, TEXT("Bodies"), EFTShape::Capsule, false);
	Heads = MakeIsm(this, Root, TEXT("Heads"), EFTShape::Ball, false);
	Hair = MakeIsm(this, Root, TEXT("Hair"), EFTShape::Ball, false);
	for (UInstancedStaticMeshComponent* I : { Bodies.Get(), Heads.Get(), Hair.Get() })
	{
		I->SetCastShadow(false);
		I->SetMobility(EComponentMobility::Movable);
	}
}

AFTCinemaSeating* AFTCinemaSeating::Find(const UWorld* World)
{
	for (TActorIterator<AFTCinemaSeating> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

FVector AFTCinemaSeating::SeatLocal(int32 Row, int32 Col) const
{
	// seat origin on the tier top; the front row is one riser up
	return FVector(Row * RowSpacing, Col * SeatSpacing, (Row + 1) * Riser);
}

bool AFTCinemaSeating::IsCrewSeat(int32 Row, int32 Col) const
{
	return Row == CrewRow && Col >= CrewFirstSeat && Col < CrewFirstSeat + 4;
}

void AFTCinemaSeating::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	for (UInstancedStaticMeshComponent* I : { Tiers.Get(), SeatBases.Get(), SeatBacks.Get() })
	{
		I->ClearInstances();
		I->SetMaterial(0, IsmMaterial());
		I->NumCustomDataFloats = 5;
	}
	const float Width = (Cols - 1) * SeatSpacing + 120.f;
	const float MidY = (Cols - 1) * SeatSpacing * 0.5f;
	for (int32 R = 0; R < Rows; ++R)
	{
		const float Top = (R + 1) * Riser;
		AddInst(Tiers, FVector(R * RowSpacing, MidY, Top * 0.5f), FVector(RowSpacing, Width, Top), R % 2 ? Hex(0x3A1A2C) : Hex(0x42203A));
		AddInst(Tiers, FVector(R * RowSpacing - RowSpacing * 0.5f + 2.f, MidY, Top - 1.f), FVector(4.f, Width, 2.f), Brass, 0.3f);
		for (int32 C = 0; C < Cols; ++C)
		{
			const FVector S = SeatLocal(R, C);
			AddInst(SeatBases, S + FVector(-4.f, 0.f, 42.f), FVector(52.f, 58.f, 12.f), Carpet);
			AddInst(SeatBacks, S + FVector(24.f, 0.f, 74.f), FVector(12.f, 60.f, 62.f), Hex(0x8E2340), 0.f, FRotator(-10.f, 0.f, 0.f));
		}
	}
	for (UInstancedStaticMeshComponent* I : { Tiers.Get(), SeatBases.Get(), SeatBacks.Get() })
	{
		I->MarkRenderStateDirty();
	}
}

void AFTCinemaSeating::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() != NM_DedicatedServer)
	{
		BuildCrowd();
	}
}

void AFTCinemaSeating::BuildCrowd()
{
	for (UInstancedStaticMeshComponent* I : { Bodies.Get(), Heads.Get(), Hair.Get() })
	{
		I->ClearInstances();
		I->SetMaterial(0, IsmMaterial());
		I->NumCustomDataFloats = 5;
	}
	// fill order: centre-front seats first, a little randomness so it never looks like a grid
	struct FSeat { int32 R; int32 C; float Key; };
	TArray<FSeat> Seats;
	for (int32 R = 0; R < Rows; ++R)
	{
		for (int32 C = 0; C < Cols; ++C)
		{
			if (!IsCrewSeat(R, C))
			{
				const float Key = FMath::Abs(R - Rows * 0.4f) * 1.f + FMath::Abs(C - (Cols - 1) * 0.5f) * 0.45f + Hash01(R, C) * 3.f;
				Seats.Add({ R, C, Key });
			}
		}
	}
	Seats.Sort([](const FSeat& A, const FSeat& B) { return A.Key < B.Key; });
	const FLinearColor Shirts[] = { Coral, Teal, Yellow, Blue, Magenta, Cream, Green, Orange, Purple, NavyLight };
	const FLinearColor Skins[] = { SkinA, SkinB, SkinC, SkinD };
	const FLinearColor Hairs[] = { HairBrown, HairDark, HairGinger, HairBlack, Grey };
	Members.Reset();
	for (const FSeat& S : Seats)
	{
		FMember M;
		M.Seat = SeatLocal(S.R, S.C);
		M.Phase = Hash01(S.C * 7, S.R * 13) * 6.28f;
		M.Energy = 0.7f + Hash01(S.R, S.C * 3) * 0.6f;
		M.BodyH = 54.f + Hash01(S.C, S.R * 5) * 14.f;
		const int32 H = FMath::FloorToInt(Hash01(S.R * 3, S.C) * 1000.f);
		// hidden (zero scale) until the release fills the seat
		const FTransform Hidden(FRotator::ZeroRotator, M.Seat, FVector(0.001f));
		const int32 BI = Bodies->AddInstance(Hidden, false);
		const FLinearColor Shirt = Shirts[H % UE_ARRAY_COUNT(Shirts)];
		Bodies->SetCustomData(BI, { Shirt.R, Shirt.G, Shirt.B, 0.f, 0.f }, false);
		const int32 HI = Heads->AddInstance(Hidden, false);
		const FLinearColor Skin = Skins[(H / 7) % UE_ARRAY_COUNT(Skins)];
		Heads->SetCustomData(HI, { Skin.R, Skin.G, Skin.B, 0.f, 0.f }, false);
		const int32 HR = Hair->AddInstance(Hidden, false);
		const FLinearColor HairC = Hairs[(H / 31) % UE_ARRAY_COUNT(Hairs)];
		Hair->SetCustomData(HR, { HairC.R, HairC.G, HairC.B, 0.f, 0.f }, false);
		Members.Add(M);
	}
	BodyScratch.SetNum(Members.Num());
	HeadScratch.SetNum(Members.Num());
	HairScratch.SetNum(Members.Num());
	AppliedShown = -1;
	Shown = 0.f;
}

int32 AFTCinemaSeating::ComputeTarget() const
{
	const AFTGameState* G = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
	if (!G || !G->LastRelease.bValid || (G->ShootPhase != EFTShootPhase::Premiere && G->ShootPhase != EFTShootPhase::Results))
	{
		return 0;
	}
	const float Fill = FFTEconomy::FillRatio(G->LastRelease.Audience, *UFTEconomyConfig::Get());
	return FMath::Clamp(FMath::RoundToInt(Fill * GetAudienceSeats()), 0, GetAudienceSeats());
}

void AFTCinemaSeating::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Target = ComputeTarget();
	if (GetNetMode() == NM_DedicatedServer)
	{
		Shown = Target;
		return;
	}
	UpdateCrowd(DeltaSeconds);
}

void AFTCinemaSeating::UpdateCrowd(float DeltaSeconds)
{
	if (Members.Num() == 0)
	{
		return;
	}
	// people stream in (about 60 per second) or leave between shows
	Shown = Target > Shown ? FMath::Min((float)Target, Shown + 60.f * DeltaSeconds) : (float)Target;
	const int32 NumShown = FMath::Clamp(FMath::FloorToInt(Shown), 0, Members.Num());

	// excitement follows the show: murmur during the scenes, a roar for a hit on the box-office card
	const AFTGameState* G = GetWorld()->GetGameState<AFTGameState>();
	const UFTFilmDefinition* Film = G ? G->GetFilm() : nullptr;
	float Want = 0.1f;
	if (G && Film && G->LastRelease.bValid)
	{
		const int32 Scenes = Film->Scenes.Num();
		const float T = G->GetServerWorldTimeSeconds() - G->PremiereStartTime;
		const int32 Card = G->ShootPhase == EFTShootPhase::Results ? Scenes + 1 : (T < 8.f ? -1 : FMath::Min(FMath::FloorToInt((T - 8.f) / 6.f), Scenes + 1));
		const float Stars = G->LastRelease.Stars;
		if (Card == Scenes)
		{
			Want = 0.45f;
		}
		else if (Card > Scenes)
		{
			Want = 0.15f + Stars * 0.17f;
		}
		else if (Card >= 0)
		{
			Want = 0.12f + (FMath::Fmod(T - 8.f, 6.f) > 1.5f && FMath::Fmod(T - 8.f, 6.f) < 3.f ? 0.2f : 0.f);
		}
		if (Card > Scenes && LastCheerCard != Card && NumShown > 0)
		{
			// the hall's own applause, as loud as the verdict deserves
			FTAudio::PlayAt(this, Stars >= 3 ? EFTSound::ApplauseBig : EFTSound::ApplauseSmall, GetHallCenter(), FMath::Clamp(0.25f + Stars * 0.15f, 0.3f, 1.f));
			if (Stars <= 1)
			{
				FTAudio::PlayAt(this, EFTSound::Groan, GetHallCenter(), 0.6f);
			}
		}
		LastCheerCard = Card;
	}
	Excite = FMath::FInterpTo(Excite, Want, DeltaSeconds, 2.f);

	AnimTimer += DeltaSeconds;
	if (AnimTimer < 1.f / 20.f && NumShown == AppliedShown)
	{
		return;
	}
	AnimTimer = 0.f;
	const float Now = GetWorld()->GetTimeSeconds();
	for (int32 i = 0; i < Members.Num(); ++i)
	{
		const FMember& M = Members[i];
		if (i >= NumShown)
		{
			BodyScratch[i] = HeadScratch[i] = HairScratch[i] = FTransform(FRotator::ZeroRotator, M.Seat, FVector(0.001f));
			continue;
		}
		// bounce + sway; at high excitement they jump out of their seats
		const float Wave = FMath::Sin(Now * (3.f + Excite * 9.f) * M.Energy + M.Phase);
		const float Lift = Excite > 0.55f ? FMath::Max(0.f, Wave) * Excite * 38.f * M.Energy : Wave * Excite * 5.f;
		const float Sway = FMath::Sin(Now * 1.3f + M.Phase) * (2.f + Excite * 6.f);
		const FVector Base = M.Seat + FVector(4.f, 0.f, 44.f + Lift);
		BodyScratch[i] = FTransform(FRotator(0.f, 0.f, Sway), Base + FVector(0.f, 0.f, M.BodyH * 0.5f), FVector(0.36f, 0.42f, M.BodyH / 100.f));
		const FVector HeadC = Base + FVector(-2.f, 0.f, M.BodyH + 12.f);
		HeadScratch[i] = FTransform(FRotator(Excite * 6.f * Wave, 0.f, Sway), HeadC, FVector(0.3f));
		HairScratch[i] = FTransform(FRotator(0.f, 0.f, Sway), HeadC + FVector(4.f, 0.f, 8.f), FVector(0.32f, 0.32f, 0.2f));
	}
	Bodies->BatchUpdateInstancesTransforms(0, BodyScratch, false, true);
	Heads->BatchUpdateInstancesTransforms(0, HeadScratch, false, true);
	Hair->BatchUpdateInstancesTransforms(0, HairScratch, false, true);
	AppliedShown = NumShown;
}

FTransform AFTCinemaSeating::GetCrewSeat(int32 Index) const
{
	const int32 Col = CrewFirstSeat + FMath::Clamp(Index, 0, 3);
	const FVector Local = SeatLocal(CrewRow, Col) + FVector(0.f, 0.f, 95.f);
	return FTransform(GetActorRotation() + FRotator(0.f, 180.f, 0.f), GetActorTransform().TransformPosition(Local));
}

FVector AFTCinemaSeating::GetHallCenter() const
{
	return GetActorTransform().TransformPosition(FVector(Rows * RowSpacing * 0.5f, (Cols - 1) * SeatSpacing * 0.5f, 300.f));
}

// ============================================================================ film case

AFTFilmCase::AFTFilmCase()
{
	PropTag = TEXT("Prop.FilmCase");
	PropName = LOCTEXT("FilmCase", "Film Case");
	Tags = { TEXT("Prop.FilmCase") };
	bCritical = true;
	FloatSink = 14.f;
	CarryOffset = FVector(16.f, 0.f, -30.f);
	auto Part = [this](const TCHAR* Name, EFTShape Shape, FVector Loc, FVector Size, FLinearColor C, FRotator R = FRotator::ZeroRotator, float Glow = 0.f)
	{
		return FTVis::MakePart(this, Visual, Name, Shape, Loc, Size, C, R, Glow);
	};
	Part(TEXT("Body"), EFTShape::Box, FVector(0.f, 0.f, 18.f), FVector(84.f, 50.f, 36.f), Grey);
	Part(TEXT("Lid"), EFTShape::Box, FVector(0.f, 0.f, 38.f), FVector(86.f, 52.f, 6.f), GreyDark);
	Part(TEXT("Stripe"), EFTShape::Box, FVector(0.f, 0.f, 18.f), FVector(86.f, 52.f, 8.f), Coral);
	Part(TEXT("Handle"), EFTShape::Torus, FVector(0.f, 0.f, 46.f), FVector(30.f, 12.f, 14.f), Charcoal, FRotator(0.f, 0.f, 90.f));
	for (float X : { -38.f, 38.f })
	{
		for (float Y : { -22.f, 22.f })
		{
			Part(*FString::Printf(TEXT("Corner%d%d"), (int32)X, (int32)Y), EFTShape::Box, FVector(X, Y, 18.f), FVector(10.f, 10.f, 38.f), Brass);
		}
	}
	for (int32 i = 0; i < 3; ++i)
	{
		UStaticMeshComponent* D = Part(*FString::Printf(TEXT("Reel%d"), i), EFTShape::Torus, FVector(-24.f + i * 24.f, 0.f, 44.f), FVector(22.f, 22.f, 6.f), Cream, FRotator::ZeroRotator, 0.5f);
		D->SetVisibility(false);
		ReelDiscs.Add(D);
	}
	Label = FTVis::MakeText(this, Visual, TEXT("Label"), LOCTEXT("CaseLabel", "FILM - HANDLE WITH CARE"), FVector(0.f, 26.f, 18.f), FRotator(0.f, 90.f, 0.f), 5.f, FColor(255, 240, 220));
	Grab->SetBoxExtent(FVector(46.f, 30.f, 26.f));
	Grab->SetRelativeLocation(FVector(0.f, 0.f, 22.f));
}

void AFTFilmCase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFTFilmCase, StoredReels);
}

void AFTFilmCase::BeginPlay()
{
	Super::BeginPlay();
	OnRep_Reels();
}

bool AFTFilmCase::CanInteract(const UFTInteractableComponent* Comp, const AFTCharacter* User, FText& OutReason) const
{
	if (User && Cast<AFTProp_Reel>(User->HeldProp))
	{
		if (Carrier && Carrier != User)
		{
			OutReason = LOCTEXT("CaseCarried", "Someone is carrying the case");
			return false;
		}
		return true;
	}
	return Super::CanInteract(Comp, User, OutReason);
}

FText AFTFilmCase::GetPromptVerb(const UFTInteractableComponent* Comp, const AFTCharacter* User) const
{
	if (const AFTProp_Reel* R = User ? Cast<AFTProp_Reel>(User->HeldProp) : nullptr)
	{
		return FText::Format(LOCTEXT("PackVerb", "Pack the scene {0} reel into"), FText::AsNumber(R->ReelSceneIndex + 1));
	}
	return FText::Format(LOCTEXT("CarryCase", "Carry ({0} reel(s) inside)"), FText::AsNumber(StoredReels.Num()));
}

void AFTFilmCase::OnInteract(UFTInteractableComponent* Comp, AFTCharacter* User)
{
	if (AFTProp_Reel* R = User ? Cast<AFTProp_Reel>(User->HeldProp) : nullptr)
	{
		User->HeldProp = nullptr;
		User->ForceNetUpdate();
		PackReel(R);
		return;
	}
	Super::OnInteract(Comp, User);
}

bool AFTFilmCase::PackReel(AFTProp_Reel* Reel)
{
	if (!HasAuthority() || !Reel || StoredReels.Contains(Reel->ReelSceneIndex))
	{
		return false;
	}
	StoredReels.Add(Reel->ReelSceneIndex);
	StoredReels.Sort();
	Reel->Destroy();
	OnRep_Reels();
	PublishCount();
	MulticastSound(EFTSound::Stamp, GetActorLocation() + FVector(0.f, 0.f, 40.f), 0.8f, 1.2f);
	ForceNetUpdate();
	return true;
}

TArray<int32> AFTFilmCase::TakeReels()
{
	TArray<int32> Out = StoredReels;
	StoredReels.Reset();
	OnRep_Reels();
	PublishCount();
	ForceNetUpdate();
	return Out;
}

void AFTFilmCase::PublishCount()
{
	if (AFTGameState* G = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr)
	{
		G->ReelsPacked = StoredReels.Num();
		G->NotifyStateChanged();
	}
}

void AFTFilmCase::OnRep_Reels()
{
	for (int32 i = 0; i < ReelDiscs.Num(); ++i)
	{
		ReelDiscs[i]->SetVisibility(i < StoredReels.Num());
	}
	Label->SetText(StoredReels.Num() > 0 ? FText::Format(LOCTEXT("CaseLabelN", "{0} REEL(S) - TO THE GRAND CINEMA"), FText::AsNumber(StoredReels.Num())) : LOCTEXT("CaseLabel2", "FILM - HANDLE WITH CARE"));
}

void AFTFilmCase::ResetForNewShoot()
{
	StoredReels.Reset();
	OnRep_Reels();
	PublishCount();
	Super::ResetForNewShoot();
}

#undef LOCTEXT_NAMESPACE
