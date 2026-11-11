<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="en" sourcelanguage="es">
<context>
    <name>BaseDatos</name>
    <message>
        <location filename="../src/basedatos.cpp" line="+112"/>
        <source>No se puede abrir %1: %2</source>
        <translation>Cannot open %1: %2</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>JSON no válido: %1</source>
        <translation>Invalid JSON: %1</translation>
    </message>
    <message>
        <location line="+439"/>
        <source>Ya existe una pregunta %1 en ese tema.</source>
        <translation>A question %1 already exists in that topic.</translation>
    </message>
    <message>
        <location line="+30"/>
        <source>No se puede crear la carpeta %1</source>
        <translation>Cannot create the folder %1</translation>
    </message>
    <message>
        <location line="+49"/>
        <source>No se ha podido guardar la copia de seguridad del banco, así que no se ha borrado nada:
%1</source>
        <translation>The backup of the bank could not be saved, so nothing has been deleted:
%1</translation>
    </message>
    <message>
        <location line="+37"/>
        <source>Tema %1, pregunta %2: %3</source>
        <translation>Topic %1, question %2: %3</translation>
    </message>
    <message>
        <location line="+141"/>
        <source>pregunta distinta</source>
        <translation>question that differs</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>preguntas distintas</source>
        <translation>questions that differ</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>pregunta solo en la base de datos</source>
        <translation>question only in the database</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>preguntas solo en la base de datos</source>
        <translation>questions only in the database</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>pregunta solo en el JSON</source>
        <translation>question only in the JSON</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>preguntas solo en el JSON</source>
        <translation>questions only in the JSON</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>tema con otro título, autor o forma de la referencia</source>
        <translation>topic with a different title, author or reference format</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>temas con otro título, autor o forma de la referencia</source>
        <translation>topics with a different title, author or reference format</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>tema solo en la base de datos</source>
        <translation>topic only in the database</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>temas solo en la base de datos</source>
        <translation>topics only in the database</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>tema solo en el JSON</source>
        <translation>topic only in the JSON</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>temas solo en el JSON</source>
        <translation>topics only in the JSON</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>En el JSON la asignatura no tiene nombre</source>
        <translation>In the JSON the subject has no name</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>La asignatura tiene otro nombre en el JSON: «%1»</source>
        <translation>The subject has a different name in the JSON: “%1”</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Los exámenes están en otro idioma en el JSON: «%1»</source>
        <extracomment>%1 es el código del idioma de los exámenes que dice el JSON («es», «en»…).</extracomment>
        <translation>The exams are in a different language in the JSON: “%1”</translation>
    </message>
</context>
<context>
    <name>ComprobadorJson</name>
    <message>
        <location filename="../src/comprobadorjson.cpp" line="+66"/>
        <source>El fichero está vacío.</source>
        <translation>The file is empty.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Sobra texto después del final del JSON: ¿hay una llave «}» de más, o dos objetos seguidos?</source>
        <translation>There is text after the end of the JSON: is there an extra “}”, or two objects one after the other?</translation>
    </message>
    <message>
        <location line="+56"/>
        <source>Los textos y los nombres de los campos van entre comillas dobles (&quot;), no simples (&apos;).</source>
        <translation>Texts and field names go in double quotes (&quot;), not single quotes (&apos;).</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Los textos van entre comillas rectas (&quot;), no tipográficas (“ ” « »): suele pasar al copiar el JSON de un procesador de textos.</source>
        <translation>Texts go in straight quotes (&quot;), not typographic ones (“ ” « »): this usually happens when the JSON is copied from a word processor.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>En JSON no se pueden poner comentarios.</source>
        <translation>Comments are not allowed in JSON.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Sobra una coma, o falta un valor antes de ella.</source>
        <translation>There is an extra comma, or a value is missing before it.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Falta un valor antes de «%1».</source>
        <translation>A value is missing before “%1”.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>«%1» no es un valor válido: los textos van entre comillas dobles, y los demás valores son números, true, false, null, listas [ … ] u objetos { … }.</source>
        <translation>“%1” is not a valid value: texts go in double quotes, and the other values are numbers, true, false, null, lists [ … ] or objects { … }.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>El fichero se acaba antes de tiempo: falta un valor.</source>
        <translation>The file ends too early: a value is missing.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Hay demasiados niveles de { } o [ ] uno dentro de otro.</source>
        <translation>There are too many levels of { } or [ ] nested inside each other.</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>«%1» se escribe en minúsculas: true, false o null.</source>
        <translation>“%1” is written in lower case: true, false or null.</translation>
    </message>
    <message>
        <location line="+17"/>
        <location line="+27"/>
        <source>Falta un «}»: el objeto que empieza en la línea %1 no se cierra.</source>
        <translation>A “}” is missing: the object that starts on line %1 is not closed.</translation>
    </message>
    <message>
        <location line="-24"/>
        <source>Sobra la coma antes de «}».</source>
        <translation>There is an extra comma before “}”.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>El nombre del campo va entre comillas dobles: &quot;%1&quot;.</source>
        <translation>Field names go in double quotes: &quot;%1&quot;.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Aquí tendría que ir el nombre de un campo, entre comillas dobles.</source>
        <translation>A field name in double quotes should go here.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Falta «:» después de &quot;%1&quot;.</source>
        <translation>“:” is missing after &quot;%1&quot;.</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Falta una coma antes de este campo.</source>
        <translation>A comma is missing before this field.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Aquí se cierra con «]» algo que se abrió con «{» en la línea %1.</source>
        <translation>Something opened with “{” on line %1 is closed here with “]”.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Después del valor de &quot;%1&quot; tiene que ir una coma o «}».</source>
        <translation>The value of &quot;%1&quot; must be followed by a comma or “}”.</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Sobra la coma antes de «]».</source>
        <translation>There is an extra comma before “]”.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Falta un «]»: la lista que empieza en la línea %1 no se cierra.</source>
        <translation>A “]” is missing: the list that starts on line %1 is not closed.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Aquí se cierra con «}» algo que se abrió con «[» en la línea %1.</source>
        <translation>Something opened with “[” on line %1 is closed here with “}”.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Falta una coma entre dos elementos de la lista.</source>
        <translation>A comma is missing between two items of the list.</translation>
    </message>
    <message>
        <location line="+10"/>
        <location line="+20"/>
        <source>Falta la comilla que cierra el texto que empieza aquí.</source>
        <translation>The quote that closes the text starting here is missing.</translation>
    </message>
    <message>
        <location line="-13"/>
        <source>Salto de línea dentro de un texto: en JSON se escribe \n. (O falta la comilla que cierra el texto que empieza en la línea %1, columna %2.)</source>
        <translation>Line break inside a text: in JSON it is written \n. (Or the quote that closes the text starting on line %1, column %2 is missing.)</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Carácter de control (código %1) dentro de un texto: en JSON no se permite; un tabulador se escribe \t.</source>
        <translation>Control character (code %1) inside a text: JSON does not allow it; a tab is written \t.</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Después de \u van cuatro cifras hexadecimales (\u00e1).</source>
        <translation>\u must be followed by four hexadecimal digits (\u00e1).</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>«\%1» no vale dentro de un texto: una barra invertida se escribe \\, y un salto de línea, \n.</source>
        <translation>“\%1” is not allowed inside a text: a backslash is written \\, and a line break \n.</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Número mal escrito: «%1».</source>
        <translation>Badly written number: “%1”.</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>null</source>
        <translation>null</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>true</source>
        <translation>true</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>false</source>
        <translation>false</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>el número %1</source>
        <translation>the number %1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>el texto «%1»</source>
        <translation>the text “%1”</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>una lista</source>
        <translation>a list</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>un objeto</source>
        <translation>an object</translation>
    </message>
    <message>
        <location line="+34"/>
        <source>El JSON tiene que ser un objeto { … } con la lista de temas en «temas», y es %1.</source>
        <translation>The JSON must be an object { … } with the list of topics in “temas”, and it is %1.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>El idioma «%1» no es ninguno de los del programa (%2): los exámenes saldrían en castellano.</source>
        <translation>The language “%1” is not one of the program’s (%2): the exams would come out in Spanish.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Falta el campo «temas», la lista de los temas con sus preguntas.</source>
        <translation>The field “temas” is missing: the list of topics with their questions.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>«temas» tiene que ser una lista [ … ] de temas, y es %1.</source>
        <translation>“temas” must be a list [ … ] of topics, and it is %1.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>La lista de temas está vacía: no hay nada que importar.</source>
        <translation>The list of topics is empty: there is nothing to import.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Es pseudorrepetida con %1, que no está en este JSON (puede estar ya en el banco).</source>
        <translation>It is a pseudo-duplicate of %1, which is not in this JSON (it may already be in the bank).</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Es pseudorrepetida con %1, que no está ni en este JSON ni en el banco.</source>
        <translation>It is a pseudo-duplicate of %1, which is neither in this JSON nor in the bank.</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>El campo «%1» no es de este formato: no se usa.</source>
        <translation>The field “%1” is not part of this format: it is ignored.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>El campo «%1» no es de este formato: no se usa. ¿Quería decir «%2»?</source>
        <translation>The field “%1” is not part of this format: it is ignored. Did you mean “%2”?</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Falta el campo «%1».</source>
        <translation>The field “%1” is missing.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>«%1» tiene que ser un texto, entre comillas dobles, y es %2.</source>
        <translation>“%1” must be a text, in double quotes, and it is %2.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>«%1» tiene que ser true o false (sin comillas), y es %2.</source>
        <translation>“%1” must be true or false (without quotes), and it is %2.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Falta el campo «numero».</source>
        <translation>The field “numero” is missing.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>«numero» tiene que ser un número entero mayor que 0, sin comillas, y es %1.</source>
        <translation>“numero” must be a whole number greater than 0, without quotes, and it is %1.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>tema n.º %1 de la lista</source>
        <translation>topic no. %1 in the list</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Cada tema tiene que ser un objeto { … }, y este es %1.</source>
        <translation>Each topic must be an object { … }, and this one is %1.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>tema %1</source>
        <translation>topic %1</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>El tema %1 está repetido (el otro, en la línea %2).</source>
        <translation>Topic %1 appears twice (the other one is on line %2).</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>El tema no tiene «titulo»: si ya está en el banco, conserva el suyo; si no, se queda sin título.</source>
        <translation>The topic has no “titulo”: if it is already in the bank, it keeps its own; if not, it will have no title.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>El título del tema está vacío.</source>
        <translation>The topic title is empty.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>«formato_referencia» tiene que ser &quot;vease&quot;, &quot;pag&quot; o &quot;&quot; (cualquiera), y es «%1».</source>
        <translation>“formato_referencia” must be &quot;vease&quot;, &quot;pag&quot; or &quot;&quot; (any), and it is “%1”.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Falta el campo «preguntas», la lista de las preguntas del tema.</source>
        <translation>The field “preguntas” is missing: the list of the topic’s questions.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>«preguntas» tiene que ser una lista [ … ], y es %1.</source>
        <translation>“preguntas” must be a list [ … ], and it is %1.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>El tema no tiene preguntas.</source>
        <translation>The topic has no questions.</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>tema %1, pregunta n.º %2 de la lista</source>
        <translation>topic %1, question no. %2 in the list</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Cada pregunta tiene que ser un objeto { … }, y esta es %1.</source>
        <translation>Each question must be an object { … }, and this one is %1.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>«variante» tiene que ser una letra minúscula (&quot;a&quot;, &quot;b&quot;…) o &quot;&quot; si no tiene variantes, y es «%1».</source>
        <translation>“variante” must be a lower-case letter (&quot;a&quot;, &quot;b&quot;…) or &quot;&quot; if there are no variants, and it is “%1”.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>tema %1, pregunta %2</source>
        <translation>topic %1, question %2</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>La pregunta %1 está repetida en el tema (la otra, en la línea %2).</source>
        <translation>Question %1 appears twice in the topic (the other one is on line %2).</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>«codigo» dice «%1», pero el número y la variante dan %2: al importar cuentan estos.</source>
        <translation>“codigo” says “%1”, but the number and the variant give %2: these are what count when importing.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>«estado» tiene que ser &quot;activa&quot;, &quot;anulada&quot; u &quot;obsoleta&quot;, y es «%1».</source>
        <translation>“estado” must be &quot;activa&quot;, &quot;anulada&quot; or &quot;obsoleta&quot;, and it is “%1”.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>El enunciado está vacío.</source>
        <translation>The question text is empty.</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>«pseudo_con» tiene que ser el código de otra pregunta sin la letra de la variante, como &quot;T1-7&quot; (tema 1, número 7), y es «%1».</source>
        <translation>“pseudo_con” must be the code of another question without the variant letter, such as &quot;T1-7&quot; (topic 1, number 7), and it is “%1”.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>«imagen» no se puede leer: tiene que ser una imagen PNG o JPG codificada en base64 (o null si no tiene).</source>
        <translation>“imagen” cannot be read: it must be a PNG or JPG image encoded in base64 (or null if there is none).</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Falta el campo «opciones», la lista de las cuatro opciones.</source>
        <translation>The field “opciones” is missing: the list of the four options.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>«opciones» tiene que ser una lista [ … ] de cuatro textos, y es %1.</source>
        <translation>“opciones” must be a list [ … ] of four texts, and it is %1.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>La lista de opciones está vacía: tienen que ser cuatro (a, b, c y d).</source>
        <translation>The list of options is empty: there must be four (a, b, c and d).</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Tiene una opción, y tienen que ser cuatro (a, b, c y d).</source>
        <translation>It has one option, and there must be four (a, b, c and d).</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Tiene %1 opciones, y tienen que ser cuatro (a, b, c y d).</source>
        <translation>It has %1 options, and there must be four (a, b, c and d).</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>La opción %1) tiene que ser un texto, entre comillas dobles, y es %2.</source>
        <translation>Option %1) must be a text, in double quotes, and it is %2.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>La opción %1) está vacía.</source>
        <translation>Option %1) is empty.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Las opciones %1) y %2) son iguales.</source>
        <translation>Options %1) and %2) are the same.</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Falta el campo «correcta», la letra de la respuesta correcta (&quot;A&quot;, &quot;B&quot;, &quot;C&quot; o &quot;D&quot;).</source>
        <translation>The field “correcta” is missing: the letter of the correct answer (&quot;A&quot;, &quot;B&quot;, &quot;C&quot; or &quot;D&quot;).</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>«correcta» tiene que ser la letra de la respuesta correcta, entre comillas: &quot;A&quot;, &quot;B&quot;, &quot;C&quot; o &quot;D&quot;, y es %1.</source>
        <translation>“correcta” must be the letter of the correct answer, in quotes: &quot;A&quot;, &quot;B&quot;, &quot;C&quot; or &quot;D&quot;, and it is %1.</translation>
    </message>
    <message>
        <location line="+53"/>
        <source>El fichero no está en UTF-8: guárdelo con esa codificación (en el Bloc de notas de Windows, «Guardar como» → Codificación: UTF-8).</source>
        <translation>The file is not in UTF-8: save it with that encoding (in Windows Notepad, “Save as” → Encoding: UTF-8).</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>El campo «%1» está dos veces en el mismo objeto (la otra, en la línea %2): solo valdría el último.</source>
        <translation>The field “%1” appears twice in the same object (the other one is on line %2): only the last one would count.</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>No se puede abrir el fichero: %1</source>
        <translation>The file cannot be opened: %1</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>error</source>
        <translation>error</translation>
    </message>
    <message>
        <location line="+0"/>
        <location filename="../tests/prueba_idiomas.cpp" line="+84"/>
        <source>aviso</source>
        <translation>warning</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>El JSON no se puede leer.</source>
        <translation>The JSON cannot be read.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>%1 en %2</source>
        <translation>%1 in %2</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>1 pregunta</source>
        <translation>1 question</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>%1 preguntas</source>
        <translation>%1 questions</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>1 tema</source>
        <translation>1 topic</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>%1 temas</source>
        <translation>%1 topics</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Sin errores ni avisos: %1.</source>
        <translation>No errors or warnings: %1.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>1 error</source>
        <translation>1 error</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>%1 errores</source>
        <translation>%1 errors</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>1 aviso</source>
        <translation>1 warning</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>%1 avisos</source>
        <translation>%1 warnings</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>%1 (%2).</source>
        <translation>%1 (%2).</translation>
    </message>
    <message>
        <location line="+0"/>
        <source> y </source>
        <translation> and </translation>
    </message>
