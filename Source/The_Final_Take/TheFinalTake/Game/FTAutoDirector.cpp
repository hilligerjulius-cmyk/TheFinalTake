#include "TheFinalTake/Game/FTAutoDirector.h"

#include "The_Final_Take.h"
#include "TheFinalTake/Characters/FTCharacter.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Game/FTPlayerController.h"
#include "TheFinalTake/Game/FTSceneManager.h"
#include "TheFinalTake/Interaction/FTInteractableComponent.h"
#include "TheFinalTake/Interaction/FTStudioActor.h"
#include "TheFinalTake/Production/FTFilmCamera.h"
#include "TheFinalTake/Props/FTProp.h"
#include "TheFinalTake/Props/FTSetPieces.h"

#include "EngineUtils.h"
#include "UnrealClient.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

AFTAutoDirector::AFTAutoDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
}

bool AFTAutoDirector::IsRequested()
{
	return FPaths::FileExists(FPaths::ProjectSavedDir() / TEXT("FTAutoTest.txt"));
}

void AFTAutoDirector::BeginPlay()
{
	Super::BeginPlay();
	Log(TEXT("AutoTest started"));
	bShots = FParse::Param(FCommandLine::Get(), TEXT("FTAutoShots"));
	BuildSteps();
}

void AFTAutoDirector::AddShot(const FString& Name)
{
	if (!bShots)
	{
		return;
	}
	Steps.Add({ FString::Printf(TEXT("Screenshot %s"), *Name), [Name]()
	{
		const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AutoShots") / Name + TEXT(".png"));
		FScreenshotRequest::RequestScreenshot(File, true, false);
		return true;
	} });
}

void AFTAutoDirector::Log(const FString& Line, bool bFail)
{
	const FString L = FString::Printf(TEXT("[%7.1fs] %s%s"), GetWorld()->GetTimeSeconds(), bFail ? TEXT("FAIL ") : TEXT(""), *Line);
	Report.Add(L);
	UE_LOG(LogFinalTake, Display, TEXT("[AutoTest] %s"), *L);
	if (bFail)
	{
		++Failures;
	}
}

AActor* AFTAutoDirector::FindByClassName(const FString& ClassName) const
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->GetName() == ClassName)
		{
			return *It;
		}
	}
	return nullptr;
}

bool AFTAutoDirector::Teleport(const FVector& Where, float Yaw)
{
	if (AFTCharacter* C = Crew.Get())
	{
		C->TeleportTo(Where, FRotator(0.f, Yaw, 0.f), false, true);
		if (AController* Ctl = C->GetController())
		{
			Ctl->SetControlRotation(FRotator(0.f, Yaw, 0.f));
		}
		return true;
	}
	return false;
}

bool AFTAutoDirector::Use(const FString& ClassName, FName ActionId, FName RequiredTag)
{
	AFTCharacter* C = Crew.Get();
	if (!C)
	{
		return false;
	}
	FString LastReason;
	for (TActorIterator<AFTStudioActor> It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->GetName() != ClassName || (!RequiredTag.IsNone() && !It->ActorHasTag(RequiredTag)))
		{
			continue;
		}
		TArray<UFTInteractableComponent*> Comps;
		It->GetComponents<UFTInteractableComponent>(Comps);
		for (UFTInteractableComponent* Comp : Comps)
		{
			if (Comp->ActionId != ActionId)
			{
				continue;
			}
			FText Reason;
			if (!It->CanInteract(Comp, C, Reason))
			{
				LastReason = Reason.ToString();
				continue;
			}
			It->OnInteract(Comp, C);
			return true;
		}
	}
	const FString Failure = FString::Printf(TEXT("could not use %s.%s (%s) [%s]"), *ClassName, *ActionId.ToString(), *LastReason, *DescribeHands());
	if (Failure != LastUseFailure)
	{
		LastUseFailure = Failure;
		Log(Failure, false);
	}
	return false;
}

bool AFTAutoDirector::EnsureActive(const FString& ClassName, FName ActionId, FName DeviceTag, FName RequiredTag)
{
	for (TActorIterator<AFTStudioActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(DeviceTag) && It->IsDeviceActive())
		{
			return true;
		}
	}
	// give the device a moment to report its new state before toggling again
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastToggleTime > 0.75f)
	{
		LastToggleTime = Now;
		Use(ClassName, ActionId, RequiredTag);
	}
	return false;
}

