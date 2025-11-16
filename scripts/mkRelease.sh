#!/usr/bin/env bash
set -e

rm -rf build
meson --prefix /usr build
ninja dist -C build

VERSION=$(grep "version:" meson.build | head -n1 | cut -d"'" -f2)
TAR="budgie-session-${VERSION}.tar.xz"
VTAR="budgie-session-v${VERSION}.tar.xz"

mv build/meson-dist/$TAR $VTAR

gpg --armor --detach-sign --local-use 0x1E1FB0017C998A8AE2C498A6C2EAA8A26ADC59EE $VTAR
gpg --verify "${VTAR}.asc"