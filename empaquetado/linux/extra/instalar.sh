#!/bin/sh
# Pone el programa en el menú del escritorio del usuario, apuntando a esta carpeta. Con --quitar, lo quita.
# Si se mueve la carpeta, hay que volver a ejecutarlo.
set -e
DIR=$(dirname "$(readlink -f "$0")")
DATOS=${XDG_DATA_HOME:-$HOME/.local/share}
LANZADOR=$DATOS/applications/GeneradorExamenes.desktop
ICONOS=$DATOS/icons/hicolor
TAMANOS="16 24 32 48 64 128 256"

if [ "$1" = "--quitar" ]; then
    rm -f "$LANZADOR" "$ICONOS/scalable/apps/generador-examenes.svg"
    for n in $TAMANOS; do
        rm -f "$ICONOS/${n}x$n/apps/generador-examenes.png"
    done
    echo "Quitado del menú del escritorio."
else
    for n in $TAMANOS; do
        mkdir -p "$ICONOS/${n}x$n/apps"
        cp "$DIR/iconos/icono_$n.png" "$ICONOS/${n}x$n/apps/generador-examenes.png"
    done
    mkdir -p "$ICONOS/scalable/apps" "$DATOS/applications"
    cp "$DIR/iconos/icono.svg" "$ICONOS/scalable/apps/generador-examenes.svg"
    sed "s|@RUTA_EJECUTABLE@|$DIR/GeneradorExamenes|" "$DIR/iconos/GeneradorExamenes.desktop.in" > "$LANZADOR"
    echo "Añadido al menú del escritorio."
fi

if [ -f "$ICONOS/icon-theme.cache" ] && command -v gtk-update-icon-cache >/dev/null; then
    gtk-update-icon-cache -f -t -q "$ICONOS" || true
fi
if command -v update-desktop-database >/dev/null; then
    update-desktop-database -q "$DATOS/applications" || true
fi
