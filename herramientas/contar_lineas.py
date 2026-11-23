#!/usr/bin/env python3
"""Cuenta las líneas de código del programa, sin las líneas en blanco ni los comentarios.

Uso (en Linux hay otro que no necesita Python y da lo mismo, herramientas/contar_lineas.sh):
    herramientas/contar_lineas.py              el programa (src/) y, aparte, las pruebas (tests/)
    herramientas/contar_lineas.py -f           además, cada fichero
    herramientas/contar_lineas.py src/docx.cpp ficheros o carpetas concretos

Cuenta como código la línea que tiene algo más que espacios y comentarios («//…» y «/*…*/»). Lo que hay dentro
de una cadena no es un comentario, aunque lo parezca («"http://…"»), y las líneas de una cadena de varias líneas
(R"(…)") cuentan todas, también las que están en blanco: son parte del texto que el programa escribe.
"""

import argparse
import re
import sys
from pathlib import Path

RAIZ = Path(__file__).resolve().parent.parent
EXTENSIONES = {".cpp", ".h"}
# Principio de una cadena de varias líneas, justo antes de su comilla: R, u8R, LR… y no el final de un nombre.
PREFIJO_CRUDA = re.compile(r"(?<![A-Za-z0-9_])(?:u8|u|U|L)?R$")


def lineas_de_codigo(texto: str) -> int:
    """Líneas de 'texto' (C++) que tienen código."""
    con_codigo = set()
    linea = 1
    i = 0
    n = len(texto)
    while i < n:
        c = texto[i]
        if c == "\n":
            linea += 1
            i += 1
        elif c.isspace():
            i += 1
        elif texto.startswith("//", i):
            # Hasta el final de la línea (o de la siguiente, si acaba en «\»).
            while i < n and texto[i] != "\n":
                if texto[i] == "\\" and i + 1 < n and texto[i + 1] == "\n":
                    linea += 1
                    i += 1
                i += 1
        elif texto.startswith("/*", i):
            fin = texto.find("*/", i + 2)
            fin = n if fin < 0 else fin + 2
            linea += texto.count("\n", i, fin)
            i = fin
        elif c == '"' and PREFIJO_CRUDA.search(texto, max(0, i - 3), i):
            # Cadena de varias líneas: R"delimitador( … )delimitador"
            abre = texto.find("(", i)
            cierre = ")" + texto[i + 1:abre] + '"'
            fin = texto.find(cierre, abre)
            fin = n if fin < 0 else fin + len(cierre)
            con_codigo.update(range(linea, linea + texto.count("\n", i, fin) + 1))
            linea += texto.count("\n", i, fin)
            i = fin
        elif c == '"' or (c == "'" and not (i > 0 and texto[i - 1].isalnum())):
            # Cadena o carácter (la comilla simple tras un dígito separa cifras: 1'000'000).
            con_codigo.add(linea)
            i += 1
            while i < n and texto[i] != c and texto[i] != "\n":
                i += 2 if texto[i] == "\\" else 1
            i += 1 if i < n and texto[i] == c else 0
        else:
            con_codigo.add(linea)
            i += 1
    return len(con_codigo)


def miles(n: int) -> str:
    """El número con el punto de los miles («13.471»), alineado a la derecha."""
    return f"{n:,}".replace(",", ".").rjust(9)


def ficheros_de(ruta: Path) -> list[Path]:
    if ruta.is_dir():
        return sorted(f for f in ruta.rglob("*") if f.suffix in EXTENSIONES and f.is_file())
    return [ruta]


def contar(rutas: list[Path], por_fichero: bool) -> int:
    """Escribe la cuenta de cada ruta (y de sus ficheros, si se pide) y devuelve el total."""
    total = 0
    for ruta in rutas:
        ficheros = ficheros_de(ruta)
        cuentas = [(f, lineas_de_codigo(f.read_text(encoding="utf-8"))) for f in ficheros]
        suma = sum(lineas for _, lineas in cuentas)
        total += suma
        if por_fichero and ruta.is_dir():
            for f, lineas in sorted(cuentas, key=lambda par: -par[1]):
                print(f"{miles(lineas)}  {nombre(f)}")
        aclaracion = f"  ({len(ficheros)} ficheros)" if ruta.is_dir() else ""
        print(f"{miles(suma)}  {nombre(ruta)}{aclaracion}")
    return total


def nombre(ruta: Path) -> str:
    """La ruta, relativa al proyecto si está dentro de él."""
    absoluta = ruta.resolve()
    if absoluta.is_relative_to(RAIZ):
        relativa = absoluta.relative_to(RAIZ)
        return f"{relativa}/" if absoluta.is_dir() else str(relativa)
    return str(ruta)


def main() -> int:
    opciones = argparse.ArgumentParser(description="Cuenta las líneas de código, sin las líneas en blanco ni los comentarios.")
    opciones.add_argument("rutas", nargs="*", type=Path, help="ficheros o carpetas (si no se dice, src/ y tests/)")
    opciones.add_argument("-f", "--ficheros", action="store_true", help="la cuenta de cada fichero, de mayor a menor")
    args = opciones.parse_args()
    rutas = args.rutas or [RAIZ / "src", RAIZ / "tests"]
    for ruta in rutas:
        if not ruta.exists():
            print(f"No existe {ruta}", file=sys.stderr)
            return 1
    total = contar(rutas, args.ficheros)
    if len(rutas) > 1:
        print(f"{miles(total)}  total")
    return 0


if __name__ == "__main__":
    sys.exit(main())
