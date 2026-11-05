#pragma once

#include "generador.h"

#include <QString>
#include <QVector>

class QDate;
class QPagedPaintDevice;
class QPdfWriter;
class QTextDocument;
class QObject;

struct DatosConvocatoria
{
    QString asignatura;              // la del banco (BaseDatos::asignatura())
    QString idioma = QStringLiteral("es");   // el de los textos del examen: el del banco (BaseDatos::idioma())
    QString convocatoria;            // Febrero, Junio, Septiembre...
    int anio = 0;
    QString instrucciones = instruccionesPorDefecto(QStringLiteral("es"));
    double puntosAcierto = 1.0;
    double puntosFallo = 0.3;        // se resta

    // «Junio 2026»: va en la cabecera, en el pie de página y en el nombre de los ficheros.
    QString convocatoriaYAnio() const
    {
        return anio > 0 ? QStringLiteral("%1 %2").arg(convocatoria).arg(anio).trimmed() : convocatoria;
    }

    // Instrucciones que se proponen para los exámenes en ese idioma.
    static QString instruccionesPorDefecto(const QString &idioma);
    // Convocatorias ordinarias, en el orden del curso: Febrero, Junio y Septiembre (en el idioma de los exámenes).
    static QStringList ordinarias(const QString &idioma = QStringLiteral("es"));
    // Mes de una convocatoria que se llama como un mes («Junio», «Septiembre (reserva)», «June»...), en cualquiera de
    // los idiomas del programa; 0 si no.
    static int mesDe(const QString &convocatoria);
    // Nombre del mes, como el de una convocatoria: «Febrero» para el 2 («February» en inglés); vacío si no es un mes.
    static QString nombreDeMes(int mes, const QString &idioma = QStringLiteral("es"));
    // La próxima convocatoria ordinaria, con su año: la primera cuyo mes aún no ha llegado. El 27 de octubre de 2026 es
    // Febrero de 2027; en junio ya es Septiembre, porque los exámenes de la del mes en curso estarán hechos.
    static DatosConvocatoria proxima(const QDate &hoy, const QString &idioma = QStringLiteral("es"));
    // La última convocatoria ordinaria, con su año: la anterior a la próxima (en octubre de 2026, Septiembre de 2026).
    static DatosConvocatoria anterior(const QDate &hoy, const QString &idioma = QStringLiteral("es"));
};

// Convierte preguntas y exámenes a HTML (subconjunto que entiende QTextDocument) y los imprime.
//
// Un examen debe caber en una página por cada 10 preguntas (paginasObjetivo). Para conseguirlo la maqueta tiene
// varios niveles de compactación (0 = normal): a partir del 1 las opciones se ponen en dos columnas (a c / b d)
// siempre que así ocupen menos líneas, y en los siguientes se reduce la separación entre preguntas y el tamaño
// de la letra. nivelParaCaber() elige el menor nivel con el que caben todos los modelos.
//
// Al contrario, el nivel kLetraGrande es la versión adaptada para estudiantes con discapacidad visual: letra de
// 16 pt (también en la cabecera y las instrucciones), una opción por línea, más separación e imágenes más grandes,
// sin límite de páginas.
class Maquetador
{
public:
    static constexpr const char *kColorCorrecta = "#0a7d32";   // verde de las respuestas correctas
    static constexpr int kLetraGrande = -1;                    // nivel de la maqueta con letra grande

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

    // A4 con los márgenes de los exámenes, en una impresora (QPrinter) o en un PDF.
    static void configurarPagina(QPagedPaintDevice &dispositivo);
    // PDF con la página de los exámenes y la resolución de QPrinter::HighResolution. Para los PDF se usa QPdfWriter
    // y no QPrinter: crear un QPrinter consulta las impresoras del sistema y tarda segundos.
    static void configurarPdf(QPdfWriter &pdf);
    // Imprime (o exporta a PDF) los modelos, cada uno en página nueva, con pie «Nacional Original · Junio 2026: n / total».
    // Con 'soluciones' antepone la hoja de soluciones (estructura y plantillas) y marca las respuestas correctas.
    static bool imprimirExamenes(QPagedPaintDevice *dispositivo, const QVector<Modelo> &modelos, const DatosConvocatoria &datos,
                                 bool soluciones, int nivel);
    // Imprime varios documentos (cada uno empieza en página nueva) con pie "texto: página / total".
    // Evita que una pregunta quede partida entre dos páginas.
    static bool imprimir(QPagedPaintDevice *dispositivo, const QVector<QTextDocument *> &documentos, const QStringList &pies);

    // «0,30»: con los decimales del idioma (el del examen).
    static QString formatoPuntos(double v, const QString &idioma = QStringLiteral("es"));
    // «A 3 · B 3 · C 2 · D 2»: cuántas veces es correcta cada letra en el examen.
    static QString textoCorrectasPorLetra(const Modelo &modelo);

private:
    static QString cuerpoExamen(const Modelo &modelo, const DatosConvocatoria &datos, bool soluciones, int nivel);
    static QString cuerpoExamenLatex(const Modelo &modelo, const DatosConvocatoria &datos, bool soluciones,
                                     const QString &carpetaImagenes, int nivel);
    static QString solucionesLatex(const QVector<Modelo> &modelos, const DatosConvocatoria &datos);
};
