#include "TheFinalTake/Game/FTSceneManager.h"

#include "The_Final_Take.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Game/FTPlayerState.h"
#include "TheFinalTake/Characters/FTCharacter.h"
#include "TheFinalTake/Data/FTFilmDefinition.h"
#include "TheFinalTake/Interaction/FTStudioActor.h"
#include "TheFinalTake/Production/FTFilmCamera.h"
#include "TheFinalTake/Props/FTProp.h"
#include "TheFinalTake/Props/FTSetPieces.h"
#include "TheFinalTake/World/FTFloodController.h"
#include "TheFinalTake/World/FTZone.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"

#define LOCTEXT_NAMESPACE "FinalTakeScene"

namespace
{
	EFTCostume CostumeFromName(FName Name)
	{
		if (Name == TEXT("Lifeguard")) return EFTCostume::Lifeguard;
		if (Name == TEXT("Shark")) return EFTCostume::Shark;
		if (Name == TEXT("Raincoat")) return EFTCostume::Raincoat;
		if (Name == TEXT("FoamKnight")) return EFTCostume::FoamKnight;
		return EFTCostume::None;
	}

	bool IsSetupType(EFTObjectiveType T)
	{
		return T != EFTObjectiveType::KeyAction && T != EFTObjectiveType::HoldShot && T != EFTObjectiveType::EventDuringRecording;
	}

	FText RatingText(EFTTakeRating R)
	{
		switch (R)
		{
		case EFTTakeRating::Perfect: return LOCTEXT("Perfect", "PERFECT");
		case EFTTakeRating::Great: return LOCTEXT("Great", "GREAT");
		case EFTTakeRating::Usable: return LOCTEXT("Usable", "USABLE");
		default: return LOCTEXT("Retake", "RETAKE");
		}
	}
}

AFTSceneManager::AFTSceneManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
}

AFTSceneManager* AFTSceneManager::Get(const UObject* WorldContext)
{
	static TWeakObjectPtr<AFTSceneManager> Cached;
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || World->GetNetMode() == NM_Client)
	{
		return nullptr;
	}
	if (Cached.IsValid() && Cached->GetWorld() == World)
	{
		return Cached.Get();
	}
	for (TActorIterator<AFTSceneManager> It(World); It; ++It)
	{
		Cached = *It;
		return *It;
	}
	return nullptr;
}

AFTGameState* AFTSceneManager::GS() const
{
	return GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
}

void AFTSceneManager::BeginPlay()
{
	Super::BeginPlay();
	if (AFTGameState* G = GS())
	{
		G->DawnDuration = DawnSeconds;
		G->FrozenRemaining = DawnSeconds;
		G->ShootPhase = G->bTitleMode ? EFTShootPhase::Title : EFTShootPhase::Lobby;
		G->NotifyStateChanged();
	}
}

// ============================================================================ requests

bool AFTSceneManager::RequestOpenScriptBook(APlayerController* PC, FText& OutReason)
{
	AFTGameState* G = GS();
	if (!G || !PC)
	{
		return false;
	}
	if (G->ScriptBookUser && G->ScriptBookUser != PC->PlayerState)
	{
		if (const AFTPlayerState* Other = Cast<AFTPlayerState>(G->ScriptBookUser))
		{
			OutReason = FText::Format(LOCTEXT("BookBusy", "{0} is reading the script"), FText::FromString(Other->GetCrewName()));
		}
		return false;
	}
	G->ScriptBookUser = PC->PlayerState;
	BookUser = PC;
	G->NotifyStateChanged();
	return true;
}

void AFTSceneManager::CloseScriptBook(APlayerController* PC)
{
	AFTGameState* G = GS();
	if (G && PC && G->ScriptBookUser == PC->PlayerState)
	{
		G->ScriptBookUser = nullptr;
		BookUser.Reset();
		G->NotifyStateChanged();
	}
}

bool AFTSceneManager::RequestSelectFilm(APlayerController* PC, FName FilmId, FText& OutReason)
{
	AFTGameState* G = GS();
	if (!G || !PC)
	{
		return false;
	}
	if (G->ScriptBookUser != PC->PlayerState)
	{
		OutReason = LOCTEXT("NotBookUser", "Open the script book first");
		return false;
	}
	const UFTFilmDefinition* Film = UFTFilmDefinition::Find(FilmId);
	if (!Film || !Film->bPlayable || Film->Scenes.Num() == 0)
	{
		OutReason = LOCTEXT("Locked", "That script is still in pre-production");
		return false;
	}
	const bool bSelectable = G->ShootPhase == EFTShootPhase::Lobby || G->ShootPhase == EFTShootPhase::Results || G->ShootPhase == EFTShootPhase::Failed;
	if (!bSelectable)
	{
		OutReason = LOCTEXT("ShootRunning", "A shoot is already running - restart from the pause menu");
		return false;
	}
	if (G->ShootPhase != EFTShootPhase::Lobby)
	{
		ResetShoot();
	}
	G->ScriptBookUser = nullptr;
	BookUser.Reset();
	StartFilm(FilmId);
	return true;
}

void AFTSceneManager::RequestRestart(APlayerController* PC)
{
	AFTGameState* G = GS();
	if (!G || !PC)
	{
		return;
	}
	const bool bEnded = G->ShootPhase == EFTShootPhase::Results || G->ShootPhase == EFTShootPhase::Failed;
	if (!bEnded && !PC->IsLocalController())
	{
		if (AFTCharacter* C = Cast<AFTCharacter>(PC->GetPawn()))
		{
			C->ClientRejected(LOCTEXT("HostOnly", "Only the host can restart a running shoot"));
		}
		return;
	}
	ResetShoot();
	G->MulticastAnnounce(LOCTEXT("FreshNight", "Fresh night, fresh shoot! Head to the Director's Office."), EFTAnnounceStyle::Slate, EFTSound::Clapper);
}

void AFTSceneManager::StartFilm(FName FilmId)
{
	AFTGameState* G = GS();
	const UFTFilmDefinition* Film = UFTFilmDefinition::Find(FilmId);
	if (!G || !Film)
	{
		return;
	}
	G->FilmId = FilmId;
	G->ShootPhase = EFTShootPhase::Shooting;
	G->TakeResults.Reset();
	G->TeamScore = 0;
	G->StyleBonus = 0;
	G->DisasterBonus = 0;
	G->StudioCondition = 100.f;
	G->DawnServerTime = G->GetServerWorldTimeSeconds() + G->DawnDuration;
	G->MulticastAnnounce(FText::Format(LOCTEXT("Greenlit", "{0} is GREENLIT!  Swipe the keycard at the STAGE 4 door."), Film->Title), EFTAnnounceStyle::Slate, EFTSound::Clapper);
	EnterScene(0);
}

