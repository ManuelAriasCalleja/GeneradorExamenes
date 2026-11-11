# Redactar preguntas para el generador de exámenes

Esta guía explica cómo escribir preguntas tipo test para importarlas en **GeneradorExamenes**: el formato del fichero,
cómo se escribe el enunciado (con código, tablas y números de línea) y las reglas que comprueba el programa. Sirve para
quien las escriba a mano y, sobre todo, para pedírselas a un asistente como Claude: basta con darle el enlace a esta
guía (la versión en texto, la más fácil de leer para un programa, está en
<https://raw.githubusercontent.com/ManuelAriasCalleja/GeneradorExamenes/main/docs/redactar_preguntas.md>).

Los ejemplos de esta guía son inventados.

## Qué hay que entregar

Un fichero **JSON** en UTF-8 con las preguntas, con el formato que describe el esquema
<https://raw.githubusercontent.com/ManuelAriasCalleja/GeneradorExamenes/main/recursos/banco.schema.json> (su
dirección va en el campo `$schema`, para que los editores lo comprueben mientras se escribe). Se importa en el programa
con *Archivo → Importar preguntas (JSON)…*.

Antes de entregarlo, hay que comprobarlo con el propio programa, y corregir todo lo que diga hasta que no haya ni
errores ni avisos:

```sh
GeneradorExamenes --comprobar preguntas.json
```

Escribe cada problema con su línea y su pregunta (`preguntas.json:12:5: error: tema 3, pregunta 12b: …`) y sale con 1
si hay errores. Desde el programa es *Archivo → Comprobar un JSON…*.

## La forma del JSON

```json
{
  "$schema": "https://raw.githubusercontent.com/ManuelAriasCalleja/GeneradorExamenes/main/recursos/banco.schema.json",
  "asignatura": "Programación inventada",
  "idioma": "es",
  "temas": [
    {
      "numero": 2,
      "titulo": "Bucles",
      "preguntas": [
        {
          "numero": 14,
          "variante": "a",
          "enunciado": "¿Qué escribe este fragmento?:\n```java\nint total = 0;\nfor (int i = 1; i <= 3; i++)\n    total += i;\nSystem.out.println(total);\n```",
          "opciones": ["3.", "6.", "0.", "1."],
          "correcta": "B",
          "referencia": "Véase la página 40 del libro de teoría.",
          "explicacion": "El bucle suma 1, 2 y 3, así que escribe 6. 3 es el número de vueltas, no la suma; 0, el valor inicial de total, que cambia en cada vuelta; y 1, lo que suma solo la primera vuelta.",
          "revisar": true
        }
      ]
    }
  ]
}
```

| Campo | Qué es |
|---|---|
| `asignatura`, `idioma` | Nombre de la asignatura e idioma de los exámenes (`"es"` o `"en"`). Solo se toman si el banco aún no los tiene. |
| `temas[].numero`, `titulo` | El tema. Si ya está en el banco, las preguntas se añaden a él y conserva su título si no se pone. |
| `temas[].formato_referencia` | Forma de la referencia que se pide en el tema: `"vease"` («Véase la página N del libro de teoría.»), `"pag"` («Pág. N libro (subsección X.Y).») o `""`. Si no se pone, la del banco. |
| `numero`, `variante` | Número de la pregunta en su tema y letra de la variante (`"a"`, `"b"`… o `""`). Las variantes de un mismo número nunca salen juntas en un examen: son la misma pregunta vista de otra forma. |
| `enunciado` | El texto de la pregunta (ver abajo). Obligatorio. |
| `opciones` | Exactamente cuatro textos, a), b), c) y d), sin la letra. Obligatorio. |
| `correcta` | La letra de la correcta: `"A"`, `"B"`, `"C"` o `"D"`. Obligatorio. |
| `referencia` | Dónde lo explica el libro, con la forma del tema. Sale en las soluciones. |
| `explicacion` | Por qué la correcta es la correcta y por qué falla cada una de las otras. Sale en las soluciones y en Moodle. |
| `notas` | Notas internas, que no salen en ningún examen. |
| `pseudo_con` | Otra pregunta equivalente de otro tema con la que no debe salir: `"T1-7"` (tema 1, número 7). |
| `revisar` | `true` para marcarla como pendiente de revisar (por ejemplo, las que propone un asistente). |
| `old` | `true` si ya ha salido en exámenes. |
| `estado` | `"activa"` (la de por defecto), `"anulada"` u `"obsoleta"`. |
| `imagen` | Una imagen PNG o JPG en base64. Es más fácil ponerla después desde el editor de preguntas. |

Al importar con **Añadir/actualizar**, una pregunta con el mismo tema, número y variante que otra del banco la
**sustituye**. Para añadir preguntas nuevas hay que usar números libres: los del banco están en su JSON enlazado (el
fichero `.json` que está junto a la base de datos del banco) o en la pestaña *Banco de preguntas* del programa.

Lo que más se olvida en un JSON: los textos van entre comillas rectas (`"`); dentro de un texto, un salto de línea se
escribe `\n`, unas comillas `\"` y una barra invertida `\\`; `true`, `false`, `null` y los números van sin comillas;
no hay coma detrás del último elemento de una lista ni del último campo; y no se pueden poner comentarios.

## El enunciado

Es texto normal, en uno o varios párrafos (separados por `\n`). No es Markdown: `**así**` o `` `así` `` salen tal
cual. Admite además:

