// Comprobación de las pautas de estilo de las preguntas y detección de opciones que remiten a las demás.

#include "datos_prueba.h"
#include "estilo.h"

#include <QtTest>

namespace {

// Pregunta inventada que cumple todas las pautas; cada caso cambia una cosa.
Pregunta buena(int tema = 1)
{
    Pregunta p;
    p.temaNumero = tema;
    p.numero = 1;
    p.enunciado = QStringLiteral("El componente inventado de prueba:");
    p.opciones = {QStringLiteral("Primera opción."), QStringLiteral("Segunda opción."), QStringLiteral("Tercera opción."),
                  QStringLiteral("Cuarta opción.")};
    p.correcta = 2;
    const bool vease = tema == 1 || tema == 3 || tema == 6 || tema == 8;
    p.referencia = vease ? QStringLiteral("Véase la página 10 (sección 1.2) del libro de teoría.")
                         : QStringLiteral("Pág. 10 libro (subsección Algo inventado 2.3.1).");
    return p;
}

Pregunta conEnunciado(const QString &enunciado)
{
    Pregunta p = buena();
    p.enunciado = enunciado;
    return p;
}

Pregunta conOpcion(int i, const QString &opcion)
{
    Pregunta p = buena();
    p.opciones[i] = opcion;
    return p;
}

Pregunta conReferencia(int tema, const QString &referencia)
{
    Pregunta p = buena(tema);
    p.referencia = referencia;
    return p;
}

} // namespace

class PruebaEstilo : public QObject
{
    Q_OBJECT

private slots:
    void revisar_data()
    {
        QTest::addColumn<Pregunta>("pregunta");
        QTest::addColumn<QString>("aviso");   // un trozo del aviso esperado; vacío si no debe haber ninguno

        QTest::newRow("correcta, tema 1") << buena(1) << QString();
        QTest::newRow("correcta, tema 2") << buena(2) << QString();
        QTest::newRow("pregunta con ¿?:") << conEnunciado("¿Cuál es el componente inventado?:") << QString();
        QTest::newRow("pregunta sin dos puntos") << conEnunciado("¿Cuál es el componente inventado?") << "«?:»";
        QTest::newRow("acaba en punto") << conEnunciado("El componente inventado.") << "no en punto";
        QTest::newRow("sin dos puntos") << conEnunciado("El componente inventado") << "acabar en dos puntos";
        QTest::newRow("falta ¿") << conEnunciado("Cuál es el componente inventado?:") << "«¿»";
        QTest::newRow("falta ?") << conEnunciado("¿Cuál es el componente inventado:") << "«?» de cierre";
        QTest::newRow("? de una URL") << conEnunciado("En la dirección https://ejemplo.es/a?b=1, la parte inventada:") << QString();
        QTest::newRow("largo") << conEnunciado(QString("El componente inventado ").repeated(9) + ":") << "largo";
        QTest::newRow("negación en minúscula") << conEnunciado("¿Cuál no es un componente inventado?:") << "mayúsculas";
        QTest::newRow("negación en mayúsculas") << conEnunciado("¿Cuál NO es un componente inventado?:") << QString();
        QTest::newRow("«es falso» en minúscula") << conEnunciado("Sobre lo inventado, es falso que:") << "mayúsculas";
        QTest::newRow("comillas rectas en el texto") << conEnunciado("Si se escribe \"hola\" en lo inventado:") << "tipográficas";
        QTest::newRow("comillas rectas en una fórmula") << conEnunciado("En la celda se escribe =A1&\"x\", y el resultado:") << QString();
        QTest::newRow("comillas tipográficas en el código")
            << conEnunciado("Este programa en C:\n```\nprintf(“hola”);\n```") << "rectas";
        QTest::newRow("código con comillas rectas")
            << conEnunciado("Este programa en C:\n```\nprintf(\"hola\");\n```") << QString();
        QTest::newRow("enunciado en minúscula") << conEnunciado("el componente inventado:") << "empezar por mayúscula";
        QTest::newRow("www no es minúscula") << conEnunciado("www significa en lo inventado:") << QString();
        QTest::newRow("errata conocida") << conEnunciado("La tecnología weareable inventada:") << "wearable";
        QTest::newRow("opción sin punto") << conOpcion(1, "Segunda opción") << "b) debe terminar en punto";
        QTest::newRow("opción en minúscula") << conOpcion(0, "primera opción.") << "a) debe empezar por mayúscula";
        QTest::newRow("comando de una palabra") << conOpcion(0, "ipconfig.") << QString();
        QTest::newRow("variable de una letra") << conOpcion(0, "b al cuadrado.") << QString();
        QTest::newRow("marca con mayúscula dentro") << conOpcion(0, "iOS está basado en algo.") << QString();
        QTest::newRow("opción dependiente") << conOpcion(3, "Todas las anteriores valen.") << "remite a las demás";
        QTest::newRow("«Todas las celdas» no es dependiente") << conOpcion(3, "Todas las celdas deben ser iguales.") << QString();
        QTest::newRow("opciones repetidas") << conOpcion(1, "Primera opción.") << "a) y b) son iguales";
        QTest::newRow("correcta mucho más larga") << conOpcion(2, "Tercera opción, que es mucho más larga y matizada que las otras.") << "delatarla";
        QTest::newRow("incorrecta larga") << conOpcion(0, "Una opción incorrecta mucho más larga que todas las demás opciones.") << QString();
        QTest::newRow("HTML en una opción") << conOpcion(1, "<p style=\"x\">.") << QString();
        QTest::newRow("sin referencia") << conReferencia(1, "") << "Falta la referencia";
        QTest::newRow("referencia sin punto") << conReferencia(1, "Véase la página 10 del libro de teoría") << "terminar en punto";
        QTest::newRow("Pág sin punto") << conReferencia(2, "Pág 10 libro (subsección 2.3.1).") << "«Pág.» se escribe con punto";
        QTest::newRow("sin número de página") << conReferencia(1, "Véase la página (sección 1.2) del libro de teoría.") << "número de página";
        QTest::newRow("Véase las páginas") << conReferencia(1, "Véase las páginas 10 y 11 del libro de teoría.") << "concordancia";
        QTest::newRow("Véase la página N y M") << conReferencia(1, "Véase la página 10 y 11 del libro de teoría.") << "concordancia";
        QTest::newRow("Véanse las páginas N-M") << conReferencia(1, "Véanse las páginas 10-11 del libro de teoría.") << QString();
        QTest::newRow("lista de páginas") << conReferencia(3, "Véanse las páginas 10, 12 y 11 del libro de teoría.") << QString();
        QTest::newRow("y siguientes") << conReferencia(3, "Véase la página 10 y siguientes del libro de teoría.") << QString();
        QTest::newRow("Págs. N-M") << conReferencia(2, "Págs. 10-11 libro (subsección 2.3.1).") << QString();
        QTest::newRow("Págs. lista") << conReferencia(2, "Págs. 10, 12 y 14 libro (subsección 2.3.1).") << QString();
        QTest::newRow("formato del otro grupo, tema 4") << conReferencia(4, "Véase la página 10 del libro de teoría.") << "En el tema 4";
        QTest::newRow("formato del otro grupo, tema 1") << conReferencia(1, "Pág. 10 libro (subsección 1.2).") << "En el tema 1";
        QTest::newRow("sin «del libro de teoría»") << conReferencia(1, "Véase la página 10.") << "forma habitual";
        QTest::newRow("sin «subsección»") << conReferencia(2, "Pág. 10 libro (2.3.1).") << "forma habitual";
    }

