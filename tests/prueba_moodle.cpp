// Exportación del banco a cuestionarios de Moodle (XML).

#include "datos_prueba.h"
#include "moodle.h"

#include <QTemporaryDir>
#include <QXmlStreamReader>
#include <QtTest>

class PruebaMoodle : public QObject
{
    Q_OBJECT

private:
    // Banco de 3 temas con una pregunta anulada, otra obsoleta, dos para revisar, una con imagen, otra con código y
    // tabla, otra con explicación, y la T2-6 con «Todas las anteriores».
    static QVector<Pregunta> banco()
    {
        QVector<Pregunta> banco = bancoInventado(3, 8);
        for (Pregunta &p : banco) {
            const QString codigo = p.codigoCompleto();
            if (codigo == QLatin1String("T1-1"))
                p.estado = QStringLiteral("anulada");
            else if (codigo == QLatin1String("T1-2"))
                p.estado = QStringLiteral("obsoleta");
            else if (codigo == QLatin1String("T1-3") || codigo == QLatin1String("T3-3"))
                p.revisar = true;
            else if (codigo == QLatin1String("T2-1"))
                p.imagen = imagenInventada();
            else if (codigo == QLatin1String("T2-2"))
                p.enunciado = QStringLiteral("Este programa:\n```\nint x = 1 << 2;\n```\ny esta tabla:\n| a | b |\nDan:");
            else if (codigo == QLatin1String("T2-3"))
                p.enunciado = QStringLiteral("Este código:\n```java numerado\nint x = 1; // uno\n```\nHace:");
            else if (codigo == QLatin1String("T3-1"))
                p.explicacion = QStringLiteral("Porque sí & punto.");
        }
        return banco;
    }

private slots:
    void seleccionDePreguntas()
    {
        Moodle::Opciones o;
        QCOMPARE(Moodle::seleccion(banco(), o).size(), 34);   // 36 menos la anulada y la obsoleta
        o.incluirParaRevisar = false;
        QCOMPARE(Moodle::seleccion(banco(), o).size(), 32);
    }

