// Sondas: casos generados al azar, con semillas fijas para poder repetirlos, contra las partes del programa que leen o
// escriben texto que no controla: el comprobador y la importación de los JSON, la maqueta y las exportaciones (HTML,
// LaTeX, Word, PDF y Moodle), las pautas de estilo, las parecidas, la lectura de los exámenes anteriores, el generador y
// la ida y vuelta base de datos ↔ JSON. Comprueban que no fallan y que cumplen sus reglas. Ver docs/revision_errores.md.
// Los textos son inventados (letras, símbolos y trozos de código y de tabla al azar), como todos los datos de las pruebas.

#include "basedatos.h"
#include "comprobadorjson.h"
#include "datos_prueba.h"
#include "docx.h"
#include "enunciado.h"
#include "estilo.h"
#include "examenesanteriores.h"
#include "exportacion.h"
#include "latex.h"
#include "generador.h"
#include "maquetador.h"
#include "moodle.h"
#include "parecidas.h"

#include <QDir>
#include <QJsonArray>
#include <QProcess>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QSet>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QXmlStreamReader>
#include <QtTest>
#include <memory>

namespace {

// Cuántas veces se repite cada sonda: 'n', o 'n' por el valor de la variable de entorno SONDAS para buscar más a fondo
// (SONDAS=20 ./prueba_sondas); las semillas son las mismas, así que lo que encuentre se puede repetir.
int vueltas(int n)
{
    return n * qMax(1, qEnvironmentVariableIntValue("SONDAS"));
}

// Trozos con los que se construyen los textos: palabras, símbolos que hay que escapar en HTML, XML, LaTeX o JSON,
// Unicode de varios tipos (con pares sustitutos, marcas combinadas, espacios raros y caracteres de control) y las
// marcas de los enunciados (``` y |).
const QStringList kTrozos{
    QStringLiteral("palabra"), QStringLiteral("Otra"), QStringLiteral("acción"), QStringLiteral("año"), QStringLiteral("¿qué?"),
    QStringLiteral("¡sí!"), QStringLiteral(" "), QStringLiteral("  "), QStringLiteral("\t"), QStringLiteral("\n"), QStringLiteral("\r\n"),
    QStringLiteral("&"), QStringLiteral("<"), QStringLiteral(">"), QStringLiteral("\""), QStringLiteral("'"), QStringLiteral("\\"),
    QStringLiteral("{"), QStringLiteral("}"), QStringLiteral("$"), QStringLiteral("#"), QStringLiteral("_"), QStringLiteral("%"),
    QStringLiteral("~"), QStringLiteral("^"), QStringLiteral("|"), QStringLiteral("`"), QStringLiteral("["), QStringLiteral("]"),
    QStringLiteral("--"), QStringLiteral("---"), QStringLiteral("<<"), QStringLiteral(">>"), QStringLiteral("?`"), QStringLiteral("]]>"),
    QStringLiteral("<!--"), QStringLiteral("&amp;"), QStringLiteral("%1"), QStringLiteral("%2"), QStringLiteral("\\n"),
    QStringLiteral("«»"), QStringLiteral("“”"), QStringLiteral("‘’"), QStringLiteral("—"), QStringLiteral("…"), QStringLiteral("·"),
    QStringLiteral("→"), QStringLiteral("≤"), QStringLiteral("≥"), QStringLiteral("≠"), QStringLiteral("µ"), QStringLiteral("π"),
    QStringLiteral("Ω"), QStringLiteral("√"), QStringLiteral("∞"), QStringLiteral("€"), QStringLiteral("✓"), QStringLiteral("✔"),
    QStringLiteral("😀"), QStringLiteral("𝔸"), QStringLiteral("e") + QChar(0x0301), QString(QChar(0x200B)), QString(QChar(0x00A0)),
    QString(QChar(0xFEFF)), QString(QChar(0x202E)), QStringLiteral("\u0001"), QStringLiteral("\u0007"), QStringLiteral("\u001F"),
    QStringLiteral("\u007F"), QStringLiteral("中文"), QStringLiteral("Ελληνικά"), QStringLiteral("```"), QStringLiteral("```java"),
    QStringLiteral("| a | b |"), QStringLiteral("|"), QStringLiteral("1.-"), QStringLiteral("a)"), QStringLiteral("T1-7"),
    QString(200, QLatin1Char('x')),
};

QString textoRaro(QRandomGenerator &rng, int trozos)
{
    QString t;
    for (int i = 0; i < trozos; ++i)
        t += kTrozos.at(rng.bounded(int(kTrozos.size())));
    return t;
}

// Texto con algo que se ve (no solo espacios ni caracteres invisibles o de control).
QString textoNoVacio(QRandomGenerator &rng, int trozos)
{
    const QString t = textoRaro(rng, trozos);
    for (const QChar c : t)
        if (c.isLetterOrNumber() || c.isPunct() || c.isSymbol())
            return t;
    return QStringLiteral("x") + t;
}

// Un enunciado con párrafos, bloques de código (de lenguajes conocidos o no, numerados o no, a veces sin cerrar) y
// tablas (con filas desiguales).
QString enunciadoRaro(QRandomGenerator &rng)
{
    static const QStringList lenguajes{QString(), QStringLiteral("java"), QStringLiteral("c"), QStringLiteral("python"),
                                       QStringLiteral("xml"), QStringLiteral("sql"), QStringLiteral("json"), QStringLiteral("yaml"),
                                       QStringLiteral("idl"), QStringLiteral("cobol"), QStringLiteral("texto")};
    QStringList lineas;
    const int bloques = 1 + rng.bounded(4);
    for (int b = 0; b < bloques; ++b) {
        switch (rng.bounded(4)) {
        case 0:
        case 1:
            lineas << textoNoVacio(rng, 1 + rng.bounded(8));
            break;
        case 2: {
            QString apertura = QStringLiteral("```") + lenguajes.at(rng.bounded(int(lenguajes.size())));
            if (rng.bounded(2))
                apertura += QStringLiteral(" numerado");
            lineas << apertura;
            for (int i = rng.bounded(6); i > 0; --i)
                lineas << textoRaro(rng, rng.bounded(6));
            if (rng.bounded(4))
                lineas << QStringLiteral("```");
            break;
        }
        case 3:
            for (int f = 1 + rng.bounded(3); f > 0; --f) {
                QStringList celdas;
                for (int c = 1 + rng.bounded(4); c > 0; --c)
                    celdas << textoRaro(rng, rng.bounded(3)).remove(QLatin1Char('|')).remove(QLatin1Char('\n'));
                lineas << QStringLiteral("| ") + celdas.join(QStringLiteral(" | ")) + QStringLiteral(" |");
            }
            break;
        }
    }
    const QString t = lineas.join(QLatin1Char('\n'));
    return t.trimmed().isEmpty() ? QStringLiteral("Enunciado:") : t;
}

// Una pregunta al azar, con textos raros, a veces con imagen, verificada, para revisar, anulada…
Pregunta preguntaRara(QRandomGenerator &rng, int tema, int numero, const QString &variante)
{
    Pregunta p;
    p.temaId = tema;
    p.temaNumero = tema;
    p.numero = numero;
    p.variante = variante;
    p.enunciado = enunciadoRaro(rng);
    for (int i = 0; i < 4; ++i)
        p.opciones[i] = textoNoVacio(rng, 1 + rng.bounded(5)).replace(QLatin1Char('\n'), QLatin1Char(' ')).trimmed();
    if (rng.bounded(8) == 0)
        p.opciones[3] = QStringLiteral("Todas las anteriores.");
    p.correcta = rng.bounded(4);
    p.referencia = textoRaro(rng, rng.bounded(4));
    p.explicacion = textoRaro(rng, rng.bounded(6));
    p.notas = textoRaro(rng, rng.bounded(3));
    p.old = rng.bounded(4) == 0;
    p.revisar = rng.bounded(5) == 0;
    p.estado = rng.bounded(10) == 0 ? QStringLiteral("anulada") : rng.bounded(10) == 0 ? QStringLiteral("obsoleta") : QStringLiteral("activa");
    if (rng.bounded(6) == 0)
        p.imagen = imagenInventada();
    p.verificada = rng.bounded(2) == 0;
    return p;
}

// Un banco al azar: 'temas' temas con números del 1 al 'numeros', algunos con variantes, y pseudorrepetidas al azar.
QVector<Pregunta> bancoRaro(QRandomGenerator &rng, int temas, int numeros)
{
    QVector<Pregunta> banco;
    int id = 1;
    for (int t = 1; t <= temas; ++t) {
        for (int n = 1; n <= numeros; ++n) {
            if (rng.bounded(5) == 0)
                continue;   // números que faltan
            const QStringList variantes = rng.bounded(4) == 0 ? QStringList{QStringLiteral("a"), QStringLiteral("b")} : QStringList{QString()};
            for (const QString &v : variantes) {
                Pregunta p = preguntaRara(rng, t, n, v);
                p.id = id++;
                banco << p;
            }
        }
    }
    for (int i = rng.bounded(4); i > 0 && !banco.isEmpty(); --i) {
        Pregunta &p = banco[rng.bounded(int(banco.size()))];
        const Pregunta &otra = banco.at(rng.bounded(int(banco.size())));
        if (otra.temaNumero != p.temaNumero)
            p.pseudoCon = otra.grupo();
    }
    return banco;
}

// Cómo se guarda un texto: sin los U+FEFF del principio (la marca de orden de bytes del UTF-16, que no se ve).
QString comoLoGuarda(QString texto)
{
    while (texto.startsWith(QChar(0xFEFF)))
        texto.remove(0, 1);
    return texto;
}

Pregunta comoLaGuarda(Pregunta p)
{
    for (QString *t : {&p.enunciado, &p.referencia, &p.explicacion, &p.notas, &p.variante, &p.pseudoCon})
        *t = comoLoGuarda(*t);
    for (QString &o : p.opciones)
        o = comoLoGuarda(o);
    for (QString &v : p.verificadores)
        v = comoLoGuarda(v);
    return p;
}

QString describir(const Pregunta &p)
{
    return QStringList{p.codigoCompleto(), p.enunciado, p.opciones.join(QLatin1Char('|')), QString::number(p.correcta),
                       p.referencia, p.estado, QString::number(p.old), QString::number(p.revisar), p.pseudoCon, p.explicacion,
                       p.notas, QString::fromLatin1(p.imagen.toBase64()), QString::number(p.verificada),
                       p.verificadores.join(QLatin1Char('|'))}
        .join(QLatin1Char('#'));
}

// Dónde empiezan a ser distintos dos textos, para el mensaje de una comparación que falla.
QString diferencia(const QString &a, const QString &b)
{
    int i = 0;
    while (i < a.size() && i < b.size() && a.at(i) == b.at(i))
        ++i;
    auto visible = [](QString t) {
        QString r;
        for (const QChar c : t)
            r += c.unicode() < 0x20 || c.unicode() == 0x7F ? QStringLiteral("\\x%1").arg(c.unicode(), 2, 16, QLatin1Char('0')) : QString(c);
        return r;
    };
    return QStringLiteral("en la posición %1: «%2» y «%3»").arg(i).arg(visible(a.mid(qMax(0, i - 15), 40)), visible(b.mid(qMax(0, i - 15), 40)));
}

// Si el XML se lee entero sin errores; si no, el error en 'error'.
bool xmlBienFormado(const QByteArray &xml, QString *error)
{
    QXmlStreamReader r(xml);
    while (!r.atEnd())
        r.readNext();
    if (r.hasError())
        *error = QStringLiteral("línea %1: %2").arg(r.lineNumber()).arg(r.errorString());
    return !r.hasError();
}

// Lo que queda de un texto escapado para LaTeX al quitarle las órdenes con que se escapan los caracteres especiales: no
// puede quedar ninguno de ellos.
QString sinEscapes(QString tex)
{
    for (const char *orden : {"\\textbackslash{}", "\\textasciitilde{}", "\\textasciicircum{}", "\\textquotedbl{}",
                              "\\textquotesingle{}", "\\textasciigrave{}", "\\textless{}", "\\textgreater{}", "\\textbar{}"})
        tex.remove(QLatin1String(orden));
    for (const char *escapado : {"\\{", "\\}", "\\$", "\\&", "\\#", "\\_", "\\%"})
        tex.remove(QLatin1String(escapado));
    return tex.remove(QStringLiteral("{}"));
}

DatosConvocatoria convocatoria()
{
    DatosConvocatoria d;
    d.asignatura = QStringLiteral("Asignatura & {inventada} #1");
    d.convocatoria = QStringLiteral("Junio_%");
    d.anio = 2026;
    d.instrucciones = QStringLiteral("Instrucciones <inventadas> con $ y \\.");
    d.semilla = 77;
    return d;
}

// Un examen con las preguntas dadas, con las opciones en un orden al azar.
Modelo modeloCon(const QVector<Pregunta> &preguntas, QRandomGenerator &rng)
{
    Modelo m;
    m.nombre = QStringLiteral("Modelo <raro> & \"1\"");
    for (const Pregunta &p : preguntas) {
        PreguntaExamen pe;
        pe.pregunta = p;
        std::shuffle(pe.orden.begin(), pe.orden.end(), rng);
        m.preguntas << pe;
    }
    return m;
}

} // namespace

