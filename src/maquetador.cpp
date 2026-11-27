#include "maquetador.h"
#include "maqueta.h"
#include "enunciado.h"
#include "resaltado.h"
#include "idioma.h"

#include <QAbstractTextDocumentLayout>
#include <QCoreApplication>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QHash>
#include <QImage>
#include <QLocale>
#include <QMap>
#include <QPainter>
#include <QBuffer>
#include <QDate>
#include <QPagedPaintDevice>
#include <QPdfWriter>
#include <QRegularExpression>
#include <QScreen>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QUrl>
#include <QtMath>
#include <memory>

// ---------------------------------------------------------------------------------------------
// Lo común a todas las maquetas (ver maqueta.h)

namespace Maqueta {

namespace {

const Nivel kNiveles[] = {
    {10.5, 12, false, "6", "\\normalsize"},
    {10.5, 10, true, "5", "\\normalsize"},
    {10.0, 8, true, "4", "\\small"},
    {9.5, 6, true, "3", "\\small"},
    {9.0, 4, true, "2", "\\small"},
};
const int kNumNiveles = int(sizeof(kNiveles) / sizeof(kNiveles[0]));
const Nivel kNivelLetraGrande = {16.0, 24, false, "14", "\\LARGE"};

} // namespace

QString tx(const DatosConvocatoria &d, const char *texto)
{
    return Idioma::examen(d.idioma, texto);
}

const Nivel &nivel(int n)
{
    return n == Maquetador::kLetraGrande ? kNivelLetraGrande : kNiveles[qBound(0, n, kNumNiveles - 1)];
}

int numNiveles()
{
    return kNumNiveles;
}

qreal escala(int n)
{
    return nivel(n).fuentePt / kNiveles[0].fuentePt;
}

qreal puntosExplicacion(int n)
{
    return n == Maquetador::kLetraGrande ? nivel(n).fuentePt : nivel(n).fuentePt - 1;
}

QSize tamanoImagen(const Pregunta &p, int n)
{
    if (p.imagen.isEmpty())
        return QSize();
    QImage img;
    if (!img.loadFromData(p.imagen) || img.width() == 0)
        return QSize();
    qreal dpi = img.dotsPerMeterX() * 0.0254;
    if (dpi < 50)
        dpi = 96;
    const int ancho = qMin(qRound(500 * escala(n)), qRound(img.width() * 96.0 / dpi * escala(n)));
    return QSize(ancho, qRound(qreal(ancho) * img.height() / img.width()));
}

int anchoImagen(const Pregunta &p, int n)
{
    const QSize tamano = tamanoImagen(p, n);
    return tamano.isEmpty() ? 0 : tamano.width();
}

// Los caracteres de una opción que se ven: sin las comillas invertidas de su código en línea.
int largo(const QString &opcion)
{
    return int(Enunciado::sinMarcas(opcion).size());
}

int filasOpciones(const PreguntaExamen &pe, int n)
{
    if (n == Maquetador::kLetraGrande)
        return 4;
    int maximo = 0;
    for (int i = 0; i < 4; ++i)
        maximo = qMax(maximo, largo(pe.opcion(i)));
    const qreal porLinea = 88.0 * 10.5 / nivel(n).fuentePt;   // caracteres por línea completa
    const qreal porColumna = porLinea * 0.48 - 3;              // en una de dos columnas, descontando «a) »
    if (maximo <= porLinea * 0.24 - 5)
        return 1;
    if (maximo <= porColumna)
        return 2;
    if (!nivel(n).dosColumnasSiAhorra)
        return 4;
    auto lineas = [](qsizetype caracteres, qreal anchura) { return qMax(1, qCeil(caracteres / anchura)); };
    int unaPorLinea = 0, dosColumnas = 0;
    for (int i = 0; i < 4; ++i)
        unaPorLinea += lineas(largo(pe.opcion(i)), porLinea);
    for (int fila = 0; fila < 2; ++fila)
        dosColumnas += qMax(lineas(largo(pe.opcion(fila)), porColumna), lineas(largo(pe.opcion(fila + 2)), porColumna));
    return dosColumnas < unaPorLinea ? 2 : 4;
}

// Cada opción tiene que caber en una línea en el ancho que deja la imagen: se estiman, como en filasOpciones(), los
// caracteres que caben, con margen para «a) », la marca de las soluciones y la negrita de la correcta.
bool imagenAlLado(const PreguntaExamen &pe, int n)
{
    if (n == Maquetador::kLetraGrande)
        return false;
    const QSize imagen = tamanoImagen(pe.pregunta, n);
    if (imagen.isEmpty())
        return false;
    const qreal ancho = kAnchoLinea - kSangriaOpciones - kSeparacionImagen - imagen.width();
    const qreal caben = 0.9 * 88.0 * 10.5 / nivel(n).fuentePt * ancho / (kAnchoLinea - kSangriaOpciones);
    for (int i = 0; i < 4; ++i)
        if (largo(pe.opcion(i)) + 5 > caben)
            return false;
    const qreal fila = nivel(n).fuentePt * 96 / 72 * 1.15 + 4;   // alto de una fila de opciones
    const qreal debajo = imagen.height() + 8 + filasOpciones(pe, n) * fila;
    return qMax(qreal(imagen.height()), 4 * fila) <= debajo;
}

QString asignaturaCabecera(const DatosConvocatoria &d)
{
    return d.asignaturaEnCabecera ? d.asignatura.trimmed() : QString();
}

QString convocatoriaCabecera(const DatosConvocatoria &d)
{
    return d.convocatoriaEnCabecera ? d.convocatoriaYAnio() : QString();
}

QString textoPie(const QString &nombre, const DatosConvocatoria &d)
{
    QStringList partes;
    if (d.asignaturaEnPie && !d.asignatura.trimmed().isEmpty())
        partes << d.asignatura.trimmed();
    partes << nombre;
    if (d.convocatoriaEnPie && !d.convocatoriaYAnio().isEmpty())
        partes << d.convocatoriaYAnio();
    return partes.join(QStringLiteral(" · "));
}

QString textoPuntuacion(const DatosConvocatoria &d)
{
    //: Puntuación del examen, en una línea bajo las instrucciones. %1 son los puntos de un acierto (1,00) y %2 los que
    //: resta un fallo (0,30); el signo menos es «−» (U+2212)
    return tx(d, QT_TRANSLATE_NOOP("Examen", "Respuesta correcta: +%1 puntos · incorrecta: −%2 puntos · en blanco: 0 puntos"))
        .arg(Maquetador::formatoPuntos(d.puntosAcierto, d.idioma), Maquetador::formatoPuntos(d.puntosFallo, d.idioma));
}

QString textoSemilla(const DatosConvocatoria &d)
{
    if (d.semilla == 0)
        return QString();
    //: En la hoja de soluciones y en las propiedades de los ficheros: la semilla con que se generaron los exámenes
    QString texto = tx(d, QT_TRANSLATE_NOOP("Examen", "Semilla: %1")).arg(d.semilla);
    //: Se añade tras «Semilla: %1» (con el espacio delante): con preguntas cambiadas, la semilla ya no da estos exámenes
    if (d.cambiadosAMano)
        texto += tx(d, QT_TRANSLATE_NOOP("Examen", " (con preguntas cambiadas a mano)"));
    return texto;
}

QString tituloDocumento(const DatosConvocatoria &d)
{
    QStringList partes;
    if (!d.asignatura.trimmed().isEmpty())
        partes << d.asignatura.trimmed();
    if (!d.convocatoriaYAnio().isEmpty())
        partes << d.convocatoriaYAnio();
    return partes.join(QStringLiteral(" · "));
}

QString programa(const DatosConvocatoria &d)
{
    const QString semilla = textoSemilla(d);
    return QStringLiteral("GeneradorExamenes " VERSION_APLICACION) + (semilla.isEmpty() ? QString() : QStringLiteral(" · ") + semilla);
}

QList<int> temasDe(const QVector<Modelo> &modelos)
{
    QMap<int, bool> temas;
    for (const Modelo &m : modelos)
        for (const PreguntaExamen &pe : m.preguntas)
            temas.insert(pe.pregunta.temaNumero, true);
    return temas.keys();
}

QStringList codigosDelTema(const Modelo &m, int tema)
{
    QVector<const Pregunta *> delTema;
    for (const PreguntaExamen &pe : m.preguntas)
        if (pe.pregunta.temaNumero == tema)
            delTema << &pe.pregunta;
    std::sort(delTema.begin(), delTema.end(), [](const Pregunta *a, const Pregunta *b) { return a->antesQue(*b); });
    QStringList codigos;
    for (const Pregunta *p : delTema)
        codigos << p->codigo();
    return codigos;
}

} // namespace Maqueta

