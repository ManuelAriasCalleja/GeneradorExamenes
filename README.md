# Generador de exámenes de Fundamentos de Informática

Programa en C++/Qt 6 que genera exámenes tipo test eligiendo al azar preguntas de una base de datos SQLite,
con el número de preguntas que se indique de cada tema.

## Compilar y ejecutar

Requisitos: Qt 6 (módulos Widgets, Sql con el driver SQLite y PrintSupport), CMake ≥ 3.19 y un compilador C++17.
En Debian/Ubuntu: `sudo apt install qt6-base-dev libqt6sql6-sqlite cmake g++`.

```sh
cmake -S . -B build
cmake --build build
./build/GeneradorExamenesFI            # o: ./build/GeneradorExamenesFI ruta/del/banco.db
```

Las preguntas no están en el repositorio (ver «Bancos de preguntas»). El programa abre el último banco de
preguntas que se abrió; la primera vez, o si se ha movido, pregunta cuál abrir o si crear uno nuevo. La ruta
del último banco y las opciones se guardan en un fichero de configuración:
`~/.config/UNED-FI/GeneradorExamenesFI.conf` en Linux y `%APPDATA%\UNED-FI\GeneradorExamenesFI.ini` en Windows
(el diálogo *Ayuda → Acerca de* dice cuál es).

Para tenerlo en el menú del escritorio (Cinnamon, GNOME, KDE…) con su icono: `herramientas/instalar_lanzador.sh`
(con `--quitar`, se desinstala). El lanzador apunta a `build/GeneradorExamenesFI`; si hay una copia en el
escritorio, también la actualiza. Si se mueve el proyecto, hay que volver a ejecutarlo.

En Windows se compila igual con CMake (o desde Qt Creator), con Qt 6 para MinGW o MSVC. Sale como aplicación
gráfica, sin ventana de consola, y el `.exe` lleva el icono (`recursos/icono.rc`).

Para llevarlo a otros ordenadores con Windows, se prepara un paquete (compilado en Release):

```bat
cmake --build build --target paquete
```

Deja `build\GeneradorExamenesFI-<versión>-windows.zip`, con una carpeta que contiene el programa, las bibliotecas de Qt
que necesita (las copia `windeployqt`, con las traducciones al castellano de los diálogos), las del compilador
(MinGW o Visual Studio) y un `LEEME.txt` para quien lo reciba (`recursos/LEEME_windows.txt`). Se descomprime y
funciona sin instalar nada; no lleva preguntas. Desde Qt Creator: *Proyectos → Compilar → Pasos de compilación*,
marcar el objetivo `paquete`. Para comprobar que no falta nada, conviene probarlo en un ordenador sin Qt.

### Pruebas automáticas

```sh
ctest --test-dir build --output-on-failure
```

Están en `tests/`, con Qt Test (incluido en `qt6-base-dev`), y tardan unos segundos. Comprueban el generador
(reparto por tema, variantes y pseudorrepetidas, reparto de la correcta, preguntas ya usadas), el comprobador de
estilo, el formato de los enunciados, la base de datos (JSON, copia de seguridad, transacciones, historial, temas),
la maqueta (que un examen de 10 preguntas quepa en un folio, y la versión con letra grande) y las exportaciones, las
parecidas, el cuestionario de Moodle, los recorridos principales de la interfaz y la ayuda (que tenga los apartados a
los que lleva cada ventana y que sus enlaces lleven a algún sitio). Usan preguntas inventadas (`tests/datos_prueba.h`); la de los bancos reales (`prueba_bancos_reales`)
trabaja con una copia de cada uno y se salta si no está el enlace `asignatura`. Para no compilarlas:
`cmake -S . -B build -DPRUEBAS=OFF`.

## Uso

*Ayuda → Manual de uso* (o **F1**) abre el manual del programa por el apartado de la ventana en la que se está; los
diálogos tienen además un botón «Ayuda». El manual es `recursos/ayuda.html` y va dentro del ejecutable: si cambia lo
que se ve o se hace en el programa, hay que actualizarlo.