class PruebaSondas : public QObject
{
    Q_OBJECT

private slots:
    // El escape de LaTeX no deja ningún carácter especial suelto, con cualquier texto.
    void escapeLatex()
    {
        QRandomGenerator rng(11);
        for (int i = 0; i < vueltas(2000); ++i) {
            const QString texto = textoRaro(rng, rng.bounded(12));
            const QString resto = sinEscapes(Latex::escapar(texto));
            for (const QChar c : QStringLiteral("\\{}$&#_%~^"))
                QVERIFY2(!resto.contains(c), qPrintable(QStringLiteral("«%1» deja «%2» en «%3»").arg(texto, QString(c), Latex::escapar(texto))));
            QVERIFY2(!Latex::escapar(texto).contains(QStringLiteral("--")),
                     qPrintable(QStringLiteral("«%1»: «--» haría una raya").arg(texto)));
        }
    }

    // Enunciados, opciones y referencias raros: la maqueta (con su paginación), el LaTeX, el Word, el PDF y Moodle no
    // fallan, y el XML del Word y de Moodle está bien formado.
    void maquetaYExportacionesConTextosRaros()
    {
        QRandomGenerator rng(12);
        QTemporaryDir dir;
        const QVector<Tema> temas = temasInventados(3);
        for (int vuelta = 0; vuelta < vueltas(12); ++vuelta) {
            QVector<Pregunta> preguntas;
            for (int i = 0; i < 6; ++i) {
                Pregunta p = preguntaRara(rng, 1 + rng.bounded(3), vuelta * 10 + i + 1, QString());
                p.id = vuelta * 10 + i + 1;
                p.estado = QStringLiteral("activa");
                preguntas << p;
            }
            const Modelo m = modeloCon(preguntas, rng);
            const DatosConvocatoria d = convocatoria();
            for (int nivel : {0, 3, Maquetador::kLetraGrande}) {
                for (bool soluciones : {false, true}) {
                    const QString html = Maquetador::examenHtml(m, d, soluciones, nivel, true, true);
                    const std::unique_ptr<QTextDocument> doc(Maquetador::crearDocumento(html, preguntas, nullptr));
                    doc->setPageSize(QSizeF(650, 900));
                    QVERIFY(doc->pageCount() >= 1);
                    QVERIFY(!Latex::examenes({m}, d, soluciones, QStringLiteral("img"), nivel).isEmpty());
                    const QMap<QString, QByteArray> partes = Docx::partes({m, m}, d, soluciones, nivel);
                    for (auto it = partes.cbegin(); it != partes.cend(); ++it) {
                        QString error;
                        if (!it.key().endsWith(QLatin1String(".png")))
                            QVERIFY2(xmlBienFormado(it.value(), &error), qPrintable(QStringLiteral("vuelta %1, %2: %3").arg(vuelta).arg(it.key(), error)));
                    }
                }
            }
            QVERIFY(Exportacion::pdf(dir.filePath(QStringLiteral("raro.pdf")), {m}, d, true, 0));
            QVERIFY(Exportacion::docx(dir.filePath(QStringLiteral("raro.docx")), {m}, d, true, 0));
            Moodle::Opciones o;
            o.asignatura = QStringLiteral("Asignatura / con <barra> & más");
            QString error;
            QVERIFY2(xmlBienFormado(Moodle::xml(temas, preguntas, o).toUtf8(), &error), qPrintable(QStringLiteral("vuelta %1: %2").arg(vuelta).arg(error)));
            // Las pautas de estilo y la ficha, también.
            for (const Pregunta &p : preguntas) {
                Estilo::revisar(p, temas, QStringLiteral("es"));
                Estilo::revisar(p, temas, QStringLiteral("en"));
                QVERIFY(!Maquetador::fichaPreguntaHtml(p, QStringLiteral("Tema <1>")).isEmpty());
            }
        }
    }