using namespace Maqueta;

namespace {

QLocale localeExamen(const QString &idioma)
{
    return idioma.isEmpty() || idioma == QLatin1String(Idioma::kOrigen) ? QLocale(QLocale::Spanish, QLocale::Spain)
                                                                         : QLocale(idioma);
}

const QStringList kMeses{QStringLiteral("Enero"), QStringLiteral("Febrero"), QStringLiteral("Marzo"), QStringLiteral("Abril"),
                         QStringLiteral("Mayo"), QStringLiteral("Junio"), QStringLiteral("Julio"), QStringLiteral("Agosto"),
                         QStringLiteral("Septiembre"), QStringLiteral("Octubre"), QStringLiteral("Noviembre"),
                         QStringLiteral("Diciembre")};

// Convocatorias ordinarias y su mes.
const QList<QPair<int, QString>> kConvocatoriasOrdinarias{
    {2, QStringLiteral("Febrero")}, {6, QStringLiteral("Junio")}, {9, QStringLiteral("Septiembre")}};

const char *const kFuente = "'Liberation Sans', Arial, 'DejaVu Sans', sans-serif";
const char *const kFuenteCodigo = "'DejaVu Sans Mono', 'Liberation Mono', monospace";
// Recuadro del código de los enunciados: fondo gris claro (que se nota también en fotocopia) y borde gris.
const char *const kFondoCodigo = Paleta::kPapelFondoCodigo;
const char *const kBordeCodigo = Paleta::kPapelBordeCodigo;

// Documento que sirve las imágenes de las preguntas como recursos "img:<id>".
class DocumentoConImagenes : public QTextDocument
{
public:
    using QTextDocument::QTextDocument;
    QHash<int, QImage> imagenes;

protected:
    QVariant loadResource(int type, const QUrl &name) override
    {
        if (type == QTextDocument::ImageResource && name.scheme() == QLatin1String("img")) {
            const int id = name.path().toInt();
            if (imagenes.contains(id))
                return imagenes.value(id);
        }
        return QTextDocument::loadResource(type, name);
    }
};

// Letra de los exámenes en la maqueta y en el pie (la misma familia que kFuente).
QFont fuenteExamen(qreal puntos)
{
    QFont f;
    f.setFamilies({QStringLiteral("Liberation Sans"), QStringLiteral("Arial"), QStringLiteral("DejaVu Sans")});
    f.setPointSizeF(puntos);
    return f;
}

// Píxeles por pulgada con los que QTextDocument maqueta cuando no tiene dispositivo de pintado.
qreal dpiDocumento()
{
    if (const QScreen *s = QGuiApplication::primaryScreen())
        return qRound(s->logicalDotsPerInchY());
    return 100;
}

QString pt(qreal puntos)
{
    return QString::number(puntos, 'f', 1) + QStringLiteral("pt");
}

QString imagenHtml(const Pregunta &p, int n = 0)
{
    const int ancho = anchoImagen(p, n);
    if (ancho == 0)
        return QString();
    return QStringLiteral("<p style=\"margin-left:26px; margin-top:4px; margin-bottom:4px\"><img src=\"img:%1\" width=\"%2\"></p>")
        .arg(p.id).arg(ancho);
}

// La marca de la respuesta correcta (✔), con una letra de símbolos que la tiene en negro: la de Windows, la de Linux y
// la de macOS. Sin decir cuál, en Windows Qt la toma de la de los emojis (Segoe UI Emoji), y en el PDF sale enorme, tapando
// media página.
const char *const kMarcaCorrecta = "<span style=\"font-family:'Segoe UI Symbol','DejaVu Sans','Zapf Dingbats'\">&#10004;</span>";

QString textoOpcion(const QString &letra, const QString &texto, bool marcada)
{
    QString t = Maquetador::textoHtml(texto);
    if (marcada)
        return QStringLiteral("<span style=\"color:%1; font-weight:bold\">%2) %3 %4</span>")
            .arg(QLatin1String(Maquetador::kColorCorrecta), letra, t, QLatin1String(kMarcaCorrecta));
    return QStringLiteral("%1) %2").arg(letra, t);
}

// Opciones a)..d) repartidas según filasOpciones().
QString opcionesHtml(const PreguntaExamen &pe, bool soluciones, int n = 0)
{
    const int filas = filasOpciones(pe, n);
    const int correcta = pe.correcta();
    auto celda = [&](int i, const QString &ancho) {
        return QStringLiteral("<td width=\"%1\" style=\"padding:1px 4px 1px 0\">%2</td>")
            .arg(ancho, textoOpcion(QString(Pregunta::letra(i)), pe.opcion(i), soluciones && i == correcta));
    };
    QString html = QStringLiteral("<table width=\"100%\" cellspacing=\"0\" cellpadding=\"1\" style=\"margin-top:2px\">");
    const QString sangria = QStringLiteral("<td width=\"26\"></td>");
    if (filas == 1) {
        html += QStringLiteral("<tr>") + sangria;
        for (int i = 0; i < 4; ++i)
            html += celda(i, QStringLiteral("24%"));
        html += QStringLiteral("</tr>");
    } else if (filas == 2) {
        for (int fila = 0; fila < 2; ++fila)
            html += QStringLiteral("<tr>") + sangria + celda(fila, QStringLiteral("48%")) + celda(fila + 2, QStringLiteral("48%")) + QStringLiteral("</tr>");
    } else {
        for (int i = 0; i < 4; ++i)
            html += QStringLiteral("<tr>") + sangria + celda(i, QStringLiteral("96%")) + QStringLiteral("</tr>");
    }
    return html + QStringLiteral("</table>");
}

QString opcionesConImagenHtml(const PreguntaExamen &pe, bool soluciones, int n)
{
    const int correcta = pe.correcta();
    QString opciones = QStringLiteral("<table cellspacing=\"0\" cellpadding=\"1\">");
    for (int i = 0; i < 4; ++i)
        opciones += QStringLiteral("<tr><td style=\"padding:1px 4px 1px 0\">%1</td></tr>")
                        .arg(textoOpcion(QString(Pregunta::letra(i)), pe.opcion(i), soluciones && i == correcta));
    opciones += QStringLiteral("</table>");
    return QStringLiteral("<table cellspacing=\"0\" cellpadding=\"0\" style=\"margin-top:2px\"><tr>"
                          "<td width=\"%1\"></td><td valign=\"top\">%2</td><td width=\"%3\"></td>"
                          "<td valign=\"top\"><img src=\"img:%4\" width=\"%5\"></td></tr></table>")
        .arg(QString::number(kSangriaOpciones), opciones, QString::number(kSeparacionImagen),
             QString::number(pe.pregunta.id), QString::number(anchoImagen(pe.pregunta, n)));
}

// En los exámenes con soluciones, debajo de las opciones: por qué es correcta la correcta (la explicación de la
// pregunta) y dónde lo dice el texto (su referencia), si las tiene.
QString explicacionHtml(const Pregunta &p, const DatosConvocatoria &d, int n)
{
    QStringList partes;
    // La explicación admite código en línea (entre comillas invertidas); la referencia, no: es texto tal cual.
    auto parte = [&](const char *etiqueta, const QString &texto, bool conCodigo) {
        if (texto.trimmed().isEmpty())
            return;
        const QString html = conCodigo ? Maquetador::textoHtml(texto.trimmed()) : Maquetador::escapar(texto.trimmed());
        partes << QStringLiteral("<span style=\"color:%1; font-weight:bold\">%2</span> %3")
                      .arg(QLatin1String(Maquetador::kColorCorrecta), Maquetador::escapar(tx(d, etiqueta)),
                           QString(html).replace(QLatin1Char('\n'), QStringLiteral("<br>")));
    };
    parte(QT_TRANSLATE_NOOP("Examen", "Explicación:"), p.explicacion, true);
    parte(QT_TRANSLATE_NOOP("Examen", "Referencia:"), p.referencia, false);
    if (partes.isEmpty())
        return QString();
    return QStringLiteral("<p style=\"margin-left:26px; margin-top:3px; margin-bottom:0px; font-size:%1\">%2</p>")
        .arg(pt(puntosExplicacion(n)), partes.join(QStringLiteral("<br>")));
}

// «Plantilla de corrección» de un modelo: «1 C», «2 A»… en filas de 10, y cuántas veces es correcta cada letra.
QString plantillaHtml(const Modelo &m, const DatosConvocatoria &d)
{
    QString html = QStringLiteral("<h3>%1</h3>")
                       .arg(Maquetador::escapar(tx(d, QT_TRANSLATE_NOOP("Examen", "Plantilla de corrección: %1")).arg(m.nombre)));
    html += QStringLiteral("<table cellspacing=\"0\" cellpadding=\"3\" style=\"border-collapse:collapse; border:1px solid #000\">");
    for (int inicio = 0; inicio < m.preguntas.size(); inicio += 10) {
        html += QStringLiteral("<tr>");
        for (int i = inicio; i < qMin(inicio + 10, int(m.preguntas.size())); ++i)
            html += QStringLiteral("<td width=\"44\" align=\"center\" style=\"border:1px solid #000\">%1&nbsp;<b>%2</b></td>")
                        .arg(QString::number(i + 1), QString(Pregunta::letraMayuscula(m.preguntas.at(i).correcta())));
        html += QStringLiteral("</tr>");
    }
    return html + QStringLiteral("</table><p style=\"margin-top:2px; margin-bottom:0\">%1</p>")
                      .arg(Maquetador::escapar(tx(d, QT_TRANSLATE_NOOP("Examen", "Respuestas correctas: %1")).arg(Maquetador::textoCorrectasPorLetra(m))));
}

// Cabecera: la asignatura (en grande) y, debajo, la convocatoria, si se ponen; lo que esté vacío no se pone. A la
// derecha, 'derecha'. Si no queda nada, no hay cabecera.
QString cabeceraHtml(const DatosConvocatoria &d, const QString &derecha, int n = 0)
{
    QStringList izquierda;
    if (!asignaturaCabecera(d).isEmpty())
        izquierda << QStringLiteral("<span style=\"font-size:%1; font-weight:bold\">%2</span>")
                         .arg(pt(14 * escala(n)), Maquetador::escapar(asignaturaCabecera(d)));
    if (!convocatoriaCabecera(d).isEmpty())
        izquierda << Maquetador::escapar(convocatoriaCabecera(d));
    if (izquierda.isEmpty() && derecha.isEmpty())
        return QString();
    return QStringLiteral(
               "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"0\"><tr>"
               "<td>%1</td>"
               "<td align=\"right\" valign=\"top\">%2</td></tr></table>")
        .arg(izquierda.join(QStringLiteral("<br>")), derecha);
}

} // namespace

