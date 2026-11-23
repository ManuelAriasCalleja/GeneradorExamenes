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

int filasOpciones(const PreguntaExamen &pe, int n)
{
    if (n == Maquetador::kLetraGrande)
        return 4;
    int maximo = 0;
    for (int i = 0; i < 4; ++i)
        maximo = qMax(maximo, int(pe.opcion(i).size()));
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
        unaPorLinea += lineas(pe.opcion(i).size(), porLinea);
    for (int fila = 0; fila < 2; ++fila)
        dosColumnas += qMax(lineas(pe.opcion(fila).size(), porColumna), lineas(pe.opcion(fila + 2).size(), porColumna));
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
        if (pe.opcion(i).size() + 5 > caben)
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
const char *const kFondoCodigo = "#eeeeee";
const char *const kBordeCodigo = "#808080";

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

QString letraExplicacionLatex(int n)
{
    if (n == Maquetador::kLetraGrande)
        return QString();
    return QLatin1String(nivel(n).letraLatex) == QLatin1String("\\small") ? QStringLiteral("\\footnotesize")
                                                                          : QStringLiteral("\\small");
}

QString imagenHtml(const Pregunta &p, int n = 0)
{
    const int ancho = anchoImagen(p, n);
    if (ancho == 0)
        return QString();
    return QStringLiteral("<p style=\"margin-left:26px; margin-top:4px; margin-bottom:4px\"><img src=\"img:%1\" width=\"%2\"></p>")
        .arg(p.id).arg(ancho);
}

QString textoOpcion(const QString &letra, const QString &texto, bool marcada)
{
    QString t = Maquetador::escapar(texto);
    if (marcada)
        return QStringLiteral("<span style=\"color:%1; font-weight:bold\">%2) %3 &#10004;</span>").arg(QLatin1String(Maquetador::kColorCorrecta), letra, t);
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
    auto parte = [&](const char *etiqueta, const QString &texto) {
        if (texto.trimmed().isEmpty())
            return;
        partes << QStringLiteral("<span style=\"color:%1; font-weight:bold\">%2</span> %3")
                      .arg(QLatin1String(Maquetador::kColorCorrecta), Maquetador::escapar(tx(d, etiqueta)),
                           Maquetador::escapar(texto.trimmed()).replace(QLatin1Char('\n'), QStringLiteral("<br>")));
    };
    parte(QT_TRANSLATE_NOOP("Examen", "Explicación:"), p.explicacion);
    parte(QT_TRANSLATE_NOOP("Examen", "Referencia:"), p.referencia);
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
                lineas << escapar(linea);
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
                    html += QStringLiteral("<td style=\"border:1px solid #000\">%1</td>").arg(escapar(c));
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

QString Maquetador::fichaPreguntaHtml(const Pregunta &p, const QString &nombreTema)
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
    html += opcionesHtml(pe, true, 0);
    html += tr("<p><b>Respuesta correcta:</b> %1. %2</p>").arg(Pregunta::letraMayuscula(p.correcta)).arg(escapar(p.referencia));
    if (!p.explicacion.isEmpty())
        html += tr("<p><b>Explicación:</b> %1</p>").arg(escapar(p.explicacion).replace(QLatin1Char('\n'), QStringLiteral("<br>")));
    if (!p.pseudoCon.isEmpty())
        html += tr("<p><b>Pseudorrepetida con:</b> %1 (no saldrán juntas en un examen)</p>").arg(escapar(p.pseudoCon));
    if (!p.notas.isEmpty())
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
        return QStringLiteral("<span style=\"color:#b45309\">&#9679; %1</span>").arg(escapar(tr("sin verificar")));
    //: Marca de la vista del examen tras el número de una pregunta revisada por personas; %1 son sus nombres
    const QString texto = p.verificadores.isEmpty() ? tr("verificada")
                                                    : tr("verificada: %1").arg(p.verificadores.join(QStringLiteral(", ")));
    return QStringLiteral("<span style=\"color:#1d4ed8\">&#10003; %1</span>").arg(escapar(texto));
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

QString Maquetador::cuerpoExamen(const Modelo &modelo, const DatosConvocatoria &d, bool soluciones, int n, bool conPlantilla,
                                 bool marcarVerificadas)
{
    QString html;
    QString derecha;
    if (soluciones)
        derecha = QStringLiteral("<span style=\"color:%1; font-weight:bold\">%2</span>")
                      .arg(QLatin1String(Maquetador::kColorCorrecta), escapar(tx(d, QT_TRANSLATE_NOOP("Examen", "CON SOLUCIONES"))));
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
                         n == Maquetador::kLetraGrande ? QString() : QStringLiteral("; font-style:italic"));
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
        derecha += QStringLiteral("<br><span style=\"font-size:8pt; color:#555555\">%1</span>").arg(escapar(textoSemilla(d)));
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

// ---------------------------------------------------------------------------------------------
// LaTeX

namespace {

// Preámbulo para pdflatex. \empezarmodelo reinicia la numeración de páginas y cambia el pie, para que cada
// modelo diga «Nacional Original · Junio 2026: página / total» como en el PDF.
const char *const kPreambuloLatex = R"tex(\documentclass[10pt,a4paper]{article}
\usepackage[T1]{fontenc}
\usepackage[utf8]{inputenc}
\usepackage[spanish,es-noshorthands]{babel}
\usepackage{lmodern}
\usepackage{helvet}
\renewcommand{\familydefault}{\sfdefault}
\usepackage[a4paper,left=18mm,right=18mm,top=15mm,bottom=12mm,includefoot,footskip=8mm]{geometry}
\usepackage{xcolor}
\usepackage{graphicx,array,tabularx,listings,fancyhdr,amssymb}
\definecolor{verde}{HTML}{0A7D32}
% Símbolos que pdflatex no reconoce por sí solo (válidos también dentro de los bloques de código).
\DeclareUnicodeCharacter{03BC}{\textmu}
\DeclareUnicodeCharacter{2264}{\ensuremath{\leq}}
\DeclareUnicodeCharacter{2265}{\ensuremath{\geq}}
\DeclareUnicodeCharacter{2260}{\ensuremath{\neq}}
\DeclareUnicodeCharacter{2248}{\ensuremath{\approx}}
\DeclareUnicodeCharacter{2192}{\ensuremath{\rightarrow}}
\DeclareUnicodeCharacter{2190}{\ensuremath{\leftarrow}}
\DeclareUnicodeCharacter{2212}{\ensuremath{-}}
\DeclareUnicodeCharacter{221A}{\ensuremath{\surd}}
\DeclareUnicodeCharacter{221E}{\ensuremath{\infty}}
\DeclareUnicodeCharacter{03B1}{\ensuremath{\alpha}}
\DeclareUnicodeCharacter{03B2}{\ensuremath{\beta}}
\DeclareUnicodeCharacter{03C0}{\ensuremath{\pi}}
\DeclareUnicodeCharacter{03A9}{\ensuremath{\Omega}}
\DeclareUnicodeCharacter{2022}{\textbullet}
\DeclareUnicodeCharacter{2713}{\checkmark}
% Las letras griegas, los símbolos de lógica, de conjuntos y de flechas, y los caracteres invisibles de formato (que no
% se escriben).
\DeclareUnicodeCharacter{03B1}{\ensuremath{\alpha}}\DeclareUnicodeCharacter{03B2}{\ensuremath{\beta}}\DeclareUnicodeCharacter{03B3}{\ensuremath{\gamma}}\DeclareUnicodeCharacter{03B4}{\ensuremath{\delta}}
\DeclareUnicodeCharacter{03B5}{\ensuremath{\varepsilon}}\DeclareUnicodeCharacter{03B6}{\ensuremath{\zeta}}\DeclareUnicodeCharacter{03B7}{\ensuremath{\eta}}\DeclareUnicodeCharacter{03B8}{\ensuremath{\theta}}
\DeclareUnicodeCharacter{03B9}{\ensuremath{\iota}}\DeclareUnicodeCharacter{03BA}{\ensuremath{\kappa}}\DeclareUnicodeCharacter{03BB}{\ensuremath{\lambda}}\DeclareUnicodeCharacter{03BC}{\ensuremath{\mu}}
\DeclareUnicodeCharacter{03BD}{\ensuremath{\nu}}\DeclareUnicodeCharacter{03BE}{\ensuremath{\xi}}\DeclareUnicodeCharacter{03BF}{\ensuremath{o}}\DeclareUnicodeCharacter{03C0}{\ensuremath{\pi}}
\DeclareUnicodeCharacter{03C1}{\ensuremath{\rho}}\DeclareUnicodeCharacter{03C2}{\ensuremath{\varsigma}}\DeclareUnicodeCharacter{03C3}{\ensuremath{\sigma}}\DeclareUnicodeCharacter{03C4}{\ensuremath{\tau}}
\DeclareUnicodeCharacter{03C5}{\ensuremath{\upsilon}}\DeclareUnicodeCharacter{03C6}{\ensuremath{\varphi}}\DeclareUnicodeCharacter{03C7}{\ensuremath{\chi}}\DeclareUnicodeCharacter{03C8}{\ensuremath{\psi}}
\DeclareUnicodeCharacter{03C9}{\ensuremath{\omega}}\DeclareUnicodeCharacter{0391}{\ensuremath{\mathrm{A}}}\DeclareUnicodeCharacter{0392}{\ensuremath{\mathrm{B}}}\DeclareUnicodeCharacter{0393}{\ensuremath{\Gamma}}
\DeclareUnicodeCharacter{0394}{\ensuremath{\Delta}}\DeclareUnicodeCharacter{0395}{\ensuremath{\mathrm{E}}}\DeclareUnicodeCharacter{0396}{\ensuremath{\mathrm{Z}}}\DeclareUnicodeCharacter{0397}{\ensuremath{\mathrm{H}}}
\DeclareUnicodeCharacter{0398}{\ensuremath{\Theta}}\DeclareUnicodeCharacter{0399}{\ensuremath{\mathrm{I}}}\DeclareUnicodeCharacter{039A}{\ensuremath{\mathrm{K}}}\DeclareUnicodeCharacter{039B}{\ensuremath{\Lambda}}
\DeclareUnicodeCharacter{039C}{\ensuremath{\mathrm{M}}}\DeclareUnicodeCharacter{039D}{\ensuremath{\mathrm{N}}}\DeclareUnicodeCharacter{039E}{\ensuremath{\Xi}}\DeclareUnicodeCharacter{039F}{\ensuremath{\mathrm{O}}}
\DeclareUnicodeCharacter{03A0}{\ensuremath{\Pi}}\DeclareUnicodeCharacter{03A1}{\ensuremath{\mathrm{P}}}\DeclareUnicodeCharacter{03A3}{\ensuremath{\Sigma}}\DeclareUnicodeCharacter{03A4}{\ensuremath{\mathrm{T}}}
\DeclareUnicodeCharacter{03A5}{\ensuremath{\Upsilon}}\DeclareUnicodeCharacter{03A6}{\ensuremath{\Phi}}\DeclareUnicodeCharacter{03A7}{\ensuremath{\mathrm{X}}}\DeclareUnicodeCharacter{03A8}{\ensuremath{\Psi}}
\DeclareUnicodeCharacter{03A9}{\ensuremath{\Omega}}
\DeclareUnicodeCharacter{2227}{\ensuremath{\land}}\DeclareUnicodeCharacter{2228}{\ensuremath{\lor}}\DeclareUnicodeCharacter{2295}{\ensuremath{\oplus}}\DeclareUnicodeCharacter{2297}{\ensuremath{\otimes}}
\DeclareUnicodeCharacter{2261}{\ensuremath{\equiv}}\DeclareUnicodeCharacter{2200}{\ensuremath{\forall}}\DeclareUnicodeCharacter{2203}{\ensuremath{\exists}}\DeclareUnicodeCharacter{2208}{\ensuremath{\in}}
\DeclareUnicodeCharacter{2209}{\ensuremath{\notin}}\DeclareUnicodeCharacter{2282}{\ensuremath{\subset}}\DeclareUnicodeCharacter{2286}{\ensuremath{\subseteq}}\DeclareUnicodeCharacter{2283}{\ensuremath{\supset}}
\DeclareUnicodeCharacter{2287}{\ensuremath{\supseteq}}\DeclareUnicodeCharacter{2229}{\ensuremath{\cap}}\DeclareUnicodeCharacter{222A}{\ensuremath{\cup}}\DeclareUnicodeCharacter{2205}{\ensuremath{\emptyset}}
\DeclareUnicodeCharacter{2211}{\ensuremath{\sum}}\DeclareUnicodeCharacter{220F}{\ensuremath{\prod}}\DeclareUnicodeCharacter{222B}{\ensuremath{\int}}\DeclareUnicodeCharacter{2202}{\ensuremath{\partial}}
\DeclareUnicodeCharacter{226A}{\ensuremath{\ll}}\DeclareUnicodeCharacter{226B}{\ensuremath{\gg}}\DeclareUnicodeCharacter{223C}{\ensuremath{\sim}}\DeclareUnicodeCharacter{2245}{\ensuremath{\cong}}
\DeclareUnicodeCharacter{22A5}{\ensuremath{\perp}}\DeclareUnicodeCharacter{221D}{\ensuremath{\propto}}\DeclareUnicodeCharacter{230A}{\ensuremath{\lfloor}}\DeclareUnicodeCharacter{230B}{\ensuremath{\rfloor}}
\DeclareUnicodeCharacter{2308}{\ensuremath{\lceil}}\DeclareUnicodeCharacter{2309}{\ensuremath{\rceil}}\DeclareUnicodeCharacter{2194}{\ensuremath{\leftrightarrow}}\DeclareUnicodeCharacter{21D2}{\ensuremath{\Rightarrow}}
\DeclareUnicodeCharacter{21D0}{\ensuremath{\Leftarrow}}\DeclareUnicodeCharacter{21D4}{\ensuremath{\Leftrightarrow}}\DeclareUnicodeCharacter{2191}{\ensuremath{\uparrow}}\DeclareUnicodeCharacter{2193}{\ensuremath{\downarrow}}
\DeclareUnicodeCharacter{22C5}{\ensuremath{\cdot}}\DeclareUnicodeCharacter{2218}{\ensuremath{\circ}}\DeclareUnicodeCharacter{221A}{\ensuremath{\surd}}\DeclareUnicodeCharacter{2714}{\checkmark}
\DeclareUnicodeCharacter{2717}{\ensuremath{\times}}\DeclareUnicodeCharacter{2718}{\ensuremath{\times}}\DeclareUnicodeCharacter{2605}{\ensuremath{\bigstar}}\DeclareUnicodeCharacter{2610}{\ensuremath{\square}}
\DeclareUnicodeCharacter{200B}{}\DeclareUnicodeCharacter{200C}{}\DeclareUnicodeCharacter{200D}{}\DeclareUnicodeCharacter{200E}{}\DeclareUnicodeCharacter{200F}{}\DeclareUnicodeCharacter{202A}{}\DeclareUnicodeCharacter{202B}{}
\DeclareUnicodeCharacter{202C}{}\DeclareUnicodeCharacter{202D}{}\DeclareUnicodeCharacter{202E}{}\DeclareUnicodeCharacter{2060}{}\DeclareUnicodeCharacter{FEFF}{}\DeclareUnicodeCharacter{00AD}{}
% Cualquier otro carácter que no conozca (un emoji, un alfabeto que no es el latino…) sale como «?» en un recuadro, en
% lugar de parar la compilación.
\makeatletter\def\UTFviii@undefined@err#1{\fbox{?}}\makeatother
\newcolumntype{Y}{>{\raggedright\arraybackslash}X}
\setlength{\parindent}{0pt}
\pagestyle{fancy}
\fancyhf{}
\renewcommand{\headrulewidth}{0pt}
\newcommand{\pie}{}
\newcommand{\finmodelo}{}
\fancyfoot[R]{\footnotesize\pie: \thepage{} / \pageref{\finmodelo}}
\newcommand{\empezarmodelo}[2]{\clearpage\setcounter{page}{1}\renewcommand{\pie}{#1}\renewcommand{\finmodelo}{#2}}
\newcommand{\sangria}{\hspace*{8mm}}
\newcommand{\enunciado}[1]{{\raggedright\bfseries\itshape #1\par}}
\newcommand{\opcion}[2]{#1)~#2}
\newcommand{\opcioncorrecta}[2]{\textcolor{verde}{\textbf{#1)~#2~\checkmark}}}
% Separación entre preguntas y tamaño de letra del examen (los fija el nivel de compactación tras \begin{document}).
\newlength{\seppregunta}\setlength{\seppregunta}{6pt}
\newcommand{\letraexamen}{\normalsize}
% Cada pregunta va en un bloque que no se parte entre dos páginas.
\newenvironment{pregunta}{\par\addvspace{\seppregunta}\noindent\begin{minipage}{\linewidth}}{\end{minipage}\par}
\newsavebox{\cajaestructura}
% Código de los enunciados, como en el PDF: en un recuadro gris (el marco y el fondo de listings) del ancho de su línea
% más larga (como mucho, el de la línea del examen, y entonces se parten las líneas que no caben), resaltado según su
% lenguaje: palabras clave en negrita, comentarios en cursiva y cadenas en granate.
% \begin{codigo}[opciones de listings]{caracteres de la línea más larga}{caracteres de los números de línea y su
% separación, o 0}. No se mete en una caja (minipage, lrbox): listings dejaría mal las letras con tilde del texto que
% sigue. Los caracteres que no son ASCII van como texto LaTeX entre (*@ y @*), salvo las letras con tilde, diéresis o
% eñe, que las cambia listings.
\definecolor{fondocodigo}{HTML}{EEEEEE}
\definecolor{bordecodigo}{HTML}{808080}
\definecolor{codigoclave}{HTML}{1F3A93}
\definecolor{codigocomentario}{HTML}{666666}
\definecolor{codigocadena}{HTML}{8B1A1A}
\definecolor{codigonumero}{HTML}{808080}
\lstset{basicstyle=\ttfamily, columns=fullflexible, keepspaces=true, showstringspaces=false, tabsize=4,
  breaklines=true, keywordstyle=\bfseries\color{codigoclave},
  commentstyle=\itshape\color{codigocomentario}, stringstyle=\color{codigocadena},
  numberstyle=\ttfamily\color{codigonumero}, escapeinside={(*@}{@*)}, frame=single, framesep=4pt,
  framerule=0.4pt, rulecolor=\color{bordecodigo}, backgroundcolor=\color{fondocodigo}, aboveskip=4pt, belowskip=4pt,
  literate={á}{{\'a}}1 {é}{{\'e}}1 {í}{{\'i}}1 {ó}{{\'o}}1 {ú}{{\'u}}1 {Á}{{\'A}}1 {É}{{\'E}}1 {Í}{{\'I}}1
    {Ó}{{\'O}}1 {Ú}{{\'U}}1 {ü}{{\"u}}1 {Ü}{{\"U}}1 {ñ}{{\~n}}1 {Ñ}{{\~N}}1}
\lstdefinelanguage{JSON}{morestring=[b]", morekeywords={true,false,null}, sensitive=true}
\lstdefinelanguage{YAML}{morecomment=[l]\#, morestring=[b]", morestring=[b]', morekeywords={true,false,null,yes,no},
  sensitive=false}
\newlength{\anchocaracter}
\newlength{\anchocodigo}
\newlength{\anchonumeros}
\lstnewenvironment{codigo}[3][]{%
  \settowidth{\anchocaracter}{\ttfamily 0}%
  \setlength{\anchocodigo}{#2\anchocaracter}%
  \setlength{\anchonumeros}{#3\anchocaracter}%
  \ifdim\anchocodigo>\dimexpr\linewidth-8mm-8.8pt-\anchonumeros\relax
    \setlength{\anchocodigo}{\dimexpr\linewidth-8mm-8.8pt-\anchonumeros\relax}\fi
  \lstset{framexleftmargin=\anchonumeros, xleftmargin=\dimexpr8mm+4.4pt+\anchonumeros\relax,
    linewidth=\dimexpr8mm+8.8pt+\anchonumeros+\anchocodigo\relax, #1}}{}
)tex";

// Sin los caracteres de control (invisibles, salvo el tabulador y los saltos de línea), que pdflatex no admite: con uno
// solo, el documento no compila. Llegan, por ejemplo, al copiar de Word (U+000B, su salto de línea manual).
bool esDeControl(QChar c)
{
    const char16_t u = c.unicode();
    return (u < 0x20 && u != '\t' && u != '\n' && u != '\r') || (u >= 0x7F && u <= 0x9F);
}

// Si el texto tiene algo que se ve: no solo espacios, caracteres de control, de formato (invisibles, como U+200B) o
// marcas que se combinan con la letra anterior.
bool tieneAlgoVisible(const QString &texto)
{
    for (const QChar c : texto)
        if (!c.isSpace() && !esDeControl(c) && c.category() != QChar::Other_Format && c.category() != QChar::Mark_NonSpacing
            && c.category() != QChar::Mark_Enclosing)
            return true;
    return false;
}

QString imagenLatex(const Pregunta &p, const QString &carpeta, int n)
{
    const int ancho = anchoImagen(p, n);
    if (ancho == 0)
        return QString();
    return QStringLiteral("\\par\\smallskip\\sangria\\includegraphics[width=%1cm]{%2/%3}\\par\\smallskip\n")
        .arg(QString::number(ancho * 2.54 / 96, 'f', 2), carpeta, Maquetador::nombreImagen(p));
}

QString opcionLatex(int i, const QString &texto, bool marcada)
{
    return QStringLiteral("\\%1{%2}{%3}")
        .arg(marcada ? QStringLiteral("opcioncorrecta") : QStringLiteral("opcion"), QString(Pregunta::letra(i)),
             Maquetador::escaparLatex(texto));
}

// Equivalente a opcionesConImagenHtml(): las opciones, una por línea, y la imagen a su derecha, con su borde de
// arriba a la altura del de la primera opción.
QString opcionesConImagenLatex(const PreguntaExamen &pe, bool soluciones, const QString &carpeta, int n)
{
    QStringList opciones;
    for (int i = 0; i < 4; ++i)
        opciones << opcionLatex(i, pe.opcion(i), soluciones && i == pe.correcta());
    return QStringLiteral("\\par\\vspace{1pt}\n\\noindent\\sangria\\begin{tabular}[t]{@{}l@{}}\n%1\n\\end{tabular}"
                          "\\hspace{%2mm}\\raisebox{\\dimexpr\\ht\\strutbox-\\height}{\\includegraphics[width=%3cm]{%4/%5}}\\par\n")
        .arg(opciones.join(QStringLiteral(" \\\\\n")), QString::number(kSeparacionImagen * 25.4 / 96, 'f', 1),
             QString::number(anchoImagen(pe.pregunta, n) * 2.54 / 96, 'f', 2), carpeta, Maquetador::nombreImagen(pe.pregunta));
}

// Mismo reparto que opcionesHtml: en una fila, en dos columnas (a c / b d) o una por línea.
QString opcionesLatex(const PreguntaExamen &pe, bool soluciones, int n)
{
    const int filas = filasOpciones(pe, n);
    const int correcta = pe.correcta();
    auto op = [&](int i) { return opcionLatex(i, pe.opcion(i), soluciones && i == correcta); };
    const QString finFila = QStringLiteral(" \\\\\n");
    const QString sep = QStringLiteral(" & ");
    QString columnas;
    QString cuerpo;
    if (filas == 1) {
        columnas = QStringLiteral("YYYY");
        cuerpo = op(0) + sep + op(1) + sep + op(2) + sep + op(3) + finFila;
    } else if (filas == 2) {
        columnas = QStringLiteral("YY");
        for (int fila = 0; fila < 2; ++fila)
            cuerpo += op(fila) + sep + op(fila + 2) + finFila;
    } else {
        columnas = QStringLiteral("Y");
        for (int i = 0; i < 4; ++i)
            cuerpo += op(i) + finFila;
    }
    return QStringLiteral("\\par\\vspace{1pt}\n\\begin{tabularx}{\\linewidth}{@{\\sangria}%1@{}}\n%2\\end{tabularx}\n")
        .arg(columnas, cuerpo);
}

// Equivalente a codigoHtml(): el entorno codigo del preámbulo, con el lenguaje en listings y los números de línea si los
// lleva. Los tabuladores pasan a ser cuatro espacios.
QString codigoLatex(const Enunciado::Bloque &b)
{
    static const QString conLiterate = QStringLiteral("áéíóúÁÉÍÓÚüÜñÑ");
    QStringList lineas;
    int maximo = 1;
    for (QString linea : b.lineas) {
        linea.replace(QLatin1Char('\t'), QStringLiteral("    "));
        linea.removeIf(esDeControl);
        // Los caracteres que van como texto LaTeX («→») pueden ser más anchos que los demás: cuentan por dos.
        int ancho = 0;
        QString escrita;
        for (const QChar c : linea) {
            const bool comoTexto = c.unicode() >= 128 && !conLiterate.contains(c);
            escrita += comoTexto ? QStringLiteral("(*@%1@*)").arg(c) : QString(c);
            ancho += comoTexto ? 2 : 1;
        }
        maximo = qMax(maximo, ancho);
        lineas << escrita;
    }
    QStringList opciones;
    const QString lenguaje = Resaltado::enListings(Resaltado::lenguaje(b.lenguaje));
    // Entre llaves si lleva corchetes («[CORBA]IDL»), que cerrarían antes de tiempo las opciones de \begin{codigo}.
    if (!lenguaje.isEmpty())
        opciones << QStringLiteral("language=") + (lenguaje.contains(QLatin1Char('[')) ? QLatin1Char('{') + lenguaje + QLatin1Char('}') : lenguaje);
    // En XML y HTML, el nombre de cada etiqueta en negrita, como las palabras clave (la opción solo existe con ellos).
    if (lenguaje == QLatin1String("XML") || lenguaje == QLatin1String("HTML"))
        opciones << QStringLiteral("markfirstintag=true");
    // Los números de línea, a la izquierda y dentro del recuadro: sus cifras y un espacio.
    int numeros = 0;
    if (b.numerado) {
        numeros = QString::number(qMax(1, int(b.lineas.size()))).size() + 1;
        opciones << QStringLiteral("numbers=left") << QStringLiteral("numbersep=\\anchocaracter");
    }
    return QStringLiteral("\\begin{codigo}[%1]{%2}{%3}\n%4\n\\end{codigo}\n")
        .arg(opciones.join(QLatin1Char(',')), QString::number(maximo), QString::number(numeros), lineas.join(QLatin1Char('\n')));
}

// Equivalente a explicacionHtml: la explicación y la referencia de la pregunta, debajo de las opciones.
QString explicacionLatex(const Pregunta &p, const DatosConvocatoria &d, int n)
{
    QStringList partes;
    auto parte = [&](const char *etiqueta, const QString &texto) {
        if (texto.trimmed().isEmpty())
            return;
        QStringList lineas;
        for (const QString &linea : texto.trimmed().split(QLatin1Char('\n')))
            lineas << Maquetador::escaparLatex(linea);
        partes << QStringLiteral("\\textcolor{verde}{\\textbf{%1}} %2")
                      .arg(Maquetador::escaparLatex(tx(d, etiqueta)), lineas.join(QStringLiteral("\\newline ")));
    };
    parte(QT_TRANSLATE_NOOP("Examen", "Explicación:"), p.explicacion);
    parte(QT_TRANSLATE_NOOP("Examen", "Referencia:"), p.referencia);
    if (partes.isEmpty())
        return QString();
    return QStringLiteral("\\par\\vspace{2pt}{%1\\leftskip=8mm\\noindent %2\\par}\n")
        .arg(letraExplicacionLatex(n), partes.join(QStringLiteral("\\par\n\\noindent ")));
}

// Equivalente a Maquetador::enunciadoHtml: párrafos en negrita cursiva, bloques ``` y tablas | a | b |.
QString enunciadoLatex(const QString &texto, const QString &prefijo)
{
    QString tex;
    bool prefijoPuesto = prefijo.isEmpty();
    auto ponerPrefijoSolo = [&]() {
        if (!prefijoPuesto) {
            tex += QStringLiteral("\\enunciado{%1}\n").arg(prefijo);
            prefijoPuesto = true;
        }
    };
    for (const Enunciado::Bloque &b : Enunciado::bloques(texto)) {
        switch (b.tipo) {
        case Enunciado::Bloque::Parrafo: {
            // Sin las líneas que no tienen nada que se vea: un \\ sin nada delante («There's no line here to end») es un
            // error de LaTeX. Y tras cada \\, \relax: si la línea siguiente empezara por «[» (una lista de Python) o «*»,
            // LaTeX los tomaría por un argumento del \\ («\\[1cm]», «\\*»).
            QStringList lineas;
            for (const QString &linea : b.lineas)
                if (tieneAlgoVisible(linea))
                    lineas << Maquetador::escaparLatex(linea);
            if (lineas.isEmpty())
                break;
            QString contenido = lineas.join(QStringLiteral("\\\\\\relax\n"));
            if (!prefijoPuesto) {
                contenido = prefijo + contenido;
                prefijoPuesto = true;
            }
            tex += QStringLiteral("\\enunciado{%1}\n").arg(contenido);
            break;
        }
        case Enunciado::Bloque::Tabla: {
            ponerPrefijoSolo();
            int columnas = 0;
            for (const QStringList &fila : b.filas)
                columnas = qMax(columnas, int(fila.size()));
            tex += QStringLiteral("\\par\\smallskip\\sangria\\begin{tabular}{|%1}\\hline\n").arg(QStringLiteral("l|").repeated(columnas));
            for (const QStringList &fila : b.filas) {
                QStringList celdas;
                for (const QString &c : fila)
                    celdas << Maquetador::escaparLatex(c);
                while (celdas.size() < columnas)
                    celdas << QString();
                tex += celdas.join(QStringLiteral(" & ")) + QStringLiteral(" \\\\ \\hline\n");
            }
            tex += QStringLiteral("\\end{tabular}\\par\\smallskip\n");
            break;
        }
        case Enunciado::Bloque::Codigo:
            ponerPrefijoSolo();
            tex += codigoLatex(b);
            break;
        }
    }
    ponerPrefijoSolo();
    return tex;
}

// Con letra grande, la cabecera y las instrucciones van en \LARGE y el título en \huge.
QString cabeceraLatex(const DatosConvocatoria &d, const QString &derecha, int n = 0)
{
    // Como en cabeceraHtml, sin las líneas vacías: un \\ sin nada delante es un error de LaTeX («There's no line here
    // to end»), que pasaba con un banco sin asignatura.
    const bool grande = n == Maquetador::kLetraGrande;
    QStringList izquierda;
    if (!asignaturaCabecera(d).isEmpty())
        izquierda << QStringLiteral("{%1\\bfseries %2}").arg(grande ? QStringLiteral("\\huge") : QStringLiteral("\\Large"),
                                                          Maquetador::escaparLatex(asignaturaCabecera(d)));
    if (!convocatoriaCabecera(d).isEmpty())
        izquierda << Maquetador::escaparLatex(convocatoriaCabecera(d));
    if (izquierda.isEmpty() && derecha.isEmpty())
        return QString();
    return QStringLiteral("\\noindent\\begin{minipage}[t]{0.58\\linewidth}\n%1\n\\end{minipage}\\hfill\n"
                          "\\begin{minipage}[t]{0.4\\linewidth}\\raggedleft\n%2\n\\end{minipage}\\par\n")
        .arg(izquierda.join(QStringLiteral("\\\\\n")), derecha);
}

} // namespace

QString Maquetador::escaparLatex(const QString &texto)
{
    QString r;
    r.reserve(texto.size() + texto.size() / 8);
    for (const QChar c : texto) {
        if (esDeControl(c))
            continue;
        switch (c.unicode()) {
        case '\\': r += QStringLiteral("\\textbackslash{}"); break;
        case '{': case '}': case '$': case '&': case '#': case '_': case '%':
            r += QLatin1Char('\\');
            r += c;
            break;
        case '~': r += QStringLiteral("\\textasciitilde{}"); break;
        case '^': r += QStringLiteral("\\textasciicircum{}"); break;
        // Comillas, < > y ` sueltos: evitan ligaduras («<<» → «, «?`» → ¿...) y los atajos de babel.
        case '"': r += QStringLiteral("\\textquotedbl{}"); break;
        case '\'': r += QStringLiteral("\\textquotesingle{}"); break;
        case '`': r += QStringLiteral("\\textasciigrave{}"); break;
        case '<': r += QStringLiteral("\\textless{}"); break;
        case '>': r += QStringLiteral("\\textgreater{}"); break;
        case '|': r += QStringLiteral("\\textbar{}"); break;
        case '-':
            if (r.endsWith(QLatin1Char('-')))
                r += QStringLiteral("{}");   // «--» y «---» no deben convertirse en rayas
            r += c;
            break;
        default:
            r += c;
        }
    }
    return r;
}

QString Maquetador::examenesLatex(const QVector<Modelo> &modelos, const DatosConvocatoria &d, bool soluciones,
                                  const QString &carpetaImagenes, int n)
{
    // La semilla, que no va en los exámenes: en un comentario del principio y, al compilar con pdflatex, en las
    // propiedades del PDF (solo con los caracteres que no hay que escapar en ellas).
    const QString semilla = textoSemilla(d);
    QString tex = QStringLiteral("% ") + programa(d) + QLatin1Char('\n');
    tex += QString::fromUtf8(kPreambuloLatex);
    // babel, en el idioma del examen.
    if (d.idioma == QLatin1String("en"))
        tex.replace(QStringLiteral("\\usepackage[spanish,es-noshorthands]{babel}"), QStringLiteral("\\usepackage[english]{babel}"));
    if (!semilla.isEmpty())
        tex += QStringLiteral("\\ifdefined\\pdfinfo\\pdfinfo{/Subject (%1)}\\fi\n")
                   .arg(QString(semilla).remove(QRegularExpression(QStringLiteral("[^A-Za-z0-9 :.,-]"))));
    tex += QStringLiteral("\\begin{document}\n");
    // Nivel de compactación: separación entre preguntas y letra. Con letra grande, enunciados sin cursiva.
    tex += QStringLiteral("\\setlength{\\seppregunta}{%1pt}\\renewcommand{\\letraexamen}{%2}\n")
               .arg(QLatin1String(nivel(n).separacionLatex), QLatin1String(nivel(n).letraLatex));
    if (n == kLetraGrande)
        tex += QStringLiteral("\\renewcommand{\\enunciado}[1]{{\\raggedright\\bfseries #1\\par}}\n");
    if (soluciones) {
        tex += QStringLiteral("\n\\empezarmodelo{") + escaparLatex(textoPie(tx(d, QT_TRANSLATE_NOOP("Examen", "Soluciones")), d))
               + QStringLiteral("}{fin:soluciones}\n");
        tex += solucionesLatex(modelos, d);
        // En un párrafo propio: detrás de la tabla, que ocupa todo el ancho, pasaría a otra línea, y con la página
        // llena, a una página más, vacía salvo el pie.
        tex += QStringLiteral("\\par\\label{fin:soluciones}\n");
    }
    for (int i = 0; i < modelos.size(); ++i) {
        const QString etiqueta = QStringLiteral("fin:modelo%1").arg(i + 1);
        tex += QStringLiteral("\n\\empezarmodelo{") + escaparLatex(textoPie(modelos.at(i).nombre, d))
               + QStringLiteral("}{") + etiqueta + QStringLiteral("}\n");
        tex += cuerpoExamenLatex(modelos.at(i), d, soluciones, carpetaImagenes, n);
        tex += QStringLiteral("\\label{") + etiqueta + QStringLiteral("}\n");
    }
    return tex + QStringLiteral("\\end{document}\n");
}

QString Maquetador::cuerpoExamenLatex(const Modelo &modelo, const DatosConvocatoria &d, bool soluciones,
                                      const QString &carpetaImagenes, int n)
{
    QString derecha;
    if (soluciones)
        derecha = QStringLiteral("\\textcolor{verde}{\\bfseries %1}").arg(escaparLatex(tx(d, QT_TRANSLATE_NOOP("Examen", "CON SOLUCIONES"))));
    // Con letra grande, todo el modelo (cabecera e instrucciones incluidas) va en \letraexamen.
    QString tex = n == Maquetador::kLetraGrande ? QStringLiteral("{\\letraexamen\n") : QString();
    tex += cabeceraLatex(d, derecha, n);

    QStringList instrucciones;
    for (const QString &linea : d.instrucciones.split(QLatin1Char('\n')))
        if (!linea.trimmed().isEmpty())
            instrucciones << escaparLatex(linea);
    // Recuadro de instrucciones: el texto y, justo debajo, la puntuación en una línea (sin líneas en blanco, para que
    // sea bajo).
    instrucciones << escaparLatex(textoPuntuacion(d));
    tex += QStringLiteral(
               "\\par\\smallskip\n{\\setlength{\\fboxsep}{4pt}\\noindent\\fbox{\\begin{minipage}{\\dimexpr\\linewidth-2\\fboxsep-2\\fboxrule\\relax}\n"
               "%1\n\\end{minipage}}}\\par\\smallskip\n")
               .arg(instrucciones.join(QStringLiteral("\\par\n")));

    if (n != Maquetador::kLetraGrande)
        tex += QStringLiteral("{\\letraexamen\n");
    for (int i = 0; i < modelo.preguntas.size(); ++i) {
        const PreguntaExamen &pe = modelo.preguntas.at(i);
        QString prefijo = QStringLiteral("%1.- ").arg(i + 1);
        if (soluciones)
            prefijo += QStringLiteral("\\textcolor{verde}{\\upshape[%1]} ").arg(escaparLatex(pe.pregunta.codigoCompleto()));
        tex += QStringLiteral("\\begin{pregunta}\n");
        tex += enunciadoLatex(pe.pregunta.enunciado, prefijo);
        if (imagenAlLado(pe, n)) {
            tex += opcionesConImagenLatex(pe, soluciones, carpetaImagenes, n);
        } else {
            tex += imagenLatex(pe.pregunta, carpetaImagenes, n);
            tex += opcionesLatex(pe, soluciones, n);
        }
        if (soluciones)
            tex += explicacionLatex(pe.pregunta, d, n);
        tex += QStringLiteral("\\end{pregunta}\n");
    }
    tex += QStringLiteral("\\par}\n");
    return tex;
}

QString Maquetador::solucionesLatex(const QVector<Modelo> &modelos, const DatosConvocatoria &d)
{
    // A la derecha, el título y, debajo, la semilla con que se generaron los exámenes (no va en ellos).
    QString derecha = QStringLiteral("{\\LARGE\\bfseries %1}").arg(escaparLatex(tx(d, QT_TRANSLATE_NOOP("Examen", "Soluciones"))));
    if (!textoSemilla(d).isEmpty())
        derecha += QStringLiteral("\\\\\n{\\scriptsize\\color{darkgray}%1}").arg(escaparLatex(textoSemilla(d)));
    QString tex = cabeceraLatex(d, derecha);

    // Estructura: qué preguntas de cada tema entran en cada modelo. Si la tabla cabe en la línea, con su ancho natural;
    // si no (muchas preguntas o muchos temas), del ancho de la página, con las celdas partiendo sus líneas tras las
    // comas, como en el PDF. Se mide en LaTeX (\sbox) para no depender de paquetes que no son los básicos.
    const QList<int> temas = temasDe(modelos);
    QString cuerpo = QStringLiteral("\\hline\n\\textbf{%1}").arg(escaparLatex(tx(d, QT_TRANSLATE_NOOP("Examen", "Examen"))));
    for (int t : temas)
        cuerpo += QStringLiteral(" & \\textbf{T%1}").arg(t);
    cuerpo += QStringLiteral(" \\\\ \\hline\n");
    for (const Modelo &m : modelos) {
        cuerpo += QStringLiteral("\\textbf{") + escaparLatex(m.nombre) + QLatin1Char('}');
        for (int t : temas)
            cuerpo += QStringLiteral(" & ") + escaparLatex(codigosDelTema(m, t).join(QStringLiteral(", ")));
        cuerpo += QStringLiteral(" \\\\ \\hline\n");
    }
    tex += QStringLiteral("\\subsection*{%1}\n").arg(escaparLatex(tx(d, QT_TRANSLATE_NOOP("Examen", "Estructura de la convocatoria"))));
    tex += QStringLiteral("\\sbox{\\cajaestructura}{\\begin{tabular}{|l|%1}%2\\end{tabular}}%\n"
                          "\\ifdim\\wd\\cajaestructura>\\linewidth\n"
                          "\\begin{tabularx}{\\linewidth}{|l|%3}%2\\end{tabularx}\n"
                          "\\else\\usebox{\\cajaestructura}\\fi\n")
               .arg(QStringLiteral("l|").repeated(temas.size()), cuerpo, QStringLiteral("Y|").repeated(temas.size()));

    // Plantilla de corrección de cada modelo («1 C», «2 A»…, en filas de 10) y, debajo, código, respuesta y referencia de
    // cada pregunta.
    for (const Modelo &m : modelos) {
        const int n = m.preguntas.size();
        tex += QStringLiteral("\\subsection*{%1}\n")
                   .arg(escaparLatex(tx(d, QT_TRANSLATE_NOOP("Examen", "Plantilla de corrección: %1")).arg(m.nombre)));
        if (n == 0)
            continue;
        tex += QStringLiteral("\\begin{tabular}{|*{%1}{>{\\centering\\arraybackslash}p{9mm}|}}\n").arg(qMin(10, n));
        tex += QStringLiteral("\\cline{1-%1}\n").arg(qMin(10, n));
        for (int inicio = 0; inicio < n; inicio += 10) {
            const int fin = qMin(inicio + 10, n);
            QStringList celdas;
            for (int i = inicio; i < fin; ++i)
                celdas << QStringLiteral("%1~\\textbf{%2}").arg(QString::number(i + 1), QString(Pregunta::letraMayuscula(m.preguntas.at(i).correcta())));
            tex += celdas.join(QStringLiteral(" & ")) + QStringLiteral(" \\\\ \\cline{1-%1}\n").arg(fin - inicio);
        }
        tex += QStringLiteral("\\end{tabular}\n\\par\\smallskip\n")
               + escaparLatex(tx(d, QT_TRANSLATE_NOOP("Examen", "Respuestas correctas: %1")).arg(textoCorrectasPorLetra(m)))
               + QStringLiteral("\\par\\smallskip\n{\\footnotesize\n"
                              "\\begin{tabularx}{\\linewidth}{@{}r@{\\hspace{1em}}l@{\\hspace{1em}}l@{\\hspace{1em}}Y@{}}\n");
        for (int i = 0; i < n; ++i) {
            const PreguntaExamen &pe = m.preguntas.at(i);
            tex += QStringLiteral("%1. & %2 & \\textbf{%3} & ").arg(i + 1).arg(escaparLatex(pe.pregunta.codigoCompleto())).arg(Pregunta::letraMayuscula(pe.correcta()))
                   + escaparLatex(pe.pregunta.referencia) + QStringLiteral(" \\\\\n");
        }
        tex += QStringLiteral("\\end{tabularx}}\n");
    }
    return tex;
}
