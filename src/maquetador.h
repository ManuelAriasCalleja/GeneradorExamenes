#pragma once

#include "generador.h"

#include <QString>
#include <QVector>

class QPrinter;
class QTextDocument;
class QObject;

struct DatosConvocatoria
{
    QString asignatura = QStringLiteral("Fundamentos de Informática");
    QString convocatoria;            // Febrero, Junio, Septiembre...
    int anio = 0;
    QString instrucciones = QStringLiteral("Seleccione la respuesta correcta, exclusivamente, en la hoja de respuestas.\n"
                                           "No entregue el enunciado.");
    int duracionMinutos = 60;
    double puntosAcierto = 1.0;
    double puntosFallo = 0.3;        // se resta

    // «Junio 2026»: va en la cabecera, en el pie de página y en el nombre de los ficheros.
    QString convocatoriaYAnio() const
    {
        return anio > 0 ? QStringLiteral("%1 %2").arg(convocatoria).arg(anio).trimmed() : convocatoria;
    }
};

// Convierte preguntas y exámenes a HTML (subconjunto que entiende QTextDocument) y los imprime.
//
// Un examen debe caber en una página por cada 10 preguntas (paginasObjetivo). Para conseguirlo la maqueta tiene
// varios niveles de compactación (0 = normal): a partir del 1 las opciones se ponen en dos columnas (a c / b d)
// siempre que así ocupen menos líneas, y en los siguientes se reduce la separación entre preguntas y el tamaño
// de la letra. nivelParaCaber() elige el menor nivel con el que caben todos los modelos.
class Maquetador
{
public:
    // Enunciado (texto con bloques ``` y tablas |) a HTML. 'prefijo' (ya en HTML) se pone al principio.
    static QString enunciadoHtml(const QString &texto, const QString &prefijo = QString());
    static QString escapar(const QString &texto);

    // Ficha de una pregunta (vista del banco de preguntas).
    static QString fichaPreguntaHtml(const Pregunta &p, const QString &nombreTema);

    // Un modelo de examen. Con 'soluciones' marca la respuesta correcta y el código de cada pregunta.
    static QString examenHtml(const Modelo &modelo, const DatosConvocatoria &datos, bool soluciones, int nivel = 0);
    // Todos los modelos en un único documento, cada uno en página nueva (para exportar a HTML).
    static QString examenesHtml(const QVector<Modelo> &modelos, const DatosConvocatoria &datos, bool soluciones, int nivel = 0);

    // Plantilla de soluciones y estructura (códigos de pregunta por tema) de todos los modelos.
    static QString solucionesHtml(const QVector<Modelo> &modelos, const DatosConvocatoria &datos);

    // Documento LaTeX (para pdflatex) con todos los modelos, cada uno en página nueva con su pie «…: página / total»
    // (hay que compilar dos veces). Con 'soluciones' empieza por la estructura y la plantilla de respuestas y
    // marca las correctas. Las imágenes se enlazan como «carpetaImagenes/<código>.png»: las guarda quien llama.
    static QString examenesLatex(const QVector<Modelo> &modelos, const DatosConvocatoria &datos, bool soluciones,
                                 const QString &carpetaImagenes, int nivel = 0);
    static QString escaparLatex(const QString &texto);
    static QString nombreImagen(const Pregunta &p) { return p.codigoCompleto() + QStringLiteral(".png"); }

    // Crea un documento con el HTML dado y las imágenes de las preguntas como recursos "img:<id>".
    static QTextDocument *crearDocumento(const QString &html, const QVector<Pregunta> &conImagenes, QObject *parent);
    static QVector<Pregunta> preguntasDe(const QVector<Modelo> &modelos);

    // Páginas en las que debe caber un examen de n preguntas: una por cada 10.
    static int paginasObjetivo(int numPreguntas) { return qMax(1, (numPreguntas + 9) / 10); }
    static int numNiveles();
    // Menor nivel de compactación con el que todos los modelos caben en sus páginas objetivo, impresos en A4 con
    // los márgenes de configurarPagina(). Si no caben ni con el último nivel, devuelve ese y pone *caben a false.
    static int nivelParaCaber(const QVector<Modelo> &modelos, const DatosConvocatoria &datos, bool *caben = nullptr);

    // A4 con los márgenes de los exámenes.
    static void configurarPagina(QPrinter &printer);
    // Imprime (o exporta a PDF) los modelos, cada uno en página nueva, con pie «Nacional Original · Junio 2026: n / total».
    // Con 'soluciones' antepone la hoja de soluciones (estructura y plantillas) y marca las respuestas correctas.
    static bool imprimirExamenes(QPrinter *printer, const QVector<Modelo> &modelos, const DatosConvocatoria &datos,
                                 bool soluciones, int nivel);
    // Imprime varios documentos (cada uno empieza en página nueva) con pie "texto: página / total".
    // Evita que una pregunta quede partida entre dos páginas.
    static bool imprimir(QPrinter *printer, const QVector<QTextDocument *> &documentos, const QStringList &pies);

    static QString formatoPuntos(double v);

private:
    static QString cuerpoExamen(const Modelo &modelo, const DatosConvocatoria &datos, bool soluciones, int nivel);
    static QString cuerpoExamenLatex(const Modelo &modelo, const DatosConvocatoria &datos, bool soluciones,
                                     const QString &carpetaImagenes, int nivel);
    static QString solucionesLatex(const QVector<Modelo> &modelos, const DatosConvocatoria &datos);
};