QString DatosConvocatoria::instruccionesPorDefecto(const QString &idioma)
{
    return Idioma::examen(idioma, QT_TRANSLATE_NOOP("Examen", "Seleccione la respuesta correcta, exclusivamente, en la hoja de respuestas.\n"
                                                            "No entregue el enunciado."));
}

void DatosConvocatoria::puntuar(Puntuacion puntuacion, int numPreguntas)
{
    if (puntuacion == Personalizada || numPreguntas <= 0)
        return;
    const double acierto = 10.0 / numPreguntas;
    const double fraccion = puntuacion == Resta30 ? 0.3 : puntuacion == RestaAlAzar ? fraccionFalloAlAzar() : 0.0;
    puntosAcierto = qRound(acierto * 100) / 100.0;
    puntosFallo = qRound(acierto * fraccion * 100) / 100.0;
}

QStringList DatosConvocatoria::ordinarias(const QString &idioma)
{
    QStringList nombres;
    for (const auto &c : kConvocatoriasOrdinarias)
        nombres << nombreDeMes(c.first, idioma);
    return nombres;
}

int DatosConvocatoria::mesDe(const QString &convocatoria)
{
    const QString c = convocatoria.trimmed();
    for (int i = 0; i < kMeses.size(); ++i)
        if (c.startsWith(kMeses.at(i), Qt::CaseInsensitive))
            return i + 1;
    // En otro idioma: los nombres de los meses en los idiomas del programa.
    for (const Idioma::Disponible &idioma : Idioma::disponibles()) {
        const QLocale l = localeExamen(idioma.codigo);
        for (int mes = 1; mes <= 12; ++mes)
            for (const QString &nombre : {l.standaloneMonthName(mes), l.monthName(mes)})
                if (!nombre.isEmpty() && c.startsWith(nombre, Qt::CaseInsensitive))
                    return mes;
    }
    return 0;
}

