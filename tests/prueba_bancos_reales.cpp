// Comprobaciones con los bancos de preguntas reales, que no están en git: cada una se salta si no está el enlace a la
// carpeta de su banco («datos», «banco2» o «BancoPreguntas»: ver CLAUDE.md). Se trabaja con una copia de cada base de
// datos, y lo que falla se dice sin el texto de las preguntas.

#include "basedatos.h"
#include "comprobadorjson.h"
#include "docx.h"
#include "estilo.h"
#include "examenesanteriores.h"
#include "exportacion.h"
#include "generador.h"
#include "maquetador.h"
#include "moodle.h"
#include "texto_exportado.h"

#include <QFile>
#include <QProcess>
#include <QSet>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QtTest>
#include <algorithm>

namespace {

const QString kBancos = QStringLiteral(RUTA_PROYECTO "/");

struct Banco
{
    QVector<Pregunta> preguntas;
    QVector<Tema> temas;
    QString asignatura;
    QString idioma;
    QVector<Reparto> repartos;
};

Banco cargar(const QString &relativa)
{
    Banco b;
    QTemporaryDir dir;
    const QString copia = dir.filePath(QStringLiteral("copia.db"));
    if (!QFile::copy(kBancos + relativa, copia))
        return b;
    BaseDatos bd;
    if (bd.abrir(copia)) {
        b.preguntas = bd.preguntas();
        b.temas = bd.temas();
        b.asignatura = bd.asignatura();
        b.idioma = bd.idioma();
        b.repartos = bd.repartos();
    }
    return b;
}

// El aviso de cuando no está un banco.
QString sinBanco(const QString &ruta)
{
    return QStringLiteral("No está el banco (enlace «%1»).").arg(ruta.section(QLatin1Char('/'), 0, 0));
}

// Grupos (T3-5) que no pueden salir juntos: los enlazados con «pseudorrepetida con», en los dos sentidos.
QSet<QPair<QString, QString>> enlaces(const QVector<Pregunta> &banco)
{
    QSet<QPair<QString, QString>> res;
    for (const Pregunta &p : banco) {
        const QString otro = p.pseudoCon.trimmed().toUpper();
        if (!otro.isEmpty()) {
            res.insert({p.grupo(), otro});
            res.insert({otro, p.grupo()});
        }
    }
    return res;
}

} // namespace

class PruebaBancosReales : public QObject
{
    Q_OBJECT

private slots:
    void bancos_data()
    {
        QTest::addColumn<QString>("ruta");
        QTest::addColumn<bool>("debenCaber");
        QTest::addColumn<bool>("conFormatoReferencia");   // si sus temas dicen la forma de su referencia (la primera asignatura)
        QTest::newRow("ver11") << QStringLiteral("datos/preguntas.db") << true << true;
        // El banco 2, pendiente de revisión, tiene preguntas el doble de largas, y las de la segunda asignatura llevan
        // mucho código (sus exámenes no se ajustan a un folio por cada 10): de ellos solo se informa.
        QTest::newRow("banco 2") << QStringLiteral("banco2/preguntas_banco2.db") << false << true;
        QTest::newRow("BancoPreguntas") << QStringLiteral("BancoPreguntas/preguntas.db") << false << false;
    }

