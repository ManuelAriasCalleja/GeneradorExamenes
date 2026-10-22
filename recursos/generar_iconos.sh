#!/bin/sh
# Genera los PNG del icono (recursos/icono/) a partir de los SVG y, con ellos, el icono.ico del ejecutable de
# Windows. Necesita Inkscape e ImageMagick. Para 16 y 24 píxeles se usa la versión simplificada; el resto sale
# de icono.svg.
set -e
cd "$(dirname "$0")"
mkdir -p icono
for n in 16 24; do
    inkscape icono_pequeno.svg -w $n -h $n -o icono/icono_$n.png
done
for n in 32 48 64 128 256; do
    inkscape icono.svg -w $n -h $n -o icono/icono_$n.png
done
magick icono/icono_16.png icono/icono_24.png icono/icono_32.png icono/icono_48.png icono/icono_64.png \
       icono/icono_128.png icono/icono_256.png icono.ico