QString DatosConvocatoria::nombreDeMes(int mes, const QString &idioma)
{
    if (mes < 1 || mes > 12)
        return QString();
    if (idioma.isEmpty() || idioma == QLatin1String(Idioma::kOrigen))
        return kMeses.value(mes - 1);
    QString nombre = localeExamen(idioma).standaloneMonthName(mes);
    if (!nombre.isEmpty())
        nombre[0] = nombre[0].toUpper();
    return nombre;
}

DatosConvocatoria DatosConvocatoria::proxima(const QDate &hoy, const QString &idioma)
{
    DatosConvocatoria d;
    d.idioma = idioma;
    d.instrucciones = instruccionesPorDefecto(idioma);
    for (const auto &c : kConvocatoriasOrdinarias) {
        if (c.first > hoy.month()) {
            d.convocatoria = nombreDeMes(c.first, idioma);
            d.anio = hoy.year();
            return d;
        }
    }
    d.convocatoria = nombreDeMes(kConvocatoriasOrdinarias.first().first, idioma);
    d.anio = hoy.year() + 1;
    return d;
}

DatosConvocatoria DatosConvocatoria::anterior(const QDate &hoy, const QString &idioma)
{
    DatosConvocatoria d;
    d.idioma = idioma;
    d.instrucciones = instruccionesPorDefecto(idioma);
    for (auto it = kConvocatoriasOrdinarias.crbegin(); it != kConvocatoriasOrdinarias.crend(); ++it) {
        if (it->first <= hoy.month()) {
            d.convocatoria = nombreDeMes(it->first, idioma);
            d.anio = hoy.year();
            return d;
        }
    }
    d.convocatoria = nombreDeMes(kConvocatoriasOrdinarias.last().first, idioma);
    d.anio = hoy.year() - 1;
    return d;
}

QString Maquetador::escapar(const QString &texto)
{
    return texto.toHtmlEscaped();
}

QString Maquetador::textoHtml(const QString &texto)
{
    QString html;
    for (const Enunciado::Trozo &t : Enunciado::trozos(texto))
        html += t.codigo ? QStringLiteral("<span style=\"font-family:%1; font-style:normal\">%2</span>")
                               .arg(QLatin1String(kFuenteCodigo), escapar(t.texto))
                         : escapar(t.texto);
    return html;
}

QString Maquetador::textoCorrectasPorLetra(const Modelo &modelo)
{
    const std::array<int, 4> cuenta = modelo.correctasPorLetra();
    QStringList partes;
    for (int l = 0; l < 4; ++l)
        partes << QStringLiteral("%1 %2").arg(Pregunta::letraMayuscula(l)).arg(cuenta[l]);
    return partes.join(QStringLiteral(" · "));
}

QString Maquetador::formatoPuntos(double v, const QString &idioma)
{
    return localeExamen(idioma).toString(v, 'f', 2);
}

