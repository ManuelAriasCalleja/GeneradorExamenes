# Revisión de errores

Búsqueda exhaustiva de errores en el código, empezada el 18 de noviembre de 2026. Se hace en tres pasos, de más
barato y fiable a más caro; este documento dice en qué punto está y lo que se ha encontrado, para seguir desde
cualquier ordenador.

## Estado

| Paso | Estado |
|---|---|
| 1. Herramientas automáticas | Hecho: ningún fallo en el código; un descuido en `.gitignore` (arreglado) |
| 2. Sondas dirigidas | En curso: hechas las primeras sondas (`tests/prueba_sondas.cpp`); tres fallos arreglados. Falta: compilar con pdflatex los LaTeX raros, pasar las sondas a fondo y con los detectores |
| 3. Revisión del código por partes | Sugerencia, para cuando acaben 1 y 2 |

## 1. Herramientas automáticas

Sin apenas falsos positivos:

- **Detectores de errores de GCC** (AddressSanitizer y UndefinedBehaviorSanitizer): compilar con ellos y pasar las
  pruebas y la herramienta de las capturas, que recorre la ventana y los diálogos. Encuentran accesos a memoria
  liberada o fuera de rango, fugas y comportamiento indefinido (como el cierre en falso al destruir la ventana que se
  arregló en `bafa613`).
- **clang-tidy**, con las comprobaciones de errores típicos (`bugprone-*`, `clang-analyzer-*`).
- **Todos los avisos del compilador** (`-Wall -Wextra -Wshadow` y otros que señalan errores).

### Resultado (18 de noviembre de 2026)

- **Detectores (ASan + UBSan)**, con la opción nueva de CMake `SANITIZADORES` (ver «Pruebas automáticas» en el
  README), compilando en `build-sanitizadores` en modo Debug:
  - las 13 pruebas: ningún error de memoria ni comportamiento indefinido;
  - la herramienta de las capturas (recorre la ventana, el editor, la exportación, la práctica, el historial, los
    exámenes anteriores, el estilo, Moodle y la ayuda): ninguno;
  - fugas de memoria (LeakSanitizer, comprobado antes que funciona en el equipo): ninguna en ninguna prueba.
- **clang-tidy 19** (`bugprone-*`, `clang-analyzer-*`, `misc-redundant-expression`, `misc-misplaced-const`,
  `readability-misleading-indentation`) sobre todo `src/`: solo dos `switch` sin `default` en
  `ModeloPreguntas::data()` y `headerData()`, que no son fallos (detrás del `switch` se devuelve un valor vacío).
- **Avisos de GCC 14** (`-Wall -Wextra -Wshadow -Wduplicated-cond -Wduplicated-branches -Wlogical-op
  -Wnull-dereference -Wformat=2 -Wimplicit-fallthrough -Wcast-qual -Wuseless-cast -Wredundant-decls`):
  - `src/estilo.cpp`: una condición `a && b || c` sin paréntesis en la regla de la negación en mayúsculas. Se
    agrupaba como se quería; se ha reescrito para que se lea sin dudas (`castellano && (… || … || …)`).
  - `src/basedatos.cpp`: una variable que tapaba a otra del mismo nombre en `diferenciasConJson()`; renombrada.
  - Sin importancia: conversiones a `qsizetype` que en 64 bits no cambian nada, inicializadores de campos que se
    quedan con su valor por defecto y nombres repetidos dentro de lambdas de las pruebas.

## 2. Sondas dirigidas

Pruebas que generan casos al azar (con semillas fijas, para que se puedan repetir) y comprueban que el programa no
falla y cumple sus reglas. Se quedan como pruebas automáticas, con datos inventados:

- JSON con errores generados al azar contra el comprobador y la importación.
- Enunciados raros (``` sin cerrar, tablas desiguales, caracteres especiales) contra la maqueta, el LaTeX, el Word y
  el HTML.
- El generador con muchas semillas y bancos al azar, comprobando sus reglas: que no salgan variantes ni
  pseudorrepetidas juntas, el reparto de la correcta y el número de preguntas.
- Ida y vuelta base de datos → JSON → base de datos con preguntas al azar.

### Las sondas (`tests/prueba_sondas.cpp`)

Se pasan con las demás pruebas (unos 2 segundos). Para buscar más a fondo, con las mismas semillas y más vueltas:
`SONDAS=20 ./build/prueba_sondas`. Cada una:

- `escapeLatex`: textos raros escapados para LaTeX no dejan ningún carácter especial suelto ni «--».
- `maquetaYExportacionesConTextosRaros`: preguntas con textos raros (símbolos de HTML, XML, LaTeX y JSON, Unicode con
  pares sustitutos, marcas combinadas, espacios raros y caracteres de control, código y tablas mal cerrados) en la
  vista (con su paginación), el LaTeX, el Word (todo su XML bien formado), el PDF, Moodle (XML bien formado), las pautas
  de estilo y la ficha, con la maqueta normal, la compacta y la de letra grande, con y sin soluciones.
- `parecidasYExamenesAnteriores`: el índice de parecidas y la lectura de exámenes anteriores con textos raros.
- `jsonEstropeados`: JSON estropeados al azar (bytes quitados, añadidos o cambiados, cortes, trozos repetidos): el
  comprobador no falla, sus problemas dicen líneas del fichero y, si no da errores, la importación trae todas las
  preguntas que ha contado.
- `idaYVueltaDelJson`: bancos al azar guardados en la base de datos se leen igual, su JSON lo acepta el comprobador, y
  al importarlo en otro banco quedan igual y sin diferencias con el JSON.
- `generadorConBancosAlAzar`: bancos y opciones al azar (variantes, pseudorrepetidas, anuladas, para revisar,
  verificadas, «old», usadas en convocatorias, opciones que remiten a otras): solo salen las que pueden, sin variantes
  ni pseudorrepetidas juntas, las pedidas de cada tema (o un aviso), sin repetir entre exámenes (o un aviso), con la
  misma semilla los mismos, las opciones barajadas son una permutación con la correcta en su sitio y repartida; y lo
  mismo tras cambiar preguntas.
- `bloquesDeLosEnunciados`: el reparto en bloques de enunciados raros.

## 3. Revisión del código por partes (sugerencia)

Cuando acaben 1 y 2: revisar el código a mano por partes (base de datos, generador, maqueta y exportaciones,
interfaz), buscando errores de lógica que las herramientas y las sondas no ven. Cada hallazgo se intenta reproducir
con una prueba antes de darlo por bueno, para descartar los falsos. Se puede hacer con varios agentes en paralelo
(uno por parte, y otro que comprueba cada hallazgo), lo que gasta bastantes más tokens: antes de lanzarlo, estimar
cuántos.

## Fallos encontrados

Cada uno con lo que pasaba, cómo se encontró y si está arreglado (con su prueba).

1. **Las carpetas de imágenes exportadas dentro del proyecto no se ignoraban** (arreglado). Al pasar los nombres de
   los ficheros a guiones (`ee66273`), la carpeta de las imágenes del HTML y el LaTeX pasó de `<nombre>_imagenes` a
   `<nombre>-imagenes`, pero `.gitignore` solo ignoraba la forma antigua: si se exportaba dentro del proyecto, git
   las habría propuesto para subir. Encontrado al revisar `.gitignore` para la carpeta de los detectores.
   Arreglado: `.gitignore` ignora las dos.
2. **El XML de Moodle podía no ser válido** (arreglado). Un texto con caracteres de control (por ejemplo U+000B, el
   salto de línea manual de Word, que se cuela al copiar de un documento) pasaba tal cual al XML, que no los admite:
   Moodle no habría podido importar el fichero. Encontrado por `maquetaYExportacionesConTextosRaros`. Arreglado: se
   quitan (`sinCaracteresNoXml()` en `src/utilidades.h`). Prueba: `sinCaracteresQueElXmlNoAdmite` (Moodle).
3. **El comprobador daba por bueno un `\u` mal escrito que la importación rechazaba** (arreglado). Leía las cuatro
   cifras de `\u` con `toUInt()`, que admite espacios, signo y «0x»: `"\u000 "` pasaba la comprobación y al importar
   salía «JSON no válido: invalid escape sequence», sin la línea. Encontrado por `jsonEstropeados`. Arreglado: exige
   cuatro cifras hexadecimales. Pruebas: tres casos nuevos en `erroresDeEscritura` (comprobador).
4. **Al cambiar una pregunta, la correcta podía quedar mal repartida** (arreglado). La nueva iba siempre a la letra de
   la quitada; si la quitada tenía la correcta fija (sus opciones remiten a otras, como «Todas las anteriores», y no se
   barajan), esa letra no la había elegido el reparto, y la nueva podía dejar el examen con la correcta varias veces
   en la misma letra. Encontrado por `generadorConBancosAlAzar` (un examen de dos preguntas con las dos en la c).
   Arreglado: va a la letra de la quitada si es de las que menos salen en el resto del examen (lo es siempre que
   estaba bien repartido, así que lo normal no cambia) y, si no, a una de ellas. Prueba:
   `cambiarUnaFijaReparteLaCorrecta` (generador). Manual y README al día.

### Vistos y no arreglados

- **SQLite quita un U+FEFF al principio de un texto.** Es la marca de orden de bytes del UTF-16, invisible; el
  controlador de Qt pasa los textos a SQLite en UTF-16 y SQLite la toma por una marca y la quita. No se ve, así que da
  igual, salvo en un caso extremo: un texto que sea solo esa marca (una opción, por ejemplo) se queda vacío, y el JSON
  que se exporta después no lo aceptaría el comprobador. No se arregla: nadie escribe una opción que sea solo un
  carácter invisible. Las sondas lo tienen en cuenta.
- **Al cambiar una pregunta por otra que se repite en otro examen, el aviso dice «está también en otro examen de la
  convocatoria»**, no el de la generación («para no repetir entre exámenes»): no es un fallo; la sonda acepta los
  dos.
