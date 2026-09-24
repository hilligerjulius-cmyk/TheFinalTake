// Automation tests for the career economy. Run headless with:
// UnrealEditor-Cmd <uproject> -ExecCmds="Automation RunTests FinalTake.Career; Quit" -unattended -nullrhi -nosound

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "TheFinalTake/Career/FTCareerSave.h"
#include "TheFinalTake/Career/FTEconomy.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags FTTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	/** Config copy without the cosmetic word-of-mouth swing, so numbers are exact. */
	UFTEconomyConfig* ExactConfig()
	{
		UFTEconomyConfig* Cfg = DuplicateObject<UFTEconomyConfig>(GetDefault<UFTEconomyConfig>(), GetTransientPackage());
		Cfg->WordOfMouthPct = 0.f;
		return Cfg;
	}

	FFTReleaseInput Input(TArray<int32> Scores)
	{
		FFTReleaseInput In;
		In.FilmId = TEXT("JawsOfTheStudio");
		In.FilmTitle = FText::FromString(TEXT("JAWS OF THE STUDIO"));
		In.SceneScores = Scores;
		for (int32 i = 0; i < Scores.Num(); ++i)
		{
			In.SceneTitles.Add(FText::FromString(FString::Printf(TEXT("Scene %d"), i + 1)));
		}
		In.StudioCondition = 100.f;
		In.Seed = 1234;
		return In;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFTReleaseFormulaTest, "FinalTake.Career.ReleaseFormula", FTTestFlags)
