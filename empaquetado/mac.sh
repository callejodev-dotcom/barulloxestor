#!/bin/bash
# Empaqueta Barullo Xestor para Mac: un .zip coa app (universal: Apple Silicon + Intel).
# Uso: empaquetado/mac.sh <carpeta da compilación> <versión> <carpeta de saída>
# Firma coma os plugins (escoita-nucleo/empaquetado/mac.sh): con MACOS_DEV_ID, co certificado de
# Apple e, con APPLE_ID, APPLE_TEAM_ID e APPLE_APP_PASSWORD, tamén notarizada; se non, «ad hoc».
set -eo pipefail
trap 'echo "::error::$(basename "$0") liña $LINENO: $BASH_COMMAND"' ERR
BUILD="$1"; VER="$2"; OUT="$3"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
WORK="$(mktemp -d)"
APP="$WORK/Barullo Xestor.app"
ditto "$BUILD/BarulloXestor_artefacts/Release/Barullo Xestor.app" "$APP"
lipo -info "$APP/Contents/MacOS/Barullo Xestor"   # comprobar que é universal
if [ -n "$MACOS_DEV_ID" ]; then
  codesign --force --deep --options runtime --timestamp --sign "$MACOS_DEV_ID" "$APP"
else
  codesign --force --deep --sign - "$APP"
fi
codesign --verify --deep --strict "$APP"
ZIP="$OUT/BarulloXestor-$VER-mac.zip"
(cd "$WORK" && ditto -c -k --sequesterRsrc --keepParent "Barullo Xestor.app" "$ZIP")
if [ -n "$APPLE_ID" ] && [ -n "$APPLE_TEAM_ID" ] && [ -n "$APPLE_APP_PASSWORD" ] && [ -n "$MACOS_DEV_ID" ]; then
  xcrun notarytool submit "$ZIP" --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" --password "$APPLE_APP_PASSWORD" --wait
  xcrun stapler staple "$APP"
  rm "$ZIP"
  (cd "$WORK" && ditto -c -k --sequesterRsrc --keepParent "Barullo Xestor.app" "$ZIP")
fi
echo "Feito: $ZIP"
