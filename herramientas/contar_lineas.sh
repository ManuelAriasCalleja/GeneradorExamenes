#!/bin/sh
# Cuenta las líneas de código del programa, sin las líneas en blanco ni los comentarios. No necesita nada que no
# venga en cualquier Linux (sh, awk, find y sort). Hace lo mismo que contar_lineas.py, que sirve donde haya Python.
#
#   herramientas/contar_lineas.sh              el programa (src/) y, aparte, las pruebas (tests/)
#   herramientas/contar_lineas.sh -f           además, cada fichero
#   herramientas/contar_lineas.sh src/docx.cpp ficheros o carpetas concretos
#
# Cuenta como código la línea que tiene algo más que espacios y comentarios («//…» y «/*…*/»). Lo que hay dentro de
# una cadena no es un comentario, aunque lo parezca («"http://…"»), y las líneas de una cadena de varias líneas
# (R"(…)") cuentan todas, también las que están en blanco: son parte del texto que el programa escribe.

RAIZ=$(cd "$(dirname "$0")/.." && pwd -P)
TAB=$(printf '\t')

por_fichero=no
case "$1" in
    -f | --ficheros)
        por_fichero=si
        shift
        ;;
    -h | --help)
        sed -n '2,7p' "$0" | sed 's/^# \{0,1\}//'
        exit 0
        ;;
esac

# Escribe «líneas de código <tabulador> fichero» de cada fichero (C++) que se le pasa.
contar() {
    awk '
    function terminar() {
        if (fichero != "")
            printf "%d\t%s\n", cuenta, fichero
    }
    FNR == 1 {
        terminar()
        fichero = FILENAME
        cuenta = 0
        estado = ""          # "bloque": dentro de un comentario de varias líneas; "cruda": de una cadena de varias líneas
        sigue_comentario = 0 # la línea anterior era un comentario «//» acabado en «\»
    }
    {
        linea = $0
        n = length(linea)
        i = 1
        codigo = 0
        if (sigue_comentario) {
            sigue_comentario = (linea ~ /\\$/)
            next
        }
        if (estado == "cruda") {
            codigo = 1
            j = index(linea, cierre)
            if (j == 0) {
                cuenta++
                next
            }
            i = j + length(cierre)
            estado = ""
        } else if (estado == "bloque") {
            j = index(linea, "*/")
            if (j == 0)
                next
            i = j + 2
            estado = ""
        }
        while (i <= n) {
            c = substr(linea, i, 1)
            dos = substr(linea, i, 2)
            if (c == " " || c == "\t" || c == "\r" || c == "\f" || c == "\v") {
                i++
            } else if (dos == "//") {
                sigue_comentario = (linea ~ /\\$/)
                break
            } else if (dos == "/*") {
                j = index(substr(linea, i + 2), "*/")
                if (j == 0) {
                    estado = "bloque"
                    break
                }
                i += j + 3
            } else if (c == "\"" && substr(linea, 1, i - 1) ~ /(^|[^A-Za-z0-9_])(u8|u|U|L)?R$/) {
                # Cadena de varias líneas: R"delimitador( … )delimitador"
                codigo = 1
                resto = substr(linea, i + 1)
                p = index(resto, "(")
                cierre = ")" substr(resto, 1, p - 1) "\""
                j = index(substr(resto, p + 1), cierre)
                if (j == 0) {
                    estado = "cruda"
                    break
                }
                i += p + j + length(cierre)
            } else if (c == "\"" || (c == "\047" && !(i > 1 && substr(linea, i - 1, 1) ~ /[A-Za-z0-9]/))) {
                # Cadena o carácter (la comilla simple tras un dígito separa cifras: 1\0471000).
                codigo = 1
                i++
                while (i <= n && substr(linea, i, 1) != c)
                    i += (substr(linea, i, 1) == "\\") ? 2 : 1
                i++
            } else {
                codigo = 1
                i++
            }
        }
        cuenta += codigo
    }
    END { terminar() }
    ' "$@"
}

# Lee «líneas <tabulador> fichero» y escribe la cuenta de cada fichero (si se pide) y la de la ruta, con el punto de los
# miles y los nombres relativos al proyecto. El segundo argumento dice qué es la ruta: «si» (una carpeta), «no» (un
# fichero) o «total» (la línea del total, que ya viene sumada).
resumir() {
    sort -t "$TAB" -k1,1nr -k2,2 | awk -F '\t' -v raiz="$RAIZ/" -v ruta="$1" -v es_carpeta="$2" -v por_fichero="$por_fichero" '
    function miles(n,    s, r) {
        s = sprintf("%d", n)
        r = ""
        while (length(s) > 3) {
            r = "." substr(s, length(s) - 2) r
            s = substr(s, 1, length(s) - 3)
        }
        return sprintf("%9s", s r)
    }
    function nombre(f) {
        return index(f, raiz) == 1 ? substr(f, length(raiz) + 1) : f
    }
    $0 == "" { next }
    es_carpeta == "total" {
        printf "%s  %s\n", miles($1), $2
        exit
    }
    {
        suma += $1
        ficheros++
        if (por_fichero == "si" && es_carpeta == "si")
            printf "%s  %s\n", miles($1), nombre($2)
    }
    END {
        if (es_carpeta == "si")
            printf "%s  %s/  (%d ficheros)\n", miles(suma), nombre(ruta), ficheros
        else if (es_carpeta == "no")
            printf "%s  %s\n", miles(suma), nombre(ruta)
    }'
}

# «líneas <tabulador> fichero» de los ficheros .cpp y .h de una carpeta.
contar_carpeta() {
    lista=$(find "$1" -type f \( -name '*.cpp' -o -name '*.h' \))
    [ -z "$lista" ] && return
    (
        IFS='
'
        set -f
        # shellcheck disable=SC2086
        contar $lista
    )
}

[ $# -eq 0 ] && set -- "$RAIZ/src" "$RAIZ/tests"
num_rutas=$#
total=0
for ruta in "$@"; do
    if [ -d "$ruta" ]; then
        absoluta=$(cd "$ruta" && pwd -P)
        cuentas=$(contar_carpeta "$absoluta")
        printf '%s\n' "$cuentas" | resumir "$absoluta" si
    elif [ -f "$ruta" ]; then
        absoluta=$(cd "$(dirname "$ruta")" && pwd -P)/$(basename "$ruta")
        cuentas=$(contar "$absoluta")
        printf '%s\n' "$cuentas" | resumir "$absoluta" no
    else
        echo "No existe $ruta" >&2
        exit 1
    fi
    suma=$(printf '%s\n' "$cuentas" | awk -F '\t' '{ s += $1 } END { printf "%d\n", s }')
    total=$((total + suma))
done
if [ "$num_rutas" -gt 1 ]; then
    printf '%s\ttotal\n' "$total" | resumir total total
fi