FString AFTAutoDirector::DescribeObjectives() const
{
	const AFTGameState* G = GetWorld()->GetGameState<AFTGameState>();
	const FFTSceneDefinition* Scene = G ? G->GetCurrentScene() : nullptr;
	if (!Scene)
	{
		return TEXT("no scene");
	}
	FString Out = FString::Printf(TEXT("scene %d state %d:"), G->SceneIndex + 1, (int32)G->SceneState);
	for (int32 i = 0; i < Scene->Objectives.Num() && i < G->Objectives.Num(); ++i)
	{
		Out += FString::Printf(TEXT(" %s=%d"), *Scene->Objectives[i].ObjectiveId.ToString(), (int32)G->Objectives[i].State);
	}
	return Out;
}

FString AFTAutoDirector::DescribeHands() const
{
	const AFTCharacter* C = Crew.Get();
	if (!C)
	{
		return TEXT("no crew");
	}
	if (!C->HeldProp)
	{
		return FString::Printf(TEXT("hands empty, using %s"), C->UsingActor ? *C->UsingActor->GetName() : TEXT("nothing"));
	}
	const AActor* Parent = C->HeldProp->GetAttachParentActor();
	return FString::Printf(TEXT("holding %s (carrier %s, attached to %s)"), *C->HeldProp->GetName(),
		C->HeldProp->GetCarrier() ? *C->HeldProp->GetCarrier()->GetName() : TEXT("none"), Parent ? *Parent->GetName() : TEXT("none"));
}

bool AFTAutoDirector::Carry(const FString& ClassName, int32 ReelIndex)
{
	AFTCharacter* C = Crew.Get();
	if (!C)
	{
		return false;
	}
	for (TActorIterator<AFTProp> It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->GetName() == ClassName && !It->GetCarrier() && (ReelIndex < 0 || It->ReelSceneIndex == ReelIndex))
		{
			Teleport(It->GetActorLocation() + FVector(-120.f, 0.f, 100.f));
			C->TakeProp(*It);
			return C->HeldProp == *It;
		}
	}
	return false;
}

