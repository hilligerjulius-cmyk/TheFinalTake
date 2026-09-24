#include "TheFinalTake/Data/FTFilmDefinition.h"

#include "TheFinalTake/Core/FTVisuals.h"

#define LOCTEXT_NAMESPACE "FinalTakeFilms"

namespace
{
	FFTObjectiveDefinition Obj(const TCHAR* Id, const FText& Text, EFTObjectiveType Type, bool bCritical,
		FName P1 = NAME_None, FName P2 = NAME_None, const FText& Callout = FText::GetEmpty())
	{
		FFTObjectiveDefinition O;
		O.ObjectiveId = Id;
		O.Text = Text;
		O.Type = Type;
		O.bCritical = bCritical;
		O.Param1 = P1;
		O.Param2 = P2;
		O.Callout = Callout;
		return O;
	}
}

const UFTFilmDefinition* UFTFilmDefinition::Find(FName FilmId)
{
	if (FilmId.IsNone())
	{
		return nullptr;
	}
	// Film assets are looked up every frame by the HUD and held by widgets: load once and keep them resident
	// (a GC'd asset left widgets with dangling pointers and caused synchronous reload hitches).
	static TMap<FName, TWeakObjectPtr<UFTFilmDefinition>> Cache;
	if (const TWeakObjectPtr<UFTFilmDefinition>* Hit = Cache.Find(FilmId))
	{
		if (Hit->IsValid())
		{
			return Hit->Get();
		}
	}
	const FString AssetPath = FString::Printf(TEXT("/Game/TheFinalTake/Data/DA_Film_%s.DA_Film_%s"), *FilmId.ToString(), *FilmId.ToString());
	if (UFTFilmDefinition* Asset = LoadObject<UFTFilmDefinition>(nullptr, *AssetPath, nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		if (!IsRunningCommandlet() && !Asset->IsRooted())
		{
			Asset->AddToRoot();
		}
		Cache.Add(FilmId, Asset);
		return Asset;
	}
	if (FilmId == FTTags::FilmJaws) return GetDefault<UFTFilm_JawsOfTheStudio>();
	if (FilmId == FTTags::FilmMoonfall) return GetDefault<UFTFilm_MoonfallMotel>();
	if (FilmId == FTTags::FilmCastle) return GetDefault<UFTFilm_CastleOnFire>();
	return nullptr;
}

TArray<const UFTFilmDefinition*> UFTFilmDefinition::All()
{
	TArray<const UFTFilmDefinition*> Films;
	for (FName Id : { FTTags::FilmJaws, FTTags::FilmMoonfall, FTTags::FilmCastle })
	{
		if (const UFTFilmDefinition* F = Find(Id))
		{
			Films.Add(F);
		}
	}
	return Films;
}

UFTFilm_JawsOfTheStudio::UFTFilm_JawsOfTheStudio()
{
	FilmId = FTTags::FilmJaws;
	Title = LOCTEXT("JawsTitle", "JAWS OF THE STUDIO");
	Genre = LOCTEXT("JawsGenre", "1980s giant-shark disaster movie");
	Logline = LOCTEXT("JawsLogline", "A lone lifeguard, a leaky rescue boat and one very rubbery shark. Summer on Studio Beach is about to get a whole lot bitey.");
	Runtime = LOCTEXT("JawsRuntime", "3 scenes  -  approx. 20 min shoot");
	SignatureHazard = LOCTEXT("JawsHazard", "Tank valve failure: the soundstage floods and something swims in it.");
	SignatureEffect = LOCTEXT("JawsEffect", "Wind + rain machines and a mechanical shark rig");
	Difficulty = 2;
	bPlayable = true;
	PosterPrimary = FTColors::Blue;
	PosterSecondary = FTColors::Coral;
	PosterTagline = LOCTEXT("JawsTagline", "YOU'LL NEED A BIGGER TANK.");
	StageId = TEXT("Stage4");
	StageLabel = LOCTEXT("JawsStage", "STAGE 4");
	AudienceMultiplier = 1.f;

	{
		FFTSceneDefinition S;
		S.SceneId = TEXT("S1_Arrival");
		S.Title = LOCTEXT("S1Title", "Scene 1: The Arrival");
		S.ScriptDescription = LOCTEXT("S1Desc", "Dusk on Studio Beach. The lifeguard stands watch below the lighthouse. A siren wails - and a fin cuts through the water.");
		S.SlateText = LOCTEXT("S1Slate", "SC 1 - THE ARRIVAL");
		S.Objectives = {
			Obj(TEXT("S1_Lifeguard"), LOCTEXT("S1O1", "A Lifeguard stands on the beach set"), EFTObjectiveType::CostumeInZone, true, TEXT("Lifeguard"), FTTags::ZoneBeach, LOCTEXT("S1C1", "Suit up as the Lifeguard at the costume rack!")),
			Obj(TEXT("S1_Lighthouse"), LOCTEXT("S1O2", "Switch on the lighthouse"), EFTObjectiveType::DeviceActive, true, FTTags::DevLighthouse, NAME_None, LOCTEXT("S1C2", "Camera needs the lighthouse!")),
			Obj(TEXT("S1_Camera"), LOCTEXT("S1O3", "Man the movie camera"), EFTObjectiveType::CameraOperated, true, NAME_None, NAME_None, LOCTEXT("S1C3", "Someone get behind the camera!")),
			Obj(TEXT("S1_Siren"), LOCTEXT("S1O4", "While rolling: play the warning siren"), EFTObjectiveType::KeyAction, true, FTTags::EvSiren, NAME_None, LOCTEXT("S1C4", "Rolling! Hit the siren on the sound console!")),
			Obj(TEXT("S1_Hold"), LOCTEXT("S1O5", "Frame Lifeguard + lighthouse for 3s"), EFTObjectiveType::HoldShot, true),
			Obj(TEXT("S1_Point"), LOCTEXT("S1O6", "Bonus: Lifeguard points at the water (2)"), EFTObjectiveType::EventDuringRecording, false, FTTags::EvEmotePoint),
			Obj(TEXT("S1_Key"), LOCTEXT("S1O7", "Bonus: warm key light on the beach"), EFTObjectiveType::DeviceActive, false, FTTags::DevKeyLight),
		};
		S.RequiredSubjects = { FTTags::SubjLifeguard, FTTags::SubjLighthouse };
		S.BonusProps = { FTTags::SubjFin, FTTags::SubjLifeRing, FTTags::SubjFinMarker, FTTags::SubjLamp };
		S.CueEvents = { FTTags::EvSiren };
		S.CueDevices = { FTTags::DevLighthouse, FTTags::DevKeyLight };
		S.KeyActionEvent = FTTags::EvSiren;
		S.CaptureDuration = 3.f;
		S.CompletionWindow = 30.f;
		S.TransitionText = LOCTEXT("S1Trans", "That's a wrap on Scene 1! Next up: the rescue boat.");
		S.ScoreHint = LOCTEXT("S1Hint", "Siren while rolling, lifeguard and lighthouse in frame.");
		Scenes.Add(S);
	}
	{
		FFTSceneDefinition S;
		S.SceneId = TEXT("S2_RescueBoat");
		S.Title = LOCTEXT("S2Title", "Scene 2: The Rescue Boat");
		S.ScriptDescription = LOCTEXT("S2Desc", "A storm rolls in. The brave rescue boat heads out - and the shark attacks!");
		S.SlateText = LOCTEXT("S2Slate", "SC 2 - THE RESCUE BOAT");
		S.Objectives = {
			Obj(TEXT("S2_Boat"), LOCTEXT("S2O1", "Push the rescue boat into the tank"), EFTObjectiveType::PropInZone, true, FTTags::PropBoat, FTTags::ZoneTank, LOCTEXT("S2C1", "Get the boat into the tank!")),
			Obj(TEXT("S2_Wind"), LOCTEXT("S2O2", "Run the wind machine"), EFTObjectiveType::DeviceActive, true, FTTags::DevWind, NAME_None, LOCTEXT("S2C2", "Someone crank the wind machine!")),
			Obj(TEXT("S2_Rain"), LOCTEXT("S2O3", "Run the rain machine"), EFTObjectiveType::DeviceActive, true, FTTags::DevRain, NAME_None, LOCTEXT("S2C3", "Someone start the rain!")),
			Obj(TEXT("S2_Camera"), LOCTEXT("S2O4", "Man the movie camera"), EFTObjectiveType::CameraOperated, true),
			Obj(TEXT("S2_Attack"), LOCTEXT("S2O5", "While rolling: shark attacks the boat"), EFTObjectiveType::KeyAction, true, FTTags::EvSharkAttackBoat, NAME_None, LOCTEXT("S2C5", "Shark rig LUNGE - or shark suit: press F at the lunge mark!")),
			Obj(TEXT("S2_Hold"), LOCTEXT("S2O6", "Frame boat + shark + storm for 3s"), EFTObjectiveType::HoldShot, true),
			Obj(TEXT("S2_Storm"), LOCTEXT("S2O7", "Bonus: storm sound cue while rolling"), EFTObjectiveType::EventDuringRecording, false, FTTags::EvStorm),
		};
		S.RequiredSubjects = { FTTags::SubjBoat, FTTags::SubjShark, FTTags::SubjStorm };
		S.BonusProps = { FTTags::SubjLifeRing, FTTags::SubjCrate, FTTags::SubjFinMarker };
		S.CueEvents = { FTTags::EvStorm, FTTags::EvSting };
		S.CueDevices = { FTTags::DevWind, FTTags::DevRain, FTTags::DevFillLight };
		S.KeyActionEvent = FTTags::EvSharkAttackBoat;
		S.CaptureDuration = 3.f;
		S.CompletionWindow = 35.f;
		S.DisasterAfterTake = EFTSceneDisaster::FloodAfterTake;
		S.TransitionText = LOCTEXT("S2Trans", "Cut! ...wait, why is the floor wet?");
		S.ScoreHint = LOCTEXT("S2Hint", "Boat, shark and storm together in one frame.");
		Scenes.Add(S);
	}
	{
		FFTSceneDefinition S;
		S.SceneId = TEXT("S3_FinalBite");
		S.Title = LOCTEXT("S3Title", "Scene 3: The Final Bite");
		S.ScriptDescription = LOCTEXT("S3Desc", "Red lights. Rising water. The hero raises the harpoon as the shark lunges one last time.");
		S.SlateText = LOCTEXT("S3Slate", "SC 3 - THE FINAL BITE");
		S.Objectives = {
			Obj(TEXT("S3_Power"), LOCTEXT("S3O1", "Restore stage power at the breaker"), EFTObjectiveType::PowerRestored, true, NAME_None, NAME_None, LOCTEXT("S3C1", "Lights are dead - restore the breaker!")),
			Obj(TEXT("S3_Harpoon"), LOCTEXT("S3O2", "Hero takes the mark holding the harpoon"), EFTObjectiveType::HeldPropInZone, true, FTTags::PropHarpoon, FTTags::ZoneHeroMark, LOCTEXT("S3C2", "Fetch the hero harpoon from the prop warehouse!")),
			Obj(TEXT("S3_HeroLight"), LOCTEXT("S3O3", "Hero spotlight on (lighting board)"), EFTObjectiveType::DeviceActive, true, FTTags::DevHeroLight, NAME_None, LOCTEXT("S3C3", "Hit the hero spotlight!")),
			Obj(TEXT("S3_Shark"), LOCTEXT("S3O4", "Raise the shark on the rig"), EFTObjectiveType::SharkRigRaised, true, NAME_None, NAME_None, LOCTEXT("S3C4", "Raise the shark rig!")),
			Obj(TEXT("S3_Camera"), LOCTEXT("S3O5", "Man the movie camera"), EFTObjectiveType::CameraOperated, true),
			Obj(TEXT("S3_Defeat"), LOCTEXT("S3O6", "While rolling: LUNGE the shark at the hero"), EFTObjectiveType::KeyAction, true, FTTags::EvSharkDefeat, NAME_None, LOCTEXT("S3C6", "Rolling! Lunge the shark for the staged defeat!")),
			Obj(TEXT("S3_Hold"), LOCTEXT("S3O7", "Frame hero + shark for 3s"), EFTObjectiveType::HoldShot, true),
			Obj(TEXT("S3_Thrust"), LOCTEXT("S3O8", "Bonus: hero thrusts the harpoon (LMB)"), EFTObjectiveType::EventDuringRecording, false, FTTags::EvHeroThrust),
		};
		S.RequiredSubjects = { FTTags::SubjHero, FTTags::SubjShark };
		S.BonusProps = { FTTags::SubjHarpoon, FTTags::SubjLifeRing, FTTags::SubjCrate };
		S.CueEvents = { FTTags::EvSting };
		S.CueDevices = { FTTags::DevHeroLight, FTTags::DevKeyLight };
		S.KeyActionEvent = FTTags::EvSharkDefeat;
		S.CaptureDuration = 3.f;
		S.CompletionWindow = 35.f;
		S.DisasterAfterTake = EFTSceneDisaster::ProjectionUnlockAfterTake;
		S.TransitionText = LOCTEXT("S3Trans", "THAT'S A WRAP! Everybody - the film is in the can!");
		S.ScoreHint = LOCTEXT("S3Hint", "Hero, harpoon and shark in the red light.");
		Scenes.Add(S);
	}
}

UFTFilm_MoonfallMotel::UFTFilm_MoonfallMotel()
{
	FilmId = FTTags::FilmMoonfall;
	Title = LOCTEXT("MoonTitle", "MOONFALL MOTEL");
	Genre = LOCTEXT("MoonGenre", "Low-budget alien invasion");
	Logline = LOCTEXT("MoonLogline", "Room 7 has a view of the desert, free ice and a flying saucer parked on the roof. Check-out is never.");
	Runtime = LOCTEXT("MoonRuntime", "3 scenes  -  approx. 20 min shoot");
	SignatureHazard = LOCTEXT("MoonHazard", "Power surges: the neon motel sign and the saucer share one very tired generator.");
	SignatureEffect = LOCTEXT("MoonEffect", "Flickering UFO prop and a practical abduction beam");
	Difficulty = 3;
	bPlayable = false;
	LockedReason = LOCTEXT("MoonLocked", "IN PRE-PRODUCTION - the neon set is still being wired. Coming after the shark shoot!");
	PosterPrimary = FTColors::Purple;
	PosterSecondary = FTColors::Cyan;
	PosterTagline = LOCTEXT("MoonTagline", "NO VACANCY. NO ESCAPE.");
	StageId = TEXT("Stage5");
	StageLabel = LOCTEXT("MoonStage", "STAGE 5");
	AudienceMultiplier = 1.1f;
	{
		FFTSceneDefinition S;
		S.SceneId = TEXT("M1_Vacancy");
		S.Title = LOCTEXT("M1Title", "Scene 1: Vacancy");
		S.ScriptDescription = LOCTEXT("M1Desc", "The motel sign buzzes to life as the saucer flickers overhead.");
		S.Objectives = {
			Obj(TEXT("M1_Sign"), LOCTEXT("M1O1", "Keep the motel sign powered"), EFTObjectiveType::DeviceActive, true, TEXT("Device.MotelSign")),
			Obj(TEXT("M1_Saucer"), LOCTEXT("M1O2", "Operate the flickering saucer prop"), EFTObjectiveType::DeviceActive, true, TEXT("Device.Saucer")),
			Obj(TEXT("M1_Camera"), LOCTEXT("M1O3", "Man the movie camera"), EFTObjectiveType::CameraOperated, true),
		};
		S.RequiredSubjects = { TEXT("Subject.Saucer"), TEXT("Subject.MotelSign") };
		S.KeyActionEvent = TEXT("Action.SaucerFlicker");
		Scenes.Add(S);
	}
	{
		FFTSceneDefinition S;
		S.SceneId = TEXT("M2_Beam");
		S.Title = LOCTEXT("M2Title", "Scene 2: The Beam");
		S.ScriptDescription = LOCTEXT("M2Desc", "A guest is lifted into the sky by a very practical abduction beam.");
		S.Objectives = {
			Obj(TEXT("M2_Beam"), LOCTEXT("M2O1", "Coordinate the abduction beam"), EFTObjectiveType::DeviceActive, true, TEXT("Device.Beam")),
			Obj(TEXT("M2_Guest"), LOCTEXT("M2O2", "A guest stands in the beam"), EFTObjectiveType::CostumeInZone, true, TEXT("Raincoat"), TEXT("Zone.Beam")),
		};
		S.KeyActionEvent = TEXT("Action.Abduction");
		Scenes.Add(S);
	}
	{
		FFTSceneDefinition S;
		S.SceneId = TEXT("M3_Reveal");
		S.Title = LOCTEXT("M3Title", "Scene 3: They Checked In");
		S.ScriptDescription = LOCTEXT("M3Desc", "The alien reveal, lit only by the motel sign.");
		S.Objectives = {
			Obj(TEXT("M3_Reveal"), LOCTEXT("M3O1", "Stage the alien reveal"), EFTObjectiveType::KeyAction, true, TEXT("Action.AlienReveal")),
		};
		S.KeyActionEvent = TEXT("Action.AlienReveal");
		Scenes.Add(S);
	}
}

UFTFilm_CastleOnFire::UFTFilm_CastleOnFire()
{
	FilmId = FTTags::FilmCastle;
	Title = LOCTEXT("CastleTitle", "CASTLE ON FIRE");
	Genre = LOCTEXT("CastleGenre", "Fantasy adventure (with an accidental dragon)");
	Logline = LOCTEXT("CastleLogline", "The Cardboard Keep has stood for a thousand years. Well, since Tuesday. Tonight the dragon comes home.");
	Runtime = LOCTEXT("CastleRuntime", "3 scenes  -  approx. 25 min shoot");
	SignatureHazard = LOCTEXT("CastleHazard", "Collapsing scenery: the castle wall needs a crew member holding it up at all times.");
	SignatureEffect = LOCTEXT("CastleEffect", "Dragon puppet rig and safe theatrical flame bars");
	Difficulty = 4;
	bPlayable = false;
	LockedReason = LOCTEXT("CastleLocked", "IN PRE-PRODUCTION - the dragon puppet is still at the dry cleaner.");
	PosterPrimary = FTColors::Coral;
	PosterSecondary = FTColors::Yellow;
	PosterTagline = LOCTEXT("CastleTagline", "HOLD THE WALL. MIND THE DRAGON.");
	StageId = TEXT("Stage6");
	StageLabel = LOCTEXT("CastleStage", "STAGE 6");
	AudienceMultiplier = 1.2f;
	{
		FFTSceneDefinition S;
		S.SceneId = TEXT("C1_Keep");
		S.Title = LOCTEXT("C1Title", "Scene 1: The Cardboard Keep");
		S.ScriptDescription = LOCTEXT("C1Desc", "The knight defends the gate while the wall wobbles.");
		S.Objectives = {
			Obj(TEXT("C1_Knight"), LOCTEXT("C1O1", "A Foam Knight guards the gate"), EFTObjectiveType::CostumeInZone, true, TEXT("FoamKnight"), TEXT("Zone.Gate")),
			Obj(TEXT("C1_Wall"), LOCTEXT("C1O2", "Hold the castle wall upright"), EFTObjectiveType::DeviceActive, true, TEXT("Device.CastleWall")),
		};
		S.KeyActionEvent = TEXT("Action.GateCharge");
		Scenes.Add(S);
	}
	{
		FFTSceneDefinition S;
		S.SceneId = TEXT("C2_Dragon");
		S.Title = LOCTEXT("C2Title", "Scene 2: The Accidental Dragon");
		S.ScriptDescription = LOCTEXT("C2Desc", "Raise the dragon puppet over the battlements and run the flame bars.");
		S.Objectives = {
			Obj(TEXT("C2_Dragon"), LOCTEXT("C2O1", "Raise the dragon puppet"), EFTObjectiveType::DeviceActive, true, TEXT("Device.Dragon")),
			Obj(TEXT("C2_Flames"), LOCTEXT("C2O2", "Run the theatrical flame bars"), EFTObjectiveType::DeviceActive, true, TEXT("Device.Flames")),
		};
		S.KeyActionEvent = TEXT("Action.DragonRoar");
		Scenes.Add(S);
	}
	{
		FFTSceneDefinition S;
		S.SceneId = TEXT("C3_Hero");
		S.Title = LOCTEXT("C3Title", "Scene 3: Hold the Wall");
		S.ScriptDescription = LOCTEXT("C3Desc", "The hero shot - with the wall held up just out of frame.");
		S.Objectives = {
			Obj(TEXT("C3_Hero"), LOCTEXT("C3O1", "Hero shot with the wall standing"), EFTObjectiveType::KeyAction, true, TEXT("Action.HeroShot")),
		};
		S.KeyActionEvent = TEXT("Action.HeroShot");
		Scenes.Add(S);
	}
}

#undef LOCTEXT_NAMESPACE
