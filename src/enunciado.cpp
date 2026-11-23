#include "enunciado.h"
#include "resaltado.h"

#include <QCoreApplication>

namespace Enunciado {

QVector<Trozo> trozos(const QString &texto)
{
    QVector<Trozo> res;
    auto anadir = [&res](const QString &t, bool codigo) {
        if (t.isEmpty())
            return;
        if (!codigo && !res.isEmpty() && !res.last().codigo)
            res.last().texto += t;
        else
            res.append({t, codigo});
    };
    int i = 0;
    while (i < texto.size()) {
        const int abre = texto.indexOf(QLatin1Char('`'), i);
        if (abre < 0)
            break;
        // Código si se cierra en la misma línea y tiene algo dentro; si no, la comilla es un carácter más.
        const int cierra = texto.indexOf(QLatin1Char('`'), abre + 1);
        const int finLinea = texto.indexOf(QLatin1Char('\n'), abre + 1);
        if (cierra < 0 || (finLinea >= 0 && finLinea < cierra)) {
            anadir(texto.mid(i, abre + 1 - i), false);
            i = abre + 1;
            continue;
        }
        if (cierra == abre + 1) {   // dos seguidas, sin nada dentro: se quedan las dos
            anadir(texto.mid(i, cierra + 1 - i), false);
            i = cierra + 1;
            continue;
        }
        anadir(texto.mid(i, abre - i), false);
        anadir(texto.mid(abre + 1, cierra - abre - 1), true);
        i = cierra + 1;
    }
    anadir(texto.mid(i), false);
    return res;
}

QString sinMarcas(const QString &texto)
{
    QString res;
    for (const Trozo &t : trozos(texto))
        res += t.texto;
    return res;
}

bool comillaSinPareja(const QString &texto)
{
    for (const Trozo &t : trozos(texto))
        if (!t.codigo && t.texto.contains(QLatin1Char('`')))
            return true;
    return false;
}

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
    bool sinPareja = false;
    for (const Bloque &b : bloques(texto)) {
        if (b.tipo == Bloque::Parrafo)
            sinPareja = sinPareja || comillaSinPareja(b.lineas.join(QLatin1Char('\n')));
        for (const QStringList &fila : b.filas)
            for (const QString &celda : fila)
                sinPareja = sinPareja || comillaSinPareja(celda);
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
    if (sinPareja)
        //: «`» es la comilla invertida (acento grave) que marca el código dentro de una línea de texto
        avisos << tr("Hay una comilla invertida (`) sin pareja en su línea: saldrá tal cual. El código dentro del texto va "
                     "entre dos, `así`.");
    return avisos;
}

} // namespace Enunciado
