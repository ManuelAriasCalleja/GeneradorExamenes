#include "resaltado.h"

#include <QHash>
#include <QSet>

namespace Resaltado {

namespace {

// Reglas de un lenguaje «de programación» (todos menos los de etiquetas): sus palabras clave, sus comentarios y sus
// cadenas. Son las de las definiciones de listings, simplificadas.
struct Reglas
{
    QSet<QString> claves;
    bool distingueMayusculas = true;
    QStringList comentarioLinea;   // «//», «#», «--»
    QString abreComentario;        // «/*»; vacío si no tiene comentarios de bloque
    QString cierraComentario;      // «*/»
    QString comillas;              // los caracteres que abren (y cierran) una cadena
    QString letrasExtra;           // otros caracteres que forman parte de las palabras
};

QSet<QString> palabras(const char *lista)
{
    QSet<QString> res;
    for (const QString &p : QString::fromLatin1(lista).split(QLatin1Char(' '), Qt::SkipEmptyParts))
        res.insert(p);
    return res;
}

const QHash<QString, Reglas> &reglas()
{
    static const QHash<QString, Reglas> r = [] {
        QHash<QString, Reglas> h;
        h.insert(QStringLiteral("c"),
                 {palabras("auto break case char const continue default do double else enum extern float for goto if "
                           "int long register return short signed sizeof static struct switch typedef union unsigned "
                           "void volatile while"),
                  true, {QStringLiteral("//")}, QStringLiteral("/*"), QStringLiteral("*/"), QStringLiteral("\"'"), {}});
        h.insert(QStringLiteral("idl"),   // el IDL de CORBA
                 {palabras("any attribute boolean case char const context default double enum exception fixed float in "
                           "inout interface long module native Object octet oneway out raises readonly sequence short "
                           "string struct switch typedef union unsigned void wchar wstring FALSE TRUE"),
                  true, {QStringLiteral("//")}, QStringLiteral("/*"), QStringLiteral("*/"), QStringLiteral("\""), {}});
        h.insert(QStringLiteral("java"),
                 {palabras("abstract boolean break byte case catch char class const continue default do double else "
                           "enum extends false final finally float for goto if implements import instanceof int "
                           "interface long native new null package private protected public return short static super "
                           "switch synchronized this throw throws transient true try void volatile while"),
                  true, {QStringLiteral("//")}, QStringLiteral("/*"), QStringLiteral("*/"), QStringLiteral("\"'"), {}});
        h.insert(QStringLiteral("python"),
                 {palabras("and as assert async await break class continue def del elif else except False finally for "
                           "from global if import in is lambda None nonlocal not or pass raise return True try while "
                           "with yield"),
                  true, {QStringLiteral("#")}, {}, {}, QStringLiteral("\"'"), {}});
        h.insert(QStringLiteral("sql"),
                 {palabras("add all alter and as asc avg between by char check column constraint count create database "
                           "date default delete desc distinct drop exists foreign from full group having in index inner "
                           "insert int integer into is join key left like limit max min not null on or order outer "
                           "primary references right select set sum table union unique update values varchar view where"),
                  false, {QStringLiteral("--")}, QStringLiteral("/*"), QStringLiteral("*/"), QStringLiteral("'\""), {}});
        h.insert(QStringLiteral("json"), {palabras("true false null"), true, {}, {}, {}, QStringLiteral("\""), {}});
        h.insert(QStringLiteral("yaml"),
                 {palabras("true false null yes no"), false, {QStringLiteral("#")}, {}, {}, QStringLiteral("\"'"), {}});
        return h;
    }();
    return r;
}

// Palabras clave dentro de las etiquetas en XML (y en las DTD, que se resaltan como XML): las de listings, que son las
// de las declaraciones de una DTD («<!ATTLIST cita hora CDATA #REQUIRED>») y las de «<?xml version="1.0"?>».
const QSet<QString> &clavesXml()
{
    static const QSet<QString> c = palabras("CDATA DOCTYPE ATTLIST termdef ELEMENT EMPTY ANY ID IDREF IDREFS ENTITY "
                                            "ENTITIES NMTOKEN NMTOKENS NOTATION INCLUDE IGNORE SYSTEM PUBLIC NDATA PCDATA "
                                            "REQUIRED IMPLIED FIXED xml xml:space xml:lang version standalone default "
                                            "preserve");
    return c;
}

// Va juntando los trozos de una línea, uniendo los seguidos del mismo estilo.
struct Linea
{
    QVector<Trozo> trozos;
    void poner(const QString &texto, Estilo estilo)
    {
        if (texto.isEmpty())
            return;
        if (!trozos.isEmpty() && trozos.last().estilo == estilo)
            trozos.last().texto += texto;
        else
            trozos.append({texto, estilo});
    }
};

bool esLetra(QChar c, const QString &extra)
{
    return c.isLetterOrNumber() || c == QLatin1Char('_') || extra.contains(c);
}

// Lenguajes de programación: palabras clave, comentarios (los de bloque pueden seguir en las líneas siguientes) y
// cadenas (no pasan de una línea; «\» escapa el carácter siguiente).
QVector<QVector<Trozo>> resaltarProgramacion(const QStringList &lineas, const Reglas &r)
{
    QVector<QVector<Trozo>> res;
    bool enComentario = false;
    for (const QString &texto : lineas) {
        Linea linea;
        qsizetype i = 0;
        const qsizetype n = texto.size();
        while (i < n) {
            if (enComentario) {
                const qsizetype fin = texto.indexOf(r.cierraComentario, i);
                const qsizetype hasta = fin < 0 ? n : fin + r.cierraComentario.size();
                linea.poner(texto.mid(i, hasta - i), Comentario);
                i = hasta;
                enComentario = fin < 0;
                continue;
            }
            if (!r.abreComentario.isEmpty() && texto.mid(i).startsWith(r.abreComentario)) {
                enComentario = true;
                linea.poner(r.abreComentario, Comentario);
                i += r.abreComentario.size();
                continue;
            }
            bool comentario = false;
            for (const QString &marca : r.comentarioLinea)
                if (texto.mid(i).startsWith(marca)) {
                    linea.poner(texto.mid(i), Comentario);
                    i = n;
                    comentario = true;
                    break;
                }
            if (comentario)
                break;
            const QChar c = texto.at(i);
            if (r.comillas.contains(c)) {
                qsizetype j = i + 1;
                while (j < n && texto.at(j) != c)
                    j += texto.at(j) == QLatin1Char('\\') ? 2 : 1;
                j = qMin(j + 1, n);
                linea.poner(texto.mid(i, j - i), Cadena);
                i = j;
            } else if (esLetra(c, r.letrasExtra)) {
                qsizetype j = i;
                while (j < n && esLetra(texto.at(j), r.letrasExtra))
                    j++;
                const QString palabra = texto.mid(i, j - i);
                const bool clave = r.claves.contains(r.distingueMayusculas ? palabra : palabra.toLower());
                linea.poner(palabra, clave ? Clave : Normal);
                i = j;
            } else {
                linea.poner(QString(c), Normal);
                i++;
            }
        }
        res.append(linea.trozos);
    }
    return res;
}

// Lenguajes de etiquetas (XML, HTML y DTD): el nombre de cada etiqueta o declaración en negrita, los valores de los
// atributos como cadenas y los comentarios «<!-- … -->»; las etiquetas y los comentarios pueden ocupar varias líneas.
// Dentro de las etiquetas, también las palabras de 'clavesEnEtiqueta' (en XML, las de una DTD: PCDATA, CDATA, ID…).
QVector<QVector<Trozo>> resaltarEtiquetas(const QStringList &lineas, const QSet<QString> &clavesEnEtiqueta)
{
    QVector<QVector<Trozo>> res;
    bool enComentario = false;
    bool enEtiqueta = false;
    const QString extra = QStringLiteral("-.:");
    for (const QString &texto : lineas) {
        Linea linea;
        qsizetype i = 0;
        const qsizetype n = texto.size();
        while (i < n) {
            if (enComentario) {
                const qsizetype fin = texto.indexOf(QLatin1String("-->"), i);
                const qsizetype hasta = fin < 0 ? n : fin + 3;
                linea.poner(texto.mid(i, hasta - i), Comentario);
                i = hasta;
                enComentario = fin < 0;
                continue;
            }
            if (texto.mid(i).startsWith(QLatin1String("<!--"))) {
                enComentario = true;
                linea.poner(QStringLiteral("<!--"), Comentario);
                i += 4;
                continue;
            }
            const QChar c = texto.at(i);
            if (c == QLatin1Char('<')) {
                // «<», «</», «<?» o «<!» y el nombre.
                qsizetype j = i + 1;
                while (j < n && (texto.at(j) == QLatin1Char('/') || texto.at(j) == QLatin1Char('?') || texto.at(j) == QLatin1Char('!')))
                    j++;
                linea.poner(texto.mid(i, j - i), Normal);
                qsizetype k = j;
                while (k < n && esLetra(texto.at(k), extra))
                    k++;
                linea.poner(texto.mid(j, k - j), Clave);
                enEtiqueta = true;
                i = k;
            } else if (enEtiqueta && c == QLatin1Char('>')) {
                linea.poner(QStringLiteral(">"), Normal);
                enEtiqueta = false;
                i++;
            } else if (enEtiqueta && (c == QLatin1Char('"') || c == QLatin1Char('\''))) {
                qsizetype j = texto.indexOf(c, i + 1);
                j = j < 0 ? n : j + 1;
                linea.poner(texto.mid(i, j - i), Cadena);
                i = j;
            } else if (enEtiqueta && esLetra(c, extra)) {
                qsizetype j = i;
                while (j < n && esLetra(texto.at(j), extra))
                    j++;
                const QString palabra = texto.mid(i, j - i);
                linea.poner(palabra, clavesEnEtiqueta.contains(palabra) ? Clave : Normal);
                i = j;
            } else {
                linea.poner(QString(c), Normal);
                i++;
            }
        }
        res.append(linea.trozos);
    }
    return res;
}

} // namespace

QStringList lenguajes()
{
    return {QStringLiteral("c"),    QStringLiteral("dtd"),    QStringLiteral("html"),   QStringLiteral("idl"),
            QStringLiteral("java"), QStringLiteral("json"),   QStringLiteral("python"),
            QStringLiteral("sql"),  QStringLiteral("xml"),    QStringLiteral("yaml")};
}

QStringList alias()
{
    return {QStringLiteral("xsd"), QStringLiteral("xsl"), QStringLiteral("xslt"), QStringLiteral("wsdl"),
            QStringLiteral("soap"), QStringLiteral("yml")};
}

QString lenguaje(const QString &nombre)
{
    const QString n = nombre.toLower();
    if (lenguajes().contains(n))
        return n;
    if (n == QLatin1String("yml"))
        return QStringLiteral("yaml");
    if (alias().contains(n))
        return QStringLiteral("xml");
    return QString();
}

bool conocido(const QString &nombre)
{
    return !lenguaje(nombre).isEmpty() || nombre.compare(QLatin1String("texto"), Qt::CaseInsensitive) == 0;
}

QString enListings(const QString &lenguaje)
{
    static const QHash<QString, QString> nombres{
        {QStringLiteral("c"), QStringLiteral("C")},        {QStringLiteral("dtd"), QStringLiteral("XML")},
        {QStringLiteral("html"), QStringLiteral("HTML")},  {QStringLiteral("idl"), QStringLiteral("[CORBA]IDL")},
        {QStringLiteral("java"), QStringLiteral("Java")},
        {QStringLiteral("json"), QStringLiteral("JSON")},  {QStringLiteral("python"), QStringLiteral("Python")},
        {QStringLiteral("sql"), QStringLiteral("SQL")},    {QStringLiteral("xml"), QStringLiteral("XML")},
        {QStringLiteral("yaml"), QStringLiteral("YAML")}};
    return nombres.value(lenguaje);
}

QVector<QVector<Trozo>> resaltar(const QStringList &lineas, const QString &nombre)
{
    const QString l = lenguaje(nombre);
    if (l == QLatin1String("xml") || l == QLatin1String("dtd"))
        return resaltarEtiquetas(lineas, clavesXml());
    if (l == QLatin1String("html"))
        return resaltarEtiquetas(lineas, {});
    if (reglas().contains(l))
        return resaltarProgramacion(lineas, reglas().value(l));
    QVector<QVector<Trozo>> res;
    for (const QString &linea : lineas)
        res.append(linea.isEmpty() ? QVector<Trozo>() : QVector<Trozo>{Trozo{linea, Normal}});
    return res;
}

QStringList html(const QStringList &lineas, const QString &lenguaje)
{
    QStringList res;
    for (const QVector<Trozo> &trozos : resaltar(lineas, lenguaje)) {
        QString linea;
        for (const Trozo &t : trozos) {
            const QString texto = t.texto.toHtmlEscaped();
            switch (t.estilo) {
            case Normal:
                linea += texto;
                break;
            case Clave:
                linea += QStringLiteral("<span style=\"font-weight:bold; color:#%1\">%2</span>").arg(QLatin1String(kColorClave), texto);
                break;
            case Comentario:
                linea += QStringLiteral("<span style=\"font-style:italic; color:#%1\">%2</span>").arg(QLatin1String(kColorComentario), texto);
                break;
            case Cadena:
                linea += QStringLiteral("<span style=\"color:#%1\">%2</span>").arg(QLatin1String(kColorCadena), texto);
                break;
            }
        }
        res << linea;
    }
    return res;
}

} // namespace Resaltado