    // El LaTeX con textos raros compila con pdflatex. Tarda y necesita pdflatex: solo con SONDAS_PDFLATEX=1.
    void latexCompilaConTextosRaros()
    {
        const QString pdflatex = QStandardPaths::findExecutable(QStringLiteral("pdflatex"));
        if (pdflatex.isEmpty() || !qEnvironmentVariableIsSet("SONDAS_PDFLATEX"))
            QSKIP("Solo con pdflatex y SONDAS_PDFLATEX=1");
        QRandomGenerator rng(18);
        QTemporaryDir dir;
        QStringList fallos;
        for (int vuelta = 0; vuelta < vueltas(12); ++vuelta) {
            QVector<Pregunta> preguntas;
            for (int i = 0; i < 6; ++i) {
                Pregunta p = preguntaRara(rng, 1 + rng.bounded(3), i + 1, QString());
                p.id = i + 1;
                p.estado = QStringLiteral("activa");
                preguntas << p;
            }
            const Modelo m = modeloCon(preguntas, rng);
            const QString nombre = QStringLiteral("raro%1").arg(vuelta);
            const int nivel = QVector<int>{0, 3, Maquetador::kLetraGrande}.at(vuelta % 3);
            QVERIFY(Exportacion::latex(dir.filePath(nombre + QStringLiteral(".tex")), {m}, convocatoria(), vuelta % 2, nivel));
            QProcess proceso;
            proceso.setWorkingDirectory(dir.path());
            proceso.start(pdflatex, {QStringLiteral("-interaction=nonstopmode"), QStringLiteral("-halt-on-error"), nombre + QStringLiteral(".tex")});
            QVERIFY(proceso.waitForFinished(60000));
            if (proceso.exitCode() != 0) {
                QFile log(dir.filePath(nombre + QStringLiteral(".log")));
                QStringList errores;
                if (log.open(QIODevice::ReadOnly))
                    for (const QByteArray &linea : log.readAll().split('\n'))
                        if (linea.startsWith('!') || linea.startsWith("l."))
                            errores << QString::fromUtf8(linea);
                fallos << QStringLiteral("vuelta %1: %2").arg(vuelta).arg(errores.join(QStringLiteral(" | ")));
            }
        }
        QVERIFY2(fallos.isEmpty(), qPrintable(fallos.join(QLatin1Char('\n'))));
    }