void AFTSceneManager::EnterScene(int32 Index)
{
	AFTGameState* G = GS();
	const UFTFilmDefinition* Film = G ? G->GetFilm() : nullptr;
	if (!Film || !Film->Scenes.IsValidIndex(Index))
	{
		return;
	}
	const FFTSceneDefinition& Scene = Film->Scenes[Index];
	G->SceneIndex = Index;
	G->TakeNumber = 1;
	G->Objectives.Reset();
	for (const FFTObjectiveDefinition& D : Scene.Objectives)
	{
		FFTObjectiveStatus S;
		S.ObjectiveId = D.ObjectiveId;
		S.State = IsSetupType(D.Type) ? EFTObjectiveState::Available : EFTObjectiveState::NotStarted;
		G->Objectives.Add(S);
	}
	G->RecordingTime = 0.f;
	G->CaptureProgress = 0.f;
	G->bKeyActionDone = false;
	G->LiveFrame = FFTFrameReport();
	AnnouncedObjectives.Reset();
	DefaultRigTimer = 0.f;
	SetSceneState(EFTSceneState::Preparation);
	if (Index > 0)
	{
		G->MulticastAnnounce(Scene.SlateText, EFTAnnounceStyle::Slate, EFTSound::Clapper);
	}
	EvaluateObjectives();
}

void AFTSceneManager::SetSceneState(EFTSceneState NewState)
{
	if (AFTGameState* G = GS())
	{
		G->SceneState = NewState;
		StateTime = 0.f;
		G->NotifyStateChanged();
	}
}

// ============================================================================ events

void AFTSceneManager::ReportEvent(FName Event, AActor* Source, AFTCharacter* EventInstigator)
{
	UWorld* World = GetWorld();
	AFTGameState* G = GS();
	if (!World || !G)
	{
		return;
	}
	const float Now = World->GetTimeSeconds();
	EventTimes.Add(Event, Now);
	const FFTSceneDefinition* Scene = G->GetCurrentScene();
	const bool bRecording = G->SceneState == EFTSceneState::Recording;

	// Translate raw actions into the scene's key actions.
	if (Scene && (Event == FTTags::EvRigLunge || Event == FTTags::EvCostumeLunge))
	{
		if (Scene->KeyActionEvent == FTTags::EvSharkAttackBoat)
		{
			FVector BoatLoc;
			float BoatR = 0.f;
			AActor* BoatActor = nullptr;
			const bool bBoat = FindSubject(World, FTTags::SubjBoat, BoatLoc, BoatR, BoatActor);
			const bool bBoatInTank = bBoat && AFTZone::IsInZone(this, FTTags::ZoneTank, BoatLoc);
			const FVector From = Source ? Source->GetActorLocation() : FVector::ZeroVector;
			const bool bNear = bBoatInTank && FVector::Dist2D(From, BoatLoc) < 900.f;
			const bool bAtMark = EventInstigator && AFTZone::IsInZone(this, FTTags::ZoneLungeMark, EventInstigator->GetActorLocation());
			if (bNear || (bAtMark && bBoatInTank))
			{
				EventTimes.Add(FTTags::EvSharkAttackBoat, Now);
				G->MulticastAnnounce(LOCTEXT("SharkAttack", "SHARK ATTACK!"), EFTAnnounceStyle::Danger, EFTSound::MonsterSting);
			}
		}
		else if (Scene->KeyActionEvent == FTTags::EvSharkDefeat && Event == FTTags::EvRigLunge)
		{
			bool bHero = false;
			for (TActorIterator<AFTCharacter> It(World); It; ++It)
			{
				if (It->HeldProp && It->HeldProp->PropTag == FTTags::PropHarpoon && AFTZone::IsInZone(this, FTTags::ZoneHeroMark, It->GetActorLocation()))
				{
					bHero = true;
					It->PlayEmote(EFTEmote::HeroPose);
				}
			}
			for (TActorIterator<AFTStandIn> It(World); It; ++It)
			{
				if (It->GetHeldItem() && It->GetHeldItem()->PropTag == FTTags::PropHarpoon && !It->GetCarrier() && AFTZone::IsInZone(this, FTTags::ZoneHeroMark, It->GetActingPoint()))
				{
					bHero = true;
				}
			}
			if (bHero)
			{
				EventTimes.Add(FTTags::EvSharkDefeat, Now);
				if (AFTStudioActor* SA = Cast<AFTStudioActor>(Source))
				{
					SA->OnSceneEvent(TEXT("Director.Defeat"), this);
				}
				G->MulticastAnnounce(LOCTEXT("Defeat", "THE HERO STRIKES!"), EFTAnnounceStyle::Success, EFTSound::MonsterSting);
			}
		}
	}

	if (Event == FTTags::EvSiren && bRecording)
	{
		EventTimes.Add(FTTags::EvFinGlide, Now);
	}

	// Let every studio actor react (fin glider, lights...).
	for (TActorIterator<AFTStudioActor> It(World); It; ++It)
	{
		if (*It != Source)
		{
			It->OnSceneEvent(Event, Source);
		}
	}

	if (Event == FTTags::EvKnockdown && bRecording)
	{
		bKnockedDuringTake = true;
	}
}

bool AFTSceneManager::WasEventDuringRecording(FName Event) const
{
	const float* T = EventTimes.Find(Event);
	return T && *T >= RecordStartTime && RecordStartTime > 0.f;
}

float AFTSceneManager::GetLastEventTime(FName Event) const
{
	const float* T = EventTimes.Find(Event);
	return T ? *T : -1000.f;
}

void AFTSceneManager::AdjustCondition(float Delta, const FText& Reason)
{
	AFTGameState* G = GS();
	if (!G || !G->IsShootActive())
	{
		return;
	}
	G->StudioCondition = FMath::Clamp(G->StudioCondition + Delta, 0.f, 100.f);
	if (Delta <= -3.f && !Reason.IsEmpty())
	{
		G->MulticastAnnounce(FText::Format(LOCTEXT("CondLoss", "{0}  (studio condition {1})"), Reason, FText::AsNumber(FMath::RoundToInt(Delta))), EFTAnnounceStyle::Info, EFTSound::None);
	}
}

// ============================================================================ world queries

