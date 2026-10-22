#pragma once

#include "maquetador.h"

// Escritura de los exámenes en ficheros: PDF, HTML (editable en Word o LibreOffice) y LaTeX (para pdflatex). Con
// 'soluciones' llevan la hoja de soluciones y las respuestas marcadas. El HTML y el LaTeX guardan las imágenes de
// las preguntas en una carpeta junto al fichero, «<nombre>_imagenes», sin espacios ni tildes.
namespace Exportacion {

bool pdf(const QString &ruta, const QVector<Modelo> &modelos, const DatosConvocatoria &datos, bool soluciones, int nivel);
bool html(const QString &ruta, const QVector<Modelo> &modelos, const DatosConvocatoria &datos, bool soluciones, int nivel);
bool latex(const QString &ruta, const QVector<Modelo> &modelos, const DatosConvocatoria &datos, bool soluciones, int nivel);

} // namespace Exportacion
