# GeneradorExamenesFI

Generador de exámenes tipo test de Fundamentos de Informática (Curso de Acceso, UNED) en C++/Qt 6.
Compilación, uso y ficheros: ver `README.md`. Mejoras pendientes: ver `PENDIENTE.md`.

## Las preguntas no van en git
El repositorio se publica en Internet y **no puede contener preguntas de ningún banco**, ni siquiera como
ejemplo o fragmento de un enunciado o de una opción. Los bancos (bases de datos, JSON, imágenes, informes y la
guía de estilo de redacción) están fuera del repositorio, en el directorio de la asignatura. Lo mismo vale
para el README, los comentarios del código, los mensajes de commit y los datos de prueba. Los exámenes
exportados (PDF, HTML, LaTeX) tampoco se guardan dentro del proyecto.

`asignatura` es un enlace local (en `.gitignore`) al directorio de la asignatura, para llegar a los bancos
desde aquí. Si falta: `ln -s "<directorio de la asignatura>" asignatura`. El programa no lo usa: abre el
último banco que se abrió, cuya ruta guarda en su fichero de configuración (`~/.config/UNED-FI/GeneradorExamenesFI.conf`).

## Pruebas
`ctest --test-dir build --output-on-failure` (ver README). Pasarlas antes de dar por bueno un cambio, y añadir
una cuando se arregle un fallo o se programe algo nuevo. Los datos de prueba son inventados (`tests/datos_prueba.h`):
nunca se copian preguntas, opciones ni frases de los bancos, tampoco para reproducir un caso real.

## Manual de uso
`recursos/ayuda.html` es la ayuda del programa (*Ayuda → Manual de uso*, F1, y el botón «Ayuda» de los diálogos, que
abren cada uno por el ancla de su apartado). Si cambia lo que el usuario ve o hace, hay que actualizarla. Como el
README, no puede citar preguntas de los bancos. Lo mismo la presentación (`presentacion/`): sus capturas se rehacen
con `cmake --build build --target capturas` y salen del banco inventado de `presentacion/banco_ejemplo.h`, en el que
tampoco puede haber preguntas, opciones ni frases de los bancos.

## La presentación no va en git
`presentacion/` (la presentación del programa para el equipo docente, en Typst) está en `.gitignore` y se queda
fuera del repositorio: no se añade a git ni se menciona en el README. El objetivo `capturas` de CMake solo existe si
está la carpeta.

## GitHub
El repositorio está en GitHub con **GitHub Actions desactivado**: no se compila ni se comprueba nada allí (no
hay minutos). No añadir flujos de trabajo (`.github/workflows`) ni otras comprobaciones automáticas.

Hay dos bancos de preguntas que **no deben mezclarse**:
- el actual (ver11), hecho por el equipo docente: `asignatura/BancoPreguntas/preguntas.db` y
  `asignatura/BancoPreguntas/preguntas_fi.json`;
- el banco 2: `asignatura/BancoPreguntas2/`, 1500 preguntas propuestas por Claude y pendientes de revisión del
  equipo docente. Ver `asignatura/BancoPreguntas2/LEEME.md`.

## Estilo de redacción de las preguntas
Está en `asignatura/BancoPreguntas/estilo_preguntas.md` (fuera de git porque cita preguntas reales). Seguirlo
al escribir, corregir o proponer preguntas:

@asignatura/BancoPreguntas/estilo_preguntas.md

Las pautas que se pueden comprobar solas están programadas en `src/estilo.cpp` (avisos del editor de preguntas
y del informe *Editar → Revisar el estilo del banco*). Si cambia la guía, hay que actualizarlas.