AFTFilmCamera* AFTSceneManager::FindCamera() const
{
	for (TActorIterator<AFTFilmCamera> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

bool AFTSceneManager::IsCostumeInZone(EFTCostume Costume, FName Zone) const
{
	for (TActorIterator<AFTCharacter> It(GetWorld()); It; ++It)
	{
		if (It->Costume == Costume && AFTZone::IsInZone(this, Zone, It->GetActorLocation() - FVector(0.f, 0.f, 60.f)))
		{
			return true;
		}
	}
	for (TActorIterator<AFTStandIn> It(GetWorld()); It; ++It)
	{
		if (It->Costume == Costume && !It->GetCarrier() && AFTZone::IsInZone(this, Zone, It->GetActorLocation() + FVector(0.f, 0.f, 30.f)))
		{
			return true;
		}
	}
	return false;
}

bool AFTSceneManager::IsDeviceActive(FName DeviceTag) const
{
	for (TActorIterator<AFTStudioActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(DeviceTag) && It->IsDeviceActive())
		{
			return true;
		}
	}
	return false;
}

bool AFTSceneManager::FindSubject(const UWorld* World, FName SubjectTag, FVector& OutCenter, float& OutRadius, AActor*& OutActor)
{
	OutActor = nullptr;
	if (!World)
	{
		return false;
	}
	if (SubjectTag == FTTags::SubjLifeguard || SubjectTag == FTTags::SubjHero)
	{
		AFTCharacter* Best = nullptr;
		for (TActorIterator<AFTCharacter> It(World); It; ++It)
		{
			const bool bMatch = SubjectTag == FTTags::SubjLifeguard
				? It->Costume == EFTCostume::Lifeguard
				: (It->HeldProp && It->HeldProp->PropTag == FTTags::PropHarpoon);
			if (!bMatch)
			{
				continue;
			}
			const FName Zone = SubjectTag == FTTags::SubjLifeguard ? FTTags::ZoneBeach : FTTags::ZoneHeroMark;
			if (!Best || AFTZone::IsInZone(World, Zone, It->GetActorLocation() - FVector(0.f, 0.f, 60.f)))
			{
				Best = *It;
			}
		}
		if (Best)
		{
			OutActor = Best;
			OutCenter = Best->GetSubjectPoint();
			OutRadius = 95.f;
			return true;
		}
		for (TActorIterator<AFTStandIn> It(World); It; ++It)
		{
			const bool bMatch = SubjectTag == FTTags::SubjLifeguard
				? It->Costume == EFTCostume::Lifeguard
				: (It->GetHeldItem() && It->GetHeldItem()->PropTag == FTTags::PropHarpoon);
			if (bMatch && !It->GetCarrier())
			{
				OutActor = *It;
				OutCenter = It->GetActingPoint();
				OutRadius = 95.f;
				return true;
			}
		}
		return false;
	}
	if (SubjectTag == FTTags::SubjShark)
	{
		// Prefer the rig when it is up, otherwise a crew member in the shark suit, otherwise the rig's fin.
		AFTStudioActor* Rig = nullptr;
		for (TActorIterator<AFTStudioActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(FTTags::SubjShark))
			{
				Rig = *It;
				break;
			}
		}
		if (Rig && Rig->IsDeviceActive())
		{
			OutActor = Rig;
			return Rig->GetSubjectBounds(SubjectTag, OutCenter, OutRadius);
		}
		for (TActorIterator<AFTCharacter> It(World); It; ++It)
		{
			if (It->Costume == EFTCostume::Shark)
			{
				OutActor = *It;
				OutCenter = It->GetSubjectPoint();
				OutRadius = 100.f;
				return true;
			}
		}
		for (TActorIterator<AFTStandIn> It(World); It; ++It)
		{
			if (It->Costume == EFTCostume::Shark && !It->GetCarrier())
			{
				OutActor = *It;
				OutCenter = It->GetActingPoint();
				OutRadius = 100.f;
				return true;
			}
		}
		if (Rig)
		{
			OutActor = Rig;
			return Rig->GetSubjectBounds(SubjectTag, OutCenter, OutRadius);
		}
		return false;
	}
	for (TActorIterator<AFTStudioActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(SubjectTag))
		{
			if (SubjectTag == FTTags::SubjStorm && !It->IsDeviceActive())
			{
				continue;
			}
			OutActor = *It;
			return It->GetSubjectBounds(SubjectTag, OutCenter, OutRadius);
		}
	}
	return false;
}

FFTFrameReport AFTSceneManager::EvaluateFrame(const AFTFilmCamera* Camera, const FFTSceneDefinition& Scene) const
{
	FFTFrameReport Report;
	if (!Camera)
	{
		return Report;
	}
	const UWorld* World = GetWorld();
	const FTransform Lens = Camera->GetLensTransform();
	const FVector CamLoc = Lens.GetLocation();
	const float HalfH = FMath::Max(Camera->GetFOV() * 0.5f, 5.f);
	const float HalfV = FMath::RadiansToDegrees(FMath::Atan(FMath::Tan(FMath::DegreesToRadians(HalfH)) * 9.f / 16.f));

	// smoke in the shot
	float Smoke = 0.f;
	for (TActorIterator<AFTStudioActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(FTTags::DevSmoke) && It->IsDeviceActive())
		{
			const FVector ToSmoke = It->GetActorLocation() - CamLoc;
			const FVector Local = Lens.InverseTransformVectorNoScale(ToSmoke);
			if (Local.X > 0.f && FMath::Abs(FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X))) < HalfH + 15.f)
			{
				Smoke = FMath::Max(Smoke, 0.35f);
			}
		}
	}
	Report.SmokeObstruction = Smoke;

	auto ScoreSubject = [&](FName Tag) -> float
	{
		FVector Center;
		float Radius = 50.f;
		AActor* SubjectActor = nullptr;
		if (!FindSubject(World, Tag, Center, Radius, SubjectActor))
		{
			return 0.f;
		}
		const FVector Dir = Center - CamLoc;
		const float Dist = Dir.Size();
		const FVector Local = Lens.InverseTransformVectorNoScale(Dir);
		if (Local.X < 20.f)
		{
			return 0.f;
		}
		const float AngH = FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X));
		const float AngV = FMath::RadiansToDegrees(FMath::Atan2(Local.Z, Local.X));
		const float AngR = FMath::RadiansToDegrees(FMath::Atan2(Radius, Dist));
		const float NX = FMath::Abs(AngH) / HalfH;
		const float NY = FMath::Abs(AngV) / HalfV;
		// allow subjects that are partly inside the frame edge
		const float Slack = AngR / HalfV;
		if (NX > 1.f + Slack * 0.6f || NY > 1.f + Slack * 0.6f)
		{
			return 0.f;
		}
		const float Edge = FMath::Max(NX, NY);
		const float Centering = 1.f - FMath::Clamp((Edge - 0.55f) / 0.6f, 0.f, 1.f) * 0.55f;
		const float SizeRatio = AngR / HalfV;
		float SizeScore = 1.f;
		if (SizeRatio < 0.05f)
		{
			SizeScore = 0.45f;
		}
		else if (SizeRatio < 0.1f)
		{
			SizeScore = 0.75f;
		}
		else if (SizeRatio > 1.1f)
		{
			SizeScore = 0.6f;
		}
		float Occlusion = 1.f;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(FTFrame), false, Camera);
		if (SubjectActor)
		{
			Params.AddIgnoredActor(SubjectActor);
		}
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, CamLoc, Center, ECC_Visibility, Params) && Hit.Distance < Dist - Radius * 0.8f)
		{
			Occlusion = Cast<APawn>(Hit.GetActor()) ? 0.7f : 0.35f;
		}
		return FMath::Clamp(Centering * SizeScore * Occlusion * (1.f - Smoke), 0.f, 1.f);
	};

	float Sum = 0.f;
	bool bAllValid = Scene.RequiredSubjects.Num() > 0;
	for (FName Tag : Scene.RequiredSubjects)
	{
		const float S = ScoreSubject(Tag);
		Report.Subjects.Add(Tag);
		Report.SubjectScores.Add(S);
		Sum += S;
		bAllValid &= S >= 0.3f;
	}
	for (FName Tag : Scene.BonusProps)
	{
		const float S = ScoreSubject(Tag);
		Report.Subjects.Add(Tag);
		Report.SubjectScores.Add(S);
	}
	Report.Quality = Scene.RequiredSubjects.Num() > 0 ? Sum / Scene.RequiredSubjects.Num() : 0.f;
	Report.bCriticalValid = bAllValid;
	return Report;
}