**Generar examen**
- Convocatoria (Febrero, Junio, Septiembre…) y año: van en la cabecera («Junio 2026»), en el pie de cada página
  y en el nombre de los ficheros. Al abrir el programa se propone la próxima convocatoria según la fecha: la primera
  de febrero, junio y septiembre cuyo mes aún no ha llegado (el 27 de octubre de 2026, Febrero 2027).
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
  También se puede imprimir o hacer el examen en pantalla y corregirlo («Practicar este examen…»): se elige la
  duración (se recuerda de una vez a otra) y el reloj empieza a contar al pulsar «Empezar».
- Cada examen cabe en una página por cada 10 preguntas (un examen de 10 preguntas, en un folio). Si con la
  maqueta normal no cabe, el programa la compacta por niveles: opciones en dos columnas (a c / b d) cuando
  así ocupan menos líneas, menos separación entre preguntas y letra algo menor (hasta 9 pt). El nivel usado
  se indica junto a la semilla y vale para todos los exámenes de la convocatoria, la vista previa y las tres
  exportaciones; si ni así cabe, se avisa.
- «Letra grande», junto a «Mostrar soluciones», es la versión adaptada para estudiantes con discapacidad
  visual, lo contrario de la maqueta compacta: letra de 16 pt (también la cabecera y las instrucciones), los
  enunciados en negrita sin cursiva, una opción por línea, más separación entre preguntas e imágenes más
  grandes, sin límite de páginas. Vale para la
  vista, la impresión y las tres exportaciones, y los ficheros llevan `_letra_grande` en el nombre
  (`2026_Junio_letra_grande_Nacional_Original.pdf`) para no pisar los normales.
- *Exportar LaTeX…* guarda `.tex` para pdflatex (y las imágenes en una carpeta `<nombre>_imagenes` al
  lado). Con «Mostrar soluciones» marcado incluye también la estructura, la plantilla de respuestas y las
  soluciones marcadas. Hay que compilarlo dos veces para que salga el «página / total» del pie:
  `pdflatex examen.tex && pdflatex examen.tex`.
- La semilla permite volver a generar exactamente los mismos exámenes, siempre que no hayan cambiado ni el banco
  ni el historial. Para recuperar los de una convocatoria pasada, mejor el historial.
- *Registrar convocatoria…* guarda en el historial (dentro de la base de datos) los exámenes generados, con una
  copia de sus preguntas tal como salieron y el orden de las opciones. Si ya había exámenes registrados con el
  mismo nombre, convocatoria y año, se sustituyen. En *Archivo → Historial de convocatorias* se puede cargar una
  convocatoria (o un solo examen) en la ventana para verla, exportarla, imprimirla o practicarla tal como salió,
  aunque después se hayan corregido sus preguntas, o eliminarla del historial. La ficha de cada pregunta, en el
  banco, dice en qué exámenes registrados ha salido.

**Reglas al elegir preguntas**
- Las variantes de una misma pregunta (3a, 3b…) nunca salen juntas en un examen; tampoco las
  pseudorrepetidas entre temas (T3-5 ≈ T1-7, T3-11 ≈ T2-4).
- Opcionalmente, no se repiten preguntas entre los exámenes de la convocatoria (si no hay
  suficientes, se avisa).
- Opcionalmente, se dejan para el final las preguntas que ya han salido, con sus variantes y pseudorrepetidas:
  primero se eligen las nunca usadas, luego las marcadas «old» (si se marca la opción) y por último las de las
  últimas N convocatorias registradas, de la más antigua a la más reciente. Si no hay bastantes, se avisa. Las
  convocatorias se ordenan por año y por mes (si se llaman como un mes: Febrero, Junio…).
