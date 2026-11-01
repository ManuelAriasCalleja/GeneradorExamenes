#!/bin/sh
# Instala para el usuario actual el lanzador del programa en el menú del escritorio de Linux (Cinnamon, GNOME,
# KDE...): el .desktop en ~/.local/share/applications y el icono en el tema hicolor de ~/.local/share/icons.
# Si hay una copia del lanzador en el escritorio, también la actualiza. El lanzador apunta al ejecutable de
# build/, así que hay que compilar antes (y volver a ejecutar esto si se mueve el proyecto). Con --quitar, lo
# desinstala (también del escritorio). Para instalar el programa compilado fuera del proyecto, compilar.sh.
set -e
cd "$(dirname "$0")/.."
PROYECTO=$(pwd)
DATOS=${XDG_DATA_HOME:-$HOME/.local/share}
LANZADOR=$DATOS/applications/GeneradorExamenesFI.desktop
ESCRITORIO=$(xdg-user-dir DESKTOP 2>/dev/null || echo "$HOME/Desktop")/GeneradorExamenesFI.desktop
ICONOS=$DATOS/icons/hicolor
NOMBRE_ICONO=generador-examenes-fi
TAMANOS="16 24 32 48 64 128 256"

if [ "$1" = "--quitar" ]; then
    rm -f "$LANZADOR" "$ESCRITORIO"
    for n in $TAMANOS; do
        rm -f "$ICONOS/${n}x$n/apps/$NOMBRE_ICONO.png"
    done
else
    if [ ! -x build/GeneradorExamenesFI ]; then
        echo "Falta build/GeneradorExamenesFI: compile primero el programa." >&2
        exit 1
    fi
    for n in $TAMANOS; do
        mkdir -p "$ICONOS/${n}x$n/apps"
        cp "recursos/icono/icono_$n.png" "$ICONOS/${n}x$n/apps/$NOMBRE_ICONO.png"
    done
    mkdir -p "$DATOS/applications"
    sed "s|@RUTA_EJECUTABLE@|$PROYECTO/build/GeneradorExamenesFI|" recursos/GeneradorExamenesFI.desktop.in > "$LANZADOR"
    # La copia del escritorio se sobrescribe sin sustituir el fichero, para que conserve el permiso de ejecución
    # y la marca de «de confianza» que le pone el gestor de archivos.
    if [ -f "$ESCRITORIO" ]; then
        cat "$LANZADOR" > "$ESCRITORIO"
    fi
fi

# La caché de iconos solo se actualiza si ya existe; si no, el escritorio lee las carpetas directamente.
if [ -f "$ICONOS/icon-theme.cache" ]; then
    gtk-update-icon-cache -f -t -q "$ICONOS"
fi
if command -v update-desktop-database >/dev/null; then
    update-desktop-database -q "$DATOS/applications"
fi