// ============================================================================ objectives

bool AFTSceneManager::EvaluateObjective(const FFTObjectiveDefinition& Def, float& OutProgress) const
{
	const AFTGameState* G = GS();
	OutProgress = 0.f;
	switch (Def.Type)
	{
	case EFTObjectiveType::CostumeInZone:
		return IsCostumeInZone(CostumeFromName(Def.Param1), Def.Param2);
	case EFTObjectiveType::DeviceActive:
		return IsDeviceActive(Def.Param1);
	case EFTObjectiveType::PropInZone:
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (!It->ActorHasTag(Def.Param1))
			{
				continue;
			}
			FVector Where = It->GetActorLocation();
			if (const AFTStudioActor* SA = Cast<AFTStudioActor>(*It))
			{
				float R = 0.f;
				SA->GetSubjectBounds(Def.Param1, Where, R);
			}
			if (AFTZone::IsInZone(this, Def.Param2, Where))
			{
				return true;
			}
		}
		return false;
	case EFTObjectiveType::HeldPropInZone:
		for (TActorIterator<AFTCharacter> It(GetWorld()); It; ++It)
		{
			if (It->HeldProp && It->HeldProp->PropTag == Def.Param1 && AFTZone::IsInZone(this, Def.Param2, It->GetActorLocation() - FVector(0.f, 0.f, 60.f)))
			{
				return true;
			}
		}
		for (TActorIterator<AFTStandIn> It(GetWorld()); It; ++It)
		{
			if (It->GetHeldItem() && It->GetHeldItem()->PropTag == Def.Param1 && !It->GetCarrier() && AFTZone::IsInZone(this, Def.Param2, It->GetActorLocation() + FVector(0.f, 0.f, 30.f)))
			{
				return true;
			}
		}
		return false;
	case EFTObjectiveType::CameraOperated:
	{
		const AFTFilmCamera* Cam = FindCamera();
		return Cam && (Cam->GetOperator() != nullptr || Cam->IsRecording());
	}
	case EFTObjectiveType::SharkRigRaised:
		return IsDeviceActive(TEXT("Device.SharkRig"));
	case EFTObjectiveType::PowerRestored:
		return G && G->bStagePower;
	case EFTObjectiveType::KeyAction:
		return G && G->bKeyActionDone;
	case EFTObjectiveType::HoldShot:
		OutProgress = G ? G->CaptureProgress : 0.f;
		return OutProgress >= 1.f;
	case EFTObjectiveType::EventDuringRecording:
		return WasEventDuringRecording(Def.Param1);
	}
	return false;
}

bool AFTSceneManager::AreSetupObjectivesComplete() const
{
	const AFTGameState* G = GS();
	const FFTSceneDefinition* Scene = G ? G->GetCurrentScene() : nullptr;
	if (!Scene)
	{
		return false;
	}
	for (int32 i = 0; i < Scene->Objectives.Num(); ++i)
	{
		const FFTObjectiveDefinition& D = Scene->Objectives[i];
		if (D.bCritical && IsSetupType(D.Type) && G->Objectives.IsValidIndex(i) && G->Objectives[i].State != EFTObjectiveState::Complete)
		{
			return false;
		}
	}
	return true;
}

void AFTSceneManager::EvaluateObjectives()
{
	AFTGameState* G = GS();
	const FFTSceneDefinition* Scene = G ? G->GetCurrentScene() : nullptr;
	if (!Scene || G->ShootPhase != EFTShootPhase::Shooting)
	{
		return;
	}
	const bool bFrozen = G->SceneState == EFTSceneState::TakeComplete || G->SceneState == EFTSceneState::Transition;
	bool bChanged = false;
	for (int32 i = 0; i < Scene->Objectives.Num() && i < G->Objectives.Num(); ++i)
	{
		const FFTObjectiveDefinition& D = Scene->Objectives[i];
		FFTObjectiveStatus& S = G->Objectives[i];
		if (bFrozen)
		{
			continue;
		}
		float Progress = 0.f;
		const bool bDone = EvaluateObjective(D, Progress);
		EFTObjectiveState NewState = S.State;
		if (IsSetupType(D.Type))
		{
			NewState = bDone ? EFTObjectiveState::Complete : EFTObjectiveState::Available;
		}
		else if (D.Type == EFTObjectiveType::EventDuringRecording)
		{
			if (bDone)
			{
				NewState = EFTObjectiveState::Complete;
			}
			else
			{
				NewState = G->SceneState == EFTSceneState::Recording ? EFTObjectiveState::InProgress : EFTObjectiveState::Available;
			}
		}
		else
		{
			if (G->SceneState == EFTSceneState::Recording)
			{
				NewState = bDone ? EFTObjectiveState::Complete : EFTObjectiveState::InProgress;
			}
			else
			{
				NewState = G->SceneState == EFTSceneState::ReadyToRoll ? EFTObjectiveState::Available : EFTObjectiveState::NotStarted;
			}
		}
		if (NewState != S.State || !FMath::IsNearlyEqual(Progress, S.Progress, 0.02f))
		{
			if (NewState == EFTObjectiveState::Complete && S.State != EFTObjectiveState::Complete && IsSetupType(D.Type))
			{
				G->MulticastSound2D(EFTSound::Stamp, 0.6f);
			}
			S.State = NewState;
			S.Progress = Progress;
			bChanged = true;
		}
	}

	if (G->SceneState == EFTSceneState::Preparation && AreSetupObjectivesComplete())
	{
		SetSceneState(EFTSceneState::ReadyToRoll);
		G->MulticastAnnounce(LOCTEXT("Ready", "READY TO ROLL!  Camera: press LMB to record"), EFTAnnounceStyle::Success, EFTSound::UIConfirm);
		bChanged = false;
	}
	else if (G->SceneState == EFTSceneState::ReadyToRoll && !AreSetupObjectivesComplete())
	{
		SetSceneState(EFTSceneState::Preparation);
		bChanged = false;
	}
	if (bChanged)
	{
		G->NotifyStateChanged();
	}
}

