#include "TheFinalTake/Career/FTEconomy.h"

#include "TheFinalTake/Career/FTCareerSave.h"
#include "TheFinalTake/Core/FTVisuals.h"

#define LOCTEXT_NAMESPACE "FinalTakeEconomy"

namespace
{
	FFTItemPart P(EFTShape Shape, FVector Loc, FVector Size, FLinearColor Color, FRotator Rot = FRotator::ZeroRotator, float Emissive = 0.f)
	{
		FFTItemPart Part;
		Part.Shape = Shape;
		Part.Location = Loc;
		Part.Size = Size;
		Part.Rotation = Rot;
		Part.Color = Color;
		Part.Emissive = Emissive;
		return Part;
	}

	FFTShopItemDef Item(const TCHAR* Id, const FText& Name, const FText& Desc, EFTShopCategory Cat, int32 Price, int32 Style, FLinearColor Swatch, TArray<FFTItemPart> Parts)
	{
		FFTShopItemDef D;
		D.ItemId = Id;
		D.Name = Name;
		D.Description = Desc;
		D.Category = Cat;
		D.Price = Price;
		D.StyleValue = Style;
		D.Swatch = Swatch;
		D.Parts = MoveTemp(Parts);
		return D;
	}
}

UFTEconomyConfig::UFTEconomyConfig()
{
	using namespace FTColors;

	// ------------------------------------------------------------ wearable accessories (wardrobe mirror)
	// Parts are relative to the wearer's attach point: Head = top of the head, Face = eye line, Body = chest front.
	{
		FFTShopItemDef D = Item(TEXT("Acc.CaptainHat"), LOCTEXT("CaptainHat", "Captain's Hat"),
			LOCTEXT("CaptainHatD", "Salt, braid and authority. Every sea picture needs one."), EFTShopCategory::Costume, 3000, 4, White, {
				P(EFTShape::Cylinder, FVector(0.f, 0.f, 7.f), FVector(31.f, 31.f, 14.f), White),
				P(EFTShape::Cylinder, FVector(0.f, 0.f, 1.f), FVector(32.f, 32.f, 5.f), Navy),
				P(EFTShape::Box, FVector(13.f, 0.f, -2.f), FVector(16.f, 28.f, 3.f), Charcoal, FRotator(-8.f, 0.f, 0.f)),
				P(EFTShape::Box, FVector(16.f, 0.f, 6.f), FVector(2.f, 9.f, 7.f), Yellow, FRotator::ZeroRotator, 1.5f) });
		D.AccessorySlot = EFTAccessorySlot::Head;
		D.SuggestedFilm = TEXT("JawsOfTheStudio");
		D.FrameRadius = 40.f;
		Items.Add(D);
	}
	{
		FFTShopItemDef D = Item(TEXT("Acc.StarShades"), LOCTEXT("StarShades", "Star Sunglasses"),
			LOCTEXT("StarShadesD", "Magenta glam frames. The camera loves them, the crew is blinded."), EFTShopCategory::Costume, 2500, 3, Magenta, {
				P(EFTShape::Box, FVector(0.f, -8.f, 0.f), FVector(3.f, 12.f, 9.f), Magenta, FRotator::ZeroRotator, 0.6f),
				P(EFTShape::Box, FVector(0.f, 8.f, 0.f), FVector(3.f, 12.f, 9.f), Magenta, FRotator::ZeroRotator, 0.6f),
				P(EFTShape::Box, FVector(0.f, 0.f, 2.f), FVector(2.f, 5.f, 2.f), Yellow),
				P(EFTShape::Box, FVector(-8.f, -14.f, 2.f), FVector(16.f, 2.f, 2.f), Yellow),
				P(EFTShape::Box, FVector(-8.f, 14.f, 2.f), FVector(16.f, 2.f, 2.f), Yellow) });
		D.AccessorySlot = EFTAccessorySlot::Face;
		D.FrameRadius = 35.f;
		Items.Add(D);
	}
	{
		FFTShopItemDef D = Item(TEXT("Acc.DirectorBeret"), LOCTEXT("Beret", "Auteur Beret"),
			LOCTEXT("BeretD", "Instantly adds 30% artistic vision. Scientifically unproven."), EFTShopCategory::Costume, 2000, 3, CoralDark, {
				P(EFTShape::Cylinder, FVector(2.f, 0.f, 4.f), FVector(34.f, 34.f, 8.f), CoralDark, FRotator(0.f, 0.f, 10.f)),
				P(EFTShape::Cylinder, FVector(2.f, 3.f, 10.f), FVector(4.f, 4.f, 6.f), Coral) });
		D.AccessorySlot = EFTAccessorySlot::Head;
		D.FrameRadius = 35.f;
		Items.Add(D);
	}
	{
		FFTShopItemDef D = Item(TEXT("Acc.AlienAntennae"), LOCTEXT("Antennae", "Alien Antennae"),
			LOCTEXT("AntennaeD", "Bobbling glow antennae. Perfect for anything from outer space."), EFTShopCategory::Costume, 3500, 4, Green, {
				P(EFTShape::Box, FVector(0.f, 0.f, 1.f), FVector(8.f, 30.f, 3.f), GreenDark),
				P(EFTShape::Cylinder, FVector(0.f, -8.f, 12.f), FVector(3.f, 3.f, 22.f), Green, FRotator(0.f, 0.f, -15.f)),
				P(EFTShape::Cylinder, FVector(0.f, 8.f, 12.f), FVector(3.f, 3.f, 22.f), Green, FRotator(0.f, 0.f, 15.f)),
				P(EFTShape::Sphere, FVector(0.f, -11.f, 24.f), FVector(8.f), Green, FRotator::ZeroRotator, 3.f),
				P(EFTShape::Sphere, FVector(0.f, 11.f, 24.f), FVector(8.f), Green, FRotator::ZeroRotator, 3.f) });
		D.AccessorySlot = EFTAccessorySlot::Head;
		D.SuggestedFilm = TEXT("MoonfallMotel");
		D.FrameRadius = 40.f;
		Items.Add(D);
	}
	{
		FFTShopItemDef D = Item(TEXT("Acc.RoyalCrown"), LOCTEXT("Crown", "Foam Royal Crown"),
			LOCTEXT("CrownD", "Painted foam, real attitude. Heavy is the head that wears it (it is not heavy)."), EFTShopCategory::Costume, 4500, 5, Yellow, {
				P(EFTShape::Cylinder, FVector(0.f, 0.f, 5.f), FVector(27.f, 27.f, 10.f), Yellow, FRotator::ZeroRotator, 0.4f),
				P(EFTShape::Cone, FVector(12.f, 0.f, 15.f), FVector(8.f, 8.f, 12.f), Yellow),
				P(EFTShape::Cone, FVector(-12.f, 0.f, 15.f), FVector(8.f, 8.f, 12.f), Yellow),
				P(EFTShape::Cone, FVector(0.f, 12.f, 15.f), FVector(8.f, 8.f, 12.f), Yellow),
				P(EFTShape::Cone, FVector(0.f, -12.f, 15.f), FVector(8.f, 8.f, 12.f), Yellow),
				P(EFTShape::Sphere, FVector(14.f, 0.f, 6.f), FVector(6.f), Red, FRotator::ZeroRotator, 2.f) });
		D.AccessorySlot = EFTAccessorySlot::Head;
		D.SuggestedFilm = TEXT("CastleOnFire");
		D.FrameRadius = 40.f;
		Items.Add(D);
	}
	{
		FFTShopItemDef D = Item(TEXT("Acc.HeroCape"), LOCTEXT("Cape", "Hero Cape"),
			LOCTEXT("CapeD", "Billows dramatically even with the wind machine off."), EFTShopCategory::Costume, 4000, 5, Red, {
				P(EFTShape::Box, FVector(-14.f, 0.f, -22.f), FVector(4.f, 44.f, 64.f), Red, FRotator(8.f, 0.f, 0.f)),
				P(EFTShape::Box, FVector(-12.f, 0.f, 10.f), FVector(6.f, 46.f, 6.f), Yellow),
				P(EFTShape::Sphere, FVector(6.f, 0.f, 10.f), FVector(8.f), Yellow, FRotator::ZeroRotator, 1.f) });
		D.AccessorySlot = EFTAccessorySlot::Body;
		D.FrameRadius = 55.f;
		Items.Add(D);
	}

	// ------------------------------------------------------------ props (carry onto a set)
	{
		FFTShopItemDef D = Item(TEXT("Prop.TreasureChest"), LOCTEXT("Chest", "Treasure Chest"),
			LOCTEXT("ChestD", "Glows from within. Contents: gold-painted bottle caps."), EFTShopCategory::Prop, 3000, 3, Yellow, {
				P(EFTShape::Box, FVector(0.f, 0.f, 20.f), FVector(62.f, 42.f, 40.f), Wood),
				P(EFTShape::Box, FVector(0.f, 0.f, 22.f), FVector(64.f, 44.f, 6.f), Yellow),
				P(EFTShape::Box, FVector(-4.f, 0.f, 46.f), FVector(64.f, 44.f, 10.f), WoodDark, FRotator(-12.f, 0.f, 0.f)),
				P(EFTShape::Box, FVector(0.f, 0.f, 41.f), FVector(52.f, 34.f, 4.f), Yellow, FRotator::ZeroRotator, 2.f),
				P(EFTShape::Box, FVector(32.f, 0.f, 30.f), FVector(4.f, 9.f, 12.f), Yellow) });
		D.SuggestedFilm = TEXT("JawsOfTheStudio");
		D.FrameRadius = 50.f;
		Items.Add(D);
	}
	{
		FFTShopItemDef D = Item(TEXT("Prop.GiantClam"), LOCTEXT("Clam", "Giant Clam"),
			LOCTEXT("ClamD", "Opens its shell for close-ups. The pearl is a painted ping-pong ball."), EFTShopCategory::Prop, 3500, 4, Magenta, {
				P(EFTShape::Ball, FVector(0.f, 0.f, 14.f), FVector(76.f, 76.f, 28.f), Magenta),
				P(EFTShape::Ball, FVector(-14.f, 0.f, 34.f), FVector(74.f, 74.f, 24.f), Coral, FRotator(-28.f, 0.f, 0.f)),
				P(EFTShape::Sphere, FVector(10.f, 0.f, 26.f), FVector(16.f), Cream, FRotator::ZeroRotator, 2.f) });
		D.SuggestedFilm = TEXT("JawsOfTheStudio");
		D.FrameRadius = 55.f;
		Items.Add(D);
	}
	{
		FFTShopItemDef D = Item(TEXT("Prop.LavaLamp"), LOCTEXT("Lava", "Retro Lava Lamp"),
			LOCTEXT("LavaD", "A glowing blob tower. Makes every motel room look haunted in a good way."), EFTShopCategory::Prop, 2500, 3, Purple, {
				P(EFTShape::Cylinder, FVector(0.f, 0.f, 8.f), FVector(24.f, 24.f, 16.f), Charcoal),
				P(EFTShape::Capsule, FVector(0.f, 0.f, 40.f), FVector(18.f, 18.f, 52.f), Magenta, FRotator::ZeroRotator, 3.f),
				P(EFTShape::Sphere, FVector(0.f, 0.f, 34.f), FVector(10.f), Yellow, FRotator::ZeroRotator, 4.f),
				P(EFTShape::Cone, FVector(0.f, 0.f, 70.f), FVector(16.f, 16.f, 12.f), Charcoal) });
		D.SuggestedFilm = TEXT("MoonfallMotel");
		D.FrameRadius = 40.f;
		Items.Add(D);
	}
	{
		FFTShopItemDef D = Item(TEXT("Prop.FoamSword"), LOCTEXT("Sword", "Legendary Foam Sword"),
			LOCTEXT("SwordD", "Glows at the hilt. Deals zero damage and maximum drama."), EFTShopCategory::Prop, 3000, 4, Cyan, {
				P(EFTShape::Box, FVector(0.f, 0.f, 60.f), FVector(6.f, 14.f, 90.f), Cream),
				P(EFTShape::Prism, FVector(0.f, 0.f, 110.f), FVector(6.f, 14.f, 14.f), Cream),
				P(EFTShape::Box, FVector(0.f, 0.f, 14.f), FVector(10.f, 40.f, 6.f), Yellow),
				P(EFTShape::Cylinder, FVector(0.f, 0.f, 4.f), FVector(8.f, 8.f, 16.f), Cyan, FRotator::ZeroRotator, 3.f) });
		D.SuggestedFilm = TEXT("CastleOnFire");
		D.FrameRadius = 55.f;
		Items.Add(D);
	}

	// ------------------------------------------------------------ practical effects (place + switch on)
	{
		FFTShopItemDef D = Item(TEXT("Fx.Pyro"), LOCTEXT("Pyro", "Cold-Spark Pyro Fountain"),
			LOCTEXT("PyroD", "Stage-safe cold sparks. Looks like fire, feels like glitter."), EFTShopCategory::Effect, 6000, 6, Orange, {
				P(EFTShape::Box, FVector(0.f, 0.f, 10.f), FVector(42.f, 42.f, 20.f), Charcoal),
				P(EFTShape::Box, FVector(0.f, 0.f, 12.f), FVector(44.f, 44.f, 4.f), Yellow),
				P(EFTShape::Cylinder, FVector(0.f, 0.f, 26.f), FVector(14.f, 14.f, 14.f), GreyDark),
				P(EFTShape::Sphere, FVector(0.f, 0.f, 34.f), FVector(8.f), Orange, FRotator::ZeroRotator, 2.f) });
		D.Fx = EFTItemFx::Sparks;
		D.FrameRadius = 120.f;
		Items.Add(D);
	}
	{
		FFTShopItemDef D = Item(TEXT("Fx.Fog"), LOCTEXT("Fog", "Deluxe Ground Fog"),
			LOCTEXT("FogD", "Rolling low fog that hugs the floor instead of hiding the actors."), EFTShopCategory::Effect, 5000, 5, Purple, {
				P(EFTShape::Box, FVector(0.f, 0.f, 20.f), FVector(60.f, 42.f, 40.f), Purple),
				P(EFTShape::Cylinder, FVector(36.f, 0.f, 22.f), FVector(16.f, 16.f, 16.f), Charcoal, FRotator(-90.f, 0.f, 0.f)),
				P(EFTShape::Cylinder, FVector(-12.f, 0.f, 48.f), FVector(18.f, 18.f, 18.f), Cream),
				P(EFTShape::Sphere, FVector(-26.f, 16.f, 36.f), FVector(8.f), Green, FRotator::ZeroRotator, 2.f) });
		D.Fx = EFTItemFx::Fog;
		D.FrameRadius = 160.f;
		Items.Add(D);
	}
	{
		FFTShopItemDef D = Item(TEXT("Fx.Confetti"), LOCTEXT("Confetti", "Confetti Cannon"),
			LOCTEXT("ConfettiD", "Paper storm on demand. Sweeping it up is somebody else's problem."), EFTShopCategory::Effect, 4000, 4, Coral, {
				P(EFTShape::Box, FVector(0.f, 0.f, 6.f), FVector(42.f, 42.f, 12.f), Charcoal),
				P(EFTShape::Cylinder, FVector(8.f, 0.f, 42.f), FVector(26.f, 26.f, 70.f), Coral, FRotator(-20.f, 0.f, 0.f)),
				P(EFTShape::Cylinder, FVector(20.f, 0.f, 74.f), FVector(28.f, 28.f, 6.f), Yellow, FRotator(-20.f, 0.f, 0.f)) });
		D.Fx = EFTItemFx::Confetti;
		D.FrameRadius = 140.f;
		Items.Add(D);
	}
	{
		FFTShopItemDef D = Item(TEXT("Fx.Bubbles"), LOCTEXT("Bubbles", "Bubble Machine"),
			LOCTEXT("BubblesD", "Instant underwater look. Also instant office party."), EFTShopCategory::Effect, 3000, 3, Cyan, {
				P(EFTShape::Box, FVector(0.f, 0.f, 18.f), FVector(40.f, 40.f, 36.f), Cyan),
				P(EFTShape::Torus, FVector(10.f, 0.f, 46.f), FVector(26.f, 26.f, 6.f), Cream, FRotator(0.f, 0.f, 90.f)),
				P(EFTShape::Box, FVector(0.f, 0.f, 37.f), FVector(42.f, 42.f, 4.f), Blue) });
		D.Fx = EFTItemFx::Bubbles;
		D.FrameRadius = 140.f;
		Items.Add(D);
	}

	// ------------------------------------------------------------ shark rig kits (install at the rig desk)
	// Parts are relative to the rig's shark body (snout along +X, see FFTSharkParts).
	{
		FFTShopItemDef D = Item(TEXT("Shark.ChromeTeeth"), LOCTEXT("Chrome", "Chrome Teeth Kit"),
			LOCTEXT("ChromeD", "Bigger, shinier teeth. The rubber shark finally looks hungry."), EFTShopCategory::SharkUpgrade, 4000, 4, Grey, {
				P(EFTShape::Cone, FVector(192.f, -30.f, -16.f), FVector(20.f, 20.f, 30.f), White, FRotator(180.f, 0.f, 0.f), 1.5f),
				P(EFTShape::Cone, FVector(196.f, -10.f, -16.f), FVector(20.f, 20.f, 32.f), White, FRotator(180.f, 0.f, 0.f), 1.5f),
				P(EFTShape::Cone, FVector(196.f, 10.f, -16.f), FVector(20.f, 20.f, 32.f), White, FRotator(180.f, 0.f, 0.f), 1.5f),
				P(EFTShape::Cone, FVector(192.f, 30.f, -16.f), FVector(20.f, 20.f, 30.f), White, FRotator(180.f, 0.f, 0.f), 1.5f) });
		D.SuggestedFilm = TEXT("JawsOfTheStudio");
		D.FrameRadius = 200.f;
		Items.Add(D);
	}
	{
		FFTShopItemDef D = Item(TEXT("Shark.GlowEyes"), LOCTEXT("GlowEyes", "Evil Glow Eyes"),
			LOCTEXT("GlowEyesD", "Red LED eyes. Reads beautifully in the dark flood scenes."), EFTShopCategory::SharkUpgrade, 3500, 4, Red, {
				P(EFTShape::Sphere, FVector(164.f, -58.f, 47.f), FVector(15.f, 9.f, 18.f), Red, FRotator::ZeroRotator, 8.f),
				P(EFTShape::Sphere, FVector(164.f, 58.f, 47.f), FVector(15.f, 9.f, 18.f), Red, FRotator::ZeroRotator, 8.f) });
		D.SuggestedFilm = TEXT("JawsOfTheStudio");
		D.FrameRadius = 200.f;
		Items.Add(D);
	}
	{
		FFTShopItemDef D = Item(TEXT("Shark.BattleScars"), LOCTEXT("Scars", "Battle Scar Paint Job"),
			LOCTEXT("ScarsD", "A veteran of many films. And one car wash."), EFTShopCategory::SharkUpgrade, 3000, 3, Cream, {
				P(EFTShape::Box, FVector(40.f, -80.f, 36.f), FVector(46.f, 3.f, 6.f), Cream, FRotator(0.f, 0.f, 0.f)),
				P(EFTShape::Box, FVector(10.f, -82.f, 20.f), FVector(40.f, 3.f, 6.f), Cream, FRotator(12.f, 0.f, 0.f)),
				P(EFTShape::Box, FVector(30.f, 80.f, 28.f), FVector(50.f, 3.f, 6.f), Cream, FRotator(-10.f, 0.f, 0.f)),
				P(EFTShape::Box, FVector(-30.f, 0.f, 80.f), FVector(30.f, 6.f, 3.f), Cream) });
		D.SuggestedFilm = TEXT("JawsOfTheStudio");
		D.FrameRadius = 200.f;
		Items.Add(D);
	}

	// ------------------------------------------------------------ set pieces (carry + place)
	{
		FFTShopItemDef D = Item(TEXT("Set.TikiTorches"), LOCTEXT("Tiki", "Tiki Torch Pair"),
			LOCTEXT("TikiD", "Two glowing torches with fake flames. Beach scenes, but make it epic."), EFTShopCategory::SetPiece, 4000, 4, Orange, {
				P(EFTShape::Cylinder, FVector(0.f, -55.f, 60.f), FVector(9.f, 9.f, 120.f), WoodDark),
				P(EFTShape::Cylinder, FVector(0.f, 55.f, 60.f), FVector(9.f, 9.f, 120.f), WoodDark),
				P(EFTShape::Cylinder, FVector(0.f, -55.f, 124.f), FVector(20.f, 20.f, 16.f), Wood),
				P(EFTShape::Cylinder, FVector(0.f, 55.f, 124.f), FVector(20.f, 20.f, 16.f), Wood),
				P(EFTShape::Cone, FVector(0.f, -55.f, 144.f), FVector(18.f, 18.f, 28.f), Orange, FRotator::ZeroRotator, 5.f),
				P(EFTShape::Cone, FVector(0.f, 55.f, 144.f), FVector(18.f, 18.f, 28.f), Orange, FRotator::ZeroRotator, 5.f),
				P(EFTShape::Box, FVector(0.f, 0.f, 4.f), FVector(30.f, 140.f, 8.f), WoodDark) });
		D.Fx = EFTItemFx::Glow;
		D.SuggestedFilm = TEXT("JawsOfTheStudio");
		D.FrameRadius = 100.f;
		Items.Add(D);
	}
	{
		FFTShopItemDef D = Item(TEXT("Set.NeonPalm"), LOCTEXT("NeonPalm", "Neon Palm Sign"),
			LOCTEXT("NeonPalmD", "Buzzing neon on a stand. Works for beaches and roadside motels alike."), EFTShopCategory::SetPiece, 5500, 5, Cyan, {
				P(EFTShape::Box, FVector(0.f, 0.f, 120.f), FVector(8.f, 130.f, 100.f), Navy),
				P(EFTShape::Box, FVector(6.f, 0.f, 104.f), FVector(3.f, 8.f, 64.f), Cyan, FRotator::ZeroRotator, 6.f),
				P(EFTShape::Box, FVector(6.f, -22.f, 140.f), FVector(3.f, 44.f, 8.f), Green, FRotator(0.f, 0.f, 20.f), 6.f),
				P(EFTShape::Box, FVector(6.f, 22.f, 140.f), FVector(3.f, 44.f, 8.f), Green, FRotator(0.f, 0.f, -20.f), 6.f),
				P(EFTShape::Box, FVector(6.f, 0.f, 150.f), FVector(3.f, 8.f, 30.f), Green, FRotator::ZeroRotator, 6.f),
				P(EFTShape::Cylinder, FVector(0.f, -50.f, 36.f), FVector(7.f, 7.f, 72.f), Charcoal),
				P(EFTShape::Cylinder, FVector(0.f, 50.f, 36.f), FVector(7.f, 7.f, 72.f), Charcoal) });
		D.Fx = EFTItemFx::Glow;
		D.FrameRadius = 100.f;
		Items.Add(D);
	}
	{
		FFTShopItemDef D = Item(TEXT("Set.ShipwreckBow"), LOCTEXT("Wreck", "Shipwreck Bow"),
			LOCTEXT("WreckD", "Half a cardboard ship, all of the atmosphere."), EFTShopCategory::SetPiece, 7000, 6, Wood, {
				P(EFTShape::Ramp, FVector(0.f, 0.f, 50.f), FVector(160.f, 110.f, 100.f), Wood, FRotator(0.f, 0.f, 0.f)),
				P(EFTShape::Box, FVector(-30.f, 0.f, 98.f), FVector(110.f, 116.f, 8.f), WoodDark),
				P(EFTShape::Box, FVector(0.f, 0.f, 40.f), FVector(150.f, 114.f, 6.f), CoralDark),
				P(EFTShape::Cylinder, FVector(-40.f, 0.f, 170.f), FVector(10.f, 10.f, 150.f), WoodDark, FRotator(-10.f, 0.f, 0.f)),
				P(EFTShape::Box, FVector(-44.f, 18.f, 220.f), FVector(3.f, 40.f, 28.f), Charcoal) });
		D.SuggestedFilm = TEXT("JawsOfTheStudio");
		D.FrameRadius = 130.f;
		Items.Add(D);
	}
	{
		FFTShopItemDef D = Item(TEXT("Set.CardboardMoon"), LOCTEXT("Moon", "Cardboard Full Moon"),
			LOCTEXT("MoonD", "A glowing moon on a stick. Every night scene is now romantic or terrifying."), EFTShopCategory::SetPiece, 4500, 4, Cream, {
				P(EFTShape::Cylinder, FVector(0.f, 0.f, 170.f), FVector(140.f, 140.f, 8.f), Cream, FRotator(90.f, 0.f, 0.f), 0.8f),
				P(EFTShape::Cylinder, FVector(5.f, -30.f, 190.f), FVector(30.f, 30.f, 3.f), CreamDark, FRotator(90.f, 0.f, 0.f)),
				P(EFTShape::Cylinder, FVector(5.f, 28.f, 150.f), FVector(22.f, 22.f, 3.f), CreamDark, FRotator(90.f, 0.f, 0.f)),
				P(EFTShape::Cylinder, FVector(-8.f, 0.f, 50.f), FVector(8.f, 8.f, 100.f), Wood),
				P(EFTShape::Box, FVector(-8.f, 0.f, 4.f), FVector(60.f, 60.f, 8.f), WoodDark) });
		D.Fx = EFTItemFx::Glow;
		D.FrameRadius = 110.f;
		Items.Add(D);
	}

	// ------------------------------------------------------------ cars (dealer)
	auto Car = [this](const TCHAR* Id, const FText& Name, const FText& Desc, int32 Price, float Speed, float Accel, float Handling, int32 Seats, EFTVehicleStyle Style, FLinearColor Body, FLinearColor Trim)
	{
		FFTVehicleDef V;
		V.VehicleId = Id;
		V.Name = Name;
		V.Description = Desc;
		V.Price = Price;
		V.TopSpeed = Speed;
		V.Acceleration = Accel;
		V.Handling = Handling;
		V.Seats = Seats;
		V.Style = Style;
		V.Body = Body;
		V.Trim = Trim;
		Vehicles.Add(V);
	};
	Car(TEXT("Car.StudioVan"), LOCTEXT("Van", "Studio Van"), LOCTEXT("VanD", "Slow, loyal, smells of gaffer tape. Every crew starts with one."),
		0, 1250.f, 520.f, 58.f, 4, EFTVehicleStyle::Van, Cream, Teal);
	Car(TEXT("Car.CheckerTaxi"), LOCTEXT("Taxi", "Checker Cab"), LOCTEXT("TaxiD", "A proper city cab. Four seats, quick off the line."),
		12000, 1700.f, 820.f, 72.f, 4, EFTVehicleStyle::Taxi, Yellow, Charcoal);
	Car(TEXT("Car.MuscleCar"), LOCTEXT("Muscle", "Coral Muscle Car"), LOCTEXT("MuscleD", "Loud and fast. Only two seats, so bring your best friend and the reels."),
		26000, 2500.f, 1250.f, 84.f, 2, EFTVehicleStyle::Muscle, Coral, Cream);
	Car(TEXT("Car.StarLimo"), LOCTEXT("Limo", "Premiere Limousine"), LOCTEXT("LimoD", "Red-carpet arrival for the whole crew. Long, smooth and surprisingly nimble."),
		38000, 2000.f, 900.f, 60.f, 4, EFTVehicleStyle::Limo, Navy, Magenta);

	// ------------------------------------------------------------ studio expansions
	{
		FFTStageDef S;
		S.StageId = FTCareerIds::Stage5;
		S.Name = LOCTEXT("Stage5", "Stage 5: Moonfall Motel");
		S.Description = LOCTEXT("Stage5D", "Desert-night soundstage with a roadside motel, neon, an UFO rig and a very tired power grid. Unlocks the film MOONFALL MOTEL.");
		S.Price = 30000;
		S.FilmId = TEXT("MoonfallMotel");
		S.Swatch = Purple;
		Stages.Add(S);
	}
	{
		FFTStageDef S;
		S.StageId = FTCareerIds::Stage6;
		S.Name = LOCTEXT("Stage6", "Stage 6: Castle on Fire");
		S.Description = LOCTEXT("Stage6D", "A giant cardboard castle, a catapult, a foam dragon puppet and a sprinkler system with opinions. Unlocks CASTLE ON FIRE.");
		S.Price = 55000;
		S.FilmId = TEXT("CastleOnFire");
		S.Swatch = Coral;
		Stages.Add(S);
	}
}