</context>
<context>
    <name>DialogoAnteriores</name>
    <message>
        <location filename="../src/dialogoanteriores.cpp" line="+49"/>
        <source>Registrar exámenes anteriores</source>
        <translation>Record previous exams</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Para que al generar se eviten las preguntas de las convocatorias hechas fuera del programa, añada aquí sus exámenes: sus PDF o su texto copiado de Word. El programa reconoce cada pregunta en el banco; compruebe las dudosas (en amarillo) y, si hace falta, corrija su código. Se registran en el historial como las generadas.</source>
        <translation>So that questions from exam sessions held outside the program are avoided when generating, add their exams here: their PDFs or their text copied from Word. The program recognises each question in the bank; check the uncertain ones (in yellow) and, if necessary, correct their code. They are recorded in the history just like generated ones.</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Convocatoria:</source>
        <translation>Exam session:</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Año:</source>
        <translation>Year:</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Examen</source>
        <translation>Exam</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Preguntas</source>
        <translation>Questions</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Dudosas</source>
        <translation>Uncertain</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Doble clic en el nombre para cambiarlo</source>
        <translation>Double-click the name to change it</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Añadir PDF…</source>
        <translation>Add PDF…</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Esta versión del programa no lee PDF: copie el texto del examen y péguelo</source>
        <translation>This version of the program cannot read PDF files: copy the text of the exam and paste it</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Pegar texto…</source>
        <translation>Paste text…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Quitar</source>
        <translation>Remove</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Nº</source>
        <extracomment>Número de la pregunta en el examen</extracomment>
        <translation>No.</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Código</source>
        <translation>Code</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Parecido</source>
        <translation>Similarity</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>En el examen</source>
        <translation>In the exam</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Registrar</source>
        <translation>Record</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+89"/>
        <source>Cancelar</source>
        <translation>Cancel</translation>
    </message>
    <message>
        <location line="-64"/>
        <source>texto pegado</source>
        <translation>pasted text</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>No se ha encontrado ninguna pregunta en %1. Deben ir numeradas «1.-», «2.-»… al principio de una línea, con sus opciones «a)»…«d)».</source>
        <extracomment>«1.-», «2.-» y «a)»…«d)» son lo que se busca en el texto del examen: se dejan tal cual</extracomment>
        <translation>No questions were found in %1. They must be numbered “1.-”, “2.-”… at the start of a line, with their options “a)”…“d)”.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Examen %1</source>
        <translation>Exam %1</translation>
    </message>
    <message>
        <location line="+30"/>
        <source>Exámenes anteriores (PDF)</source>
        <translation>Previous exams (PDF)</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>PDF (*.pdf)</source>
        <translation>PDF (*.pdf)</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Pegar el texto de un examen</source>
        <translation>Paste the text of an exam</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Abra el examen (en Word o en un visor de PDF), seleccione todo su texto, cópielo y péguelo aquí:</source>
        <translation>Open the exam (in Word or a PDF viewer), select all its text, copy it and paste it here:</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Añadir</source>
        <translation>Add</translation>
    </message>
    <message>
        <location line="+37"/>
        <source>no está en el banco</source>
        <translation>not in the bank</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>a mano</source>
        <extracomment>En la columna «Parecido»: el código de la pregunta se ha escrito a mano</extracomment>
        <translation>by hand</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>En el banco:
%1

Doble clic para escribir otro código (T1-7a)</source>
        <extracomment>T1-7a es un ejemplo de código de pregunta: se escribe igual en cualquier idioma</extracomment>
        <translation>In the bank:
%1

Double-click to enter another code (T1-7a)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Doble clic para escribir el código de la pregunta (T1-7a)</source>
        <translation>Double-click to enter the question code (T1-7a)</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Se parece poco: compruebe que es esta pregunta.</source>
        <translation>Low similarity: check that it is this question.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Se parece casi lo mismo a %1 (%2 %): compruebe cuál es.</source>
        <translation>It is almost as similar to %1 (%2%): check which one it is.</translation>
    </message>
    <message>
        <location line="+55"/>
        <source>Añada los exámenes de la convocatoria con «Añadir PDF…» (varios a la vez) o «Pegar texto…».</source>
        <extracomment>«Añadir PDF…» y «Pegar texto…» son botones de este diálogo: tradúzcanse igual</extracomment>
        <translation>Add the exams of the exam session with “Add PDF…” (several at once) or “Paste text…”.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>%1 exámenes, %2 preguntas. %3</source>
        <translation>%1 exams, %2 questions. %3</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Todas reconocidas sin dudas.</source>
        <translation>All recognised with certainty.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dudosas: %1 (en amarillo); compruébelas y, si hace falta, escriba su código.</source>
        <translation>Uncertain: %1 (in yellow); check them and, if necessary, enter their code.</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Hay un examen sin nombre.</source>
        <translation>There is an exam without a name.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Hay dos exámenes con el mismo nombre: %1.</source>
        <translation>There are two exams with the same name: %1.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>«%1», pregunta %2</source>
        <translation>“%1”, question %2</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Indique la convocatoria (Febrero, Junio…).</source>
        <translation>Enter the exam session (February, June…).</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Añada los exámenes de la convocatoria.</source>
        <translation>Add the exams of the exam session.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Escriba el código de estas preguntas, que no se han reconocido:
%1</source>
        <translation>Enter the code of these questions, which have not been recognised:
%1</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Se registrarán en el historial %1 exámenes de %2. Las preguntas se guardan tal como están ahora en el banco.</source>
        <translation>%1 exams from %2 will be recorded in the history. The questions are saved as they are now in the bank.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>

Quedan %1 preguntas dudosas, en amarillo.</source>
        <translation>

%1 uncertain questions remain, in yellow.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>

Ya había registrados con el mismo nombre, y se sustituirán: %1.</source>
        <translation>

Some with the same name had already been recorded and will be replaced: %1.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>

¿Registrarlos?</source>
        <translation>

Record them?</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>No se pudieron registrar:
%1</source>
        <translation>They could not be recorded:
%1</translation>
    </message>
</context>
<context>
    <name>DialogoAyuda</name>
    <message>
        <location filename="../src/dialogoayuda.cpp" line="+22"/>
        <source>Ayuda del generador de exámenes</source>
        <translation>Exam Generator Help</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Buscar en la ayuda…</source>
        <translation>Search the help…</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Intro: siguiente vez que aparece</source>
        <extracomment>«Intro» es la tecla Intro (Entrar)</extracomment>
        <translation>Enter: next occurrence</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Buscar:</source>
        <translation>Search:</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Cerrar</source>
        <translation>Close</translation>
    </message>
    <message>
        <location line="+40"/>
        <source>Ayuda</source>
        <translation>Help</translation>
    </message>
