#pragma once

// Lo que comparten las maquetas de los exámenes (la vista y el PDF, el HTML, el LaTeX y el Word): los niveles de la
// maqueta, cómo se reparten las opciones, el tamaño de las imágenes y los textos fijos que pone el programa. Es
// interno: lo usan maquetador.cpp, docx.cpp y exportacion.cpp.

#include "maquetador.h"

#include <QList>
#include <QSize>
#include <QString>
#include <QStringList>

namespace Maqueta {

// Texto del examen en su idioma (el del banco): ver Idioma::examen(). Se escribe tx(d, QT_TRANSLATE_NOOP("Examen", "…")).
QString tx(const DatosConvocatoria &d, const char *texto);

// Niveles de compactación de la maqueta (ver maquetador.h). El 0 es la maqueta normal. En LaTeX el artículo es de
// 10pt y se aprovecha mejor la página que en QTextDocument, así que basta con \small (9pt) en los niveles altos.
// Maquetador::kLetraGrande es la versión adaptada (letra grande).
struct Nivel
{
    qreal fuentePt;            // tamaño de la letra del examen
    int separacionPx;          // espacio entre preguntas
    bool dosColumnasSiAhorra;  // opciones largas en dos columnas cuando así ocupen menos líneas que una por línea
    const char *separacionLatex;   // lo mismo en LaTeX (en pt)
    const char *letraLatex;
};
const Nivel &nivel(int n);
int numNiveles();
// Cuánto se agrandan la cabecera, las instrucciones y las imágenes respecto a la maqueta normal (10,5 pt).
qreal escala(int n);
// Letra de la explicación y la referencia de las soluciones: algo menor que la del examen (un punto menos), salvo con
// letra grande, que se queda en la misma.
qreal puntosExplicacion(int n);

// Tamaño con el que se muestra la imagen de la pregunta, en píxeles a 96 ppp: el suyo según su resolución (96 ppp si
// no la dice), a escala con la letra del nivel y como mucho 500 de ancho (a escala). Vacío si no tiene.
QSize tamanoImagen(const Pregunta &p, int n = 0);
// Ancho con el que se muestra la imagen de la pregunta, en píxeles a 96 ppp (0 si no tiene).
int anchoImagen(const Pregunta &p, int n = 0);

// Filas en que se reparten las opciones: 1 (a b c d), 2 (a c / b d) o 4 (una por línea). Se decide estimando
// los caracteres que caben en una línea (A4 con los márgenes de configurarPagina() y la letra del nivel). Con
// letra grande, siempre una por línea.
int filasOpciones(const PreguntaExamen &pe, int n);

// Imagen al lado de las opciones: estas, una por línea en su sitio de siempre, y la imagen a su derecha, alineadas
// por arriba, en lugar de la imagen debajo del enunciado y las opciones debajo de ella.
constexpr qreal kAnchoLinea = 174 / 25.4 * 96;   // la línea del examen (A4 menos los márgenes), en píxeles a 96 ppp
constexpr int kSangriaOpciones = 26;               // la de las opciones (y de las imágenes, el código y las tablas)
constexpr int kSeparacionImagen = 24;              // entre la opción más larga y la imagen
// Se pone así si la pregunta tiene imagen, cada opción cabe en una línea en el ancho que deja la imagen y no ocupa más
// alto que la imagen debajo. Con letra grande, nunca: la imagen va siempre debajo.
bool imagenAlLado(const PreguntaExamen &pe, int n);

// Lo que va a la izquierda de la cabecera: la asignatura y la convocatoria («Junio 2026»), si se ponen en la cabecera
// (DatosConvocatoria) y no están vacías.
QString asignaturaCabecera(const DatosConvocatoria &d);
QString convocatoriaCabecera(const DatosConvocatoria &d);
// Pie de página de un examen o de la hoja de soluciones: «Asignatura · Nacional Original · Junio 2026», con la
// asignatura y la convocatoria si se ponen en el pie y no están vacías.
QString textoPie(const QString &nombre, const DatosConvocatoria &d);
// La puntuación, en una línea, debajo de las instrucciones: «Respuesta correcta: +0,50 puntos · incorrecta: …».
QString textoPuntuacion(const DatosConvocatoria &d);
// «Semilla: 123» (y si se han cambiado preguntas a mano, que ya no los reproduce), para la hoja de soluciones y las
// propiedades de los ficheros; vacío si no se sabe.
QString textoSemilla(const DatosConvocatoria &d);
// Título de los ficheros (en sus propiedades): «Asignatura · Junio 2026».
QString tituloDocumento(const DatosConvocatoria &d);
// El programa que ha hecho el fichero, para sus propiedades, con la semilla: «GeneradorExamenes 1.1 · Semilla: 123».
QString programa(const DatosConvocatoria &d);

// Temas que aparecen en la convocatoria y códigos (en orden) de las preguntas de un tema en un modelo.
QList<int> temasDe(const QVector<Modelo> &modelos);
QStringList codigosDelTema(const Modelo &m, int tema);

} // namespace Maqueta