- Si se marca «Barajar las opciones», en cada examen la correcta sale en a), b), c) y d) por igual (como mucho,
  una de diferencia) y, si se puede, nunca tres veces seguidas en la misma letra. Las preguntas con opciones que
  se refieren a otras («Todas las anteriores», «a y b»…) no se barajan. Sin barajar, la correcta queda donde está
  en el banco, y si en algún examen una letra sale bastante más de lo que le toca (5 de 10, por ejemplo), se
  avisa. La ventana y la hoja de soluciones muestran cuántas veces es correcta cada letra.
- Las preguntas anuladas u obsoletas no salen nunca. Las marcadas para revisar se pueden excluir.
- Por defecto las preguntas se ordenan por tema y número, como en los exámenes oficiales.

**Banco de preguntas**: buscar, filtrar, crear, editar, duplicar como variante y eliminar preguntas;
menú *Editar → Temas* para añadir o renombrar temas. En el enunciado se puede escribir código entre
líneas ```` ``` ```` y filas de tabla como `| celda | celda |`; también se puede añadir una imagen.

Al editar una pregunta, bajo la vista previa se avisa de las pautas de redacción que no cumple: enunciado
acabado en dos puntos (o en «?:» si es una pregunta) y de no más de unos 180 caracteres, negaciones en
mayúsculas, opciones con mayúscula y punto final, opciones del tipo «Todas las anteriores», opciones repetidas,
correcta mucho más larga que las demás, comillas (tipográficas en el texto, rectas en el código) y forma de la
referencia al libro según el tema. Son avisos: al guardar se pide confirmación, pero se puede guardar igual.
*Editar → Revisar el estilo del banco…* revisa todas las preguntas del banco abierto: lista los avisos (todos o
solo los del enunciado, las opciones o la referencia), abre cada pregunta para corregirla y guarda el informe
en un fichero de texto, solo con los códigos de las preguntas y los avisos.

*Archivo → Exportar a Moodle (XML)…* guarda las preguntas activas del banco como cuestionario en formato
«Moodle XML», para que los estudiantes practiquen en el curso virtual: una categoría por tema (`Fundamentos de
Informática/Tema 1: …`), cada pregunta de opción múltiple con una sola respuesta, la referencia al libro y la
explicación como realimentación general, las imágenes incrustadas y, si se marca, las respuestas erróneas
restando el 30 % de la pregunta, como en el examen. Las opciones se barajan en Moodle salvo en las preguntas con
opciones del tipo «Todas las anteriores»; las anuladas y obsoletas no se exportan, y las marcadas para revisar
se pueden excluir. En Moodle se importa desde *Banco de preguntas → Importar*. No hace falta el formato GIFT: el
XML admite imágenes, categorías y realimentación, y GIFT no.

El editor muestra también las preguntas del banco más parecidas a la que se escribe (de otros números; las
variantes de la misma no cuentan). Al guardar una pregunta nueva con un número que no existe, si se parece mucho
a otras, las enseña con su ficha para compararlas y propone guardarla como la siguiente variante de una de ellas
(si es del mismo tema) o enlazarla como pseudorrepetida (si es de otro), para que nunca salgan juntas en un
examen. El parecido compara trozos de palabras del enunciado y de las opciones; con los dos bancos, la variante
de una pregunta sale entre las tres más parecidas en ocho de cada diez casos. *Duplicar como variante* y las
variantes nuevas toman la siguiente letra libre (si la pregunta existente no tiene letra, cuenta como la «a»).

## Ficheros

| Ruta | Contenido |
|---|---|
| `PENDIENTE.md` | Mejoras aceptadas que quedan por hacer. |
| `herramientas/importar_docx.py` | Convierte el .docx de preguntas en el JSON. |
| `herramientas/instalar_lanzador.sh` | Instala (o quita) el lanzador del programa en el menú del escritorio de Linux. |
| `recursos/` | Manual de uso (`ayuda.html`, el de *Ayuda → Manual de uso*), foto del autor para el diálogo *Ayuda → Acerca de* e icono de la aplicación (`icono.svg`, y `icono_pequeno.svg` para 16 y 24 píxeles). Los PNG de `recursos/icono/` y el `icono.ico` del ejecutable de Windows se generan con `recursos/generar_iconos.sh`. `LEEME_windows.txt` va en el paquete de Windows. |
| `src/` | Código fuente del programa. |
| `tests/` | Pruebas automáticas, con datos inventados. |

## Bancos de preguntas

El repositorio se publica en Internet y no puede contener preguntas de ningún banco. Los bancos están fuera,
en el directorio de la asignatura, y se copian aparte de un ordenador a otro:

| Carpeta | Contenido |
|---|---|
| `BancoPreguntas/` | El banco del equipo docente (ver11): `preguntas.db` (base de datos SQLite, con el historial de convocatorias), `preguntas_fi.json` (las preguntas extraídas del .docx), `imagenes/` (diagramas), `estilo_preguntas.md` (guía de redacción) y `copias/` (copias de seguridad de la base de datos que hace el programa antes de «Reemplazar todo»). |
| `BancoPreguntas2/` | **Banco 2**: propuesta de 1500 preguntas nuevas, separada del banco actual y pendiente de revisión del equipo docente (base de datos propia, .docx para revisar e informes). Ver su `LEEME.md`. |

`asignatura` es un enlace local (fuera de git) a ese directorio, para llegar a los bancos desde el proyecto:
`ln -s "<directorio de la asignatura>" asignatura`. El `.gitignore` excluye además los `.db`, `.docx`, los JSON
de preguntas, los exámenes exportados y los cuestionarios de Moodle (`.xml`), por si alguno se guarda dentro del
proyecto.

## Actualizar desde una nueva versión del .docx

```sh
python3 herramientas/importar_docx.py asignatura/FI_preguntas_..._ver12.docx asignatura/BancoPreguntas/preguntas_fi.json \
        --imagenes asignatura/BancoPreguntas/imagenes