    void bancos()
    {
        QFETCH(QString, ruta);
        QFETCH(bool, debenCaber);
        QFETCH(bool, conFormatoReferencia);
        if (!QFile::exists(kBancos + ruta))
            QSKIP(qPrintable(sinBanco(ruta)));
        const Banco b = cargar(ruta);
        QVERIFY(!b.preguntas.isEmpty());
        // Con el nombre de la asignatura, sus repartos y, en los de la primera, la forma de la referencia de los temas
        // (los bancos de antes de que los llevasen se ponen al día al abrirlos).
        QVERIFY(!b.asignatura.isEmpty());
        QVERIFY(!b.repartos.isEmpty());
        if (conFormatoReferencia)
            QVERIFY(std::any_of(b.temas.cbegin(), b.temas.cend(), [](const Tema &t) { return !t.formatoReferencia.isEmpty(); }));

        // Las reglas del generador, con cien semillas y un reparto de diez preguntas como el de los exámenes.
        const QMap<int, int> disponibles = Generador::gruposDisponibles(b.preguntas, true);
        ConfigExamen c;
        int total = 0;
        for (int tema : disponibles.keys())
            if (total < 10) {
                c.preguntasPorTema.insert(tema, 1);
                ++total;
            }
        for (auto it = c.preguntasPorTema.begin(); it != c.preguntasPorTema.end() && total < 10; ++it)
            if (disponibles.value(it.key()) > 1) {
                ++it.value();
                ++total;
            }
        c.nombresModelos = {QStringLiteral("Uno"), QStringLiteral("Dos")};
        c.barajarOpciones = true;
        const QSet<QPair<QString, QString>> enlazadas = enlaces(b.preguntas);
        int nivelMaximo = 0, noCaben = 0;
        for (quint32 semilla = 1; semilla <= 100; ++semilla) {
            c.semilla = semilla;
            const ResultadoGeneracion r = Generador::generar(b.preguntas, c);
            for (const Modelo &m : r.modelos) {
                QSet<QString> grupos;
                for (const PreguntaExamen &pe : m.preguntas) {
                    QVERIFY2(!grupos.contains(pe.pregunta.grupo()), qPrintable(QStringLiteral("dos variantes de %1").arg(pe.pregunta.grupo())));
                    for (const QString &g : std::as_const(grupos))
                        QVERIFY2(!enlazadas.contains({g, pe.pregunta.grupo()}),
                                 qPrintable(QStringLiteral("%1 y %2 están enlazadas").arg(g, pe.pregunta.grupo())));
                    grupos.insert(pe.pregunta.grupo());
                    QVERIFY(pe.pregunta.activa());
                }
            }
            // Que quepan en su folio, con una de cada diez semillas (la maqueta es lo más lento).
            if (semilla % 10 == 0) {
                DatosConvocatoria d;
                d.convocatoria = QStringLiteral("Junio");
                d.anio = 2026;
                bool caben = false;
                nivelMaximo = qMax(nivelMaximo, Maquetador::nivelParaCaber(r.modelos, d, &caben));
                if (debenCaber)
                    QVERIFY2(caben, qPrintable(QStringLiteral("semilla %1: no caben en un folio").arg(semilla)));
                noCaben += !caben;
            }
        }
        qInfo().noquote() << QStringLiteral("%1 preguntas; nivel de compactación máximo: %2; generaciones que no caben en un folio: %3 de 10")
                                 .arg(b.preguntas.size()).arg(nivelMaximo).arg(noCaben);
    }

    // Las erratas corregidas el 2026-10-20 que volverían si se regenerase el JSON desde el .docx ver11 (ver CLAUDE.md).
    void erratasCorregidas()
    {
        const QString ruta = QStringLiteral("datos/preguntas.db");
        if (!QFile::exists(kBancos + ruta))
            QSKIP(qPrintable(sinBanco(ruta)));
        const QStringList erratas{QStringLiteral("wearable"), QStringLiteral("rectas"), QStringLiteral("«¿»"),
                                  QStringLiteral("«?» de cierre"), QStringLiteral("concordancia")};
        const Banco b = cargar(ruta);
        for (const Pregunta &p : b.preguntas)
            for (const Estilo::Aviso &a : Estilo::revisar(p, b.temas))
                for (const QString &e : erratas)
                    QVERIFY2(!a.texto.contains(e), qPrintable(p.codigoCompleto() + QStringLiteral(": ") + a.texto));
    }

