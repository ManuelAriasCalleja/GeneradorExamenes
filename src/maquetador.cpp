#include "maquetador.h"
#include "enunciado.h"

#include <QAbstractTextDocumentLayout>
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
#include <QScreen>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QUrl>
#include <QtMath>
#include <memory>

namespace {

const QStringList kMeses{QStringLiteral("Enero"), QStringLiteral("Febrero"), QStringLiteral("Marzo"), QStringLiteral("Abril"),
                         QStringLiteral("Mayo"), QStringLiteral("Junio"), QStringLiteral("Julio"), QStringLiteral("Agosto"),
                         QStringLiteral("Septiembre"), QStringLiteral("Octubre"), QStringLiteral("Noviembre"),
                         QStringLiteral("Diciembre")};

// Convocatorias ordinarias y su mes.
const QList<QPair<int, QString>> kConvocatoriasOrdinarias{
    {2, QStringLiteral("Febrero")}, {6, QStringLiteral("Junio")}, {9, QStringLiteral("Septiembre")}};

const char *const kFuente = "'Liberation Sans', Arial, 'DejaVu Sans', sans-serif";
const char *const kFuenteCodigo = "'DejaVu Sans Mono', 'Liberation Mono', monospace";

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

// Niveles de compactación de la maqueta (ver maquetador.h). El 0 es la maqueta normal. En LaTeX el artículo es de
// 10pt y se aprovecha mejor la página que en QTextDocument, así que basta con \small (9pt) en los niveles altos.
// kLetraGrande es la versión adaptada (letra grande).
struct Nivel
{
    qreal fuentePt;            // tamaño de la letra del examen
    int separacionPx;          // espacio entre preguntas
    bool dosColumnasSiAhorra;  // opciones largas en dos columnas cuando así ocupen menos líneas que una por línea
    const char *separacionLatex;   // lo mismo en LaTeX (en pt)
    const char *letraLatex;
};
const Nivel kNiveles[] = {
    {10.5, 12, false, "6", "\\normalsize"},
    {10.5, 10, true, "5", "\\normalsize"},
    {10.0, 8, true, "4", "\\small"},
    {9.5, 6, true, "3", "\\small"},
    {9.0, 4, true, "2", "\\small"},
};
const int kNumNiveles = int(sizeof(kNiveles) / sizeof(kNiveles[0]));
const Nivel kLetraGrande = {16.0, 24, false, "14", "\\LARGE"};

const Nivel &nivel(int n)
{
    return n == Maquetador::kLetraGrande ? kLetraGrande : kNiveles[qBound(0, n, kNumNiveles - 1)];
}

// Cuánto se agrandan la cabecera, las instrucciones y las imágenes respecto a la maqueta normal (10,5 pt).
qreal escala(int n)
{
    return nivel(n).fuentePt / kNiveles[0].fuentePt;
}

QString pt(qreal puntos)
{
    return QString::number(puntos, 'f', 1) + QStringLiteral("pt");
}

// Ancho con el que se muestra la imagen de la pregunta, en píxeles a 96 ppp (0 si no tiene).
int anchoImagen(const Pregunta &p, int n = 0)
{
    if (p.imagen.isEmpty())
        return 0;
    QImage img;
    if (!img.loadFromData(p.imagen))
        return 0;
    qreal dpi = img.dotsPerMeterX() * 0.0254;
    if (dpi < 50)
        dpi = 96;
    return qMin(qRound(500 * escala(n)), qRound(img.width() * 96.0 / dpi * escala(n)));
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

// Filas en que se reparten las opciones: 1 (a b c d), 2 (a c / b d) o 4 (una por línea). Se decide estimando
// los caracteres que caben en una línea (A4 con los márgenes de configurarPagina() y la letra del nivel). Con
// letra grande, siempre una por línea.
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

QString cabeceraHtml(const DatosConvocatoria &d, const QString &derecha, int n = 0)
{
    return QStringLiteral(
               "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"0\"><tr>"
               "<td><span style=\"font-size:%4; font-weight:bold\">%1</span><br>%2</td>"
               "<td align=\"right\" valign=\"top\">%3</td></tr></table>")
        .arg(Maquetador::escapar(d.asignatura), Maquetador::escapar(d.convocatoriaYAnio()), derecha, pt(14 * escala(n)));
}

// Pie de página de un examen o de la hoja de soluciones: «Nacional Original · Junio 2026».
QString textoPie(const QString &nombre, const DatosConvocatoria &d)
{
    const QString fecha = d.convocatoriaYAnio();
    return fecha.isEmpty() ? nombre : nombre + QStringLiteral(" · ") + fecha;
}

// Temas que aparecen en la convocatoria y códigos (en orden) de las preguntas de un tema en un modelo.
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

} // namespace

QStringList DatosConvocatoria::ordinarias()
{
    QStringList nombres;
    for (const auto &c : kConvocatoriasOrdinarias)
        nombres << c.second;
    return nombres;
}

int DatosConvocatoria::mesDe(const QString &convocatoria)
{
    const QString c = convocatoria.trimmed();
    for (int i = 0; i < kMeses.size(); ++i)
        if (c.startsWith(kMeses.at(i), Qt::CaseInsensitive))
            return i + 1;
    return 0;
}

QString DatosConvocatoria::nombreDeMes(int mes)
{
    return kMeses.value(mes - 1);
}

DatosConvocatoria DatosConvocatoria::proxima(const QDate &hoy)
{
    DatosConvocatoria d;
    for (const auto &c : kConvocatoriasOrdinarias) {
        if (c.first > hoy.month()) {
            d.convocatoria = c.second;
            d.anio = hoy.year();
            return d;
        }
    }
    d.convocatoria = kConvocatoriasOrdinarias.first().second;
    d.anio = hoy.year() + 1;
    return d;
}

DatosConvocatoria DatosConvocatoria::anterior(const QDate &hoy)
{
    DatosConvocatoria d;
    for (auto it = kConvocatoriasOrdinarias.crbegin(); it != kConvocatoriasOrdinarias.crend(); ++it) {
        if (it->first <= hoy.month()) {
            d.convocatoria = it->second;
            d.anio = hoy.year();
            return d;
        }
    }
    d.convocatoria = kConvocatoriasOrdinarias.last().second;
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

QString Maquetador::formatoPuntos(double v)
{
    return QLocale(QLocale::Spanish, QLocale::Spain).toString(v, 'f', 2);
}

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
            html += QStringLiteral("<pre style=\"margin-left:26px; margin-top:2px; margin-bottom:2px; font-family:%1; font-weight:normal; font-style:normal\">%2</pre>")
                        .arg(QLatin1String(kFuenteCodigo), escapar(b.lineas.join(QLatin1Char('\n'))));
            break;
        }
    }
    ponerPrefijoSolo();
    return html;
}

