#!/bin/bash
# As probas do núcleo (con swift-testing das Command Line Tools, sen Xcode).
# Traballan en cartafoles temporais: nunca tocan os plugins de verdade.
set -eo pipefail
cd "$(dirname "$0")"
DEV="$(xcode-select -p)"
F="$DEV/Library/Developer/Frameworks"
L="$DEV/Library/Developer/usr/lib"
if [ -d "$F/Testing.framework" ]; then
  exec swift test -Xswiftc -F -Xswiftc "$F" -Xlinker -F -Xlinker "$F" -Xlinker -rpath -Xlinker "$F" -Xlinker -rpath -Xlinker "$L" "$@"
else
  exec swift test "$@"   # con Xcode, todo está no sitio
fi