void AFTSceneManager::UpdateCallout()
{
	AFTGameState* G = GS();
	if (!G)
	{
		return;
	}
	FText NewCallout;
	const FFTSceneDefinition* Scene = G->GetCurrentScene();
	switch (G->ShootPhase)
	{
	case EFTShootPhase::Lobby:
		NewCallout = LOCTEXT("CLobby", "Follow the coral arrows to the Director's Office and hold E on the script book.");
		break;
	case EFTShootPhase::Shooting:
		if (!Scene)
		{
			break;
		}
		switch (G->SceneState)
		{
		case EFTSceneState::Preparation:
			for (int32 i = 0; i < Scene->Objectives.Num() && i < G->Objectives.Num(); ++i)
			{
				const FFTObjectiveDefinition& D = Scene->Objectives[i];
				if (D.bCritical && IsSetupType(D.Type) && G->Objectives[i].State != EFTObjectiveState::Complete)
				{
					NewCallout = D.Callout.IsEmpty() ? D.Text : D.Callout;
					break;
				}
			}
			break;
		case EFTSceneState::ReadyToRoll:
			NewCallout = LOCTEXT("CReady", "Ready to roll! Camera operator: press LMB to start recording.");
			break;
		case EFTSceneState::Recording:
			if (!G->bKeyActionDone)
			{
				for (const FFTObjectiveDefinition& D : Scene->Objectives)
				{
					if (D.Type == EFTObjectiveType::KeyAction)
					{
						NewCallout = D.Callout.IsEmpty() ? D.Text : D.Callout;
					}
				}
			}
			else
			{
				NewCallout = LOCTEXT("CHold", "Hold the shot! Keep everyone in frame.");
			}
			break;
		case EFTSceneState::TakeComplete:
			NewCallout = LOCTEXT("CTake", "That's a take!");
			break;
		case EFTSceneState::Transition:
			NewCallout = Scene->TransitionText;
			break;
		}
		break;
	case EFTShootPhase::Finale:
		if (!G->bProjectionUnlocked)
		{
			NewCallout = LOCTEXT("CFinaleLocked", "Wait for the projection room to unlock...");
		}
		else if (G->ReelsLoaded < G->GetCompletedTakeCount())
		{
			NewCallout = FText::Format(LOCTEXT("CReels", "Open the projection room! Carry the reels from the reel tray up the catwalk stairs ({0}/{1} loaded)."),
				FText::AsNumber(G->ReelsLoaded), FText::AsNumber(G->GetCompletedTakeCount()));
		}
		else if (!G->bProjectorPower)
		{
			NewCallout = LOCTEXT("CPower", "Restore projector power at the panel in the projection room.");
		}
		else
		{
			NewCallout = LOCTEXT("CStart", "Hold E on the projector to start the premiere!");
		}
		break;
	case EFTShootPhase::Premiere:
		NewCallout = LOCTEXT("CPremiere", "Roll the premiere! Enjoy the show.");
		break;
	case EFTShootPhase::Results:
	case EFTShootPhase::Failed:
		NewCallout = LOCTEXT("CResults", "Press Retry Shoot to go again, or pick a new script in the office.");
		break;
	default:
		break;
	}
	if (!NewCallout.EqualTo(G->Callout))
	{
		G->Callout = NewCallout;
		G->NotifyStateChanged();
	}
}

// ============================================================================ recording & scoring

void AFTSceneManager::RequestRecordToggle(AFTFilmCamera* Camera, AFTCharacter* User)
{
	AFTGameState* G = GS();
	if (!G || !Camera)
	{
		return;
	}
	if (Camera->IsRecording())
	{
		FinishTake(true);
		return;
	}
	auto Reject = [User](const FText& Why)
	{
		if (User)
		{
			User->ClientRejected(Why);
		}
	};
	if (G->ShootPhase != EFTShootPhase::Shooting)
	{
		Reject(LOCTEXT("NoScene", "No scene to shoot right now"));
		return;
	}
	if (G->SceneState == EFTSceneState::Preparation)
	{
		const FFTSceneDefinition* Scene = G->GetCurrentScene();
		FText Missing = LOCTEXT("Setup", "finish the setup first");
		if (Scene)
		{
			for (int32 i = 0; i < Scene->Objectives.Num() && i < G->Objectives.Num(); ++i)
			{
				if (Scene->Objectives[i].bCritical && IsSetupType(Scene->Objectives[i].Type) && G->Objectives[i].State != EFTObjectiveState::Complete)
				{
					Missing = Scene->Objectives[i].Text;
					break;
				}
			}
		}
		Reject(FText::Format(LOCTEXT("NotReady", "Not ready to roll: {0}"), Missing));
		return;
	}
	if (G->SceneState != EFTSceneState::ReadyToRoll)
	{
		Reject(LOCTEXT("Busy", "Hang on - the crew is resetting the set"));
		return;
	}
	Camera->SetRecording(true);
	RecordStartTime = GetWorld()->GetTimeSeconds();
	FrameAccum = 0.f;
	FrameSamples = 0;
	SubjectBest.Reset();
	bKnockedDuringTake = false;
	G->RecordingTime = 0.f;
	G->CaptureProgress = 0.f;
	G->bKeyActionDone = false;
	SetSceneState(EFTSceneState::Recording);
	const FFTSceneDefinition* Scene = G->GetCurrentScene();
	G->MulticastAnnounce(FText::Format(LOCTEXT("Rolling", "ROLLING!  {0}  -  TAKE {1}"), Scene ? Scene->SlateText : FText::GetEmpty(), FText::AsNumber(G->TakeNumber)), EFTAnnounceStyle::Slate, EFTSound::Clapper);
}

void AFTSceneManager::TickRecording(float DeltaSeconds)
{
	AFTGameState* G = GS();
	const FFTSceneDefinition* Scene = G ? G->GetCurrentScene() : nullptr;
	AFTFilmCamera* Cam = FindCamera();
	if (!Scene || !Cam || !Cam->IsRecording())
	{
		FinishTake(true);
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	G->RecordingTime = Now - RecordStartTime;

	const FFTFrameReport Frame = EvaluateFrame(Cam, *Scene);
	G->LiveFrame = Frame;
	FrameAccum += Frame.Quality;
	++FrameSamples;
	for (int32 i = 0; i < Frame.Subjects.Num(); ++i)
	{
		float& Best = SubjectBest.FindOrAdd(Frame.Subjects[i]);
		Best = FMath::Max(Best, Frame.SubjectScores[i]);
	}

	G->bKeyActionDone = WasEventDuringRecording(Scene->KeyActionEvent);
	if (G->bKeyActionDone && Frame.bCriticalValid)
	{
		G->CaptureProgress = FMath::Min(1.f, G->CaptureProgress + DeltaSeconds / FMath::Max(Scene->CaptureDuration, 0.5f));
	}

	// Safe default: if nobody drives the shark rig during the finale scene, the rig lunges on its own.
	if (Scene->KeyActionEvent == FTTags::EvSharkDefeat && !G->bKeyActionDone && G->RecordingTime > 12.f && G->RecordingTime - DefaultRigTimer > 12.f)
	{
		DefaultRigTimer = G->RecordingTime;
		for (TActorIterator<AFTStudioActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(TEXT("Device.SharkRig")))
			{
				It->OnSceneEvent(TEXT("Director.AutoLunge"), this);
			}
		}
	}

	if (G->CaptureProgress >= 1.f)
	{
		FinishTake(false);
	}
	else if (G->RecordingTime > Scene->CompletionWindow)
	{
		G->MulticastAnnounce(LOCTEXT("Cut", "CUT! We ran out of film on that one."), EFTAnnounceStyle::Info, EFTSound::Clapper);
		FinishTake(false);
	}
}

