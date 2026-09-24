#include "TheFinalTake/Career/FTCareerSave.h"

#include "The_Final_Take.h"
#include "TheFinalTake/Career/FTEconomy.h"

#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

#define LOCTEXT_NAMESPACE "FinalTakeCareerSave"

namespace
{
	constexpr uint32 SaveMagic = 0x52435446; // "FTCR"
	constexpr uint32 FileFormat = 1;
	constexpr int32 FTSaveHeaderSize = 16;
}

FString UFTCareerSave::SlotPath(int32 Slot)
{
	return FPaths::ProjectSavedDir() / TEXT("SaveGames") / FString::Printf(TEXT("FTCareer_%d.ftsave"), Slot);
}

UFTCareerSave* UFTCareerSave::CreateFresh(int32 Slot, const UFTEconomyConfig& Cfg)
{
	UFTCareerSave* S = NewObject<UFTCareerSave>();
	S->Version = CurrentVersion;
	S->StudioName = FString::Printf(TEXT("Studio %d"), Slot);
	S->Money = Cfg.StartingMoney;
	S->UnlockedStages.Add(FTCareerIds::Stage4);
	S->OwnedVehicles.Add(FTCareerIds::StarterVehicle);
	S->Created = FDateTime::UtcNow();
	S->LastSaved = S->Created;
	return S;
}

bool UFTCareerSave::WriteValidated(const FString& Path, const TArray<uint8>& Payload, FString& OutError)
{
	TArray<uint8> Bytes;
	Bytes.Reserve(Payload.Num() + FTSaveHeaderSize);
	FMemoryWriter W(Bytes);
	uint32 Magic = SaveMagic, Format = FileFormat, Size = Payload.Num(), Crc = FCrc::MemCrc32(Payload.GetData(), Payload.Num());
	W << Magic << Format << Size << Crc;
	Bytes.Append(Payload);

	IFileManager& FM = IFileManager::Get();
	FM.MakeDirectory(*FPaths::GetPath(Path), true);
	const FString Temp = Path + TEXT(".tmp");
	if (!FFileHelper::SaveArrayToFile(Bytes, *Temp))
	{
		OutError = FString::Printf(TEXT("could not write %s"), *Temp);
		return false;
	}
	// read back what actually landed on disk before replacing the good file
	TArray<uint8> Check;
	FString CheckError;
	if (!ReadValidated(Temp, Check, CheckError))
	{
		FM.Delete(*Temp, false, true, true);
		OutError = FString::Printf(TEXT("verification failed: %s"), *CheckError);
		return false;
	}
	if (FM.FileExists(*Path))
	{
		FM.Move(*(Path + TEXT(".bak")), *Path, true, true, false, true);
	}
	if (!FM.Move(*Path, *Temp, true, true, false, true))
	{
		OutError = FString::Printf(TEXT("could not move %s into place"), *Temp);
		return false;
	}
	return true;
}

bool UFTCareerSave::ReadValidated(const FString& Path, TArray<uint8>& OutPayload, FString& OutError)
{
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *Path, FILEREAD_Silent))
	{
		OutError = TEXT("missing");
		return false;
	}
	if (Bytes.Num() < FTSaveHeaderSize)
	{
		OutError = TEXT("file too short");
		return false;
	}
	FMemoryReader R(Bytes);
	uint32 Magic = 0, Format = 0, Size = 0, Crc = 0;
	R << Magic << Format << Size << Crc;
	if (Magic != SaveMagic)
	{
		OutError = TEXT("not a career save");
		return false;
	}
	if (Format > FileFormat)
	{
		OutError = TEXT("written by a newer version of the game");
		return false;
	}
	if (static_cast<int64>(Size) != static_cast<int64>(Bytes.Num()) - FTSaveHeaderSize)
	{
		OutError = TEXT("truncated payload");
		return false;
	}
	OutPayload.SetNumUninitialized(Size);
	FMemory::Memcpy(OutPayload.GetData(), Bytes.GetData() + FTSaveHeaderSize, Size);
	if (FCrc::MemCrc32(OutPayload.GetData(), OutPayload.Num()) != Crc)
	{
		OutError = TEXT("checksum mismatch");
		return false;
	}
	return true;
}

namespace
{
	UFTCareerSave* TryLoadFile(const FString& Path, FString& OutError)
	{
		TArray<uint8> Payload;
		if (!UFTCareerSave::ReadValidated(Path, Payload, OutError))
		{
			return nullptr;
		}
		UFTCareerSave* S = Cast<UFTCareerSave>(UGameplayStatics::LoadGameFromMemory(Payload));
		if (!S)
		{
			OutError = TEXT("payload could not be deserialized");
		}
		return S;
	}
}