    // Las parecidas y la lectura de los exámenes anteriores, con textos raros.
    void parecidasYExamenesAnteriores()
    {
        QRandomGenerator rng(13);
        const QVector<Pregunta> banco = bancoRaro(rng, 3, 15);
        const Parecidas::Indice indice(banco);
        for (int i = 0; i < vueltas(100); ++i) {
            Pregunta buscada = preguntaRara(rng, 1, 99, QString());
            if (i % 3 == 0)
                buscada = banco.at(rng.bounded(int(banco.size())));
            for (const Parecidas::Parecida &p : indice.masParecidas(buscada, 5, 0.0))
                QVERIFY2(p.parecido >= 0 && p.parecido <= 1.0001, qPrintable(QString::number(p.parecido)));
            QString texto;
            for (int j = rng.bounded(5); j >= 0; --j)
                texto += QStringLiteral("%1.- %2\na) %3\nb) %4\nc) %5\nd) %6\n")
                             .arg(rng.bounded(30))
                             .arg(textoRaro(rng, 4), textoRaro(rng, 2), textoRaro(rng, 2), textoRaro(rng, 2), textoRaro(rng, 2));
            texto += textoRaro(rng, rng.bounded(10));
            for (const ExamenesAnteriores::Leida &l : ExamenesAnteriores::separar(texto))
                QVERIFY(l.opciones.size() <= 4);
            ExamenesAnteriores::reconocer(ExamenesAnteriores::separar(texto), banco);
            ExamenesAnteriores::datosDeFichero(textoRaro(rng, 4) + QStringLiteral(".pdf"), {QStringLiteral("Nacional Original")});
        }
    }

