# Änderungsprotokoll

## 2026-09-24 – Fixes nach dem ersten Spieltest
- **Absturz behoben:** Wurde das Drehbuch nach einer Garbage Collection erneut geöffnet, las es freigegebene Filmdaten. Das Widget hält die Filme jetzt fest, und der Testlauf prüft genau diesen Fall.
- **Ruckler behoben:**
  - Materialien, Meshes, Sounds und Filmdaten bleiben nach dem ersten Laden im Speicher. Vorher wurden sie nach jeder GC synchron nachgeladen.
  - Alle Sounds und Filme werden beim Start vorgeladen.
  - Das Highlight wird nicht mehr auf Partikel-ISMs gelegt. Die ISM-Nutzung ist an allen FT-Materialien gesetzt, vorher wurden sie bei jedem Start neu kompiliert.
  - Der Kamera-Monitor rendert ohne Lumen, Distance-Field-AO und Volumetric Fog.
  - Gemessen (1600×900, gerendert): 121 FPS im Schnitt, 1 %-Low 93 FPS, 0 Ruckler über 100 ms.
- **Filmkamera:**
  - Die Neigung ist umgedreht (Maus hoch = Kamera hoch), die Bewegungen sind gedämpft wie bei einem Fluid-Head.
  - Cinematic-Look: Schärfentiefe mit Autofokus, Vignette, Filmkorn, leichte Farbsäume, warm/teal Grading und 2.39:1-Letterbox im Sucher.
- **Physik:**
  - Das Rettungsboot ist begehbar (Rumpf und Boden haben Kollision).
  - Das Hai-Rig steht jetzt im freien Wasser zwischen Insel und Steg; Schiene, Heck und Sprung bleiben außerhalb der Insel.
  - Der Flut-Hai prüft, ob Wasser tief genug ist und kein Hindernis im Weg liegt, und schwimmt nicht mehr durch Kulissen.
- **Optik:**
  - Die Tonkabine hat einen echten Rahmen (Pfosten, Querbalken, Neonleiste, Schild) statt frei schwebender Teile.
  - Schilder an Garderobe und Projektionsraum gibt es nicht mehr doppelt.
  - Beschriftungen nutzen ein einseitiges Textmaterial (`M_FT_Text`) und sind von hinten nicht mehr gespiegelt sichtbar.

## 2026-09-24 – `8fd8a14` Politur nach gerendertem Spieltest
- HUD (`UI/FTHUDWidget.*`): feste Umbruchbreiten statt Auto-Wrap (Hinweisbanner, Ansagen, Aufgaben), Sucher-Motivliste als fester Zeilenpool (Zeilen lagen übereinander), Hinweis und Steuerungskarte im Sucher und in Menüs ausgeblendet, Uhr im 12-Stunden-Format.
- Menüs (`UI/FTMenus.cpp`): Drehbuch-Details scrollen innerhalb der Seite, Einstellungen mit gestyltem Regler und Checkbox, Premieren-Untertitel unten verankert.
- Premiere: Projektorstrahl behält das Glow-Material (vorher undurchsichtiger Kegel), Leinwand hängt unter der Decke.
- Figur: helle Handschuhe und Akzent-Manschetten. `bCarrying` wird aus `HeldProp` abgeleitet (CARRY-Marke blieb hängen).
- Test (`Game/FTAutoDirector.*`): `-FTAutoPlayers=N`, `-FTAutoShots`, echte Drehbuch-Interaktion, weitere Kameraeinstellung. Client-Logs in `FTGameState.cpp`.
- Map (`Editor/FTMapBuilder.cpp`): Titelkamera neu ausgerichtet, feste Belichtung, Leinwandhöhe.

## 2026-09-24 – `cb66a3d` Map-Generator und vollständiger Spielablauf
- `Editor/FTMapBuilder.cpp`: baut `L_FinalTake_Studio` komplett im Commandlet (alle Stationen, Requisiten, Zonen, Licht, Nebel, Postprocess).
- Automatischer Testlauf: Diagnose der Hände und Ziele. Geräte werden nur eingeschaltet, nie hin- und hergeschaltet (vorher flackerte der Leuchtturm zurück auf „aus“).
- `FTFilmCamera::FrameTargets` richtet die Kamera auf die Motive. Der besiegte Hai bleibt 6 s sichtbar, damit der 3-s-Halt klappt.
- Optik: Roll-Leinwand statt rotem Vorhangblock, Leuchtturm-Strahl standardmäßig aus, Wandverkleidung an der Stage-Wand, Pfeilrichtung am Schild, hellerer Asphalt, leuchtende Lampenschirme.

## 2026-09-23 – `a6d13f7` C++-Modul, UI, Studio, Content-Commandlet
- Modul `The_Final_Take` in `.uproject`, Targets, alle Spielsysteme, UMG-Oberflächen in C++.
- `FTContentCommandlet`: Low-Poly-Meshes, WAV-Import (35), Film-Datenassets.

## 2026-09-23 – `33617f6` Sicherung vor der Demo-Arbeit

## Befehle
| Zweck | Befehl |
|---|---|
| Build | `Build.bat The_Final_TakeEditor Win64 Development -Project=<uproject> -WaitMutex` |
| Content + Map | `UnrealEditor-Cmd.exe <uproject> -run=FTContent -map -unattended -nullrhi` (vorher `.umap` löschen) |
| Testlauf headless | `UnrealEditor-Cmd.exe <uproject> /Game/TheFinalTake/Maps/L_FinalTake_Studio -game -nullrhi -nosound -benchmark -fps=30 -FTAutoQuit` (mit `Saved/FTAutoTest.txt`) |
| Testlauf mit Bildern | `UnrealEditor.exe <uproject> … -game -windowed -ResX=1600 -ResY=900 -FTAutoShots -FTAutoQuit` |
| 2 Spieler | Server `…?listen?ftplay=1 -game -port=7777 -FTAutoPlayers=2`, Client `<uproject> 127.0.0.1:7777 -game` |
| Audio neu erzeugen | `python Tools/generate_audio.py` |
