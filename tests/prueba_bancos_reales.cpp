// Comprobaciones con los dos bancos de preguntas reales, que no están en git: se saltan si no está el enlace
// «asignatura» al directorio de la asignatura (ver README.md). Se trabaja con una copia de cada base de datos.

#include "basedatos.h"
#include "comprobadorjson.h"
#include "estilo.h"
#include "generador.h"
#include "maquetador.h"

#include <QFile>
#include <QSet>
#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>

namespace {

const QString kBancos = QStringLiteral(RUTA_PROYECTO "/asignatura/");

struct Banco
{
    QVector<Pregunta> preguntas;
    QVector<Tema> temas;
    QString asignatura;
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
        b.repartos = bd.repartos();
    }
    return b;
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
        QTest::newRow("ver11") << QStringLiteral("BancoPreguntas/preguntas.db") << true;
        // El banco 2, pendiente de revisión, tiene preguntas el doble de largas: de él solo se informa.
        QTest::newRow("banco 2") << QStringLiteral("BancoPreguntas2/preguntas_banco2.db") << false;
    }

    void bancos()
    {
        QFETCH(QString, ruta);
        QFETCH(bool, debenCaber);
        if (!QFile::exists(kBancos + ruta))
            QSKIP("No está el banco de preguntas (enlace «asignatura»).");
        const Banco b = cargar(ruta);
        QVERIFY(!b.preguntas.isEmpty());
        // Con el nombre de la asignatura, sus repartos y la forma de la referencia de los temas (los bancos de antes
        // de que los llevasen se ponen al día al abrirlos).
        QVERIFY(!b.asignatura.isEmpty());
        QVERIFY(!b.repartos.isEmpty());
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
        const QString ruta = QStringLiteral("BancoPreguntas/preguntas.db");
        if (!QFile::exists(kBancos + ruta))
            QSKIP("No está el banco de preguntas (enlace «asignatura»).");
        const QStringList erratas{QStringLiteral("wearable"), QStringLiteral("rectas"), QStringLiteral("«¿»"),
                                  QStringLiteral("«?» de cierre"), QStringLiteral("concordancia")};
        const Banco b = cargar(ruta);
        for (const Pregunta &p : b.preguntas)
            for (const Estilo::Aviso &a : Estilo::revisar(p, b.temas))
                for (const QString &e : erratas)
                    QVERIFY2(!a.texto.contains(e), qPrintable(p.codigoCompleto() + QStringLiteral(": ") + a.texto));
    }

    // Los JSON de los dos bancos pasan el comprobador sin errores ni avisos (no los hay de verdad: si sale alguno, es
    // del comprobador, o del JSON). Solo se enseñan la línea y la pregunta, no su texto.
    void jsonComprobados_data()
    {
        QTest::addColumn<QString>("ruta");
        QTest::newRow("ver11") << QStringLiteral("BancoPreguntas/preguntas_fi.json");
        QTest::newRow("banco 2") << QStringLiteral("BancoPreguntas2/preguntas_banco2.json");
    }
    void jsonComprobados()
    {
        QFETCH(QString, ruta);
        if (!QFile::exists(kBancos + ruta))
            QSKIP("No está el banco de preguntas (enlace «asignatura»).");
        const ComprobadorJson::Resultado r = ComprobadorJson::comprobarFichero(kBancos + ruta);
        QStringList problemas;
        for (const ComprobadorJson::Problema &p : r.problemas)
            problemas << QStringLiteral("línea %1, %2").arg(p.linea).arg(p.donde);
        QVERIFY2(r.problemas.isEmpty(), qPrintable(problemas.join(QStringLiteral("; "))));
        QVERIFY(r.preguntas > 300);
    }
};

QTEST_MAIN(PruebaBancosReales)
#include "prueba_bancos_reales.moc"