void AFTAutoDirector::BuildSteps()
{
	auto GS = [this]() { return GetWorld()->GetGameState<AFTGameState>(); };
	auto SM = [this]() { return AFTSceneManager::Get(this); };
	auto PC = [this]() { return Crew.IsValid() ? Cast<AFTPlayerController>(Crew->GetController()) : nullptr; };
	auto Cam = [this]() { AFTFilmCamera* F = nullptr; for (TActorIterator<AFTFilmCamera> It(GetWorld()); It; ++It) { F = *It; break; } return F; };
	auto ObjectiveDone = [GS](int32 Index) { AFTGameState* G = GS(); return G && G->Objectives.IsValidIndex(Index) && G->Objectives[Index].State == EFTObjectiveState::Complete; };
	auto TakeAccepted = [GS, this](int32 Scene)
	{
		AFTGameState* G = GS();
		if (const FFTTakeResult* R = G ? G->GetBestTake(Scene) : nullptr)
		{
			Log(FString::Printf(TEXT("Scene %d take accepted: %d pts (action %d, subjects %d, cues %d, props %d, style %d)"),
				Scene + 1, R->Score, R->ActionPoints, R->SubjectPoints, R->CuePoints, R->PropPoints, R->StylePoints));
			return true;
		}
		return false;
	};
	auto ClaimCamera = [this, GS, Cam]()
	{
		AFTFilmCamera* F = Cam();
		AFTGameState* G = GS();
		if (!F || !G)
		{
			return false;
		}
		if (F->GetOperator() != Crew.Get())
		{
			Teleport(F->GetActorLocation() + FVector(-120.f, 0.f, 90.f));
			Use(TEXT("FTFilmCamera"), TEXT("Operate"));
			return false;
		}
		if (G->SceneState != EFTSceneState::ReadyToRoll)
		{
			return false;
		}
		// frame the scene's required subjects like an operator would before locking off the shot
		TArray<FVector> Points;
		float MaxRadius = 120.f;
		if (const FFTSceneDefinition* Scene = G->GetCurrentScene())
		{
			for (FName Tag : Scene->RequiredSubjects)
			{
				FVector Center;
				float Radius = 0.f;
				AActor* SubjectActor = nullptr;
				if (AFTSceneManager::FindSubject(GetWorld(), Tag, Center, Radius, SubjectActor))
				{
					Points.Add(Center);
					MaxRadius = FMath::Max(MaxRadius, Radius);
				}
			}
		}
		F->FrameTargets(Points, MaxRadius);
		return true;
	};
	auto RollCamera = [this, GS, SM, Cam]()
	{
		AFTFilmCamera* F = Cam();
		AFTGameState* G = GS();
		if (!F || !G || F->GetOperator() != Crew.Get() || G->SceneState != EFTSceneState::ReadyToRoll)
		{
			Log(TEXT("camera was not ready to roll"), true);
			return true;
		}
		SM()->RequestRecordToggle(F, Crew.Get());
		const bool bRec = F->IsRecording();
		Crew->StopUsing();
		Log(FString::Printf(TEXT("camera rolling, operator stepped off (locked-off shot): %s"), bRec && F->IsRecording() ? TEXT("still recording") : TEXT("NOT recording")), !(bRec && F->IsRecording()));
		return true;
	};

	// -FTAutoPlayers=N: wait until N crew members are in (multiplayer validation), the first one drives the test
	int32 WantPlayers = 1;
	FParse::Value(FCommandLine::Get(), TEXT("FTAutoPlayers="), WantPlayers);
	Steps.Add({ FString::Printf(TEXT("%d crew member(s) spawned"), WantPlayers), [this, WantPlayers]()
	{
		int32 Count = 0;
		for (TActorIterator<AFTCharacter> It(GetWorld()); It; ++It)
		{
			if (It->GetController())
			{
				++Count;
				if (!Crew.IsValid() && It->IsLocallyControlled())
				{
					Crew = *It;
				}
			}
		}
		if (!Crew.IsValid())
		{
			for (TActorIterator<AFTCharacter> It(GetWorld()); It; ++It)
			{
				if (It->GetController())
				{
					Crew = *It;
					break;
				}
			}
		}
		if (Count >= WantPlayers && Crew.IsValid())
		{
			Log(FString::Printf(TEXT("%d crew member(s) connected, driving %s"), Count, *Crew->GetCrewName()));
			return true;
		}
		return false;
	}, 90.f });
	AddShot(TEXT("01_lobby"));
	if (bShots)
	{
		// same path as a player holding E on the book, so the client widget really opens
		Steps.Add({ TEXT("Open the script book"), [this]()
		{
			Teleport(FVector(-1250.f, 1480.f, 100.f), 0.f);
			return Use(TEXT("FTScriptBook"), TEXT("Open"));
		} });
		AddShot(TEXT("02_script_book"));
	}
	Steps.Add({ TEXT("Open the script book and greenlight Jaws of the Studio"), [this, SM, PC, GS]()
	{
		FText Why;
		if (!SM() || !PC() || !SM()->RequestOpenScriptBook(PC(), Why))
		{
			return false;
		}
		const bool bOk = SM()->RequestSelectFilm(PC(), FTTags::FilmJaws, Why);
		return bOk && GS()->ShootPhase == EFTShootPhase::Shooting;
	} });
	Steps.Add({ TEXT("Locked film Moonfall Motel cannot be greenlit"), [this, SM, PC]()
	{
		FText Why;
		SM()->RequestOpenScriptBook(PC(), Why);
		const bool bRejected = !SM()->RequestSelectFilm(PC(), FTTags::FilmMoonfall, Why);
		SM()->CloseScriptBook(PC());
		Log(FString::Printf(TEXT("locked film rejected with: %s"), *Why.ToString()), !bRejected);
		return true;
	} });
	// regression: reopening the book after a GC pass used to read freed film data and crash
	Steps.Add({ TEXT("Script book reopens after garbage collection"), [this, SM, PC]()
	{
		Teleport(FVector(-1250.f, 1480.f, 100.f), 0.f);
		const bool bFirst = Use(TEXT("FTScriptBook"), TEXT("Open"));
		SM()->CloseScriptBook(PC());
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);
		const bool bSecond = Use(TEXT("FTScriptBook"), TEXT("Open"));
		SM()->CloseScriptBook(PC());
		return bFirst && bSecond;
	} });
	Steps.Add({ TEXT("Swipe keycard at Stage 4"), [this]() { Teleport(FVector(-800.f, -300.f, 100.f)); return Use(TEXT("FTDoor"), TEXT("Keycard")); } });
	Steps.Add({ TEXT("Pick up the cardboard stand-in"), [this]() { return Carry(TEXT("FTStandIn")); } });
	Steps.Add({ TEXT("Dress the stand-in as lifeguard"), [this]() { Teleport(FVector(-1550.f, -1300.f, 100.f), 180.f); return Use(TEXT("FTCostumeRack"), TEXT("H0")); } });
	Steps.Add({ TEXT("Put the stand-in on the beach"), [this, ObjectiveDone]()
	{
		if (Crew->HeldProp)
		{
			Teleport(FVector(1990.f, -420.f, 90.f), 0.f);
			const FString Before = DescribeHands();
			Crew->ReleaseProp(false);
			Log(FString::Printf(TEXT("dropped stand-in: before [%s] after [%s]"), *Before, *DescribeHands()));
		}
		return ObjectiveDone(0);
	}, 8.f });
	Steps.Add({ TEXT("Switch on the lighthouse"), [this, ObjectiveDone]()
	{
		Teleport(FVector(2020.f, -560.f, 90.f));
		return EnsureActive(TEXT("FTLighthouse"), TEXT("Toggle"), FTTags::DevLighthouse) && ObjectiveDone(1);
	} });
	Steps.Add({ TEXT("Man the camera (scene 1)"), ClaimCamera, 15.f });
	AddShot(TEXT("03_lens_scene1"));
	Steps.Add({ TEXT("Roll, operator steps off (scene 1)"), RollCamera });
	Steps.Add({ TEXT("Hit the warning siren"), [this]() { Teleport(FVector(60.f, -1300.f, -30.f)); return Use(TEXT("FTSoundConsole"), TEXT("Cue0")); } });
	Steps.Add({ TEXT("Take 1 accepted"), [TakeAccepted]() { return TakeAccepted(0); }, 25.f });
	Steps.Add({ TEXT("Scene 2 begins"), [GS]() { return GS()->SceneIndex == 1 && GS()->SceneState == EFTSceneState::Preparation; }, 20.f });
	Steps.Add({ TEXT("Push the rescue boat into the tank"), [this, ObjectiveDone]()
	{
		AFTRescueBoat* Boat = nullptr;
		for (TActorIterator<AFTRescueBoat> It(GetWorld()); It; ++It) { Boat = *It; break; }
		if (!Boat)
		{
			return false;
		}
		TArray<UFTInteractableComponent*> Comps;
		Boat->GetComponents<UFTInteractableComponent>(Comps);
		UFTInteractableComponent* Handle = Comps.Num() ? Comps[0] : nullptr;
		if (ObjectiveDone(0))
		{
			Crew->StopUsing();
			return true;
		}
		if (Handle)
		{
			Teleport(Handle->GetComponentLocation() + FVector(-70.f, 0.f, 60.f));
			if (Crew->UsingActor != Boat)
			{
				Crew->SetUsingActor(Boat);
				Boat->OnBeginUse(Handle, Crew.Get());
			}
		}
		return false;
	}, 30.f });
	Steps.Add({ TEXT("Start wind and rain"), [this, ObjectiveDone]()
	{
		const bool bWind = EnsureActive(TEXT("FTEffectMachine"), TEXT("Toggle"), FTTags::DevWind, FTTags::DevWind);
		const bool bRain = bWind && EnsureActive(TEXT("FTEffectMachine"), TEXT("Toggle"), FTTags::DevRain, FTTags::DevRain);
		return bWind && bRain && ObjectiveDone(1) && ObjectiveDone(2);
	}, 5.f });
	Steps.Add({ TEXT("Man the camera (scene 2)"), ClaimCamera, 15.f });
	AddShot(TEXT("04_lens_scene2"));
	Steps.Add({ TEXT("Roll, operator steps off (scene 2)"), RollCamera });
	Steps.Add({ TEXT("Shark rig lunge at the boat"), [this]() { Teleport(FVector(1400.f, -1150.f, -30.f)); return Use(TEXT("FTSharkRig"), TEXT("Rig3")); } });
	Steps.Add({ TEXT("Take 2 accepted"), [TakeAccepted]() { return TakeAccepted(1); }, 25.f });
	Steps.Add({ TEXT("Flood: valve bursts (Leaking)"), [GS]() { return GS()->FloodStage != EFTFloodStage::Dry && !GS()->bStagePower; }, 25.f });
	Steps.Add({ TEXT("Flood: stage fully flooded"), [GS]() { return GS()->FloodStage == EFTFloodStage::Flooded; }, 40.f });
	if (bShots)
	{
		Steps.Add({ TEXT("Look over the flooded stage"), [this]() { Teleport(FVector(-400.f, 300.f, 100.f), 20.f); return StepTime > 2.f; } });
		AddShot(TEXT("05_flooded"));
	}
	Steps.Add({ TEXT("Scene 3 begins"), [GS]() { return GS()->SceneIndex == 2; }, 20.f });
	Steps.Add({ TEXT("Restore stage power at the breaker"), [this, GS]() { Teleport(FVector(-460.f, 650.f, 100.f), 180.f); Use(TEXT("FTBreaker"), TEXT("Restore")); return GS()->bStagePower; } });
	Steps.Add({ TEXT("Hero spotlight on"), [this, ObjectiveDone]()
	{
		Teleport(FVector(330.f, 620.f, 30.f));
		return EnsureActive(TEXT("FTLightingBoard"), TEXT("T3"), FTTags::DevHeroLight) && ObjectiveDone(2);
	} });
	Steps.Add({ TEXT("Fetch the hero harpoon from the warehouse"), [this]() { return Crew->HeldProp || Carry(TEXT("FTProp_Harpoon")); } });
	Steps.Add({ TEXT("Hand the harpoon to the stand-in"), [this]()
	{
		for (TActorIterator<AFTStandIn> It(GetWorld()); It; ++It)
		{
			Teleport(It->GetActorLocation() + FVector(-120.f, 0.f, 100.f));
			break;
		}
		return Use(TEXT("FTStandIn"), TEXT("Hand"));
	} });
	Steps.Add({ TEXT("Carry the stand-in onto the hero mark"), [this, ObjectiveDone]()
	{
		if (!Crew->HeldProp)
		{
			Carry(TEXT("FTStandIn"));
			return false;
		}
		Teleport(FVector(1480.f, 880.f, 70.f), 0.f);
		Crew->ReleaseProp(false);
		return true;
	} });
	Steps.Add({ TEXT("Hero objective satisfied"), [ObjectiveDone]() { return ObjectiveDone(1); }, 8.f });
	Steps.Add({ TEXT("Raise the shark on the rig"), [this, ObjectiveDone]()
	{
		Teleport(FVector(1400.f, -1150.f, -30.f));
		return EnsureActive(TEXT("FTSharkRig"), TEXT("Rig0"), TEXT("Device.SharkRig")) && ObjectiveDone(3);
	} });
	Steps.Add({ TEXT("Man the camera (scene 3)"), ClaimCamera, 15.f });
	AddShot(TEXT("06_lens_scene3"));
	Steps.Add({ TEXT("Roll, operator steps off (scene 3)"), RollCamera });
	Steps.Add({ TEXT("Lunge for the staged defeat"), [this]() { return Use(TEXT("FTSharkRig"), TEXT("Rig3")); } });
	Steps.Add({ TEXT("Take 3 accepted"), [TakeAccepted]() { return TakeAccepted(2); }, 25.f });
	Steps.Add({ TEXT("Finale: projection room unlocked"), [GS]() { return GS()->ShootPhase == EFTShootPhase::Finale && GS()->bProjectionUnlocked; }, 20.f });
	Steps.Add({ TEXT("Load all reels into the projector"), [this, GS]()
	{
		if (GS()->ReelsLoaded >= GS()->GetCompletedTakeCount())
		{
			return true;
		}
		if (!Crew->HeldProp)
		{
			Carry(TEXT("FTProp_Reel"));
		}
		Teleport(FVector(20.f, -1125.f, 520.f));
		Use(TEXT("FTProjector"), TEXT("Load"));
		return false;
	}, 15.f });
	Steps.Add({ TEXT("Restore projector power"), [this, GS]() { Use(TEXT("FTProjector"), TEXT("Power")); return GS()->bProjectorPower; } });
	Steps.Add({ TEXT("Start the premiere"), [this, GS]() { Use(TEXT("FTProjector"), TEXT("Start")); return GS()->ShootPhase == EFTShootPhase::Premiere; } });
	if (bShots)
	{
		Steps.Add({ TEXT("Watch the premiere"), [this]() { Teleport(FVector(200.f, -1250.f, 520.f), 30.f); return StepTime > 9.f; } });
		AddShot(TEXT("07_premiere"));
	}
	Steps.Add({ TEXT("Premiere ends on the results"), [this, GS]()
	{
		if (GS()->ShootPhase != EFTShootPhase::Results)
		{
			return false;
		}
		Log(FString::Printf(TEXT("results: team score %d, style %d, disaster %d, takes %d, condition %.0f"),
			GS()->TeamScore, GS()->StyleBonus, GS()->DisasterBonus, GS()->GetCompletedTakeCount(), GS()->StudioCondition));
		return true;
	}, 60.f });
	if (bShots)
	{
		Steps.Add({ TEXT("Results screen settles"), [this]() { return StepTime > 2.f; } });
		AddShot(TEXT("08_results"));
	}
	Steps.Add({ TEXT("Retry shoot resets everything"), [this, GS, SM, PC]()
	{
		SM()->RequestRestart(PC());
		AFTGameState* G = GS();
		bool bHarpoonHome = false;
		for (TActorIterator<AFTProp_Harpoon> It(GetWorld()); It; ++It)
		{
			bHarpoonHome = It->DistanceFromHome() < 5.f && !It->GetCarrier() && !It->GetHolder();
		}
		int32 Reels = 0;
		for (TActorIterator<AFTProp_Reel> It(GetWorld()); It; ++It)
		{
			Reels += IsValid(*It) && !It->IsActorBeingDestroyed() ? 1 : 0;
		}
		Log(FString::Printf(TEXT("after restart: phase=%d flood=%d power=%d takes=%d harpoonHome=%d reelsLeft=%d"),
			(int32)G->ShootPhase, (int32)G->FloodStage, G->bStagePower ? 1 : 0, G->TakeResults.Num(), bHarpoonHome ? 1 : 0, Reels));
		return G->ShootPhase == EFTShootPhase::Lobby && G->FloodStage == EFTFloodStage::Dry && G->bStagePower && G->TakeResults.Num() == 0 && bHarpoonHome;
	}, 5.f });
}

void AFTAutoDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDone)
	{
		return;
	}
	const double Stamp = FPlatformTime::Seconds();
	if (LastFrameStamp > 0.0 && GetWorld()->GetTimeSeconds() > 5.f)
	{
		FrameTimes.Add(static_cast<float>(Stamp - LastFrameStamp));
	}
	LastFrameStamp = Stamp;
	if (Delay > 0.f)
	{
		Delay -= DeltaSeconds;
		return;
	}
	if (!Steps.IsValidIndex(Current))
	{
		Finish();
		return;
	}
	FStep& S = Steps[Current];
	StepTime += DeltaSeconds;
	const bool bOk = S.Run ? S.Run() : true;
	if (bOk)
	{
		Log(FString::Printf(TEXT("PASS %s   [%s]"), *S.Name, *DescribeHands()));
		LastUseFailure.Reset();
		++Current;
		StepTime = 0.f;
		Delay = 1.2f;
	}
	else if (StepTime > S.Timeout)
	{
		Log(FString::Printf(TEXT("%s (timeout %.0fs)   [%s] [%s]"), *S.Name, S.Timeout, *DescribeHands(), *DescribeObjectives()), true);
		LastUseFailure.Reset();
		++Current;
		StepTime = 0.f;
		Delay = 0.5f;
	}
}

