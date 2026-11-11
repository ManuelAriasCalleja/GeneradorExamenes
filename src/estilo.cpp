#include "estilo.h"
#include "enunciado.h"

#include <QCoreApplication>
#include <QRegularExpression>

namespace Estilo {

namespace {

const QRegularExpression::PatternOptions kUnicode = QRegularExpression::UseUnicodePropertiesOption;

// Formas de la referencia al libro que puede pedir un tema (Tema::formatoReferencia), y cómo se escriben.
const QString kVease = QStringLiteral("vease");
const QString kPag = QStringLiteral("pag");
const QString kFormatoVease = QStringLiteral("«Véase la página N (sección X.Y) del libro de teoría.»");
const QString kFormatoPag = QStringLiteral("«Pág. N libro (subsección X.Y.Z).»");

// Erratas que ya se corrigieron en el banco y no hay que reintroducir.
const QList<QPair<QString, QString>> kErratas{{QStringLiteral("weareable"), QStringLiteral("wearable")}};

// Líneas de texto normal del enunciado (sin los bloques de código ni las filas de tabla) y líneas de código.
struct Partes
{
    QStringList texto;
    QStringList codigo;
};

Partes partesDe(const QString &enunciado)
{
    Partes partes;
    for (const Enunciado::Bloque &b : Enunciado::bloques(enunciado)) {
        if (b.tipo == Enunciado::Bloque::Codigo) {
            partes.codigo << b.lineas;
        } else if (b.tipo == Enunciado::Bloque::Parrafo) {
            for (const QString &linea : b.lineas)
                if (!linea.trimmed().isEmpty())
                    partes.texto << linea.trimmed();
        }
    }
    return partes;
}

// Texto que parece código o una fórmula (HTML, C, hoja de cálculo): en él las comillas rectas son correctas.
bool pareceCodigo(const QString &s)
{
    static const QRegularExpression re(QStringLiteral("[<>=(){};]"));
    return re.match(s).hasMatch();
}

// Empieza por minúscula una palabra normal. No cuentan una sola palabra (un comando, un nombre de celda, un
// dominio...), una letra suelta (una variable), ni una palabra con símbolos, mayúsculas dentro (iOS) o sin
// vocales (www).
bool empiezaPorMinuscula(const QString &s)
{
    if (!s.contains(QLatin1Char(' ')))
        return false;
    const QString primera = s.section(QLatin1Char(' '), 0, 0);
    if (primera.size() < 2)
        return false;
    for (const QChar c : primera)
        if (!c.isLetter() || !c.isLower())
            return false;
    static const QRegularExpression vocal(QStringLiteral("[aeiouáéíóúü]"));
    return vocal.match(primera).hasMatch();
}

// Sin las URL ni lo que va entre comillas, donde «?» no abre ni cierra una pregunta.
QString sinUrlsNiCitas(QString s)
{
    static const QRegularExpression urls(QStringLiteral("\\S+://\\S+|www\\.\\S+"));
    static const QRegularExpression citas(QStringLiteral("“[^”]*”|\"[^\"]*\""));
    s.remove(urls);
    s.remove(citas);
    return s;
}

void revisarEnunciado(const Pregunta &p, bool castellano, QVector<Aviso> &avisos)
{
    auto avisar = [&](const QString &texto) { avisos.append({Aviso::Enunciado, texto}); };
    const Partes partes = partesDe(p.enunciado);
    const QString texto = partes.texto.join(QLatin1Char(' '));

    if (castellano && !partes.texto.isEmpty()) {
        const QString ultima = partes.texto.last();
        if (ultima.endsWith(QLatin1Char('?')))
            avisar(QCoreApplication::translate("Estilo", "Las preguntas acaban en «?:», con los dos puntos detrás."));
        else if (ultima.endsWith(QLatin1Char('.')))
            avisar(QCoreApplication::translate("Estilo", "El enunciado debe acabar en dos puntos, no en punto."));
        else if (!ultima.endsWith(QLatin1Char(':')))
            avisar(QCoreApplication::translate("Estilo", "El enunciado debe acabar en dos puntos (o en «?:» si es una pregunta)."));
    }
    const QString sinCitas = sinUrlsNiCitas(texto);
    if (!castellano)
        ;   // ¿? solo en castellano
    else if (sinCitas.contains(QLatin1Char('?')) && !sinCitas.contains(QStringLiteral("¿")))
        avisar(QCoreApplication::translate("Estilo", "Falta el «¿» de apertura de la pregunta."));
    else if (sinCitas.contains(QStringLiteral("¿")) && !sinCitas.contains(QLatin1Char('?')))
        avisar(QCoreApplication::translate("Estilo", "Falta el «?» de cierre de la pregunta."));
    if (texto.size() > 180)
        avisar(QCoreApplication::translate("Estilo", "El enunciado es largo (%1 caracteres); conviene que no pase de unos 180.").arg(texto.size()));
    if (empiezaPorMinuscula(texto))
        avisar(QCoreApplication::translate("Estilo", "El enunciado debe empezar por mayúscula."));

    static const QRegularExpression falso(QStringLiteral("\\bes falso\\b"), kUnicode);
    static const QRegularExpression noEnPregunta(QStringLiteral("¿[^?]*\\b(?i:cuál|cuáles|qué)\\b[^?]*\\bno\\b"), kUnicode);
    static const QRegularExpression noAlPrincipio(QStringLiteral("^No es\\b"), kUnicode);
    static const QRegularExpression noMayusculas(QStringLiteral("\\bNO\\b"), kUnicode);
    if (castellano && (falso.match(texto).hasMatch() || noAlPrincipio.match(texto).hasMatch())
        || (castellano && noEnPregunta.match(texto).hasMatch() && !noMayusculas.match(texto).hasMatch()))
        //: «NO» y «FALSO» son las palabras de los enunciados en castellano que la regla quiere en mayúsculas.
        avisar(QCoreApplication::translate("Estilo", "La negación va en mayúsculas: «NO», «FALSO»."));

    if (castellano && texto.contains(QLatin1Char('"')) && !pareceCodigo(texto))
        avisar(QCoreApplication::translate("Estilo", "En el texto, los literales van entre comillas tipográficas (“…”), no rectas."));
    static const QRegularExpression tipograficas(QStringLiteral("[“”‘’«»]"));
    if (tipograficas.match(partes.codigo.join(QLatin1Char('\n'))).hasMatch())
        avisar(QCoreApplication::translate("Estilo", "Dentro del código, las comillas deben ser rectas (\")."));
    for (const QString &aviso : Enunciado::avisosDelCodigo(p.enunciado))
        avisar(aviso);
}

void revisarOpciones(const Pregunta &p, bool castellano, QVector<Aviso> &avisos)
{
    auto avisar = [&](const QString &texto) { avisos.append({Aviso::Opciones, texto}); };
    for (int i = 0; i < 4; ++i) {
        const QString o = p.opciones.value(i).trimmed();
        const QChar letra = Pregunta::letra(i);
        if (o.isEmpty())
            continue;   // los campos vacíos los rechaza el editor
        if (castellano && !o.endsWith(QLatin1Char('.')))
            avisar(QCoreApplication::translate("Estilo", "La opción %1) debe terminar en punto.").arg(letra));
        if (empiezaPorMinuscula(o))
            avisar(QCoreApplication::translate("Estilo", "La opción %1) debe empezar por mayúscula.").arg(letra));
        if (opcionDependiente(o))
            //: %1 es la letra de la opción (a, b, c o d). «Todas las anteriores» es un ejemplo de opción de las que busca la regla.
            avisar(QCoreApplication::translate("Estilo", "La opción %1) remite a las demás («Todas las anteriores»…): conviene evitarlo, porque "
                                                         "impide barajar las opciones.").arg(letra));
        if (castellano && o.contains(QLatin1Char('"')) && !pareceCodigo(o))
            avisar(QCoreApplication::translate("Estilo", "En la opción %1), los literales van entre comillas tipográficas (“…”), no rectas.").arg(letra));
        for (int j = i + 1; j < 4; ++j)
            if (o.compare(p.opciones.value(j).trimmed(), Qt::CaseInsensitive) == 0)
                avisar(QCoreApplication::translate("Estilo", "Las opciones %1) y %2) son iguales.").arg(letra).arg(Pregunta::letra(j)));
    }

    // La correcta no debe destacar por ser mucho más larga que las demás.
    const int larga = p.opciones.value(p.correcta).trimmed().size();
    int otras = 0;
    for (int i = 0; i < 4; ++i)
        if (i != p.correcta)
            otras = qMax(otras, int(p.opciones.value(i).trimmed().size()));
    if (larga > otras * 3 / 2 && larga - otras >= 15)
        avisar(QCoreApplication::translate("Estilo", "La correcta es bastante más larga que las demás (%1 caracteres frente a %2 como mucho): "
                                                     "puede delatarla.").arg(larga).arg(otras));
}

void revisarReferencia(const Pregunta &p, const QString &formato, bool castellano, QVector<Aviso> &avisos)
{
    auto avisar = [&](const QString &texto) { avisos.append({Aviso::Referencia, texto}); };
    const QString r = p.referencia.trimmed();
    if (r.isEmpty()) {
        avisar(QCoreApplication::translate("Estilo", "Falta la referencia al libro."));
        return;
    }
    if (!castellano)
        return;   // lo demás son las formas de la referencia en castellano
    const int antes = avisos.size();
    static const QRegularExpression pagSinPunto(QStringLiteral("^Págs? "), kUnicode);
    static const QRegularExpression sinNumero(QStringLiteral("^(Véase la página|Véanse las páginas|Págs?\\.?) +(?!\\d)"), kUnicode);
    static const QRegularExpression concordancia(
        QStringLiteral("^Véase las\\b|^Véase la páginas\\b|^Véanse la\\b|^Véanse las página\\b|^Véase la página \\d+ *(y|-|–) *\\d+"), kUnicode);
    if (pagSinPunto.match(r).hasMatch())
        //: «Pág.» es la abreviatura de las referencias al libro en castellano que busca la regla.
        avisar(QCoreApplication::translate("Estilo", "«Pág.» se escribe con punto."));
    if (!r.endsWith(QLatin1Char('.')))
        avisar(QCoreApplication::translate("Estilo", "La referencia debe terminar en punto."));
    if (sinNumero.match(r).hasMatch())
        avisar(QCoreApplication::translate("Estilo", "Falta el número de página."));
    if (concordancia.match(r).hasMatch())
        //: Las formas entre «» son las de las referencias al libro en castellano que exige la regla.
        avisar(QCoreApplication::translate("Estilo", "Falta concordancia: «Véase la página N» o «Véanse las páginas N y M»."));

    const bool vease = r.startsWith(QStringLiteral("Véase")) || r.startsWith(QStringLiteral("Véanse"));
    const bool pag = r.startsWith(QStringLiteral("Pág"));
    if (formato == kVease && !vease)
        //: %2 es la forma de la referencia al libro, en castellano y entre «»; no se traduce.
        avisar(QCoreApplication::translate("Estilo", "En el tema %1 la referencia tiene la forma %2").arg(p.temaNumero).arg(kFormatoVease));
    else if (formato == kPag && !pag)
        avisar(QCoreApplication::translate("Estilo", "En el tema %1 la referencia tiene la forma %2").arg(p.temaNumero).arg(kFormatoPag));

    // Si no se ha visto nada concreto, el formato completo: «sección» o «subsección», y una página («y
    // siguientes») o varias («N-M», «N y M», «N, M y K»).
    if (avisos.size() == antes) {
        static const QRegularExpression completoVease(
            QStringLiteral("^Véase la página \\d+( y siguientes)?( \\((sub)?secci[oó]n [^)]+\\))? del libro de teoría\\.$"
                           "|^Véanse las páginas \\d+(-\\d+|(, \\d+)* y \\d+)( \\((sub)?secci[oó]n(es)? [^)]+\\))? del libro de teoría\\.$"),
            kUnicode);
        static const QRegularExpression completoPag(
            QStringLiteral("^Pág\\. \\d+ libro \\((sub)?secci[oó]n [^)]+\\)\\.$"
                           "|^Págs\\. \\d+(-\\d+|(, \\d+)* y \\d+) libro \\((sub)?secci[oó]n(es)? [^)]+\\)\\.$"),
            kUnicode);
        if (vease && !completoVease.match(r).hasMatch())
            //: %1 es la forma de la referencia al libro, en castellano y entre «»; no se traduce.
            avisar(QCoreApplication::translate("Estilo", "La referencia no sigue la forma habitual: %1").arg(kFormatoVease));
        else if (pag && !completoPag.match(r).hasMatch())
            avisar(QCoreApplication::translate("Estilo", "La referencia no sigue la forma habitual: %1").arg(kFormatoPag));
    }
}

} // namespace

QString nombreParte(Aviso::Parte parte)
{
    switch (parte) {
    //: Parte de una pregunta: su enunciado.
    case Aviso::Enunciado: return QCoreApplication::translate("Estilo", "Enunciado");
    //: Parte de una pregunta: sus cuatro opciones de respuesta.
    case Aviso::Opciones: return QCoreApplication::translate("Estilo", "Opciones");
    //: Parte de una pregunta: la referencia al libro de teoría.
    case Aviso::Referencia: return QCoreApplication::translate("Estilo", "Referencia");
    }
    return QString();
}

QVector<QPair<QString, QString>> formatosReferencia()
{
    //: Forma de la referencia al libro que se pide a un tema: cualquiera (no se pide ninguna).
    return {{QString(), QCoreApplication::translate("Estilo", "Cualquiera")},
            {kVease, QStringLiteral("Véase la página N (sección X.Y) del libro de teoría.")},
            {kPag, QStringLiteral("Pág. N libro (subsección X.Y.Z).")}};
}

QVector<Aviso> revisar(const Pregunta &p, const QVector<Tema> &temas, const QString &idioma)
{
    const bool castellano = idioma.isEmpty() || idioma == QLatin1String("es");
    QString formato;
    for (const Tema &t : temas)
        if (t.numero == p.temaNumero)
            formato = t.formatoReferencia;
    QVector<Aviso> avisos;
    revisarEnunciado(p, castellano, avisos);
    revisarOpciones(p, castellano, avisos);
    revisarReferencia(p, formato, castellano, avisos);
    for (const auto &errata : kErratas) {
        if (p.enunciado.contains(errata.first, Qt::CaseInsensitive))
            //: %1 es una palabra mal escrita y %2, cómo se escribe.
            avisos.append({Aviso::Enunciado, QCoreApplication::translate("Estilo", "«%1» está mal escrito: es «%2».").arg(errata.first, errata.second)});
        if (p.opciones.join(QLatin1Char(' ')).contains(errata.first, Qt::CaseInsensitive))
            avisos.append({Aviso::Opciones, QCoreApplication::translate("Estilo", "«%1» está mal escrito: es «%2».").arg(errata.first, errata.second)});
    }
    return avisos;
}

bool opcionDependiente(const QString &opcion)
{
    static const QRegularExpression re(
        QStringLiteral("\\b(todos|todas|ninguno|ninguna|ambos|ambas|los dos|las dos)( de)?( los| las)?( opciones| respuestas)? anteriores\\b"
                       "|\\bcualquiera? de (los|las) (\\w+ )?anteriores\\b"
                       "|\\b(todos|todas|ninguno|ninguna|ambos|ambas) (son|es|están|está) (correct|ciert|verdader|fals)[oa]s?\\b"
                       "|^ *(ambos|ambas|los dos|las dos|todos|todas|ninguno|ninguna) *\\.? *$"
                       "|^ *[a-d]\\)? y [a-d]\\)?( (son|es|están) (correct|ciert|verdader|fals)[oa]s?)? *\\.? *$"
                       "|\\b(opciones|respuestas) [a-d]\\)? y [a-d]\\b"),
        QRegularExpression::CaseInsensitiveOption | kUnicode);
    static const QRegularExpression ingles(
        QStringLiteral("\\b(all|none|both|neither|any|each|either) of (the|these)( \\w+)? (above|previous|preceding|options|answers)\\b"
                       "|\\b(all|none|both) (of )?the (above|previous)\\b"
                       "|\\b(all|none|both) (of them )?(are|is) (correct|true|false)\\b"
                       "|^ *(both|all|none|neither)( of them)? *\\.? *$"
                       "|^ *[a-d]\\)? and [a-d]\\)?( (are|is) (correct|true|false))? *\\.? *$"
                       "|\\b(options|answers) [a-d]\\)? and [a-d]\\b"),
        QRegularExpression::CaseInsensitiveOption | kUnicode);
    return re.match(opcion).hasMatch() || ingles.match(opcion).hasMatch();
}

bool tieneOpcionesDependientes(const Pregunta &p)
{
    for (const QString &o : p.opciones)
        if (opcionDependiente(o))
            return true;
    return false;
}

} // namespace Estilo
