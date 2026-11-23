#pragma once

#include "maquetador.h"

#include <QString>

// Exámenes en LaTeX (para pdflatex), con la misma maqueta que el PDF (la del nivel que se le pida).
namespace Latex {

// Documento con todos los modelos, cada uno en página nueva con su pie «…: página / total» (hay que compilar dos
// veces). Con 'soluciones', como el PDF de soluciones: empieza por la hoja de soluciones (estructura y plantilla de
// corrección de cada modelo), marca las correctas y pone debajo de cada pregunta su explicación y su referencia. Las
// imágenes se enlazan como «carpetaImagenes/<código>.png» (Maquetador::nombreImagen()): las guarda quien llama.
QString examenes(const QVector<Modelo> &modelos, const DatosConvocatoria &datos, bool soluciones,
                 const QString &carpetaImagenes, int nivel = 0);

// El texto, con lo que en LaTeX significa algo («%», «&», «_»…) escrito para que salga tal cual.
QString escapar(const QString &texto);

} // namespace Latex
