#pragma once

#include "maquetador.h"

// Escritura de los exámenes en ficheros: PDF, HTML (editable en Word o LibreOffice), LaTeX (para pdflatex) y Word
// (.docx). Con 'soluciones' llevan la hoja de soluciones y las respuestas marcadas. El HTML y el LaTeX guardan las
// imágenes de las preguntas en una carpeta junto al fichero, «<nombre>-imagenes», sin espacios ni tildes; el Word las
// lleva dentro. La semilla de los exámenes va en las propiedades de los ficheros (y en la hoja de soluciones).
namespace Exportacion {

bool pdf(const QString &ruta, const QVector<Modelo> &modelos, const DatosConvocatoria &datos, bool soluciones, int nivel);
bool html(const QString &ruta, const QVector<Modelo> &modelos, const DatosConvocatoria &datos, bool soluciones, int nivel);
bool latex(const QString &ruta, const QVector<Modelo> &modelos, const DatosConvocatoria &datos, bool soluciones, int nivel);
bool docx(const QString &ruta, const QVector<Modelo> &modelos, const DatosConvocatoria &datos, bool soluciones, int nivel);

} // namespace Exportacion
