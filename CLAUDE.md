# GeneradorExamenesFI

Generador de exámenes tipo test de Fundamentos de Informática (Curso de Acceso, UNED) en C++/Qt 6.
Compilación, uso y ficheros: ver `README.md`. Mejoras pendientes: ver `PENDIENTE.md`.

## Las preguntas no van en git
El repositorio se publica en Internet y **no puede contener preguntas de ningún banco**, ni siquiera como
ejemplo o fragmento de un enunciado o de una opción. Todo lo que las contenga va en `datos/`, que está en
`.gitignore`: bases de datos, JSON, imágenes, banco 2, informes y la guía de estilo de redacción. Lo mismo
vale para el README, los comentarios del código, los mensajes de commit y los datos de prueba. Los exámenes
exportados (PDF, HTML, LaTeX) tampoco se guardan dentro del proyecto.

Hay dos bancos de preguntas que **no deben mezclarse**:
- el actual (ver11): `datos/preguntas.db` y `datos/preguntas_fi.json`;
- el banco 2: `datos/banco2/`, 1500 preguntas propuestas y pendientes de revisión del equipo docente.
  Ver `datos/banco2/LEEME.md`.

## Estilo de redacción de las preguntas
Está en `datos/estilo_preguntas.md` (fuera de git porque cita preguntas reales). Seguirlo al escribir,
corregir o proponer preguntas:

@datos/estilo_preguntas.md
