#include "enunciado.h"

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
            if (!enCodigo)
                res.append(Bloque{Bloque::Codigo, {}, {}});   // aunque se quede vacío
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

} // namespace Enunciado
