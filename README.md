# Generador de exámenes de Fundamentos de Informática

Programa en C++/Qt 6 que genera exámenes tipo test eligiendo al azar preguntas de una base de datos SQLite,
con el número de preguntas que se indique de cada tema.

## Compilar y ejecutar

Requisitos: Qt 6 (módulos Widgets, Sql con el driver SQLite y PrintSupport), CMake ≥ 3.19 y un compilador C++17.
En Debian/Ubuntu: `sudo apt install qt6-base-dev libqt6sql6-sqlite cmake g++`.

```sh
cmake -S . -B build
cmake --build build
./build/GeneradorExamenesFI            # o: ./build/GeneradorExamenesFI otra_base.db
```

Para tenerlo en el menú del escritorio (Cinnamon, GNOME, KDE…) con su icono: `herramientas/instalar_lanzador.sh`
(con `--quitar`, se desinstala). El lanzador apunta a `build/GeneradorExamenesFI`.

En Windows se compila igual con CMake (o desde Qt Creator), con Qt 6 para MinGW o MSVC. Sale como aplicación
gráfica, sin ventana de consola, y el `.exe` lleva el icono (`recursos/icono.rc`). Para usarlo en otro
ordenador hay que copiar a su lado las bibliotecas de Qt con `windeployqt GeneradorExamenesFI.exe`.

Las preguntas no están en git (ver más abajo «Preguntas y git»): para tenerlas hay que copiar aparte la
carpeta `datos/`. Sin `datos/preguntas_fi.json` el programa compila igual, pero no lleva las preguntas
iniciales y una base de datos nueva empieza vacía.

## Uso

**Generar examen**
- Convocatoria (Febrero, Junio, Septiembre…) y año: van en la cabecera («Junio 2026»), en el pie de cada página
  y en el nombre de los ficheros.
- Lista de exámenes de la convocatoria (Nacional Original, Nacional Reserva…) con casillas: se genera un examen
  por cada nombre marcado, en el orden de la lista. La lista se mantiene de un año a otro; los nombres se
  añaden, se quitan, se cambian (doble clic) y se ordenan (arrastrando). El pie de cada página lleva el nombre
  del examen, la convocatoria y el año: «Nacional Original · Junio 2026: 1 / 2».
- Número de preguntas de cada tema (los botones T1–T4, T5–T8 y T1–T8 ponen el reparto habitual
  3-2-3-2 / 2-3-2-3). Junto a cada tema se ve cuántas preguntas distintas hay. Al abrir el programa por
  primera vez se propone T1–T4 (3-2-3-2, 10 preguntas), como en los exámenes de febrero de 2026; después
  se recuerda el último reparto usado.
- Puntuación automática (10 puntos en total y cada fallo resta el 30 %: +1/−0,30 con 10 preguntas,
  +0,50/−0,15 con 20) o manual.
- Exportar a PDF, a HTML editable en Word/LibreOffice o a LaTeX: se abre un diálogo donde se elige la
  carpeta y el nombre del fichero de cada examen (por defecto `2026_Junio_Nacional_Original.pdf`…) y se
  exportan todos de una vez; también se puede marcar «Todos los exámenes en un fichero». Las soluciones en PDF
  (tabla de estructura por temas como la de las instrucciones, plantilla de respuestas y exámenes con la
  solución marcada) se exportan igual, por examen (`2026_Junio_soluciones_Nacional_Original.pdf`…) o todas
  juntas.
  También se puede imprimir o hacer el examen en pantalla y corregirlo («Practicar»).
- Cada examen cabe en una página por cada 10 preguntas (un examen de 10 preguntas, en un folio). Si con la
  maqueta normal no cabe, el programa la compacta por niveles: opciones en dos columnas (a c / b d) cuando
  así ocupan menos líneas, menos separación entre preguntas y letra algo menor (hasta 9 pt). El nivel usado
  se indica junto a la semilla y vale para todos los exámenes de la convocatoria, la vista previa y las tres
  exportaciones; si ni así cabe, se avisa.
