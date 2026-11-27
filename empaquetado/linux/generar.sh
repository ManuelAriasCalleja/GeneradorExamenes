#!/bin/bash
# Genera el paquete de Linux de GeneradorExamenes y lo prueba en cinco distribuciones limpias.
#
#   ./generar.sh [etiqueta-o-commit] [carpeta-de-trabajo]
#
# Por omisión usa HEAD y la carpeta «trabajo» junto a este fichero. El paquete queda en <carpeta-de-trabajo>/salida.
# La versión del nombre sale del CMakeLists.txt de ese commit. Solo se compila lo que está en git (git archive), así
# que los bancos, que están fuera del repositorio, no pueden acabar en el paquete. Hace falta Docker.
# REPO indica otro clon del proyecto (por omisión, este, en el que está el script). Los ficheros de extra, imagen y
# prueba son los de esta carpeta, no los del commit.
set -euo pipefail

AQUI=$(cd "$(dirname "$0")" && pwd)
REPO=${REPO:-$(cd "$AQUI/../.." && pwd)}
REF=${1:-HEAD}
T=${2:-$AQUI/trabajo}
DISTROS=(ubuntu:22.04 ubuntu:24.04 debian:12 debian:trixie fedora:43)

COMMIT=$(git -C "$REPO" rev-parse --verify "$REF^{commit}")
mkdir -p "$T"
rm -rf "$T/src" "$T/salida" "$T/resultado" "$T/registros"
mkdir -p "$T/src" "$T/salida" "$T/resultado" "$T/registros"
chmod 777 "$T/resultado"
git -C "$REPO" archive "$COMMIT" | tar -x -C "$T/src"
echo "Código: $COMMIT"

docker build -q -t generador-paquete:22.04 "$AQUI/imagen" >/dev/null

echo 'Compilando y pasando las pruebas…'
if ! docker run --rm --network none --user "$(id -u):$(id -g)" -e HOME=/tmp \
        -v "$T/src:/src:ro" -v "$AQUI/extra:/extra:ro" -v "$T/salida:/salida" \
        generador-paquete:22.04 /extra/construir.sh > "$T/registros/construir.txt" 2>&1; then
    tail -20 "$T/registros/construir.txt"
    echo "FALLA LA CONSTRUCCIÓN. Registro: $T/registros/construir.txt"
    exit 1
fi
grep -E 'tests passed|tests failed' "$T/registros/construir.txt"

echo 'Probando en las distribuciones…'
for img in "${DISTROS[@]}"; do
    e=${img/:/-}
    docker run --rm -v "$T/salida:/salida:ro" -v "$AQUI/prueba:/prueba:ro" -v "$T/resultado:/resultado" \
        -v "$T/src/recursos:/ejemplo:ro" "$img" bash /prueba/probar.sh "$e" > "$T/registros/$e.log" 2>&1 &
done
wait

fallos=0
for img in "${DISTROS[@]}"; do
    e=${img/:/-}
    linea=$(grep -E "^$e " "$T/registros/$e.log" || true)
    echo "${linea:-$e: sin resultado}"
    case "$linea" in
        *"faltan=[] comprobar=0 ventana=si sigue_abierto=si banco_creado=si lanzador=si"*) ;;
        *) fallos=$((fallos + 1)) ;;
    esac
done

if [ "$fallos" -ne 0 ]; then
    echo "FALLA EN $fallos DISTRIBUCIONES. Registros: $T/registros"
    exit 1
fi
echo "Paquete correcto en $T/salida:"
ls -la "$T/salida" | tail -n +4 | awk '{print $5, $NF}'
