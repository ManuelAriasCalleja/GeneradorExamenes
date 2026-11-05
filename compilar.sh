#!/bin/sh
# Compila el programa en Linux (en Release, en build/), pasa las pruebas automáticas y lo instala para el usuario
# actual: el ejecutable en ~/.local/bin, el lanzador en el menú del escritorio (Cinnamon, GNOME, KDE...) y en el
# escritorio, y su icono. Si las pruebas fallan, no instala nada.
#
#   ./compilar.sh                        compila, pasa las pruebas e instala en ~/.local
#   ./compilar.sh --prefijo /usr/local   instala en otro sitio (si no se puede escribir en él, con sudo)
#   ./compilar.sh --sin-pruebas          no compila ni pasa las pruebas
#   ./compilar.sh --sin-instalar         solo compila (y pasa las pruebas)
#   ./compilar.sh --sin-escritorio       no pone el lanzador en el escritorio (si ya estaba, lo actualiza)
#   ./compilar.sh --desinstalar          quita lo instalado la última vez (según build/install_manifest.txt)
#
# Requisitos (Debian/Ubuntu): sudo apt install qt6-base-dev libqt6sql6-sqlite cmake g++ (y, opcional, qt6-pdf-dev).
set -e
cd "$(dirname "$0")"
PROYECTO=$(pwd)
COMPILACION=$PROYECTO/build
PREFIJO=$HOME/.local
PRUEBAS=ON
INSTALAR=1
DESINSTALAR=0
EN_ESCRITORIO=1

uso() {
    sed -n '2,13s/^# \{0,1\}//p' "$PROYECTO/compilar.sh"
}

while [ $# -gt 0 ]; do
    case $1 in
        --prefijo)
            [ $# -ge 2 ] || { echo "Falta la carpeta después de --prefijo." >&2; exit 2; }
            PREFIJO=$2; shift 2 ;;
        --prefijo=*) PREFIJO=${1#*=}; shift ;;
        --sin-pruebas) PRUEBAS=OFF; shift ;;
        --sin-instalar) INSTALAR=0; shift ;;
        --sin-escritorio) EN_ESCRITORIO=0; shift ;;
        --desinstalar) DESINSTALAR=1; shift ;;
        -h|--help|--ayuda) uso; exit 0 ;;
        *) echo "Opción desconocida: $1" >&2; uso >&2; exit 2 ;;
    esac
done

# Ruta absoluta del prefijo, para que el lanzador apunte bien al ejecutable.
case $PREFIJO in
    /*) ;;
    *) PREFIJO=$PROYECTO/$PREFIJO ;;
esac

DATOS=${XDG_DATA_HOME:-$HOME/.local/share}
ESCRITORIO=$(xdg-user-dir DESKTOP 2>/dev/null || echo "$HOME/Desktop")/GeneradorExamenes.desktop

# Pone al día las cachés de iconos y de lanzadores de la carpeta de datos (share/) del prefijo. Cambiar la fecha de
# las carpetas hace que el escritorio (y el menú de Cinnamon) vuelva a leerlas sin cerrar la sesión.
actualizar_caches() {
    $SUDO touch "$1/icons/hicolor" "$1/applications" 2>/dev/null || true
    if [ -f "$1/icons/hicolor/icon-theme.cache" ] && command -v gtk-update-icon-cache >/dev/null; then
        $SUDO gtk-update-icon-cache -f -t -q "$1/icons/hicolor" || true
    fi
    if command -v update-desktop-database >/dev/null; then
        $SUDO update-desktop-database -q "$1/applications" 2>/dev/null || true
    fi
}

SUDO=
if [ $DESINSTALAR = 1 ]; then
    MANIFIESTO=$COMPILACION/install_manifest.txt
    if [ ! -f "$MANIFIESTO" ]; then
        echo "No hay $MANIFIESTO: no consta que se haya instalado desde esta carpeta." >&2
        exit 1
    fi
    # El prefijo de la instalación, por la ruta del ejecutable (…/bin/GeneradorExamenes).
    EJECUTABLE=$(grep '/GeneradorExamenes$' "$MANIFIESTO" | head -n 1)
    [ -n "$EJECUTABLE" ] && PREFIJO=${EJECUTABLE%/*/*}
    # Las líneas del manifiesto son rutas absolutas; si alguna no se puede borrar sin permisos, con sudo.
    while IFS= read -r fichero || [ -n "$fichero" ]; do  # la última línea no acaba en salto de línea
        if [ -e "$fichero" ] || [ -L "$fichero" ]; then
            echo "Quitando $fichero"
            rm -f "$fichero" 2>/dev/null || sudo rm -f "$fichero"
        fi
    done < "$MANIFIESTO"
    # La copia del lanzador del escritorio, solo si es la de lo instalado (y no la de instalar_lanzador.sh).
    if [ -f "$ESCRITORIO" ] && grep -q "^Exec=\"$PREFIJO/" "$ESCRITORIO"; then
        rm -f "$ESCRITORIO"
    fi
    rm -f "$MANIFIESTO" 2>/dev/null || sudo rm -f "$MANIFIESTO"
    [ -w "$PREFIJO/share" ] || SUDO=sudo
    actualizar_caches "$PREFIJO/share"
    echo "Desinstalado. La configuración (~/.config/UNED/) y los bancos de preguntas no se han tocado."
    exit 0