FFTTakeResult AFTSceneManager::ScoreTake() const
{
	const AFTGameState* G = GS();
	const FFTSceneDefinition* Scene = G->GetCurrentScene();
	FFTTakeResult R;
	R.FilmId = G->FilmId;
	R.SceneId = Scene ? Scene->SceneId : NAME_None;
	R.SceneIndex = G->SceneIndex;
	R.TakeNumber = G->TakeNumber;
	R.Timestamp = G->GetServerWorldTimeSeconds();
	if (!Scene)
	{
		return R;
	}

	// 1) main action (40)
	if (G->bKeyActionDone)
	{
		const float Q = FrameSamples > 0 ? FrameAccum / FrameSamples : 0.f;
		R.ActionPoints = Q > 0.25f ? 40 : 30;
		R.Reasons.Add(FString::Printf(TEXT("+%d  Key action while rolling"), R.ActionPoints));
	}
	else
	{
		R.Reasons.Add(TEXT("+0  The key action never happened"));
	}

	// 2) required subjects (25)
	float SubjSum = 0.f;
	for (FName Tag : Scene->RequiredSubjects)
	{
		const float* B = SubjectBest.Find(Tag);
		const float V = B ? *B : 0.f;
		SubjSum += V;
		const FString Name = Tag.ToString().RightChop(8);
		if (V >= 0.35f)
		{
			R.VisibleSubjects.Add(Tag);
		}
		else
		{
			R.Reasons.Add(FString::Printf(TEXT("-  %s barely in frame"), *Name));
		}
	}
	const float SubjAvg = Scene->RequiredSubjects.Num() > 0 ? SubjSum / Scene->RequiredSubjects.Num() : 0.f;
	R.SubjectPoints = FMath::Clamp(FMath::RoundToInt(25.f * SubjAvg), 0, 25);
	R.Reasons.Add(FString::Printf(TEXT("+%d  Framing (%d%%)"), R.SubjectPoints, FMath::RoundToInt(SubjAvg * 100.f)));

	// 3) light & sound cues (15)
	int32 CueTotal = Scene->CueEvents.Num() + Scene->CueDevices.Num();
	int32 CueHit = 0;
	for (FName E : Scene->CueEvents)
	{
		CueHit += WasEventDuringRecording(E) ? 1 : 0;
	}
	for (FName D : Scene->CueDevices)
	{
		if (IsDeviceActive(D))
		{
			++CueHit;
			R.ActiveEffects.Add(D);
		}
	}
	R.CuePoints = CueTotal > 0 ? FMath::Clamp(FMath::RoundToInt(15.f * CueHit / CueTotal), 0, 15) : 15;
	R.Reasons.Add(FString::Printf(TEXT("+%d  Light & sound cues (%d/%d)"), R.CuePoints, CueHit, CueTotal));

	// 4) props & set (10)
	int32 PropsSeen = 0;
	for (FName P : Scene->BonusProps)
	{
		const float* B = SubjectBest.Find(P);
		if (B && *B >= 0.3f)
		{
			++PropsSeen;
			R.VisibleSubjects.AddUnique(P);
		}
	}
	R.PropPoints = FMath::Min(10, 4 + PropsSeen * 3);
	R.Reasons.Add(FString::Printf(TEXT("+%d  Props & set dressing"), R.PropPoints));

	// 5) style & mishaps (10)
	int32 Style = 0;
	if (WasEventDuringRecording(FTTags::EvFoam)) { Style += 4; R.Reasons.Add(TEXT("+4  Foam chaos!")); }
	if (bKnockedDuringTake) { Style += 5; R.Reasons.Add(TEXT("+5  Real stunt (someone fell over)")); }
	if (WasEventDuringRecording(FTTags::EvEmoteCheer) || WasEventDuringRecording(FTTags::EvEmotePanic)) { Style += 3; R.Reasons.Add(TEXT("+3  Big performance")); }
	if (WasEventDuringRecording(FTTags::EvEmotePoint)) { Style += 2; R.Reasons.Add(TEXT("+2  Dramatic pointing")); }
	if (WasEventDuringRecording(FTTags::EvHeroThrust)) { Style += 3; R.Reasons.Add(TEXT("+3  Heroic harpoon thrust")); }
	if (WasEventDuringRecording(FTTags::EvCostumeLunge)) { Style += 3; R.Reasons.Add(TEXT("+3  Shark-suit acting")); }
	if (IsDeviceActive(FTTags::DevSmoke)) { Style += 2; R.Reasons.Add(TEXT("+2  Atmospheric smoke")); }
	R.StylePoints = FMath::Clamp(Style, 0, 10);

	if (G->FloodStage != EFTFloodStage::Dry) { R.DisasterFlags |= 1; }
	if (!G->bStagePower) { R.DisasterFlags |= 2; }
	if (bKnockedDuringTake) { R.DisasterFlags |= 4; }

	R.Score = FMath::Clamp(R.ActionPoints + R.SubjectPoints + R.CuePoints + R.PropPoints + R.StylePoints, 0, 100);
	R.bAccepted = G->bKeyActionDone && R.Score >= 40 && (G->CaptureProgress >= 0.99f || R.Score >= 55);
	if (!R.bAccepted)
	{
		R.Rating = EFTTakeRating::Retake;
	}
	else if (R.Score >= 85)
	{
		R.Rating = EFTTakeRating::Perfect;
	}
	else if (R.Score >= 65)
	{
		R.Rating = EFTTakeRating::Great;
	}
	else
	{
		R.Rating = EFTTakeRating::Usable;
	}
	return R;
}

void AFTSceneManager::FinishTake(bool bManualStop)
{
	AFTGameState* G = GS();
	if (!G || G->SceneState != EFTSceneState::Recording)
	{
		return;
	}
	if (AFTFilmCamera* Cam = FindCamera())
	{
		Cam->SetRecording(false);
	}
	FFTTakeResult R = ScoreTake();
	G->TakeResults.Add(R);
	G->MulticastTakeResult(R);
	if (R.bAccepted)
	{
		G->TeamScore += R.Score;
		G->StyleBonus += R.StylePoints;
		for (FFTObjectiveStatus& S : G->Objectives)
		{
			if (S.State != EFTObjectiveState::Complete)
			{
				S.State = S.State == EFTObjectiveState::NotStarted || S.State == EFTObjectiveState::InProgress ? EFTObjectiveState::Complete : EFTObjectiveState::FailedOptional;
			}
		}
		// optional bonus objectives that did not happen are marked as failed (never blocking)
		if (const FFTSceneDefinition* Scene = G->GetCurrentScene())
		{
			for (int32 i = 0; i < Scene->Objectives.Num() && i < G->Objectives.Num(); ++i)
			{
				if (!Scene->Objectives[i].bCritical)
				{
					float P = 0.f;
					G->Objectives[i].State = EvaluateObjective(Scene->Objectives[i], P) ? EFTObjectiveState::Complete : EFTObjectiveState::FailedOptional;
				}
			}
		}
		SetSceneState(EFTSceneState::TakeComplete);
		SpawnReel(G->SceneIndex);
		G->MulticastAnnounce(FText::Format(LOCTEXT("TakeOk", "THAT'S A TAKE!  {0}  -  {1} pts"), RatingText(R.Rating), FText::AsNumber(R.Score)), EFTAnnounceStyle::Success, EFTSound::ApplauseSmall);
	}
	else
	{
		G->TakeNumber++;
		G->CaptureProgress = 0.f;
		G->bKeyActionDone = false;
		SetSceneState(EFTSceneState::Preparation);
		const FText Why = !R.bAccepted && R.ActionPoints == 0
			? LOCTEXT("RetakeAction", "RETAKE!  The key action never happened while rolling.")
			: FText::Format(LOCTEXT("RetakeScore", "RETAKE!  Only {0} pts - hold the shot longer."), FText::AsNumber(R.Score));
		G->MulticastAnnounce(Why, EFTAnnounceStyle::Danger, EFTSound::Groan);
		EvaluateObjectives();
	}
	G->NotifyStateChanged();
}

