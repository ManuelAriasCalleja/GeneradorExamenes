// Reglas del generador de exámenes: reparto por tema, variantes y pseudorrepetidas, repetición entre exámenes,
// orden, reparto de la respuesta correcta y preferencia por las preguntas no usadas antes.

#include "datos_prueba.h"
#include "estilo.h"
#include "generador.h"

#include <QSet>
#include <QtTest>

namespace {

ConfigExamen configuracion(quint32 semilla)
{
    ConfigExamen c;
    c.preguntasPorTema = {{1, 3}, {2, 2}, {3, 3}, {4, 2}, {5, 2}, {6, 3}, {7, 2}, {8, 3}};
    c.nombresModelos = {QStringLiteral("Uno"), QStringLiteral("Dos"), QStringLiteral("Tres")};
    c.semilla = semilla;
    return c;
}

// Preguntas y orden de las opciones de todos los exámenes, para comparar dos generaciones.
QString firma(const ResultadoGeneracion &r)
{
    QString texto;
    for (const Modelo &m : r.modelos) {
        texto += m.nombre + QLatin1Char(':');
        for (const PreguntaExamen &pe : m.preguntas)
            texto += QStringLiteral(" %1/%2%3%4%5").arg(pe.pregunta.id).arg(pe.orden[0]).arg(pe.orden[1]).arg(pe.orden[2]).arg(pe.orden[3]);
        texto += QLatin1Char('\n');
    }
    return texto;
}

bool avisoQueContiene(const ResultadoGeneracion &r, const QString &texto)
{
    for (const QString &a : r.avisos)
        if (a.contains(texto))
            return true;
    return false;
}

} // namespace

class PruebaGenerador : public QObject
{
    Q_OBJECT

private slots:
    void mismaSemillaMismosExamenes()
    {
        const QVector<Pregunta> banco = bancoInventado();
        for (quint32 semilla : {1u, 7u, 12345u}) {
            ConfigExamen c = configuracion(semilla);
            c.barajarOpciones = true;
            QCOMPARE(firma(Generador::generar(banco, c)), firma(Generador::generar(banco, c)));
        }
        // Sin semilla se elige una al azar, que permite repetir la generación.
        ConfigExamen c = configuracion(0);
        const ResultadoGeneracion r = Generador::generar(banco, c);
        QVERIFY(r.semilla != 0);
        c.semilla = r.semilla;
        QCOMPARE(firma(Generador::generar(banco, c)), firma(r));
    }

    void repartoPorTema()
    {
        const QVector<Pregunta> banco = bancoInventado();
        for (quint32 semilla = 1; semilla <= 50; ++semilla) {
            const ConfigExamen c = configuracion(semilla);
            for (const Modelo &m : Generador::generar(banco, c).modelos) {
                QMap<int, int> porTema;
                for (const PreguntaExamen &pe : m.preguntas)
                    ++porTema[pe.pregunta.temaNumero];
                QCOMPARE(porTema, c.preguntasPorTema);
            }
        }
    }

    void nuncaDosVariantesNiPseudorrepetidas()
    {
        const QVector<Pregunta> banco = bancoInventado();
        for (quint32 semilla = 1; semilla <= 300; ++semilla) {
            ConfigExamen c = configuracion(semilla);
            c.noRepetirEntreModelos = semilla % 2;
            for (const Modelo &m : Generador::generar(banco, c).modelos) {
                QSet<QString> grupos;
                for (const PreguntaExamen &pe : m.preguntas)
                    grupos.insert(pe.pregunta.grupo());
                QCOMPARE(grupos.size(), m.preguntas.size());
                QVERIFY2(!(grupos.contains(QStringLiteral("T3-5")) && grupos.contains(QStringLiteral("T1-7"))),
                         "dos pseudorrepetidas en el mismo examen");
            }
        }
    }

    void noRepiteEntreExamenesSiHayBastantes()
    {
        const QVector<Pregunta> banco = bancoInventado();
        for (quint32 semilla = 1; semilla <= 50; ++semilla) {
            ConfigExamen c = configuracion(semilla);
            c.noRepetirEntreModelos = true;
            const ResultadoGeneracion r = Generador::generar(banco, c);
            QSet<QString> grupos;
            int total = 0;
            for (const Modelo &m : r.modelos)
                for (const PreguntaExamen &pe : m.preguntas) {
                    grupos.insert(pe.pregunta.grupo());
                    ++total;
                }
            QCOMPARE(grupos.size(), total);
            QVERIFY(!avisoQueContiene(r, QStringLiteral("se repiten")));
        }
    }

    void avisaSiFaltanPreguntas()
    {
        ConfigExamen c = configuracion(3);
        c.preguntasPorTema = {{1, 25}};
        c.nombresModelos = {QStringLiteral("Uno")};
        const ResultadoGeneracion r = Generador::generar(bancoInventado(), c);
        QCOMPARE(r.modelos.first().preguntas.size(), 20);
        QVERIFY(avisoQueContiene(r, QStringLiteral("Tema 1: se pidieron 25 preguntas pero solo hay 20")));
    }

    void avisaSiTieneQueRepetir()
    {
        ConfigExamen c = configuracion(3);
        c.preguntasPorTema = {{1, 8}};
        c.noRepetirEntreModelos = true;
        const ResultadoGeneracion r = Generador::generar(bancoInventado(), c);
        QVERIFY(avisoQueContiene(r, QStringLiteral("no hay preguntas suficientes para no repetir entre exámenes")));
    }

