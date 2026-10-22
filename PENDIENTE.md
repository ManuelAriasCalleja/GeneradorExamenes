# Mejoras pendientes

Propuestas aceptadas el 22 de octubre de 2026, por orden de prioridad. Se van haciendo de una en una; al
terminar cada una, se quita de aquí.

1. **Reparto de la respuesta correcta en cada examen.** Si no se barajan las opciones, la correcta queda en la
   misma letra que en el banco, donde ya hay más A y B que C y D. Barajar buscando un reparto equilibrado (o al
   menos mostrar el recuento de cada letra en cada examen).
2. **Comprobación de estilo al guardar una pregunta.** Avisos, no bloqueos, con las reglas de
   `datos/estilo_preguntas.md`: enunciado acabado en «:» o «¿…?:», opciones con mayúscula y punto final,
   «Todas las anteriores», longitud máxima, formato de la referencia al libro según el tema, comillas
   tipográficas dentro del código… Y un informe para revisar todo el banco de una vez (vale también para el
   banco 2).
3. **Aviso de preguntas parecidas** al crear una, por similitud de texto, proponiendo guardarla como la
   siguiente variante de la existente.
4. **Copia de seguridad automática** de la base de datos antes de «Importar → Reemplazar todo», que borra
   todos los temas y preguntas.
5. **Una sola fuente de las preguntas.** La base de datos y `datos/preguntas_fi.json` se mantienen a mano por
   separado. Que el programa avise cuando no coincidan, o que actualice el JSON él solo.
6. **Windows: base de datos en la carpeta de datos del usuario** (`QStandardPaths::AppDataLocation`) cuando no
   exista la ruta fijada al compilar, para que el `.exe` funcione en cualquier ordenador. Y un zip o instalador
   preparado con `windeployqt`.
7. **Duración, al diálogo de práctica.** El campo «Duración» de la convocatoria ya solo sirve para el reloj de
   «Practicar este examen…».
8. **Versión adaptada con letra grande**, para estudiantes con discapacidad visual (el contrario de los niveles
   de compactación de la maqueta).
9. **Exportar el banco a cuestionarios de Moodle** (XML o GIFT), para practicar en el curso virtual si la
   plataforma los admite.
10. **Pruebas automáticas** (Qt Test): con cientos de semillas, que nunca salgan juntas dos variantes ni dos
    pseudorrepetidas, que se respete el reparto por tema y que un examen de 10 preguntas quepa en un folio.
    Los datos de prueba deben ser inventados (ver «Las preguntas no van en git» en `CLAUDE.md`).
