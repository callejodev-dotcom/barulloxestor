#!/bin/bash
# Empaqueta Barullo Xestor para Linux (x86_64): un .tar.gz co programa, a icona e un instalador
# para o usuario (sen sudo: ~/.local/bin e o menú).
# Uso: empaquetado/linux.sh <carpeta da compilación> <versión> <carpeta de saída>
set -eo pipefail
trap 'echo "::error::$(basename "$0") liña $LINENO: $BASH_COMMAND"' ERR
BUILD="$1"; VER="$2"; OUT="$3"
RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
WORK="$(mktemp -d)"
D="$WORK/BarulloXestor-$VER-linux-x86_64"
mkdir -p "$D"
cp "$BUILD/BarulloXestor_artefacts/Release/Barullo Xestor" "$D/barullo-xestor"
strip "$D/barullo-xestor" || true
cp "$RAIZ/Resources/Icona.png" "$D/barullo-xestor.png"
cat > "$D/instalar.sh" <<'EOI'
#!/bin/sh
# Instala Barullo Xestor para o teu usuario (sen sudo)
set -e
cd "$(dirname "$0")"
mkdir -p "$HOME/.local/bin" "$HOME/.local/share/applications" "$HOME/.local/share/icons/hicolor/256x256/apps"
cp barullo-xestor "$HOME/.local/bin/"
cp barullo-xestor.png "$HOME/.local/share/icons/hicolor/256x256/apps/"
cat > "$HOME/.local/share/applications/barullo-xestor.desktop" <<EOD
[Desktop Entry]
Type=Application
Name=Barullo Xestor
Comment=Os plugins Barullo de Escoita: instalar e ter ao día
Exec=$HOME/.local/bin/barullo-xestor
Icon=barullo-xestor
Categories=AudioVideo;Audio;
EOD
echo "Instalado: está no menú e en ~/.local/bin/barullo-xestor"
EOI
chmod +x "$D/instalar.sh" "$D/barullo-xestor"
cat > "$D/LEEME.txt" <<EOL
BARULLO XESTOR $VER para Linux (x86_64)
Unha ferramenta de Escoita · https://www.escoita.link

Instalar: ./instalar.sh   (para o teu usuario, sen sudo)

Mira, instala e ten ao día os plugins Barullo (VST3, CLAP e LV2 en ~/.vst3, ~/.clap e ~/.lv2).
Para quitar unha copia de /usr/lib/vst3 ou /usr/local/lib/vst3 pide o permiso con pkexec.
Precisa libcurl (case todas as distribucións a levan).
Sen xanela: barullo-xestor --axuda
EOL
tar -C "$WORK" -czf "$OUT/BarulloXestor-$VER-linux-x86_64.tar.gz" "BarulloXestor-$VER-linux-x86_64"
echo "Feito: $OUT/BarulloXestor-$VER-linux-x86_64.tar.gz"