fi

if ! command -v cmake >/dev/null; then
    echo "Falta CMake. En Debian/Ubuntu: sudo apt install qt6-base-dev libqt6sql6-sqlite cmake g++" >&2
    exit 1
fi

NUCLEOS=$(nproc 2>/dev/null || echo 2)

echo "== Configurando"
cmake -S "$PROYECTO" -B "$COMPILACION" -DCMAKE_BUILD_TYPE=Release -DPRUEBAS=$PRUEBAS

echo "== Compilando"
cmake --build "$COMPILACION" --parallel "$NUCLEOS"

if [ $PRUEBAS = ON ]; then
    echo "== Pasando las pruebas"
    ctest --test-dir "$COMPILACION" --output-on-failure
fi

if [ $INSTALAR = 0 ]; then
    echo "Compilado: $COMPILACION/GeneradorExamenes"
    exit 0
fi

# sudo solo si no se puede escribir en el prefijo.
if ! mkdir -p "$PREFIJO" 2>/dev/null || [ ! -w "$PREFIJO" ]; then
    SUDO=sudo
fi
echo "== Instalando en $PREFIJO"
$SUDO cmake --install "$COMPILACION" --prefix "$PREFIJO"

# El lanzador del escritorio. Si ya estaba, se sobrescribe sin sustituir el fichero, para que conserve la marca de «de
# confianza» que le pone el gestor de archivos. Si es nuevo, se marca como ejecutable y de confianza (en Nemo, el de
# Cinnamon, y en Nautilus), para que se abra con doble clic sin preguntar.
LANZADOR=$PREFIJO/share/applications/GeneradorExamenes.desktop
if [ -f "$ESCRITORIO" ] && [ -w "$ESCRITORIO" ]; then
    cat "$LANZADOR" > "$ESCRITORIO"
elif [ $EN_ESCRITORIO = 1 ] && [ -d "$(dirname "$ESCRITORIO")" ]; then
    cp "$LANZADOR" "$ESCRITORIO"
    chmod +x "$ESCRITORIO"
    gio set "$ESCRITORIO" metadata::trusted true 2>/dev/null || true
fi
actualizar_caches "$PREFIJO/share"

echo
echo "Instalado: $PREFIJO/bin/GeneradorExamenes"
echo "En el menú está en Educación, como «Generador de exámenes»."
[ -f "$ESCRITORIO" ] && echo "Lanzador en el escritorio: $ESCRITORIO"
case ":$PATH:" in
    *":$PREFIJO/bin:"*) ;;
    *) echo "Aviso: $PREFIJO/bin no está en el PATH; para abrirlo desde la terminal, escriba la ruta completa." ;;
esac
