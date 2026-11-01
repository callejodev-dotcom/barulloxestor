#!/bin/bash
# Compila Barullo Xestor (JUCE + CMake) e pasa as probas.
# Uso: ./build.sh            → build/BarulloXestor_artefacts/Release/
#      ./build.sh --instalar → (Mac) e cópiaa a ~/Applications (a anterior, ao Lixo; nunca rm)
# JUCE e o núcleo: os cartafoles irmáns ../JUCE e ../escoita-nucleo, coma nos plugins.
set -eo pipefail
cd "$(dirname "$0")"
GEN=()
command -v ninja >/dev/null 2>&1 && GEN=(-G Ninja)
cmake -B build ${GEN[@]+"${GEN[@]}"} -DCMAKE_BUILD_TYPE=Release
J="$( (sysctl -n hw.ncpu || nproc) 2>/dev/null)"
cmake --build build --config Release -j "$J"
./build/XestorProbas_artefacts/Release/XestorProbas

if [ "$1" = "--instalar" ] && [ "$(uname)" = Darwin ]; then
  APP="build/BarulloXestor_artefacts/Release/Barullo Xestor.app"
  codesign --force --deep -s - "$APP"
  codesign --verify --deep --strict "$APP"
  mkdir -p "$HOME/Applications"
  DEST="$HOME/Applications/Barullo Xestor.app"
  if [ -d "$DEST" ]; then
    mv "$DEST" "$HOME/.Trash/Barullo Xestor $(date +%Y-%m-%d\ %H.%M.%S).app"
  fi
  ditto "$APP" "$DEST"
  echo "Copiada a $DEST"
fi