</context>
<context>
    <name>DialogoComprobacion</name>
    <message>
        <location filename="../src/dialogocomprobacion.cpp" line="+22"/>
        <source>Comprobar %1</source>
        <translation>Check %1</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Tipo</source>
        <extracomment>Columnas de la lista de problemas de un JSON: gravedad (error o aviso), línea del fichero, pregunta y qué pasa</extracomment>
        <translation>Type</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Línea</source>
        <translation>Line</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Dónde</source>
        <translation>Where</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Problema</source>
        <translation>Problem</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Volver a comprobar</source>
        <translation>Check again</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Después de corregir el fichero en un editor de textos (y guardarlo)</source>
        <translation>After correcting the file in a text editor (and saving it)</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Abrir el fichero</source>
        <translation>Open the file</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Copiar el informe</source>
        <translation>Copy the report</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+45"/>
        <source>Importar de todas formas</source>
        <translation>Import anyway</translation>
    </message>
    <message>
        <location line="-44"/>
        <source>Cancelar</source>
        <translation>Cancel</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Cerrar</source>
        <translation>Close</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Con errores no se puede importar: corríjalos en el fichero y pulse «Volver a comprobar».</source>
        <translation>It cannot be imported with errors: correct them in the file and press “Check again”.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Ya no tiene problemas: se puede importar.</source>
        <translation>It has no problems now: it can be imported.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Los avisos no impiden importarlo.</source>
        <translation>The warnings do not prevent importing it.</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Importar</source>
        <translation>Import</translation>
    </message>
</context>
<context>
    <name>DialogoEstilo</name>
    <message>
        <location filename="../src/dialogoestilo.cpp" line="+26"/>
        <source>Revisión de estilo del banco</source>
        <translation>Bank style check</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Todos los avisos</source>
        <translation>All warnings</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Incluir las anuladas y obsoletas</source>
        <translation>Include voided and obsolete questions</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Mostrar:</source>
        <translation>Show:</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Pregunta</source>
        <translation>Question</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Aviso</source>
        <translation>Warning</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Editar la pregunta…</source>
        <translation>Edit the question…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Guardar el informe…</source>
        <translation>Save the report…</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Cerrar</source>
        <translation>Close</translation>
    </message>
    <message>
        <location line="+39"/>
        <source>%1 avisos</source>
        <translation>%1 warnings</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Ninguna de las %1 preguntas tiene avisos.</source>
        <translation>None of the %1 questions has any warnings.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Con avisos: %1 de %2 preguntas (%3 avisos). Doble clic para editar una.</source>
        <translation>With warnings: %1 of %2 questions (%3 warnings). Double-click one to edit it.</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>No hay más preguntas con avisos en la lista.</source>
        <translation>There are no more questions with warnings in the list.</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Guardar el informe de estilo</source>
        <translation>Save the style report</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Texto (*.txt)</source>
        <translation>Text (*.txt)</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Revisión de estilo del banco %1 (%2)
%3

</source>
        <translation>Style check of the bank %1 (%2)
%3

</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>No se pudo escribir %1</source>
        <translation>Could not write %1</translation>
    </message>
</context>
<context>
    <name>DialogoExportar</name>
    <message>
        <location filename="../src/dialogoexportar.cpp" line="+40"/>
        <source>Elegir…</source>
        <translation>Choose…</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Carpeta:</source>
        <translation>Folder:</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Marque los exámenes que quiere exportar y cambie el nombre de sus ficheros si lo desea:</source>
        <translation>Tick the exams you want to export and change their file names if you wish:</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Examen</source>
        <translation>Exam</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Fichero</source>
        <translation>File</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Pulse sobre el nombre para cambiarlo</source>
        <translation>Click the name to change it</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Todos los exámenes en un fichero</source>
        <translation>All the exams in one file</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Exportar</source>
        <translation>Export</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Cancelar</source>
        <translation>Cancel</translation>
    </message>
    <message>
        <location line="+78"/>
        <source>Carpeta donde guardar los exámenes</source>
        <translation>Folder in which to save the exams</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Indique la carpeta donde guardar los ficheros.</source>
        <translation>Specify the folder in which to save the files.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>No se pudo crear la carpeta
%1</source>
        <translation>Could not create the folder
%1</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Marque al menos un examen.</source>
        <translation>Tick at least one exam.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Hay un examen marcado sin nombre de fichero.</source>
        <translation>A ticked exam has no file name.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>El nombre «%1» no puede contener barras.</source>
        <translation>The name “%1” cannot contain slashes.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Dos exámenes tienen el mismo nombre de fichero: %1</source>
        <translation>Two exams have the same file name: %1</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Ya existen estos ficheros en la carpeta:
%1

¿Quiere sobrescribirlos?</source>
        <translation>These files already exist in the folder:
%1

Do you want to overwrite them?</translation>
    </message>
</context>
<context>
    <name>DialogoHistorial</name>
    <message>
        <location filename="../src/dialogohistorial.cpp" line="+23"/>
        <source>Historial de convocatorias</source>
        <translation>Exam session history</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Convocatoria / examen</source>
        <translation>Exam session / exam</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Preguntas</source>
        <translation>Questions</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Registrado</source>
        <translation>Recorded</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Eliminar del historial…</source>
        <translation>Delete from history…</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Cargar</source>
        <translation>Load</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Cerrar</source>
        <translation>Close</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Exámenes guardados con «Registrar convocatoria…». Cargue una convocatoria o un examen para verlo, exportarlo o imprimirlo tal como salió, aunque después se hayan corregido sus preguntas.</source>
        <extracomment>«Registrar convocatoria…» es una orden del menú Archivo de la ventana principal: tradúzcase igual</extracomment>
        <translation>Exams saved with “Record exam session…”. Load an exam session or an exam to view, export or print it exactly as it was set, even if its questions have been corrected since.</translation>
    </message>
    <message>
        <location line="+39"/>
        <source>Todavía no hay ninguna convocatoria registrada. Las anteriores al programa se registran con Archivo → Registrar exámenes anteriores…</source>
        <extracomment>«Archivo → Registrar exámenes anteriores…» es un menú de la ventana principal: tradúzcase igual</extracomment>
        <translation>No exam session has been recorded yet. Those from before the program are recorded with File → Record previous exams…</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Elija una convocatoria o un examen.</source>
        <translation>Choose an exam session or an exam.</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>el examen «%1» de %2</source>
        <translation>the exam “%1” from %2</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>la convocatoria %1 (un examen)</source>
        <translation>the exam session %1 (one exam)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>la convocatoria %1 (%2 exámenes)</source>
        <translation>the exam session %1 (%2 exams)</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>¿Eliminar del historial %1?

Sus preguntas dejarán de contar como usadas al generar exámenes y no se podrá volver a cargarlo.</source>
        <translation>Delete %1 from the history?

Its questions will no longer count as used when generating exams, and it will not be possible to load it again.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>No se pudo eliminar: %1</source>
        <translation>Could not delete: %1</translation>
    </message>
</context>
<context>
    <name>DialogoMoodle</name>
    <message>
        <location filename="../src/dialogomoodle.cpp" line="+25"/>
        <source>Exportar el banco a Moodle</source>
        <translation>Export the bank to Moodle</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Se guarda un cuestionario en formato «Moodle XML» con las preguntas activas del banco (las anuladas y obsoletas no van): una categoría por tema%1, cada pregunta de opción múltiple con una sola respuesta, la referencia al libro y la explicación como realimentación, y las imágenes incrustadas. En Moodle se importa desde &lt;i&gt;Banco de preguntas → Importar&lt;/i&gt;.</source>
        <extracomment>«Banco de preguntas → Importar» es el menú de Moodle: póngase como se llame en el Moodle de ese idioma. %1 es « bajo «asignatura»» o nada</extracomment>
        <translation>A quiz is saved in “Moodle XML” format with the active questions of the bank (voided and obsolete ones are left out): one category per topic%1, each question as multiple choice with a single answer, the reference to the textbook and the explanation as feedback, and the images embedded. In Moodle, it is imported from &lt;i&gt;Question bank → Import&lt;/i&gt;.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source> bajo «%1»</source>
        <comment>%1: la asignatura</comment>
        <translation> under “%1”</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Elegir…</source>
        <translation>Choose…</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Fichero:</source>
        <translation>File:</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Incluir las preguntas marcadas para revisar</source>
        <translation>Include questions flagged for review</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Las respuestas erróneas restan el 30 % de la pregunta (como en el examen)</source>
        <translation>Wrong answers deduct 30% of the question’s value (as in the exam)</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Exportar</source>
        <translation>Export</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Cancelar</source>
        <translation>Cancel</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Moodle XML (*.xml)</source>
        <translation>Moodle XML (*.xml)</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Se exportarán %1 preguntas de %2 temas.</source>
        <translation>%1 questions from %2 topics will be exported.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Indique el fichero.</source>
        <translation>Specify the file.</translation>
    </message>
</context>
<context>
    <name>DialogoParecidas</name>
    <message>
        <location filename="../src/dialogoparecidas.cpp" line="+18"/>
        <source>Preguntas parecidas</source>
        <translation>Similar questions</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>La pregunta nueva se parece mucho a estas del banco. Si es la misma idea (otro dato, la pregunta al revés u otro aspecto del mismo concepto), guárdela como variante de la existente: así nunca saldrán las dos en el mismo examen. Si es de otro tema, enlácelas como pseudorrepetidas.</source>
        <translation>The new question is very similar to these in the bank. If it is the same idea (different data, the question the other way round or another aspect of the same concept), save it as a variant of the existing one: that way the two will never appear in the same exam. If it belongs to another topic, link them as pseudo-duplicates.</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Guardar como pregunta nueva</source>
        <translation>Save as a new question</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Seguir editando</source>
        <translation>Continue editing</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>Guardar como variante de %1</source>
        <translation>Save as a variant of %1</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Enlazar con %1 (pseudorrepetida)</source>
        <translation>Link to %1 (pseudo-duplicate)</translation>
    </message>
</context>
<context>
    <name>DialogoPractica</name>
    <message>
        <location filename="../src/dialogopractica.cpp" line="+27"/>
        <source>Practicar: %1 - %2</source>
        <translation>Practice: %1 - %2</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;%1&lt;/b&gt; &amp;mdash; %2 &amp;mdash; %3 preguntas&lt;br&gt;&lt;small&gt;Acierto: +%4 &amp;nbsp; Fallo: −%5 &amp;nbsp; En blanco: 0&lt;/small&gt;</source>
        <translation>&lt;b&gt;%1&lt;/b&gt; &amp;mdash; %2 &amp;mdash; %3 questions&lt;br&gt;&lt;small&gt;Correct: +%4 &amp;nbsp; Wrong: −%5 &amp;nbsp; Blank: 0&lt;/small&gt;</translation>
    </message>
    <message>
        <location line="+9"/>
        <source> min</source>
        <extracomment>Sufijo de la duración del examen, en minutos (con el espacio delante)</extracomment>
        <translation> min</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Empezar</source>
        <translation>Start</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Duración:</source>
        <translation>Duration:</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Terminar y corregir</source>
        <translation>Finish and mark</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Cerrar</source>
        <translation>Close</translation>
    </message>
    <message>
        <location line="+72"/>
        <source>Dejar en blanco</source>
        <translation>Leave blank</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Se ha acabado el tiempo.</source>
        <translation>Time is up.</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>&lt;span style=&quot;color:#7f8c8d&quot;&gt;&lt;b&gt;En blanco.&lt;/b&gt;&lt;/span&gt;</source>
        <translation>&lt;span style=&quot;color:#7f8c8d&quot;&gt;&lt;b&gt;Blank.&lt;/b&gt;&lt;/span&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;span style=&quot;color:%1&quot;&gt;&lt;b&gt;¡Correcto!&lt;/b&gt;&lt;/span&gt;</source>
        <translation>&lt;span style=&quot;color:%1&quot;&gt;&lt;b&gt;Correct!&lt;/b&gt;&lt;/span&gt;</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>&lt;span style=&quot;color:#c0392b&quot;&gt;&lt;b&gt;Incorrecto.&lt;/b&gt;&lt;/span&gt; La respuesta correcta es la %1).</source>
        <translation>&lt;span style=&quot;color:#c0392b&quot;&gt;&lt;b&gt;Incorrect.&lt;/b&gt;&lt;/span&gt; The correct answer is %1).</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Nota: %1 / %2</source>
        <translation>Score: %1 / %2</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Aciertos: %1
Fallos: %2
En blanco: %3

Nota: %4 sobre %5</source>
        <translation>Correct: %1
