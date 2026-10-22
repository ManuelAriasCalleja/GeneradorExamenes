#!/bin/sh
# Instala para el usuario actual el lanzador del programa en el menú del escritorio de Linux (Cinnamon, GNOME,
# KDE...): el .desktop en ~/.local/share/applications y el icono en el tema hicolor de ~/.local/share/icons.
# El lanzador apunta al ejecutable de build/, así que hay que compilar antes. Con --quitar, lo desinstala.
set -e
cd "$(dirname "$0")/.."
PROYECTO=$(pwd)
DATOS=${XDG_DATA_HOME:-$HOME/.local/share}
LANZADOR=$DATOS/applications/GeneradorExamenesFI.desktop
ICONOS=$DATOS/icons/hicolor
NOMBRE_ICONO=generador-examenes-fi
TAMANOS="16 24 32 48 64 128 256"

if [ "$1" = "--quitar" ]; then
    rm -f "$LANZADOR"
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
    # StartupWMClass es la clase de las ventanas del programa (su nombre de aplicación en Qt): así el panel
    # agrupa la ventana con el lanzador.
    cat > "$LANZADOR" <<FIN
[Desktop Entry]
Type=Application
Name=Generador de exámenes FI
GenericName=Generador de exámenes tipo test
Comment=Exámenes tipo test de Fundamentos de Informática (Curso de Acceso, UNED)
Exec="$PROYECTO/build/GeneradorExamenesFI"
Icon=$NOMBRE_ICONO
Terminal=false
Categories=Education;
StartupWMClass=GeneradorExamenesFI
FIN
fi

# La caché de iconos solo se actualiza si ya existe; si no, el escritorio lee las carpetas directamente.
if [ -f "$ICONOS/icon-theme.cache" ]; then
    gtk-update-icon-cache -f -t -q "$ICONOS"
fi
if command -v update-desktop-database >/dev/null; then
    update-desktop-database -q "$DATOS/applications"
fi