    // El comprobador de los JSON con JSON estropeados al azar: no falla, los problemas dicen líneas del fichero y, si no
    // da errores, la importación funciona y trae todas las preguntas que ha contado.
    void jsonEstropeados()
    {
        QRandomGenerator rng(14);
        QTemporaryDir dir;
        int importados = 0;
        for (int vuelta = 0; vuelta < vueltas(300); ++vuelta) {
            // Un JSON bueno, de un banco pequeño al azar...
            QJsonArray jtemas;
            for (int t = 1; t <= 2; ++t) {
                QJsonArray jpreguntas;
                for (int n = 1; n <= 3; ++n) {
                    Pregunta p = preguntaRara(rng, t, n, QString());
                    QJsonObject jp{{QStringLiteral("numero"), n},
                                   {QStringLiteral("variante"), QString()},
                                   {QStringLiteral("enunciado"), p.enunciado},
                                   {QStringLiteral("opciones"), QJsonArray::fromStringList(p.opciones)},
                                   {QStringLiteral("correcta"), QString(Pregunta::letraMayuscula(p.correcta))},
                                   {QStringLiteral("referencia"), p.referencia},
                                   {QStringLiteral("explicacion"), p.explicacion}};
                    if (rng.bounded(3) == 0) {
                        jp[QStringLiteral("verificada")] = true;
                        jp[QStringLiteral("verificadores")] = QJsonArray{textoNoVacio(rng, 2)};
                    }
                    jpreguntas.append(jp);
                }
                jtemas.append(QJsonObject{{QStringLiteral("numero"), t}, {QStringLiteral("titulo"), textoNoVacio(rng, 2)},
                                          {QStringLiteral("preguntas"), jpreguntas}});
            }
            QByteArray json = QJsonDocument(QJsonObject{{QStringLiteral("temas"), jtemas}}).toJson();
            // ... estropeado: se quitan, se añaden o se cambian bytes, se corta o se repite un trozo.
            for (int cambios = rng.bounded(4); cambios > 0 && !json.isEmpty(); --cambios) {
                const int donde = rng.bounded(int(json.size()));
                static const char kEspeciales[] = "{}[],:\"\\ \n0123456789-.etrufaln\xc3\xa1\xff\x00";
                static const QByteArray especiales(kEspeciales, sizeof(kEspeciales) - 1);   // con el byte 0
                switch (rng.bounded(6)) {
                case 0: json.remove(donde, 1 + rng.bounded(8)); break;
                case 1: json.insert(donde, especiales.at(rng.bounded(int(especiales.size())))); break;
                case 2: json[donde] = especiales.at(rng.bounded(int(especiales.size()))); break;
                case 3: json.truncate(donde); break;
                case 4: json.insert(donde, json.mid(rng.bounded(int(json.size())), 1 + rng.bounded(30))); break;
                case 5: json.replace(QByteArray("\"A\""), QByteArray("\"E\"")); break;
                }
            }
            const int lineas = int(json.count('\n')) + 1;
            const ComprobadorJson::Resultado r = ComprobadorJson::comprobar(json);
            QVERIFY(!ComprobadorJson::resumen(r).isEmpty());
            for (const ComprobadorJson::Problema &p : r.problemas)
                QVERIFY2(p.linea >= 0 && p.linea <= lineas + 1, qPrintable(QStringLiteral("vuelta %1: línea %2 de %3").arg(vuelta).arg(p.linea).arg(lineas)));
            if (!r.valido())
                continue;
            // Sin errores: se importa, con todas sus preguntas.
            const QString ruta = dir.filePath(QStringLiteral("estropeado%1.json").arg(vuelta));
            QFile f(ruta);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(json);
            f.close();
            BaseDatos bd;
            QVERIFY(bd.abrir(dir.filePath(QStringLiteral("estropeado%1.db").arg(vuelta))));
            const int n = bd.importarJson(ruta, false);
            QVERIFY2(n == r.preguntas, qPrintable(QStringLiteral("vuelta %1: el comprobador cuenta %2 preguntas y se importan %3 (%4)\n%5")
                                                      .arg(vuelta).arg(r.preguntas).arg(n).arg(bd.ultimoError(), QString::fromUtf8(json))));
            QCOMPARE(int(bd.preguntas().size()), n);
            ++importados;
        }
        QVERIFY(importados > 50);   // la sonda prueba también JSON que se importan
    }

