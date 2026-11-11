// Maqueta de los exámenes: que quepan en su folio (compactando si hace falta) y los ficheros exportados.

#include "datos_prueba.h"
#include "exportacion.h"
#include "generador.h"
#include "maquetador.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

namespace {

QVector<Modelo> examenes(const QVector<Pregunta> &banco, quint32 semilla, int modelos = 2)
{
    ConfigExamen c;
    c.preguntasPorTema = {{1, 2}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 2}, {7, 1}, {8, 1}};
    for (int i = 0; i < modelos; ++i)
        c.nombresModelos << QStringLiteral("Examen %1").arg(i + 1);
    c.semilla = semilla;
    c.barajarOpciones = true;
    return Generador::generar(banco, c).modelos;
}

DatosConvocatoria convocatoria()
{
    DatosConvocatoria d;
    d.convocatoria = QStringLiteral("Junio");
    d.anio = 2026;
    return d;
}

QByteArray leer(const QString &ruta)
{
    QFile f(ruta);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

} // namespace

class PruebaMaqueta : public QObject
{
    Q_OBJECT

private slots:
    void proximaConvocatoria_data()
    {
        QTest::addColumn<QDate>("hoy");
        QTest::addColumn<QString>("convocatoria");
        QTest::addColumn<int>("anio");
        QTest::newRow("octubre") << QDate(2026, 10, 27) << QStringLiteral("Febrero") << 2027;
        QTest::newRow("diciembre") << QDate(2026, 12, 31) << QStringLiteral("Febrero") << 2027;
        QTest::newRow("enero") << QDate(2027, 1, 1) << QStringLiteral("Febrero") << 2027;
        QTest::newRow("febrero") << QDate(2027, 2, 1) << QStringLiteral("Junio") << 2027;
        QTest::newRow("mayo") << QDate(2027, 5, 31) << QStringLiteral("Junio") << 2027;
        QTest::newRow("junio") << QDate(2027, 6, 1) << QStringLiteral("Septiembre") << 2027;
        QTest::newRow("agosto") << QDate(2027, 8, 31) << QStringLiteral("Septiembre") << 2027;
        QTest::newRow("septiembre") << QDate(2027, 9, 1) << QStringLiteral("Febrero") << 2028;
    }

    void proximaConvocatoria()
    {
        QFETCH(QDate, hoy);
        QFETCH(QString, convocatoria);
        QFETCH(int, anio);
        const DatosConvocatoria d = DatosConvocatoria::proxima(hoy);
        QCOMPARE(d.convocatoria, convocatoria);
        QCOMPARE(d.anio, anio);
        QVERIFY(DatosConvocatoria::ordinarias().contains(d.convocatoria));
    }

    void convocatoriaAnterior()
    {
        auto anterior = [](const QDate &hoy) { return DatosConvocatoria::anterior(hoy).convocatoriaYAnio(); };
        QCOMPARE(anterior(QDate(2026, 10, 27)), QStringLiteral("Septiembre 2026"));
        QCOMPARE(anterior(QDate(2027, 1, 31)), QStringLiteral("Septiembre 2026"));
        QCOMPARE(anterior(QDate(2027, 2, 1)), QStringLiteral("Febrero 2027"));
        QCOMPARE(anterior(QDate(2027, 5, 31)), QStringLiteral("Febrero 2027"));
        QCOMPARE(anterior(QDate(2027, 6, 1)), QStringLiteral("Junio 2027"));
        QCOMPARE(anterior(QDate(2027, 9, 1)), QStringLiteral("Septiembre 2027"));
        // Siempre la anterior a la próxima.
        for (QDate d(2026, 1, 1); d.year() == 2026; d = d.addDays(1)) {
            const DatosConvocatoria a = DatosConvocatoria::anterior(d), p = DatosConvocatoria::proxima(d);
            QVERIFY(a.anio * 12 + DatosConvocatoria::mesDe(a.convocatoria) < p.anio * 12 + DatosConvocatoria::mesDe(p.convocatoria));
        }
        QCOMPARE(DatosConvocatoria::mesDe(QStringLiteral("septiembre (reserva)")), 9);
        QCOMPARE(DatosConvocatoria::mesDe(QStringLiteral("Extraordinaria")), 0);
        QCOMPARE(DatosConvocatoria::nombreDeMes(2), QStringLiteral("Febrero"));
        QCOMPARE(DatosConvocatoria::nombreDeMes(13), QString());
    }

    void paginasObjetivo()
    {
        QCOMPARE(Maquetador::paginasObjetivo(0), 1);
        QCOMPARE(Maquetador::paginasObjetivo(10), 1);
        QCOMPARE(Maquetador::paginasObjetivo(11), 2);
        QCOMPARE(Maquetador::paginasObjetivo(20), 2);
    }

