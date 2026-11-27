# Revisión de errores

Búsqueda exhaustiva de errores en el código, empezada el 18 de noviembre de 2026. Se hace en tres pasos, de más
barato y fiable a más caro; este documento dice en qué punto está y lo que se ha encontrado, para seguir desde
cualquier ordenador.

## Estado

| Paso | Estado |
|---|---|
| 1. Herramientas automáticas | Hecho: ningún fallo en el código; un descuido en `.gitignore` (arreglado). Repetido el 23 de noviembre, tras partir `VentanaPrincipal` y otros cambios: sin nada nuevo |
| 2. Sondas dirigidas | Hecho: sondas en `tests/prueba_sondas.cpp`, pasadas a fondo (×20) y con los detectores; seis fallos arreglados. Repetidas el 23 de noviembre, también la de pdflatex: sin nada. El 27 de noviembre, tras un fallo de la maqueta que no veían (el 8), una sonda más, que busca en lo exportado todo el texto de las preguntas: un fallo más (el 9) |
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

### Repetición (23 de noviembre de 2026)

Tras los cambios de esos días: `VentanaPrincipal` partida en `PestanaBanco` y `PanelConfiguracion`, el diálogo de
verificar en una clase por fichero, el LaTeX en `latex.cpp`, la paleta de colores (`paleta.h`) y el código dentro del
texto de las preguntas (entre comillas invertidas).

- **Detectores (ASan + UBSan + fugas)**, en `build-sanitizadores` en modo Debug (comprobado antes que están enlazados
  y que el de fugas funciona en el equipo): las 14 pruebas, sin ningún error. La herramienta de las capturas no se ha
  pasado: en este equipo no está la carpeta `presentacion/`.
- **clang-tidy 19**, con las mismas comprobaciones (sin `bugprone-easily-swappable-parameters`,
  `bugprone-narrowing-conversions` ni `bugprone-implicit-widening-of-multiplication-result`, que son de estilo) sobre
  todo `src/`: los dos `switch` sin `default` de `ModeloPreguntas` de la otra vez, y nada más.
- **Avisos de GCC 14**, con las mismas opciones: 13, todos de los que ya se dieron por sin importancia (conversiones a
  `qsizetype`, inicializadores de campos con su valor por defecto y nombres repetidos en las pruebas). Ninguno en el
  código nuevo.
- **cppcheck 2.17** (`--enable=warning,portability`), que no se había pasado: un solo aviso, falso.
  `DialogoExportar::ofrecerSoluciones()` crea una casilla sin borrar la anterior, pero se llama una vez por diálogo y
  la casilla es del diálogo, que la destruye.

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

Se pasan con las demás pruebas (unos 6 segundos). Para buscar más a fondo, con las mismas semillas y más vueltas:
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
- `nadaSePierdeAlExportar`: exámenes de 8 a 40 preguntas legibles inventadas (palabras de sílabas al azar, que no se
  repiten de una pregunta a otra), con código, tablas, imágenes, superíndices y subíndices y textos de largos muy
  distintos, con todos los niveles de la maqueta, con y sin soluciones: en el texto del HTML, del Word y del PDF está
  cada pregunta, en su orden, con todas las líneas y celdas de su enunciado, sus opciones y, con soluciones, su
  explicación y su referencia; y los pies del PDF cuentan las páginas que salen. Con `SONDAS_PDFLATEX=1`, también el
  LaTeX compilado. Las demás sondas solo miran que exportar no falle; esta, que no se pierda nada. Con
  `SONDAS_GUARDAR=<carpeta>`, el PDF en que encuentre un fallo se copia allí, con el texto que se ha leído de él; en
  GitHub, el flujo los deja para descargar cuando fallan las pruebas.
- `latexCompilaConTextosRaros` (solo con `SONDAS_PDFLATEX=1` y pdflatex instalado, porque tarda): el LaTeX de
  preguntas con textos raros compila con pdflatex.