    // Lo que exporta el programa lo acepta su comprobador, y la ida y vuelta base de datos → JSON → base de datos
    // conserva cada pregunta tal cual (con sus textos raros, su imagen y su verificación).
    void idaYVueltaDelJson()
    {
        QRandomGenerator rng(15);
        QTemporaryDir dir;
        for (int vuelta = 0; vuelta < vueltas(15); ++vuelta) {
            BaseDatos origen;
            QVERIFY(origen.abrir(dir.filePath(QStringLiteral("origen%1.db").arg(vuelta))));
            QVERIFY(origen.guardarAsignatura(textoNoVacio(rng, 3).replace(QLatin1Char('\n'), QLatin1Char(' '))));
            QVector<Tema> temas;
            for (int t = 1; t <= 3; ++t) {
                Tema tema;
                tema.numero = t;
                tema.titulo = textoNoVacio(rng, 3);
                tema.autor = textoRaro(rng, 2);
                QVERIFY(origen.guardarTema(tema));
                temas << tema;
            }
            QVector<Pregunta> banco = bancoRaro(rng, 3, 6);
            const QStringList personas{textoNoVacio(rng, 2).trimmed(), QStringLiteral("Persona ") + textoNoVacio(rng, 1).trimmed()};
            for (Pregunta &p : banco) {
                p.id = 0;
                p.temaId = temas.at(p.temaNumero - 1).id;
                if (p.verificada)
                    p.verificadores = rng.bounded(2) ? QStringList{personas.at(0)} : personas;
                QVERIFY2(origen.guardarPregunta(p), qPrintable(origen.ultimoError()));
            }
            // Lo que se guarda es lo que se lee.
            const QVector<Pregunta> guardadas = origen.preguntas();
            QCOMPARE(guardadas.size(), banco.size());
            QHash<QString, QString> esperadas;
            for (Pregunta p : std::as_const(banco)) {
                p = comoLaGuarda(p);
                p.verificadores.sort(Qt::CaseInsensitive);
                esperadas.insert(p.codigoCompleto(), describir(p));
            }
            for (Pregunta p : guardadas) {
                p.verificadores.sort(Qt::CaseInsensitive);
                QVERIFY2(describir(p) == esperadas.value(p.codigoCompleto()),
                         qPrintable(QStringLiteral("vuelta %1, %2: lo leído no es lo guardado, %3")
                                        .arg(vuelta).arg(p.codigoCompleto(), diferencia(describir(p), esperadas.value(p.codigoCompleto())))));
            }

            const QString json = dir.filePath(QStringLiteral("ida%1.json").arg(vuelta));
            QVERIFY(origen.exportarJson(json));
            const ComprobadorJson::Resultado r = ComprobadorJson::comprobarFichero(json);
            QString errores;
            for (const ComprobadorJson::Problema &p : r.problemas)
                if (p.gravedad == ComprobadorJson::Problema::Error)
                    errores += QStringLiteral("\nlínea %1 (%2): %3").arg(p.linea).arg(p.donde, p.texto);
            QVERIFY2(r.valido(), qPrintable(QStringLiteral("vuelta %1: el comprobador rechaza lo que exporta el programa:%2").arg(vuelta).arg(errores)));

            BaseDatos destino;
            QVERIFY(destino.abrir(dir.filePath(QStringLiteral("destino%1.db").arg(vuelta))));
            QCOMPARE(destino.importarJson(json, false), int(banco.size()));
            const QVector<Pregunta> vueltas = destino.preguntas();
            QCOMPARE(vueltas.size(), guardadas.size());
            for (int i = 0; i < vueltas.size(); ++i)
                QVERIFY2(describir(vueltas.at(i)) == describir(guardadas.at(i)),
                         qPrintable(QStringLiteral("vuelta %1, %2: cambia al pasar por el JSON, %3")
                                        .arg(vuelta).arg(guardadas.at(i).codigoCompleto(), diferencia(describir(vueltas.at(i)), describir(guardadas.at(i))))));
            QCOMPARE(destino.asignatura(), origen.asignatura());
            QVERIFY2(destino.diferenciasConJson(json).isEmpty(), qPrintable(destino.diferenciasConJson(json).join(QLatin1Char('\n'))));
        }
    }