UFTCareerSave* UFTCareerSave::LoadOrCreate(int32 Slot, const UFTEconomyConfig& Cfg, FText& OutNotice)
{
	const FString Path = SlotPath(Slot);
	IFileManager& FM = IFileManager::Get();
	if (!FM.FileExists(*Path))
	{
		// a crash between the two moves of a save can leave only the backup behind
		FString BakError;
		if (UFTCareerSave* FromBak = FM.FileExists(*(Path + TEXT(".bak"))) ? TryLoadFile(Path + TEXT(".bak"), BakError) : nullptr)
		{
			FromBak->Sanitize(Cfg);
			OutNotice = LOCTEXT("RestoredBak", "Career restored from its backup copy.");
			return FromBak;
		}
		UFTCareerSave* Fresh = CreateFresh(Slot, Cfg);
		OutNotice = FText::Format(LOCTEXT("NewCareer", "New career started in slot {0}."), FText::AsNumber(Slot));
		return Fresh;
	}
	FString Error;
	if (UFTCareerSave* S = TryLoadFile(Path, Error))
	{
		const int32 Loaded = S->Version;
		S->Sanitize(Cfg);
		if (Loaded < CurrentVersion)
		{
			OutNotice = FText::Format(LOCTEXT("Migrated", "Career save upgraded from version {0}."), FText::AsNumber(Loaded));
		}
		return S;
	}
	UE_LOG(LogFinalTake, Warning, TEXT("[Career] slot %d is damaged (%s); moving it aside"), Slot, *Error);
	FM.Move(*(Path + TEXT(".corrupt")), *Path, true, true, false, true);
	FString BakError;
	if (UFTCareerSave* FromBak = TryLoadFile(Path + TEXT(".bak"), BakError))
	{
		FromBak->Sanitize(Cfg);
		OutNotice = LOCTEXT("UsedBackup", "The career save was damaged - the last backup was loaded instead.");
		return FromBak;
	}
	OutNotice = LOCTEXT("StartedFresh", "The career save was damaged and had no usable backup - a fresh career was started (the damaged file was kept as .corrupt).");
	return CreateFresh(Slot, Cfg);
}

bool UFTCareerSave::SaveToSlot(int32 Slot, FString& OutError)
{
	Version = CurrentVersion;
	LastSaved = FDateTime::UtcNow();
	TArray<uint8> Payload;
	if (!UGameplayStatics::SaveGameToMemory(this, Payload))
	{
		OutError = TEXT("serialization failed");
		return false;
	}
	return WriteValidated(SlotPath(Slot), Payload, OutError);
}

bool UFTCareerSave::DeleteSlot(int32 Slot)
{
	IFileManager& FM = IFileManager::Get();
	const FString Path = SlotPath(Slot);
	bool bOk = true;
	for (const FString& P : { Path, Path + TEXT(".bak"), Path + TEXT(".tmp") })
	{
		if (FM.FileExists(*P))
		{
			bOk &= FM.Delete(*P, false, true, true);
		}
	}
	return bOk;
}

FFTCareerSlotInfo UFTCareerSave::Peek(int32 Slot)
{
	FFTCareerSlotInfo Info;
	Info.Slot = Slot;
	const FString Path = SlotPath(Slot);
	if (!IFileManager::Get().FileExists(*Path))
	{
		return Info;
	}
	Info.bExists = true;
	FString Error;
	if (const UFTCareerSave* S = TryLoadFile(Path, Error))
	{
		Info.StudioName = S->StudioName;
		Info.Money = S->Money;
		Info.FilmsReleased = S->Films.Num();
		Info.StagesUnlocked = S->UnlockedStages.Num();
		Info.BestRevenue = S->BestRevenue();
		Info.LastSaved = S->LastSaved;
	}
	else
	{
		Info.bCorrupt = true;
	}
	return Info;
}

void UFTCareerSave::Sanitize(const UFTEconomyConfig& Cfg)
{
	// ---- version migration (none needed yet; version 0 means "written before versioning")
	if (Version < 1)
	{
		if (StudioName.IsEmpty())
		{
			StudioName = TEXT("Studio");
		}
	}
	Version = CurrentVersion;

	// ---- repair impossible values
	Money = FMath::Max(0, Money);
	TotalRevenue = FMath::Max<int64>(0, TotalRevenue);
	TotalSpent = FMath::Max<int64>(0, TotalSpent);
	OwnedItems.RemoveAll([&Cfg](FName Id) { return Cfg.FindItem(Id) == nullptr; });
	OwnedVehicles.RemoveAll([&Cfg](FName Id) { return Cfg.FindVehicle(Id) == nullptr; });
	UnlockedStages.RemoveAll([&Cfg](FName Id) { return Id != FTCareerIds::Stage4 && Cfg.FindStage(Id) == nullptr; });
	for (TArray<FName>* List : { &OwnedItems, &OwnedVehicles, &UnlockedStages, &ActiveRigKits })
	{
		TArray<FName> Unique;
		for (FName Id : *List)
		{
			Unique.AddUnique(Id);
		}
		*List = MoveTemp(Unique);
	}
	UnlockedStages.AddUnique(FTCareerIds::Stage4);
	OwnedVehicles.AddUnique(FTCareerIds::StarterVehicle);
	Placements.RemoveAll([this](const FFTPlacedItem& P) { return !OwnedItems.Contains(P.ItemId) || P.Transform.ContainsNaN(); });
	ParkedVehicles.RemoveAll([this](const FFTParkedVehicle& V) { return !OwnedVehicles.Contains(V.VehicleId) || V.Transform.ContainsNaN(); });
	Accessories.RemoveAll([this](const FFTEquippedAccessory& A) { return !OwnedItems.Contains(A.ItemId); });
	ActiveRigKits.RemoveAll([this](FName Id) { return !OwnedItems.Contains(Id); });
	for (const FFTReleasedFilm& F : Films)
	{
		ReleaseCounter = FMath::Max(ReleaseCounter, F.ReleaseNumber);
	}
}

int32 UFTCareerSave::TimesReleased(FName FilmId) const
{
	int32 N = 0;
	for (const FFTReleasedFilm& F : Films)
	{
		N += F.FilmId == FilmId ? 1 : 0;
	}
	return N;
}

int32 UFTCareerSave::BestRevenue() const
{
	int32 Best = 0;
	for (const FFTReleasedFilm& F : Films)
	{
		Best = FMath::Max(Best, F.Revenue);
	}
	return Best;
}

#undef LOCTEXT_NAMESPACE