QString Maquetador::fichaPreguntaHtml(const Pregunta &p, const QString &nombreTema)
{
    QString estado = p.estado;
    estado[0] = estado[0].toUpper();
    QStringList etiquetas{estado};
    if (p.old)
        etiquetas << QStringLiteral("ya salió en exámenes (old)");
    if (p.revisar)
        etiquetas << QStringLiteral("para revisar");

    QString html = QStringLiteral("<html><body style=\"font-family:%1\">").arg(QLatin1String(kFuente));
    html += QStringLiteral("<p style=\"color:#555\">%1<br><b>%2</b> &middot; %3</p>")
                .arg(escapar(nombreTema), escapar(p.codigoCompleto()), escapar(etiquetas.join(QStringLiteral(" · "))));
    html += QStringLiteral("<div style=\"font-weight:bold; font-style:italic\">%1</div>").arg(enunciadoHtml(p.enunciado));
    html += imagenHtml(p);
    PreguntaExamen pe;
    pe.pregunta = p;
    html += opcionesHtml(pe, true, 0);
    html += QStringLiteral("<p><b>Respuesta correcta:</b> %1. %2</p>").arg(Pregunta::letraMayuscula(p.correcta)).arg(escapar(p.referencia));
    if (!p.explicacion.isEmpty())
        html += QStringLiteral("<p><b>Explicación:</b> %1</p>").arg(escapar(p.explicacion).replace(QLatin1Char('\n'), QStringLiteral("<br>")));
    if (!p.pseudoCon.isEmpty())
        html += QStringLiteral("<p><b>Pseudorrepetida con:</b> %1 (no saldrán juntas en un examen)</p>").arg(escapar(p.pseudoCon));
    if (!p.notas.isEmpty())
        html += QStringLiteral("<p><b>Notas:</b> %1</p>").arg(escapar(p.notas));
    return html + QStringLiteral("</body></html>");
}

