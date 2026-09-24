// Reproducible level build for L_FinalTake_Studio (editor-only; used by the FTContent commandlet with -map).

#include "CoreMinimal.h"
#include "The_Final_Take.h"

#if WITH_EDITOR
#include "TheFinalTake/Core/FTTypes.h"
#include "TheFinalTake/Game/FTGameMode.h"
#include "TheFinalTake/Production/FTFilmCamera.h"
#include "TheFinalTake/Production/FTShark.h"
#include "TheFinalTake/Production/FTStations.h"
#include "TheFinalTake/Props/FTProp.h"
#include "TheFinalTake/Props/FTSetPieces.h"
#include "TheFinalTake/World/FTFloodController.h"
#include "TheFinalTake/World/FTStudioObjects.h"
#include "TheFinalTake/World/FTStudioShell.h"
#include "TheFinalTake/World/FTZone.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "Factories/WorldFactory.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#include "UObject/Package.h"

namespace
{
	template <typename T>
	T* Spawn(UWorld* W, const FVector& Loc, float Yaw, const FString& Label, TFunctionRef<void(T*)> Setup)
	{
		const FTransform X(FRotator(0.f, Yaw, 0.f), Loc);
		T* A = W->SpawnActorDeferred<T>(T::StaticClass(), X, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!A)
		{
			return nullptr;
		}
		Setup(A);
		A->FinishSpawning(X);
		A->SetActorLabel(Label);
		return A;
	}

	template <typename T>
	T* Spawn(UWorld* W, const FVector& Loc, float Yaw, const FString& Label)
	{
		return Spawn<T>(W, Loc, Yaw, Label, [](T*) {});
	}

	void Zone(UWorld* W, const FString& Label, FName Tag, const FVector& Center, const FVector& Extent, EFTZoneMark Mark = EFTZoneMark::None, const FLinearColor& Color = FLinearColor::Yellow, const FString& Text = FString())
	{
		Spawn<AFTZone>(W, Center, 0.f, Label, [&](AFTZone* Z)
		{
			Z->ZoneTag = Tag;
			Z->Extent = Extent;
			Z->Mark = Mark;
			Z->MarkColor = Color;
			Z->MarkLabel = FText::FromString(Text);
		});
	}
}