    // Los JSON de los bancos pasan el comprobador sin errores ni avisos (no los hay de verdad: si sale alguno, es
    // del comprobador, o del JSON). Solo se enseñan la línea y la pregunta, no su texto.
    void jsonComprobados_data()
    {
        QTest::addColumn<QString>("ruta");
        QTest::newRow("ver11") << QStringLiteral("datos/preguntas_fi.json");
        QTest::newRow("banco 2") << QStringLiteral("banco2/preguntas_banco2.json");
        QTest::newRow("BancoPreguntas") << QStringLiteral("BancoPreguntas/preguntas_tpi.json");
    }
    void jsonComprobados()
    {
        QFETCH(QString, ruta);
        if (!QFile::exists(kBancos + ruta))
            QSKIP(qPrintable(sinBanco(ruta)));
        const ComprobadorJson::Resultado r = ComprobadorJson::comprobarFichero(kBancos + ruta);
        QStringList problemas;
        for (const ComprobadorJson::Problema &p : r.problemas)
            problemas << QStringLiteral("línea %1, %2").arg(p.linea).arg(p.donde);
        QVERIFY2(r.problemas.isEmpty(), qPrintable(problemas.join(QStringLiteral("; "))));
        QVERIFY(r.preguntas > 300);
    }

    // Todas las preguntas de cada banco, de 20 en 20 y en su orden, exportadas como exámenes (con la maqueta que toque,
    // con la de letra grande y con la más compacta; con y sin soluciones) en PDF, HTML y Word (y, con pdflatex y
    // SONDAS_PDFLATEX=1, en LaTeX compilado), y el banco entero a Moodle: en cada fichero está cada pregunta, en su
    // orden, con todos sus textos (ver faltanDelExamen()). Como la sonda nadaSePierdeAlExportar, pero con las preguntas
    // de verdad. Tarda (unos 20 segundos con los tres bancos; con el LaTeX, minuto y medio): solo con BANCOS_EXPORTAR=1.
    void nadaSePierdeAlExportar_data()
    {
        QTest::addColumn<QString>("ruta");
        QTest::newRow("ver11") << QStringLiteral("datos/preguntas.db");
        QTest::newRow("banco 2") << QStringLiteral("banco2/preguntas_banco2.db");
        QTest::newRow("BancoPreguntas") << QStringLiteral("BancoPreguntas/preguntas.db");
    }
    void nadaSePierdeAlExportar()
    {
        QFETCH(QString, ruta);
        if (!qEnvironmentVariableIsSet("BANCOS_EXPORTAR"))
            QSKIP("Tarda: solo con BANCOS_EXPORTAR=1");
        if (!QFile::exists(kBancos + ruta))
            QSKIP(qPrintable(sinBanco(ruta)));
        Banco b = cargar(ruta);
        QVERIFY(!b.preguntas.isEmpty());
        QVERIFY(!b.asignatura.isEmpty());   // los pies de página empiezan por ella (textoSinPies())
        const bool leePdf = ExamenesAnteriores::puedeLeerPdf();
        const QString pdflatex = qEnvironmentVariableIsSet("SONDAS_PDFLATEX") ? QStandardPaths::findExecutable(QStringLiteral("pdflatex"))
                                                                              : QString();
        DatosConvocatoria d;
        d.asignatura = b.asignatura;
        d.idioma = b.idioma;
        d.convocatoria = QStringLiteral("Junio");
        d.anio = 2026;
        d.instrucciones = DatosConvocatoria::instruccionesPorDefecto(b.idioma);
        QVector<Pregunta> preguntas = b.preguntas;
        std::stable_sort(preguntas.begin(), preguntas.end(), [](const Pregunta &x, const Pregunta &y) { return x.antesQue(y); });
        QTemporaryDir dir;
        QStringList faltan;
        for (int desde = 0, n = 0; desde < preguntas.size(); desde += 20, ++n) {
            Modelo m;
            m.nombre = QStringLiteral("Examen %1").arg(n + 1);
            for (int i = desde; i < qMin(desde + 20, int(preguntas.size())); ++i) {
                PreguntaExamen pe;
                pe.pregunta = preguntas.at(i);
                m.preguntas << pe;
            }
            const int nivel = n % 3 == 1 ? Maquetador::kLetraGrande
                            : n % 3 == 2 ? Maquetador::numNiveles() - 1
                                         : Maquetador::nivelParaCaber({m}, d);
            for (bool soluciones : {false, true}) {
                const QString donde = QStringLiteral("examen %1 (nivel %2%3)").arg(n + 1).arg(nivel)
                                          .arg(soluciones ? QStringLiteral(", con soluciones") : QString());
                const auto comprobar = [&](const QString &formato, const QString &texto) {
                    for (const QString &falta : faltanDelExamen(texto, m, soluciones, false))
                        faltan << QStringLiteral("%1, %2: %3").arg(formato, donde, falta);
                };
                QTextDocument html;
                html.setHtml(Maquetador::examenesHtml({m}, d, soluciones, nivel));
                comprobar(QStringLiteral("HTML"), html.toPlainText());
                comprobar(QStringLiteral("Word"), textoDeWord(Docx::partes({m}, d, soluciones, nivel).value(QStringLiteral("word/document.xml"))));
                if (leePdf) {
                    const QString pdf = dir.filePath(QStringLiteral("examen.pdf"));
                    QVERIFY(Exportacion::pdf(pdf, {m}, d, soluciones, nivel));
                    comprobar(QStringLiteral("PDF"), textoSinPies(ExamenesAnteriores::textoDePdf(pdf), d));
                }
                if (!pdflatex.isEmpty() && leePdf) {
                    QVERIFY(Exportacion::latex(dir.filePath(QStringLiteral("latex.tex")), {m}, d, soluciones, nivel));
                    QProcess proceso;
                    proceso.setWorkingDirectory(dir.path());
                    proceso.start(pdflatex, {QStringLiteral("-interaction=nonstopmode"), QStringLiteral("-halt-on-error"), QStringLiteral("latex.tex")});
                    QVERIFY(proceso.waitForFinished(60000));
                    if (proceso.exitCode() != 0)
                        faltan << QStringLiteral("LaTeX, %1: no compila").arg(donde);
                    else
                        comprobar(QStringLiteral("LaTeX"), textoSinPies(ExamenesAnteriores::textoDePdf(dir.filePath(QStringLiteral("latex.pdf"))), d));
                }
            }
        }
        // Moodle: cada pregunta, con su enunciado, sus opciones, su explicación y su referencia.
        Moodle::Opciones o;
        o.asignatura = b.asignatura;
        o.idioma = b.idioma;
        const QVector<Pregunta> seleccion = Moodle::seleccion(b.preguntas, o);
        QXmlStreamReader r(Moodle::xml(b.temas, seleccion, o));
        QStringList textos;
        bool enPregunta = false;
        while (!r.atEnd()) {
            r.readNext();
            if (r.isStartElement() && r.name() == QLatin1String("question")) {
                enPregunta = r.attributes().value(QStringLiteral("type")) == QLatin1String("multichoice");
                if (enPregunta)
                    textos << QString();
            } else if (enPregunta && r.isStartElement() && r.name() == QLatin1String("text")) {
                QTextDocument texto;
                texto.setHtml(r.readElementText());
                textos.last() += soloLetras(texto.toPlainText());
            }
        }
        QVERIFY2(!r.hasError(), qPrintable(r.errorString()));
        QCOMPARE(textos.size(), seleccion.size());
        for (int i = 0; i < seleccion.size(); ++i) {
            const TrozosDePregunta t = trozosDe(seleccion.at(i), {{0, 1, 2, 3}}, true);
            for (const QStringList &l : {t.enunciado, t.opciones, t.solucion})
                for (const QString &trozo : l)
                    if (!textos.at(i).contains(trozo))
                        faltan << QStringLiteral("Moodle: %1: falta un trozo").arg(seleccion.at(i).codigoCompleto());
        }
        QVERIFY2(faltan.isEmpty(), qPrintable(QStringLiteral("%1 fallos: %2").arg(faltan.size()).arg(faltan.mid(0, 30).join(QStringLiteral("; ")))));
        qInfo().noquote() << QStringLiteral("%1 preguntas exportadas sin perder nada").arg(preguntas.size());
    }
};

QTEST_MAIN(PruebaBancosReales)
#include "prueba_bancos_reales.moc"
