// Exámenes hechos fuera del programa: separar su texto en preguntas, reconocerlas en el banco con el orden de sus
// opciones y sacar la convocatoria y el nombre del examen del nombre de su fichero.

#include "datos_prueba.h"
#include "examenesanteriores.h"
#include "exportacion.h"
#include "maquetador.h"

#include <QTemporaryDir>
#include <QTextDocument>
#include <QtTest>
#include <algorithm>

using namespace ExamenesAnteriores;

namespace {

// Un examen con las preguntas del banco variado, cada una con las opciones en otro orden.
Modelo examenVariado()
{
    Modelo m;
    m.nombre = QStringLiteral("Nacional Original");
    const QVector<Pregunta> banco = bancoVariado();
    for (int i = 0; i < banco.size(); ++i) {
        PreguntaExamen pe;
        pe.pregunta = banco.at(i);
        std::rotate(pe.orden.begin(), pe.orden.begin() + i % 4, pe.orden.end());
        m.preguntas << pe;
    }
    return m;
}

DatosConvocatoria febrero()
{
    DatosConvocatoria d;
    d.convocatoria = QStringLiteral("Febrero");
    d.anio = 2026;
    return d;
}

// Que se hayan reconocido, sin dudas, las preguntas del examen y el orden de sus opciones.
void comprobar(const QVector<Reconocida> &reconocidas, const Modelo &m)
{
    QCOMPARE(reconocidas.size(), m.preguntas.size());
    for (int i = 0; i < m.preguntas.size(); ++i) {
        const Reconocida &r = reconocidas.at(i);
        QCOMPARE(r.pregunta.pregunta.codigoCompleto(), m.preguntas.at(i).pregunta.codigoCompleto());
        QVERIFY2(r.pregunta.orden == m.preguntas.at(i).orden, qPrintable(QStringLiteral("pregunta %1").arg(i + 1)));
        QVERIFY2(!r.dudosa, qPrintable(QStringLiteral("pregunta %1: %2").arg(i + 1).arg(r.parecido)));
    }
}

} // namespace

class PruebaAnteriores : public QObject
{
    Q_OBJECT

private slots:
    void separarElTexto()
    {
        const QString texto = QStringLiteral(
            "Fundamentos de Informática · Febrero 2026\n"
            "1. Lea todas las preguntas.\n"
            "1.- Primer enunciado inventado:\n"
            "a) Uno.\nb) Dos.\nc) Tres.\nd) Cuatro.\n"
            "2.- Segundo enunciado inventado, que sigue\n"
            "en otra línea:\n"
            "a) Alfa.        c) Gamma.\n"
            "b) Beta.        d) Delta.\n"
            "Nacional Original · Febrero 2026: 1 / 2\n"
            "3 Tercer enunciado, sin punto ni guion:\n"
            "a) Sí.\r\nb) No.\r\nc) Quizá.\r\nd) Nunca.\r\n"
            "4.- Cuarto enunciado, con f(a) dentro:\n"
            "a) A.\nb) B.\nc) C.\nd) D.\n");
        const QVector<Leida> leidas = separar(texto);
        QCOMPARE(leidas.size(), 4);
        QCOMPARE(leidas.at(0).enunciado, QStringLiteral("Primer enunciado inventado:"));
        QCOMPARE(leidas.at(0).opciones, (QStringList{"Uno.", "Dos.", "Tres.", "Cuatro."}));
        QCOMPARE(leidas.at(1).enunciado, QStringLiteral("Segundo enunciado inventado, que sigue en otra línea:"));
        QCOMPARE(leidas.at(1).opciones, (QStringList{"Alfa.", "Beta.", "Gamma.", "Delta."}));   // sin el pie
        QCOMPARE(leidas.at(2).enunciado, QStringLiteral("Tercer enunciado, sin punto ni guion:"));
        QCOMPARE(leidas.at(2).opciones, (QStringList{"Sí.", "No.", "Quizá.", "Nunca."}));
        QCOMPARE(leidas.at(3).enunciado, QStringLiteral("Cuarto enunciado, con f(a) dentro:"));
        QCOMPARE(leidas.at(3).opciones, (QStringList{"A.", "B.", "C.", "D."}));

        // Sin numeración, ninguna pregunta.
        QVERIFY(separar(QStringLiteral("Texto cualquiera\na) Uno.\nb) Dos.")).isEmpty());
    }