    void ordenadasPorTemaYNumero()
    {
        const ConfigExamen c = configuracion(5);
        for (const Modelo &m : Generador::generar(bancoInventado(), c).modelos)
            QVERIFY(std::is_sorted(m.preguntas.begin(), m.preguntas.end(),
                                   [](const PreguntaExamen &a, const PreguntaExamen &b) { return a.pregunta.antesQue(b.pregunta); }));
    }

    void barajarReparteLaCorrecta()
    {
        const QVector<Pregunta> banco = bancoInventado();
        for (quint32 semilla = 1; semilla <= 300; ++semilla) {
            ConfigExamen c = configuracion(semilla);
            c.preguntasPorTema = {{1, 3}, {2, 2}, {3, 3}, {4, 2}};
            c.barajarOpciones = true;
            c.ordenAleatorio = semilla % 2;
            for (const Modelo &m : Generador::generar(banco, c).modelos) {
                bool hayFijas = false;
                int racha = 0, anterior = -1;
                for (const PreguntaExamen &pe : m.preguntas) {
                    std::array<int, 4> orden = pe.orden;
                    std::sort(orden.begin(), orden.end());
                    QCOMPARE(orden, (std::array<int, 4>{{0, 1, 2, 3}}));
                    QCOMPARE(pe.opcion(pe.correcta()), pe.pregunta.opciones.at(pe.pregunta.correcta));
                    if (Estilo::tieneOpcionesDependientes(pe.pregunta)) {
                        hayFijas = true;
                        QCOMPARE(pe.orden, (std::array<int, 4>{{0, 1, 2, 3}}));
                    }
                    racha = pe.correcta() == anterior ? racha + 1 : 1;
                    anterior = pe.correcta();
                    QVERIFY2(racha < 3, "tres veces seguidas la misma letra");
                }
                const std::array<int, 4> cuenta = m.correctasPorLetra();
                if (!hayFijas)
                    QVERIFY(*std::max_element(cuenta.begin(), cuenta.end()) - *std::min_element(cuenta.begin(), cuenta.end()) <= 1);
            }
        }
    }

    void sinBarajarAvisaDelRepartoDesigual()
    {
        QVector<Pregunta> banco = bancoInventado();
        for (Pregunta &p : banco)
            p.correcta = 0;
        const ResultadoGeneracion r = Generador::generar(banco, configuracion(9));
        QVERIFY(avisoQueContiene(r, QStringLiteral("marque «Barajar las opciones» para repartirla")));
    }

    void evitaLasMarcadasOld()
    {
        const QVector<Pregunta> banco = bancoInventado();
        for (quint32 semilla = 1; semilla <= 50; ++semilla) {
            ConfigExamen c = configuracion(semilla);
            c.nombresModelos = {QStringLiteral("Uno"), QStringLiteral("Dos")};
            c.noRepetirEntreModelos = true;
            c.evitarOld = true;
            for (const Modelo &m : Generador::generar(banco, c).modelos)
                for (const PreguntaExamen &pe : m.preguntas)
                    QVERIFY2(!pe.pregunta.old, qPrintable(pe.pregunta.codigoCompleto()));
        }
    }

    void evitaLasDeConvocatoriasRecientes()
    {
        const QVector<Pregunta> banco = bancoInventado();
        const ResultadoGeneracion anterior = Generador::generar(banco, configuracion(1));
        ConfigExamen c = configuracion(2);
        QSet<QString> usados;
        for (const Modelo &m : anterior.modelos)
            for (const PreguntaExamen &pe : m.preguntas) {
                c.usoReciente.insert(pe.pregunta.codigoCompleto(), 1);
                usados.insert(pe.pregunta.grupo());
            }
        for (const Modelo &m : Generador::generar(banco, c).modelos)
            for (const PreguntaExamen &pe : m.preguntas)
                QVERIFY2(!usados.contains(pe.pregunta.grupo()), qPrintable(pe.pregunta.codigoCompleto()));
    }

    void siHayQueRepetirUsaLasMasAntiguas()
    {
        // Del tema 1, los números 1-10 salieron en la última convocatoria y los 11-20 en la anterior.
        const QVector<Pregunta> banco = bancoInventado(1, 20);
        ConfigExamen c = configuracion(4);
        c.preguntasPorTema = {{1, 10}};
        c.nombresModelos = {QStringLiteral("Uno")};
        for (const Pregunta &p : banco)
            c.usoReciente.insert(p.codigoCompleto(), p.numero <= 10 ? 1 : 2);
        const ResultadoGeneracion r = Generador::generar(banco, c);
        for (const PreguntaExamen &pe : r.modelos.first().preguntas)
            QVERIFY2(pe.pregunta.numero > 10, qPrintable(pe.pregunta.codigoCompleto()));
        QVERIFY(avisoQueContiene(r, QStringLiteral("convocatorias registradas")));
    }

    void gruposDisponibles()
    {
        QVector<Pregunta> banco = bancoInventado(2, 20);
        QCOMPARE(Generador::gruposDisponibles(banco, true), (QMap<int, int>{{1, 20}, {2, 20}}));
        banco[0].revisar = true;      // T1-1, sin variantes
        banco[1].estado = QStringLiteral("anulada");   // T1-2
        QCOMPARE(Generador::gruposDisponibles(banco, true), (QMap<int, int>{{1, 19}, {2, 20}}));
        QCOMPARE(Generador::gruposDisponibles(banco, false), (QMap<int, int>{{1, 18}, {2, 20}}));
    }
};

QTEST_MAIN(PruebaGenerador)
#include "prueba_generador.moc"