Pasadas hechas el 18 de noviembre: todas con `SONDAS=20` y con los detectores (ASan + UBSan + fugas), sin nada; la de
pdflatex con `SONDAS=5` (60 documentos) compila tras los arreglos 6 y 7.

El 27 de noviembre, `nadaSePierdeAlExportar` con `SONDAS=20` y `SONDAS_PDFLATEX=1` (140 exámenes, con y sin
soluciones, en los cuatro formatos), sin nada tras el arreglo 9. Ese día, además, con un programa aparte que se queda
fuera del repositorio porque usa los bancos, se exportaron todas las preguntas de los tres bancos (de 20 en 20, con
la maqueta normal, la compacta y la de letra grande) y exámenes generados con sus repartos, en PDF, HTML, Word, LaTeX
y Moodle, buscando cada texto en lo exportado: solo apareció el 9. Desde ese día es una prueba más,
`nadaSePierdeAlExportar` de `prueba_bancos_reales` (se salta si no están los bancos, y tarda: solo con
`BANCOS_EXPORTAR=1`; con `SONDAS_PDFLATEX=1`, también el LaTeX), que comparte con la sonda la forma de buscar cada
pregunta (`tests/texto_exportado.h`) y no dice el texto de lo que falta.

Repetidas el 23 de noviembre: todas con `SONDAS=20` y con los detectores, sin nada, y la de pdflatex con `SONDAS=5`
(60 documentos, ya con código dentro del texto: las sondas ponen comillas invertidas al azar), que compila.

## 3. Revisión del código por partes (sugerencia)

