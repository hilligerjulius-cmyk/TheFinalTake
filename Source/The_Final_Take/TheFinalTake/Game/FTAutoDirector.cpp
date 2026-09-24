#include "TheFinalTake/Game/FTAutoDirector.h"

#include "The_Final_Take.h"
#include "TheFinalTake/Career/FTCareerManager.h"
#include "TheFinalTake/Career/FTEconomy.h"
#include "TheFinalTake/Career/FTShopItems.h"
#include "TheFinalTake/Characters/FTCharacter.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Game/FTPlayerController.h"
#include "TheFinalTake/Game/FTSceneManager.h"
#include "TheFinalTake/Interaction/FTInteractableComponent.h"
#include "TheFinalTake/Interaction/FTStudioActor.h"
#include "TheFinalTake/Production/FTFilmCamera.h"
#include "TheFinalTake/Props/FTProp.h"
#include "TheFinalTake/Props/FTSetPieces.h"
#include "TheFinalTake/World/FTCity.h"

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
	Steps.Add({ TEXT("Career slot loaded on the host"), [this, GS]()
	{
		AFTGameState* G = GS();
		if (!G || G->CareerSlot <= 0)
		{
			return false;
		}
		StartMoney = G->StudioMoney;
		StartFilms = G->CareerFilmsTotal;
		Log(FString::Printf(TEXT("career slot %d '%s': %s, %d film(s) released, %d item(s), %d car(s), %d stage(s)"), G->CareerSlot, *G->StudioName,
			*FFTEconomy::MoneyString(G->StudioMoney), G->CareerFilmsTotal, G->OwnedItems.Num(), G->OwnedVehicles.Num(), G->UnlockedStages.Num()));
		return true;
	} });
	// ------------------------------------------------ -FTDawnTest: a short night that nobody finishes (fair loss at dawn)
	if (FParse::Param(FCommandLine::Get(), TEXT("FTDawnTest")))
	{
		Steps.Add({ TEXT("Greenlight a film with a short night"), [this, SM, PC, GS]()
		{
			FText Why;
			if (!SM() || !PC() || !SM()->RequestOpenScriptBook(PC(), Why) || !SM()->RequestSelectFilm(PC(), FTTags::FilmJaws, Why))
			{
				return false;
			}
			Log(FString::Printf(TEXT("night length %.0f s, dawn in %.0f s"), GS()->DawnDuration, GS()->GetDawnRemaining()), GS()->DawnDuration > 120.f);
			return GS()->ShootPhase == EFTShootPhase::Shooting;
		} });
		Steps.Add({ TEXT("Dawn breaks: the shoot fails fairly"), [this, GS]()
		{
			if (GS()->ShootPhase != EFTShootPhase::Failed)
			{
				return false;
			}
			const bool bDawn = GS()->FailReason.ToString().Contains(TEXT("DAWN"));
			Log(FString::Printf(TEXT("failed with: %s (money unchanged: %s)"), *GS()->FailReason.ToString(), GS()->StudioMoney == StartMoney ? TEXT("yes") : TEXT("NO")), !bDawn || GS()->StudioMoney != StartMoney);
			return true;
		}, 200.f });
		Steps.Add({ TEXT("Restart after the failed night"), [this, SM, PC, GS]()
		{
			SM()->RequestRestart(PC());
			return GS()->ShootPhase == EFTShootPhase::Lobby && GS()->GetDawnRemaining() > 0.f;
		}, 5.f });
		return;
	}

	// ------------------------------------------------ studio shop (WP2)
	// fresh career: $5,000 buys exactly the test kit (a costume for the stand-in + a practical effect)
	static const FName TestCostume(TEXT("Acc.DirectorBeret"));
	static const FName TestEffect(TEXT("Fx.Bubbles"));
	Steps.Add({ TEXT("Shop: buy the test kit for the team"), [this, GS, PC, WantPlayers]()
	{
		AFTGameState* G = GS();
		const UFTEconomyConfig* Cfg = UFTEconomyConfig::Get();
		for (const FName Id : { TestCostume, TestEffect })
		{
			if (G->IsOwned(Id))
			{
				continue;
			}
			const FFTShopItemDef* Def = Cfg->FindItem(Id);
			// multiplayer: the costume is bought by the remote client through its own RPC (client-initiated path)
			AFTPlayerController* Buyer = PC();
			if (WantPlayers > 1 && Id == TestCostume)
			{
				for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
				{
					if (AFTPlayerController* P = Cast<AFTPlayerController>(It->Get()); P && !P->IsLocalController())
					{
						if (StepTime < 0.2f || FMath::Fmod(StepTime, 5.f) < 0.05f)
						{
							P->ClientAutoTestAction(TEXT("Purchase"), Id);
						}
						return false; // wait for the client's request to arrive and be booked
					}
				}
			}
			if (!Buyer || !Def)
			{
				return false;
			}
			const int32 Before = G->StudioMoney;
			Buyer->ServerPurchase(Id, 900 + Current);
			if (!G->IsOwned(Id) || G->StudioMoney != Before - Def->Price)
			{
				Log(FString::Printf(TEXT("purchase of %s failed: %s"), *Id.ToString(), *Buyer->LastPurchaseMessage.ToString()), true);
				return true;
			}
		}
		return G->IsOwned(TestCostume) && G->IsOwned(TestEffect);
	}, 20.f });
	Steps.Add({ TEXT("Shop: team account charged exactly once per item"), [this, GS]()
	{
		AFTGameState* G = GS();
		const UFTEconomyConfig* Cfg = UFTEconomyConfig::Get();
		const bool bFreshKit = StartMoney == G->StudioMoney + Cfg->FindItem(TestCostume)->Price + Cfg->FindItem(TestEffect)->Price;
		SpentMoney = StartMoney - G->StudioMoney;
		Log(FString::Printf(TEXT("shop: spent %s, balance %s (%s)"), *FFTEconomy::MoneyString(SpentMoney), *FFTEconomy::MoneyString(G->StudioMoney),
			bFreshKit ? TEXT("bought both test items now") : (SpentMoney == 0 ? TEXT("kit already owned from an earlier run") : TEXT("partial"))), SpentMoney != 0 && !bFreshKit);
		return true;
	} });
	Steps.Add({ TEXT("Shop: invalid purchases are rejected without charging"), [this, GS]()
	{
		AFTGameState* G = GS();
		AFTCareerManager* CM = AFTCareerManager::Get(this);
		AFTPlayerController* P = Crew.IsValid() ? Cast<AFTPlayerController>(Crew->GetController()) : nullptr;
		const UFTEconomyConfig* Cfg = UFTEconomyConfig::Get();
		const int32 Money = G->StudioMoney;
		FText Why;
		const bool bDouble = CM->TryPurchase(P, TestCostume, Why);
		Log(FString::Printf(TEXT("buy owned item again -> %s"), *Why.ToString()), bDouble || G->StudioMoney != Money);
		const bool bUnknown = CM->TryPurchase(P, TEXT("Acc.DoesNotExist"), Why);
		Log(FString::Printf(TEXT("buy unknown item -> %s"), *Why.ToString()), bUnknown || G->StudioMoney != Money);
		FName TooExpensive;
		int32 Price = 0;
		EFTShopCategory Cat = EFTShopCategory::Prop;
		FText Name;
		for (const FFTShopItemDef& D : Cfg->Items) { if (!G->IsOwned(D.ItemId) && D.Price > Money) { TooExpensive = D.ItemId; } }
		for (const FFTStageDef& D : Cfg->Stages) { if (!G->IsOwned(D.StageId) && D.Price > Money) { TooExpensive = D.StageId; } }
		if (!TooExpensive.IsNone() && Cfg->Describe(TooExpensive, Price, Cat, Name))
		{
			const bool bRich = CM->TryPurchase(P, TooExpensive, Why);
			Log(FString::Printf(TEXT("buy %s (%s) with %s -> %s"), *TooExpensive.ToString(), *FFTEconomy::MoneyString(Price), *FFTEconomy::MoneyString(Money), *Why.ToString()), bRich || G->StudioMoney != Money);
		}
		else
		{
			Log(TEXT("(every unowned entry is affordable - not-enough-money case skipped this run)"));
		}
		return true;
	} });
	Steps.Add({ TEXT("Shop: bought effect waits in a delivery bay (or its saved spot)"), [this]()
	{
		AFTShopItemActor* A = FTShop::FindItemActor(GetWorld(), TestEffect);
		AFTCareerManager* CM = AFTCareerManager::Get(this);
		FTransform Saved;
		const bool bSaved = CM && CM->GetPlacement(TestEffect, Saved);
		if (A)
		{
			Log(FString::Printf(TEXT("%s spawned at %s, saved placement %s (distance %.0f)"), *TestEffect.ToString(), *A->GetActorLocation().ToCompactString(),
				bSaved ? *Saved.GetLocation().ToCompactString() : TEXT("none"), bSaved ? FVector::Dist(Saved.GetLocation(), A->GetActorLocation()) : -1.f),
				!bSaved || FVector::Dist(Saved.GetLocation(), A->GetActorLocation()) > 60.f);
		}
		return A != nullptr;
	}, 5.f });
	AddShot(TEXT("01_lobby"));
	if (bShots)
	{
		Steps.Add({ TEXT("Look at the Studio Supply counter"), [this]() { Teleport(FVector(-830.f, 520.f, 100.f), 90.f); return StepTime > 1.5f; } });
		AddShot(TEXT("01b_supply_counter"));
		Steps.Add({ TEXT("Open the shop catalogue"), [this]() { return StepTime > 0.3f && Use(TEXT("FTShopTerminal"), TEXT("Browse")); } });
		Steps.Add({ TEXT("Catalogue preview settles"), [this]() { return StepTime > 2.5f; } });
		AddShot(TEXT("01c_shop"));
		Steps.Add({ TEXT("Close the shop catalogue"), [this]()
		{
			if (AFTPlayerController* P = Crew.IsValid() ? Cast<AFTPlayerController>(Crew->GetController()) : nullptr)
			{
				P->LocalCloseShop();
			}
			return true;
		} });
		Steps.Add({ TEXT("Look at the accessory wall"), [this]() { Teleport(FVector(-1190.f, -1230.f, 100.f), -90.f); return StepTime > 1.5f; } });
		AddShot(TEXT("01d_accessory_wall"));
		Steps.Add({ TEXT("Look at the loading bay"), [this]() { Teleport(FVector(-1300.f, 580.f, 100.f), 167.f); return StepTime > 1.5f; } });
		AddShot(TEXT("01e_loading_bay"));
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
	Steps.Add({ TEXT("Dress the stand-in with the purchased beret at the accessory wall"), [this]()
	{
		AFTStandIn* S = Crew.IsValid() ? Cast<AFTStandIn>(Crew->HeldProp) : nullptr;
		if (!S)
		{
			return false;
		}
		if (S->GetAccessory(EFTAccessorySlot::Head) == TestCostume)
		{
			return true; // still dressed from an earlier session (persisted)
		}
		int32 Hook = 0;
		for (const FFTShopItemDef& D : UFTEconomyConfig::Get()->Items)
		{
			if (D.Category == EFTShopCategory::Costume)
			{
				if (D.ItemId == TestCostume)
				{
					break;
				}
				++Hook;
			}
		}
		Teleport(FVector(-1190.f, -1380.f, 100.f), -90.f);
		Use(TEXT("FTAccessoryStand"), *FString::Printf(TEXT("Hook%d"), Hook));
		return S->GetAccessory(EFTAccessorySlot::Head) == TestCostume;
	} });
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
	Steps.Add({ TEXT("Carry the bubble machine onto the beach"), [this]()
	{
		AFTShopItemActor* A = FTShop::FindItemActor(GetWorld(), TestEffect);
		if (!A)
		{
			return false;
		}
		// between the lifeguard stand-in and the lighthouse, so the locked-off scene 1 shot sees it
		if (FVector::Dist2D(A->GetActorLocation(), FVector(1960.f, -560.f, 0.f)) < 90.f && !A->GetCarrier())
		{
			return true;
		}
		if (Crew->HeldProp != A)
		{
			Crew->ReleaseProp(false);
			Teleport(A->GetActorLocation() + FVector(-120.f, 0.f, 100.f));
			Crew->TakeProp(A);
			return false;
		}
		Teleport(FVector(1905.f, -560.f, 90.f), 0.f);
		Crew->ReleaseProp(false);
		return false;
	}, 10.f });
	Steps.Add({ TEXT("Switch the bubble machine on (placement saved)"), [this]()
	{
		AFTShopItemActor* A = FTShop::FindItemActor(GetWorld(), TestEffect);
		AFTCareerManager* CM = AFTCareerManager::Get(this);
		FTransform Saved;
		if (!A || !CM || !CM->GetPlacement(TestEffect, Saved) || FVector::Dist(Saved.GetLocation(), A->GetActorLocation()) > 5.f)
		{
			return false; // still falling / not landed yet
		}
		if (!A->IsDeviceActive())
		{
			Teleport(A->GetActorLocation() + FVector(-140.f, 0.f, 90.f), 0.f);
			Use(TEXT("FTShopItemActor"), TEXT("Switch"));
		}
		return A->IsDeviceActive() && A->IsShowcased();
	}, 8.f });
	Steps.Add({ TEXT("Man the camera (scene 1)"), ClaimCamera, 15.f });
	AddShot(TEXT("03_lens_scene1"));
	Steps.Add({ TEXT("Roll, operator steps off (scene 1)"), RollCamera });
	Steps.Add({ TEXT("Hit the warning siren"), [this]() { Teleport(FVector(60.f, -1300.f, -30.f)); return Use(TEXT("FTSoundConsole"), TEXT("Cue0")); } });
	Steps.Add({ TEXT("Take 1 accepted"), [TakeAccepted]() { return TakeAccepted(0); }, 25.f });
	Steps.Add({ TEXT("Take 1 counted the purchased upgrades that were in the shot"), [this, GS]()
	{
		const FFTTakeResult* R = GS()->GetBestTake(0);
		if (!R)
		{
			return false;
		}
		FString Seen;
		for (const FName Id : R->VisibleItems)
		{
			Seen += Id.ToString() + TEXT(" ");
		}
		const bool bBoth = R->VisibleItems.Contains(TestCostume) && R->VisibleItems.Contains(TestEffect);
		Log(FString::Printf(TEXT("take 1 visible upgrades: %s(style %d)"), Seen.IsEmpty() ? TEXT("none ") : *Seen, R->StylePoints), !bBoth);
		return true;
	} });
	Steps.Add({ TEXT("Switch the bubble machine off again"), [this]()
	{
		AFTShopItemActor* A = FTShop::FindItemActor(GetWorld(), TestEffect);
		if (A && A->IsDeviceActive())
		{
			Teleport(A->GetActorLocation() + FVector(-140.f, 0.f, 90.f), 0.f);
			Use(TEXT("FTShopItemActor"), TEXT("Switch"));
		}
		return A && !A->IsDeviceActive() && !A->IsShowcased();
	} });
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
	Steps.Add({ TEXT("Switched-off effect did not count in take 2"), [this, GS]()
	{
		const FFTTakeResult* R = GS()->GetBestTake(1);
		if (!R)
		{
			return false;
		}
		Log(FString::Printf(TEXT("take 2 visible upgrades: %d item(s), bubble machine %s"), R->VisibleItems.Num(), R->VisibleItems.Contains(TestEffect) ? TEXT("COUNTED") : TEXT("not counted")),
			R->VisibleItems.Contains(TestEffect));
		return true;
	} });
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
	Steps.Add({ TEXT("Test screening in the studio projection room (optional)"), [this, GS]()
	{
		Teleport(FVector(20.f, -1125.f, 520.f));
		if (!GS()->bProjectorPower)
		{
			Use(TEXT("FTProjector"), TEXT("Power"), TEXT("Projector.Studio"));
			return false;
		}
		if (!GS()->IsTestScreeningActive())
		{
			Use(TEXT("FTProjector"), TEXT("Start"), TEXT("Projector.Studio"));
		}
		return GS()->IsTestScreeningActive() && GS()->ShootPhase == EFTShootPhase::Finale;
	}, 10.f });
	if (bShots)
	{
		Steps.Add({ TEXT("Watch the test screening"), [this]() { Teleport(FVector(200.f, -1250.f, 520.f), 30.f); return StepTime > 9.f; } });
		AddShot(TEXT("07a_test_screening"));
	}
	Steps.Add({ TEXT("Pack every reel into the film case"), [this, GS]()
	{
		if (GS()->ReelsPacked >= GS()->GetCompletedTakeCount())
		{
			return true;
		}
		if (!Cast<AFTProp_Reel>(Crew->HeldProp))
		{
			Crew->ReleaseProp(false);
			Carry(TEXT("FTProp_Reel"));
			return false;
		}
		for (TActorIterator<AFTFilmCase> It(GetWorld()); It; ++It)
		{
			Teleport(It->GetActorLocation() + FVector(-110.f, 0.f, 100.f));
		}
		Use(TEXT("FTFilmCase"), TEXT("Carry"));
		return false;
	}, 20.f });
	// no softlock on foot: the whole walk from Stage 4 to the cinema booth must be open for a crew-sized capsule
	Steps.Add({ TEXT("Walking route studio -> Grand Cinema is open"), [this]()
	{
		const FVector Route[] = { FVector(-450.f, 0.f, 100.f), FVector(-1300.f, 0.f, 100.f), FVector(-2000.f, 0.f, 100.f), FVector(-3000.f, -150.f, 100.f),
			FVector(-5500.f, -150.f, 100.f), FVector(-8500.f, -150.f, 100.f), FVector(-10500.f, -150.f, 100.f), FVector(-11600.f, -150.f, 100.f),
			FVector(-12720.f, -50.f, 100.f), FVector(-11600.f, -150.f, 100.f), FVector(-11330.f, -700.f, 100.f), FVector(-11330.f, -1250.f, 100.f) };
		FCollisionQueryParams Params(SCENE_QUERY_STAT(FTRoute), false, Crew.Get());
		const FCollisionShape Capsule = FCollisionShape::MakeCapsule(34.f, 60.f);
		int32 Blocked = 0;
		for (int32 i = 0; i + 1 < UE_ARRAY_COUNT(Route); ++i)
		{
			FHitResult Hit;
			if (GetWorld()->SweepSingleByChannel(Hit, Route[i], Route[i + 1], FQuat::Identity, ECC_Pawn, Capsule, Params))
			{
				++Blocked;
				Log(FString::Printf(TEXT("route blocked between %s and %s by %s"), *Route[i].ToCompactString(), *Route[i + 1].ToCompactString(), Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("?")), true);
			}
		}
		// the booth stairs: every step must have floor under it and the mezzanine must be reachable
		int32 MissingSteps = 0;
		for (int32 k = 0; k < 21; ++k)
		{
			FHitResult Hit;
			const FVector P(-11400.f - 30.f * (k + 0.5f), -1250.f, 24.f + 20.f * (k + 1) + 60.f);
			if (!GetWorld()->LineTraceSingleByChannel(Hit, P, P - FVector(0.f, 0.f, 150.f), ECC_Visibility, Params) || FMath::Abs(Hit.ImpactPoint.Z - (4.f + 20.f * (k + 1))) > 6.f)
			{
				++MissingSteps;
			}
		}
		Log(FString::Printf(TEXT("route check: %d blocked segment(s), %d stair step(s) without floor"), Blocked, MissingSteps), MissingSteps > 0);
		return true;
	} });
	if (bShots)
	{
		Steps.Add({ TEXT("Look down the boulevard"), [this]() { Teleport(FVector(-3500.f, -150.f, 120.f), 180.f); return StepTime > 2.f; } });
		AddShot(TEXT("07b_boulevard"));
		Steps.Add({ TEXT("Look at the Grand Cinema"), [this]() { Teleport(FVector(-9700.f, -250.f, 120.f), 180.f); return StepTime > 2.f; } });
		AddShot(TEXT("07c_cinema_exterior"));
		Steps.Add({ TEXT("Look around the foyer"), [this]() { Teleport(FVector(-11350.f, 300.f, 120.f), 200.f); return StepTime > 2.f; } });
		AddShot(TEXT("07d_foyer"));
	}
	Steps.Add({ TEXT("Carry the film case to the Grand Cinema booth"), [this]()
	{
		AFTFilmCase* Case = nullptr;
		for (TActorIterator<AFTFilmCase> It(GetWorld()); It; ++It) { Case = *It; }
		if (!Case)
		{
			return false;
		}
		if (Crew->HeldProp != Case)
		{
			Crew->ReleaseProp(false);
			Carry(TEXT("FTFilmCase"));
			return false;
		}
		Teleport(FVector(-12200.f, -1000.f, 520.f), 180.f);
		return Case->GetCarrier() == Crew.Get() && Crew->GetActorLocation().X < -12000.f;
	} });
	Steps.Add({ TEXT("Load the reels into the cinema projector"), [this, GS]()
	{
		Use(TEXT("FTProjector"), TEXT("Load"), TEXT("Projector.Premiere"));
		return GS()->ReelsLoaded >= GS()->GetCompletedTakeCount() && GS()->ReelsPacked == 0;
	} });
	Steps.Add({ TEXT("Start the premiere in the Grand Cinema"), [this, GS]() { Use(TEXT("FTProjector"), TEXT("Start"), TEXT("Projector.Premiere")); return GS()->ShootPhase == EFTShootPhase::Premiere; } });
	Steps.Add({ TEXT("Crew is seated in the cinema hall"), [this]()
	{
		const FVector L = Crew->GetActorLocation();
		const bool bInHall = L.X < -12650.f && L.X > -14700.f && L.Y > -1350.f && L.Y < 1050.f;
		Log(FString::Printf(TEXT("crew seat at %s"), *L.ToCompactString()), !bInHall);
		return true;
	} });
	Steps.Add({ TEXT("Audience fills the hall for the release"), [this, GS]()
	{
		const AFTCinemaSeating* Seats = AFTCinemaSeating::Find(GetWorld());
		const FFTReleaseReport& R = GS()->LastRelease;
		if (!Seats || !R.bValid)
		{
			return false;
		}
		const int32 Expected = FMath::RoundToInt(FFTEconomy::FillRatio(R.Audience, *UFTEconomyConfig::Get()) * Seats->GetAudienceSeats());
		if (Seats->GetShownAudience() < Seats->GetTargetAudience() && StepTime < 8.f)
		{
			return false;
		}
		Log(FString::Printf(TEXT("crowd: %d of %d seats for %d viewers (expected %d, shown %d)"), Seats->GetTargetAudience(), Seats->GetAudienceSeats(), R.Audience, Expected, Seats->GetShownAudience()),
			Seats->GetTargetAudience() != Expected || Seats->GetShownAudience() != Expected);
		return true;
	}, 10.f });
	if (bShots)
	{
		Steps.Add({ TEXT("Watch the premiere"), [this]() { return StepTime > 9.f; } });
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
	Steps.Add({ TEXT("Release booked into the career"), [this, GS]()
	{
		AFTGameState* G = GS();
		const FFTReleaseReport& R = G->LastRelease;
		if (!R.bValid)
		{
			return false;
		}
		Log(FString::Printf(TEXT("release #%d: quality %d, production %d (%d item(s)), arrival +%d%%, %d viewers, revenue %s, %d stars, %s, rank %d/%d"),
			R.ReleaseNumber, R.Quality, R.ProductionPoints, R.VisibleItems.Num(), R.ArrivalBonusPct, R.Audience, *FFTEconomy::MoneyString(R.Revenue), R.Stars,
			*FFTEconomy::TierName(R.Tier).ToString(), R.Rank, R.RankOf));
		for (const FFTReleaseLine& L : R.Lines)
		{
			Log(FString::Printf(TEXT("   %+6d  %s"), L.Audience, *L.Label.ToString()));
		}
		for (int32 i = 0; i < R.Reviews.Num(); ++i)
		{
			Log(FString::Printf(TEXT("   review: %s"), *R.Reviews[i].ToString()));
		}
		int32 StyleSum = 0;
		for (const FName Id : R.VisibleItems)
		{
			const FFTShopItemDef* D = UFTEconomyConfig::Get()->FindItem(Id);
			StyleSum += D ? D->StyleValue : 0;
		}
		const int32 ExpectedProduction = FMath::Min(StyleSum, UFTEconomyConfig::Get()->MaxProductionPoints);
		Log(FString::Printf(TEXT("production value: %d point(s) from %d visible upgrade(s) (expected %d)"), R.ProductionPoints, R.VisibleItems.Num(), ExpectedProduction),
			R.ProductionPoints != ExpectedProduction || !R.VisibleItems.Contains(TestCostume));
		const int32 Expected = StartMoney - SpentMoney + R.Revenue;
		const bool bBooked = G->StudioMoney == Expected && G->CareerFilmsTotal == StartFilms + 1;
		Log(FString::Printf(TEXT("studio account %s (expected %s), films %d"), *FFTEconomy::MoneyString(G->StudioMoney), *FFTEconomy::MoneyString(Expected), G->CareerFilmsTotal), !bBooked);
		return true;
	}, 10.f });
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
		bool bCaseHome = false;
		for (TActorIterator<AFTFilmCase> It(GetWorld()); It; ++It)
		{
			bCaseHome = It->DistanceFromHome() < 5.f && It->NumReels() == 0;
		}
		Log(FString::Printf(TEXT("film case home and empty: %d, crew back in the studio: %d"), bCaseHome ? 1 : 0, Crew->GetActorLocation().X > -2000.f ? 1 : 0), !bCaseHome || Crew->GetActorLocation().X < -2000.f);
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