namespace {

// Un bloque de código en un recuadro gris, para que se distinga del texto (QTextDocument solo pone borde a las
// tablas), resaltado según su lenguaje. Las líneas que no caben en el ancho de la página se parten. Con números de
// línea, una fila por línea: el número a la izquierda, en gris, y la línea (aunque se parta) a su derecha.
QString codigoHtml(const Enunciado::Bloque &b)
{
    const QString pre = QStringLiteral("<pre style=\"margin:0px; white-space:pre-wrap; font-family:%1; font-weight:normal; font-style:normal\">%2</pre>");
    const QStringList lineas = Resaltado::html(b.lineas, b.lenguaje);
    QString contenido;
    if (b.numerado) {
        contenido = QStringLiteral("<table cellspacing=\"0\" cellpadding=\"0\">");
        for (int i = 0; i < lineas.size(); ++i)
            contenido += QStringLiteral("<tr><td align=\"right\" style=\"padding-right:8px; color:#%1\">%2</td><td>%3</td></tr>")
                             .arg(QLatin1String(Resaltado::kColorNumero),
                                  pre.arg(QLatin1String(kFuenteCodigo), QString::number(i + 1)),
                                  pre.arg(QLatin1String(kFuenteCodigo), lineas.at(i).isEmpty() ? QStringLiteral(" ") : lineas.at(i)));
        contenido += QStringLiteral("</table>");
    } else {
        contenido = pre.arg(QLatin1String(kFuenteCodigo), lineas.join(QLatin1Char('\n')));
    }
    return QStringLiteral("<table cellspacing=\"0\" cellpadding=\"4\" style=\"margin-left:26px; margin-top:3px; margin-bottom:3px; border-collapse:collapse; border:1px solid %1\">"
                          "<tr><td bgcolor=\"%2\" style=\"border:1px solid %1\">%3</td></tr></table>")
        .arg(QLatin1String(kBordeCodigo), QLatin1String(kFondoCodigo), contenido);
}

} // namespace

QString Maquetador::enunciadoHtml(const QString &texto, const QString &prefijo)
{
    QString html;
    bool prefijoPuesto = prefijo.isEmpty();
    const QString estiloParrafo = QStringLiteral("margin-top:0px; margin-bottom:2px");
    // El prefijo («3.- ») va al principio del primer párrafo; si antes hay código o una tabla, en un párrafo propio.
    auto ponerPrefijoSolo = [&]() {
        if (!prefijoPuesto) {
            html += QStringLiteral("<p style=\"%1\">%2</p>").arg(estiloParrafo, prefijo);
            prefijoPuesto = true;
        }
    };
    for (const Enunciado::Bloque &b : Enunciado::bloques(texto)) {
        switch (b.tipo) {
        case Enunciado::Bloque::Parrafo: {
            QStringList lineas;
            for (const QString &linea : b.lineas)
                lineas << textoHtml(linea);
            QString contenido = lineas.join(QStringLiteral("<br>"));
            if (!prefijoPuesto) {
                contenido = prefijo + contenido;
                prefijoPuesto = true;
            }
            html += QStringLiteral("<p style=\"%1\">%2</p>").arg(estiloParrafo, contenido);
            break;
        }
        case Enunciado::Bloque::Tabla:
            ponerPrefijoSolo();
            html += QStringLiteral("<table cellspacing=\"0\" cellpadding=\"3\" style=\"margin-left:26px; margin-top:3px; margin-bottom:3px; border-collapse:collapse; border:1px solid #000; font-weight:normal; font-style:normal\">");
            for (const QStringList &fila : b.filas) {
                html += QStringLiteral("<tr>");
                for (const QString &c : fila)
                    html += QStringLiteral("<td style=\"border:1px solid #000\">%1</td>").arg(textoHtml(c));
                html += QStringLiteral("</tr>");
            }
            html += QStringLiteral("</table>");
            break;
        case Enunciado::Bloque::Codigo:
            ponerPrefijoSolo();
            html += codigoHtml(b);
            break;
        }
    }
    ponerPrefijoSolo();
    return html;
}

QString Maquetador::fichaPreguntaHtml(const Pregunta &p, const QString &nombreTema, bool conSolucion)
{
    // La ficha se ve en el programa: va en el idioma de la interfaz.
    const auto tr = [](const char *texto) { return QCoreApplication::translate("Maquetador", texto); };
    QString estado = p.estado;
    if (p.estado == QLatin1String("activa"))
        estado = tr("Activa");
    else if (p.estado == QLatin1String("anulada"))
        estado = tr("Anulada");
    else if (p.estado == QLatin1String("obsoleta"))
        estado = tr("Obsoleta");
    else if (!estado.isEmpty())
        estado[0] = estado[0].toUpper();
    QStringList etiquetas{estado};
    if (p.old)
        etiquetas << tr("ya salió en exámenes (old)");
    if (p.revisar)
        etiquetas << tr("para revisar");
    //: En la ficha de una pregunta: la han revisado personas; %1 son sus nombres
    if (p.verificada)
        etiquetas << (p.verificadores.isEmpty() ? tr("verificada") : tr("verificada por %1").arg(p.verificadores.join(QStringLiteral(", "))));
    else
        //: En la ficha de una pregunta: no la han revisado personas
        etiquetas << tr("sin verificar");

    QString html = QStringLiteral("<html><body style=\"font-family:%1\">").arg(QLatin1String(kFuente));
    html += QStringLiteral("<p style=\"color:#555\">%1<br><b>%2</b> &middot; %3</p>")
                .arg(escapar(nombreTema), escapar(p.codigoCompleto()), escapar(etiquetas.join(QStringLiteral(" · "))));
    html += QStringLiteral("<div style=\"font-weight:bold; font-style:italic\">%1</div>").arg(enunciadoHtml(p.enunciado));
    html += imagenHtml(p);
    PreguntaExamen pe;
    pe.pregunta = p;
    html += opcionesHtml(pe, conSolucion, 0);
    if (conSolucion)
        html += tr("<p><b>Respuesta correcta:</b> %1. %2</p>").arg(Pregunta::letraMayuscula(p.correcta)).arg(escapar(p.referencia));
    if (conSolucion && !p.explicacion.isEmpty())
        html += tr("<p><b>Explicación:</b> %1</p>").arg(textoHtml(p.explicacion).replace(QLatin1Char('\n'), QStringLiteral("<br>")));
    if (!p.pseudoCon.isEmpty())
        html += tr("<p><b>Pseudorrepetida con:</b> %1 (no saldrán juntas en un examen)</p>").arg(escapar(p.pseudoCon));
    if (conSolucion && !p.notas.isEmpty())
        html += tr("<p><b>Notas:</b> %1</p>").arg(escapar(p.notas));
    return html + QStringLiteral("</body></html>");
}

int Maquetador::numNiveles()
{
    return Maqueta::numNiveles();
}

QString Maquetador::examenHtml(const Modelo &modelo, const DatosConvocatoria &d, bool soluciones, int n, bool conPlantilla,
                              bool marcarVerificadas)
{
    return QStringLiteral("<html><body style=\"font-family:%1; font-size:%2pt\">%3</body></html>")
        .arg(QLatin1String(kFuente), QString::number(nivel(n).fuentePt),
             cuerpoExamen(modelo, d, soluciones, n, conPlantilla, marcarVerificadas));
}