    void preguntasNormalesCabenSinCompactar()
    {
        const QVector<Pregunta> banco = bancoInventado();
        for (quint32 semilla = 1; semilla <= 10; ++semilla) {
            bool caben = false;
            QCOMPARE(Maquetador::nivelParaCaber(examenes(banco, semilla), convocatoria(), &caben), 0);
            QVERIFY(caben);
        }
    }

    void preguntasLargasCompactan()
    {
        // Enunciados de unos 110 caracteres (el doble de lo habitual) y opciones de unos 30, y alguna pregunta de código:
        // hay que compactar, pero deben caber.
        QVector<Pregunta> banco = bancoInventado();
        for (Pregunta &p : banco) {
            p.enunciado = QStringLiteral("Enunciado inventado y largo de la pregunta %1, con dos frases seguidas, para ocupar "
                                         "dos líneas de la hoja:").arg(p.codigoCompleto());
            if (p.numero == 5)
                p.enunciado = QStringLiteral("Este programa en C:\n```\nint i;\nfor (i = 0; i < 4; i++)\n"
                                             "    printf(\"%d\", i);\n```\nQué hace:");
            for (QString &o : p.opciones)
                o = QStringLiteral("Una opción inventada de prueba.");
        }
        int maximo = 0;
        for (quint32 semilla = 1; semilla <= 10; ++semilla) {
            bool caben = false;
            const int nivel = Maquetador::nivelParaCaber(examenes(banco, semilla), convocatoria(), &caben);
            QVERIFY2(caben, qPrintable(QStringLiteral("semilla %1").arg(semilla)));
            maximo = qMax(maximo, nivel);
        }
        QVERIFY(maximo > 0);
    }

    void formasDePuntuar()
    {
        // Sobre 10 puntos; con «al azar», contestar al azar da 0 de media (salvo el redondeo a centésimas).
        auto puntos = [](DatosConvocatoria::Puntuacion forma, int preguntas) {
            DatosConvocatoria d;
            d.puntosAcierto = 2;
            d.puntosFallo = 0.5;
            d.puntuar(forma, preguntas);
            return qMakePair(d.puntosAcierto, d.puntosFallo);
        };
        QCOMPARE(puntos(DatosConvocatoria::Resta30, 10), qMakePair(1.0, 0.3));
        QCOMPARE(puntos(DatosConvocatoria::Resta30, 20), qMakePair(0.5, 0.15));
        QCOMPARE(puntos(DatosConvocatoria::RestaAlAzar, 10), qMakePair(1.0, 0.33));
        QCOMPARE(puntos(DatosConvocatoria::RestaAlAzar, 20), qMakePair(0.5, 0.17));
        QCOMPARE(puntos(DatosConvocatoria::SinResta, 20), qMakePair(0.5, 0.0));
        QCOMPARE(puntos(DatosConvocatoria::Personalizada, 20), qMakePair(2.0, 0.5));
        QCOMPARE(puntos(DatosConvocatoria::Resta30, 0), qMakePair(2.0, 0.5));   // sin preguntas, como estaba
        const double alAzar = 0.25 * 1.0 - 0.75 * DatosConvocatoria::fraccionFalloAlAzar();
        QVERIFY(qAbs(alAzar) < 1e-12);
    }

    void avisaSiNoCabe()
    {
        QVector<Pregunta> banco = bancoInventado();
        for (Pregunta &p : banco)
            p.enunciado = QStringLiteral("Una línea inventada.\n").repeated(12) + QStringLiteral("Pregunta:");
        bool caben = true;
        QCOMPARE(Maquetador::nivelParaCaber(examenes(banco, 1, 1), convocatoria(), &caben), Maquetador::numNiveles() - 1);
        QVERIFY(!caben);
    }