void AFTSceneManager::SpawnReel(int32 SceneIdx)
{
	FTransform Where(FRotator::ZeroRotator, FVector(400.f, -900.f, -60.f));
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("Spawn.ReelTray")))
		{
			Where = It->GetActorTransform();
			break;
		}
	}
	Where.AddToTranslation(Where.GetRotation().RotateVector(FVector(0.f, (SceneIdx - 1) * 55.f, 46.f)));
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AFTProp_Reel* Reel = GetWorld()->SpawnActor<AFTProp_Reel>(AFTProp_Reel::StaticClass(), Where, Params))
	{
		Reel->SetReelScene(SceneIdx);
	}
}

// ============================================================================ disaster, finale, clock

void AFTSceneManager::ApplyDisaster(EFTSceneDisaster Disaster)
{
	AFTGameState* G = GS();
	if (!G)
	{
		return;
	}
	if (Disaster == EFTSceneDisaster::FloodAfterTake && G->FloodStage == EFTFloodStage::Dry)
	{
		G->MulticastAnnounce(LOCTEXT("ValveWarn", "WARNING: the tank valve is failing!  Get to higher ground!"), EFTAnnounceStyle::Danger, EFTSound::Alarm);
		LeakWarningTime = GetWorld()->GetTimeSeconds();
		G->DisasterBonus += 15;
	}
	else if (Disaster == EFTSceneDisaster::ProjectionUnlockAfterTake)
	{
		G->bProjectionUnlocked = true;
		G->MulticastAnnounce(LOCTEXT("ProjOpen", "The PROJECTION ROOM is open!  Get the reels up there!"), EFTAnnounceStyle::Success, EFTSound::Fanfare);
		G->NotifyStateChanged();
	}
}

void AFTSceneManager::TickFlood(float DeltaSeconds)
{
	AFTGameState* G = GS();
	AFTFloodController* Flood = AFTFloodController::Get(this);
	if (!G || !Flood)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	// Fail-safe: if scene 2 drags on, the valve gives out anyway.
	if (G->FloodStage == EFTFloodStage::Dry && LeakWarningTime < 0.f && G->ShootPhase == EFTShootPhase::Shooting
		&& G->SceneIndex >= 1 && G->GetDawnRemaining() < G->DawnDuration * 0.3f)
	{
		ApplyDisaster(EFTSceneDisaster::FloodAfterTake);
	}
	if (LeakWarningTime > 0.f && G->FloodStage == EFTFloodStage::Dry && Now - LeakWarningTime > 4.f)
	{
		Flood->SetStage(EFTFloodStage::Leaking);
		G->bStagePower = false;
		G->NotifyPowerChanged();
		G->MulticastAnnounce(LOCTEXT("PowerOut", "The valve burst!  Stage power is out - emergency lights on."), EFTAnnounceStyle::Danger, EFTSound::PowerDown);
		FloodTimer = 0.f;
	}
	if (G->FloodStage == EFTFloodStage::Leaking)
	{
		FloodTimer += DeltaSeconds;
		if (FloodTimer > Flood->LeakRiseTime + 4.f)
		{
			Flood->SetStage(EFTFloodStage::Flooded);
			G->MulticastAnnounce(LOCTEXT("Flooded", "The soundstage is FLOODED - and something is swimming in it!"), EFTAnnounceStyle::Danger, EFTSound::MonsterSting);
		}
	}
}

void AFTSceneManager::OnBreakerRestored()
{
	AFTGameState* G = GS();
	if (G && !G->bStagePower)
	{
		G->bStagePower = true;
		G->NotifyPowerChanged();
		AdjustCondition(8.f, FText::GetEmpty());
		G->MulticastAnnounce(LOCTEXT("PowerBack", "Stage power restored!  Lighting board is live again."), EFTAnnounceStyle::Success, EFTSound::PowerUp);
	}
}

void AFTSceneManager::OnReelLoaded(int32 ReelSceneIndex)
{
	AFTGameState* G = GS();
	if (!G)
	{
		return;
	}
	G->ReelsLoaded++;
	G->MulticastAnnounce(FText::Format(LOCTEXT("ReelLoaded", "Reel {0} loaded into the projector ({1}/{2})"), FText::AsNumber(ReelSceneIndex + 1),
		FText::AsNumber(G->ReelsLoaded), FText::AsNumber(G->GetCompletedTakeCount())), EFTAnnounceStyle::Info, EFTSound::Stamp);
	G->NotifyStateChanged();
}

void AFTSceneManager::OnProjectorPowered()
{
	AFTGameState* G = GS();
	if (G && !G->bProjectorPower)
	{
		G->bProjectorPower = true;
		G->MulticastAnnounce(LOCTEXT("ProjPower", "Projector power ON."), EFTAnnounceStyle::Info, EFTSound::PowerUp);
		G->NotifyStateChanged();
	}
}

bool AFTSceneManager::RequestStartPremiere(AFTCharacter* User, FText& OutReason)
{
	AFTGameState* G = GS();
	if (!G)
	{
		return false;
	}
	if (G->ShootPhase != EFTShootPhase::Finale)
	{
		OutReason = LOCTEXT("NotYet", "Finish shooting the film first");
		return false;
	}
	if (G->ReelsLoaded < G->GetCompletedTakeCount())
	{
		OutReason = FText::Format(LOCTEXT("NeedReels", "Load all reels first ({0}/{1})"), FText::AsNumber(G->ReelsLoaded), FText::AsNumber(G->GetCompletedTakeCount()));
		return false;
	}
	if (!G->bProjectorPower)
	{
		OutReason = LOCTEXT("NeedPower", "The projector has no power - use the power panel");
		return false;
	}
	G->ShootPhase = EFTShootPhase::Premiere;
	G->PremiereStartTime = G->GetServerWorldTimeSeconds();
	G->FrozenRemaining = G->GetDawnRemaining();
	G->DawnServerTime = 0.f;
	PremiereTimer = 0.f;
	G->MulticastAnnounce(LOCTEXT("Premiere", "LIGHTS DOWN - IT'S PREMIERE TIME!"), EFTAnnounceStyle::Slate, EFTSound::Fanfare);
	G->NotifyStateChanged();
	return true;
}

