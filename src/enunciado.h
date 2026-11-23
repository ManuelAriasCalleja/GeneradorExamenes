#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// Formato del texto de los enunciados, el que se escribe en el editor y el que genera herramientas/importar_docx.py:
// párrafos de texto normal, bloques de código entre líneas ``` y filas de tabla «| celda | celda |». La línea que abre
// un bloque de código puede decir su lenguaje y si lleva números de línea: «```java», «```xml numerado», «```numerado»
// (ver Resaltado). Lo leen la maqueta (HTML y LaTeX), Moodle y los comprobadores de estilo y del JSON.
//
// Dentro del texto (del enunciado, de las celdas de una tabla, de las opciones y de la explicación), lo que va entre
// dos comillas invertidas de una misma línea, «el método `leer()`», es código en línea: sale con letra de ancho fijo
// y sin cursiva, para que no se confundan caracteres como la I y la l. Una comilla invertida sin pareja en su línea
// es un carácter más.
namespace Enunciado {

// Un trozo de texto: normal o código en línea (sin sus comillas invertidas).
struct Trozo
{
    QString texto;
    bool codigo = false;
};

// El texto, partido en trozos normales y de código en línea, en orden. Los saltos de línea se quedan en los normales.
QVector<Trozo> trozos(const QString &texto);
// El texto sin las comillas invertidas del código en línea, para donde no se puede cambiar de letra.
QString sinMarcas(const QString &texto);
// Si alguna línea del texto tiene una comilla invertida sin pareja (que saldrá tal cual).
bool comillaSinPareja(const QString &texto);

struct Bloque
{
    enum Tipo { Parrafo, Codigo, Tabla };
    Tipo tipo;
    QStringList lineas;          // párrafo o código, tal como se escribieron (un párrafo puede tener líneas vacías)
    QVector<QStringList> filas;  // tabla: las celdas de cada fila, sin espacios alrededor
    // Código: lo que dice la línea de apertura. El lenguaje, en minúsculas como se escribió (vacío si no lo dice); si
    // lleva números de línea («numerado»); las demás palabras, que no significan nada; y si se cierra con otra ```.
    QString lenguaje;
    bool numerado = false;
    QStringList sobrantes;
    bool cerrado = true;
};

// Los bloques del enunciado, en orden. Un bloque de código sin cerrar llega hasta el final.
QVector<Bloque> bloques(const QString &texto);

// Problemas de los bloques de código del enunciado, para avisar (en el editor y al comprobar un JSON): un lenguaje
// que no se resalta, palabras que sobran en la línea de apertura, un bloque sin cerrar y una comilla invertida sin
// pareja en el texto. En el idioma de la interfaz.
QStringList avisosDelCodigo(const QString &texto);

} // namespace Enunciado
