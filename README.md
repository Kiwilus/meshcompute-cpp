# MeshCompute C++ – Verteiltes Steuerungssystem

MeshCompute ist ein in modernem **C++17** geschriebenes, dreiteiliges System zur sicheren Fernsteuerung einer Vielzahl von Geräten (Bots) über einen zentralen Server.

Ein **Controller** auf dem Rechner des Administrators kommuniziert per WebSocket über einen **Server** (VPS) mit beliebig vielen **Bots**, die als native Executables (`.exe`, `.apk`, Linux-Binary) verteilt werden.

---

# Architektur

```text
┌─────────────┐       wss://        ┌─────────────┐       wss://       ┌─────────────┐
│ Controller  │ ─────────────────▶ │   Server    │ ◀──────────────── │    Bots     │
│ (Admin-PC)  │                    │    (VPS)    │                   │ (.exe/.apk) │
└─────────────┘                    └─────────────┘                   └─────────────┘
```

- **Server** (C++, Boost.Beast)
  - Läuft auf einem Linux-VPS (Docker)
  - TLS-gesicherter WebSocket-Relay
  - Authentifizierung über SHA-256-gehashte Tokens
  - Leitet Befehle und Antworten zwischen Controller und Bots weiter

- **Controller** (C++, interaktive Konsole)
  - Läuft auf Linux, macOS oder Windows
  - Befehle:
    - `list`
    - `exec`
    - `shell`
    - `sysinfo`
    - `upload`
  - Verbindung via:
    ```text
    wss://<vps-ip>:8443
    ```

- **Bot** (C++, plattformabhängig)
  - Verfügbar als:
    - Windows `.exe`
    - Android `.apk`
    - Linux-Binary
  - Führt Befehle aus
  - Sendet Systeminformationen
  - Empfängt Dateien
  - Baut nach Start automatisch eine Verbindung zum Server auf

---

# Projektstruktur

```text
meshcompute-cpp/
├── CMakeLists.txt
├── config/
│   └── config.json
├── common/
│   ├── config_manager.cpp
│   ├── config_manager.h
│   ├── crypto_utils.cpp
│   └── crypto_utils.h
├── server/
│   ├── main_server.cpp
│   ├── hub.cpp
│   ├── hub.h
│   ├── client_handler.cpp
│   └── client_handler.h
├── controller/
│   ├── main_controller.cpp
│   ├── mesh_controller.cpp
│   └── mesh_controller.h
├── bot/
│   ├── main_bot.cpp
│   ├── bot_handler.cpp
│   └── bot_handler.h
├── android/
│   └── app/
│       ├── build.gradle
│       └── src/main/
│           ├── AndroidManifest.xml
│           ├── java/
│           └── cpp/
├── windows/
│   └── toolchain-mingw.cmake
├── scripts/
│   ├── setup_server.sh
│   ├── setup_controller.sh
│   ├── build_bot_windows.sh
│   ├── build_bot_android.sh
│   └── generate_secrets.sh
└── docker/
    ├── Dockerfile.server
    └── docker-compose.yml
```

---

# Voraussetzungen

## Server (VPS)

- Linux
- Docker
- Docker Compose

Es werden keine weiteren Abhängigkeiten benötigt, da alles innerhalb des Containers gebaut wird.

---

## Controller

Unterstützte Systeme:

- Linux
- macOS
- Windows

Benötigte Tools:

```bash
cmake >= 3.14
g++
make
jq
```

Bibliotheken:

- Boost (system, thread)
- OpenSSL
- nlohmann-json
- spdlog

---

## Bot-Builds

### Windows (.exe)

Möglichkeiten:

- Linux mit `mingw-w64`
- MSYS2/MinGW
- Visual Studio

Benötigt:

- Statische Boost-Bibliotheken
- Statische OpenSSL-Bibliotheken

---

### Android (.apk)

Benötigt:

- Android Studio oder Gradle
- Android NDK
- Vorgebaute Boost/OpenSSL-Libraries

---

# Schnellstart

# 1. Server deployen

```bash
cd scripts
chmod +x setup_server.sh
./setup_server.sh
```

Das Skript generiert automatisch:

- `ADMIN_TOKEN`
- `REGISTRATION_TOKEN`
- TLS-Zertifikate
- `deploy/`-Verzeichnis

Danach Deployment auf den VPS:

```bash
scp -r deploy/ user@<vps-ip>:~/meshcompute

ssh user@<vps-ip>

cd ~/meshcompute
docker compose up -d --build
```

Der Server läuft anschließend unter:

```text
wss://<vps-ip>:8443
```

---

# 2. Controller bauen

```bash
cd scripts
./setup_controller.sh
```

Danach starten:

```bash
cd build
./controller/meshcompute-controller ../config/config.json
```

---

# Controller-Befehle

| Befehl | Beschreibung |
|---|---|
| `list` | Alle verbundenen Bots anzeigen |
| `exec <bot_id> <cmd>` | Befehl auf Bot ausführen |
| `shell <bot_id> <cmd>` | Interaktive Shell |
| `sysinfo <bot_id>` | Systeminformationen abrufen |
| `upload <bot_id> <file>` | Datei hochladen |
| `quit` | Controller beenden |

---

# 3. Bots bauen

## Windows-Bot (.exe)

```bash
./build_bot_windows.sh
```

Ausgabe:

```text
build-win/bot/meshcompute-bot.exe
```

Die EXE ist statisch gelinkt und benötigt keine zusätzlichen DLLs.

---

## Android-Bot (.apk)

Vor dem ersten Build:

```bash
cd android/app
mkdir -p libs
cd libs

git clone https://github.com/PurpleI2P/Boost-for-Android-Prebuilt.git -b boost-1_78_0 boost
git clone https://github.com/PurpleI2P/OpenSSL-for-Android-Prebuilt.git openssl
```

APK bauen:

```bash
cd ../../scripts
./build_bot_android.sh
```

Ausgabe:

```text
android/app/build/outputs/apk/release/app-release.apk
```

---

# Sicherheit

- Tokens werden als SHA-256-Hash gespeichert
- Kommunikation erfolgt ausschließlich über TLS (`wss://`)
- Selbstsignierte Zertifikate werden automatisch erstellt
- Für Produktion sollten echte Zertifikate genutzt werden (z.B. Let's Encrypt)

Wichtige Tokens:

- `ADMIN_TOKEN`
- `REGISTRATION_TOKEN`

Bei Verlust:

```bash
./generate_secrets.sh
```

Danach muss der Server neu gebaut werden.

---

# Plattformdetails

## Linux

Verwendet:

```cpp
sysinfo()
```

für:

- RAM
- Load
- Uptime

---

## Windows

Verwendet:

```cpp
GlobalMemoryStatusEx()
GetTickCount64()
```

---

## Android

Verwendet:

```text
/proc/meminfo
/proc/uptime
```

Die Android-App startet einen Foreground-Service, der dauerhaft aktiv bleibt.

---

# Docker-Deployment

Der Server wird vollständig über Docker betrieben.

Container stoppen:

```bash
docker compose down
```

Container neu bauen:

```bash
docker compose up -d --build
```

---

# Fehlerbehebung

| Problem | Lösung |
|---|---|
| Android-Bibliotheken liefern 404 | Verwende die Git-Repositories statt wget |
| Controller verbindet sich nicht | Firewall, URL und Docker prüfen |
| Bot erscheint nicht in `list` | Registration-Token prüfen |
| Windows-Build findet `boost_system` nicht | mingw-w64 + statische Libraries installieren |
| Android-APK stürzt ab | Bibliothekspfade in CMake prüfen |

---

# Features

- TLS-gesicherte WebSocket-Kommunikation
- Plattformübergreifende Bots
- Docker-basierter Server
- SHA-256-Authentifizierung
- Dateiübertragung
- Remote-Kommandoausführung
- Systeminformationen
- Android-Hintergrunddienst
- Windows-Standalone-EXE
- Modernes C++17
- Boost.Beast Networking

---

# Lizenz

Nur für Bildungs-, Forschungs- und Testzwecke.

Der Einsatz auf fremden Systemen ohne ausdrückliche Zustimmung ist untersagt.