bool FTBuildStudioMap()
{
	const FString PackagePath = TEXT("/Game/TheFinalTake/Maps/L_FinalTake_Studio");
	// regenerate from scratch: the previous build is tracked in version control
	const FString File = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetMapPackageExtension());
	if (IFileManager::Get().FileExists(*File))
	{
		IFileManager::Get().Delete(*File, false, true);
	}
	UPackage* Pkg = CreatePackage(*PackagePath);
	UWorldFactory* Factory = NewObject<UWorldFactory>();
	Factory->WorldType = EWorldType::Editor;
	Factory->bInformEngineOfWorld = true;
	Factory->bCreateWorldPartition = false;
	UWorld* W = CastChecked<UWorld>(Factory->FactoryCreateNew(UWorld::StaticClass(), Pkg, TEXT("L_FinalTake_Studio"), RF_Public | RF_Standalone, nullptr, GWarn));
	if (!W)
	{
		return false;
	}
	if (AWorldSettings* WS = W->GetWorldSettings())
	{
		WS->DefaultGameMode = AFTGameMode::StaticClass();
		WS->KillZ = -3000.f;
	}

	// ---------------------------------------------------------------- architecture
	Spawn<AFTStudioShell>(W, FVector::ZeroVector, 0.f, TEXT("StudioShell"));
	Spawn<AFTAmbientRain>(W, FVector(-2700.f, 200.f, 0.f), 0.f, TEXT("StreetRain"), [](AFTAmbientRain* R) { R->Extent = FVector(700.f, 2100.f, 10.f); });

	// ---------------------------------------------------------------- environment lighting
	Spawn<ADirectionalLight>(W, FVector(0.f, 0.f, 3000.f), 0.f, TEXT("Moonlight"), [](ADirectionalLight* L)
	{
		L->SetActorRotation(FRotator(-38.f, 35.f, 0.f));
		UDirectionalLightComponent* C = CastChecked<UDirectionalLightComponent>(L->GetLightComponent());
		C->SetIntensity(1.6f);
		C->SetLightColor(FLinearColor(0.55f, 0.65f, 1.f));
		C->SetMobility(EComponentMobility::Movable);
	});
	Spawn<ASkyLight>(W, FVector(0.f, 0.f, 2500.f), 0.f, TEXT("SkyLight"), [](ASkyLight* S)
	{
		USkyLightComponent* C = S->GetLightComponent();
		C->SetMobility(EComponentMobility::Movable);
		C->SourceType = ESkyLightSourceType::SLS_CapturedScene;
		C->SetIntensity(1.2f);
		C->SetLightColor(FLinearColor(0.7f, 0.78f, 1.f));
		C->bLowerHemisphereIsBlack = false;
		C->LowerHemisphereColor = FLinearColor(0.08f, 0.09f, 0.16f);
	});
	Spawn<AExponentialHeightFog>(W, FVector(-2000.f, 0.f, 0.f), 0.f, TEXT("NightFog"), [](AExponentialHeightFog* F)
	{
		UExponentialHeightFogComponent* C = F->GetComponent();
		C->SetFogDensity(0.012f);
		C->SetFogHeightFalloff(0.08f);
		C->SetFogInscatteringColor(FLinearColor(0.06f, 0.09f, 0.22f));
		C->SetStartDistance(600.f);
	});
	Spawn<APostProcessVolume>(W, FVector::ZeroVector, 0.f, TEXT("PostProcess"), [](APostProcessVolume* P)
	{
		P->bUnbound = true;
		FPostProcessSettings& S = P->Settings;
		// Fixed exposure: the look is authored with unit-less light intensities, and eye adaptation
		// made the stage go black whenever a bright prop filled the frame.
		S.bOverride_AutoExposureMethod = true;
		S.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		S.AutoExposureApplyPhysicalCameraExposure = 0;
		S.bOverride_AutoExposureBias = true;
		S.AutoExposureBias = 0.f;
		S.bOverride_BloomIntensity = true;
		S.BloomIntensity = 0.25f;
		S.bOverride_VignetteIntensity = true;
		S.VignetteIntensity = 0.16f;
		S.bOverride_ColorSaturation = true;
		S.ColorSaturation = FVector4(1.04f, 1.04f, 1.04f, 1.f);
		S.bOverride_SceneFringeIntensity = true;
		S.SceneFringeIntensity = 0.f;
		if (UMaterialInterface* Outline = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/TheFinalTake/Materials/PP_FT_Outline.PP_FT_Outline")))
		{
			S.WeightedBlendables.Array.Add(FWeightedBlendable(0.35f, Outline));
		}
	});

	// ---------------------------------------------------------------- spawns + title camera
	for (int32 i = 0; i < 4; ++i)
	{
		Spawn<APlayerStart>(W, FVector(-1560.f, -240.f + i * 160.f, 100.f), 0.f, FString::Printf(TEXT("PlayerStart_%d"), i));
	}
	Spawn<ACameraActor>(W, FVector(-3150.f, -1050.f, 230.f), 0.f, TEXT("TitleCamera"), [](ACameraActor* C)
	{
		// the marquee sits right of the menu panel instead of behind the logo
		C->SetActorRotation(FRotator(7.f, 25.f, 0.f));
		C->Tags.Add(TEXT("TitleCamera"));
		C->GetCameraComponent()->SetFieldOfView(62.f);
		C->GetCameraComponent()->SetConstraintAspectRatio(false);
	});

	// ---------------------------------------------------------------- doors, office, wardrobe
	Spawn<AFTDoor>(W, FVector(-600.f, 0.f, 0.f), 0.f, TEXT("Door_Stage4"), [](AFTDoor* D)
	{
		D->Lock = EFTDoorLock::Keycard;
		D->Size = FVector2D(600.f, 400.f);
		D->Sign = FText::FromString(TEXT("STAGE 4"));
		D->DoorColor = FLinearColor(0.02f, 0.2f, 0.19f);
	});
	Spawn<AFTDoor>(W, FVector(-1200.f, 1000.f, 0.f), 90.f, TEXT("Door_Office"), [](AFTDoor* D)
	{
		D->Size = FVector2D(300.f, 300.f);
		D->Sign = FText::FromString(TEXT("OFFICE"));
		D->DoorColor = FLinearColor(0.6f, 0.2f, 0.12f);
	});
	Spawn<AFTDoor>(W, FVector(-600.f, -1325.f, 0.f), 180.f, TEXT("Door_Wardrobe"), [](AFTDoor* D)
	{
		D->Size = FVector2D(250.f, 300.f);
		D->Sign = FText::FromString(TEXT("WARDROBE & MAKEUP"));
		D->DoorColor = FLinearColor(0.45f, 0.08f, 0.3f);
	});
	Spawn<AFTDoor>(W, FVector(250.f, -1460.f, 420.f), 180.f, TEXT("Door_Projection"), [](AFTDoor* D)
	{
		D->Lock = EFTDoorLock::Projection;
		D->Size = FVector2D(200.f, 220.f);
		D->Sign = FText::FromString(TEXT("PROJECTION"));
		D->DoorColor = FLinearColor(0.15f, 0.08f, 0.35f);
	});
	Spawn<AFTScriptBook>(W, FVector(-1050.f, 1480.f, 86.f), 0.f, TEXT("ScriptBook"));
	Spawn<AFTSceneBoard>(W, FVector(-1150.f, 1972.f, 230.f), -90.f, TEXT("SceneBoard_Office"));
	Spawn<AFTSceneBoard>(W, FVector(-560.f, -700.f, 220.f), 0.f, TEXT("SceneBoard_Stage"));
	Spawn<AFTCostumeRack>(W, FVector(-1740.f, -1300.f, 0.f), 0.f, TEXT("CostumeRack"));
	Spawn<AFTStandIn>(W, FVector(-1250.f, -1150.f, 0.f), 0.f, TEXT("StandIn"));

	// ---------------------------------------------------------------- production equipment
	Spawn<AFTFilmCamera>(W, FVector(450.f, 0.f, -60.f), 0.f, TEXT("FilmCamera"), [](AFTFilmCamera* C) { C->TrackLength = 1400.f; });
	Spawn<AFTReelTray>(W, FVector(400.f, -700.f, -60.f), 0.f, TEXT("ReelTray"));
	Spawn<AFTLightingBoard>(W, FVector(430.f, 640.f, -60.f), 0.f, TEXT("LightingBoard"));
	Spawn<AFTStageLight>(W, FVector(1500.f, -1250.f, -120.f), 55.f, TEXT("Light_Key"), [](AFTStageLight* L) { L->Circuit = 1; L->StartColor = 0; L->StandHeight = 300.f; L->AimPitch = -10.f; });
	Spawn<AFTStageLight>(W, FVector(1100.f, 1350.f, -120.f), -61.f, TEXT("Light_Fill"), [](AFTStageLight* L) { L->Circuit = 2; L->StartColor = 1; L->StandHeight = 260.f; L->AimPitch = -8.f; L->Intensity = 45000.f; });
	Spawn<AFTStageLight>(W, FVector(1200.f, 1300.f, -120.f), -49.f, TEXT("Light_Hero"), [](AFTStageLight* L) { L->Circuit = 3; L->StartColor = 3; L->StandHeight = 260.f; L->AimPitch = -16.f; L->Intensity = 70000.f; });
	Spawn<AFTBreaker>(W, FVector(-575.f, 650.f, 0.f), 0.f, TEXT("MainBreaker"));
	const FVector Beacons[] = { FVector(2380.f, -1200.f, 900.f), FVector(2380.f, 1500.f, 900.f), FVector(700.f, -1580.f, 900.f), FVector(700.f, 1980.f, 900.f) };
	for (int32 i = 0; i < 4; ++i)
	{
		Spawn<AFTEmergencyLight>(W, Beacons[i], 0.f, FString::Printf(TEXT("EmergencyLight_%d"), i));
	}
	Spawn<AFTSoundConsole>(W, FVector(230.f, -1300.f, -120.f), 0.f, TEXT("SoundConsole"), [](AFTSoundConsole* S) { S->SpeakerLocation = FVector(1500.f, 1300.f, 500.f); });
	Spawn<AFTEffectMachine>(W, FVector(1150.f, 1350.f, -120.f), -60.f, TEXT("WindMachine"), [](AFTEffectMachine* E) { E->Effect = EFTEffectType::Wind; });
	Spawn<AFTEffectMachine>(W, FVector(950.f, 1500.f, -120.f), 0.f, TEXT("RainMachine"), [](AFTEffectMachine* E)
	{
		E->Effect = EFTEffectType::Rain;
		E->EffectCenter = FVector(850.f, -1400.f, 520.f);
		E->EffectExtent = FVector(420.f, 900.f, 20.f);
	});
	Spawn<AFTEffectMachine>(W, FVector(750.f, 1650.f, -120.f), -80.f, TEXT("SmokeMachine"), [](AFTEffectMachine* E) { E->Effect = EFTEffectType::Smoke; });
	Spawn<AFTEffectMachine>(W, FVector(1000.f, 1760.f, -120.f), -65.f, TEXT("FoamCannon"), [](AFTEffectMachine* E) { E->Effect = EFTEffectType::Foam; });

	// ---------------------------------------------------------------- beach set, tank, shark
	Spawn<AFTLighthouse>(W, FVector(2150.f, -700.f, -10.f), 180.f, TEXT("Lighthouse"));
	Spawn<AFTFinGlider>(W, FVector(1700.f, -750.f, -50.f), 0.f, TEXT("FinGlider"), [](AFTFinGlider* G) { G->PathEnd = FVector(0.f, 1500.f, 0.f); });
	// Rig sits in open water between island (Y < 30) and dock (Y > 720): the tail (+290 behind the root) and the
	// rail stay clear of both, and the boat parks south of the rail. Station stays on the stage floor at (1480, -1180).
	Spawn<AFTSharkRig>(W, FVector(1700.f, 460.f, -120.f), 180.f, TEXT("SharkRig"), [](AFTSharkRig* R)
	{
		R->WaterHeight = 70.f;
		R->RailHalfLength = 180.f;
		R->LungeReach = 200.f;
		R->StationOffset = FVector(220.f, 1640.f, 0.f);
		R->StationYaw = 180.f;
	});
	Spawn<AFTRescueBoat>(W, FVector(850.f, 250.f, -120.f), 0.f, TEXT("RescueBoat"), [](AFTRescueBoat* B) { B->PathEnd = FVector(650.f, -150.f, 70.f); });
	Spawn<AFTFloodController>(W, FVector(900.f, 200.f, -120.f), 0.f, TEXT("FloodController"), [](AFTFloodController* F)
	{
		F->FloorZ = -120.f;
		F->ValveLocation = FVector(370.f, -500.f, 60.f);
	});
	Spawn<AFTSharkHazard>(W, FVector(900.f, 200.f, -120.f), 0.f, TEXT("FloodShark"), [](AFTSharkHazard* H)
	{
		H->Patrol = { FVector(-180.f, -1350.f, 0.f), FVector(330.f, -1350.f, 0.f), FVector(330.f, 1050.f, 0.f), FVector(-180.f, 1050.f, 0.f) };
	});
	// roller housing just under the 1320 ceiling; the sheet unrolls in front of the sky backdrop
	Spawn<AFTCinemaScreen>(W, FVector(2360.f, 100.f, 760.f), 180.f, TEXT("CinemaScreen"));
	Spawn<AFTProjector>(W, FVector(120.f, -1125.f, 420.f), 0.f, TEXT("Projector"), [](AFTProjector* P) { P->PowerPanelOffset = FVector(-250.f, -330.f, 0.f); });

	// ---------------------------------------------------------------- props
	Spawn<AFTProp_Harpoon>(W, FVector(2250.f, 1700.f, -36.f), 90.f, TEXT("Prop_Harpoon"));
	Spawn<AFTProp_LifeRing>(W, FVector(1960.f, 1000.f, -30.f), 0.f, TEXT("Prop_LifeRing"));
	Spawn<AFTProp_Lamp>(W, FVector(650.f, 900.f, -120.f), -120.f, TEXT("Prop_Lamp"));
	Spawn<AFTProp_Crate>(W, FVector(1230.f, -1050.f, -120.f), 20.f, TEXT("Prop_Crate"));
	Spawn<AFTProp_Crate>(W, FVector(2050.f, 1560.f, -120.f), -10.f, TEXT("Prop_Crate2"));
	Spawn<AFTProp_FinMarker>(W, FVector(820.f, 1150.f, -120.f), 0.f, TEXT("Prop_FinMarker"));
	Spawn<AFTPropShelf>(W, FVector(1450.f, 1900.f, -120.f), -90.f, TEXT("LostAndFound"));

	// ---------------------------------------------------------------- zones
	Zone(W, TEXT("Zone_Beach"), FTTags::ZoneBeach, FVector(2060.f, -420.f, 20.f), FVector(210.f, 460.f, 80.f));
	Zone(W, TEXT("Zone_Tank"), FTTags::ZoneTank, FVector(1800.f, 100.f, -40.f), FVector(470.f, 970.f, 100.f));
	Zone(W, TEXT("Zone_HeroMark"), FTTags::ZoneHeroMark, FVector(1590.f, 880.f, 20.f), FVector(140.f, 160.f, 90.f));
	Zone(W, TEXT("Zone_LungeMark"), FTTags::ZoneLungeMark, FVector(1180.f, 250.f, -60.f), FVector(110.f, 150.f, 90.f), EFTZoneMark::Ring, FLinearColor(1.f, 0.35f, 0.25f), TEXT("SHARK LUNGE (F)"));
	Zone(W, TEXT("Zone_Safe_Harpoon"), FTTags::ZoneSafe, FVector(2200.f, 1700.f, -60.f), FVector(260.f, 320.f, 150.f), EFTZoneMark::Outline, FLinearColor(0.3f, 1.f, 0.6f));
	Zone(W, TEXT("Zone_Safe_Shelf"), FTTags::ZoneSafe, FVector(1450.f, 1820.f, -60.f), FVector(200.f, 200.f, 150.f), EFTZoneMark::Outline, FLinearColor(0.3f, 1.f, 0.6f));
	Zone(W, TEXT("Zone_Safe_SharkStation"), FTTags::ZoneSafe, FVector(1420.f, -1180.f, -60.f), FVector(200.f, 220.f, 150.f), EFTZoneMark::Outline, FLinearColor(0.3f, 1.f, 0.6f));
	Zone(W, TEXT("Zone_Safe_Stairs"), FTTags::ZoneSafe, FVector(1150.f, -1480.f, -60.f), FVector(250.f, 150.f, 150.f));

	const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(W, PackagePath);
	UE_LOG(LogFinalTake, Display, TEXT("[FTContent] map %s saved=%d"), *PackagePath, bSaved ? 1 : 0);
	GEngine->DestroyWorldContext(W);
	W->DestroyWorld(false);
	return bSaved;
}
#else
bool FTBuildStudioMap()
{
	return false;
}
#endif
