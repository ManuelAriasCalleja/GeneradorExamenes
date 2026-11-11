#include "enunciado.h"
#include "resaltado.h"

#include <QCoreApplication>

namespace Enunciado {

QVector<Bloque> bloques(const QString &texto)
{
    QVector<Bloque> res;
    bool enCodigo = false;
    // Las líneas seguidas de texto (o de tabla) forman un mismo bloque.
    auto bloqueDe = [&res](Bloque::Tipo tipo) -> Bloque & {
        if (res.isEmpty() || res.last().tipo != tipo)
            res.append(Bloque{tipo, {}, {}});
        return res.last();
    };
    for (const QString &linea : texto.split(QLatin1Char('\n'))) {
        const QString t = linea.trimmed();
        if (t.startsWith(QLatin1String("```"))) {
            if (!enCodigo) {
                res.append(Bloque{Bloque::Codigo, {}, {}});   // aunque se quede vacío
                Bloque &b = res.last();
                b.cerrado = false;
                // «```java numerado»: el lenguaje (la primera palabra que no es «numerado») y las opciones.
                for (const QString &palabra : t.mid(3).split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
                    if (palabra.compare(QLatin1String("numerado"), Qt::CaseInsensitive) == 0)
                        b.numerado = true;
                    else if (b.lenguaje.isEmpty())
                        b.lenguaje = palabra.toLower();
                    else
                        b.sobrantes << palabra;
                }
            } else {
                res.last().cerrado = true;
            }
            enCodigo = !enCodigo;
        } else if (enCodigo) {
            res.last().lineas << linea;
        } else if (t.startsWith(QLatin1Char('|'))) {
            QString contenido = t.mid(1);
            if (contenido.endsWith(QLatin1Char('|')))
                contenido.chop(1);
            QStringList celdas = contenido.split(QLatin1Char('|'));
            for (QString &c : celdas)
                c = c.trimmed();
            bloqueDe(Bloque::Tabla).filas << celdas;
        } else {
            bloqueDe(Bloque::Parrafo).lineas << linea;
        }
    }
    return res;
}

QStringList avisosDelCodigo(const QString &texto)
{
    const auto tr = [](const char *t) { return QCoreApplication::translate("Enunciado", t); };
    QStringList avisos;
    for (const Bloque &b : bloques(texto)) {
        if (b.tipo != Bloque::Codigo)
            continue;
        if (!b.lenguaje.isEmpty() && !Resaltado::conocido(b.lenguaje))
            //: %1 es el lenguaje que se ha escrito tras ``` y %2, la lista de los que conoce el programa
            avisos << tr("El código dice ser «%1», un lenguaje que el programa no resalta: saldrá sin resaltar. Los que "
                         "resalta son %2.").arg(b.lenguaje, Resaltado::lenguajes().join(QStringLiteral(", ")));
        if (!b.sobrantes.isEmpty())
            //: «numerado» no se traduce: es la palabra que hay que escribir tras ``` para que el código lleve números de línea
            avisos << tr("En la línea que abre el código sobra «%1»: detrás de ``` solo van el lenguaje y «numerado».")
                          .arg(b.sobrantes.join(QLatin1Char(' ')));
        if (!b.cerrado)
            avisos << tr("Un bloque de código no se cierra con otra línea ```: llega hasta el final del enunciado.");
    }
    return avisos;
}

} // namespace Enunciado
