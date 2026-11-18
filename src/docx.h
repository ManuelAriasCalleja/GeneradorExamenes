#pragma once

#include "maquetador.h"

#include <QByteArray>
#include <QMap>
#include <QString>

// Exámenes en un documento de Word (.docx, Office Open XML), que se abre y se modifica con Word o con LibreOffice. Lleva
// lo mismo que el PDF, con la misma maqueta (la del nivel que se le pida): cada modelo empieza en página nueva, en su
// propia sección, con su pie «…: página / total»; las preguntas no se parten entre dos páginas y las imágenes van
// dentro del documento. Con 'soluciones', como el PDF de soluciones: empieza por la hoja de soluciones, marca las
// correctas y pone debajo de cada pregunta su explicación y su referencia. La semilla va en sus propiedades.
namespace Docx {

// El .docx (un ZIP).
QByteArray examenes(const QVector<Modelo> &modelos, const DatosConvocatoria &datos, bool soluciones, int nivel = 0);

// Sus partes (ruta dentro del ZIP → contenido), para las pruebas.
QMap<QString, QByteArray> partes(const QVector<Modelo> &modelos, const DatosConvocatoria &datos, bool soluciones,
                                 int nivel = 0);

} // namespace Docx
