# Revisión de errores

Búsqueda exhaustiva de errores en el código, empezada el 18 de noviembre de 2026. Se hace en tres pasos, de más
barato y fiable a más caro; este documento dice en qué punto está y lo que se ha encontrado, para seguir desde
cualquier ordenador.

## Estado

| Paso | Estado |
|---|---|
| 1. Herramientas automáticas | Hecho: ningún fallo en el código; un descuido en `.gitignore` (arreglado) |
| 2. Sondas dirigidas | En curso |
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
