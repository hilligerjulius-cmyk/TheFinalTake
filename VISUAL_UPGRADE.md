# Visuelle Weiterentwicklung des bestehenden Spiels

## Bestandsaufnahme – 24.09.2026

Basis: vorhandene `L_FinalTake_Studio`, C++-Studioaufbau und vorhandene Originalassets. Kein neues Spiel und kein Ersatz des Spielablaufs. Git-Ausgangsstand ist sauber; frühere Vergleichsbilder sind zusätzlich unter `Saved/VisualBaseline/AutoShots` gesichert.

| Bereich | Vorhandene sichtbare Inhalte | Gestaltungsbedarf |
|---|---|---|
| Straße | Studiomarquee, Regen, Gehweg, Straßenlampen, Eingang | Lichtstaffelung, Oberflächendetails |
| Lobby | Teppich, Fliesen, Empfang, Poster, Pflanzen, Stage-4-Tor | Gelbstich reduzieren, Pflanzen und Möbel ausarbeiten |
| Büro | Drehbuch, Tisch, Regiestuhl, Sofa, Bücher, Tischlampe | Materialtrennung, dekorative Details |
| Garderobe | Kostümständer, Spiegel, Leuchten, Spinde | Beschläge, Spiegelrahmen, Kostümformen |
| Stage 4 | Träger, Arbeitslichter, Filmkamera, Ton-/Lichtpulte, Kulissen | Plastizität, charaktervolle technische Requisiten |
| Tank/Strand | Wasser, Sandinsel, Palmen, Leuchtturm, Dock, Boot, Hai-Rig | Organische Formen, Gummi-/Metallunterschiede |
| Lager | Kisten, Studiokoffer, Regale, Ersatzrequisiten | Kofferbeschläge und differenzierte Oberflächen |
| Projektion | Projektor, Rollen, Strahl, Leinwand, Galerie | Lichtakzente und saubere Silhouetten |
| Flut | Wasserfläche, Sprühstrahl, Alarm, schwimmender Hai | Lesbarkeit unter vorhandenen Zuständen prüfen |
| Crew | Vier Looks, Ego-Arme, Gesichter, vier Kostüme | Handschuhe lesen sich als kugelige Platzhalter |

Materialbestand: Matte/MatteISM, Glow, Translucent, Water, Highlight, Screen, Text und Outline-Postprocessing. Geometrie wird aus zehn projektspezifischen Basismeshes und Engine-Flächen zusammengesetzt. Vorhandene Stärken: konsistente Teal/Koralle/Creme-Palette, klare Wegführung, instanzierte Studioarchitektur, vollständiger Demoablauf. Vorhandene Effekte: Regen, Spritzwasser, Rauch, Schaum, Wind, Projektorstrahl und Wasserbewegung.

## Grenzen

Bestehende Kollisionen, Wege, Trigger, Bedienung, Kamera-Perspektive, Missionsablauf, Wertung, Netzwerk und UI bleiben erhalten. Neue Ausstattungsdetails sind rein dekorativ. Audio bleibt im ersten visuellen Durchgang unverändert; vorhandene Originalsounds sind integriert, eine Hörprüfung ist noch offen.

## Umgesetzt