Wrong: %2
Blank: %3

Score: %4 out of %5</translation>
    </message>
</context>
<context>
    <name>DialogoPregunta</name>
    <message>
        <location filename="../src/dialogopregunta.cpp" line="+37"/>
        <source>Editar pregunta %1</source>
        <translation>Edit question %1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Nueva pregunta</source>
        <translation>New question</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Guardar</source>
        <translation>Save</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cancelar</source>
        <translation>Cancel</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>La pregunta tiene cambios sin guardar. ¿Descartarlos?</source>
        <translation>The question has unsaved changes. Discard them?</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Descartar los cambios</source>
        <translation>Discard changes</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+218"/>
        <source>Seguir editando</source>
        <translation>Continue editing</translation>
    </message>
    <message>
        <location line="-208"/>
        <source>Identificación</source>
        <translation>Identification</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Las preguntas con el mismo número dentro de un tema son variantes (pseudorrepetidas) y nunca salen juntas en un examen.</source>
        <translation>Questions with the same number within a topic are variants (pseudo-duplicates) and never appear together in an exam.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Letra de la variante: a, b, c... (vacío si no tiene variantes)</source>
        <translation>Variant letter: a, b, c... (leave empty if there are no variants)</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Activa</source>
        <extracomment>Estado de una pregunta (femenino)</extracomment>
        <translation>Active</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Anulada</source>
        <extracomment>Estado de una pregunta (femenino)</extracomment>
        <translation>Voided</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Obsoleta</source>
        <extracomment>Estado de una pregunta (femenino)</extracomment>
        <translation>Obsolete</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Ya ha salido en exámenes (old)</source>
        <extracomment>«old» es el nombre de esta marca en el banco y en la lista de preguntas</extracomment>
        <translation>Already used in exams (old)</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Marcar para revisar</source>
        <translation>Flag for review</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>p. ej. T1-7</source>
        <extracomment>T1-7 es un código de pregunta (tema 1, número 7): se escribe igual en cualquier idioma</extracomment>
        <translation>e.g. T1-7</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Pregunta equivalente de otro tema (tema-número). No saldrán juntas.</source>
        <translation>Equivalent question from another topic (topic-number). They will never appear together.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Tema:</source>
        <translation>Topic:</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Número:</source>
        <translation>Number:</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Variante:</source>
        <translation>Variant:</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Estado:</source>
        <translation>Status:</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Pseudorrepetida con:</source>
        <translation>Pseudo-duplicate of:</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Enunciado y respuestas</source>
        <translation>Question text and answers</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Código entre líneas &lt;tt&gt;```&lt;/tt&gt;; filas de tabla como &lt;tt&gt;| celda | celda |&lt;/tt&gt;.</source>
        <extracomment>«```» y «|» son marcas de los textos de las preguntas: no se traducen («celda» sí)</extracomment>
        <translation>Code between &lt;tt&gt;```&lt;/tt&gt; lines; table rows as &lt;tt&gt;| cell | cell |&lt;/tt&gt;.</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Marque la respuesta correcta</source>
        <translation>Mark the correct answer</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>El botón marcado indica la respuesta correcta.</source>
        <translation>The selected button indicates the correct answer.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Imagen (opcional, se muestra bajo el enunciado)</source>
        <translation>Image (optional, shown below the question text)</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Cargar…</source>
        <translation>Load…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Quitar</source>
        <translation>Remove</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Referencia y notas</source>
        <translation>Reference and notes</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Véase la página ... del libro de teoría.</source>
        <translation>See page ... of the textbook.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Referencia:</source>
        <translation>Reference:</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Explicación:</source>
        <translation>Explanation:</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Notas internas:</source>
        <translation>Internal notes:</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Vista previa</source>
        <translation>Preview</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Si es la misma idea que una de ellas, guárdela como su variante (mismo número, otra letra) o, si es de otro tema, enlácela en «Pseudorrepetida con».</source>
        <extracomment>«Pseudorrepetida con» es la etiqueta de un campo de este diálogo: tradúzcase igual</extracomment>
        <translation>If it is the same idea as one of them, save it as its variant (same number, another letter) or, if it belongs to another topic, link it in “Pseudo-duplicate of”.</translation>
    </message>
    <message>
        <location line="+62"/>
        <source>Elija un tema.</source>
        <translation>Choose a topic.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>El enunciado está vacío.</source>
        <translation>The question text is empty.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Falta el texto de la opción %1).</source>
        <translation>The text of option %1) is missing.</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>La pregunta no sigue algunas pautas de estilo:

%1

¿Quiere guardarla así?</source>
        <translation>The question does not follow some style guidelines:

%1

Do you want to save it anyway?</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Guardar así</source>
        <translation>Save anyway</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>&lt;b&gt;Pautas de estilo&lt;/b&gt;&lt;br&gt;</source>
        <translation>&lt;b&gt;Style guidelines&lt;/b&gt;&lt;br&gt;</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>&lt;b&gt;Preguntas parecidas&lt;/b&gt; (de otros números): </source>
        <translation>&lt;b&gt;Similar questions&lt;/b&gt; (with other numbers): </translation>
    </message>
    <message>
        <location line="+25"/>
        <source>%1 ya tiene variantes de la a a la z.</source>
        <translation>%1 already has variants from a to z.</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>La pregunta ya está enlazada con %1. ¿Enlazarla con %2 en su lugar?</source>
        <translation>The question is already linked to %1. Link it to %2 instead?</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>(sin imagen)</source>
        <translation>(no image)</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Cargar imagen</source>
        <translation>Load image</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Imágenes (*.png *.jpg *.jpeg *.bmp *.gif)</source>
        <translation>Images (*.png *.jpg *.jpeg *.bmp *.gif)</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>No se pudo leer la imagen.</source>
        <translation>Could not read the image.</translation>
    </message>
</context>
<context>
    <name>DialogoTemas</name>
    <message>
        <location filename="../src/dialogotemas.cpp" line="+25"/>
        <source>Temas</source>
        <translation>Topics</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Nº</source>
        <extracomment>Número del tema</extracomment>
        <translation>No.</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Título</source>
        <translation>Title</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Autor</source>
        <translation>Author</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Referencia al libro</source>
        <translation>Textbook reference</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Preguntas</source>
        <translation>Questions</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Forma que debe tener la referencia al libro en las preguntas del tema (la comprueba la revisión de estilo)</source>
        <translation>Format that the textbook reference must have in the questions of the topic (verified by the style check)</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Añadir tema</source>
        <translation>Add topic</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Eliminar tema</source>
        <translation>Delete topic</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Guardar</source>
        <translation>Save</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cancelar</source>
        <translation>Cancel</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Haga doble clic en una celda para modificarla.</source>
        <translation>Double-click a cell to edit it.</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>Nuevo tema</source>
        <translation>New topic</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>El tema tiene %1 preguntas, que también se borrarán. ¿Continuar?</source>
        <translation>The topic has %1 questions, which will also be deleted. Continue?</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Los números de tema deben ser positivos y distintos.</source>
        <translation>Topic numbers must be positive and different from each other.</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>No se guardó ningún cambio:
%1</source>
        <translation>No changes were saved:
%1</translation>
    </message>
</context>
<context>
    <name>Estilo</name>
    <message>
        <location filename="../src/estilo.cpp" line="+87"/>
        <source>Las preguntas acaban en «?:», con los dos puntos detrás.</source>
        <translation>Questions end in “?:”, with the colon after the question mark.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>El enunciado debe acabar en dos puntos, no en punto.</source>
        <translation>The question text must end in a colon, not a full stop.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>El enunciado debe acabar en dos puntos (o en «?:» si es una pregunta).</source>
        <translation>The question text must end in a colon (or in “?:” if it is a question).</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Falta el «¿» de apertura de la pregunta.</source>
        <translation>The opening “¿” of the question is missing.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Falta el «?» de cierre de la pregunta.</source>
        <translation>The closing “?” of the question is missing.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>El enunciado es largo (%1 caracteres); conviene que no pase de unos 180.</source>
        <translation>The question text is long (%1 characters); it should not exceed about 180.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>El enunciado debe empezar por mayúscula.</source>
        <translation>The question text must begin with a capital letter.</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>La negación va en mayúsculas: «NO», «FALSO».</source>
        <extracomment>«NO» y «FALSO» son las palabras de los enunciados en castellano que la regla quiere en mayúsculas.</extracomment>
        <translation>Negations go in capitals: “NO”, “FALSO”.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>En el texto, los literales van entre comillas tipográficas (“…”), no rectas.</source>
        <translation>In the text, literals go in typographic quotation marks (“…”), not straight ones.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Dentro del código, las comillas deben ser rectas (&quot;).</source>
        <translation>Inside code, quotation marks must be straight (&quot;).</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>La opción %1) debe terminar en punto.</source>
        <translation>Option %1) must end with a full stop.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>La opción %1) debe empezar por mayúscula.</source>
        <translation>Option %1) must begin with a capital letter.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>La opción %1) remite a las demás («Todas las anteriores»…): conviene evitarlo, porque impide barajar las opciones.</source>
        <extracomment>%1 es la letra de la opción (a, b, c o d). «Todas las anteriores» es un ejemplo de opción de las que busca la regla.</extracomment>
        <translation>Option %1) refers to the others (“Todas las anteriores”…): this is best avoided, because it prevents the options from being shuffled.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>En la opción %1), los literales van entre comillas tipográficas (“…”), no rectas.</source>
        <translation>In option %1), literals go in typographic quotation marks (“…”), not straight ones.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Las opciones %1) y %2) son iguales.</source>
        <translation>Options %1) and %2) are the same.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>La correcta es bastante más larga que las demás (%1 caracteres frente a %2 como mucho): puede delatarla.</source>
        <translation>The correct option is considerably longer than the others (%1 characters against at most %2): it may give it away.</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Falta la referencia al libro.</source>
        <translation>The reference to the textbook is missing.</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>«Pág.» se escribe con punto.</source>
        <extracomment>«Pág.» es la abreviatura de las referencias al libro en castellano que busca la regla.</extracomment>
        <translation>“Pág.” is written with a full stop.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>La referencia debe terminar en punto.</source>
        <translation>The reference must end with a full stop.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Falta el número de página.</source>
        <translation>The page number is missing.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Falta concordancia: «Véase la página N» o «Véanse las páginas N y M».</source>
        <extracomment>Las formas entre «» son las de las referencias al libro en castellano que exige la regla.</extracomment>
        <translation>Singular and plural do not agree: “Véase la página N” or “Véanse las páginas N y M”.</translation>
    </message>
    <message>
        <location line="+6"/>
        <location line="+2"/>
        <source>En el tema %1 la referencia tiene la forma %2</source>
        <extracomment>%2 es la forma de la referencia al libro, en castellano y entre «»; no se traduce.</extracomment>
        <translation>In topic %1 the reference has the format %2</translation>
    </message>
    <message>
        <location line="+15"/>
        <location line="+2"/>
        <source>La referencia no sigue la forma habitual: %1</source>
        <extracomment>%1 es la forma de la referencia al libro, en castellano y entre «»; no se traduce.</extracomment>
        <translation>The reference does not follow the usual format: %1</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Enunciado</source>
        <extracomment>Parte de una pregunta: su enunciado.</extracomment>
        <translation>Question text</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Opciones</source>
        <extracomment>Parte de una pregunta: sus cuatro opciones de respuesta.</extracomment>
        <translation>Options</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Referencia</source>
        <extracomment>Parte de una pregunta: la referencia al libro de teoría.</extracomment>
        <translation>Reference</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Cualquiera</source>
        <extracomment>Forma de la referencia al libro que se pide a un tema: cualquiera (no se pide ninguna).</extracomment>
        <translation>Any</translation>
    </message>
    <message>
        <location line="+19"/>
        <location line="+2"/>
        <source>«%1» está mal escrito: es «%2».</source>
        <extracomment>%1 es una palabra mal escrita y %2, cómo se escribe.</extracomment>
        <translation>“%1” is misspelt: it should be “%2”.</translation>
    </message>
</context>
<context>
    <name>Examen</name>
    <message>
        <location filename="../src/pregunta.cpp" line="+14"/>
        <source>Tema %1: %2</source>
        <translation>Topic %1: %2</translation>
    </message>
    <message>
        <location filename="../src/maquetador.cpp" line="+289"/>
        <location line="+721"/>
        <source>Explicación:</source>
        <translation>Explanation:</translation>
    </message>
    <message>
        <location line="-720"/>
        <location line="+721"/>
        <source>Referencia:</source>
        <translation>Reference:</translation>
    </message>
    <message>
        <location line="-710"/>
        <location line="+939"/>
        <source>Plantilla de corrección: %1</source>
        <translation>Answer key: %1</translation>
    </message>
    <message>
        <location line="-876"/>
        <source>Seleccione la respuesta correcta, exclusivamente, en la hoja de respuestas.
No entregue el enunciado.</source>
        <translation>Mark the correct answer on the answer sheet only.
Do not hand in the question paper.</translation>
    </message>
    <message>
        <location line="+226"/>
        <location line="+571"/>
        <source>CON SOLUCIONES</source>
        <translation>WITH SOLUTIONS</translation>
    </message>
    <message>
        <location line="-558"/>
        <location line="+577"/>
        <source>Pregunta contestada correctamente:</source>
        <translation>Question answered correctly:</translation>
    </message>
    <message>
        <location line="-576"/>
        <location line="+577"/>
        <source>+ %1 puntos</source>
        <translation>+ %1 points</translation>
    </message>
    <message>
        <location line="-576"/>
        <location line="+577"/>
        <source>Pregunta contestada incorrectamente:</source>
        <translation>Question answered incorrectly:</translation>
    </message>
    <message>
        <location line="-576"/>
        <location line="+577"/>
        <source>- %1 puntos</source>
        <translation>- %1 points</translation>
    </message>
    <message>
        <location line="-576"/>
        <location line="+577"/>
        <source>Pregunta no contestada:</source>
        <translation>Question not answered:</translation>
    </message>
    <message>
        <location line="-576"/>
        <location line="+577"/>
        <source>0 puntos</source>
        <translation>0 points</translation>
    </message>
    <message>
        <location line="-539"/>
        <location line="+179"/>
        <location line="+314"/>
        <location line="+73"/>
        <location filename="../tests/prueba_idiomas.cpp" line="+15"/>
        <location line="+1"/>
        <location line="+2"/>
        <location line="+3"/>
        <source>Soluciones</source>
        <translation>Solutions</translation>
    </message>
    <message>
        <location line="-560"/>
        <location line="+576"/>
        <source>Estructura de la convocatoria</source>
        <translation>Session structure</translation>
    </message>
    <message>
        <location line="-575"/>
        <location line="+565"/>
        <source>Examen</source>
        <translation>Exam</translation>
    </message>
    <message>
        <location line="-907"/>
        <location line="+942"/>
        <source>Respuestas correctas: %1</source>
        <translation>Correct answers: %1</translation>
    </message>
    <message>
        <location filename="../src/moodle.cpp" line="+144"/>
        <source>Respuesta correcta.</source>
        <translation>Correct answer.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Respuesta incorrecta.</source>
        <translation>Incorrect answer.</translation>
    </message>
    <message>
        <location line="+40"/>
        <source>Tema %1</source>
        <translation>Topic %1</translation>
    </message>
</context>
<context>
    <name>ExamenesAnteriores</name>
    <message>
        <location filename="../src/examenesanteriores.cpp" line="+272"/>
        <source>No se pudo leer el PDF.</source>
        <translation>Could not read the PDF.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Esta versión del programa no lee PDF: abra el examen, copie su texto y péguelo.</source>
        <translation>This version of the program cannot read PDF files: open the exam, copy its text and paste it.</translation>
    </message>
</context>
<context>
    <name>Generador</name>
    <message>
        <location filename="../src/generador.cpp" line="+287"/>
        <source>no queda ninguna otra pregunta del tema %1; %2 es (o es variante de) una de las ya quitadas.</source>
        <extracomment>Va detrás de «Examen», pregunta N: (por eso empieza en minúscula). %2 es el código de una pregunta (T1-7a).</extracomment>
        <translation>there is no other question left from topic %1; %2 is (or is a variant of) one of those already removed.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>%1 está también en otro examen de la convocatoria: no queda ninguna otra del tema %2.</source>
        <translation>%1 is also in another exam in the same exam session: there is no other question left from topic %2.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>%1 salió en una de las últimas convocatorias registradas: no queda ninguna otra del tema %2 sin usar.</source>
        <translation>%1 appeared in one of the last recorded exam sessions: there is no other unused question left from topic %2.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>%1 está marcada «old»: no queda ninguna otra del tema %2 sin usar.</source>
        <extracomment>«old» es la marca de las preguntas que ya han salido en exámenes (la casilla «Ya ha salido en exámenes (old)»).</extracomment>
        <translation>%1 is marked “old”: there is no other unused question left from topic %2.</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Tema %1: no quedan bastantes preguntas sin usar en las últimas convocatorias registradas; se repiten algunas (o variantes suyas).</source>
        <translation>Topic %1: there are not enough questions left that were not used in the last recorded exam sessions; some (or their variants) are repeated.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Tema %1: no quedan bastantes preguntas sin la marca «old»; se usan algunas (o variantes suyas).</source>
        <translation>Topic %1: there are not enough questions left without the “old” mark; some (or their variants) are used.</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>«%1»: </source>
        <extracomment>Comienzo de un aviso sobre un examen entero: %1 es su nombre. Detrás va el aviso («la respuesta correcta es…»).</extracomment>
        <translation>“%1”: </translation>
    </message>
    <message>
        <location line="+13"/>
        <source>la respuesta correcta es la %1) en %2 de %3 preguntas%4.</source>
        <extracomment>Va detrás de «Nombre del examen»: . %1 es una letra (a, b, c o d); %4 está vacío o es «; marque «Barajar las opciones»…».</extracomment>
        <translation>the correct answer is %1) in %2 of %3 questions%4.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>; marque «Barajar las opciones» para repartirla</source>
        <extracomment>«Barajar las opciones» es la casilla de la ventana principal que reparte la correcta entre a)–d).</extracomment>
        <translation>; tick “Shuffle the options” to spread it out</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>«%1», pregunta %2: </source>
        <extracomment>Comienzo de un aviso sobre una pregunta: %1 es el nombre del examen y %2, el número de la pregunta en él.</extracomment>
        <translation>“%1”, question %2: </translation>
    </message>
    <message>
        <location line="+46"/>
        <source>Tema %1: se pidieron %2 preguntas pero solo hay %3 distintas disponibles.</source>
        <translation>Topic %1: %2 questions were requested but only %3 different ones are available.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Tema %1: no hay preguntas suficientes para no repetir entre exámenes; a partir del examen «%2» se repiten algunas.</source>
        <translation>Topic %1: there are not enough questions to avoid repeats between exams; from exam “%2” onwards some are repeated.</translation>
    </message>
</context>
<context>
    <name>Maquetador</name>
    <message>
        <location filename="../src/maquetador.cpp" line="-726"/>
        <source>Activa</source>
        <translation>Active</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Anulada</source>
        <translation>Voided</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Obsoleta</source>
        <translation>Obsolete</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>ya salió en exámenes (old)</source>
        <translation>already used in exams (old)</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>para revisar</source>
        <translation>flagged for review</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>&lt;p&gt;&lt;b&gt;Respuesta correcta:&lt;/b&gt; %1. %2&lt;/p&gt;</source>
        <translation>&lt;p&gt;&lt;b&gt;Correct answer:&lt;/b&gt; %1. %2&lt;/p&gt;</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&lt;p&gt;&lt;b&gt;Explicación:&lt;/b&gt; %1&lt;/p&gt;</source>
        <translation>&lt;p&gt;&lt;b&gt;Explanation:&lt;/b&gt; %1&lt;/p&gt;</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&lt;p&gt;&lt;b&gt;Pseudorrepetida con:&lt;/b&gt; %1 (no saldrán juntas en un examen)&lt;/p&gt;</source>
        <translation>&lt;p&gt;&lt;b&gt;Pseudo-duplicate of:&lt;/b&gt; %1 (they will never appear together in an exam)&lt;/p&gt;</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&lt;p&gt;&lt;b&gt;Notas:&lt;/b&gt; %1&lt;/p&gt;</source>
        <translation>&lt;p&gt;&lt;b&gt;Notes:&lt;/b&gt; %1&lt;/p&gt;</translation>
    </message>
</context>
<context>
    <name>ModeloPreguntas</name>
    <message>
        <location filename="../src/modelopreguntas.cpp" line="+47"/>
        <source> (old)</source>
        <extracomment>Tras el código de las preguntas que ya han salido en exámenes</extracomment>
        <translation> (old)</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>activa</source>
        <extracomment>Estado de una pregunta: se puede usar en los exámenes</extracomment>
        <translation>active</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>anulada</source>
        <translation>voided</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>obsoleta</source>
        <translation>obsolete</translation>
    </message>
    <message>
        <location line="+4"/>
        <source> · revisar</source>
        <extracomment>Tras el estado (activa, anulada, obsoleta) de una pregunta marcada para revisar</extracomment>
        <translation> · review</translation>
    </message>
    <message>
        <location line="+3"/>
        <source> · imagen</source>
        <extracomment>Tras el estado (activa, anulada, obsoleta) de una pregunta con imagen</extracomment>
        <translation> · image</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>Tema</source>
        <translation>Topic</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Código</source>
        <translation>Code</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Estado</source>
        <translation>Status</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enunciado</source>
        <translation>Question text</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Resp.</source>
        <extracomment>Abreviatura de «Respuesta correcta» (cabecera de una columna estrecha)</extracomment>
        <translation>Ans.</translation>
    </message>
</context>
<context>
    <name>Tema</name>
    <message>
        <location filename="../src/pregunta.cpp" line="-5"/>
        <location filename="../tests/prueba_idiomas.cpp" line="-22"/>
        <source>Tema %1: %2</source>
        <translation>Topic %1: %2</translation>
    </message>
</context>
<context>
    <name>Utilidades</name>
    <message>
        <location filename="../src/utilidades.h" line="+87"/>
        <source>Sí</source>
        <extracomment>Botón para responder que sí a una pregunta.</extracomment>
        <translation>Yes</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>No</source>
        <extracomment>Botón para responder que no a una pregunta.</extracomment>
        <translation>No</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Aceptar</source>
        <extracomment>Botón para aceptar el texto escrito.</extracomment>
        <translation>OK</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Cancelar</source>
        <extracomment>Botón para cancelar sin cambiar nada.</extracomment>
        <translation>Cancel</translation>
    </message>
</context>
<context>
    <name>VentanaPrincipal</name>
    <message>
        <location filename="../src/ventanaprincipal.cpp" line="+85"/>
        <source>%1 (%2 preguntas)</source>
        <extracomment>Texto de un reparto: %1 son los temas y su número de preguntas («T1: 3, T2: 2»), %2 el total</extracomment>
        <translation>%1 (%2 questions)</translation>
    </message>
    <message>
        <location line="+81"/>
        <source>Generar examen</source>
        <translation>Generate exam</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Banco de preguntas</source>
        <translation>Question bank</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Abrir la carpeta</source>
        <translation>Open the folder</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>Generar exámenes</source>
        <translation>Generate exams</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Convocatoria</source>
        <extracomment>Convocatoria de examen (Febrero, Junio, Septiembre…): datos de la cabecera de los exámenes</extracomment>
        <translation>Exam session</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Nombre de la asignatura</source>
        <translation>Subject name</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Va en la cabecera de los exámenes. Se guarda en el banco de preguntas: cada banco tiene el de su asignatura.</source>
        <translation>Printed in the exam header. It is saved in the question bank, so each bank keeps the name of its own subject.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Idioma de lo que pone el programa en los exámenes (instrucciones propuestas, puntuación, soluciones, meses de las convocatorias) y en Moodle, y de las pautas de estilo que se comprueban. Se guarda en el banco de preguntas: cada banco tiene el de su asignatura.</source>
        <translation>Language of the text that the program writes in the exams (suggested instructions, scoring, solutions, exam session months) and in Moodle, and of the style guidelines that are checked. It is saved in the question bank, so each bank keeps the language of its own subject.</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Asignatura:</source>
        <translation>Subject:</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Idioma de los exámenes:</source>
        <translation>Exam language:</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Convocatoria:</source>
        <translation>Exam session:</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Año:</source>
        <translation>Year:</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Instrucciones:</source>
        <translation>Instructions:</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Exámenes</source>
        <translation>Exams</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Se genera un examen por cada nombre marcado, en este orden.
Doble clic para cambiar el nombre; arrastre para cambiar el orden.</source>
        <translation>One exam is generated for each ticked name, in this order.
Double-click to change the name; drag to change the order.</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>Preguntas por tema</source>
        <translation>Questions per topic</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Guardar reparto…</source>
        <translation>Save distribution…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Guarda en el banco el número de preguntas de cada tema que hay ahora, con un nombre: sale un botón que lo pone</source>
        <translation>Saves in the bank the number of questions per topic currently set, under a name: a button appears that applies it</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Puntuación</source>
        <translation>Scoring</translation>
    </message>
    <message>
        <location line="-35"/>
        <source>Aña&amp;dir</source>
        <extracomment>Botón: añade un examen a la lista de exámenes que se generan. El &amp; marca la letra del atajo (Alt+letra): no puede ser la de un menú ni la de otro de estos cuatro botones.</extracomment>
        <translation>A&amp;dd</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Añade un examen a la lista</source>
        <translation>Adds an exam to the list</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&amp;Quitar</source>
        <extracomment>Botón: quita de la lista el examen seleccionado. El &amp; marca la letra del atajo (Alt+letra).</extracomment>
        <translation>&amp;Remove</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Quita de la lista el examen seleccionado</source>
        <translation>Removes the selected exam from the list</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&amp;Todos</source>
        <extracomment>Botón: marca todos los exámenes de la lista. El &amp; marca la letra del atajo (Alt+letra).</extracomment>
        <translation>&amp;All</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Marca todos los exámenes de la lista</source>
        <translation>Ticks all the exams in the list</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&amp;Ninguno</source>
        <extracomment>Botón: desmarca todos los exámenes de la lista. El &amp; marca la letra del atajo (Alt+letra).</extracomment>
        <translation>&amp;None</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Desmarca todos los exámenes de la lista</source>
        <translation>Unticks all the exams in the list</translation>
    </message>
    <message>
        <location line="+36"/>
        <source>El examen vale 10 puntos: cada acierto, 10 entre el número de preguntas.</source>
        <translation>The exam is worth 10 points: each correct answer, 10 divided by the number of questions.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Cada fallo resta el 30 %</source>
        <translation>Each wrong answer deducts 30%</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Cada fallo resta el %1 %</source>
        <extracomment>%1 es el porcentaje de un acierto que resta cada fallo para que contestar al azar dé 0 de media («33,3»)</extracomment>
        <translation>Each wrong answer deducts %1%</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Es lo justo para que contestar al azar dé 0 puntos de media: con cuatro opciones se acierta una vez de cada cuatro, y ese acierto compensa los tres fallos.</source>
        <translation>Just enough for random guessing to score 0 points on average: with four options, one guess in four is right, and that correct answer makes up for the three wrong ones.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Los fallos no restan</source>
        <translation>Wrong answers deduct nothing</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Personalizada</source>
        <extracomment>Forma de puntuar: los puntos de cada acierto y de cada fallo los escribe quien hace el examen</extracomment>
        <translation>Custom</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Los puntos de cada acierto y de cada fallo, los que se escriban abajo.</source>
        <translation>The points for each correct and each wrong answer are the ones entered below.</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Acierto:</source>
        <extracomment>Puntos que suma cada respuesta correcta</extracomment>
        <translation>Correct:</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Fallo:</source>
        <extracomment>Puntos que resta cada respuesta incorrecta</extracomment>
        <translation>Wrong:</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Opciones</source>
        <translation>Options</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>No repetir preguntas entre exámenes</source>
        <translation>Do not repeat questions between exams</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Evitar las preguntas de las últimas</source>
        <extracomment>Casilla seguida de un número y del sufijo « convocatorias»: «Evitar las preguntas de las últimas [3] convocatorias»</extracomment>
        <translation>Avoid questions from the last</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Deja para el final las preguntas (y sus variantes) que salieron en las últimas convocatorias registradas, empezando por las más recientes</source>
        <translation>Leaves until last the questions (and their variants) that appeared in the last recorded exam sessions, starting with the most recent</translation>
    </message>
    <message>
        <location line="+6"/>
        <source> convocatorias</source>
        <extracomment>Sufijo del número: «Evitar las preguntas de las últimas [3] convocatorias» (con el espacio delante)</extracomment>
        <translation> exam sessions</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Evitar también las marcadas «old»</source>
        <extracomment>«old» es el nombre de la marca de las preguntas que ya salieron en exámenes anteriores al historial</extracomment>
        <translation>Also avoid those marked “old”</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Deja para el final (aunque antes que las de las convocatorias registradas) las preguntas que ya salieron en exámenes anteriores al historial</source>
        <translation>Leaves until last (though ahead of those from recorded exam sessions) the questions that already appeared in exams from before the history was kept</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Incluir preguntas marcadas para revisar</source>
        <translation>Include questions flagged for review</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Orden aleatorio (si no, por tema y número)</source>
        <translation>Random order (otherwise, by topic and number)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Barajar las opciones (la correcta, repartida entre a)–d))</source>
        <translation>Shuffle the options (correct answer spread across a)–d))</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>En cada examen, la correcta sale en a), b), c) y d) por igual y nunca tres veces seguidas en la misma letra (si se puede).
No se barajan las preguntas con opciones del tipo «Todas las anteriores».</source>
        <translation>In each exam, the correct answer falls on a), b), c) and d) equally often and never three times in a row on the same letter (where possible).
Questions with options such as “All of the above” are not shuffled.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Ajustar a una página por cada 10 preguntas</source>
        <translation>Fit on one page per 10 questions</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Compacta la maqueta (opciones en dos columnas, menos separación, letra menor) hasta que cada examen quepa en una página por cada 10 preguntas, y avisa si no se consigue.
Sin marcar, los exámenes van con la maqueta normal y ocupan las páginas que necesiten.</source>
        <translation>Compacts the layout (options in two columns, less spacing, smaller font) until each exam fits on one page per 10 questions, and warns if it cannot.
Unticked, exams use the normal layout and take as many pages as they need.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>aleatoria</source>
        <extracomment>Lo que pone el campo «Semilla» cuando está vacío (la semilla será aleatoria)</extracomment>
        <translation>random</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Con la misma semilla y la misma configuración se obtienen los mismos exámenes.
Vacía, cada vez salen otros.</source>
        <translation>The same seed and the same settings produce the same exams.
If empty, different exams come out each time.</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Semilla:</source>
        <extracomment>Semilla del generador de números aleatorios</extracomment>
        <translation>Seed:</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Mostrar soluciones</source>
        <translation>Show solutions</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Letra grande</source>
        <extracomment>Versión del examen en letra grande, adaptada para estudiantes con discapacidad visual</extracomment>
        <translation>Large print</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Versión adaptada para estudiantes con discapacidad visual: letra de 16 pt, una opción por línea (enunciados sin cursiva), más separación e imágenes más grandes, sin límite de páginas.
Vale para la vista, la impresión y las exportaciones; los ficheros llevan «_letra_grande» en el nombre.</source>
        <translation>Adapted version for visually impaired students: 16 pt font, one option per line (question text not in italics), more spacing and larger images, with no page limit.
Applies to the preview, printing and exports; the files have “_letra_grande” in their name.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Examen:</source>
        <extracomment>Delante de la lista para elegir cuál de los exámenes generados se ve</extracomment>
        <translation>Exam:</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Cuántas veces es correcta cada letra en este examen</source>
        <translation>How many times each letter is correct in this exam</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>&lt;p style=&quot;color:gray&quot;&gt;Elija cuántas preguntas quiere de cada tema y pulse &lt;b&gt;Generar exámenes&lt;/b&gt;.&lt;/p&gt;</source>
        <translation>&lt;p style=&quot;color:gray&quot;&gt;Choose how many questions you want from each topic and click &lt;b&gt;Generate exams&lt;/b&gt;.&lt;/p&gt;</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Imprimir…</source>
        <translation>Print…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Exportar PDF…</source>
        <translation>Export PDF…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Soluciones PDF…</source>
        <translation>Solutions PDF…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Exportar HTML…</source>
        <translation>Export HTML…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Exportar LaTeX…</source>
        <translation>Export LaTeX…</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Número de la pregunta que se quiere cambiar, en el examen que se ve.
También se puede pulsar con el botón derecho sobre la pregunta.</source>
        <translation>Number of the question to replace, in the exam being shown.
You can also right-click the question.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Pregunta:</source>
        <extracomment>Delante del número de la pregunta del examen que se quiere cambiar</extracomment>
        <translation>Question:</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Cambiar por otra</source>
        <extracomment>Botón: cambia esa pregunta del examen por otra del mismo tema</extracomment>
        <translation>Replace with another</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Sustituye la pregunta por otra del mismo tema, en el mismo sitio y con la correcta en la misma letra, con las mismas reglas que al generar. El resto de los exámenes no cambia.</source>
        <translation>Replaces the question with another from the same topic, in the same place and with the correct answer on the same letter, following the same rules as when generating. The rest of the exams do not change.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Deshacer el cambio</source>
        <translation>Undo the change</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Registrar convocatoria…</source>
        <translation>Record exam session…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Practicar este examen…</source>
        <translation>Practise this exam…</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Todas</source>
        <extracomment>Filtro por estado de las preguntas (en femenino: «las preguntas»): todas</extracomment>
        <translation>All</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Activas</source>
        <extracomment>Filtro por estado de las preguntas: las activas</extracomment>
        <translation>Active</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Anuladas</source>
        <extracomment>Filtro por estado de las preguntas: las anuladas</extracomment>
        <translation>Voided</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Obsoletas</source>
        <extracomment>Filtro por estado de las preguntas: las obsoletas</extracomment>
        <translation>Obsolete</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Para revisar</source>
        <extracomment>Filtro por estado de las preguntas: las marcadas para revisar</extracomment>
        <translation>Flagged for review</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Con imagen</source>
        <extracomment>Filtro por estado de las preguntas: las que tienen imagen</extracomment>
        <translation>With image</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Buscar texto o código (T1-22b)…</source>
        <translation>Search text or code (T1-22b)…</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tema:</source>
        <translation>Topic:</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Estado:</source>
        <extracomment>Estado de las preguntas (activa, anulada, obsoleta…), para filtrar la tabla</extracomment>
        <translation>Status:</translation>
    </message>
    <message>
        <location line="+39"/>
        <source>Nueva…</source>
        <extracomment>Botón: nueva pregunta</extracomment>
        <translation>New…</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Editar…</source>
        <extracomment>Botón: editar la pregunta seleccionada</extracomment>
        <translation>Edit…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Duplicar como variante…</source>
        <translation>Duplicate as variant…</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Eliminar</source>
        <extracomment>Botón: eliminar la pregunta seleccionada</extracomment>
        <translation>Delete</translation>
    </message>
    <message>
        <location line="+14"/>
        <location filename="../tests/prueba_idiomas.cpp" line="-1"/>
        <location line="+9"/>
        <source>&amp;Archivo</source>
        <translation>&amp;File</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&amp;Abrir banco de preguntas…</source>
        <translation>&amp;Open question bank…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&amp;Nuevo banco de preguntas…</source>
        <translation>&amp;New question bank…</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&amp;Importar preguntas (JSON)…</source>
        <translation>&amp;Import questions (JSON)…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&amp;Comprobar un JSON…</source>
        <translation>&amp;Check a JSON file…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dice los errores de un JSON de preguntas, con su línea, sin importarlo</source>
        <translation>Lists the errors in a JSON file of questions, with their line, without importing it</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Guardar un JSON de e&amp;jemplo…</source>
        <translation>Save an e&amp;xample JSON…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Un JSON con unas preguntas inventadas, para ver cómo se escribe uno</source>
        <translation>A JSON file with some made-up questions, to see how one is written</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&amp;Exportar preguntas (JSON)…</source>
        <translation>&amp;Export questions (JSON)…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Exportar a &amp;Moodle (XML)…</source>
        <translation>Export to &amp;Moodle (XML)…</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Actuali&amp;zar el JSON del banco…</source>
        <translation>&amp;Update the bank’s JSON…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Sobrescribe el JSON enlazado con el banco (la primera vez pide cuál es) con el contenido de la base de datos</source>
        <translation>Overwrites the JSON linked to the bank (the first time, it asks which one) with the contents of the database</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&amp;Mantener el JSON del banco al día</source>
        <translation>&amp;Keep the bank’s JSON up to date</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Tras cada cambio en el banco (pregunta o tema guardado o borrado) se reescribe el JSON enlazado, si antes coincidía con la base de datos. Un JSON que ya no coincida (por ejemplo, recién generado desde el .docx) no se toca: se avisa.</source>
        <translation>After each change to the bank (a question or topic saved or deleted), the linked JSON is rewritten if it matched the database beforehand. A JSON that no longer matches (for example, one freshly generated from the .docx) is left untouched, and a warning is shown.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>&amp;Historial de convocatorias…</source>
        <translation>Exam session &amp;history…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&amp;Registrar exámenes anteriores…</source>
        <translation>&amp;Record previous exams…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Registra en el historial los exámenes de una convocatoria hecha fuera del programa (de sus PDF o su texto), para que se eviten sus preguntas al generar</source>
        <translation>Records in the history the exams of an exam session held outside the program (from their PDFs or their text), so that their questions are avoided when generating</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&amp;Salir</source>
        <translation>&amp;Quit</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&amp;Editar</source>
        <translation>&amp;Edit</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&amp;Temas…</source>
        <translation>&amp;Topics…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&amp;Nueva pregunta…</source>
        <translation>&amp;New question…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&amp;Revisar el estilo del banco…</source>
        <translation>&amp;Check the bank’s style…</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&amp;Idioma</source>
        <extracomment>Menú para elegir el idioma de la interfaz del programa (menús, diálogos, ayuda)</extracomment>
        <translation>&amp;Language</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>El del sistema (%1)</source>
        <extracomment>Opción del menú Idioma: el idioma del sistema; %1 es su nombre («Español», «English»)</extracomment>
        <translation>System default (%1)</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>A&amp;yuda</source>
        <translation>&amp;Help</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&amp;Manual de uso</source>
        <translation>&amp;User manual</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&amp;Acerca de…</source>
        <translation>&amp;About…</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>No se pudo abrir la base de datos
%1

%2</source>
        <translation>Could not open the database
%1

%2</translation>
    </message>
    <message>
        <location line="+54"/>
        <source>Elija el banco de preguntas (fichero .db) con el que quiere trabajar.</source>
        <translation>Choose the question bank (.db file) you want to work with.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>No se encuentra el banco de preguntas
%1

Elija dónde está o cree uno nuevo.</source>
        <translation>The question bank cannot be found
%1

Choose where it is or create a new one.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Abrir banco…</source>
        <translation>Open bank…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Nuevo banco…</source>
        <translation>New bank…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Salir</source>
        <translation>Quit</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Abrir banco de preguntas</source>
        <translation>Open question bank</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Bases de datos SQLite (*.db *.sqlite);;Todos (*)</source>
        <translation>SQLite databases (*.db *.sqlite);;All files (*)</translation>
    </message>
    <message>
        <location line="+9"/>
        <location line="+16"/>
        <source>Nuevo banco de preguntas</source>
        <translation>New question bank</translation>
    </message>
    <message>
        <location line="-13"/>
        <source>Bases de datos SQLite (*.db)</source>
        <translation>SQLite databases (*.db)</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Esa base de datos está abierta; elija otro nombre.</source>
        <translation>That database is already open; choose another name.</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Nombre de la asignatura del banco (va en la cabecera de los exámenes):</source>
        <translation>Name of the bank’s subject (printed in the exam header):</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>El banco nuevo está vacío. ¿Quiere importar ahora preguntas de un JSON?</source>
        <translation>The new bank is empty. Do you want to import questions from a JSON file now?</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Importar preguntas</source>
        <translation>Import questions</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>¿Cómo quiere importar las preguntas?

Añadir/actualizar: las preguntas con el mismo tema, número y variante se sobrescriben; el resto se conservan.
Reemplazar todo: se borran antes todos los temas y preguntas (antes de borrarlos se guarda una copia de seguridad de la base de datos en la carpeta «copias», junto a ella).</source>
        <translation>How do you want to import the questions?

Add/update: questions with the same topic, number and variant are overwritten; the rest are kept.
Replace all: all topics and questions are deleted first (before deleting them, a backup of the database is saved in the “copias” folder next to it).</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Añadir/actualizar</source>
        <translation>Add/update</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Reemplazar todo</source>
        <translation>Replace all</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+932"/>
        <source>Cancelar</source>
        <translation>Cancel</translation>
    </message>
    <message>
        <location line="-924"/>
        <source>Error al importar:
%1</source>
        <translation>Import error:
%1</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Se han importado %1 preguntas.</source>
        <translation>%1 questions have been imported.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>

Antes de borrar el banco se ha guardado una copia de seguridad en:
%1</source>
        <translation>

Before the bank was cleared, a backup was saved in:
%1</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Comprobar un JSON de preguntas</source>
        <translation>Check a JSON file of questions</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Guardar un JSON de ejemplo</source>
        <translation>Save an example JSON</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>ejemplo_preguntas.json</source>
        <translation>example_questions.json</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>No se pudo guardar el ejemplo en
%1</source>
        <translation>The example could not be saved in
%1</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Se ha guardado el ejemplo en
%1

Ábralo con un editor de textos; en uno que entienda los esquemas JSON, como VS Code, se ven los errores mientras se escribe. El formato está explicado en la ayuda (F1, «Formato del JSON»).</source>
        <translation>The example has been saved in
%1

Open it with a text editor; in one that understands JSON schemas, such as VS Code, errors are shown as you type. The format is explained in the help (F1, “JSON format”).</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Exportar preguntas</source>
        <translation>Export questions</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Preguntas exportadas a %1</source>
        <translation>Questions exported to %1</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>%1 preguntas exportadas a %2 (en Moodle: Banco de preguntas → Importar, formato «Moodle XML»)</source>
        <translation>%1 questions exported to %2 (in Moodle: Question bank → Import, “Moodle XML” format)</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>JSON del banco de preguntas</source>
        <translation>Question bank JSON</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>El JSON del banco ya coincide con la base de datos:
%1</source>
        <translation>The bank’s JSON already matches the database:
%1</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Actualizar el JSON del banco</source>
        <translation>Update the bank’s JSON</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>El JSON del banco no coincide con la base de datos:
%1

%2

¿Sobrescribirlo con el contenido de la base de datos?</source>
        <translation>The bank’s JSON does not match the database:
%1

%2

Overwrite it with the contents of the database?</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>No se pudo escribir el JSON:
%1</source>
        <translation>Could not write the JSON:
%1</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>JSON del banco actualizado: %1</source>
        <translation>Bank’s JSON updated: %1</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>El JSON del banco no coincide con la base de datos: actualizarlo…</source>
        <translation>The bank’s JSON does not match the database: update it…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>%1

%2

Pulse para sobrescribir el JSON con el contenido de la base de datos.</source>
        <translation>%1

%2

Click to overwrite the JSON with the contents of the database.</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>No se pudo actualizar el JSON del banco:
%1</source>
        <translation>Could not update the bank’s JSON:
%1</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Guardar y siguiente con avisos</source>
        <translation>Save and next with warnings</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Acerca de</source>
        <translation>About</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>&lt;h3&gt;Generador de exámenes&lt;/h3&gt;&lt;p&gt;Versión %1&lt;/p&gt;&lt;p&gt;Desarrollado por Manuel Arias Calleja&lt;/p&gt;&lt;p&gt;Genera exámenes tipo test eligiendo al azar preguntas de la base de datos, con el número de preguntas que se indique de cada tema.&lt;/p&gt;&lt;p&gt;Nunca pone en un mismo examen dos variantes de una pregunta (3a/3b) ni pseudorrepetidas, y puede evitar que se repitan preguntas entre exámenes.&lt;/p&gt;&lt;p&gt;Base de datos: &lt;tt&gt;%2&lt;/tt&gt;&lt;/p&gt;&lt;p&gt;Configuración: &lt;tt&gt;%3&lt;/tt&gt;&lt;/p&gt;&lt;p&gt;Qt %4&lt;/p&gt;</source>
        <translation>&lt;h3&gt;Exam Generator&lt;/h3&gt;&lt;p&gt;Version %1&lt;/p&gt;&lt;p&gt;Developed by Manuel Arias Calleja&lt;/p&gt;&lt;p&gt;Generates multiple-choice exams by picking questions from the database at random, with the number of questions specified for each topic.&lt;/p&gt;&lt;p&gt;It never puts two variants of a question (3a/3b) or pseudo-duplicates in the same exam, and it can avoid repeating questions between exams.&lt;/p&gt;&lt;p&gt;Database: &lt;tt&gt;%2&lt;/tt&gt;&lt;/p&gt;&lt;p&gt;Settings: &lt;tt&gt;%3&lt;/tt&gt;&lt;/p&gt;&lt;p&gt;Qt %4&lt;/p&gt;</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>Todos los temas</source>
        <translation>All topics</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Base de datos: %1  ·  %2 temas, %3 preguntas (%4 activas)</source>
        <translation>Database: %1  ·  %2 topics, %3 questions (%4 active)</translation>
    </message>
    <message>
        <location line="+27"/>
        <source>de %1</source>
        <extracomment>Junto al número de preguntas que se piden de un tema: «[3] de 12» (12 preguntas disponibles)</extracomment>
        <translation>of %1</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Preguntas distintas disponibles (las variantes a, b, c... cuentan como una)</source>
        <translation>Distinct questions available (variants a, b, c... count as one)</translation>
    </message>
    <message>
        <location line="+156"/>
        <source>Generador de exámenes</source>
        <translation>Exam Generator</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Generador de exámenes - %1</source>
        <translation>Exam Generator - %1</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>No se pudo guardar el nombre de la asignatura en el banco:
%1</source>
        <translation>Could not save the subject name in the bank:
%1</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>No se pudo guardar el idioma de los exámenes en el banco:
%1</source>
        <translation>Could not save the exam language in the bank:
%1</translation>
    </message>
    <message>
        <location line="+41"/>
        <source>El programa estará en %1 la próxima vez que se abra.</source>
        <extracomment>%1 es el nombre del idioma elegido, en ese idioma («English»)</extracomment>
        <translation>The program will be in %1 the next time it starts.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Reiniciar ahora</source>
        <translation>Restart now</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Más tarde</source>
        <translation>Later</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>%1
Botón derecho: sustituirlo o quitarlo</source>
        <extracomment>%1 es el reparto: «T1: 3, T2: 2 (5 preguntas)»</extracomment>
        <translation>%1
Right-click: replace or remove it</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Ninguna</source>
        <extracomment>Botón de reparto: ninguna pregunta de ningún tema (pone todos los temas a 0)</extracomment>
        <translation>None</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Poner todos los temas a 0</source>
        <translation>Set all topics to 0</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Sustituir «%1» por el reparto de ahora</source>
        <translation>Replace “%1” with the current distribution</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Quitar «%1»</source>
        <translation>Remove “%1”</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>¿Quitar el reparto «%1»?</source>
        <translation>Remove the distribution “%1”?</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>No se pudo guardar el cambio:
%1</source>
        <translation>Could not save the change:
%1</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Indique antes cuántas preguntas quiere de cada tema.</source>
        <translation>First specify how many questions you want from each topic.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Guardar reparto</source>
        <translation>Save distribution</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>%1

Nombre del reparto (el texto de su botón):</source>
        <translation>%1

Name of the distribution (the text of its button):</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Ya hay un reparto «%1»: %2.

¿Sustituirlo por este?</source>
        <translation>There is already a distribution “%1”: %2.

Replace it with this one?</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>No se pudo guardar el reparto:
%1</source>
        <translation>Could not save the distribution:
%1</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>&lt;b&gt;Total: %1 preguntas&lt;/b&gt;</source>
        <translation>&lt;b&gt;Total: %1 questions&lt;/b&gt;</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>Nuevo examen</source>
        <extracomment>Nombre que se propone para un examen nuevo de la lista (se cambia en el momento)</extracomment>
        <translation>New exam</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>¿Quitar «%1» de la lista de exámenes?</source>
        <translation>Remove “%1” from the list of exams?</translation>
    </message>
    <message>
        <location line="+37"/>
        <source>Indique cuántas preguntas quiere de cada tema.</source>
        <translation>Specify how many questions you want from each topic.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Marque en la lista los exámenes que quiere generar.</source>
        <translation>Tick the exams you want to generate in the list.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Hay un examen marcado sin nombre.</source>
        <translation>A ticked exam has no name.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Hay dos exámenes marcados con el mismo nombre.</source>
        <translation>Two ticked exams have the same name.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Generados con la semilla fija %1: cada vez salen los mismos exámenes. Borre el campo «Semilla» para que salgan otros.</source>
        <translation>Generated with the fixed seed %1: the same exams come out every time. Clear the “Seed” field to get different ones.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>No hay ninguna convocatoria registrada en el historial, así que no se evitan las preguntas de las anteriores. Si se hicieron fuera del programa, regístrelas con Archivo → Registrar exámenes anteriores…</source>
        <translation>There is no exam session recorded in the history, so questions from previous ones are not avoided. If they were held outside the program, record them with File → Record previous exams…</translation>
    </message>
    <message>
        <location line="+40"/>
        <source>Semilla: %1</source>
        <extracomment>Línea de información del examen generado: semilla del generador de números aleatorios</extracomment>
        <translation>Seed: %1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source> (fija: cada vez salen los mismos exámenes)</source>
        <extracomment>Se añade tras «Semilla: %1» (con el espacio delante)</extracomment>
        <translation> (fixed: the same exams come out every time)</translation>
    </message>
    <message>
        <location line="+3"/>
        <source> (con una pregunta cambiada a mano)</source>
        <extracomment>Se añade a la línea de información del examen (con el espacio delante)</extracomment>
        <translation> (with one question changed by hand)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source> (con %1 preguntas cambiadas a mano)</source>
        <translation> (with %1 questions changed by hand)</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>  ·  Letra grande (versión adaptada, sin límite de páginas)</source>
        <extracomment>Se añade a la línea de información del examen (con el separador delante)</extracomment>
        <translation>  ·  Large print (adapted version, no page limit)</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>  ·  Sin límite de páginas</source>
        <extracomment>Se añade a la línea de información del examen (con el separador delante): no se ajusta a una página por cada 10 preguntas</extracomment>
        <translation>  ·  No page limit</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Algún examen no cabe en %1 ni con la maqueta más compacta: acorte las instrucciones o quite preguntas.</source>
        <extracomment>%1 es «una página» o «N páginas»</extracomment>
        <translation>At least one exam does not fit on %1, even with the most compact layout: shorten the instructions or remove questions.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>una página</source>
        <translation>one page</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>%1 páginas</source>
        <translation>%1 pages</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>  ·  Maqueta compacta (nivel %1 de %2)</source>
        <extracomment>Se añade a la línea de información del examen (con el separador delante)</extracomment>
        <translation>  ·  Compact layout (level %1 of %2)</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Escriba esta semilla en Opciones para volver a obtener los mismos exámenes.
Si se han cambiado preguntas a mano, ya no los reproduce: regístrelos para conservarlos.
Con «Ajustar a una página por cada 10 preguntas» (en Opciones), la maqueta se compacta (opciones en dos columnas, menos separación, letra menor) hasta que cada examen cabe en sus páginas; con «Letra grande» no se compacta.</source>
        <translation>Enter this seed in Options to get the same exams again.
If questions have been changed by hand, the seed no longer reproduces them: record them to keep them.
With “Fit on one page per 10 questions” (in Options), the layout is compacted (options in two columns, less spacing, smaller font) until each exam fits on its pages; with “Large print” it is not compacted.</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Correctas: %1</source>
        <extracomment>Cuántas veces es correcta cada letra en el examen; %1 es la lista: «A 3 · B 2 · C 3 · D 2»</extracomment>
        <translation>Correct: %1</translation>
    </message>
    <message>
        <location line="+45"/>
        <source>No se pudieron escribir estos ficheros:
%1</source>
        <translation>Could not write these files:
%1</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Fichero guardado en %1</source>
        <translation>File saved to %1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>%1 ficheros guardados en %2</source>
        <translation>%1 files saved to %2</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Vista previa de impresión</source>
        <translation>Print preview</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>  ·  Exportados y sin registrar</source>
        <extracomment>Se añade a la línea de información de los exámenes (con el separador delante)</extracomment>
        <translation>  ·  Exported but not recorded</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Los exámenes se han exportado pero no se han registrado: si son los definitivos, regístrelos para que sus preguntas se eviten en las convocatorias siguientes.</source>
        <translation>The exams have been exported but not recorded: if they are the final ones, record them so that their questions are avoided in later exam sessions.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Los exámenes de %1 se han exportado, pero no se han registrado en el historial. Si sigue, se perderán y sus preguntas no se evitarán en las convocatorias siguientes.

Si son los definitivos, regístrelos antes.</source>
        <translation>The exams for %1 have been exported but not recorded in the history. If you continue, they will be lost and their questions will not be avoided in later exam sessions.

If they are the final ones, record them first.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Registrarlos…</source>
        <translation>Record them…</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>No registrarlos</source>
        <translation>Do not record them</translation>
    </message>
    <message>
        <location line="+34"/>
        <source>Exportar exámenes a PDF</source>
        <translation>Export exams to PDF</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Exportar soluciones a PDF</source>
        <translation>Export solutions to PDF</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Con soluciones (plantilla de corrección y, en cada pregunta, la correcta, la explicación y la referencia)</source>
        <translation>With solutions (answer key and, for each question, the correct answer, the explanation and the reference)</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Exportar a HTML (editable en Word / LibreOffice)</source>
        <translation>Export to HTML (editable in Word / LibreOffice)</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Exportar a LaTeX</source>
        <translation>Export to LaTeX</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>compílelos dos veces con pdflatex</source>
        <translation>compile them twice with pdflatex</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Indique la convocatoria (Febrero, Junio…).</source>
        <translation>Enter the exam session (February, June…).</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Se guardarán en el historial los exámenes de %1: %2.

Al generar, se evitarán sus preguntas, y desde Archivo → Historial de convocatorias se podrán volver a cargar tal como están.</source>
        <translation>The exams for %1 will be saved in the history: %2.

Their questions will be avoided when generating, and they can be loaded again exactly as they are from File → Exam session history.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>

Ya había registrados con el mismo nombre, y se sustituirán: %1.</source>
        <translation>

Some with the same name had already been recorded and will be replaced: %1.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Registrar convocatoria</source>
        <translation>Record exam session</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>No se pudo registrar la convocatoria:
%1</source>
        <translation>Could not record the exam session:
%1</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+24"/>
        <source>Registrados en el historial los exámenes de %1</source>
        <translation>Exams for %1 recorded in the history</translation>
    </message>
    <message>
        <location line="+42"/>
        <source>Registrada el %1</source>
        <extracomment>Origen de los exámenes cargados del historial: la convocatoria se registró el día %1</extracomment>
        <translation>Recorded on %1</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>No hay ninguna otra pregunta del tema %1 que pueda ir en lugar de la %2 (%3): las demás son variantes o pseudorrepetidas de otras del examen.</source>
        <translation>There is no other question from topic %1 that can replace question %2 (%3): the others are variants or pseudo-duplicates of other questions in the exam.</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>«%1», pregunta %2: %3 en lugar de %4</source>
        <extracomment>%1 es el nombre del examen, %2 el número de la pregunta en él, %3 y %4 los códigos de la nueva y de la quitada (T2-15b)</extracomment>
        <translation>“%1”, question %2: %3 instead of %4</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Deshecho el cambio de la pregunta %1 de «%2»</source>
        <translation>Change to question %1 of “%2” undone</translation>
    </message>
    <message>
        <location line="+30"/>
        <source>Cambiar la pregunta %1 por otra del tema %2</source>
        <translation>Replace question %1 with another from topic %2</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Deshacer el último cambio de pregunta</source>
        <translation>Undo the last question change</translation>
    </message>
    <message>
        <location line="+35"/>
        <source>1 pregunta</source>
        <translation>1 question</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>%1 preguntas</source>
        <translation>%1 questions</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>&lt;p&gt;&lt;b&gt;Ha salido en:&lt;/b&gt; %1&lt;/p&gt;&lt;/body&gt;</source>
        <translation>&lt;p&gt;&lt;b&gt;Appeared in:&lt;/b&gt; %1&lt;/p&gt;&lt;/body&gt;</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Primero cree algún tema (Editar → Temas).</source>
        <translation>First create a topic (Edit → Topics).</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>No se pudo guardar la pregunta:
%1</source>
        <translation>Could not save the question:
%1</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Guardar y nueva</source>
        <translation>Save and new</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Guardar y siguiente</source>
        <translation>Save and next</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Era la última pregunta de la lista.</source>
        <translation>That was the last question in the list.</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>%1 ya tiene variantes de la a a la z.</source>
        <translation>%1 already has variants from a to z.</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>¿Eliminar definitivamente la pregunta %1?

%2

(Si solo quiere que deje de salir en los exámenes, edítela y márquela como anulada.)</source>
        <translation>Permanently delete question %1?

%2

(If you only want it to stop appearing in exams, edit it and mark it as voided.)</translation>
    </message>
</context>
<context>
    <name>main</name>
    <message>
        <location filename="../src/main.cpp" line="+31"/>
        <source>Indique el JSON que quiere comprobar: GeneradorExamenes --comprobar preguntas.json
</source>
        <translation>Give the JSON file to check: GeneradorExamenes --comprobar questions.json
</translation>
    </message>
    <message>
        <location line="+46"/>
        <source>Comprueba los JSON de preguntas indicados, sin abrir el programa: escribe sus errores y avisos, con su línea, y sale con 1 si hay errores.</source>
        <translation>Checks the given JSON files of questions without opening the program: prints their errors and warnings, with their line, and exits with 1 if there are errors.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Fichero SQLite de preguntas (se crea si no existe); con --comprobar, los JSON que se comprueban</source>
        <translation>SQLite question file (created if it does not exist); with --comprobar, the JSON files to check</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Generador de exámenes</source>
        <translation>Exam Generator</translation>
    </message>
    <message>
        <location line="-12"/>
        <source>Generador de exámenes tipo test</source>
        <translation>Multiple-choice exam generator</translation>
    </message>
</context>
</TS>
