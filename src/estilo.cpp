#include "estilo.h"
#include "enunciado.h"

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

void revisarEnunciado(const Pregunta &p, QVector<Aviso> &avisos)
{
    auto avisar = [&](const QString &texto) { avisos.append({Aviso::Enunciado, texto}); };
    const Partes partes = partesDe(p.enunciado);
    const QString texto = partes.texto.join(QLatin1Char(' '));

    if (!partes.texto.isEmpty()) {
        const QString ultima = partes.texto.last();
        if (ultima.endsWith(QLatin1Char('?')))
            avisar(QStringLiteral("Las preguntas acaban en «?:», con los dos puntos detrás."));
        else if (ultima.endsWith(QLatin1Char('.')))
            avisar(QStringLiteral("El enunciado debe acabar en dos puntos, no en punto."));
        else if (!ultima.endsWith(QLatin1Char(':')))
            avisar(QStringLiteral("El enunciado debe acabar en dos puntos (o en «?:» si es una pregunta)."));
    }
    const QString sinCitas = sinUrlsNiCitas(texto);
    if (sinCitas.contains(QLatin1Char('?')) && !sinCitas.contains(QStringLiteral("¿")))
        avisar(QStringLiteral("Falta el «¿» de apertura de la pregunta."));
    else if (sinCitas.contains(QStringLiteral("¿")) && !sinCitas.contains(QLatin1Char('?')))
        avisar(QStringLiteral("Falta el «?» de cierre de la pregunta."));
    if (texto.size() > 180)
        avisar(QStringLiteral("El enunciado es largo (%1 caracteres); conviene que no pase de unos 180.").arg(texto.size()));
    if (empiezaPorMinuscula(texto))
        avisar(QStringLiteral("El enunciado debe empezar por mayúscula."));

    static const QRegularExpression falso(QStringLiteral("\\bes falso\\b"), kUnicode);
    static const QRegularExpression noEnPregunta(QStringLiteral("¿[^?]*\\b(?i:cuál|cuáles|qué)\\b[^?]*\\bno\\b"), kUnicode);
    static const QRegularExpression noAlPrincipio(QStringLiteral("^No es\\b"), kUnicode);
    static const QRegularExpression noMayusculas(QStringLiteral("\\bNO\\b"), kUnicode);
    if (falso.match(texto).hasMatch() || noAlPrincipio.match(texto).hasMatch()
        || (noEnPregunta.match(texto).hasMatch() && !noMayusculas.match(texto).hasMatch()))
        avisar(QStringLiteral("La negación va en mayúsculas: «NO», «FALSO»."));

    if (texto.contains(QLatin1Char('"')) && !pareceCodigo(texto))
        avisar(QStringLiteral("En el texto, los literales van entre comillas tipográficas (“…”), no rectas."));
    static const QRegularExpression tipograficas(QStringLiteral("[“”‘’«»]"));
    if (tipograficas.match(partes.codigo.join(QLatin1Char('\n'))).hasMatch())
        avisar(QStringLiteral("Dentro del código, las comillas deben ser rectas (\")."));
}

void revisarOpciones(const Pregunta &p, QVector<Aviso> &avisos)
{
    auto avisar = [&](const QString &texto) { avisos.append({Aviso::Opciones, texto}); };
    for (int i = 0; i < 4; ++i) {
        const QString o = p.opciones.value(i).trimmed();
        const QChar letra = Pregunta::letra(i);
        if (o.isEmpty())
            continue;   // los campos vacíos los rechaza el editor
        if (!o.endsWith(QLatin1Char('.')))
            avisar(QStringLiteral("La opción %1) debe terminar en punto.").arg(letra));
        if (empiezaPorMinuscula(o))
            avisar(QStringLiteral("La opción %1) debe empezar por mayúscula.").arg(letra));
        if (opcionDependiente(o))
            avisar(QStringLiteral("La opción %1) remite a las demás («Todas las anteriores»…): conviene evitarlo, porque "
                                  "impide barajar las opciones.").arg(letra));
        if (o.contains(QLatin1Char('"')) && !pareceCodigo(o))
            avisar(QStringLiteral("En la opción %1), los literales van entre comillas tipográficas (“…”), no rectas.").arg(letra));
        for (int j = i + 1; j < 4; ++j)
            if (o.compare(p.opciones.value(j).trimmed(), Qt::CaseInsensitive) == 0)
                avisar(QStringLiteral("Las opciones %1) y %2) son iguales.").arg(letra).arg(Pregunta::letra(j)));
    }

    // La correcta no debe destacar por ser mucho más larga que las demás.
    const int larga = p.opciones.value(p.correcta).trimmed().size();
    int otras = 0;
    for (int i = 0; i < 4; ++i)
        if (i != p.correcta)
            otras = qMax(otras, int(p.opciones.value(i).trimmed().size()));
    if (larga > otras * 3 / 2 && larga - otras >= 15)
        avisar(QStringLiteral("La correcta es bastante más larga que las demás (%1 caracteres frente a %2 como mucho): "
                              "puede delatarla.").arg(larga).arg(otras));
}

