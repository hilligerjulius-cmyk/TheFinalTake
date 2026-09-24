#include "TheFinalTake/Career/FTCareerManager.h"

#include "The_Final_Take.h"
#include "TheFinalTake/Career/FTCareerSave.h"
#include "TheFinalTake/Career/FTEconomy.h"
#include "TheFinalTake/Game/FTGameState.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#define LOCTEXT_NAMESPACE "FinalTakeCareer"

AFTCareerManager::AFTCareerManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
}

AFTCareerManager* AFTCareerManager::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || World->GetNetMode() == NM_Client)
	{
		return nullptr;
	}
	for (TActorIterator<AFTCareerManager> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

AFTGameState* AFTCareerManager::GS() const
{
	return GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
}

void AFTCareerManager::InitCareer(int32 InSlot)
{
	Slot = FMath::Max(1, InSlot);
	const UFTEconomyConfig* Cfg = UFTEconomyConfig::Get();
	FText Notice;
	Save = UFTCareerSave::LoadOrCreate(Slot, *Cfg, Notice);
	UE_LOG(LogFinalTake, Display, TEXT("[Career] slot %d loaded: %s, money %d, %d film(s), %d item(s), %d car(s), %d stage(s). %s"),
		Slot, *Save->StudioName, Save->Money, Save->Films.Num(), Save->OwnedItems.Num(), Save->OwnedVehicles.Num(), Save->UnlockedStages.Num(), *Notice.ToString());
	WriteSave(TEXT("session start"));
	Publish();
	if (!Notice.IsEmpty())
	{
		if (AFTGameState* G = GS())
		{
			G->MulticastAnnounce(Notice, EFTAnnounceStyle::Info, EFTSound::None);
		}
	}
}

void AFTCareerManager::Publish()
{
	AFTGameState* G = GS();
	if (!G || !Save)
	{
		return;
	}
	G->CareerSlot = Slot;
	G->StudioName = Save->StudioName;
	G->StudioMoney = Save->Money;
	G->OwnedItems = Save->OwnedItems;
	G->OwnedVehicles = Save->OwnedVehicles;
	G->UnlockedStages = Save->UnlockedStages;
	// clients only need the recent history for the box-office board; the save keeps everything
	const int32 First = FMath::Max(0, Save->Films.Num() - 12);
	G->ReleasedFilms.Reset();
	for (int32 i = First; i < Save->Films.Num(); ++i)
	{
		G->ReleasedFilms.Add(Save->Films[i]);
	}
	G->CareerFilmsTotal = Save->Films.Num();
	G->CareerBestRevenue = Save->BestRevenue();
	G->NotifyCareerChanged();
}

bool AFTCareerManager::WriteSave(const TCHAR* Why)
{
	AFTGameState* G = GS();
	if (!Save)
	{
		return false;
	}
	FString Error;
	const bool bOk = Save->SaveToSlot(Slot, Error);
	bDirty = false;
	if (bOk)
	{
		UE_LOG(LogFinalTake, Display, TEXT("[Career] saved slot %d (%s): money %d"), Slot, Why, Save->Money);
	}
	else
	{
		UE_LOG(LogFinalTake, Error, TEXT("[Career] SAVE FAILED for slot %d (%s): %s"), Slot, Why, *Error);
	}
	if (G)
	{
		G->bLastSaveOk = bOk;
		G->LastSaveServerTime = G->GetServerWorldTimeSeconds();
		G->NotifyCareerChanged();
		if (!bOk)
		{
			G->MulticastAnnounce(LOCTEXT("SaveFail", "Career could not be saved - check disk space. The game keeps running."), EFTAnnounceStyle::Danger, EFTSound::UIError);
		}
	}
	return bOk;
}

void AFTCareerManager::SaveNow(const TCHAR* Why)
{
	WriteSave(Why);
}

void AFTCareerManager::MarkDirty()
{
	bDirty = true;
	DirtyAge = 0.f;
}

void AFTCareerManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDirty)
	{
		DirtyAge += DeltaSeconds;
		if (DirtyAge > 2.f)
		{
			WriteSave(TEXT("autosave"));
		}
	}
}

void AFTCareerManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Save && bDirty)
	{
		WriteSave(TEXT("session end"));
	}
	Super::EndPlay(EndPlayReason);
}

bool AFTCareerManager::TryPurchase(APlayerController* Buyer, FName Id, FText& OutReason)
{
	if (!Save || bInTransaction)
	{
		OutReason = FFTCareerLedger::ReasonText(EFTPurchaseResult::NotAllowed, 0, Save ? Save->Money : 0);
		return false;
	}
	TGuardValue<bool> Guard(bInTransaction, true);
	const UFTEconomyConfig* Cfg = UFTEconomyConfig::Get();
	int32 Price = 0;
	const EFTPurchaseResult Result = FFTCareerLedger::Purchase(*Save, *Cfg, Id, Price);
	OutReason = FFTCareerLedger::ReasonText(Result, Price, Save->Money);
	if (Result != EFTPurchaseResult::Ok)
	{
		UE_LOG(LogFinalTake, Display, TEXT("[Career] purchase of %s rejected: %s"), *Id.ToString(), *OutReason.ToString());
		return false;
	}
	int32 IgnoredPrice = 0;
	EFTShopCategory Cat = EFTShopCategory::Prop;
	FText Name;
	Cfg->Describe(Id, IgnoredPrice, Cat, Name);
	UE_LOG(LogFinalTake, Display, TEXT("[Career] bought %s for %d, balance %d"), *Id.ToString(), Price, Save->Money);
	WriteSave(TEXT("purchase"));
	Publish();
	OnPurchased.Broadcast(Id);
	if (AFTGameState* G = GS())
	{
		FString Who = TEXT("The crew");
		if (Buyer && Buyer->PlayerState)
		{
			Who = Buyer->PlayerState->GetPlayerName();
		}
		G->MulticastAnnounce(FText::Format(LOCTEXT("Bought", "{0} bought {1} for {2}  -  studio balance {3}"), FText::FromString(Who), Name,
			FText::FromString(FFTEconomy::MoneyString(Price)), FText::FromString(FFTEconomy::MoneyString(Save->Money))), EFTAnnounceStyle::Success, EFTSound::UIConfirm);
	}
	OutReason = FText::Format(LOCTEXT("BoughtShort", "Bought {0}!"), Name);
	return true;
}

bool AFTCareerManager::RecordRelease(FFTReleaseReport& Report)
{
	if (!Save || !Report.bValid)
	{
		return false;
	}
	Report.ReleaseNumber = Save->ReleaseCounter + 1;
	FFTEconomy::RankRelease(Report, Save->Films);
	if (!FFTCareerLedger::BookRelease(*Save, Report))
	{
		UE_LOG(LogFinalTake, Warning, TEXT("[Career] release %d was already booked - ignoring"), Report.ReleaseNumber);
		return false;
	}
	Report.BalanceAfter = Save->Money;
	UE_LOG(LogFinalTake, Display, TEXT("[Career] release #%d %s: quality %d, production %d, audience %d, revenue %d, rank %d/%d, balance %d"),
		Report.ReleaseNumber, *Report.FilmId.ToString(), Report.Quality, Report.ProductionPoints, Report.Audience, Report.Revenue, Report.Rank, Report.RankOf, Save->Money);
	WriteSave(TEXT("release"));
	Publish();
	return true;
}

void AFTCareerManager::ResetCareer()
{
	UFTCareerSave::DeleteSlot(Slot);
	Save = UFTCareerSave::CreateFresh(Slot, *UFTEconomyConfig::Get());
	WriteSave(TEXT("career reset"));
	Publish();
	OnCareerReset.Broadcast();
	if (AFTGameState* G = GS())
	{
		G->MulticastAnnounce(LOCTEXT("Reset", "Career reset - a brand new studio opens its doors."), EFTAnnounceStyle::Slate, EFTSound::Clapper);
	}
}