bool FFTReleaseFormulaTest::RunTest(const FString& Parameters)
{
	UFTEconomyConfig* Cfg = ExactConfig();
	const double Core = 800.0 + 9000.0 * FMath::Pow(0.8, 1.6);

	// known input: three takes of 80, no upgrades, premiere at dawn, undamaged studio
	const FFTReleaseReport R = FFTEconomy::ComputeRelease(Input({ 80, 80, 80 }), *Cfg);
	TestEqual(TEXT("quality is the mean take score"), R.Quality, 80);
	TestEqual(TEXT("audience = 800 + 9000 * 0.8^1.6"), R.Audience, 7098);
	TestEqual(TEXT("revenue = audience * $10 * 50%"), R.Revenue, 35490);
	TestEqual(TEXT("4 stars at quality 80"), R.Stars, 4);
	TestEqual(TEXT("7,098 viewers is a solid run"), R.Tier, EFTBoxOfficeTier::Solid);
	int32 LineSum = 0;
	for (const FFTReleaseLine& L : R.Lines)
	{
		LineSum += L.Audience;
	}
	TestEqual(TEXT("breakdown lines add up to the audience"), LineSum, R.Audience);

	// better takes -> more viewers, but never exponential blow-ups
	const FFTReleaseReport Better = FFTEconomy::ComputeRelease(Input({ 95, 95, 95 }), *Cfg);
	TestTrue(TEXT("higher quality draws more viewers"), Better.Audience > R.Audience);
	TestTrue(TEXT("quality 95 stays below 1.4x of quality 80"), Better.Audience < R.Audience * 1.4f);
	const FFTReleaseReport Bad = FFTEconomy::ComputeRelease(Input({ 20, 0, 30 }), *Cfg);
	TestTrue(TEXT("a bad film still earns something (no dead end)"), Bad.Revenue > 0);

	// production value: counted, but capped
	FFTReleaseInput Prod = Input({ 80, 80, 80 });
	Prod.ProductionPoints = 10;
	Prod.VisibleItems = { TEXT("Fx.Pyro") };
	const FFTReleaseReport WithProd = FFTEconomy::ComputeRelease(Prod, *Cfg);
	TestEqual(TEXT("+10% for 10 production points"), WithProd.Audience, (int32)FMath::RoundToInt(Core * 1.10));
	Prod.ProductionPoints = 500;
	const FFTReleaseReport Capped = FFTEconomy::ComputeRelease(Prod, *Cfg);
	TestEqual(TEXT("production points are capped"), Capped.ProductionPoints, Cfg->MaxProductionPoints);
	TestEqual(TEXT("production bonus stops at +35%"), Capped.Audience, (int32)FMath::RoundToInt(Core * 1.35));

	// punctuality: linear up to half the night left, then capped at +15%
	FFTReleaseInput Early = Input({ 80, 80, 80 });
	Early.RemainingFraction = 0.25f;
	TestEqual(TEXT("quarter night left = +8% (7.5 rounded)"), FFTEconomy::ComputeRelease(Early, *Cfg).ArrivalBonusPct, 8);
	Early.RemainingFraction = 1.f;
	const FFTReleaseReport Fast = FFTEconomy::ComputeRelease(Early, *Cfg);
	TestEqual(TEXT("arrival bonus capped at 15%"), Fast.ArrivalBonusPct, 15);
	TestEqual(TEXT("capped arrival audience"), Fast.Audience, (int32)FMath::RoundToInt(Core * 1.15));

	// repeated releases of the same film fade, but not below 60%
	FFTReleaseInput Sequel = Input({ 80, 80, 80 });
	Sequel.TimesReleasedBefore = 20;
	TestEqual(TEXT("sequel fatigue floor"), FFTEconomy::ComputeRelease(Sequel, *Cfg).Audience, (int32)FMath::RoundToInt(Core * 0.6));

	// studio damage below 60 condition
	FFTReleaseInput Damaged = Input({ 80, 80, 80 });
	Damaged.StudioCondition = 40.f;
	TestEqual(TEXT("-10% at condition 40"), FFTEconomy::ComputeRelease(Damaged, *Cfg).Audience, (int32)FMath::RoundToInt(Core * 0.9));

	// weak film + lots of visible production value = cult classic
	FFTReleaseInput Cult = Input({ 50, 50, 50 });
	Cult.ProductionPoints = 14;
	TestEqual(TEXT("cult classic"), FFTEconomy::ComputeRelease(Cult, *Cfg).Tier, EFTBoxOfficeTier::CultClassic);

	// the cosmetic swing is seeded and small
	const UFTEconomyConfig* Real = GetDefault<UFTEconomyConfig>();
	const FFTReleaseReport A = FFTEconomy::ComputeRelease(Input({ 80, 80, 80 }), *Real);
	const FFTReleaseReport B = FFTEconomy::ComputeRelease(Input({ 80, 80, 80 }), *Real);
	TestEqual(TEXT("same seed, same result"), A.Audience, B.Audience);
	TestTrue(TEXT("word of mouth stays within +/-2%"), FMath::Abs(A.Audience - 7098) <= 7098 * 0.021f);

	// ranking among earlier releases
	TArray<FFTReleasedFilm> Earlier;
	FFTReleasedFilm Big;
	Big.Revenue = 90000;
	FFTReleasedFilm Small;
	Small.Revenue = 1000;
	Earlier = { Big, Small };
	FFTReleaseReport Ranked = R;
	FFTEconomy::RankRelease(Ranked, Earlier);
	TestEqual(TEXT("rank 2 of 3"), Ranked.Rank, 2);
	TestEqual(TEXT("ranked among 3"), Ranked.RankOf, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFTReviewTest, "FinalTake.Career.Reviews", FTTestFlags)
bool FFTReviewTest::RunTest(const FString& Parameters)
{
	UFTEconomyConfig* Cfg = ExactConfig();
	const FFTReleaseReport Great = FFTEconomy::ComputeRelease(Input({ 95, 90, 92 }), *Cfg);
	TestTrue(TEXT("great film gets praise"), Great.Reviews[0].ToString().Contains(TEXT("triumph")));
	TestTrue(TEXT("no upgrades seen -> bare sets"), Great.Reviews[1].ToString().Contains(TEXT("Bare sets")));
	TestTrue(TEXT("late premiere is mentioned"), Great.Reviews[2].ToString().Contains(TEXT("sun came up")));

	FFTReleaseInput PoorIn = Input({ 20, 25, 30 });
	PoorIn.ProductionPoints = 25;
	PoorIn.VisibleItems = { TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D"), TEXT("E") };
	PoorIn.RemainingFraction = 0.6f;
	const FFTReleaseReport Poor = FFTEconomy::ComputeRelease(PoorIn, *Cfg);
	TestTrue(TEXT("weak film is not praised"), !Poor.Reviews[0].ToString().Contains(TEXT("triumph")));
	TestTrue(TEXT("rich production is praised"), Poor.Reviews[1].ToString().Contains(TEXT("Lavish")));
	TestTrue(TEXT("punctual premiere is praised"), Poor.Reviews[2].ToString().Contains(TEXT("schedule")));
	TestEqual(TEXT("3 reviews with sources"), Poor.ReviewSources.Num(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFTLedgerTest, "FinalTake.Career.Ledger", FTTestFlags)
bool FFTLedgerTest::RunTest(const FString& Parameters)
{
	const UFTEconomyConfig* Cfg = GetDefault<UFTEconomyConfig>();
	UFTCareerSave* Save = UFTCareerSave::CreateFresh(97, *Cfg);
	TestEqual(TEXT("starting money"), Save->Money, 5000);
	TestTrue(TEXT("starter van owned"), FFTCareerLedger::Owns(*Save, FTCareerIds::StarterVehicle));
	TestTrue(TEXT("stage 4 unlocked"), FFTCareerLedger::Owns(*Save, FTCareerIds::Stage4));

	int32 Price = 0;
	TestEqual(TEXT("valid purchase"), FFTCareerLedger::Purchase(*Save, *Cfg, TEXT("Acc.StarShades"), Price), EFTPurchaseResult::Ok);
	TestEqual(TEXT("price deducted"), Save->Money, 2500);
	// a double click / second player buying the same thing at the same moment
	TestEqual(TEXT("duplicate purchase rejected"), FFTCareerLedger::Purchase(*Save, *Cfg, TEXT("Acc.StarShades"), Price), EFTPurchaseResult::AlreadyOwned);
	TestEqual(TEXT("duplicate costs nothing"), Save->Money, 2500);
	TestEqual(TEXT("too expensive"), FFTCareerLedger::Purchase(*Save, *Cfg, TEXT("Fx.Pyro"), Price), EFTPurchaseResult::NotEnoughMoney);
	TestEqual(TEXT("unknown id"), FFTCareerLedger::Purchase(*Save, *Cfg, TEXT("Hack.FreeMoney"), Price), EFTPurchaseResult::UnknownItem);
	TestEqual(TEXT("starter car cannot be rebought"), FFTCareerLedger::Purchase(*Save, *Cfg, FTCareerIds::StarterVehicle, Price), EFTPurchaseResult::AlreadyOwned);
	TestTrue(TEXT("balance never negative"), Save->Money >= 0);

	FFTReleaseReport Rep;
	Rep.bValid = true;
	Rep.Revenue = 60000;
	Rep.ReleaseNumber = 1;
	TestTrue(TEXT("release booked"), FFTCareerLedger::BookRelease(*Save, Rep));
	TestEqual(TEXT("revenue credited"), Save->Money, 62500);
	TestFalse(TEXT("same release cannot be booked twice"), FFTCareerLedger::BookRelease(*Save, Rep));
	TestEqual(TEXT("no double credit"), Save->Money, 62500);
	TestEqual(TEXT("stage purchase"), FFTCareerLedger::Purchase(*Save, *Cfg, FTCareerIds::Stage6, Price), EFTPurchaseResult::Ok);
	TestEqual(TEXT("stage cost 55,000"), Save->Money, 7500);
	TestTrue(TEXT("stage 6 unlocked"), Save->UnlockedStages.Contains(FTCareerIds::Stage6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFTSaveRoundTripTest, "FinalTake.Career.SaveRoundTrip", FTTestFlags)
bool FFTSaveRoundTripTest::RunTest(const FString& Parameters)
{
	const UFTEconomyConfig* Cfg = GetDefault<UFTEconomyConfig>();
	constexpr int32 Slot = 98;
	UFTCareerSave::DeleteSlot(Slot);
	IFileManager::Get().Delete(*(UFTCareerSave::SlotPath(Slot) + TEXT(".corrupt")), false, true, true);

	FText Notice;
	UFTCareerSave* Save = UFTCareerSave::LoadOrCreate(Slot, *Cfg, Notice);
	TestTrue(TEXT("missing slot -> fresh career"), Save != nullptr && Save->Money == Cfg->StartingMoney);
	int32 Price = 0;
	FFTCareerLedger::Purchase(*Save, *Cfg, TEXT("Acc.DirectorBeret"), Price);
	FFTPlacedItem Placed;
	Placed.ItemId = TEXT("Acc.DirectorBeret");
	Placed.Transform = FTransform(FRotator(0.f, 45.f, 0.f), FVector(100.f, 200.f, 30.f));
	Save->Placements.Add(Placed);
	FFTReleaseReport Rep;
	Rep.bValid = true;
	Rep.Revenue = 1234;
	Rep.ReleaseNumber = 1;
	Rep.FilmId = TEXT("JawsOfTheStudio");
	FFTCareerLedger::BookRelease(*Save, Rep);
	FString Error;
	TestTrue(TEXT("first save"), Save->SaveToSlot(Slot, Error));
	const int32 SavedMoney = Save->Money;
	TestTrue(TEXT("second save (creates .bak)"), Save->SaveToSlot(Slot, Error));

	UFTCareerSave* Loaded = UFTCareerSave::LoadOrCreate(Slot, *Cfg, Notice);
	TestEqual(TEXT("money survives"), Loaded->Money, SavedMoney);
	TestTrue(TEXT("purchase survives"), Loaded->OwnedItems.Contains(TEXT("Acc.DirectorBeret")));
	TestEqual(TEXT("film record survives"), Loaded->Films.Num(), 1);
	TestTrue(TEXT("placement survives"), Loaded->Placements.Num() == 1 && Loaded->Placements[0].Transform.GetLocation().Equals(FVector(100.f, 200.f, 30.f)));
	TestEqual(TEXT("release counter survives"), Loaded->ReleaseCounter, 1);

	// damage the main file: the backup must be used and the game must keep going
	const FString Path = UFTCareerSave::SlotPath(Slot);
	TArray<uint8> Bytes;
	FFileHelper::LoadFileToArray(Bytes, *Path);
	Bytes[Bytes.Num() / 2] ^= 0xFF;
	FFileHelper::SaveArrayToFile(Bytes, *Path);
	UFTCareerSave* Recovered = UFTCareerSave::LoadOrCreate(Slot, *Cfg, Notice);
	TestEqual(TEXT("backup restores the money"), Recovered->Money, SavedMoney);
	TestTrue(TEXT("damaged file kept aside"), IFileManager::Get().FileExists(*(Path + TEXT(".corrupt"))));

	// damage everything: a fresh career instead of a crash
	UFTCareerSave::DeleteSlot(Slot);
	FFileHelper::SaveStringToFile(TEXT("garbage"), *Path);
	UFTCareerSave* Fresh = UFTCareerSave::LoadOrCreate(Slot, *Cfg, Notice);
	TestTrue(TEXT("garbage file -> fresh career"), Fresh != nullptr && Fresh->Money == Cfg->StartingMoney && Fresh->Films.Num() == 0);
	TestFalse(TEXT("user is told what happened"), Notice.IsEmpty());

	// old / impossible data is repaired on load
	UFTCareerSave* Old = UFTCareerSave::CreateFresh(Slot, *Cfg);
	Old->Version = 0;
	Old->Money = -500;
	Old->OwnedItems.Add(TEXT("Removed.OldItem"));
	Old->OwnedVehicles.Reset();
	// write it as an old build would have (no version bump on the way out)
	TArray<uint8> Payload;
	TestTrue(TEXT("serialize old data"), UGameplayStatics::SaveGameToMemory(Old, Payload));
	TestTrue(TEXT("write old data"), UFTCareerSave::WriteValidated(Path, Payload, Error));
	UFTCareerSave* Migrated = UFTCareerSave::LoadOrCreate(Slot, *Cfg, Notice);
	TestTrue(TEXT("migration is announced"), Notice.ToString().Contains(TEXT("upgraded")));
	TestEqual(TEXT("negative money repaired"), Migrated->Money, 0);
	TestFalse(TEXT("unknown items dropped"), Migrated->OwnedItems.Contains(TEXT("Removed.OldItem")));
	TestTrue(TEXT("starter van restored"), Migrated->OwnedVehicles.Contains(FTCareerIds::StarterVehicle));
	TestEqual(TEXT("version upgraded"), Migrated->Version, UFTCareerSave::CurrentVersion);

	UFTCareerSave::DeleteSlot(Slot);
	IFileManager::Get().Delete(*(Path + TEXT(".corrupt")), false, true, true);
	return true;
}

#endif
