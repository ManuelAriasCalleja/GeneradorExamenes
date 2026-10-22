// Formato de los enunciados: párrafos, bloques de código ``` y tablas «| a | b |», y cómo los maquetan el HTML y
// el LaTeX.

#include "datos_prueba.h"
#include "enunciado.h"
#include "maquetador.h"

#include <QtTest>

namespace {

// «P[línea|línea] C[código] T[a,b|c,d]»: los bloques en texto, para comparar.
QString describir(const QVector<Enunciado::Bloque> &bloques)
{
    QStringList partes;
    for (const Enunciado::Bloque &b : bloques) {
        switch (b.tipo) {
        case Enunciado::Bloque::Parrafo:
            partes << QStringLiteral("P[%1]").arg(b.lineas.join(QLatin1Char('|')));
            break;
        case Enunciado::Bloque::Codigo:
            partes << QStringLiteral("C[%1]").arg(b.lineas.join(QLatin1Char('|')));
            break;
        case Enunciado::Bloque::Tabla: {
            QStringList filas;
            for (const QStringList &f : b.filas)
                filas << f.join(QLatin1Char(','));
            partes << QStringLiteral("T[%1]").arg(filas.join(QLatin1Char('|')));
            break;
        }
        }
    }
    return partes.join(QLatin1Char(' '));
}

} // namespace

class PruebaEnunciado : public QObject
{
    Q_OBJECT

private slots:
    void bloques_data()
    {
        QTest::addColumn<QString>("texto");
        QTest::addColumn<QString>("bloques");
        QTest::newRow("una línea") << "Texto:" << "P[Texto:]";
        QTest::newRow("dos líneas") << "Línea 1\nLínea 2:" << "P[Línea 1|Línea 2:]";
        QTest::newRow("línea en blanco") << "Intro:\n\nMás:" << "P[Intro:||Más:]";
        QTest::newRow("código") << "Código:\n```\nint a;\n  a = 1;\n```\nQué hace:" << "P[Código:] C[int a;|  a = 1;] P[Qué hace:]";
        QTest::newRow("dos bloques de código") << "```\na\n```\n```\nb\n```" << "C[a] C[b]";
        QTest::newRow("código vacío") << "```\n```" << "C[]";
        QTest::newRow("código sin cerrar") << "Texto\n```\nsin cerrar" << "P[Texto] C[sin cerrar]";
        QTest::newRow("tabla") << "Tabla:\n| a | b |\n| c | d |\nDespués:" << "P[Tabla:] T[a,b|c,d] P[Después:]";
        QTest::newRow("tabla al principio, sin barra final") << "| a | b |\n|c|d" << "T[a,b|c,d]";
        QTest::newRow("línea en blanco entre filas") << "| a |\n\n| b |" << "T[a] P[] T[b]";
        QTest::newRow("vacío") << "" << "P[]";
    }

    void bloques()
    {
        QFETCH(QString, texto);
        QFETCH(QString, bloques);
        QCOMPARE(describir(Enunciado::bloques(texto)), bloques);
    }

    void prefijoEnElPrimerParrafo()
    {
        const QString html = Maquetador::enunciadoHtml(QStringLiteral("Texto:"), QStringLiteral("1.- "));
        QCOMPARE(html, QStringLiteral("<p style=\"margin-top:0px; margin-bottom:2px\">1.- Texto:</p>"));
    }

    void prefijoSoloSiEmpiezaPorCodigo()
    {
        const QString html = Maquetador::enunciadoHtml(QStringLiteral("```\nx < 1\n```"), QStringLiteral("1.- "));
        QVERIFY(html.startsWith(QStringLiteral("<p style=\"margin-top:0px; margin-bottom:2px\">1.- </p><pre")));
        QVERIFY(html.contains(QStringLiteral("x &lt; 1")));
    }

    void tablaEnHtml()
    {
        const QString html = Maquetador::enunciadoHtml(QStringLiteral("| a | b & c |"));
        QVERIFY(html.contains(QStringLiteral("<td style=\"border:1px solid #000\">b &amp; c</td>")));
    }

    void latexSinLineasVacias()
    {
        Modelo m;
        PreguntaExamen pe;
        pe.pregunta = bancoInventado(1, 1).first();
        pe.pregunta.enunciado = QStringLiteral("A:\n\nB:");
        m.preguntas << pe;
        const QString tex = Maquetador::examenesLatex({m}, DatosConvocatoria(), false, QStringLiteral("img"), 0);
        QVERIFY(tex.contains(QStringLiteral("\\enunciado{1.- A:\\\\\nB:}")));
    }
};

QTEST_MAIN(PruebaEnunciado)
#include "prueba_enunciado.moc"
