#!/bin/bash
# Compila Barullo Xestor e fai dist/Barullo Xestor.app (sen Xcode: SwiftPM + este script).
# Uso: ./build.sh            → dist/Barullo Xestor.app
#      ./build.sh --instalar → e cópiaa a ~/Applications
set -eo pipefail
cd "$(dirname "$0")"
VERSION="0.1.0"
APP="dist/Barullo Xestor.app"

swift build -c release --product BarulloXestor
BIN="$(swift build -c release --show-bin-path)/BarulloXestor"

rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
cp "$BIN" "$APP/Contents/MacOS/BarulloXestor"
sed "s/VERSION/$VERSION/g" Recursos/Info.plist > "$APP/Contents/Info.plist"
cp Recursos/AppIcon.icns "$APP/Contents/Resources/"
printf 'APPL????' > "$APP/Contents/PkgInfo"
codesign --force --deep -s - "$APP"
codesign --verify --deep --strict "$APP"
echo "Feito: $APP"

if [ "$1" = "--instalar" ]; then
  mkdir -p "$HOME/Applications"
  DEST="$HOME/Applications/Barullo Xestor.app"
  # a anterior, ao Lixo (nunca rm)
  if [ -d "$DEST" ]; then
    mv "$DEST" "$HOME/.Trash/Barullo Xestor $(date +%s).app"
  fi
  ditto "$APP" "$DEST"
  echo "Copiada a $DEST"
fi