- *Exportar LaTeX…* guarda `.tex` para pdflatex (y las imágenes en una carpeta `<nombre>_imagenes` al
  lado). Con «Mostrar soluciones» marcado incluye también la estructura, la plantilla de respuestas y las
  soluciones marcadas. Hay que compilarlo dos veces para que salga el «página / total» del pie:
  `pdflatex examen.tex && pdflatex examen.tex`.
- La semilla permite volver a generar exactamente los mismos exámenes.

**Reglas al elegir preguntas**
- Las variantes de una misma pregunta (3a, 3b…) nunca salen juntas en un examen; tampoco las
  pseudorrepetidas entre temas (T3-5 ≈ T1-7, T3-11 ≈ T2-4).
- Opcionalmente, no se repiten preguntas entre los exámenes de la convocatoria (si no hay
  suficientes, se avisa).
- Las preguntas anuladas u obsoletas no salen nunca. Las marcadas para revisar se pueden excluir.
- Por defecto las preguntas se ordenan por tema y número, como en los exámenes oficiales.

**Banco de preguntas**: buscar, filtrar, crear, editar, duplicar como variante y eliminar preguntas;
menú *Editar → Temas* para añadir o renombrar temas. En el enunciado se puede escribir código entre
líneas ```` ``` ```` y filas de tabla como `| celda | celda |`; también se puede añadir una imagen.

## Ficheros

| Ruta | Contenido |
|---|---|
| `datos/preguntas.db` | Base de datos SQLite (tablas `temas` y `preguntas`). Es la que abre el programa por defecto. |
| `datos/preguntas_fi.json` | Las 347 preguntas extraídas de `FI_preguntas_examen_base_de_datos_total_ver_11.docx`. Va incluido en el ejecutable: si se abre una base de datos vacía, se rellena con él. |
| `datos/imagenes/` | Diagramas de las preguntas T4-14a, 14b y 14c (recortados del documento). |
| `datos/banco2/` | **Banco 2**: propuesta de 1500 preguntas nuevas, separada del banco actual y pendiente de revisión del equipo docente (base de datos propia, .docx para revisar e informes). Ver `datos/banco2/LEEME.md`. |
| `PENDIENTE.md` | Mejoras aceptadas que quedan por hacer. |
| `herramientas/importar_docx.py` | Convierte el .docx de preguntas en el JSON. |
| `herramientas/instalar_lanzador.sh` | Instala (o quita) el lanzador del programa en el menú del escritorio de Linux. |
| `recursos/` | Foto del autor para el diálogo *Ayuda → Acerca de* e icono de la aplicación (`icono.svg`, y `icono_pequeno.svg` para 16 y 24 píxeles). Los PNG de `recursos/icono/` y el `icono.ico` del ejecutable de Windows se generan con `recursos/generar_iconos.sh`. |
| `src/` | Código fuente del programa. |

## Preguntas y git

El repositorio se publica en Internet y no puede contener preguntas de ningún banco. Por eso `datos/` está en
`.gitignore` entera (bases de datos, JSON, imágenes, banco 2 con sus informes y la guía de estilo de redacción
de las preguntas) y también los `.db`, `.docx` y los exámenes exportados. La carpeta `datos/` se copia aparte
de un ordenador a otro.

## Actualizar desde una nueva versión del .docx

```sh
python3 herramientas/importar_docx.py ../FI_preguntas_..._ver12.docx datos/preguntas_fi.json --imagenes datos/imagenes
```

y en el programa *Archivo → Importar preguntas (JSON)…* (añadir/actualizar o reemplazar todo).
El script avisa de preguntas sin cuatro opciones o sin respuesta y de dibujos sin imagen.
Las etiquetas del documento se interpretan así: `old` → ya salió en exámenes; `ANULADA`/`OBSOLETA` → no
sale; `MEJORAR`, `REFINAR`, `DEFINIR …` → marcada para revisar; `PSEUDORREPETIDA … TEMA1 preg 7` → enlace
con T1-7.
