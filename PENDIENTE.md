# Mejoras pendientes

Propuestas aceptadas el 22 de octubre de 2026, por orden de prioridad. Se van haciendo de una en una; al
terminar cada una, se quita de aquí.

1. **Aviso de preguntas parecidas** al crear una, por similitud de texto, proponiendo guardarla como la
   siguiente variante de la existente.
2. **Copia de seguridad automática** de la base de datos antes de «Importar → Reemplazar todo», que borra
   todos los temas y preguntas.
3. **Una sola fuente de las preguntas.** La base de datos y `asignatura/BancoPreguntas/preguntas_fi.json` se mantienen a mano por
   separado. Que el programa avise cuando no coincidan, o que actualice el JSON él solo.
4. **Windows: paquete para otros ordenadores**, un zip o instalador preparado con `windeployqt`. (El programa ya no
   depende de rutas fijadas al compilar: pregunta por el banco si no lo encuentra.)
5. **Duración, al diálogo de práctica.** El campo «Duración» de la convocatoria ya solo sirve para el reloj de
   «Practicar este examen…».
6. **Versión adaptada con letra grande**, para estudiantes con discapacidad visual (el contrario de los niveles
   de compactación de la maqueta).
7. **Exportar el banco a cuestionarios de Moodle** (XML o GIFT), para practicar en el curso virtual si la
   plataforma los admite.
8. **Pruebas automáticas** (Qt Test): con cientos de semillas, que nunca salgan juntas dos variantes ni dos
   pseudorrepetidas, que se respete el reparto por tema y que un examen de 10 preguntas quepa en un folio.
   Los datos de prueba deben ser inventados (ver «Las preguntas no van en git» en `CLAUDE.md`).