    void xmlBienFormadoYCompleto()
    {
        Moodle::Opciones o;
        o.asignatura = QStringLiteral("Asignatura inventada");
        const QString xml = Moodle::xml(temasInventados(3), banco(), o);
        QVERIFY(xml.startsWith(QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")));

        QXmlStreamReader r(xml);
        int categorias = 0, preguntas = 0, correctas = 0, erroneas = 0, barajan = 0;
        QStringList nombres;
        while (!r.atEnd()) {
            r.readNext();
            if (!r.isStartElement())
                continue;
            if (r.name() == QLatin1String("question")) {
                const QString tipo = r.attributes().value(QStringLiteral("type")).toString();
                if (tipo == QLatin1String("category"))
                    ++categorias;
                else if (tipo == QLatin1String("multichoice"))
                    ++preguntas;
            } else if (r.name() == QLatin1String("answer")) {
                const QString fraccion = r.attributes().value(QStringLiteral("fraction")).toString();
                if (fraccion == QLatin1String("100"))
                    ++correctas;
                else if (fraccion == QLatin1String("-30"))
                    ++erroneas;
            } else if (r.name() == QLatin1String("shuffleanswers")) {
                if (r.readElementText() == QLatin1String("true"))
                    ++barajan;
            } else if (r.name() == QLatin1String("name")) {
                nombres << r.readElementText(QXmlStreamReader::IncludeChildElements).trimmed();
            }
        }
        QVERIFY2(!r.hasError(), qPrintable(r.errorString()));
        QCOMPARE(categorias, 3);
        QCOMPARE(preguntas, 34);
        QCOMPARE(correctas, 34);
        QCOMPARE(erroneas, 34 * 3);
        QCOMPARE(barajan, 33);   // la T2-6 («Todas las anteriores.») no se baraja
        QCOMPARE(nombres.size(), 34);
        QVERIFY2(nombres.first().startsWith(QStringLiteral("T1-3: Enunciado inventado de la pregunta T1-3")), qPrintable(nombres.first()));
        QVERIFY(xml.contains(QStringLiteral("<text>$course$/top/Asignatura inventada/Tema 2: Tema inventado 2</text>")));

        // Imagen incrustada; código y tabla en HTML; la referencia y la explicación (escapada) como realimentación.
        QVERIFY(xml.contains(QStringLiteral("<img src=\"@@PLUGINFILE@@/T2-1.png\"")));
        QVERIFY(xml.contains(QStringLiteral("<file name=\"T2-1.png\" path=\"/\" encoding=\"base64\">")
                             + QString::fromLatin1(imagenInventada().toBase64())));
        QVERIFY(xml.contains(QStringLiteral("<pre style=\"background-color:#eeeeee; border:1px solid #808080; padding:4px\">int x = 1 &lt;&lt; 2;</pre>")));
        // Con lenguaje, resaltado; con «numerado», el número de cada línea delante, en gris.
        QVERIFY2(xml.contains(QStringLiteral("padding:4px\"&gt;&lt;span style=\"color:#808080\"&gt;1&lt;/span&gt;  &lt;span "
                                             "style=\"font-weight:bold; color:#1F3A93\"&gt;int&lt;/span&gt; x = 1; &lt;span "
                                             "style=\"font-style:italic; color:#666666\"&gt;// uno&lt;/span&gt;&lt;/pre&gt;"))
                     || xml.contains(QStringLiteral("padding:4px\"><span style=\"color:#808080\">1</span>  <span "
                                                    "style=\"font-weight:bold; color:#1F3A93\">int</span> x = 1; <span "
                                                    "style=\"font-style:italic; color:#666666\">// uno</span></pre>")),
                 qPrintable(xml.mid(xml.indexOf(QStringLiteral("Este código")), 600)));
        QVERIFY(xml.contains(QStringLiteral("<tr><td>a</td><td>b</td></tr>")));
        QVERIFY(xml.contains(QStringLiteral("<p>Véase la página 31 del libro de teoría.</p><p>Porque sí &amp; punto.</p>")));

        // Sin penalizar los fallos, las erróneas valen 0; con una asignatura con barra, la barra se dobla.
        o.penalizarFallos = false;
        o.asignatura = QStringLiteral("FI / Acceso");
        const QString sinPenalizar = Moodle::xml(temasInventados(3), banco(), o);
        QCOMPARE(sinPenalizar.count(QStringLiteral("fraction=\"0\"")), 34 * 3);
        QVERIFY(sinPenalizar.contains(QStringLiteral("$course$/top/FI // Acceso/Tema 1:")));

        // Sin nombre de la asignatura, los temas van directamente en la categoría del curso.
        o.asignatura.clear();
        QVERIFY(Moodle::xml(temasInventados(3), banco(), o).contains(QStringLiteral("<text>$course$/top/Tema 1: Tema inventado 1</text>")));
    }

    void escribeFichero()
    {
        QTemporaryDir dir;
        QString error;
        const QString ruta = dir.filePath(QStringLiteral("banco_moodle.xml"));
        Moodle::Opciones o;
        o.asignatura = QStringLiteral("Asignatura inventada de Informática");
        QVERIFY2(Moodle::exportar(ruta, temasInventados(3), banco(), o, &error), qPrintable(error));
        QFile f(ruta);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray contenido = f.readAll();
        QVERIFY(contenido.startsWith("<?xml"));
        QVERIFY(contenido.contains("Asignatura inventada de Inform\xc3\xa1tica"));   // UTF-8
        QVERIFY(!Moodle::exportar(dir.filePath(QStringLiteral("no_existe/x.xml")), temasInventados(3), banco(), Moodle::Opciones(), &error));
        QVERIFY(!error.isEmpty());
    }
};

QTEST_MAIN(PruebaMoodle)
#include "prueba_moodle.moc"
