// Formato de los enunciados: párrafos, bloques de código ``` y tablas «| a | b |», y cómo los maquetan el HTML y
// el LaTeX.

#include "datos_prueba.h"
#include "enunciado.h"
#include "maquetador.h"
#include "resaltado.h"

#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextTable>
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

// El LaTeX de un examen de una pregunta con ese enunciado.
QString latex(const QString &enunciado)
{
    Modelo m;
    PreguntaExamen pe;
    pe.pregunta = bancoInventado(1, 1).first();
    pe.pregunta.enunciado = enunciado;
    m.preguntas << pe;
    return Maquetador::examenesLatex({m}, DatosConvocatoria(), false, QStringLiteral("img"), 0);
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
        QVERIFY(html.startsWith(QStringLiteral("<p style=\"margin-top:0px; margin-bottom:2px\">1.- </p><table")));
        QVERIFY(html.contains(QStringLiteral("x &lt; 1")));
    }

    void codigoEnUnRecuadro()
    {
        // El código va en un recuadro con fondo gris, en letra de ancho fijo, para que se distinga del texto: en la
        // vista y el PDF (QTextDocument) y en LaTeX.
        const QString texto = QStringLiteral("Texto:\n```\nint x = 1;\n```");
        QTextDocument doc;
        doc.setHtml(Maquetador::enunciadoHtml(texto));
        QTextTable *recuadro = nullptr;
        for (QTextBlock b = doc.begin(); b.isValid(); b = b.next())
            if (b.text() == QLatin1String("int x = 1;")) {
                recuadro = QTextCursor(b).currentTable();
                QVERIFY(b.charFormat().fontFixedPitch() || b.charFormat().fontFamilies().toStringList().join(QLatin1Char(' ')).contains(QLatin1String("Mono")));
            }
        QVERIFY(recuadro);
        QCOMPARE(recuadro->cellAt(0, 0).format().background().color(), QColor(QStringLiteral("#eeeeee")));
        QVERIFY(recuadro->format().border() > 0);

        // En LaTeX, el entorno codigo (listings con marco y fondo): sin lenguaje ni números, con los caracteres de la
        // línea más larga.
        const QString tex = latex(texto);
        QVERIFY(tex.contains(QStringLiteral("\\begin{codigo}[]{10}{0}\nint x = 1;\n\\end{codigo}")));
        QVERIFY(tex.contains(QStringLiteral("backgroundcolor=\\color{fondocodigo}")));
    }

    void lenguajeYNumerosDeLinea()
    {
        // Lo que dice la línea que abre el código: el lenguaje (en minúsculas) y «numerado», en cualquier orden.
        auto primero = [](const QString &texto) { return Enunciado::bloques(texto).first(); };
        const Enunciado::Bloque java = primero(QStringLiteral("```Java numerado\nint x;\n```"));
        QCOMPARE(java.lenguaje, QStringLiteral("java"));
        QVERIFY(java.numerado && java.cerrado && java.sobrantes.isEmpty());
        QCOMPARE(java.lineas, QStringList{QStringLiteral("int x;")});
        const Enunciado::Bloque soloNumeros = primero(QStringLiteral("``` numerado\nx\n```"));
        QVERIFY(soloNumeros.lenguaje.isEmpty() && soloNumeros.numerado);
        const Enunciado::Bloque sinNada = primero(QStringLiteral("```\nx\n```"));
        QVERIFY(sinNada.lenguaje.isEmpty() && !sinNada.numerado && sinNada.cerrado);
        const Enunciado::Bloque sobra = primero(QStringLiteral("```xml numerdo\n<a/>"));
        QCOMPARE(sobra.sobrantes, QStringList{QStringLiteral("numerdo")});
        QVERIFY(!sobra.cerrado);

        // Los avisos: lenguaje que no se resalta, palabras que sobran y bloque sin cerrar; nada si está bien.
        QVERIFY(Enunciado::avisosDelCodigo(QStringLiteral("Texto:\n```java numerado\nint x;\n```\n```xsd\n<a/>\n```")).isEmpty());
        QVERIFY(Enunciado::avisosDelCodigo(QStringLiteral("```texto\nx\n```")).isEmpty());
        const QStringList avisos = Enunciado::avisosDelCodigo(QStringLiteral("```cobol numerdo\nMOVE A TO B."));
        QCOMPARE(avisos.size(), 3);
        QVERIFY2(avisos.at(0).contains(QStringLiteral("«cobol»")) && avisos.at(0).contains(QStringLiteral("idl, java, json")), qPrintable(avisos.at(0)));
        QVERIFY(avisos.at(1).contains(QStringLiteral("sobra «numerdo»")));
        QVERIFY(avisos.at(2).contains(QStringLiteral("no se cierra")));
    }

    void resaltado_data()
    {
        // Cada línea resaltada, con «*palabra*» para las claves, «/comentario/» y «"cadena"» entre ‹›.
        QTest::addColumn<QString>("lenguaje");
        QTest::addColumn<QString>("codigo");
        QTest::addColumn<QString>("esperado");
        QTest::newRow("java") << "java" << "int x = 1; // uno\nString s = \"a\\\"b\";"
                              << "*int* x = 1; /// uno/\nString s = ‹\"a\\\"b\"›;";
        QTest::newRow("comentario de varias líneas") << "c" << "/* a\nb */ return 0;" << "//* a/\n/b *// *return* 0;";
        QTest::newRow("idl") << "idl" << "interface A { attribute short b; }; // c"
                             << "*interface* A { *attribute* *short* b; }; /// c/";
        QTest::newRow("python") << "python" << "def f(): # nada\n    return None" << "*def* f(): /# nada/\n    *return* *None*";
        QTest::newRow("sql sin mayúsculas") << "sql" << "select * FROM t -- todas" << "*select* * *FROM* t /-- todas/";
        QTest::newRow("xml") << "xml" << "<?xml version=\"1.0\"?>\n<a b='c'>texto \"no\"</a> <!-- nota\n-->"
                             << "<?*xml* *version*=‹\"1.0\"›?>\n<*a* b=‹'c'›>texto \"no\"</*a*> /<!-- nota/\n/-->/";
        QTest::newRow("alias xsd") << "xsd" << "<xs:element name=\"a\"/>" << "<*xs:element* name=‹\"a\"›/>";
        QTest::newRow("dtd") << "dtd" << "<!ATTLIST cita hora CDATA #REQUIRED>" << "<!*ATTLIST* cita hora *CDATA* #*REQUIRED*>";
        QTest::newRow("html") << "html" << "<p class=\"x\">Hola</p>" << "<*p* class=‹\"x\"›>Hola</*p*>";
        QTest::newRow("json") << "json" << "{\"a\": true, \"b\": null, \"c\": 1}" << "{‹\"a\"›: *true*, ‹\"b\"›: *null*, ‹\"c\"›: 1}";
        QTest::newRow("yaml") << "yml" << "activo: yes # sí" << "activo: *yes* /# sí/";
        QTest::newRow("sin lenguaje") << "" << "int x; // no" << "int x; // no";
        QTest::newRow("desconocido") << "cobol" << "int x;" << "int x;";
    }

    void resaltado()
    {
        QFETCH(QString, lenguaje);
        QFETCH(QString, codigo);
        QFETCH(QString, esperado);
        QStringList lineas;
        for (const QVector<Resaltado::Trozo> &linea : Resaltado::resaltar(codigo.split(QLatin1Char('\n')), lenguaje)) {
            QString texto;
            for (const Resaltado::Trozo &t : linea)
                texto += t.estilo == Resaltado::Clave        ? QStringLiteral("*%1*").arg(t.texto)
                         : t.estilo == Resaltado::Comentario ? QStringLiteral("/%1/").arg(t.texto)
                         : t.estilo == Resaltado::Cadena     ? QStringLiteral("‹%1›").arg(t.texto)
                                                             : t.texto;
            lineas << texto;
        }
        QCOMPARE(lineas.join(QLatin1Char('\n')), esperado);
    }

    void codigoResaltadoYNumerado()
    {
        // HTML: las palabras clave en negrita y color; con «numerado», una fila por línea con su número.
        const QString texto = QStringLiteral("Texto:\n```java numerado\nint x = 1;\n\nreturn x;\n```");
        const QString html = Maquetador::enunciadoHtml(texto);
        QVERIFY(html.contains(QStringLiteral("<span style=\"font-weight:bold; color:#1F3A93\">int</span> x = 1;")));
        QVERIFY(html.contains(QStringLiteral(">1</pre></td>")) && html.contains(QStringLiteral(">3</pre></td>")));
        QVERIFY(html.contains(QStringLiteral("white-space:pre-wrap")));   // las líneas largas se parten
        QVERIFY(!Maquetador::enunciadoHtml(QStringLiteral("```java\nint x;\n```")).contains(QStringLiteral(">1</pre></td>")));

        // LaTeX: el lenguaje de listings, los números y su margen (2 cifras más el espacio en 10 líneas); los caracteres
        // que no son ASCII, como texto LaTeX, salvo las letras con tilde o eñe; los tabuladores, cuatro espacios.
        QString diez = QStringLiteral("```xml numerado\n");
        for (int i = 0; i < 10; ++i)
            diez += QStringLiteral("<a>%1</a>\n").arg(i);
        diez += QStringLiteral("```");
        QVERIFY(latex(diez).contains(QStringLiteral("\\begin{codigo}[language=XML,markfirstintag=true,numbers=left,numbersep=\\anchocaracter]{8}{3}\n<a>0</a>")));
        QVERIFY(latex(QStringLiteral("```dtd\n<!ELEMENT a (#PCDATA)>\n```")).contains(QStringLiteral("\\begin{codigo}[language=XML,markfirstintag=true]{22}{0}")));
        QVERIFY(latex(QStringLiteral("```yaml\nx: 1\n```")).contains(QStringLiteral("\\begin{codigo}[language=YAML]{4}{0}")));
        QVERIFY(latex(QStringLiteral("```idl\nmodule A {};\n```")).contains(QStringLiteral("\\begin{codigo}[language={[CORBA]IDL}]{12}{0}")));
        QVERIFY(latex(QStringLiteral("```cobol\nx\n```")).contains(QStringLiteral("\\begin{codigo}[]{1}{0}")));
        const QString acentos = latex(QStringLiteral("```\naño → b\tc\n```"));
        QVERIFY2(acentos.contains(QStringLiteral("\\begin{codigo}[]{13}{0}\naño (*@→@*) b    c\n")), qPrintable(acentos));
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