    void reconocerLosExamenesDelPrograma()
    {
        // Con las opciones en una columna, en dos y con letra grande.
        const Modelo m = examenVariado();
        for (int nivel : {0, 1, Maquetador::kLetraGrande}) {
            QTextDocument doc;
            doc.setHtml(Maquetador::examenHtml(m, febrero(), false, nivel));
            comprobar(reconocer(separar(doc.toPlainText()), bancoVariado()), m);
        }
    }

    void reconocerUnPdf()
    {
        if (!puedeLeerPdf())
            QSKIP("Compilado sin Qt PDF");
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("2026_Febrero_Nacional_Original.pdf"));
        const Modelo m = examenVariado();
        QVERIFY(Exportacion::pdf(ruta, {m}, febrero(), false, 0));
        QString error;
        const QString texto = textoDePdf(ruta, &error);
        QVERIFY2(!texto.isEmpty(), qPrintable(error));
        comprobar(reconocer(separar(texto), bancoVariado()), m);

        QVERIFY(textoDePdf(dir.filePath(QStringLiteral("no_existe.pdf")), &error).isEmpty());
        QVERIFY(!error.isEmpty());
    }

    void ordenDeLasOpciones()
    {
        const Pregunta p = bancoVariado().first();
        Leida l;
        l.opciones = {p.opciones.at(2), p.opciones.at(0), p.opciones.at(3), p.opciones.at(1)};
        QVERIFY((ordenDeOpciones(l, p) == std::array<int, 4>{{2, 0, 3, 1}}));
        // Sin opciones, o con otras que no se le parecen, en su orden.
        QVERIFY((ordenDeOpciones(Leida(), p) == std::array<int, 4>{{0, 1, 2, 3}}));
        l.opciones = {QStringLiteral("Nada que ver."), QStringLiteral("Tampoco."), QStringLiteral("Otra cosa."), QStringLiteral("Ni esta.")};
        QVERIFY((ordenDeOpciones(l, p) == std::array<int, 4>{{0, 1, 2, 3}}));
    }

    void reconocerUnaQueNoEstaEsDudosa()
    {
        Leida l;
        l.enunciado = QStringLiteral("Enunciado que no se parece a ninguna pregunta del banco:");
        l.opciones = {QStringLiteral("Una."), QStringLiteral("Otra."), QStringLiteral("La tercera."), QStringLiteral("La última.")};
        const QVector<Reconocida> r = reconocer({l}, bancoVariado());
        QCOMPARE(r.size(), 1);
        QVERIFY(r.first().dudosa);
    }

    void datosDelNombreDelFichero()
    {
        auto datos = [](const QString &ruta, const QStringList &conocidos = {}) {
            const DatosFichero d = datosDeFichero(ruta, conocidos);
            return QStringLiteral("%1 %2 · %3").arg(d.convocatoria).arg(d.anio).arg(d.nombre);
        };
        QCOMPARE(datos(QStringLiteral("/x/2026_02_01_nacional_original_ex_fi.pdf")), QStringLiteral("Febrero 2026 · Nacional Original"));
        QCOMPARE(datos(QStringLiteral("2026_02_05_ae_original_ex_fi.pdf"), {"América Este Original", "América Este Reserva"}),
                 QStringLiteral("Febrero 2026 · América Este Original"));
        QCOMPARE(datos(QStringLiteral("2026_02_03_ue_original_ex_fi.pdf"), {"UE-Original y Guinea", "UE-Reserva y Guinea"}),
                 QStringLiteral("Febrero 2026 · UE-Original y Guinea"));
        QCOMPARE(datos(QStringLiteral("2026_Junio_soluciones_Nacional_Reserva.pdf")), QStringLiteral("Junio 2026 · Nacional Reserva"));
        QCOMPARE(datos(QStringLiteral("2026_Junio_letra_grande_America_Este_Original.pdf"), {"América Este Original"}),
                 QStringLiteral("Junio 2026 · América Este Original"));
        QCOMPARE(datos(QStringLiteral("examen.pdf")), QStringLiteral(" 0 · Examen"));
    }
};

QTEST_MAIN(PruebaAnteriores)
#include "prueba_anteriores.moc"
