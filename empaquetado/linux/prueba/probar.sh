#!/bin/bash
# Dentro de una distribución limpia: descomprime el paquete y lo abre de verdad, con ventana y un banco nuevo.
set -u
E=$1
if command -v apt-get >/dev/null; then
    export DEBIAN_FRONTEND=noninteractive
    apt-get update -qq >/dev/null 2>&1
    G=libglib2.0-0
    apt-cache show libglib2.0-0t64 >/dev/null 2>&1 && G=libglib2.0-0t64
    apt-get install -y -qq --no-install-recommends xvfb x11-utils imagemagick libgl1 libegl1 libopengl0 \
        libfontconfig1 libdbus-1-3 "$G" libx11-xcb1 libxcb1 fonts-dejavu-core >/dev/null 2>&1
else
    dnf install -y -q xorg-x11-server-Xvfb xwininfo ImageMagick mesa-libGL mesa-libEGL libglvnd-opengl fontconfig \
        dbus-libs glib2 libX11-xcb libxcb dejavu-sans-fonts >/dev/null 2>&1
fi

mkdir -p /tmp/p /tmp/casa
tar xzf /salida/GeneradorExamenes-*-linux-x86_64.tar.gz -C /tmp/p
A=/tmp/p/GeneradorExamenes
export HOME=/tmp/casa
glibc=$(ldd --version | head -1 | grep -oE '[0-9]+\.[0-9]+$')
faltan=$(ldd "$A/bin/GeneradorExamenes" "$A/plugins/platforms/libqxcb.so" "$A/plugins/sqldrivers/libqsqlite.so" 2>&1 \
    | grep 'not found' | awk '{print $1}' | sort -u | tr '\n' ' ')

QT_QPA_PLATFORM=offscreen "$A/GeneradorExamenes" --comprobar /ejemplo/ejemplo_banco.json >/tmp/comprobar.log 2>&1
comprobar=$?

Xvfb :99 -screen 0 1400x900x24 >/dev/null 2>&1 &
export DISPLAY=:99
for _ in $(seq 1 50); do xwininfo -root >/dev/null 2>&1 && break; sleep 0.2; done
QT_QPA_PLATFORM=xcb "$A/GeneradorExamenes" /tmp/casa/nuevo.db >/tmp/ventana.log 2>&1 &
PID=$!
ventana=no
for _ in $(seq 1 80); do
    xwininfo -root -tree 2>/dev/null | grep -q '"GeneradorExamenes"' && { ventana=si; break; }
    sleep 0.25
done
sleep 2
vivo=no
kill -0 "$PID" 2>/dev/null && vivo=si
banco=no
[ -s /tmp/casa/nuevo.db ] && banco=si
if command -v magick >/dev/null; then magick import -window root "/resultado/$E.png" 2>/dev/null
else import -window root "/resultado/$E.png" 2>/dev/null; fi
chmod a+rw "/resultado/$E.png" 2>/dev/null
kill "$PID" 2>/dev/null

"$A/instalar.sh" >/dev/null 2>&1
lanzador=no
grep -q "^Exec=\"$A/GeneradorExamenes\"$" "$HOME/.local/share/applications/GeneradorExamenes.desktop" 2>/dev/null && lanzador=si

echo "$E glibc=$glibc faltan=[${faltan}] comprobar=$comprobar ventana=$ventana sigue_abierto=$vivo banco_creado=$banco lanzador=$lanzador"
if [ "$ventana" != si ] || [ "$vivo" != si ] || [ "$banco" != si ]; then
    echo "--- registro de $E"
    head -12 /tmp/ventana.log | cut -c1-180
fi
