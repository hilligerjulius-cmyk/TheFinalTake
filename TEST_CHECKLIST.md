# The Final Take – Testcheckliste

`[x]` = von mir tatsächlich ausgeführt (2026-09-24), `[ ]` = noch von Hand zu prüfen.

## 0. Vorbereitung
- [x] Editor schließen, dann bauen:
  `"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" The_Final_TakeEditor Win64 Development -Project="<Pfad>\The_Final_Take.uproject" -WaitMutex`
- [x] Nur nach Änderungen am Map-Builder: `Content/TheFinalTake/Maps/L_FinalTake_Studio.umap` löschen und
  `UnrealEditor-Cmd.exe "<Pfad>\The_Final_Take.uproject" -run=FTContent -map -unattended -nullrhi` ausführen.
- [x] Sicherstellen, dass `Saved/FTAutoTest.txt` **nicht** existiert (sonst startet der automatische Testlauf statt des Titelmenüs; zum Abschalten in `.off` umbenennen).

## 1. Einzelspieler in PIE (Editor → Play, „Selected Viewport“)
1. [x] Titelmenü erscheint über der Straßenansicht. Sichtbar: PLAY DEMO, HOST/JOIN (LAN-Prototyp), SETTINGS, QUIT.
2. [x] PLAY DEMO → Lobby, HUD mit Aufgabenzettel, Uhr 12:00 AM, Studiozustand, Punkten, Hinweisbanner.
3. [x] Zur Stage-4-Tür gehen: Hinweis „Locked – pick a script in the Director's Office first“.
4. [ ] Den Koralle-Pfeilen ins Regiebüro folgen, **E halten** am großen Drehbuch → Drehbuchfenster.
5. [ ] Moonfall Motel anklicken → „IN PRE-PRODUCTION“, nicht wählbar. Jaws of the Studio → GREENLIGHT.
6. [ ] Keycard an der Stage-4-Tür (E) → Tür öffnet.
7. **Szene 1**
   - [ ] Garderobe: Pappaufsteller tragen (E), am Kostümständer „Lifeguard“ anziehen, auf dem Strand absetzen (Q). Alternativ selbst das Kostüm tragen.
   - [ ] Leuchtturm-Schalter (E).
   - [ ] Filmkamera bedienen (E): Sucher-HUD, Maus schwenken, Rad zoomen, A/D Dolly. LMB = Aufnahme.
   - [ ] Q = von der Kamera treten, sie läuft weiter („locked-off shot“). Am Tonpult die Sirene (Cue 0) spielen.
   - [ ] Nach 3 s gehaltenem Bild: Stempel „THAT'S A TAKE“ mit Punkten und Gründen.
8. **Szene 2**
   - [ ] Rettungsboot in den Tank schieben (E halten).
   - [ ] Wind- und Regenmaschine an, Kamera rollen, am Hai-Rig „Lunge“.
   - [ ] Nach dem Take: Sirene, Leck, Stromausfall, Notlicht, Bühne läuft voll.
9. **Szene 3 (geflutet)**
   - [ ] Sicherung am Stage-Eingang halten (E) → Strom zurück. Lichtpult T3 = Heldenspot.
   - [ ] Harpune aus dem Lager holen. Selbst auf die Heldenmarke stellen oder dem Aufsteller geben (Hand-Knopf).
   - [ ] Hai heben (Rig 0), rollen, Lunge → „THE HERO STRIKES!“.
10. **Finale**
    - [ ] Projektionsraum öffnet. Rollen von der Ablage die Treppe hochtragen und am Projektor einlegen (E).
    - [ ] Strom am Projektor-Panel halten, Projektor starten (E halten).
    - [ ] Leinwand rollt ab, Montage mit Standbildern jeder Szene, danach Ergebnisbildschirm.
11. [ ] Ergebnis: RETRY SHOOT setzt alles zurück (Wasser weg, Harpune im Lager, Rollen weg). LEAVE TO TITLE → Titel.
12. [x] Esc/P → Pause (Fortsetzen, Steuerung, Einstellungen, Zurück zur Lobby, Titel, Beenden). [x] Einstellungen öffnen sich. [ ] Regler/Checkbox ändern und nach Neustart prüfen, ob die Werte gespeichert sind.

Automatische Variante (ausgeführt, 36/36 bzw. 39/39 bestanden): `Saved/FTAutoTest.txt` anlegen und PIE starten, oder headless:
`UnrealEditor-Cmd.exe "<Pfad>\The_Final_Take.uproject" /Game/TheFinalTake/Maps/L_FinalTake_Studio -game -nullrhi -nosound -benchmark -fps=30 -FTAutoQuit`
→ Ergebnis in `Saved/FTAutoTestResult.txt`. Mit `-windowed -ResX=1600 -ResY=900 -FTAutoShots` (ohne `-nullrhi`) entstehen Screenshots in `Saved/AutoShots/`.

## 2. Zwei Spieler
Variante A – PIE: Editor → Play-Optionen → *Number of Players* = 2, *Net Mode* = „Play As Listen Server“.
Variante B – zwei Prozesse (von mir so ausgeführt, headless):
- Server: `UnrealEditor-Cmd.exe "<Pfad>\The_Final_Take.uproject" /Game/TheFinalTake/Maps/L_FinalTake_Studio?listen?ftplay=1 -game -port=7777`
- Client: `UnrealEditor-Cmd.exe "<Pfad>\The_Final_Take.uproject" 127.0.0.1:7777 -game`

- [x] Client wird aufgenommen („Welcomed by server“) und bekommt eine eigene Crew-Farbe.
- [x] Client empfängt Drehbuchwahl, alle Ansagen, Take-Ergebnisse, Flut, Finale, Premiere, Ergebnis, Neustart (Client-Log `[Client] …`).
- [ ] Client trägt eine Requisite: Der Host sieht sie in der Hand des Clients.
- [ ] Client bedient die Kamera, Host sieht Kopfbewegung, REC-Lampe und Monitorbild.
- [ ] Beide halten gleichzeitig E am Drehbuch: nur einer bekommt es („… is reading the script“).
- [ ] Zu zweit das Boot schieben (schneller als allein).
- [ ] Beitritt während eines laufenden Drehs wird abgelehnt (Studio geschlossen).
- [ ] HOST STUDIO / JOIN STUDIO im Titelmenü mit zwei Rechnern im LAN.

## 3. Bekannte offene Punkte beim Test
- Der Dawn-Timer (30 Min. → „Blackout“) und die Niederlage durch den Studiozustand wurden nicht ausgelöst.
- Flut-Hai: Telegraph, Angriff und Umwerfen in tiefem Wasser nur im Code, nicht im Test.