**Tablas**, una fila por línea con las celdas entre barras:

```
| Entrada | Salida |
| 0       | 1      |
```

**Código**, entre dos líneas que solo tengan tres comillas invertidas. Se imprime en un recuadro gris, en letra de
ancho fijo, respetando los espacios. La línea que lo abre puede decir **el lenguaje**, que hace que salga resaltado
(palabras clave en negrita, comentarios en cursiva y cadenas en otro color), y **`numerado`**, para que lleve los números
de línea:

````
¿Qué escribe este método?:
```java numerado
static int doble(int x) {
    return 2 * x;   // el doble
}
```
````

En el JSON, el enunciado de arriba es `"¿Qué escribe este método?:\n```java numerado\nstatic int doble(int x) {\n    return 2 * x;   // el doble\n}\n```"`.

| Se escribe | Para |
|---|---|
| ```` ```xml ```` | XML, y también XML Schema, XSLT, WSDL o mensajes SOAP (se admiten `xsd`, `xsl`, `xslt`, `wsdl` y `soap`, que salen como XML). |
| ```` ```dtd ```` | Una DTD, o un documento XML con su DTD interna. |
| ```` ```html ```` | HTML. |
| ```` ```json ```` | JSON. |
| ```` ```yaml ```` | YAML (también `yml`). |
| ```` ```java ```` | Java. |
| ```` ```idl ```` | El IDL de CORBA. |
| ```` ```c ```` | C. |
| ```` ```python ```` | Python. |
| ```` ```sql ```` | SQL. |
| ```` ```texto ```` o ```` ``` ```` | Código o texto de ancho fijo sin resaltar (pseudocódigo, una salida por pantalla, un fichero de datos…). |

- `numerado` va detrás del lenguaje (```` ```xml numerado ````) o solo (```` ``` numerado ````). Úsalo solo cuando el
  enunciado o las opciones hablen de líneas concretas («¿Qué hace la línea 3?»); si no, sobra.
- Detrás de ```` ``` ```` solo pueden ir el lenguaje y `numerado`. Un lenguaje que no está en la tabla, una palabra de
  más o un bloque sin la línea ```` ``` ```` que lo cierra dan un aviso al comprobar.
- Las líneas, de **70 caracteres como mucho**: las más largas se parten y se leen peor. Y que el código sea corto
  (unas 15 líneas): la pregunta tiene que caber en el examen.
- Sangría con **espacios**, no con tabuladores.
- Dentro del código, comillas rectas (`"` y `'`), nunca tipográficas (“ ” ‘ ’ « »).
- Con caracteres ASCII, letras con tilde y eñes. Otros símbolos (flechas, letras griegas…) pueden dar problemas al
  compilar los exámenes en LaTeX.
- Las opciones no pueden llevar bloques de código: el código va en el enunciado y las opciones hablan de él con
  texto normal («La línea 3.», «El elemento cita.»).

## Las opciones

- Exactamente cuatro, sin la letra delante («Mercurio.», no «a) Mercurio.»).
- Empiezan por mayúscula y terminan en punto.
- Todas distintas, y la correcta no mucho más larga que las demás, porque se delataría.
- Sin opciones que remitan a las otras («Todas las anteriores.», «Las opciones a y b.»): el programa baraja el orden
  de las opciones en cada examen, y esas preguntas no se pueden barajar.
- La correcta, una sola, y sin discusión.

## La explicación y la referencia

- La **explicación** dice por qué la correcta lo es y por qué falla cada una de las otras, **sin citar letras** («la
  b)», «la opción c)»): las opciones se barajan y las letras cambian de un examen a otro. Se nombran por su contenido.
- La **referencia** dice dónde lo explica el libro, con la forma de su tema (`formato_referencia`), y termina en punto.

## El estilo

El programa avisa (en el editor y en *Editar → Revisar el estilo del banco*) de estas pautas en los bancos en
castellano:

- El enunciado empieza por mayúscula y termina en dos puntos; si es una pregunta, en «?:», con sus signos de apertura
  y cierre (`¿Cuál es…?:`). Conviene que no pase de unos 180 caracteres (sin contar el código).
- Las negaciones, en mayúsculas: «¿Cuál de estas afirmaciones es FALSA?:», «¿Qué NO hace…?:».
- Los literales, entre comillas tipográficas en el texto (“así”) y rectas dentro del código.
- Las opciones, como se dice arriba.

Cada asignatura puede tener además su propia guía de estilo (por ejemplo, un `estilo_preguntas.md` junto al banco):
si la hay, manda.

## Lista de comprobación

Para quien redacte las preguntas, persona o asistente:

1. El JSON tiene el `$schema`, los temas con su `numero` y cada pregunta con `numero`, `variante`, `enunciado`, cuatro
   `opciones` y `correcta`.
2. Los números de las preguntas nuevas están libres en su tema (si no, sustituirían a otras al importar), y las
   variantes de una misma pregunta comparten número.
3. El código va entre ```` ```lenguaje ```` y ```` ``` ````, con un lenguaje de la tabla, `numerado` solo si se habla de
   líneas, líneas de 70 caracteres como mucho, espacios y comillas rectas.
4. Las opciones siguen sus reglas y la explicación no cita letras.
5. `GeneradorExamenes --comprobar preguntas.json` no da ni errores ni avisos.
6. Si las preguntas no las ha revisado aún el profesor, llevan `"revisar": true`.