    void revisar()
    {
        QFETCH(Pregunta, pregunta);
        QFETCH(QString, aviso);
        QStringList avisos;
        for (const Estilo::Aviso &a : Estilo::revisar(pregunta))
            avisos << a.texto;
        if (aviso.isEmpty())
            QVERIFY2(avisos.isEmpty(), qPrintable(avisos.join(QStringLiteral(" | "))));
        else
            QVERIFY2(avisos.join(QStringLiteral(" | ")).contains(aviso), qPrintable(avisos.join(QStringLiteral(" | "))));
    }

    void opcionDependiente_data()
    {
        QTest::addColumn<QString>("opcion");
        QTest::addColumn<bool>("dependiente");
        // Una por cada forma que reconoce el detector.
        for (const char *o : {"Todas las anteriores.", "Ninguna de las opciones anteriores.", "Para todos los anteriores.",
                              "Ninguno de los anteriores.", "Todas las respuestas anteriores valen.",
                              "Ninguna de las respuestas anteriores es válida.", "Las dos anteriores.", "Ambas son verdaderas.",
                              "Todos son correctos.", "A y C son correctas.", "a y b.", "a) y c).", "Las opciones a y b.",
                              "En cualquiera de los casos anteriores.", "Ninguna."})
            QTest::newRow(o) << QString::fromUtf8(o) << true;
        // Empiezan como ellas, pero no remiten a las demás opciones.
        for (const char *o : {"Los dos colores básicos, rojo y verde.", "Las dos impresoras son de inyección.", "A ninguna de las tres redes.",
                              "Falso, porque basta con que una sea falsa.", "Cierto, porque ambas cosas pasan a la vez.",
                              "Todas las celdas deben tener el mismo formato.",
                              "Solo se usaba en máquinas anteriores a los transistores.", "A y B son dos letras.",
                              "Viajan en ambos sentidos a la vez.", "Que ninguno de los campos quede vacío."})
            QTest::newRow(o) << QString::fromUtf8(o) << false;
    }

    void opcionDependiente()
    {
        QFETCH(QString, opcion);
        QFETCH(bool, dependiente);
        QCOMPARE(Estilo::opcionDependiente(opcion), dependiente);
    }

    void bancoInventadoSinAvisos()
    {
        // El banco de las demás pruebas cumple las pautas, salvo la opción «Todas las anteriores» de la T2-6.
        for (const Pregunta &p : bancoInventado()) {
            const QVector<Estilo::Aviso> avisos = Estilo::revisar(p);
            if (p.grupo() == QLatin1String("T2-6"))
                QCOMPARE(avisos.size(), 1);
            else
                QVERIFY2(avisos.isEmpty(), qPrintable(p.codigoCompleto() + QStringLiteral(": ") + avisos.value(0).texto));
        }
    }
};

QTEST_MAIN(PruebaEstilo)
#include "prueba_estilo.moc"