    // El generador con bancos y opciones al azar cumple sus reglas, y también después de cambiar preguntas.
    void generadorConBancosAlAzar()
    {
        QRandomGenerator rng(16);
        for (int vuelta = 0; vuelta < vueltas(150); ++vuelta) {
            const int temas = 1 + rng.bounded(5);
            const QVector<Pregunta> banco = bancoRaro(rng, temas, 2 + rng.bounded(14));
            ConfigExamen c;
            for (int t = 1; t <= temas; ++t)
                if (rng.bounded(4))
                    c.preguntasPorTema.insert(t, rng.bounded(7));
            for (int m = 1 + rng.bounded(4); m > 0; --m)
                c.nombresModelos << QStringLiteral("Modelo %1").arg(m);
            c.noRepetirEntreModelos = rng.bounded(2);
            c.incluirParaRevisar = rng.bounded(2);
            c.ordenAleatorio = rng.bounded(2);
            c.barajarOpciones = rng.bounded(2);
            c.evitarOld = rng.bounded(2);
            c.soloVerificadas = rng.bounded(3) == 0;
            for (const Pregunta &p : banco)
                if (rng.bounded(5) == 0)
                    c.usoReciente.insert(p.codigoCompleto(), 1 + rng.bounded(3));
            c.semilla = 1 + rng.bounded(100000);
            const QString contexto = QStringLiteral("vuelta %1 (semilla %2)").arg(vuelta).arg(c.semilla);

            ResultadoGeneracion r = Generador::generar(banco, c);
            const ResultadoGeneracion otra = Generador::generar(banco, c);
            QCOMPARE(r.semilla, c.semilla);
            auto firma = [](const ResultadoGeneracion &x) {
                QString f;
                for (const Modelo &m : x.modelos)
                    for (const PreguntaExamen &pe : m.preguntas)
                        f += QStringLiteral("%1/%2%3%4%5 ").arg(pe.pregunta.id).arg(pe.orden[0]).arg(pe.orden[1]).arg(pe.orden[2]).arg(pe.orden[3]);
                return f;
            };
            QVERIFY2(firma(r) == firma(otra), qPrintable(contexto + QStringLiteral(": con la misma semilla salen otros")));
            QCOMPARE(r.modelos.size(), c.nombresModelos.size());

            auto comprobar = [&](const ResultadoGeneracion &res, const QString &cuando) {
                const QString aviso = res.avisos.join(QLatin1Char('\n'));
                QHash<QString, QSet<int>> modelosDelGrupo;
                for (int mi = 0; mi < res.modelos.size(); ++mi) {
                    const Modelo &m = res.modelos.at(mi);
                    QCOMPARE(m.nombre, c.nombresModelos.at(mi));
                    QSet<QString> grupos;
                    QMap<int, int> porTema;
                    std::array<int, 4> letras{};
                    bool todasBarajables = true;
                    for (const PreguntaExamen &pe : m.preguntas) {
                        const Pregunta &p = pe.pregunta;
                        const QString donde = contexto + cuando + QStringLiteral(", %1, %2").arg(m.nombre, p.codigoCompleto());
                        QVERIFY2(p.activa() && (c.incluirParaRevisar || !p.revisar) && (!c.soloVerificadas || p.verificada),
                                 qPrintable(donde + QStringLiteral(": no debería salir")));
                        QVERIFY2(!grupos.contains(p.grupo()), qPrintable(donde + QStringLiteral(": dos variantes en el examen")));
                        grupos.insert(p.grupo());
                        modelosDelGrupo[p.grupo()].insert(mi);
                        ++porTema[p.temaNumero];
                        std::array<int, 4> orden = pe.orden;
                        std::sort(orden.begin(), orden.end());
                        QVERIFY2(orden == (std::array<int, 4>{{0, 1, 2, 3}}), qPrintable(donde + QStringLiteral(": el orden no es una permutación")));
                        QCOMPARE(pe.opcion(pe.correcta()), p.opciones.at(p.correcta));
                        if (Estilo::tieneOpcionesDependientes(p)) {
                            todasBarajables = false;
                            QVERIFY2(pe.orden == (std::array<int, 4>{{0, 1, 2, 3}}), qPrintable(donde + QStringLiteral(": barajada con opciones que remiten a otras")));
                        }
                        ++letras[pe.correcta()];
                    }
                    // Ninguna pseudorrepetida junto a la suya.
                    for (const PreguntaExamen &pe : m.preguntas)
                        if (!pe.pregunta.pseudoCon.isEmpty())
                            QVERIFY2(!grupos.contains(pe.pregunta.pseudoCon.toUpper()),
                                     qPrintable(contexto + cuando + QStringLiteral(", %1: %2 sale con su pseudorrepetida %3")
                                                                     .arg(m.nombre, pe.pregunta.codigoCompleto(), pe.pregunta.pseudoCon)));
                    // De cada tema, las pedidas; si no, un aviso.
                    for (auto it = c.preguntasPorTema.cbegin(); it != c.preguntasPorTema.cend(); ++it) {
                        QVERIFY2(porTema.value(it.key()) <= it.value(), qPrintable(contexto + cuando + QStringLiteral(": más de las pedidas")));
                        if (porTema.value(it.key()) < it.value())
                            QVERIFY2(aviso.contains(QStringLiteral("Tema %1: se pidieron").arg(it.key())),
                                     qPrintable(contexto + cuando + QStringLiteral(": faltan preguntas del tema %1 sin aviso:\n%2").arg(it.key()).arg(aviso)));
                    }
                    for (auto it = porTema.cbegin(); it != porTema.cend(); ++it)
                        QVERIFY2(c.preguntasPorTema.contains(it.key()), qPrintable(contexto + QStringLiteral(": sale un tema que no se pidió")));
                    // Barajadas, la correcta repartida por igual (como mucho, una de diferencia).
                    if (c.barajarOpciones && todasBarajables && !m.preguntas.isEmpty())
                        QVERIFY2(*std::max_element(letras.begin(), letras.end()) - *std::min_element(letras.begin(), letras.end()) <= 1,
                                 qPrintable(contexto + cuando + QStringLiteral(", %1: la correcta no está repartida (A %2 · B %3 · C %4 · D %5)")
                                                                 .arg(m.nombre).arg(letras[0]).arg(letras[1]).arg(letras[2]).arg(letras[3])));
                }
                // Sin repetir entre exámenes, salvo que no haya bastantes (y entonces, un aviso).
                if (c.noRepetirEntreModelos)
                    for (auto it = modelosDelGrupo.cbegin(); it != modelosDelGrupo.cend(); ++it)
                        if (it.value().size() > 1)
                            // El de la generación o, si ha sido al cambiar una pregunta, el de ese cambio.
                            QVERIFY2(aviso.contains(QStringLiteral("para no repetir entre exámenes"))
                                         || aviso.contains(QStringLiteral("está también en otro examen de la convocatoria")),
                                     qPrintable(contexto + cuando + QStringLiteral(": %1 se repite sin aviso. Avisos:\n%2").arg(it.key(), aviso)));
            };
            comprobar(r, QString());
            if (QTest::currentTestFailed())
                return;

            // Cambiar unas preguntas por otras: siguen las reglas y, barajadas, la correcta se queda en su letra.
            for (int k = 0; k < 4 && !r.modelos.isEmpty(); ++k) {
                const int mi = rng.bounded(int(r.modelos.size()));
                if (r.modelos.at(mi).preguntas.isEmpty())
                    continue;
                const int pi = rng.bounded(int(r.modelos.at(mi).preguntas.size()));
                const int letra = r.modelos.at(mi).preguntas.at(pi).correcta();
                const int tema = r.modelos.at(mi).preguntas.at(pi).pregunta.temaNumero;
                std::array<int, 4> resto{};   // la correcta en el resto del examen
                for (int i = 0; i < r.modelos.at(mi).preguntas.size(); ++i)
                    if (i != pi)
                        ++resto[r.modelos.at(mi).preguntas.at(i).correcta()];
                const int menos = *std::min_element(resto.begin(), resto.end());
                if (Generador::cambiarPregunta(banco, c, r, mi, pi, {})) {
                    const PreguntaExamen &nueva = r.modelos.at(mi).preguntas.at(pi);
                    QCOMPARE(nueva.pregunta.temaNumero, tema);
                    // Barajada, en la letra de la quitada si es de las que menos salen en el resto; si no, en una de ellas.
                    if (c.barajarOpciones && !Estilo::tieneOpcionesDependientes(nueva.pregunta))
                        QVERIFY2(resto[letra] == menos ? nueva.correcta() == letra : resto[nueva.correcta()] == menos,
                                 qPrintable(contexto + QStringLiteral(": al cambiar, la correcta va a la %1 (era la %2)").arg(nueva.correcta()).arg(letra)));
                }
                comprobar(r, QStringLiteral(", tras cambiar la %1 de «%2»").arg(pi + 1).arg(r.modelos.at(mi).nombre));
                if (QTest::currentTestFailed())
                    return;
            }
        }
    }

    // Los bloques de los enunciados: el texto raro se reparte en bloques sin perder nada que no sea una marca.
    void bloquesDeLosEnunciados()
    {
        QRandomGenerator rng(17);
        for (int i = 0; i < vueltas(3000); ++i) {
            const QString texto = rng.bounded(2) ? enunciadoRaro(rng) : textoRaro(rng, rng.bounded(15));
            for (const Enunciado::Bloque &b : Enunciado::bloques(texto)) {
                if (b.tipo == Enunciado::Bloque::Tabla)
                    QVERIFY(!b.filas.isEmpty());
                for (const QString &linea : b.lineas)
                    QVERIFY(!linea.contains(QLatin1Char('\n')));
            }
            Enunciado::avisosDelCodigo(texto);
        }
    }
};

QTEST_MAIN(PruebaSondas)
#include "prueba_sondas.moc"
