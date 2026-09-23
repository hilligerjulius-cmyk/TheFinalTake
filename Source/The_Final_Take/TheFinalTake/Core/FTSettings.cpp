#include "TheFinalTake/Core/FTSettings.h"

#include "Kismet/GameplayStatics.h"

UFTSettings::FOnSettingsChanged UFTSettings::OnChanged;

static const TCHAR* GSettingsSlot = TEXT("FTSettings");

UFTSettings* UFTSettings::Get()
{
	static TStrongObjectPtr<UFTSettings> Instance;
	if (!Instance.IsValid())
	{
		UFTSettings* Loaded = nullptr;
		if (UGameplayStatics::DoesSaveGameExist(GSettingsSlot, 0))
		{
			Loaded = Cast<UFTSettings>(UGameplayStatics::LoadGameFromSlot(GSettingsSlot, 0));
		}
		if (!Loaded)
		{
			Loaded = Cast<UFTSettings>(UGameplayStatics::CreateSaveGameObject(UFTSettings::StaticClass()));
		}
		Instance.Reset(Loaded);
	}
	return Instance.Get();
}

void UFTSettings::Save()
{
	MasterVolume = FMath::Clamp(MasterVolume, 0.f, 1.f);
	MouseSensitivity = FMath::Clamp(MouseSensitivity, 0.2f, 3.f);
	UGameplayStatics::SaveGameToSlot(this, GSettingsSlot, 0);
	OnChanged.Broadcast();
}