```

y en el programa *Archivo → Importar preguntas (JSON)…* (añadir/actualizar o reemplazar todo). El JSON del que
se importa todo el banco queda **enlazado** con él como su copia en texto: cuando la base de datos y ese JSON
dejan de coincidir (por ejemplo, tras corregir una pregunta en el programa), la barra de estado lo avisa y dice
en qué se diferencian (preguntas distintas, solo en uno de los dos, temas renombrados); pulsando el aviso, o con
*Archivo → Actualizar el JSON del banco…*, se sobrescribe el JSON con el contenido de la base de datos (la
primera vez, si no hay ninguno enlazado, pide cuál). Con *Archivo → Mantener el JSON del banco al día* marcado,
el programa lo reescribe él solo tras cada cambio del banco; nunca toca un JSON que ya no coincida con la base
de datos (por ejemplo, uno recién generado desde el .docx y aún sin importar), solo avisa. El enlace y esta
opción se guardan dentro de la base de datos, con la ruta del JSON relativa a ella. Antes de
reemplazar todo, el programa guarda una copia de seguridad de la base de datos en la carpeta `copias/` junto a
ella, con la fecha y la hora en el nombre (`copias/preguntas_2026-10-22_153045.db`), y al terminar dice dónde la
ha dejado; si la importación falla, la base de datos no cambia y la copia se quita. Para volver atrás, basta con
abrir esa copia (*Archivo → Abrir banco de preguntas…*) o copiarla sobre `preguntas.db`.
El script avisa de preguntas sin cuatro opciones o sin respuesta y de dibujos sin imagen.
Las etiquetas del documento se interpretan así: `old` → ya salió en exámenes; `ANULADA`/`OBSOLETA` → no
sale; `MEJORAR`, `REFINAR`, `DEFINIR …` → marcada para revisar; `PSEUDORREPETIDA … TEMA1 preg 7` → enlace
con T1-7.
