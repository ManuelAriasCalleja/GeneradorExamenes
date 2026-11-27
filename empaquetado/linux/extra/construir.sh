#!/bin/bash
# Dentro del contenedor: compila /src, pasa las pruebas y deja el paquete en /salida.
set -euo pipefail
QT=/opt/Qt/6.8.3/gcc_64
B=/tmp/build
R=/tmp/paquete
P=$R/GeneradorExamenes

cmake -S /src -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QT"
cmake --build "$B" --parallel
ctest --test-dir "$B" --output-on-failure

VERSION=$(sed -n 's/^CMAKE_PROJECT_VERSION:STATIC=//p' "$B/CMakeCache.txt")
mkdir -p "$P"/{bin,lib,translations,iconos,plugins}
cp "$B/GeneradorExamenes" "$P/bin/"
strip "$P/bin/GeneradorExamenes"
patchelf --set-rpath '$ORIGIN/../lib' "$P/bin/GeneradorExamenes"
printf '[Paths]\nPrefix=..\nLibraries=lib\nPlugins=plugins\nTranslations=translations\n' > "$P/bin/qt.conf"

# Complementos de Qt que usa un programa de escritorio.
copiar_plugin() {   # carpeta fichero...
    local d=$1; shift
    mkdir -p "$P/plugins/$d"
    for f in "$@"; do
        for g in "$QT/plugins/$d"/$f; do
            [ -e "$g" ] && cp -L "$g" "$P/plugins/$d/"
        done
    done
    return 0
}
copiar_plugin platforms 'libqxcb.so' 'libqoffscreen.so' 'libqwayland-*.so'
copiar_plugin xcbglintegrations '*.so'
copiar_plugin sqldrivers 'libqsqlite.so'
copiar_plugin imageformats '*.so'
copiar_plugin iconengines '*.so'
copiar_plugin platforminputcontexts '*.so'
copiar_plugin platformthemes '*.so'
copiar_plugin printsupport '*.so'
for d in wayland-decoration-client wayland-graphics-integration-client wayland-shell-integration; do
    [ -d "$QT/plugins/$d" ] && copiar_plugin "$d" '*.so'
done
find "$P/plugins" -type d -empty -delete
for f in qt_es.qm qtbase_es.qm qtbase_en.qm qt_en.qm; do
    [ -e "$QT/translations/$f" ] && cp "$QT/translations/$f" "$P/translations/"
done

# Bibliotecas: las de Qt, ICU y las de X11 que no suelen venir instaladas. glibc, libstdc++, OpenGL, GTK, CUPS o
# Wayland son las del sistema.
incluir='/(libQt6[^/]*|libicu[^/]*|libxcb-(cursor|icccm|image|keysyms|render-util|util|xinerama|xkb|randr|render|shape|shm|sync|xfixes)\.so[^/]*|libxkbcommon(-x11)?\.so[^/]*|libgssapi_krb5\.so[^/]*|libkrb5\.so[^/]*|libkrb5support\.so[^/]*|libk5crypto\.so[^/]*|libcom_err\.so[^/]*|libkeyutils\.so[^/]*|libatomic\.so[^/]*)$'
while :; do
    nuevas=0
    while IFS= read -r f; do
        while IFS= read -r dep; do
            [ -n "$dep" ] || continue
            base=$(basename "$dep")
            if [ ! -e "$P/lib/$base" ]; then
                cp -L "$dep" "$P/lib/$base"
                chmod u+w "$P/lib/$base"
                case "$base" in libQt6*|libicu*) ;; *) patchelf --set-rpath '$ORIGIN' "$P/lib/$base" ;; esac
                nuevas=1
            fi
        done < <(LD_LIBRARY_PATH="$P/lib:$QT/lib" ldd "$f" 2>/dev/null | awk '$2 == "=>" && $3 ~ /^\// {print $3}' | grep -E "$incluir" || true)
    done < <(find "$P/bin/GeneradorExamenes" "$P/plugins" "$P/lib" -type f \( -name 'GeneradorExamenes' -o -name '*.so*' \))
    [ "$nuevas" = 1 ] || break
done
find "$P/plugins" -type f -name '*.so' -exec patchelf --set-rpath '$ORIGIN/../../lib' {} \;
find "$P/lib" -type f -name 'libQt6*' -exec strip --strip-unneeded {} \; 2>/dev/null || true

for n in 16 24 32 48 64 128 256; do cp "/src/recursos/icono/icono_$n.png" "$P/iconos/"; done
cp /src/recursos/icono.svg "$P/iconos/"
cp /src/recursos/GeneradorExamenes.desktop.in "$P/iconos/"
cp /extra/GeneradorExamenes "$P/GeneradorExamenes"
cp /extra/instalar.sh "$P/instalar.sh"
cp /extra/LEEME.txt "$P/LEEME.txt"
chmod 755 "$P/GeneradorExamenes" "$P/instalar.sh" "$P/bin/GeneradorExamenes"

echo '--- dependencias que quedan al sistema (binario y complementos principales)'
for f in "$P/bin/GeneradorExamenes" "$P/plugins/platforms/libqxcb.so" "$P/plugins/sqldrivers/libqsqlite.so"; do
    ldd "$f" | awk '{print $1, $3}' | grep -v "$P" | awk '{print $1}'
done | sort -u | tr '\n' ' '
echo
echo '--- sin resolver en el binario y en xcb/sqlite'
for f in "$P/bin/GeneradorExamenes" "$P/plugins/platforms/libqxcb.so" "$P/plugins/sqldrivers/libqsqlite.so"; do
    ldd "$f" | grep 'not found' || true
done

# Licencias: la carpeta que prepara CMake (Qt, con la LGPL v3) y, en terceros, el fichero copyright del paquete de
# Ubuntu de cada una de las demás bibliotecas que van en lib. ICU viene con Qt (la 73): su licencia es la de la ICU 70
# de Ubuntu, que es la misma. Si alguna no tiene, no se hace el paquete.
cp -r "$B/licencias" "$P/licencias"
mkdir -p "$P/licencias/terceros"
for f in "$P/lib"/*; do
    base=$(basename "$f")
    case "$base" in
        libQt6*) continue ;;
        libicu*) paquete=libicu70 ;;
        *) paquete=$(dpkg -S "*/$base" 2>/dev/null | head -1 | cut -d: -f1 || true) ;;
    esac
    if [ -z "$paquete" ] || [ ! -e "/usr/share/doc/$paquete/copyright" ]; then
        echo "No se encuentra la licencia de $base" >&2
        exit 1
    fi
    cp "/usr/share/doc/$paquete/copyright" "$P/licencias/terceros/$paquete.txt"
done
echo "--- licencias: $(ls "$P/licencias/terceros" | tr '\n' ' ')"

Z="GeneradorExamenes-$VERSION-linux-x86_64.tar.gz"
tar -C "$R" --owner=0 --group=0 --numeric-owner -czf "/salida/$Z" GeneradorExamenes
(cd /salida && sha256sum "$Z" > "$Z.sha256")
ls -la /salida
