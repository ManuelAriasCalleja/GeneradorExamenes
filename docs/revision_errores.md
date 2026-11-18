# Revisión de errores

Búsqueda exhaustiva de errores en el código, empezada el 18 de noviembre de 2026. Se hace en tres pasos, de más
barato y fiable a más caro; este documento dice en qué punto está y lo que se ha encontrado, para seguir desde
cualquier ordenador.

## Estado

| Paso | Estado |
|---|---|
| 1. Herramientas automáticas | En curso |
| 2. Sondas dirigidas | Pendiente |
| 3. Revisión del código por partes | Sugerencia, para cuando acaben 1 y 2 |

## 1. Herramientas automáticas

Sin apenas falsos positivos:

- **Detectores de errores de GCC** (AddressSanitizer y UndefinedBehaviorSanitizer): compilar con ellos y pasar las
  pruebas y la herramienta de las capturas, que recorre la ventana y los diálogos. Encuentran accesos a memoria
  liberada o fuera de rango, fugas y comportamiento indefinido (como el cierre en falso al destruir la ventana que se
  arregló en `bafa613`).
- **clang-tidy**, con las comprobaciones de errores típicos (`bugprone-*`, `clang-analyzer-*`).
- **Todos los avisos del compilador** (`-Wall -Wextra -Wshadow` y otros que señalan errores).

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

_Aún ninguno._