QString Maquetador::marcaVerificacion(const Pregunta &p)
{
    // En la vista, que va en el idioma de la interfaz. Azul (y no verde, el de las soluciones) las verificadas; naranja
    // las que no.
    const auto tr = [](const char *texto) { return QCoreApplication::translate("Maquetador", texto); };
    if (!p.verificada)
        //: Marca de la vista del examen tras el número de una pregunta que no han revisado personas
        return QStringLiteral("<span style=\"color:%1\">&#9679; %2</span>").arg(QLatin1String(Paleta::kPapelSinVerificar), escapar(tr("sin verificar")));
    //: Marca de la vista del examen tras el número de una pregunta revisada por personas; %1 son sus nombres
    const QString texto = p.verificadores.isEmpty() ? tr("verificada")
                                                    : tr("verificada: %1").arg(p.verificadores.join(QStringLiteral(", ")));
    return QStringLiteral("<span style=\"color:%1\">&#10003; %2</span>").arg(QLatin1String(Paleta::kPapelVerificada), escapar(texto));
}

QString Maquetador::examenesHtml(const QVector<Modelo> &modelos, const DatosConvocatoria &d, bool soluciones, int n)
{
    QString html = QStringLiteral("<html><body style=\"font-family:%1; font-size:%2pt\">")
                       .arg(QLatin1String(kFuente), QString::number(nivel(n).fuentePt));
    // Con soluciones, primero la hoja de soluciones, como en el PDF.
    if (soluciones)
        html += QStringLiteral("<div style=\"font-size:10pt\">%1</div>").arg(cuerpoSoluciones(modelos, d));
    for (int i = 0; i < modelos.size(); ++i) {
        if (i > 0 || soluciones)
            html += QStringLiteral("<p style=\"page-break-before:always; margin:0\"></p>");
        html += cuerpoExamen(modelos.at(i), d, soluciones, n, false);
    }
    return html + QStringLiteral("</body></html>");
}

QString Maquetador::textoVersion(const Modelo &modelo, const DatosConvocatoria &d)
{
    if (!d.versionEnCabecera || modelo.version.isEmpty())
        return QString();
    //: Arriba a la derecha de la primera página de un examen con versiones: %1 es la versión («B» o «2»)
    return tx(d, QT_TRANSLATE_NOOP("Examen", "Versión %1")).arg(modelo.version);
}

QString Maquetador::cuerpoExamen(const Modelo &modelo, const DatosConvocatoria &d, bool soluciones, int n, bool conPlantilla,
                                 bool marcarVerificadas)
{
    QString html;
    QStringList derechas;
    if (!textoVersion(modelo, d).isEmpty())
        derechas << QStringLiteral("<span style=\"font-size:%1; font-weight:bold\">%2</span>")
                        .arg(pt(14 * escala(n)), escapar(textoVersion(modelo, d)));
    if (soluciones)
        derechas << QStringLiteral("<span style=\"color:%1; font-weight:bold\">%2</span>")
                        .arg(QLatin1String(Maquetador::kColorCorrecta), escapar(tx(d, QT_TRANSLATE_NOOP("Examen", "CON SOLUCIONES"))));
    const QString derecha = derechas.join(QStringLiteral("<br>"));
    html += cabeceraHtml(d, derecha, n);

    // Recuadro de instrucciones: el texto y, justo debajo, la puntuación en una línea (sin líneas en blanco, para que
    // sea bajo). Sin cabecera, sin el espacio de encima.
    QString instrucciones = escapar(d.instrucciones).replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    if (!instrucciones.isEmpty())
        instrucciones += QStringLiteral("<br>");
    html += QStringLiteral(
                "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"4\" style=\"margin-top:%1px; border-collapse:collapse; border:1px solid #000\"><tr><td style=\"border:1px solid #000\">"
                "%2%3</td></tr></table>")
                .arg(html.isEmpty() ? 0 : 6)
                .arg(instrucciones, escapar(textoPuntuacion(d)));
    if (soluciones && conPlantilla)
        html += plantillaHtml(modelo, d);

    const int separacion = nivel(n).separacionPx;
    for (int i = 0; i < modelo.preguntas.size(); ++i) {
        const PreguntaExamen &pe = modelo.preguntas.at(i);
        QString prefijo = QStringLiteral("<a name=\"q%1\">%1.-</a> ").arg(i + 1);
        if (soluciones)
            prefijo += QStringLiteral("<span style=\"color:%1; font-style:normal\">[%2]</span> ")
                           .arg(QLatin1String(Maquetador::kColorCorrecta), escapar(pe.pregunta.codigoCompleto()));
        if (marcarVerificadas)
            prefijo += QStringLiteral("<span style=\"font-style:normal; font-weight:normal; font-size:%1\">%2</span> ")
                           .arg(pt(puntosExplicacion(n)), marcaVerificacion(pe.pregunta));
        // Con letra grande, el enunciado va en negrita sin cursiva, que cuesta más de leer.
        html += QStringLiteral("<div style=\"margin-top:%1px; font-weight:bold%3\">%2</div>")
                    .arg(i == 0 ? separacion + 4 : separacion)
                    .arg(enunciadoHtml(pe.pregunta.enunciado, prefijo),
                         n == Maquetador::kLetraGrande || !d.enunciadoEnCursiva ? QString() : QStringLiteral("; font-style:italic"));
        if (imagenAlLado(pe, n)) {
            html += opcionesConImagenHtml(pe, soluciones, n);
        } else {
            html += imagenHtml(pe.pregunta, n);
            html += opcionesHtml(pe, soluciones, n);
        }
        if (soluciones)
            html += explicacionHtml(pe.pregunta, d, n);
    }
    html += QStringLiteral("<p style=\"margin:0\"><a name=\"fin\">&nbsp;</a></p>");
    return html;
}

QString Maquetador::solucionesHtml(const QVector<Modelo> &modelos, const DatosConvocatoria &d)
{
    return QStringLiteral("<html><body style=\"font-family:%1; font-size:10pt\">%2</body></html>")
        .arg(QLatin1String(kFuente), cuerpoSoluciones(modelos, d));
}