const UFTEconomyConfig* UFTEconomyConfig::Get()
{
	static TWeakObjectPtr<UFTEconomyConfig> Cached;
	if (Cached.IsValid())
	{
		return Cached.Get();
	}
	UFTEconomyConfig* Asset = LoadObject<UFTEconomyConfig>(nullptr, TEXT("/Game/TheFinalTake/Data/DA_Economy.DA_Economy"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (Asset)
	{
		if (!IsRunningCommandlet() && !Asset->IsRooted())
		{
			Asset->AddToRoot();
		}
		Cached = Asset;
		return Asset;
	}
	return GetDefault<UFTEconomyConfig>();
}

const FFTShopItemDef* UFTEconomyConfig::FindItem(FName ItemId) const
{
	return Items.FindByPredicate([ItemId](const FFTShopItemDef& D) { return D.ItemId == ItemId; });
}

const FFTVehicleDef* UFTEconomyConfig::FindVehicle(FName VehicleId) const
{
	return Vehicles.FindByPredicate([VehicleId](const FFTVehicleDef& D) { return D.VehicleId == VehicleId; });
}

const FFTStageDef* UFTEconomyConfig::FindStage(FName StageId) const
{
	return Stages.FindByPredicate([StageId](const FFTStageDef& D) { return D.StageId == StageId; });
}

const FFTStageDef* UFTEconomyConfig::FindStageForFilm(FName FilmId) const
{
	return Stages.FindByPredicate([FilmId](const FFTStageDef& D) { return D.FilmId == FilmId; });
}

bool UFTEconomyConfig::Describe(FName Id, int32& OutPrice, EFTShopCategory& OutCategory, FText& OutName) const
{
	if (const FFTShopItemDef* I = FindItem(Id))
	{
		OutPrice = I->Price;
		OutCategory = I->Category;
		OutName = I->Name;
		return true;
	}
	if (const FFTVehicleDef* V = FindVehicle(Id))
	{
		OutPrice = V->Price;
		OutCategory = EFTShopCategory::Vehicle;
		OutName = V->Name;
		return true;
	}
	if (const FFTStageDef* S = FindStage(Id))
	{
		OutPrice = S->Price;
		OutCategory = EFTShopCategory::Stage;
		OutName = S->Name;
		return true;
	}
	return false;
}

// ============================================================================ release maths

int32 FFTEconomy::StarsForQuality(int32 Quality)
{
	if (Quality >= 88) return 5;
	if (Quality >= 72) return 4;
	if (Quality >= 55) return 3;
	if (Quality >= 35) return 2;
	return 1;
}

EFTBoxOfficeTier FFTEconomy::TierFor(int32 Audience, int32 Quality, int32 ProductionPoints, const UFTEconomyConfig& Cfg)
{
	if (Quality < Cfg.CultMaxQuality && ProductionPoints >= Cfg.CultMinProduction)
	{
		return EFTBoxOfficeTier::CultClassic;
	}
	if (Audience >= Cfg.BlockbusterAudience) return EFTBoxOfficeTier::Blockbuster;
	if (Audience >= Cfg.HitAudience) return EFTBoxOfficeTier::WeekendHit;
	if (Audience >= Cfg.SolidAudience) return EFTBoxOfficeTier::Solid;
	if (Audience >= Cfg.ModestAudience) return EFTBoxOfficeTier::Modest;
	return EFTBoxOfficeTier::Flop;
}

FText FFTEconomy::TierName(EFTBoxOfficeTier Tier)
{
	switch (Tier)
	{
	case EFTBoxOfficeTier::Blockbuster: return LOCTEXT("TBlock", "BLOCKBUSTER");
	case EFTBoxOfficeTier::WeekendHit: return LOCTEXT("THit", "WEEKEND HIT");
	case EFTBoxOfficeTier::Solid: return LOCTEXT("TSolid", "SOLID RUN");
	case EFTBoxOfficeTier::Modest: return LOCTEXT("TModest", "MODEST RUN");
	case EFTBoxOfficeTier::CultClassic: return LOCTEXT("TCult", "CULT CLASSIC");
	default: return LOCTEXT("TFlop", "FLOP");
	}
}

FText FFTEconomy::CategoryName(EFTShopCategory Category)
{
	switch (Category)
	{
	case EFTShopCategory::Costume: return LOCTEXT("CCostume", "Costumes");
	case EFTShopCategory::Prop: return LOCTEXT("CProp", "Props");
	case EFTShopCategory::Effect: return LOCTEXT("CEffect", "Effects");
	case EFTShopCategory::SharkUpgrade: return LOCTEXT("CShark", "Shark Rig");
	case EFTShopCategory::SetPiece: return LOCTEXT("CSet", "Set Pieces");
	case EFTShopCategory::Vehicle: return LOCTEXT("CCar", "Cars");
	default: return LOCTEXT("CStage", "Studio Expansion");
	}
}

float FFTEconomy::FillRatio(int32 Audience, const UFTEconomyConfig& Cfg)
{
	return FMath::Clamp(static_cast<float>(Audience) / FMath::Max(1, Cfg.FullHouseAudience), 0.04f, 1.f);
}

FString FFTEconomy::NumberString(int64 Amount)
{
	const bool bNeg = Amount < 0;
	FString Digits = FString::Printf(TEXT("%lld"), bNeg ? -Amount : Amount);
	for (int32 i = Digits.Len() - 3; i > 0; i -= 3)
	{
		Digits.InsertAt(i, TEXT(","));
	}
	return bNeg ? TEXT("-") + Digits : Digits;
}

FString FFTEconomy::MoneyString(int64 Amount)
{
	return (Amount < 0 ? TEXT("-$") : TEXT("$")) + NumberString(Amount < 0 ? -Amount : Amount);
}

namespace
{
	FText ReviewForQuality(const FFTReleaseInput& In, int32 Quality, int32 BestIdx, int32 WorstIdx)
	{
		const FText Best = In.SceneTitles.IsValidIndex(BestIdx) ? In.SceneTitles[BestIdx] : FText::GetEmpty();
		const FText Worst = In.SceneTitles.IsValidIndex(WorstIdx) ? In.SceneTitles[WorstIdx] : FText::GetEmpty();
		if (Quality >= 88)
		{
			return FText::Format(LOCTEXT("RQ5", "\"{0}\" is a triumph. \"{1}\" alone is worth the ticket."), In.FilmTitle, Best);
		}
		if (Quality >= 72)
		{
			return FText::Format(LOCTEXT("RQ4", "Confident, funny and loud. \"{0}\" is the scene everyone will quote."), Best);
		}
		if (Quality >= 55)
		{
			return FText::Format(LOCTEXT("RQ3", "Charming chaos. \"{0}\" lands, \"{1}\" wobbles."), Best, Worst);
		}
		if (Quality >= 35)
		{
			return FText::Format(LOCTEXT("RQ2", "Rough around every edge. \"{0}\" felt like a rehearsal."), Worst);
		}
		return FText::Format(LOCTEXT("RQ1", "We are still not sure what we watched. Especially \"{0}\"."), Worst);
	}

	FText ReviewForProduction(int32 Points, int32 ItemCount)
	{
		if (Points >= 20)
		{
			return FText::Format(LOCTEXT("RP3", "Lavish! {0} dazzling production touches fill every frame."), FText::AsNumber(ItemCount));
		}
		if (Points >= 8)
		{
			return FText::Format(LOCTEXT("RP2", "Nice set dressing - {0} upgrades you can actually see on screen."), FText::AsNumber(ItemCount));
		}
		if (Points > 0)
		{
			return LOCTEXT("RP1", "A few shiny extras. The budget shows, a little.");
		}
		return LOCTEXT("RP0", "Bare sets and cardboard. Did they spend the budget on snacks?");
	}

	FText ReviewForEvening(const FFTReleaseInput& In, int32 ArrivalPct)
	{
		if (In.bSurvivedDisaster && ArrivalPct >= 8)
		{
			return LOCTEXT("RE1", "Rumour says the studio really flooded mid-shoot - and they still premiered on time.");
		}
		if (ArrivalPct >= 12)
		{
			return LOCTEXT("RE2", "A red-carpet premiere right on schedule. Classy.");
		}
		if (ArrivalPct <= 2)
		{
			return LOCTEXT("RE3", "The reels arrived as the sun came up. We nearly went home.");
		}
		if (In.StudioCondition < 50.f)
		{
			return LOCTEXT("RE4", "You can hear the studio falling apart in the background. Authentic!");
		}
		if (In.bSurvivedDisaster)
		{
			return LOCTEXT("RE5", "The disaster scenes feel real. Probably because they were.");
		}
		return LOCTEXT("RE6", "A fun night out. The popcorn was great too.");
	}
}

FFTReleaseReport FFTEconomy::ComputeRelease(const FFTReleaseInput& In, const UFTEconomyConfig& Cfg)
{
	FFTReleaseReport R;
	R.bValid = true;
	R.FilmId = In.FilmId;
	R.FilmTitle = In.FilmTitle;
	R.SceneScores = In.SceneScores;
	R.VisibleItems = In.VisibleItems;
	R.TicketPrice = Cfg.TicketPrice;

	// quality: mean of the best take per scene (a scene that was never accepted counts as 0)
	int32 Sum = 0, BestIdx = 0, WorstIdx = 0;
	for (int32 i = 0; i < In.SceneScores.Num(); ++i)
	{
		const int32 S = FMath::Clamp(In.SceneScores[i], 0, 100);
		Sum += S;
		BestIdx = S > In.SceneScores[BestIdx] ? i : BestIdx;
		WorstIdx = S < In.SceneScores[WorstIdx] ? i : WorstIdx;
	}
	R.Quality = In.SceneScores.Num() > 0 ? FMath::RoundToInt(static_cast<float>(Sum) / In.SceneScores.Num()) : 0;
	R.ProductionPoints = FMath::Clamp(In.ProductionPoints, 0, Cfg.MaxProductionPoints);

	// Audience is built step by step; every step is one visible line, so the lines add up exactly.
	double Running = 0.0;
	int32 Shown = 0;
	auto Step = [&](double NewValue, const FText& Label)
	{
		Running = FMath::Max(0.0, NewValue);
		const int32 Rounded = FMath::RoundToInt(Running);
		const int32 Delta = Rounded - Shown;
		Shown = Rounded;
		if (Delta != 0)
		{
			FFTReleaseLine L;
			L.Label = Label;
			L.Audience = Delta;
			R.Lines.Add(L);
		}
	};

	Step(Cfg.BaseAudience, LOCTEXT("LBase", "Opening-night walk-ins"));
	const double QualityTerm = Cfg.QualityAudience * FMath::Pow(R.Quality / 100.0, static_cast<double>(Cfg.QualityExponent));
	Step(Running + QualityTerm, FText::Format(LOCTEXT("LQuality", "Film quality {0}/100 (best takes of {1} scenes)"), FText::AsNumber(R.Quality), FText::AsNumber(In.SceneScores.Num())));
	if (!FMath::IsNearlyEqual(In.GenreMultiplier, 1.f))
	{
		Step(Running * In.GenreMultiplier, FText::Format(LOCTEXT("LGenre", "Genre appeal x{0}"), FText::AsNumber(In.GenreMultiplier)));
	}
	const float ProdPct = FMath::Min(R.ProductionPoints * Cfg.ProductionPctPerPoint, Cfg.MaxProductionPoints * Cfg.ProductionPctPerPoint);
	if (ProdPct > 0.f)
	{
		Step(Running * (1.0 + ProdPct), FText::Format(LOCTEXT("LProd", "Production value +{0}% ({1} upgrades seen in takes)"),
			FText::AsNumber(FMath::RoundToInt(ProdPct * 100.f)), FText::AsNumber(In.VisibleItems.Num())));
	}
	const float Arrival = FMath::Clamp(In.RemainingFraction / FMath::Max(Cfg.ArrivalFullBonusAt, 0.01f), 0.f, 1.f) * Cfg.ArrivalBonusMaxPct;
	R.ArrivalBonusPct = FMath::RoundToInt(Arrival * 100.f);
	if (Arrival > 0.f)
	{
		Step(Running * (1.0 + Arrival), FText::Format(LOCTEXT("LArrival", "Premiere punctuality +{0}% ({1}% of the night left)"),
			FText::AsNumber(R.ArrivalBonusPct), FText::AsNumber(FMath::RoundToInt(In.RemainingFraction * 100.f))));
	}
	if (In.bSurvivedDisaster && Cfg.DisasterBuzzPct > 0.f)
	{
		Step(Running * (1.0 + Cfg.DisasterBuzzPct), FText::Format(LOCTEXT("LBuzz", "Behind-the-scenes buzz +{0}%"), FText::AsNumber(FMath::RoundToInt(Cfg.DisasterBuzzPct * 100.f))));
	}
	const float Damage = FMath::Clamp((Cfg.ConditionPenaltyBelow - In.StudioCondition) * Cfg.ConditionPenaltyPerPoint, 0.f, 0.3f);
	if (Damage > 0.f)
	{
		Step(Running * (1.0 - Damage), FText::Format(LOCTEXT("LDamage", "Studio damage -{0}%"), FText::AsNumber(FMath::RoundToInt(Damage * 100.f))));
	}
	const float Fatigue = FMath::Max(Cfg.MinSequelFactor, 1.f - Cfg.SequelFatiguePerRelease * FMath::Max(0, In.TimesReleasedBefore));
	if (Fatigue < 1.f)
	{
		Step(Running * Fatigue, FText::Format(LOCTEXT("LFatigue", "Seen it before (release #{0} of this film) -{1}%"),
			FText::AsNumber(In.TimesReleasedBefore + 1), FText::AsNumber(FMath::RoundToInt((1.f - Fatigue) * 100.f))));
	}
	FRandomStream Rng(In.Seed);
	const float Word = Rng.FRandRange(-Cfg.WordOfMouthPct, Cfg.WordOfMouthPct);
	Step(Running * (1.0 + Word), LOCTEXT("LWord", "Word of mouth"));

	R.Audience = Shown;
	R.Revenue = FMath::RoundToInt(R.Audience * Cfg.TicketPrice * Cfg.StudioShare);
	R.Stars = StarsForQuality(R.Quality);
	R.Tier = TierFor(R.Audience, R.Quality, R.ProductionPoints, Cfg);

	R.Reviews.Add(ReviewForQuality(In, R.Quality, BestIdx, WorstIdx));
	R.ReviewSources.Add(LOCTEXT("SrcClapper", "The Daily Clapper"));
	R.Reviews.Add(ReviewForProduction(R.ProductionPoints, In.VisibleItems.Num()));
	R.ReviewSources.Add(LOCTEXT("SrcReel", "Reel Talk Weekly"));
	R.Reviews.Add(ReviewForEvening(In, R.ArrivalBonusPct));
	R.ReviewSources.Add(LOCTEXT("SrcPopcorn", "Popcorn Gazette"));
	return R;
}

void FFTEconomy::RankRelease(FFTReleaseReport& Report, const TArray<FFTReleasedFilm>& Previous)
{
	int32 Better = 0;
	for (const FFTReleasedFilm& F : Previous)
	{
		Better += F.Revenue > Report.Revenue ? 1 : 0;
	}
	Report.Rank = Better + 1;
	Report.RankOf = Previous.Num() + 1;
}

// ============================================================================ ledger

bool FFTCareerLedger::Owns(const UFTCareerSave& Save, FName Id)
{
	return Save.OwnedItems.Contains(Id) || Save.OwnedVehicles.Contains(Id) || Save.UnlockedStages.Contains(Id);
}

EFTPurchaseResult FFTCareerLedger::CanPurchase(const UFTCareerSave& Save, const UFTEconomyConfig& Cfg, FName Id, int32& OutPrice, EFTShopCategory& OutCategory)
{
	FText Name;
	if (Id.IsNone() || !Cfg.Describe(Id, OutPrice, OutCategory, Name))
	{
		return EFTPurchaseResult::UnknownItem;
	}
	if (Owns(Save, Id))
	{
		return EFTPurchaseResult::AlreadyOwned;
	}
	if (OutPrice < 0 || Save.Money < OutPrice)
	{
		return EFTPurchaseResult::NotEnoughMoney;
	}
	return EFTPurchaseResult::Ok;
}

EFTPurchaseResult FFTCareerLedger::Purchase(UFTCareerSave& Save, const UFTEconomyConfig& Cfg, FName Id, int32& OutPrice)
{
	EFTShopCategory Cat;
	const EFTPurchaseResult Result = CanPurchase(Save, Cfg, Id, OutPrice, Cat);
	if (Result != EFTPurchaseResult::Ok)
	{
		return Result;
	}
	Save.Money -= OutPrice;
	switch (Cat)
	{
	case EFTShopCategory::Vehicle: Save.OwnedVehicles.AddUnique(Id); break;
	case EFTShopCategory::Stage: Save.UnlockedStages.AddUnique(Id); break;
	default: Save.OwnedItems.AddUnique(Id); break;
	}
	Save.TotalSpent += OutPrice;
	return EFTPurchaseResult::Ok;
}

bool FFTCareerLedger::BookRelease(UFTCareerSave& Save, const FFTReleaseReport& Report)
{
	if (!Report.bValid || Report.ReleaseNumber <= Save.ReleaseCounter)
	{
		return false;
	}
	Save.ReleaseCounter = Report.ReleaseNumber;
	Save.Money += FMath::Max(0, Report.Revenue);
	Save.TotalRevenue += FMath::Max(0, Report.Revenue);
	FFTReleasedFilm F;
	F.FilmId = Report.FilmId;
	F.Title = Report.FilmTitle.ToString();
	F.SceneScores = Report.SceneScores;
	F.Quality = Report.Quality;
	F.ProductionPoints = Report.ProductionPoints;
	F.Audience = Report.Audience;
	F.Revenue = Report.Revenue;
	F.Stars = Report.Stars;
	F.Headline = Report.Reviews.Num() > 0 ? Report.Reviews[0].ToString() : FString();
	F.Tier = Report.Tier;
	F.RankAtRelease = Report.Rank;
	F.ReleaseNumber = Report.ReleaseNumber;
	F.ReleasedAt = FDateTime::UtcNow();
	Save.Films.Add(F);
	return true;
}

FText FFTCareerLedger::ReasonText(EFTPurchaseResult Result, int32 Price, int32 Balance)
{
	switch (Result)
	{
	case EFTPurchaseResult::Ok: return LOCTEXT("POk", "Purchased!");
	case EFTPurchaseResult::UnknownItem: return LOCTEXT("PUnknown", "That item is not in the catalogue");
	case EFTPurchaseResult::AlreadyOwned: return LOCTEXT("POwned", "The studio already owns this");
	case EFTPurchaseResult::NotEnoughMoney:
		return FText::Format(LOCTEXT("PPoor", "Not enough studio money: costs {0}, the studio has {1}"),
			FText::FromString(FFTEconomy::MoneyString(Price)), FText::FromString(FFTEconomy::MoneyString(Balance)));
	default: return LOCTEXT("PNope", "Purchases are not possible right now");
	}
}

#undef LOCTEXT_NAMESPACE