Acabados 1 y 2, es lo que queda: revisar el código a mano por partes (base de datos, generador, maqueta y exportaciones,
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
5. **Un texto que empezaba por varias U+FEFF cambiaba al pasar por el JSON** (arreglado). SQLite quita una de esas
   marcas invisibles del principio cada vez que guarda, así que la ida y vuelta base de datos → JSON → base de datos no
   lo dejaba igual. Encontrado por `idaYVueltaDelJson` con `SONDAS=20`. Arreglado: se quitan todas antes de guardar
   (`texto()` en `src/basedatos.cpp`). Prueba: `marcaDeOrdenAlPrincipio` (base de datos).
6. **El LaTeX no compilaba con muchos caracteres** (arreglado). Con un solo carácter que pdflatex no conoce, el
   documento entero falla: letras griegas no declaradas (Ε, λ, σ…), el ✔ (U+2714), símbolos de lógica y conjuntos
   (∧ ∨ ⊕ ≡ ∀ ∈…), emojis, y los caracteres de control que llegan al copiar de Word (U+000B…). Encontrado por
   `latexCompilaConTextosRaros`. Arreglado: el programa quita los caracteres de control; el preámbulo declara el
   alfabeto griego, los símbolos de lógica, conjuntos y flechas, ✔/✗ y los caracteres invisibles de formato; y
   cualquier otro carácter desconocido sale como «?» en un recuadro en vez de parar la compilación.
7. **El LaTeX no compilaba si una línea del enunciado empezaba por «[»** (arreglado). LaTeX tomaba el `\\` de la línea
   anterior por `\\[espacio]` (pasa con una lista de Python, `[1, 2, 3]`); y una línea con solo caracteres invisibles
   dejaba un `\\` sin línea que terminar. Encontrado por `latexCompilaConTextosRaros` con `SONDAS=5`. Arreglado: las
   líneas se separan con `\\\relax` y se saltan las que no tienen nada visible. Prueba: `latexSinLineasVacias`
   (enunciado) al día.
8. **En el PDF se perdían los enunciados a partir de una pregunta** (arreglado en `8b8edb7`). Al pasar una pregunta
   entera a la página siguiente para que no se parta, Qt rehacía la maqueta solo desde ella, y lo que venía detrás (con
   tablas) quedaba mal colocado: enunciados que no se pintaban (salían las opciones sin su pregunta) y menos páginas en
   el pie de las que había. Pasaba con exámenes normales, sin nada raro. Lo encontró un profesor en los exámenes de
   una convocatoria; las sondas no lo veían porque solo miraban que exportar no fallara. Arreglado: tras cada salto se
   rehace la maqueta entera. Pruebas: `ningunEnunciadoSePierdeTrasUnSaltoDePagina` (maqueta) y la sonda
   `nadaSePierdeAlExportar`, que lo encuentra en cuanto se quita el arreglo.
9. **En el LaTeX, los superíndices y subíndices salían como «?»** (arreglado). ⁰, ⁴ a ⁹, ⁺, ⁻, ⁿ y ₀ a ₉ no los conoce
   pdflatex, y salían como «?» en un recuadro (lo que se hace desde el arreglo 6 con los caracteres desconocidos); ¹,
   ² y ³ sí salían. Uno de los bancos los usa. Encontrado al buscar en lo exportado los textos de todas las preguntas
   de los bancos. Arreglado: el preámbulo los declara. Pruebas: `latexConoceLosSuperindicesYSubindices` (maqueta) y
   `nadaSePierdeAlExportar` con `SONDAS_PDFLATEX=1`.
10. **En Windows, la marca de la correcta (✔) salía enorme en el PDF de soluciones** (arreglado). Arial no la tiene, y
    Qt la tomaba de la letra de los emojis de Windows (Segoe UI Emoji), que en el PDF sale como una gran mancha verde
    que tapa media página. Pasaba desde que hay soluciones en PDF, y también al imprimirlas. Encontrado por
    `nadaSePierdeAlExportar` en el flujo de GitHub, la primera vez que se pasaba en Windows: el lector de PDF no
    encontraba las opciones que había debajo (con `SONDAS_GUARDAR`, el flujo dejó el PDF para verlo). Arreglado: la
    marca va con una letra de símbolos que la tiene en negro (Segoe UI Symbol, DejaVu Sans o Zapf Dingbats). Prueba:
    `laMarcaDeLaCorrectaNoEsUnEmoji` (maqueta), que mira que el PDF no lleve ninguna letra de emojis.

### Vistos y no arreglados

- **Cuando un párrafo se parte entre dos páginas, el PDF lleva dos veces la línea del corte.** Qt
  (`QTextLayout::draw()`) pinta también la última línea que queda por encima de la zona que se le pide, así que la
  última línea de la primera página se escribe otra vez en la segunda, por encima de su borde: no se ve ni se imprime,
  pero sale repetida al copiar el texto del PDF, al buscar en él o al leerlo con un lector de pantalla. Solo pasa con
  las preguntas que no caben en una página, que se parten (casi siempre con letra grande; las demás se pasan enteras a
  la página siguiente). No se arregla: estrechar la zona no sirve, y habría que pintar las líneas una a una en lugar
  de usar la maqueta de Qt. Encontrado por `nadaSePierdeAlExportar` con `SONDAS=20`, que lo tiene en cuenta.

- **SQLite quita un U+FEFF al principio de un texto.** Es la marca de orden de bytes del UTF-16, invisible; el
  controlador de Qt pasa los textos a SQLite en UTF-16 y SQLite la toma por una marca y la quita. No se ve, así que da
  igual, salvo en un caso extremo: un texto que sea solo esa marca (una opción, por ejemplo) se queda vacío, y el JSON
  que se exporta después no lo aceptaría el comprobador. No se arregla: nadie escribe una opción que sea solo un
  carácter invisible. Las sondas lo tienen en cuenta.
- **Al cambiar una pregunta por otra que se repite en otro examen, el aviso dice «está también en otro examen de la
  convocatoria»**, no el de la generación («para no repetir entre exámenes»): no es un fallo; la sonda acepta los
  dos.