void AFTAutoDirector::Finish()
{
	bDone = true;
	if (FrameTimes.Num() > 10)
	{
		TArray<float> Sorted = FrameTimes;
		Sorted.Sort();
		double Sum = 0.0;
		int32 Hitches = 0;
		for (const float F : FrameTimes)
		{
			Sum += F;
			Hitches += F > 0.1f ? 1 : 0;
		}
		const float Avg = static_cast<float>(Sum / FrameTimes.Num());
		const float P99 = Sorted[FMath::Min(Sorted.Num() - 1, FMath::FloorToInt(Sorted.Num() * 0.99f))];
		Log(FString::Printf(TEXT("performance: avg %.1f fps (%.1f ms), 1%% low %.1f fps, worst frame %.0f ms, %d hitch(es) > 100 ms over %d frames"),
			1.f / Avg, Avg * 1000.f, 1.f / P99, Sorted.Last() * 1000.f, Hitches, FrameTimes.Num()));
	}
	Log(FString::Printf(TEXT("AutoTest finished: %d step(s), %d failure(s)"), Steps.Num(), Failures));
	FFileHelper::SaveStringArrayToFile(Report, *(FPaths::ProjectSavedDir() / TEXT("FTAutoTestResult.txt")));
	if (FParse::Param(FCommandLine::Get(), TEXT("FTAutoQuit")))
	{
		FPlatformMisc::RequestExit(false, TEXT("FTAutoTest finished"));
	}
}