QString Maquetador::cuerpoSoluciones(const QVector<Modelo> &modelos, const DatosConvocatoria &d)
{
    // A la derecha, el título y, debajo, la semilla con que se generaron los exámenes (no va en ellos).
    QString derecha = QStringLiteral("<span style=\"font-size:16pt; font-weight:bold\">%1</span>")
                          .arg(escapar(tx(d, QT_TRANSLATE_NOOP("Examen", "Soluciones"))));
    if (!textoSemilla(d).isEmpty())
        derecha += QStringLiteral("<br><span style=\"font-size:8pt; color:%1\">%2</span>").arg(QLatin1String(Paleta::kPapelSecundario), escapar(textoSemilla(d)));
    QString html = cabeceraHtml(d, derecha);

    // Estructura: qué preguntas de cada tema entran en cada modelo (como la tabla que se prepara a mano).
    const QList<int> temas = temasDe(modelos);
    html += QStringLiteral("<h3>%1</h3>"
                           "<table cellspacing=\"0\" cellpadding=\"3\" style=\"border-collapse:collapse; border:1px solid #000\"><tr><th style=\"border:1px solid #000\">%2</th>")
                .arg(escapar(tx(d, QT_TRANSLATE_NOOP("Examen", "Estructura de la convocatoria"))),
                     escapar(tx(d, QT_TRANSLATE_NOOP("Examen", "Examen"))));
    for (int t : temas)
        html += QStringLiteral("<th style=\"border:1px solid #000\">T%1</th>").arg(t);
    html += QStringLiteral("</tr>");
    for (const Modelo &m : modelos) {
        html += QStringLiteral("<tr><td style=\"border:1px solid #000\"><b>%1</b></td>").arg(escapar(m.nombre));
        for (int t : temas)
            html += QStringLiteral("<td style=\"border:1px solid #000\">%1</td>").arg(escapar(codigosDelTema(m, t).join(QStringLiteral(", "))));
        html += QStringLiteral("</tr>");
    }
    html += QStringLiteral("</table>");

    // Plantilla de corrección de cada modelo y, debajo, código, respuesta y referencia de cada pregunta.
    for (const Modelo &m : modelos) {
        html += plantillaHtml(m, d);
        html += QStringLiteral("<table cellspacing=\"0\" cellpadding=\"2\" width=\"100%\" style=\"font-size:9pt\">");
        for (int i = 0; i < m.preguntas.size(); ++i) {
            const PreguntaExamen &pe = m.preguntas.at(i);
            html += QStringLiteral("<tr><td width=\"28\" align=\"right\">%1.</td><td width=\"60\">%2</td><td width=\"22\"><b>%3</b></td><td>%4</td></tr>")
                        .arg(i + 1)
                        .arg(escapar(pe.pregunta.codigoCompleto()))
                        .arg(Pregunta::letraMayuscula(pe.correcta()))
                        .arg(escapar(pe.pregunta.referencia));
        }
        html += QStringLiteral("</table>");
    }
    return html;
}

QVector<Pregunta> Maquetador::preguntasDe(const QVector<Modelo> &modelos)
{
    QVector<Pregunta> res;
    for (const Modelo &m : modelos)
        for (const PreguntaExamen &pe : m.preguntas)
            res << pe.pregunta;
    return res;
}

QTextDocument *Maquetador::crearDocumento(const QString &html, const QVector<Pregunta> &conImagenes, QObject *parent)
{
    auto *doc = new DocumentoConImagenes(parent);
    for (const Pregunta &p : conImagenes) {
        if (p.imagen.isEmpty() || doc->imagenes.contains(p.id))
            continue;
        QImage img;
        if (img.loadFromData(p.imagen))
            doc->imagenes.insert(p.id, img);
    }
    doc->setDefaultFont(fuenteExamen(10.5));
    doc->setHtml(html);
    return doc;
}

namespace {

// Si una pregunta (desde su ancla "qN" hasta la siguiente) cruza un salto de página, la pasa entera a la siguiente.
void evitarCortes(QTextDocument *doc)
{
    const qreal alto = doc->pageSize().height();
    QVector<QTextBlock> inicios;
    QTextBlock fin;
    for (QTextBlock b = doc->begin(); b.isValid(); b = b.next()) {
        for (auto it = b.begin(); !it.atEnd(); ++it) {
            const QTextFragment fr = it.fragment();
            if (!fr.isValid())
                continue;
            for (const QString &nombre : fr.charFormat().anchorNames()) {
                if (nombre == QLatin1String("fin"))
                    fin = b;
                else if (nombre.startsWith(QLatin1Char('q')) && (inicios.isEmpty() || inicios.last() != b))
                    inicios << b;
            }
        }
    }
    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    for (int i = 0; i < inicios.size(); ++i) {
        const QTextBlock ini = inicios.at(i);
        const QTextBlock limite = i + 1 < inicios.size() ? inicios.at(i + 1) : fin;
        const qreal arriba = layout->blockBoundingRect(ini).top();
        qreal abajo = arriba;
        for (QTextBlock b = ini; b.isValid() && b != limite; b = b.next())
            abajo = qMax(abajo, layout->blockBoundingRect(b).bottom());
        if (abajo - arriba > alto * 0.9)
            continue;   // no cabe en una página: se deja como está
        if (int(arriba / alto) != int((abajo - 1) / alto)) {
            QTextCursor c(ini);
            QTextBlockFormat f;
            f.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
            c.mergeBlockFormat(f);
            // Qt rehace la maqueta solo desde el bloque cambiado, y lo que sigue (con tablas) puede quedar mal colocado:
            // enunciados que no se pintan y menos páginas de las que hay. Se rehace entera.
            doc->setPageSize(doc->pageSize());
        }
    }
}

// El pie de una página: el del examen (textoPie()), con la página y cuántas tiene. En letra de kPuntosPie puntos.
constexpr int kPuntosPie = 8;

QString textoPiePagina(const QString &pie, int pagina, int total)
{
    return QStringLiteral("%1: %2 / %3").arg(pie).arg(pagina).arg(total);
}

// Medidas de una página del dispositivo: zona de texto en píxeles del documento (descontado el pie) y escala
// para pintarla. Se maqueta con la resolución "de pantalla" del documento y se escala al pintar (como QTextDocument::print).
struct Pagina
{
    QSizeF documento;
    qreal escala = 1;
    qreal anchoDispositivo = 0;
    qreal altoDispositivo = 0;
    QFont fuentePie;
};

Pagina paginaDe(QPagedPaintDevice *dispositivo)
{
    Pagina p;
    const qreal dpiDoc = dpiDocumento();
    p.escala = dispositivo->logicalDpiY() / dpiDoc;
    const QRectF area = dispositivo->pageLayout().paintRect(QPageLayout::Point);
    p.fuentePie = fuenteExamen(kPuntosPie);
    const QFontMetricsF fm(p.fuentePie, dispositivo);
    const qreal altoPieDispositivo = fm.height() * 2.2;
    const qreal anchoDoc = area.width() * dpiDoc / 72.0;
    const qreal altoDoc = area.height() * dpiDoc / 72.0 - altoPieDispositivo / p.escala;
    p.documento = QSizeF(anchoDoc, altoDoc);
    p.anchoDispositivo = anchoDoc * p.escala;
    p.altoDispositivo = area.height() * dispositivo->logicalDpiY() / 72.0;
    return p;
}

} // namespace

