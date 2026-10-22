#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// Formato del texto de los enunciados, el que se escribe en el editor y el que genera herramientas/importar_docx.py:
// párrafos de texto normal, bloques de código entre líneas ``` y filas de tabla «| celda | celda |». Lo leen la
// maqueta (HTML y LaTeX) y el comprobador de estilo.
namespace Enunciado {

struct Bloque
{
    enum Tipo { Parrafo, Codigo, Tabla };
    Tipo tipo;
    QStringList lineas;          // párrafo o código, tal como se escribieron (un párrafo puede tener líneas vacías)
    QVector<QStringList> filas;  // tabla: las celdas de cada fila, sin espacios alrededor
};

// Los bloques del enunciado, en orden. Un bloque de código sin cerrar llega hasta el final.
QVector<Bloque> bloques(const QString &texto);

} // namespace Enunciado