bool AFTCareerManager::Owns(FName Id) const
{
	return Save && FFTCareerLedger::Owns(*Save, Id);
}

int32 AFTCareerManager::TimesReleased(FName FilmId) const
{
	return Save ? Save->TimesReleased(FilmId) : 0;
}

bool AFTCareerManager::IsStageUnlocked(FName StageId) const
{
	return Save && (StageId.IsNone() || StageId == FTCareerIds::Stage4 || Save->UnlockedStages.Contains(StageId));
}

void AFTCareerManager::SetPlacement(FName ItemId, const FTransform& Where)
{
	if (!Save || Where.ContainsNaN())
	{
		return;
	}
	for (FFTPlacedItem& P : Save->Placements)
	{
		if (P.ItemId == ItemId)
		{
			if (!P.Transform.Equals(Where, 1.f))
			{
				P.Transform = Where;
				MarkDirty();
			}
			return;
		}
	}
	FFTPlacedItem P;
	P.ItemId = ItemId;
	P.Transform = Where;
	Save->Placements.Add(P);
	MarkDirty();
}

bool AFTCareerManager::GetPlacement(FName ItemId, FTransform& OutWhere) const
{
	if (Save)
	{
		for (const FFTPlacedItem& P : Save->Placements)
		{
			if (P.ItemId == ItemId)
			{
				OutWhere = P.Transform;
				return true;
			}
		}
	}
	return false;
}

void AFTCareerManager::SetParkedVehicle(FName VehicleId, const FTransform& Where)
{
	if (!Save || Where.ContainsNaN())
	{
		return;
	}
	for (FFTParkedVehicle& V : Save->ParkedVehicles)
	{
		if (V.VehicleId == VehicleId)
		{
			if (!V.Transform.Equals(Where, 5.f))
			{
				V.Transform = Where;
				MarkDirty();
			}
			return;
		}
	}
	FFTParkedVehicle V;
	V.VehicleId = VehicleId;
	V.Transform = Where;
	Save->ParkedVehicles.Add(V);
	MarkDirty();
}

bool AFTCareerManager::GetParkedVehicle(FName VehicleId, FTransform& OutWhere) const
{
	if (Save)
	{
		for (const FFTParkedVehicle& V : Save->ParkedVehicles)
		{
			if (V.VehicleId == VehicleId)
			{
				OutWhere = V.Transform;
				return true;
			}
		}
	}
	return false;
}

void AFTCareerManager::SetAccessory(FName Wearer, FName ItemId)
{
	if (!Save)
	{
		return;
	}
	Save->Accessories.RemoveAll([Wearer](const FFTEquippedAccessory& A) { return A.Wearer == Wearer; });
	if (!ItemId.IsNone())
	{
		FFTEquippedAccessory A;
		A.Wearer = Wearer;
		A.ItemId = ItemId;
		Save->Accessories.Add(A);
	}
	MarkDirty();
}

FName AFTCareerManager::GetAccessory(FName Wearer) const
{
	if (Save)
	{
		for (const FFTEquippedAccessory& A : Save->Accessories)
		{
			if (A.Wearer == Wearer)
			{
				return A.ItemId;
			}
		}
	}
	return NAME_None;
}

void AFTCareerManager::SetRigKitActive(FName ItemId, bool bActive)
{
	if (!Save)
	{
		return;
	}
	if (bActive)
	{
		Save->ActiveRigKits.AddUnique(ItemId);
	}
	else
	{
		Save->ActiveRigKits.Remove(ItemId);
	}
	MarkDirty();
}

bool AFTCareerManager::IsRigKitActive(FName ItemId) const
{
	return Save && Save->ActiveRigKits.Contains(ItemId);
}

#undef LOCTEXT_NAMESPACE