int Maquetador::numNiveles()
{
    return kNumNiveles;
}

QString Maquetador::examenHtml(const Modelo &modelo, const DatosConvocatoria &d, bool soluciones, int n)
{
    return QStringLiteral("<html><body style=\"font-family:%1; font-size:%2pt\">%3</body></html>")
        .arg(QLatin1String(kFuente), QString::number(nivel(n).fuentePt), cuerpoExamen(modelo, d, soluciones, n));
}

QString Maquetador::examenesHtml(const QVector<Modelo> &modelos, const DatosConvocatoria &d, bool soluciones, int n)
{
    QString html = QStringLiteral("<html><body style=\"font-family:%1; font-size:%2pt\">")
                       .arg(QLatin1String(kFuente), QString::number(nivel(n).fuentePt));
    for (int i = 0; i < modelos.size(); ++i) {
        if (i > 0)
            html += QStringLiteral("<p style=\"page-break-before:always; margin:0\"></p>");
        html += cuerpoExamen(modelos.at(i), d, soluciones, n);
    }
    return html + QStringLiteral("</body></html>");
}

QString Maquetador::cuerpoExamen(const Modelo &modelo, const DatosConvocatoria &d, bool soluciones, int n)
{
    QString html;
    QString derecha;
    if (soluciones)
        derecha = QStringLiteral("<span style=\"color:%1; font-weight:bold\">CON SOLUCIONES</span>").arg(QLatin1String(Maquetador::kColorCorrecta));
    html += cabeceraHtml(d, derecha, n);

    // Recuadro de instrucciones: el texto y, justo debajo, la puntuación (sin líneas en blanco, para que sea bajo).
    QString instrucciones = escapar(d.instrucciones).replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    html += QStringLiteral(
                "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"4\" style=\"margin-top:6px; border-collapse:collapse; border:1px solid #000\"><tr><td style=\"border:1px solid #000\">"
                "%1"
                "<table cellspacing=\"0\" cellpadding=\"0\" style=\"margin-top:2px\">"
                "<tr><td>Pregunta contestada correctamente:&nbsp;&nbsp;</td><td align=\"right\">+ %2 puntos</td></tr>"
                "<tr><td>Pregunta contestada incorrectamente:&nbsp;&nbsp;</td><td align=\"right\">- %3 puntos</td></tr>"
                "<tr><td>Pregunta no contestada:</td><td align=\"right\">0 puntos</td></tr>"
                "</table></td></tr></table>")
                .arg(instrucciones, formatoPuntos(d.puntosAcierto), formatoPuntos(d.puntosFallo));

    const int separacion = nivel(n).separacionPx;
    for (int i = 0; i < modelo.preguntas.size(); ++i) {
        const PreguntaExamen &pe = modelo.preguntas.at(i);
        QString prefijo = QStringLiteral("<a name=\"q%1\">%1.-</a> ").arg(i + 1);
        if (soluciones)
            prefijo += QStringLiteral("<span style=\"color:%1; font-style:normal\">[%2]</span> ")
                           .arg(QLatin1String(Maquetador::kColorCorrecta), escapar(pe.pregunta.codigoCompleto()));
        // Con letra grande, el enunciado va en negrita sin cursiva, que cuesta más de leer.
        html += QStringLiteral("<div style=\"margin-top:%1px; font-weight:bold%3\">%2</div>")
                    .arg(i == 0 ? separacion + 4 : separacion)
                    .arg(enunciadoHtml(pe.pregunta.enunciado, prefijo),
                         n == Maquetador::kLetraGrande ? QString() : QStringLiteral("; font-style:italic"));
        html += imagenHtml(pe.pregunta, n);
        html += opcionesHtml(pe, soluciones, n);
    }
    html += QStringLiteral("<p style=\"margin:0\"><a name=\"fin\">&nbsp;</a></p>");
    return html;
}

