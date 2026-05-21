#!/bin/bash
set -e

echo "======================================"
echo " Android Bot Build (APK)"
echo "======================================"

if [ ! -d "../android" ]; then
    echo "Android-Ordner nicht gefunden. Bitte Projektstruktur prüfen."
    exit 1
fi

read -p "Server URL (wss://<ip>:<port>): " SERVER_URL
read -sp "Registration Token: " REG_TOKEN
echo
read -p "Bot ID (z.B. android-1): " BOT_ID

# Token in MainActivity.java eintragen (einfache sed-Ersetzung)
MAIN_ACTIVITY="../android/app/src/main/java/com/meshcompute/bot/MainActivity.java"
if [ -f "$MAIN_ACTIVITY" ]; then
    sed -i "s|wss://your-vps:8443|${SERVER_URL}|g" "$MAIN_ACTIVITY"
    sed -i "s|your-registration-token|${REG_TOKEN}|g" "$MAIN_ACTIVITY"
    sed -i "s|android-bot-1|${BOT_ID}|g" "$MAIN_ACTIVITY"
else
    echo "Warnung: MainActivity.java nicht gefunden, Tokens müssen manuell gesetzt werden."
fi

cd ../android
./gradlew assembleRelease

echo ""
echo "======================================"
echo " APK erstellt: android/app/build/outputs/apk/release/app-release.apk"
echo "======================================"