# The Final Take – Demo

Koop-Komödie in der Ego-Perspektive für 1–4 Spieler (Unreal Engine 5.8, C++). Eine Nachtschicht im Filmstudio: Drehbuch wählen, drei Szenen von **Jaws of the Studio** drehen, die Flut überstehen, die Filmrollen in den Projektionsraum bringen und die Premiere starten.

## Starten
1. `The_Final_Take.uproject` öffnen. Wenn Unreal fragt, ob Module gebaut werden sollen: **Ja**. Oder vorher bauen (siehe `CHANGELOG.md`).
2. Die Map `L_FinalTake_Studio` öffnet automatisch → **Play**.
3. Im Titelmenü **PLAY DEMO**.

Ablauf und Prüfschritte: `TEST_CHECKLIST.md`. Stand, Grenzen und Risiken: `IMPLEMENTATION_STATUS.md`. Handmodellierte Ersatz-Meshes aus Blender (noch nicht eingebaut): `BLENDER_ASSETS.md`.

## Steuerung
| Taste | Aktion |
|---|---|
| WASD / Maus | laufen / schauen |
| Shift / Leertaste | sprinten / springen |
| E | benutzen, tragen (große Schalter halten) |
| Q | ablegen, von der Kamera treten (sie läuft weiter) |
| LMB | werfen, Heldenstoß. An der Kamera: Aufnahme starten/stoppen |
| Mausrad, A/D, R | Kamera: Zoom, Dolly, zentrieren |
| F | Kostüm-Aktion (Hai-Kostüm: Lunge) |
| 1–4 | winken, zeigen, jubeln, Panik |
| G / MMB | Ping |
| H / F1 | Steuerungskarte |
| Esc / P | Pause |

## Projektaufbau
- `Source/The_Final_Take/TheFinalTake/` – gesamte Spiellogik, UI und Level-Aufbau in C++
- `Content/TheFinalTake/` – generierte Meshes, Materialien, Audio, Film-Daten, Map
- `RawAudio/`, `Tools/generate_audio.py` – synthetisch erzeugte Originalsounds
- `TheFinalTake_Art_References_Friendslop/` – Stil-Referenzen (Friendslop-Low-Poly-Palette)