QString Maquetador::solucionesHtml(const QVector<Modelo> &modelos, const DatosConvocatoria &d)
{
    QString html = QStringLiteral("<html><body style=\"font-family:%1; font-size:10pt\">").arg(QLatin1String(kFuente));
    html += cabeceraHtml(d, QStringLiteral("<span style=\"font-size:16pt; font-weight:bold\">Soluciones</span>"));

    // Estructura: qué preguntas de cada tema entran en cada modelo (como la tabla que se prepara a mano).
    const QList<int> temas = temasDe(modelos);
    html += QStringLiteral("<h3>Estructura de la convocatoria</h3>"
                           "<table cellspacing=\"0\" cellpadding=\"3\" style=\"border-collapse:collapse; border:1px solid #000\"><tr><th style=\"border:1px solid #000\">Examen</th>");
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

    // Plantilla de respuestas de cada modelo.
    for (const Modelo &m : modelos) {
        html += QStringLiteral("<h3>%1</h3>").arg(escapar(m.nombre));
        html += QStringLiteral("<table cellspacing=\"0\" cellpadding=\"3\" style=\"border-collapse:collapse; border:1px solid #000\">");
        for (int inicio = 0; inicio < m.preguntas.size(); inicio += 10) {
            QString filaNum = QStringLiteral("<tr>"), filaResp = QStringLiteral("<tr>");
            for (int i = inicio; i < qMin(inicio + 10, int(m.preguntas.size())); ++i) {
                filaNum += QStringLiteral("<td align=\"center\" width=\"34\" bgcolor=\"#eeeeee\" style=\"border:1px solid #000\">%1</td>").arg(i + 1);
                filaResp += QStringLiteral("<td align=\"center\" style=\"border:1px solid #000\"><b>%1</b></td>").arg(Pregunta::letraMayuscula(m.preguntas.at(i).correcta()));
            }
            html += filaNum + QStringLiteral("</tr>") + filaResp + QStringLiteral("</tr>");
        }
        html += QStringLiteral("</table><p style=\"margin-top:2px; margin-bottom:0\">Respuestas correctas: %1</p>")
                    .arg(textoCorrectasPorLetra(m));
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
    return html + QStringLiteral("</body></html>");
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
    p.fuentePie = fuenteExamen(8);
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
    for (int n = 0; n < kNumNiveles; ++n) {
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
    return kNumNiveles - 1;
}

bool Maquetador::imprimirExamenes(QPagedPaintDevice *dispositivo, const QVector<Modelo> &modelos, const DatosConvocatoria &d,
                                  bool soluciones, int nivel)
{
    const QVector<Pregunta> conImagen = preguntasDe(modelos);
    QVector<QTextDocument *> docs;
    QStringList pies;
    if (soluciones) {
        docs << crearDocumento(solucionesHtml(modelos, d), conImagen, nullptr);
        pies << textoPie(QStringLiteral("Soluciones"), d);
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
            painter.drawText(caja, Qt::AlignRight | Qt::AlignVCenter,
                             QStringLiteral("%1: %2 / %3").arg(pies.value(i)).arg(pg + 1).arg(paginas));
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
\usepackage[table]{xcolor}
\usepackage{graphicx,array,tabularx,fancyvrb,fancyhdr,amssymb}
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
)tex";

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
            QStringList lineas;
            for (const QString &linea : b.lineas)
                if (!linea.trimmed().isEmpty())   // una línea vacía con \\ daría error en LaTeX
                    lineas << Maquetador::escaparLatex(linea);
            if (lineas.isEmpty())
                break;
            QString contenido = lineas.join(QStringLiteral("\\\\\n"));
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
            tex += QStringLiteral("\\begin{Verbatim}[xleftmargin=8mm,formatcom=\\normalfont\\ttfamily]\n%1\n\\end{Verbatim}\n")
                       .arg(b.lineas.join(QLatin1Char('\n')));
            break;
        }
    }
    ponerPrefijoSolo();
    return tex;
}