    void letraGrande()
    {
        QVector<Pregunta> banco = bancoInventado();
        for (Pregunta &p : banco)
            if (p.numero == 2)
                p.imagen = imagenInventada();   // 40 píxeles de ancho a 96 ppp
        const Modelo m = examenes(banco, 3, 1).first();
        const int n = m.preguntas.size();

        // Letra de 16 pt (la cabecera, a escala), una opción por línea e imágenes más grandes.
        const QString normal = Maquetador::examenHtml(m, convocatoria(), false, 0);
        const QString grande = Maquetador::examenHtml(m, convocatoria(), false, Maquetador::kLetraGrande);
        QVERIFY(normal.contains(QStringLiteral("font-size:10.5pt")) && normal.contains(QStringLiteral("font-size:14.0pt")));
        QVERIFY(grande.contains(QStringLiteral("font-size:16pt")) && grande.contains(QStringLiteral("font-size:21.3pt")));
        QVERIFY(normal.contains(QStringLiteral("width=\"48%\"")));     // opciones cortas: en dos columnas
        QVERIFY(!grande.contains(QStringLiteral("width=\"48%\"")));
        QCOMPARE(grande.count(QStringLiteral("width=\"96%\"")), 4 * n);
        QVERIFY(normal.contains(QStringLiteral("width=\"40\"")));
        QVERIFY(grande.contains(QStringLiteral("width=\"61\"")));      // 40 · 16 / 10,5
        QVERIFY(normal.contains(QStringLiteral("font-style:italic")));   // los enunciados, sin cursiva
        QVERIFY(!grande.contains(QStringLiteral("font-style:italic")));

        // En LaTeX: \LARGE para todo el modelo, más separación y una opción por línea.
        const QString tex = Maquetador::examenesLatex({m}, convocatoria(), false, QStringLiteral("img"), Maquetador::kLetraGrande);
        QVERIFY(tex.contains(QStringLiteral("\\renewcommand{\\letraexamen}{\\LARGE}")));
        QVERIFY(tex.contains(QStringLiteral("\\setlength{\\seppregunta}{14pt}")));
        QVERIFY(tex.contains(QStringLiteral("{\\huge\\bfseries ")));
        QVERIFY(tex.contains(QStringLiteral("\\renewcommand{\\enunciado}[1]{{\\raggedright\\bfseries #1\\par}}")));
        QCOMPARE(tex.count(QStringLiteral("{@{\\sangria}Y@{}}")), n);
        QVERIFY(!tex.contains(QStringLiteral("YY")));

        // Se exporta sin mirar si cabe en una página.
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("grande.pdf"));
        QVERIFY(Exportacion::pdf(ruta, {m}, convocatoria(), false, Maquetador::kLetraGrande));
        QVERIFY(leer(ruta).startsWith("%PDF"));
    }

    void solucionesMarcanLaCorrecta()
    {
        const Modelo m = examenes(bancoInventado(), 3, 1).first();
        QVERIFY(!Maquetador::examenHtml(m, convocatoria(), false).contains(QLatin1String(Maquetador::kColorCorrecta)));
        const QString html = Maquetador::examenHtml(m, convocatoria(), true);
        QCOMPARE(html.count(QStringLiteral("&#10004;")), m.preguntas.size());   // la marca de la correcta
        QVERIFY(html.contains(m.preguntas.first().pregunta.codigoCompleto()));
    }

    void solucionesConExplicacionYReferencia()
    {
        // Debajo de cada pregunta, su explicación y su referencia, si las tiene; solo con soluciones.
        Modelo m = examenes(bancoInventado(), 3, 1).first();
        const QString explicacion = QStringLiteral("Porque así lo dice una explicación inventada & breve.");
        m.preguntas[0].pregunta.explicacion = explicacion;
        m.preguntas[1].pregunta.referencia.clear();
        const QString referencia = m.preguntas[0].pregunta.referencia;
        const QString sin = Maquetador::examenHtml(m, convocatoria(), false);
        QVERIFY(!sin.contains(QStringLiteral("Explicación:")) && !sin.contains(QStringLiteral("Referencia:")));
        QVERIFY(!sin.contains(referencia));
        const QString con = Maquetador::examenHtml(m, convocatoria(), true);
        QVERIFY(con.contains(QStringLiteral("Explicación:</span> ") + Maquetador::escapar(explicacion)));
        QVERIFY(con.contains(QStringLiteral("Referencia:</span> ") + Maquetador::escapar(referencia)));
        QCOMPARE(con.count(QStringLiteral("Explicación:")), 1);
        QCOMPARE(con.count(QStringLiteral("Referencia:")), m.preguntas.size() - 1);
        // La explicación va con su pregunta: después de sus opciones y antes de la siguiente.
        const qsizetype donde = con.indexOf(QStringLiteral("Explicación:"));
        QVERIFY(donde > con.indexOf(QStringLiteral("name=\"q1\"")) && donde < con.indexOf(QStringLiteral("name=\"q2\"")));

        const QString tex = Maquetador::examenesLatex({m}, convocatoria(), true, QStringLiteral("img"));
        QVERIFY(tex.contains(QStringLiteral("\\textcolor{verde}{\\textbf{Explicación:}} ") + Maquetador::escaparLatex(explicacion)));
        QCOMPARE(tex.count(QStringLiteral("\\textbf{Explicación:}")), 1);
        QCOMPARE(tex.count(QStringLiteral("\\textbf{Referencia:}")), m.preguntas.size() - 1);
        QVERIFY(!Maquetador::examenesLatex({m}, convocatoria(), false, QStringLiteral("img")).contains(QStringLiteral("Explicación:")));
    }

    void plantillaDeCorreccion()
    {
        // «1 C», «2 A»…: en la hoja de soluciones (PDF, LaTeX y, ahora también, HTML) y en la vista con soluciones.
        const Modelo m = examenes(bancoInventado(), 3, 1).first();
        const QString titulo = QStringLiteral("Plantilla de corrección: Examen 1");
        const QString hoja = Maquetador::solucionesHtml({m}, convocatoria());
        QVERIFY(hoja.contains(titulo));
        const QString tex = Maquetador::examenesLatex({m}, convocatoria(), true, QStringLiteral("img"));
        QVERIFY(tex.contains(titulo));
        for (int i = 0; i < m.preguntas.size(); ++i) {
            const QString letra(Pregunta::letraMayuscula(m.preguntas.at(i).correcta()));
            QVERIFY(hoja.contains(QStringLiteral(">%1&nbsp;<b>%2</b><").arg(QString::number(i + 1), letra)));
            QVERIFY(tex.contains(QStringLiteral("%1~\\textbf{%2}").arg(QString::number(i + 1), letra)));
        }
        const QString vista = Maquetador::examenHtml(m, convocatoria(), true, 0, true);
        QVERIFY(vista.contains(titulo));
        QVERIFY(vista.indexOf(titulo) < vista.indexOf(QStringLiteral("name=\"q1\"")));
        QVERIFY(!Maquetador::examenHtml(m, convocatoria(), true).contains(titulo));        // en el PDF va en la hoja
        QVERIFY(!Maquetador::examenHtml(m, convocatoria(), false, 0, true).contains(titulo));
        QVERIFY(Maquetador::examenesHtml({m}, convocatoria(), true).contains(titulo));
        QVERIFY(!Maquetador::examenesHtml({m}, convocatoria(), false).contains(titulo));
    }

    void textosDeLaHojaDeSoluciones()
    {
        Modelo m = examenes(bancoInventado(), 3, 1).first();
        const std::array<int, 4> c = m.correctasPorLetra();
        QCOMPARE(Maquetador::textoCorrectasPorLetra(m), QStringLiteral("A %1 · B %2 · C %3 · D %4").arg(c[0]).arg(c[1]).arg(c[2]).arg(c[3]));
        QCOMPARE(Maquetador::formatoPuntos(0.3), QStringLiteral("0,30"));
        QCOMPARE(Maquetador::formatoPuntos(1), QStringLiteral("1,00"));
    }

    void exportarPdf()
    {
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("examen.pdf"));
        QVERIFY(Exportacion::pdf(ruta, examenes(bancoInventado(), 1), convocatoria(), true, 0));
        QVERIFY(leer(ruta).startsWith("%PDF"));
    }

    void exportarHtmlConImagenes()
    {
        QVector<Pregunta> banco = bancoInventado();
        for (Pregunta &p : banco)
            p.imagen = imagenInventada();
        const QVector<Modelo> modelos = examenes(banco, 1);
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("Exámenes de junio.html"));
        QVERIFY(Exportacion::html(ruta, modelos, convocatoria(), false, 0));
        const QString html = QString::fromUtf8(leer(ruta));
        const Pregunta &p = modelos.first().preguntas.first().pregunta;
        const QString imagen = QStringLiteral("Examenes_de_junio_imagenes/") + Maquetador::nombreImagen(p);
        QVERIFY(html.contains(QStringLiteral("src=\"%1\"").arg(imagen)));
        QVERIFY(!html.contains(QStringLiteral("img:")));
        QCOMPARE(leer(dir.filePath(imagen)), p.imagen);
    }

    void exportarLatexConImagenes()
    {
        QVector<Pregunta> banco = bancoInventado();
        for (Pregunta &p : banco)
            p.imagen = imagenInventada();
        const QVector<Modelo> modelos = examenes(banco, 1);
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("junio.tex"));
        QVERIFY(Exportacion::latex(ruta, modelos, convocatoria(), true, 0));
        const QString tex = QString::fromUtf8(leer(ruta));
        QVERIFY(tex.startsWith(QStringLiteral("\\documentclass")));
        QVERIFY(tex.trimmed().endsWith(QStringLiteral("\\end{document}")));
        const QString imagen = QStringLiteral("junio_imagenes/") + Maquetador::nombreImagen(modelos.first().preguntas.first().pregunta);
        QVERIFY(tex.contains(imagen));
        QVERIFY(QFile::exists(dir.filePath(imagen)));
    }

    void latexEscapaLosCaracteresEspeciales()
    {
        QCOMPARE(Maquetador::escaparLatex(QStringLiteral("50% & $5 #1 a_b {x}")),
                 QStringLiteral("50\\% \\& \\$5 \\#1 a\\_b \\{x\\}"));
    }
};

QTEST_MAIN(PruebaMaqueta)
#include "prueba_maqueta.moc"