void revisarReferencia(const Pregunta &p, const QString &formato, QVector<Aviso> &avisos)
{
    auto avisar = [&](const QString &texto) { avisos.append({Aviso::Referencia, texto}); };
    const QString r = p.referencia.trimmed();
    if (r.isEmpty()) {
        avisar(QStringLiteral("Falta la referencia al libro."));
        return;
    }
    const int antes = avisos.size();
    static const QRegularExpression pagSinPunto(QStringLiteral("^Págs? "), kUnicode);
    static const QRegularExpression sinNumero(QStringLiteral("^(Véase la página|Véanse las páginas|Págs?\\.?) +(?!\\d)"), kUnicode);
    static const QRegularExpression concordancia(
        QStringLiteral("^Véase las\\b|^Véase la páginas\\b|^Véanse la\\b|^Véanse las página\\b|^Véase la página \\d+ *(y|-|–) *\\d+"), kUnicode);
    if (pagSinPunto.match(r).hasMatch())
        avisar(QStringLiteral("«Pág.» se escribe con punto."));
    if (!r.endsWith(QLatin1Char('.')))
        avisar(QStringLiteral("La referencia debe terminar en punto."));
    if (sinNumero.match(r).hasMatch())
        avisar(QStringLiteral("Falta el número de página."));
    if (concordancia.match(r).hasMatch())
        avisar(QStringLiteral("Falta concordancia: «Véase la página N» o «Véanse las páginas N y M»."));

    const bool vease = r.startsWith(QStringLiteral("Véase")) || r.startsWith(QStringLiteral("Véanse"));
    const bool pag = r.startsWith(QStringLiteral("Pág"));
    if (formato == kVease && !vease)
        avisar(QStringLiteral("En el tema %1 la referencia tiene la forma %2").arg(p.temaNumero).arg(kFormatoVease));
    else if (formato == kPag && !pag)
        avisar(QStringLiteral("En el tema %1 la referencia tiene la forma %2").arg(p.temaNumero).arg(kFormatoPag));

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
            avisar(QStringLiteral("La referencia no sigue la forma habitual: %1").arg(kFormatoVease));
        else if (pag && !completoPag.match(r).hasMatch())
            avisar(QStringLiteral("La referencia no sigue la forma habitual: %1").arg(kFormatoPag));
    }
}

} // namespace

QString nombreParte(Aviso::Parte parte)
{
    switch (parte) {
    case Aviso::Enunciado: return QStringLiteral("Enunciado");
    case Aviso::Opciones: return QStringLiteral("Opciones");
    case Aviso::Referencia: return QStringLiteral("Referencia");
    }
    return QString();
}

QVector<QPair<QString, QString>> formatosReferencia()
{
    return {{QString(), QStringLiteral("Cualquiera")},
            {kVease, QStringLiteral("Véase la página N (sección X.Y) del libro de teoría.")},
            {kPag, QStringLiteral("Pág. N libro (subsección X.Y.Z).")}};
}

QVector<Aviso> revisar(const Pregunta &p, const QVector<Tema> &temas)
{
    QString formato;
    for (const Tema &t : temas)
        if (t.numero == p.temaNumero)
            formato = t.formatoReferencia;
    QVector<Aviso> avisos;
    revisarEnunciado(p, avisos);
    revisarOpciones(p, avisos);
    revisarReferencia(p, formato, avisos);
    for (const auto &errata : kErratas) {
        if (p.enunciado.contains(errata.first, Qt::CaseInsensitive))
            avisos.append({Aviso::Enunciado, QStringLiteral("«%1» está mal escrito: es «%2».").arg(errata.first, errata.second)});
        if (p.opciones.join(QLatin1Char(' ')).contains(errata.first, Qt::CaseInsensitive))
            avisos.append({Aviso::Opciones, QStringLiteral("«%1» está mal escrito: es «%2».").arg(errata.first, errata.second)});
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
    return re.match(opcion).hasMatch();
}

bool tieneOpcionesDependientes(const Pregunta &p)
{
    for (const QString &o : p.opciones)
        if (opcionDependiente(o))
            return true;
    return false;
}

} // namespace Estilo
