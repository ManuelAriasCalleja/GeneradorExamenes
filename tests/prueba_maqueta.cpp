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

    void imagenAlLadoDeLasOpciones()
    {
        // Una imagen de 'ancho' × 'alto' píxeles a 96 ppp, con cuatro opciones como 'opcion'.
        auto modelo = [](int ancho, int alto, const QString &opcion) {
            QImage img(ancho, alto, QImage::Format_RGB32);
            img.fill(Qt::blue);
            img.setDotsPerMeterX(3780);
            img.setDotsPerMeterY(3780);
            QByteArray datos;
            QBuffer buffer(&datos);
            buffer.open(QIODevice::WriteOnly);
            img.save(&buffer, "PNG");
            Modelo m;
            PreguntaExamen pe;
            pe.pregunta = bancoInventado(1, 1).first();
            pe.pregunta.imagen = datos;
            for (int i = 0; i < 4; ++i)
                pe.pregunta.opciones[i] = opcion.arg(i + 1);
            m.preguntas << pe;
            return m;
        };
        auto alLado = [](const Modelo &m, int nivel, bool soluciones = false) {
            const QString html = Maquetador::examenHtml(m, convocatoria(), soluciones, nivel);
            const QString tex = Maquetador::examenesLatex({m}, convocatoria(), soluciones, QStringLiteral("img"), nivel);
            const bool htmlAlLado = html.contains(QStringLiteral("<td valign=\"top\"><img src=\"img:"));
            const bool texAlLado = tex.contains(QStringLiteral("\\raisebox{\\dimexpr\\ht\\strutbox-\\height}{\\includegraphics"));
            // Una cosa o la otra, igual en los dos formatos.
            if (htmlAlLado != texAlLado || htmlAlLado == html.contains(QStringLiteral("margin-bottom:4px\"><img src=\"img:")))
                return -1;
            return htmlAlLado ? 1 : 0;
        };
        // Estrecha y con opciones cortas: al lado, también con soluciones, y en la maqueta compacta.
        const Modelo estrecha = modelo(80, 110, QStringLiteral("Opción corta %1."));
        QCOMPARE(alLado(estrecha, 0), 1);
        QCOMPARE(alLado(estrecha, 0, true), 1);
        QCOMPARE(alLado(estrecha, Maquetador::numNiveles() - 1), 1);
        QVERIFY(Maquetador::examenHtml(estrecha, convocatoria(), false).contains(QStringLiteral("<img src=\"img:1\" width=\"80\"></td>")));
        // Con letra grande, siempre debajo.
        QCOMPARE(alLado(estrecha, Maquetador::kLetraGrande), 0);
        // Con alguna opción que no cabe en una línea al lado de la imagen, debajo.
        Modelo larga = estrecha;
        larga.preguntas[0].pregunta.opciones[2] = QStringLiteral("Una opción inventada bastante larga, que al lado de la imagen ya no cabría.");
        QCOMPARE(alLado(larga, 0), 0);
        // Ancha, pero con opciones muy cortas: al lado, si caben.
        QCOMPARE(alLado(modelo(450, 120, QStringLiteral("Corta %1.")), 0), 1);
        QCOMPARE(alLado(modelo(560, 120, QStringLiteral("Opción corta %1.")), 0), 0);
        // Pequeña y con opciones que van en una fila: al lado ocuparía más (cuatro líneas), así que debajo.
        QCOMPARE(alLado(modelo(40, 20, QStringLiteral("Uno %1.")), 0), 0);
    }

    void letraGrande()
    {
        QVector<Pregunta> banco = bancoInventado();
        for (Pregunta &p : banco)
            if (p.numero == 2)
                p.imagen = imagenInventada();   // 40 píxeles de ancho a 96 ppp
        const Modelo m = examenes(banco, 3, 1).first();
        const int n = m.preguntas.size();

        // Letra de 16 pt (la cabecera, a escala: la asignatura, de 14 pt, pasa a 21,3), una opción por línea e imágenes
        // más grandes.
        DatosConvocatoria d = convocatoria();
        d.asignatura = QStringLiteral("Asignatura inventada");
        const QString normal = Maquetador::examenHtml(m, d, false, 0);
        const QString grande = Maquetador::examenHtml(m, d, false, Maquetador::kLetraGrande);
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
        const QString tex = Maquetador::examenesLatex({m}, d, false, QStringLiteral("img"), Maquetador::kLetraGrande);
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

        // Algo menor que el examen (un punto; en LaTeX, un tamaño), salvo con letra grande: igual en el PDF y en LaTeX.
        auto letra = [&m](int nivel) {
            const QString html = Maquetador::examenHtml(m, convocatoria(), true, nivel);
            const qsizetype donde = html.lastIndexOf(QStringLiteral("font-size:"), html.indexOf(QStringLiteral("Explicación:")));
            return html.mid(donde, html.indexOf(QLatin1Char('"'), donde) - donde);
        };
        auto letraLatex = [&m](int nivel) {
            const QString tex = Maquetador::examenesLatex({m}, convocatoria(), true, QStringLiteral("img"), nivel);
            const qsizetype donde = tex.lastIndexOf(QStringLiteral("\\vspace{2pt}{"), tex.indexOf(QStringLiteral("\\textbf{Explicación:}")));
            return tex.mid(donde + 13, tex.indexOf(QStringLiteral("\\leftskip"), donde) - donde - 13);
        };
        QCOMPARE(letra(0), QStringLiteral("font-size:9.5pt"));
        QCOMPARE(letra(Maquetador::numNiveles() - 1), QStringLiteral("font-size:8.0pt"));
        QCOMPARE(letra(Maquetador::kLetraGrande), QStringLiteral("font-size:16.0pt"));
        QCOMPARE(letraLatex(0), QStringLiteral("\\small"));
        QCOMPARE(letraLatex(Maquetador::numNiveles() - 1), QStringLiteral("\\footnotesize"));
        QCOMPARE(letraLatex(Maquetador::kLetraGrande), QString());
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
        // La estructura: a su ancho si cabe; si no, al de la página, partiendo las celdas (antes se salía por la
        // derecha con muchas preguntas). Con los paquetes básicos de LaTeX: tabularx sí, tabulary (de los «extra») no.
        QVERIFY(tex.contains(QStringLiteral("\\sbox{\\cajaestructura}{\\begin{tabular}{|l|l|l|l|l|l|l|l|l|}")));
        QVERIFY(tex.contains(QStringLiteral("\\ifdim\\wd\\cajaestructura>\\linewidth\n\\begin{tabularx}{\\linewidth}{|l|Y|Y|Y|Y|Y|Y|Y|Y|}")));
        QVERIFY(!tex.contains(QStringLiteral("tabulary")));
        // El fin de la hoja de soluciones, en un párrafo propio: detrás de la tabla de las referencias, que ocupa todo el
        // ancho, podía pasar a una página más, vacía salvo el pie.
        QVERIFY(tex.contains(QStringLiteral("\\end{tabularx}}\n\\par\\label{fin:soluciones}")));
    }

    void cabeceraSinAsignatura()
    {
        // Un banco sin asignatura: la cabecera empieza por la convocatoria, sin una línea vacía delante (en LaTeX, un
        // «\\» sin nada delante es un error de pdflatex).
        const Modelo m = examenes(bancoInventado(), 3, 1).first();
        DatosConvocatoria d = convocatoria();
        d.asignatura.clear();
        const QString tex = Maquetador::examenesLatex({m}, d, true, QStringLiteral("img"));
        QVERIFY(!tex.contains(QStringLiteral("\\bfseries }")));
        QCOMPARE(tex.count(QStringLiteral("\\begin{minipage}[t]{0.58\\linewidth}\nJunio 2026\n\\end{minipage}")), 2);   // hoja y examen
        QVERIFY(Maquetador::examenHtml(m, d, false).contains(QStringLiteral("<td>Junio 2026</td>")));
        d.asignatura = QStringLiteral("Asignatura inventada");
        QVERIFY(Maquetador::examenesLatex({m}, d, false, QStringLiteral("img")).contains(QStringLiteral("{\\Large\\bfseries Asignatura inventada}\\\\\nJunio 2026\n")));
        QVERIFY(Maquetador::examenHtml(m, d, false).contains(QStringLiteral("Asignatura inventada</span><br>Junio 2026</td>")));
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