void Maquetador::configurarPagina(QPagedPaintDevice &dispositivo)
{
    dispositivo.setPageSize(QPageSize(QPageSize::A4));
    dispositivo.setPageMargins(QMarginsF(18, 15, 18, 12), QPageLayout::Millimeter);
}

void Maquetador::configurarPdf(QPdfWriter &pdf)
{
    pdf.setResolution(1200);
    configurarPagina(pdf);
}

int Maquetador::nivelParaCaber(const QVector<Modelo> &modelos, const DatosConvocatoria &d, bool *caben)
{
    // Solo hacen falta las medidas de la página: un PDF que no se llega a escribir.
    QBuffer nada;
    QPdfWriter pdf(&nada);
    configurarPdf(pdf);
    const QSizeF pagina = paginaDe(&pdf).documento;
    for (int n = 0; n < numNiveles(); ++n) {
        bool todos = true;
        for (const Modelo &m : modelos) {
            const std::unique_ptr<QTextDocument> doc(crearDocumento(examenHtml(m, d, false, n), preguntasDe({m}), nullptr));
            doc->setPageSize(pagina);
            evitarCortes(doc.get());
            if (doc->pageCount() > paginasObjetivo(m.preguntas.size())) {
                todos = false;
                break;
            }
        }
        if (todos) {
            if (caben)
                *caben = true;
            return n;
        }
    }
    if (caben)
        *caben = false;
    return numNiveles() - 1;
}

int Maquetador::paginas(const Modelo &modelo, const DatosConvocatoria &d, bool soluciones, int nivel)
{
    // Solo hacen falta las medidas de la página: un PDF que no se llega a escribir.
    QBuffer nada;
    QPdfWriter pdf(&nada);
    configurarPdf(pdf);
    const std::unique_ptr<QTextDocument> doc(crearDocumento(examenHtml(modelo, d, soluciones, nivel), preguntasDe({modelo}), nullptr));
    doc->setPageSize(paginaDe(&pdf).documento);
    evitarCortes(doc.get());
    return doc->pageCount();
}

QString Maquetador::pieVistaHtml(const Modelo &modelo, const DatosConvocatoria &d, bool soluciones, int nivel)
{
    const int total = paginas(modelo, d, soluciones, nivel);
    return QStringLiteral("<hr><p align=\"right\" style=\"margin:0; font-size:%1pt\">%2</p>")
        .arg(kPuntosPie)
        .arg(escapar(textoPiePagina(textoPie(modelo.nombre, d), total, total)));
}

bool Maquetador::imprimirExamenes(QPagedPaintDevice *dispositivo, const QVector<Modelo> &modelos, const DatosConvocatoria &d,
                                  bool soluciones, int nivel)
{
    const QVector<Pregunta> conImagen = preguntasDe(modelos);
    QVector<QTextDocument *> docs;
    QStringList pies;
    if (soluciones) {
        docs << crearDocumento(solucionesHtml(modelos, d), conImagen, nullptr);
        pies << textoPie(tx(d, QT_TRANSLATE_NOOP("Examen", "Soluciones")), d);
    }
    for (const Modelo &m : modelos) {
        docs << crearDocumento(examenHtml(m, d, soluciones, nivel), conImagen, nullptr);
        pies << textoPie(m.nombre, d);
    }
    const bool ok = imprimir(dispositivo, docs, pies);
    qDeleteAll(docs);
    return ok;
}

bool Maquetador::imprimir(QPagedPaintDevice *dispositivo, const QVector<QTextDocument *> &documentos, const QStringList &pies)
{
    QPainter painter;
    if (!painter.begin(dispositivo))
        return false;

    const Pagina pagina = paginaDe(dispositivo);
    const QFontMetricsF fm(pagina.fuentePie, dispositivo);
    const qreal anchoDoc = pagina.documento.width();
    const qreal altoDoc = pagina.documento.height();

    bool primera = true;
    for (int i = 0; i < documentos.size(); ++i) {
        QTextDocument *doc = documentos.at(i);
        doc->setPageSize(pagina.documento);
        evitarCortes(doc);
        const int paginas = doc->pageCount();
        for (int pg = 0; pg < paginas; ++pg) {
            if (!primera)
                dispositivo->newPage();
            primera = false;

            painter.save();
            painter.scale(pagina.escala, pagina.escala);
            const QRectF ventana(0, pg * altoDoc, anchoDoc, altoDoc);
            painter.translate(0, -ventana.top());
            painter.setClipRect(ventana);
            QAbstractTextDocumentLayout::PaintContext ctx;
            ctx.clip = ventana;
            ctx.palette.setColor(QPalette::Text, Qt::black);
            doc->documentLayout()->draw(&painter, ctx);
            painter.restore();

            painter.setFont(pagina.fuentePie);
            painter.setPen(Qt::black);
            const QRectF caja(0, pagina.altoDispositivo - fm.height() * 1.3, pagina.anchoDispositivo, fm.height() * 1.3);
            painter.drawText(caja, Qt::AlignRight | Qt::AlignVCenter, textoPiePagina(pies.value(i), pg + 1, paginas));
        }
    }
    painter.end();
    return true;
}
