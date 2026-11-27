#pragma once

// Comprobaciones de lo que sale en un examen exportado (PDF, HTML, Word, LaTeX compilado): que cada pregunta esté en su
// orden y con todos sus textos. Las usan las sondas (con preguntas inventadas) y la prueba de los bancos reales.

#include "enunciado.h"
#include "generador.h"
#include "maquetador.h"

#include <QRegularExpression>
#include <QStringList>
#include <QXmlStreamReader>

// Lo que se compara de un texto exportado: sus letras y cifras, en minúsculas (y los superíndices y subíndices como
// cifras normales, que es como salen en el LaTeX). Los espacios, los saltos de línea, la puntuación y los números de
// línea del código cambian de un formato a otro. Tampoco cuentan las letras modificadoras («ˆ», que en el LaTeX sale
// como «^»).
inline QString soloLetras(const QString &texto)
{
    QString res;
    for (const QChar c : texto.normalized(QString::NormalizationForm_KC))
        if (c.isLetterOrNumber() && c.category() != QChar::Letter_Modifier)
            res += c.toLower();
    return res;
}

// Los textos de una pregunta del examen que deben salir, ya con soloLetras(): las líneas y las celdas del enunciado,
// sus opciones (en el orden en que se ven) y, con soluciones, su explicación y su referencia. Sin los muy cortos, que se
// podrían encontrar en cualquier sitio.
struct TrozosDePregunta
{
    QStringList enunciado;
    QStringList opciones;
    QStringList solucion;
};

inline TrozosDePregunta trozosDe(const Pregunta &p, const std::array<int, 4> &orden, bool soluciones)
{
    TrozosDePregunta t;
    for (const Enunciado::Bloque &b : Enunciado::bloques(p.enunciado)) {
        for (const QStringList &fila : b.filas)
            for (const QString &celda : fila)
                t.enunciado << soloLetras(Enunciado::sinMarcas(celda));
        for (const QString &linea : b.lineas)
            t.enunciado << soloLetras(b.tipo == Enunciado::Bloque::Codigo ? linea : Enunciado::sinMarcas(linea));
    }
    for (int i = 0; i < 4; ++i)
        for (const QString &linea : p.opciones.value(orden[i]).split(QLatin1Char('\n')))
            t.opciones << soloLetras(Enunciado::sinMarcas(linea));
    if (soluciones)
        for (const QString &texto : {p.explicacion, p.referencia})
            for (const QString &linea : texto.split(QLatin1Char('\n')))
                t.solucion << soloLetras(Enunciado::sinMarcas(linea));
    for (QStringList *l : {&t.enunciado, &t.opciones, &t.solucion})
        l->removeIf([](const QString &s) { return s.size() < 4; });
    return t;
}

// Lo que falta en 'texto' (el de un fichero exportado) de las preguntas del examen: cada una debe estar en su orden, con
// todos sus trozos (trozosDe()) entre el principio de su enunciado y el de la siguiente. Cada pregunta se busca tras la
// anterior por su enunciado entero seguido de su opción a) (que no se confunde con el mismo texto dentro de otra
// pregunta); si no está así (el código numerado mete los números entre sus líneas), por su enunciado entero, y si no,
// por su primera línea. Con 'conTextos', lo que falta se dice con su texto; sin él (con los bancos reales), solo qué es.
inline QStringList faltanDelExamen(const QString &texto, const Modelo &m, bool soluciones, bool conTextos = true)
{
    const QString t = soloLetras(texto);
    QStringList faltan;
    QVector<TrozosDePregunta> trozos;
    QVector<int> inicios;
    int desde = 0;
    for (int k = 0; k < m.preguntas.size(); ++k) {
        const PreguntaExamen &pe = m.preguntas.at(k);
        trozos << trozosDe(pe.pregunta, pe.orden, soluciones);
        const QString enunciado = trozos.last().enunciado.join(QString());
        const QString primeraOpcion = soloLetras(Enunciado::sinMarcas(pe.opcion(0)));
        int inicio = -1;
        for (const QString &buscado : {enunciado + QLatin1Char('a') + primeraOpcion, enunciado, trozos.last().enunciado.value(0)}) {
            if (!buscado.isEmpty() && inicio < 0)
                inicio = int(t.indexOf(buscado, desde));
        }
        if (inicio < 0)
            faltan << QStringLiteral("pregunta %1 (%2): no está su enunciado (o no en su sitio)").arg(k + 1).arg(pe.pregunta.codigoCompleto());
        else
            desde = inicio + 1;
        inicios << inicio;
    }
    for (int k = 0; k < m.preguntas.size(); ++k) {
        if (inicios.at(k) < 0)
            continue;
        int fin = int(t.size());
        for (int j = k + 1; j < m.preguntas.size(); ++j)
            if (inicios.at(j) >= 0) {
                fin = inicios.at(j);
                break;
            }
        const QString suyo = t.mid(inicios.at(k), fin - inicios.at(k));
        const auto mirar = [&](const QStringList &l, const QString &que) {
            for (const QString &trozo : l)
                if (!suyo.contains(trozo))
                    faltan << QStringLiteral("pregunta %1 (%2): falta %3%4")
                                  .arg(k + 1)
                                  .arg(m.preguntas.at(k).pregunta.codigoCompleto(), que,
                                       conTextos ? QStringLiteral(" «%1»").arg(trozo.left(50)) : QString());
        };
        mirar(trozos.at(k).enunciado, QStringLiteral("un trozo del enunciado"));
        mirar(trozos.at(k).opciones, QStringLiteral("una opción"));
        mirar(trozos.at(k).solucion, QStringLiteral("un trozo de la explicación o la referencia"));
    }
    return faltan;
}

// El texto de un PDF exportado sin sus pies de página («Asignatura · Modelo · Junio 2026: 1 / 2», que empiezan por la
// asignatura de 'd'), que pueden caer en medio de una pregunta larga o pegados a la última línea de la página. Cuando
// un párrafo se parte entre dos páginas, Qt escribe otra vez su última línea de la primera al principio de la segunda,
// fuera de la página: no se ve, pero está en el texto del PDF (ver «Vistos y no arreglados» en
// docs/revision_errores.md). Esa repetida, justo detrás del pie, también se quita.
inline QString textoSinPies(const QString &texto, const DatosConvocatoria &d)
{
    const QRegularExpression pie(QRegularExpression::escape(d.asignatura) + QStringLiteral(".*: (\\d+|\\?\\?) / (\\d+|\\?\\?)"));
    QStringList lineas;
    bool trasPie = false;
    for (const QString &linea : texto.split(QLatin1Char('\n'))) {
        const QRegularExpressionMatch m = pie.match(linea);
        if (m.hasMatch()) {
            const QString antes = linea.left(m.capturedStart()).trimmed();
            if (!antes.isEmpty())
                lineas << antes;
            trasPie = true;
            continue;
        }
        if (!trasPie || lineas.isEmpty() || linea.trimmed() != lineas.last().trimmed())
            lineas << linea;
        trasPie = false;
    }
    return lineas.join(QLatin1Char('\n'));
}

// El texto del cuerpo de un Word (sus párrafos, uno por línea).
inline QString textoDeWord(const QByteArray &documento)
{
    QString texto;
    QXmlStreamReader r(documento);
    while (!r.atEnd()) {
        r.readNext();
        if (r.isStartElement() && r.name() == QLatin1String("p"))
            texto += QLatin1Char('\n');
        else if (r.isStartElement() && r.name() == QLatin1String("t"))
            texto += r.readElementText();
    }
    return texto;
}
