# Projekt: Kaffee-Fragebogen (NDS Homebrew)

## Was das hier ist

Eine kleine Nintendo-DS/DSi-Homebrew-Anwendung. Gäste bekommen die Konsole in die Hand
gedrückt und beantworten über den Touchscreen ein paar Fragen zu ihrer Kaffeebestellung
(z. B. Kaffeeart, Milch ja/nein, Zucker, Größe). Am Ende soll das Ergebnis übersichtlich
angezeigt werden (und optional gespeichert/exportiert werden können).

Zielplattform: Nintendo DSi, läuft über einen R4-Klon (Ace3DS+/R4iLS-Familie) mit
AceOS als Kernel. Entwicklung und Tests laufen überwiegend im Emulator (melonDS),
finale Verifikation auf echter Hardware.

## Tech-Stack

- **Sprache:** C (ggf. C++, falls sich das als sinnvoller erweist)
- **Toolchain:** devkitARM (Teil von devkitPro)
- **Bibliothek:** libnds (Grafik, Touchscreen-Input, Text-Rendering)
- **Für Datei-I/O (falls gebraucht):** libfat
- **Build-System:** Makefile (Standard-devkitARM-Konventionen), Build via `make`
- **Output:** `.nds`-Datei, landet im Projekt-Root bzw. `build`-Ordner (je nach Makefile)

## Wie man baut und testet

```bash
make            # kompiliert das Projekt, erzeugt die .nds-Datei
make clean      # räumt Build-Artefakte auf
```

Zum Testen im Emulator (melonDS), Pfad zur erzeugten `.nds`-Datei übergeben:

```bash
/Applications/melonDS.app/Contents/MacOS/melonDS <pfad-zur-nds-datei>
```

In CLion ist dafür eine Custom-Build-Run-Konfiguration eingerichtet: Build-Target ruft
`make` auf, danach startet die Konfiguration automatisch melonDS mit der frischen
`.nds`-Datei. Für den finalen Test auf echter Hardware: `.nds`-Datei in den `Games`-Ordner
im Root der SD-Karte kopieren (AceOS listet alles dort automatisch im Menü auf).

## Hardware-Kontext (relevant für Debugging)

- DSi-Firmware-Version: 1.4.4E
- Flashcart: R4-SDHC-Klon, identifiziert als **Ace3DS+/R4iLS-Familie**
  (Fehlerbild bei leerer SD-Karte war `Can't open_dsmenu.dat`)
- Kernel: AceOS (R4iLS-Paket), Setup-Anleitung: github.com/flashcarts/AOS
- **Wichtig bei SD-Karten-Handling unter macOS:** Nach dem Kopieren von Dateien auf die
  FAT32-SD-Karte `dot_clean /Volumes/<KARTENNAME>` ausführen, um versteckte macOS-
  Systemdateien (`.DS_Store`, `._*`) zu entfernen — diese können den Kernel stören.

## Bekannte Stolpersteine aus dem Setup

- `dkp-pacman` braucht `sudo`.
- Nach der devkitPro-Installation braucht es einen Neustart/Neu-Login, damit
  `DEVKITPRO`/`DEVKITARM`-Umgebungsvariablen systemweit verfügbar sind (auch für
  grafische Apps wie CLion).
- CLion findet `make` und die Emulator-Executable oft nicht über den bloßen Namen,
  weil grafische macOS-Apps nicht automatisch die Shell-`PATH`-Variable erben.
  Volle, absolute Pfade verwenden (z. B. `/usr/bin/make`,
  `/Applications/melonDS.app/Contents/MacOS/melonDS`).
- Bei "Custom Build Application"-Run-Konfigurationen in CLion sauber trennen:
    - **Executable** = Pfad zum Emulator
    - **Program arguments** = kompletter Pfad zur `.nds`-Datei (nicht nur der Ordner!)
    - **Working directory** = Projektordner

  
## Konventionen / Präferenzen

- Kleine, nachvollziehbare Commits.
- Kein Übernehmen von devkitPro-Beispielcode 1:1 ohne Verständnis — lieber Stück für
  Stück anpassen und die Codebasis wirklich durchdringen (Lernprojekt).