// Con letra grande, la cabecera y las instrucciones van en \LARGE y el título en \huge.
QString cabeceraLatex(const DatosConvocatoria &d, const QString &derecha, int n = 0)
{
    const bool grande = n == Maquetador::kLetraGrande;
    QString izquierda = QStringLiteral("{%1\\bfseries %2}").arg(grande ? QStringLiteral("\\huge") : QStringLiteral("\\Large"),
                                                               Maquetador::escaparLatex(d.asignatura));
    if (!d.convocatoriaYAnio().isEmpty())
        izquierda += QStringLiteral("\\\\\n") + Maquetador::escaparLatex(d.convocatoriaYAnio());
    return QStringLiteral("\\noindent\\begin{minipage}[t]{0.58\\linewidth}\n%1\n\\end{minipage}\\hfill\n"
                          "\\begin{minipage}[t]{0.4\\linewidth}\\raggedleft\n%2\n\\end{minipage}\\par\n")
        .arg(izquierda, derecha);
}

} // namespace

QString Maquetador::escaparLatex(const QString &texto)
{
    QString r;
    r.reserve(texto.size() + texto.size() / 8);
    for (const QChar c : texto) {
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
    QString tex = QString::fromUtf8(kPreambuloLatex);
    tex += QStringLiteral("\\begin{document}\n");
    // Nivel de compactación: separación entre preguntas y letra. Con letra grande, enunciados sin cursiva.
    tex += QStringLiteral("\\setlength{\\seppregunta}{%1pt}\\renewcommand{\\letraexamen}{%2}\n")
               .arg(QLatin1String(nivel(n).separacionLatex), QLatin1String(nivel(n).letraLatex));
    if (n == kLetraGrande)
        tex += QStringLiteral("\\renewcommand{\\enunciado}[1]{{\\raggedright\\bfseries #1\\par}}\n");
    if (soluciones) {
        tex += QStringLiteral("\n\\empezarmodelo{") + escaparLatex(textoPie(QStringLiteral("Soluciones"), d))
               + QStringLiteral("}{fin:soluciones}\n");
        tex += solucionesLatex(modelos, d);
        tex += QStringLiteral("\\label{fin:soluciones}\n");
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
        derecha = QStringLiteral("\\textcolor{verde}{\\bfseries CON SOLUCIONES}");
    // Con letra grande, todo el modelo (cabecera e instrucciones incluidas) va en \letraexamen.
    QString tex = n == Maquetador::kLetraGrande ? QStringLiteral("{\\letraexamen\n") : QString();
    tex += cabeceraLatex(d, derecha, n);

    QStringList instrucciones;
    for (const QString &linea : d.instrucciones.split(QLatin1Char('\n')))
        if (!linea.trimmed().isEmpty())
            instrucciones << escaparLatex(linea);
    // Recuadro de instrucciones: el texto y, justo debajo, la puntuación (sin líneas en blanco, para que sea bajo).
    tex += QStringLiteral(
               "\\par\\smallskip\n{\\setlength{\\fboxsep}{4pt}\\noindent\\fbox{\\begin{minipage}{\\dimexpr\\linewidth-2\\fboxsep-2\\fboxrule\\relax}\n"
               "%1\\par\\vspace{2pt}\n"
               "\\begin{tabular}{@{}l@{\\hspace{1em}}r@{}}\n"
               "Pregunta contestada correctamente: & + %2 puntos \\\\\n"
               "Pregunta contestada incorrectamente: & - %3 puntos \\\\\n"
               "Pregunta no contestada: & 0 puntos\n"
               "\\end{tabular}\n\\end{minipage}}}\\par\\smallskip\n")
               .arg(instrucciones.join(QStringLiteral("\\par\n")), formatoPuntos(d.puntosAcierto), formatoPuntos(d.puntosFallo));

    if (n != Maquetador::kLetraGrande)
        tex += QStringLiteral("{\\letraexamen\n");
    for (int i = 0; i < modelo.preguntas.size(); ++i) {
        const PreguntaExamen &pe = modelo.preguntas.at(i);
        QString prefijo = QStringLiteral("%1.- ").arg(i + 1);
        if (soluciones)
            prefijo += QStringLiteral("\\textcolor{verde}{\\upshape[%1]} ").arg(escaparLatex(pe.pregunta.codigoCompleto()));
        tex += QStringLiteral("\\begin{pregunta}\n");
        tex += enunciadoLatex(pe.pregunta.enunciado, prefijo);
        tex += imagenLatex(pe.pregunta, carpetaImagenes, n);
        tex += opcionesLatex(pe, soluciones, n);
        tex += QStringLiteral("\\end{pregunta}\n");
    }
    tex += QStringLiteral("\\par}\n");
    return tex;
}

QString Maquetador::solucionesLatex(const QVector<Modelo> &modelos, const DatosConvocatoria &d)
{
    QString tex = cabeceraLatex(d, QStringLiteral("{\\LARGE\\bfseries Soluciones}"));

    // Estructura: qué preguntas de cada tema entran en cada modelo.
    const QList<int> temas = temasDe(modelos);
    tex += QStringLiteral("\\subsection*{Estructura de la convocatoria}\n\\begin{tabular}{|l|%1}\\hline\n\\textbf{Examen}")
               .arg(QStringLiteral("l|").repeated(temas.size()));
    for (int t : temas)
        tex += QStringLiteral(" & \\textbf{T%1}").arg(t);
    tex += QStringLiteral(" \\\\ \\hline\n");
    for (const Modelo &m : modelos) {
        tex += QStringLiteral("\\textbf{") + escaparLatex(m.nombre) + QLatin1Char('}');
        for (int t : temas)
            tex += QStringLiteral(" & ") + escaparLatex(codigosDelTema(m, t).join(QStringLiteral(", ")));
        tex += QStringLiteral(" \\\\ \\hline\n");
    }
    tex += QStringLiteral("\\end{tabular}\n");

    // Plantilla de respuestas de cada modelo (filas de 10) y, debajo, código y referencia de cada pregunta.
    for (const Modelo &m : modelos) {
        const int n = m.preguntas.size();
        tex += QStringLiteral("\\subsection*{%1}\n").arg(escaparLatex(m.nombre));
        if (n == 0)
            continue;
        tex += QStringLiteral("\\begin{tabular}{|*{%1}{>{\\centering\\arraybackslash}p{6mm}|}}\n").arg(qMin(10, n));
        for (int inicio = 0; inicio < n; inicio += 10) {
            const int fin = qMin(inicio + 10, n);
            QStringList numeros, letras;
            for (int i = inicio; i < fin; ++i) {
                numeros << QStringLiteral("\\cellcolor{gray!15}%1").arg(i + 1);
                letras << QStringLiteral("\\textbf{%1}").arg(Pregunta::letraMayuscula(m.preguntas.at(i).correcta()));
            }
            const QString linea = QStringLiteral("\\cline{1-%1}\n").arg(fin - inicio);
            tex += linea + numeros.join(QStringLiteral(" & ")) + QStringLiteral(" \\\\\n")
                   + linea + letras.join(QStringLiteral(" & ")) + QStringLiteral(" \\\\\n") + linea;
        }
        tex += QStringLiteral("\\end{tabular}\n\\par\\smallskip\nRespuestas correctas: ") + textoCorrectasPorLetra(m)
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
