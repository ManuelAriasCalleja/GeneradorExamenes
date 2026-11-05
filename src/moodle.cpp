#include "moodle.h"
#include "enunciado.h"
#include "estilo.h"
#include "idioma.h"
#include "maquetador.h"

#include <QFile>
#include <QXmlStreamWriter>

#include <algorithm>

namespace Moodle {

namespace {

// Enunciado en HTML sencillo, que el editor de Moodle conserva: párrafos, código en <pre> y tablas con borde.
QString enunciadoHtml(const QString &texto)
{
    QString html;
    for (const Enunciado::Bloque &b : Enunciado::bloques(texto)) {
        switch (b.tipo) {
        case Enunciado::Bloque::Parrafo: {
            QStringList lineas;
            for (const QString &linea : b.lineas)
                lineas << Maquetador::escapar(linea);
            html += QStringLiteral("<p>%1</p>").arg(lineas.join(QStringLiteral("<br>")));
            break;
        }
        case Enunciado::Bloque::Codigo:
            html += QStringLiteral("<pre>%1</pre>").arg(Maquetador::escapar(b.lineas.join(QLatin1Char('\n'))));
            break;
        case Enunciado::Bloque::Tabla:
            html += QStringLiteral("<table border=\"1\" cellpadding=\"3\" style=\"border-collapse:collapse\">");
            for (const QStringList &fila : b.filas) {
                html += QStringLiteral("<tr>");
                for (const QString &celda : fila)
                    html += QStringLiteral("<td>%1</td>").arg(Maquetador::escapar(celda));
                html += QStringLiteral("</tr>");
            }
            html += QStringLiteral("</table>");
            break;
        }
    }
    return html;
}

// Nombre de la pregunta en el banco de Moodle: el código y el principio del enunciado («T1-7a: Un byte son…»).
QString nombrePregunta(const Pregunta &p)
{
    QString principio;
    for (const Enunciado::Bloque &b : Enunciado::bloques(p.enunciado)) {
        if (b.tipo == Enunciado::Bloque::Parrafo) {
            principio = b.lineas.join(QLatin1Char(' ')).simplified();
            break;
        }
    }
    if (principio.isEmpty())
        principio = p.enunciado.simplified();
    if (principio.size() > 60)
        principio = principio.left(59).trimmed() + QStringLiteral("…");
    return principio.isEmpty() ? p.codigoCompleto() : QStringLiteral("%1: %2").arg(p.codigoCompleto(), principio);
}

// <elemento format="html"><text><![CDATA[…]]></text></elemento>
void escribirHtml(QXmlStreamWriter &x, const QString &elemento, const QString &html)
{
    x.writeStartElement(elemento);
    x.writeAttribute(QStringLiteral("format"), QStringLiteral("html"));
    x.writeStartElement(QStringLiteral("text"));
    if (!html.isEmpty())
        x.writeCDATA(html);
    x.writeEndElement();
    x.writeEndElement();
}

// En las rutas de categoría la barra separa niveles; Moodle la escapa doblándola.
QString nombreCategoria(QString nombre)
{
    return nombre.replace(QLatin1Char('/'), QStringLiteral("//"));
}

void escribirCategoria(QXmlStreamWriter &x, const QString &ruta)
{
    x.writeStartElement(QStringLiteral("question"));
    x.writeAttribute(QStringLiteral("type"), QStringLiteral("category"));
    x.writeStartElement(QStringLiteral("category"));
    x.writeTextElement(QStringLiteral("text"), ruta);
    x.writeEndElement();
    x.writeStartElement(QStringLiteral("info"));
    x.writeAttribute(QStringLiteral("format"), QStringLiteral("html"));
    x.writeTextElement(QStringLiteral("text"), QString());
    x.writeEndElement();
    x.writeEndElement();
}

void escribirPregunta(QXmlStreamWriter &x, const Pregunta &p, const Opciones &opciones)
{
    x.writeStartElement(QStringLiteral("question"));
    x.writeAttribute(QStringLiteral("type"), QStringLiteral("multichoice"));
    x.writeStartElement(QStringLiteral("name"));
    x.writeTextElement(QStringLiteral("text"), nombrePregunta(p));
    x.writeEndElement();

    // Enunciado y, si la hay, la imagen: en el HTML como @@PLUGINFILE@@ y su contenido en <file>.
    QString html = enunciadoHtml(p.enunciado);
    const QString imagen = Maquetador::nombreImagen(p);
    if (!p.imagen.isEmpty())
        html += QStringLiteral("<p><img src=\"@@PLUGINFILE@@/%1\" alt=\"Figura de la pregunta %2\"></p>")
                    .arg(imagen, Maquetador::escapar(p.codigoCompleto()));
    x.writeStartElement(QStringLiteral("questiontext"));
    x.writeAttribute(QStringLiteral("format"), QStringLiteral("html"));
    x.writeStartElement(QStringLiteral("text"));
    x.writeCDATA(html);
    x.writeEndElement();
    if (!p.imagen.isEmpty()) {
        x.writeStartElement(QStringLiteral("file"));
        x.writeAttribute(QStringLiteral("name"), imagen);
        x.writeAttribute(QStringLiteral("path"), QStringLiteral("/"));
        x.writeAttribute(QStringLiteral("encoding"), QStringLiteral("base64"));
        x.writeCharacters(QString::fromLatin1(p.imagen.toBase64()));
        x.writeEndElement();
    }
    x.writeEndElement();

    // Realimentación general: la referencia al libro y la explicación.
    QString realimentacion;
    if (!p.referencia.isEmpty())
        realimentacion += QStringLiteral("<p>%1</p>").arg(Maquetador::escapar(p.referencia));
    if (!p.explicacion.isEmpty())
        realimentacion += QStringLiteral("<p>%1</p>").arg(Maquetador::escapar(p.explicacion).replace(QLatin1Char('\n'), QStringLiteral("<br>")));
    escribirHtml(x, QStringLiteral("generalfeedback"), realimentacion);

    x.writeTextElement(QStringLiteral("defaultgrade"), QStringLiteral("1.0000000"));
    x.writeTextElement(QStringLiteral("penalty"), QStringLiteral("0.3333333"));
    x.writeTextElement(QStringLiteral("hidden"), QStringLiteral("0"));
    x.writeTextElement(QStringLiteral("single"), QStringLiteral("true"));
    x.writeTextElement(QStringLiteral("shuffleanswers"), Estilo::tieneOpcionesDependientes(p) ? QStringLiteral("false") : QStringLiteral("true"));
    x.writeTextElement(QStringLiteral("answernumbering"), QStringLiteral("abc"));
    x.writeTextElement(QStringLiteral("showstandardinstruction"), QStringLiteral("0"));
    escribirHtml(x, QStringLiteral("correctfeedback"),
                 QStringLiteral("<p>%1</p>").arg(Maquetador::escapar(Idioma::examen(opciones.idioma, QT_TRANSLATE_NOOP("Examen", "Respuesta correcta.")))));
    escribirHtml(x, QStringLiteral("partiallycorrectfeedback"), QString());
    escribirHtml(x, QStringLiteral("incorrectfeedback"),
                 QStringLiteral("<p>%1</p>").arg(Maquetador::escapar(Idioma::examen(opciones.idioma, QT_TRANSLATE_NOOP("Examen", "Respuesta incorrecta.")))));

    const QString erronea = opciones.penalizarFallos ? QStringLiteral("-30") : QStringLiteral("0");
    for (int i = 0; i < 4; ++i) {
        x.writeStartElement(QStringLiteral("answer"));
        x.writeAttribute(QStringLiteral("fraction"), i == p.correcta ? QStringLiteral("100") : erronea);
        x.writeAttribute(QStringLiteral("format"), QStringLiteral("html"));
        x.writeStartElement(QStringLiteral("text"));
        x.writeCDATA(QStringLiteral("<p>%1</p>").arg(Maquetador::escapar(p.opciones.value(i))));
        x.writeEndElement();
        escribirHtml(x, QStringLiteral("feedback"), QString());
        x.writeEndElement();
    }
    x.writeEndElement();
}

} // namespace

QVector<Pregunta> seleccion(const QVector<Pregunta> &preguntas, const Opciones &opciones)
{
    QVector<Pregunta> res;
    for (const Pregunta &p : preguntas)
        if (p.activa() && (opciones.incluirParaRevisar || !p.revisar))
            res << p;
    std::sort(res.begin(), res.end(), [](const Pregunta &a, const Pregunta &b) { return a.antesQue(b); });
    return res;
}

QString xml(const QVector<Tema> &temas, const QVector<Pregunta> &preguntas, const Opciones &opciones)
{
    QByteArray salida;
    QXmlStreamWriter x(&salida);
    x.setAutoFormatting(true);
    x.setAutoFormattingIndent(2);
    x.writeStartDocument();
    x.writeStartElement(QStringLiteral("quiz"));
    int temaActual = 0;
    for (const Pregunta &p : seleccion(preguntas, opciones)) {
        if (p.temaId != temaActual) {
            temaActual = p.temaId;
            QString tema = Idioma::examen(opciones.idioma, QT_TRANSLATE_NOOP("Examen", "Tema %1")).arg(p.temaNumero);
            for (const Tema &t : temas)
                if (t.id == p.temaId)
                    tema = t.nombreEn(opciones.idioma);
            const QString raiz = opciones.asignatura.trimmed().isEmpty()
                                     ? QStringLiteral("$course$/top")
                                     : QStringLiteral("$course$/top/") + nombreCategoria(opciones.asignatura);
            escribirCategoria(x, raiz + QLatin1Char('/') + nombreCategoria(tema));
        }
        escribirPregunta(x, p, opciones);
    }
    x.writeEndElement();
    x.writeEndDocument();
    return QString::fromUtf8(salida);
}

bool exportar(const QString &ruta, const QVector<Tema> &temas, const QVector<Pregunta> &preguntas, const Opciones &opciones,
              QString *error)
{
    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        *error = QStringLiteral("No se puede escribir %1: %2").arg(ruta, f.errorString());
        return false;
    }
    f.write(xml(temas, preguntas, opciones).toUtf8());
    return true;
}

} // namespace Moodle