void AFTSceneManager::TickFinale(float DeltaSeconds)
{
	AFTGameState* G = GS();
	if (!G)
	{
		return;
	}
	if (G->ShootPhase == EFTShootPhase::Premiere)
	{
		PremiereTimer += DeltaSeconds;
		const float Length = 8.f + 6.f * FMath::Max(1, G->GetCompletedTakeCount()) + 8.f;
		if (PremiereTimer > Length)
		{
			G->ShootPhase = EFTShootPhase::Results;
			G->MulticastAnnounce(LOCTEXT("Results", "THE REVIEWS ARE IN!"), EFTAnnounceStyle::Slate, EFTSound::ApplauseBig);
			G->NotifyStateChanged();
		}
	}
}

void AFTSceneManager::Fail(const FText& Reason)
{
	AFTGameState* G = GS();
	if (!G || G->ShootPhase == EFTShootPhase::Failed)
	{
		return;
	}
	if (AFTFilmCamera* Cam = FindCamera())
	{
		Cam->Release();
	}
	G->FrozenRemaining = G->GetDawnRemaining();
	G->DawnServerTime = 0.f;
	G->ShootPhase = EFTShootPhase::Failed;
	G->FailReason = Reason;
	G->bStagePower = false;
	G->NotifyPowerChanged();
	G->MulticastAnnounce(Reason, EFTAnnounceStyle::Danger, EFTSound::PowerDown);
	G->NotifyStateChanged();
}

void AFTSceneManager::TickClockAndCondition(float DeltaSeconds)
{
	AFTGameState* G = GS();
	if (!G || !G->IsShootActive())
	{
		return;
	}
	if (G->DawnServerTime > 0.f && G->GetDawnRemaining() <= 0.f)
	{
		Fail(LOCTEXT("Dawn", "DAWN BROKE - the premiere never started!"));
		return;
	}
	if (G->FloodStage != EFTFloodStage::Dry)
	{
		const float Drain = (G->FloodStage == EFTFloodStage::Flooded ? 0.07f : 0.04f) + (G->bStagePower ? 0.f : 0.02f);
		G->StudioCondition = FMath::Max(0.f, G->StudioCondition - Drain * DeltaSeconds);
	}
	if (G->StudioCondition <= 0.f)
	{
		Fail(LOCTEXT("Shutdown", "TOTAL SHUTDOWN - the studio went dark!"));
	}
}

void AFTSceneManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AFTGameState* G = GS();
	if (!G || !HasAuthority())
	{
		return;
	}
	StateTime += DeltaSeconds;
	TickClockAndCondition(DeltaSeconds);

	EvalTimer += DeltaSeconds;
	if (EvalTimer >= 0.1f)
	{
		EvalTimer = 0.f;
		EvaluateObjectives();
		CalloutTimer += 0.1f;
		if (CalloutTimer >= 0.3f)
		{
			CalloutTimer = 0.f;
			UpdateCallout();
		}
	}

	if (G->ShootPhase == EFTShootPhase::Shooting)
	{
		switch (G->SceneState)
		{
		case EFTSceneState::Recording:
			TickRecording(DeltaSeconds);
			break;
		case EFTSceneState::TakeComplete:
			if (StateTime > 4.f)
			{
				if (AFTFilmCamera* Cam = FindCamera())
				{
					Cam->Release(LOCTEXT("SceneChange", "Camera released while the crew resets the set."));
				}
				if (const FFTSceneDefinition* Scene = G->GetCurrentScene())
				{
					ApplyDisaster(Scene->DisasterAfterTake);
					G->MulticastAnnounce(Scene->TransitionText, EFTAnnounceStyle::Info, EFTSound::None);
				}
				SetSceneState(EFTSceneState::Transition);
			}
			break;
		case EFTSceneState::Transition:
			if (StateTime > 6.f)
			{
				const UFTFilmDefinition* Film = G->GetFilm();
				if (Film && Film->Scenes.IsValidIndex(G->SceneIndex + 1))
				{
					EnterScene(G->SceneIndex + 1);
				}
				else
				{
					G->ShootPhase = EFTShootPhase::Finale;
					if (!G->bProjectionUnlocked)
					{
						ApplyDisaster(EFTSceneDisaster::ProjectionUnlockAfterTake);
					}
					G->NotifyStateChanged();
				}
			}
			break;
		default:
			break;
		}
	}
	TickFlood(DeltaSeconds);
	TickFinale(DeltaSeconds);
}

// ============================================================================ reset

void AFTSceneManager::ResetShoot()
{
	AFTGameState* G = GS();
	UWorld* World = GetWorld();
	if (!G || !World)
	{
		return;
	}
	// characters first (drop props, leave stations, change back into work clothes)
	TArray<APlayerStart*> Starts;
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		Starts.Add(*It);
	}
	int32 StartIdx = 0;
	for (TActorIterator<AFTCharacter> It(World); It; ++It)
	{
		AFTCharacter* C = *It;
		C->StopUsing();
		C->ReleaseProp(false);
		C->EquipCostume(EFTCostume::None);
		if (Starts.Num() > 0)
		{
			const APlayerStart* S = Starts[StartIdx++ % Starts.Num()];
			C->TeleportTo(S->GetActorLocation(), S->GetActorRotation(), false, true);
			if (AController* Ctl = C->GetController())
			{
				Ctl->SetControlRotation(S->GetActorRotation());
			}
		}
	}
	TArray<AFTStudioActor*> Actors;
	for (TActorIterator<AFTStudioActor> It(World); It; ++It)
	{
		Actors.Add(*It);
	}
	for (AFTStudioActor* A : Actors)
	{
		if (IsValid(A))
		{
			A->ResetForNewShoot();
		}
	}

	EventTimes.Reset();
	RecordStartTime = 0.f;
	LeakWarningTime = -1.f;
	FloodTimer = 0.f;
	SubjectBest.Reset();
	G->FilmId = NAME_None;
	G->ShootPhase = EFTShootPhase::Lobby;
	G->SceneIndex = 0;
	G->SceneState = EFTSceneState::Preparation;
	G->Objectives.Reset();
	G->TakeNumber = 1;
	G->DawnServerTime = 0.f;
	G->FrozenRemaining = G->DawnDuration;
	G->StudioCondition = 100.f;
	G->TeamScore = 0;
	G->StyleBonus = 0;
	G->DisasterBonus = 0;
	G->RecordingTime = 0.f;
	G->CaptureProgress = 0.f;
	G->bKeyActionDone = false;
	G->LiveFrame = FFTFrameReport();
	G->TakeResults.Reset();
	G->bProjectionUnlocked = false;
	G->bProjectorPower = false;
	G->ReelsLoaded = 0;
	G->PremiereStartTime = 0.f;
	G->FailReason = FText::GetEmpty();
	G->ScriptBookUser = nullptr;
	G->bStagePower = true;
	G->NotifyPowerChanged();
	if (G->FloodStage != EFTFloodStage::Dry)
	{
		if (AFTFloodController* Flood = AFTFloodController::Get(this))
		{
			Flood->SetStage(EFTFloodStage::Dry);
		}
	}
	G->NotifyStateChanged();
}

#undef LOCTEXT_NAMESPACE