- Eigene neue Meshes `SM_FT_CrewTorso` (abgerundete Jackenform) und `SM_FT_Shoreline` (zusammenhängende Sandkante). Die vorhandenen Basismeshes bleiben erhalten; beim Ball-Mesh wurden nur die Oberflächennormalen geglättet.
- Crew: kräftigere Gliedmaßen, Gürtelverschluss, Funkgerät-Details, Brusttasche, Stift, Crew-Ausweis, Sohlen und Schnürsenkel. Ego-Handschuhe mit Daumen, Polstern und Nähten; Manschetten und Ärmelproportionen angepasst.
- Pflanzen: einzelne Blätter, Stiele, Erde und Topfrand; Palmen mit gebogenen Blattgruppen und Kokosnüssen. Dekorative Felsen und Tankufer überarbeitet.
- Studiokoffer: Kantenschutz, Beschläge, Verschlüsse, Griff und Kennzeichnung. Holzkisten: Verstärkungen und Griffmulden. Details verwenden die vorhandenen instanzierten Meshgruppen.
- Lagerregale: unterschiedliche Objektivkoffer, Filmrollen, gefaltete Tondecken und Kabel. Garderobe: Spiegelrahmen, Spindgriffe und Lüftungsschlitze. Projektionsraum: Sitz-/Rückenpolster, Armlehnen und Getränkehalter.
- Filmkamera: offener Objektivrahmen, Gegenlichtblende, Fokusrad und Gehäuseschlitze. Lack, Objektivring und Glas mit unterschiedlichen Glanzwerten.
- Materialien: subtile Pigmentvariation, differenzierbarer Oberflächenglanz auch für instanzierte Studiodekoration und schwächere, schmalere Konturen. Wasser- und Effektmaterialien bleiben erhalten.
- Licht: weichere Lichtquellen, ausgewogeneres Warm/Kalt-Verhältnis in Lobby und Büro, neutraleres Garderobenlicht. Weniger Bloom und Vignette; im Filmsucher weniger Korn und Farbsäume, bestehende Schärfentiefe bleibt erhalten.

## Technisch getrennte Anpassungen

Zwei Einträge im visuellen Shape-Katalog und ein zusätzlicher Glanzwert pro Studio-Instanz unterstützen die neuen Assets. Keine Änderungen an Missions-, Netzwerk-, Eingabe-, Wertungs- oder Speicherlogik. Verpackung: Projektname korrigiert; Studio-Map und dynamisch geladene Projektassets explizit in den Cook aufgenommen. Das betrifft die Auslieferung, nicht den Spielablauf.

## Bisher tatsächlich geprüft

- Ausgangslauf: 52/52 automatische Schritte bestanden; 1600 × 900, RTX 4060, durchschnittlich 117,3 FPS, 1%-Low 84,0 FPS. Sieben Ausreißer über 100 ms im Lauf mit Screenshot-Aufnahmen.
- Erster Umbau: 52/52 Schritte bestanden; durchschnittlich 113,5 FPS, 1%-Low 82,4 FPS, ebenfalls sieben Ausreißer. Diese Werte gelten noch nicht für den zuletzt erweiterten Stand.
- C++-Build und Map-/Asset-Generierung des letzten Standes erfolgreich; Content-Commandlet meldet null Fehler und null Warnungen.
- Alle acht Bereiche und vier Crew-/Kostümvarianten im tatsächlichen Unreal-Editor-Viewport aufgenommen und betrachtet. Ergebnisse in `Saved/VisualReview/`; Tank und Crew nach den neuen Meshes erneut geprüft. Die Charakteraufstellung existiert nur vorübergehend im Prüfeditor und wird nicht in die Spielmap gespeichert.
- 35 vorhandene WAVs geprüft: keine stummen Dateien, keine geclippten Samples, höchster Sample-Peak 0,8499. Audio wurde nicht subjektiv abgehört und in diesem visuellen Durchgang nicht ersetzt.
- Windows-Paket noch in Arbeit. Erster Paketlauf scheiterte an einer vom Prüfeditor gesperrten DLL; der Spielcode selbst wurde dabei erfolgreich kompiliert. Abschließende Paket- und Laufzeitergebnisse folgen nach tatsächlicher Prüfung.

## Reproduzierbarkeit

`Tools/polish_materials.py` aktualisiert die bestehenden Materialien idempotent im Unreal-Python-Commandlet. `Tools/visual_review.py` erzeugt Inventar und Editorbilder; mit `-FTReviewModelsOnly` werden nur Tank, Crew und Kostüme erfasst. Beide Skripte sind Hilfen für die Gestaltung und werden nicht vom normalen Spiel ausgeführt.
