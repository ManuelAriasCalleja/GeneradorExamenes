// Reglas del generador de exámenes: reparto por tema, variantes y pseudorrepetidas, repetición entre exámenes,
// orden, reparto de la respuesta correcta, preferencia por las preguntas no usadas antes y cambio de una pregunta.

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

    void cambiarUnaPregunta()
    {
        const QVector<Pregunta> banco = bancoInventado();
        for (quint32 semilla = 1; semilla <= 50; ++semilla) {
            ConfigExamen c = configuracion(semilla);
            c.barajarOpciones = true;
            c.evitarOld = true;
            const ResultadoGeneracion antes = Generador::generar(banco, c);
            ResultadoGeneracion r = antes;
            const int modelo = semilla % 3, indice = semilla % 20;
            const PreguntaExamen quitada = antes.modelos.at(modelo).preguntas.at(indice);
            QVERIFY(Generador::cambiarPregunta(banco, c, r, modelo, indice, {}));
            const PreguntaExamen nueva = r.modelos.at(modelo).preguntas.at(indice);
            // Otra pregunta del mismo tema, en el mismo sitio y con la correcta en la misma letra; lo demás, igual.
            QVERIFY(nueva.pregunta.grupo() != quitada.pregunta.grupo());
            QCOMPARE(nueva.pregunta.temaNumero, quitada.pregunta.temaNumero);
            QCOMPARE(nueva.correcta(), quitada.correcta());
            ResultadoGeneracion deshecho = r;
            deshecho.modelos[modelo].preguntas[indice] = quitada;
            QCOMPARE(firma(deshecho), firma(antes));
            // Ni variantes ni pseudorrepetidas en el examen, ni preguntas de los otros exámenes, ni marcadas «old»
            // (hay bastantes de las otras).
            QSet<QString> grupos;
            int total = 0;
            for (const Modelo &m : r.modelos)
                for (const PreguntaExamen &pe : m.preguntas) {
                    grupos.insert(pe.pregunta.grupo());
                    ++total;
                }
            QCOMPARE(grupos.size(), total);
            QSet<QString> delExamen;
            for (const PreguntaExamen &pe : r.modelos.at(modelo).preguntas)
                delExamen.insert(pe.pregunta.grupo());
            QVERIFY(!(delExamen.contains(QStringLiteral("T3-5")) && delExamen.contains(QStringLiteral("T1-7"))));
            QVERIFY2(!nueva.pregunta.old, qPrintable(nueva.pregunta.codigoCompleto()));
            QVERIFY(!avisoQueContiene(r, QStringLiteral(", pregunta ")));
        }
    }

    // Si la quitada tenía la correcta fija (sus opciones remiten a otras), la nueva no hereda su letra si con ella quedaría
    // peor repartida: va a una de las que menos salen (lo encontró una sonda, ver docs/revision_errores.md).
    void cambiarUnaFijaReparteLaCorrecta()
    {
        QVector<Pregunta> banco = bancoInventado(1, 3);
        banco[0].opciones[3] = QStringLiteral("Todas las anteriores.");
        banco[0].correcta = 3;
        QVERIFY(Estilo::tieneOpcionesDependientes(banco[0]));
        ConfigExamen c;
        c.preguntasPorTema = {{1, 2}};
        c.nombresModelos = {QStringLiteral("Uno")};
        c.barajarOpciones = true;
        ResultadoGeneracion r;
        Modelo m;
        m.nombre = QStringLiteral("Uno");
        PreguntaExamen fija;
        fija.pregunta = banco[0];                    // la correcta, en la d)
        PreguntaExamen otra;
        otra.pregunta = banco[1];
        otra.pregunta.correcta = 0;
        otra.orden = {{1, 2, 3, 0}};                 // también en la d)
        m.preguntas = {fija, otra};
        r.modelos = {m};
        for (int vez = 0; vez < 20; ++vez) {
            ResultadoGeneracion cambiado = r;
            QVERIFY(Generador::cambiarPregunta(banco, c, cambiado, 0, 0, {}));
            const PreguntaExamen &nueva = cambiado.modelos.at(0).preguntas.at(0);
            QCOMPARE(nueva.pregunta.id, banco[2].id);
            QVERIFY2(nueva.correcta() != 3, "la nueva se queda en la d), como la otra");
        }
    }

    void cambiarNoVuelveALasQuitadas()
    {
        // Un examen con 3 de los 6 números del tema 1: para cada sitio quedan otros 3.
        const QVector<Pregunta> banco = bancoInventado(1, 6);
        ConfigExamen c = configuracion(3);
        c.preguntasPorTema = {{1, 3}};
        c.nombresModelos = {QStringLiteral("Uno")};
        ResultadoGeneracion r = Generador::generar(banco, c);
        const QSet<QString> otras{r.modelos[0].preguntas[1].pregunta.grupo(), r.modelos[0].preguntas[2].pregunta.grupo()};
        QSet<QString> quitadas;
        for (int vez = 0; vez < 3; ++vez) {
            quitadas.insert(r.modelos[0].preguntas[0].pregunta.grupo());
            QVERIFY(Generador::cambiarPregunta(banco, c, r, 0, 0, quitadas));
            const QString nueva = r.modelos[0].preguntas[0].pregunta.grupo();
            QVERIFY2(!quitadas.contains(nueva) && !otras.contains(nueva), qPrintable(nueva));
        }
        // Probadas todas, vuelve una de las quitadas, con un aviso (uno solo, aunque se cambie otra vez).
        quitadas.insert(r.modelos[0].preguntas[0].pregunta.grupo());
        for (int vez = 0; vez < 2; ++vez) {
            QVERIFY(Generador::cambiarPregunta(banco, c, r, 0, 0, quitadas));
            QVERIFY(quitadas.contains(r.modelos[0].preguntas[0].pregunta.grupo()));
            QCOMPARE(r.avisos.filter(QStringLiteral("«Uno», pregunta 1: ")).size(), 1);
            QVERIFY(avisoQueContiene(r, QStringLiteral("ya quitadas")));
        }
    }

    void cambiarAvisaSiRepiteDeOtroExamen()
    {
        // Dos exámenes con los 6 números del tema 1: la nueva tiene que ser de los del otro examen.
        const QVector<Pregunta> banco = bancoInventado(1, 6);
        ConfigExamen c = configuracion(5);
        c.preguntasPorTema = {{1, 3}};
        c.nombresModelos = {QStringLiteral("Uno"), QStringLiteral("Dos")};
        ResultadoGeneracion r = Generador::generar(banco, c);
        QSet<QString> delOtro;
        for (const PreguntaExamen &pe : r.modelos[1].preguntas)
            delOtro.insert(pe.pregunta.grupo());
        QVERIFY(Generador::cambiarPregunta(banco, c, r, 0, 2, {}));
        QVERIFY(delOtro.contains(r.modelos[0].preguntas[2].pregunta.grupo()));
        QVERIFY(avisoQueContiene(r, QStringLiteral("«Uno», pregunta 3: ")));
        QVERIFY(avisoQueContiene(r, QStringLiteral("está también en otro examen")));
    }

    void cambiarSinOtraNoCambiaNada()
    {
        // El tema 1 tiene 3 números y el examen los usa todos.
        const QVector<Pregunta> banco = bancoInventado(1, 3);
        ConfigExamen c = configuracion(8);
        c.preguntasPorTema = {{1, 3}};
        c.nombresModelos = {QStringLiteral("Uno")};
        ResultadoGeneracion r = Generador::generar(banco, c);
        const QString antes = firma(r);
        QVERIFY(!Generador::cambiarPregunta(banco, c, r, 0, 1, {}));
        QCOMPARE(firma(r), antes);
        QVERIFY(!Generador::cambiarPregunta(banco, c, r, 0, 3, {}));   // no hay pregunta 4
    }

    void corregirUnaPreguntaDeLosExamenes()
    {
        ConfigExamen c = configuracion(4);
        c.barajarOpciones = true;
        ResultadoGeneracion r = Generador::generar(bancoInventado(), c);
        // Una pregunta con las opciones barajadas, que se pone también en otro examen.
        int i = 0;
        while (r.modelos[0].preguntas[i].orden == std::array<int, 4>{{0, 1, 2, 3}})
            ++i;
        const PreguntaExamen antes = r.modelos[0].preguntas[i];
        r.modelos[1].preguntas[0] = antes;
        ResultadoGeneracion sinCambios = r;

        // Otro texto: cambia en los dos sitios, con las opciones en el mismo orden; lo demás no cambia.
        Pregunta corregida = antes.pregunta;
        corregida.enunciado = QStringLiteral("Enunciado inventado y corregido:");
        corregida.opciones[0] = QStringLiteral("Opción inventada y corregida.");
        QCOMPARE(Generador::ponerCorregida(r, c, antes.pregunta, corregida), 2);
        for (const PreguntaExamen &pe : {r.modelos[0].preguntas[i], r.modelos[1].preguntas[0]}) {
            QCOMPARE(pe.pregunta.enunciado, corregida.enunciado);
            QVERIFY(pe.orden == antes.orden);
        }
        sinCambios.modelos[0].preguntas[i].pregunta = corregida;
        sinCambios.modelos[1].preguntas[0].pregunta = corregida;
        QCOMPARE(firma(r), firma(sinCambios));
        QCOMPARE(r.modelos[0].preguntas[i].correcta(), antes.correcta());

        // Otra correcta: va a la letra de la anterior.
        Pregunta otraCorrecta = corregida;
        otraCorrecta.correcta = (corregida.correcta + 1) % 4;
        QCOMPARE(Generador::ponerCorregida(r, c, corregida, otraCorrecta), 2);
        QCOMPARE(r.modelos[0].preguntas[i].correcta(), antes.correcta());
        QCOMPARE(r.modelos[0].preguntas[i].opcion(antes.correcta()), otraCorrecta.opciones[otraCorrecta.correcta]);

        // Con una opción que remite a las demás, sin barajar.
        Pregunta dependiente = otraCorrecta;
        dependiente.opciones[3] = QStringLiteral("Todas las anteriores.");
        QCOMPARE(Generador::ponerCorregida(r, c, otraCorrecta, dependiente), 2);
        QVERIFY(r.modelos[0].preguntas[i].orden == (std::array<int, 4>{{0, 1, 2, 3}}));

        // Otra pregunta (con otro código o con otro id) no está en ningún sitio: no cambia nada.
        const QString hasta = firma(r);
        Pregunta otroCodigo = dependiente;
        otroCodigo.numero += 100;
        Pregunta otroId = dependiente;
        otroId.id += 10000;
        QCOMPARE(Generador::ponerCorregida(r, c, otroCodigo, corregida), 0);
        QCOMPARE(Generador::ponerCorregida(r, c, otroId, corregida), 0);
        QCOMPARE(firma(r), hasta);
    }

    void gruposDisponibles()
    {
        QVector<Pregunta> banco = bancoInventado(2, 20);
        QCOMPARE(Generador::gruposDisponibles(banco, true), (QMap<int, int>{{1, 20}, {2, 20}}));
        banco[0].revisar = true;      // T1-1, sin variantes
        banco[1].estado = QStringLiteral("anulada");   // T1-2
        QCOMPARE(Generador::gruposDisponibles(banco, true), (QMap<int, int>{{1, 19}, {2, 20}}));
        QCOMPARE(Generador::gruposDisponibles(banco, false), (QMap<int, int>{{1, 18}, {2, 20}}));
        // Solo las verificadas: las de los números pares del tema 2 (las variantes, una del grupo basta).
        for (Pregunta &p : banco)
            p.verificada = p.temaNumero == 2 && p.numero % 2 == 0 && p.variante != QLatin1String("b");
        QCOMPARE(Generador::gruposDisponibles(banco, true, true), (QMap<int, int>{{2, 10}}));
    }

    void soloVerificadas()
    {
        QVector<Pregunta> banco = bancoInventado(8, 20);
        for (Pregunta &p : banco)
            p.verificada = p.numero % 2 == 0;
        ConfigExamen c = configuracion(9);
        c.soloVerificadas = true;
        const ResultadoGeneracion r = Generador::generar(banco, c);
        int preguntas = 0;
        for (const Modelo &m : r.modelos) {
            for (const PreguntaExamen &pe : m.preguntas) {
                QVERIFY2(pe.pregunta.verificada, qPrintable(pe.pregunta.codigoCompleto()));
                ++preguntas;
            }
        }
        QCOMPARE(preguntas, 3 * 20);
        // Si no hay bastantes, se avisa diciendo que son las verificadas.
        c.preguntasPorTema = {{1, 12}};
        c.nombresModelos = {QStringLiteral("Uno")};
        const ResultadoGeneracion pocas = Generador::generar(banco, c);
        QCOMPARE(pocas.avisos.value(0), QStringLiteral("Tema 1: se pidieron 12 preguntas pero solo hay 10 distintas verificadas disponibles."));
        // Sin la opción, entran todas: hay bastantes.
        c.soloVerificadas = false;
        QVERIFY(!Generador::generar(banco, c).avisos.join(QLatin1Char('\n')).contains(QStringLiteral("se pidieron")));
    }

    void ponerVerificacion()
    {
        ResultadoGeneracion r = Generador::generar(bancoInventado(), configuracion(4));
        Pregunta p = r.modelos.at(1).preguntas.at(2).pregunta;
        p.verificada = true;
        p.verificadores = {QStringLiteral("Persona Uno")};
        p.enunciado = QStringLiteral("Esto no se pone: solo la verificación.");
        const QString antes = firma(r);
        QCOMPARE(Generador::ponerVerificacion(r, p), 1);
        QVERIFY(r.modelos.at(1).preguntas.at(2).pregunta.verificada);
        QCOMPARE(r.modelos.at(1).preguntas.at(2).pregunta.verificadores, QStringList{QStringLiteral("Persona Uno")});
        QVERIFY(r.modelos.at(1).preguntas.at(2).pregunta.enunciado != p.enunciado);
        QCOMPARE(firma(r), antes);
        Pregunta otra = p;
        otra.id = -1;
        QCOMPARE(Generador::ponerVerificacion(r, otra), 0);
    }

    // Las preguntas que ya han salido en exámenes registrados, y las marcadas «old», llevan las opciones barajadas
    // aunque no se pida, con la correcta en otra letra que la vez anterior (de las «old», que en el banco). Las demás,
    // sin «Barajar las opciones», como en el banco.
    void lasQueYaSalieronSeBarajanSiempre()
    {
        const QVector<Pregunta> banco = bancoInventado();
        for (quint32 semilla = 1; semilla <= 50; ++semilla) {
            ConfigExamen c = configuracion(semilla);
            for (const Pregunta &p : banco)
                if (p.id % 2 == 0)
                    c.salidas.insert(p.codigoCompleto(), {QStringLiteral("Junio 2025"), {p.correcta}});
            for (const Modelo &m : Generador::generar(banco, c).modelos)
                for (const PreguntaExamen &pe : m.preguntas) {
                    const Pregunta &p = pe.pregunta;
                    QCOMPARE(pe.opcion(pe.correcta()), p.opciones.at(p.correcta));
                    if (Estilo::tieneOpcionesDependientes(p) || (p.id % 2 != 0 && !p.old))
                        QCOMPARE(pe.orden, (std::array<int, 4>{{0, 1, 2, 3}}));
                    else
                        QVERIFY2(pe.correcta() != p.correcta, qPrintable(p.codigoCompleto()));
                }
        }
    }

    // Si salió varias veces, la correcta va a una letra en que no estuvo nunca; si estuvo en todas, a otra que la
    // última vez. Con «Barajar las opciones» o sin ello.
    void laCorrectaVaDondeNoEstuvo()
    {
        const QVector<Pregunta> banco = bancoInventado(1, 12);
        for (quint32 semilla = 1; semilla <= 50; ++semilla) {
            ConfigExamen c = configuracion(semilla);
            c.preguntasPorTema = {{1, 12}};
            c.nombresModelos = {QStringLiteral("Uno")};
            c.barajarOpciones = semilla % 2;
            for (const Pregunta &p : banco)
                c.salidas.insert(p.codigoCompleto(), {QStringLiteral("Junio 2025"),
                                                      p.numero % 2 ? QVector<int>{2, 0, 1} : QVector<int>{1, 3, 0, 2}});
            const ResultadoGeneracion r = Generador::generar(banco, c);
            for (const PreguntaExamen &pe : r.modelos.first().preguntas) {
                if (pe.pregunta.numero % 2)
                    QCOMPARE(pe.correcta(), 3);   // la única en que no estuvo
                else
                    QVERIFY(pe.correcta() != 1);  // estuvo en todas: otra que la última vez
            }
        }
    }

    // Barajadas, la correcta sigue repartida por igual aunque algunas no puedan ir en alguna letra, si se puede.
    void yaSalidasYRepartidas()
    {
        const QVector<Pregunta> banco = bancoInventado();
        for (quint32 semilla = 1; semilla <= 100; ++semilla) {
            ConfigExamen c = configuracion(semilla);
            c.preguntasPorTema = {{1, 4}, {3, 4}, {4, 4}, {5, 4}, {6, 4}};   // el tema 2 tiene una que no se baraja
            c.barajarOpciones = true;
            for (const Pregunta &p : banco)
                if (p.id % 3 == 0)
                    c.salidas.insert(p.codigoCompleto(), {QStringLiteral("Junio 2025"), {p.id % 4}});
            for (const Modelo &m : Generador::generar(banco, c).modelos) {
                for (const PreguntaExamen &pe : m.preguntas)
                    if (pe.pregunta.id % 3 == 0)
                        QVERIFY(pe.correcta() != pe.pregunta.id % 4);
                const std::array<int, 4> cuenta = m.correctasPorLetra();
                QVERIFY(*std::max_element(cuenta.begin(), cuenta.end()) - *std::min_element(cuenta.begin(), cuenta.end()) <= 1);
            }
        }
    }

    // Lo que ya ha salido no cambia las preguntas que se eligen con una semilla: solo cómo van sus opciones.
    void yaSalidasConLasMismasPreguntas()
    {
        const QVector<Pregunta> banco = bancoInventado();
        auto preguntas = [](const ResultadoGeneracion &r) {
            QStringList ids;
            for (const Modelo &m : r.modelos)
                for (const PreguntaExamen &pe : m.preguntas)
                    ids << QString::number(pe.pregunta.id);
            return ids;
        };
        for (bool barajar : {false, true}) {
            ConfigExamen c = configuracion(33);
            c.barajarOpciones = barajar;
            const QStringList sinSalidas = preguntas(Generador::generar(banco, c));
            for (const Pregunta &p : banco)
                c.salidas.insert(p.codigoCompleto(), {QStringLiteral("Junio 2025"), {(p.correcta + 1) % 4}});
            QCOMPARE(preguntas(Generador::generar(banco, c)), sinSalidas);
        }
    }

    // La que tiene opciones que remiten a otras (la T2-6, marcada «old») no se baraja: si su correcta va donde estuvo,
    // se avisa.
    void yaSalidaSinBarajarAvisa()
    {
        const QVector<Pregunta> banco = bancoInventado(2, 6);
        ConfigExamen c = configuracion(3);
        c.preguntasPorTema = {{2, 6}};
        c.nombresModelos = {QStringLiteral("Uno")};
        Pregunta t26;
        for (const Pregunta &p : banco)
            if (p.codigoCompleto() == QStringLiteral("T2-6"))
                t26 = p;
        QVERIFY(Estilo::tieneOpcionesDependientes(t26) && t26.old);
        const QString letra(Pregunta::letra(t26.correcta));
        ResultadoGeneracion r = Generador::generar(banco, c);
        QVERIFY2(avisoQueContiene(r, QStringLiteral("T2-6 está marcada «old» y sus opciones no se pueden barajar porque "
                                                    "remiten a otras: la correcta sigue en la %1)").arg(letra)),
                 qPrintable(r.avisos.join(QLatin1Char('\n'))));
        c.salidas.insert(QStringLiteral("T2-6"), {QStringLiteral("Junio 2025"), {t26.correcta}});
        r = Generador::generar(banco, c);
        QVERIFY(avisoQueContiene(r, QStringLiteral("T2-6 ya salió en Junio 2025 con la correcta en la %1)").arg(letra)));
        // Si salió varias veces, la convocatoria en que tuvo esa letra, aunque no sea la última.
        c.salidas.insert(QStringLiteral("T2-6"), {QStringLiteral("Junio 2025"), {(t26.correcta + 1) % 4, t26.correcta},
                                                  {QStringLiteral("Junio 2025"), QStringLiteral("Junio 2023")}});
        r = Generador::generar(banco, c);
        QVERIFY2(avisoQueContiene(r, QStringLiteral("T2-6 ya salió en Junio 2023 con la correcta en la %1)").arg(letra)),
                 qPrintable(r.avisos.join(QLatin1Char('\n'))));
        // Si salió con las opciones en otro orden (un examen anterior al programa, leído de su PDF), no.
        c.salidas.insert(QStringLiteral("T2-6"), {QStringLiteral("Junio 2025"), {(t26.correcta + 1) % 4}});
        QVERIFY(!avisoQueContiene(Generador::generar(banco, c), QStringLiteral("T2-6")));
    }

    // Al corregir una pregunta que ya ha salido, su correcta no vuelve a una letra en que estuvo; y si ya no se puede
    // barajar y queda donde estuvo, se avisa.
    void corregirUnaYaSalida()
    {
        const QVector<Pregunta> banco = bancoInventado();
        ConfigExamen c = configuracion(6);
        ResultadoGeneracion r = Generador::generar(banco, c);
        // Una con las opciones como en el banco (ni «old» ni que remita a otras).
        int indice = -1;
        for (int i = 0; i < r.modelos.at(0).preguntas.size() && indice < 0; ++i) {
            const PreguntaExamen &pe = r.modelos.at(0).preguntas.at(i);
            if (!pe.pregunta.old && !Estilo::tieneOpcionesDependientes(pe.pregunta))
                indice = i;
        }
        QVERIFY(indice >= 0);
        const Pregunta antes = r.modelos.at(0).preguntas.at(indice).pregunta;
        QCOMPARE(r.modelos.at(0).preguntas.at(indice).orden, (std::array<int, 4>{{0, 1, 2, 3}}));
        // Salió con la correcta en la c), y se corrige para que la correcta sea la tercera opción.
        Pregunta corregida = antes;
        corregida.correcta = 2;
        if (antes.correcta == 2)
            corregida.correcta = 1;
        c.salidas.insert(antes.codigoCompleto(), {QStringLiteral("Junio 2025"), {corregida.correcta}});
        QCOMPARE(Generador::ponerCorregida(r, c, antes, corregida), 1);
        const PreguntaExamen &pe = r.modelos.at(0).preguntas.at(indice);
        QVERIFY(pe.correcta() != corregida.correcta);
        QCOMPARE(pe.opcion(pe.correcta()), corregida.opciones.at(corregida.correcta));
        QVERIFY(!avisoQueContiene(r, antes.codigoCompleto() + QStringLiteral(" ya salió")));

        // Corregida para que remita a las demás: ya no se baraja, queda donde estuvo, y se avisa.
        Pregunta dependiente = corregida;
        dependiente.opciones[3] = QStringLiteral("Todas las anteriores.");
        dependiente.correcta = 3;
        c.salidas.insert(antes.codigoCompleto(), {QStringLiteral("Junio 2025"), {3}});
        QCOMPARE(Generador::ponerCorregida(r, c, corregida, dependiente), 1);
        QCOMPARE(r.modelos.at(0).preguntas.at(indice).orden, (std::array<int, 4>{{0, 1, 2, 3}}));
        QVERIFY(avisoQueContiene(r, antes.codigoCompleto() + QStringLiteral(" ya salió en Junio 2025 con la correcta en la d)")));
        // Y si se corrige otra vez para que no remita, el aviso se va.
        QCOMPARE(Generador::ponerCorregida(r, c, dependiente, corregida), 1);
        QVERIFY(!avisoQueContiene(r, antes.codigoCompleto() + QStringLiteral(" ya salió")));
        QVERIFY(r.modelos.at(0).preguntas.at(indice).correcta() != 3);
    }

    // Al cambiar una pregunta, no se elige una que no se puede barajar si su correcta va a caer en una letra en que ya
    // estuvo (la T2-6, marcada «old»: en la de su banco), habiendo otras.
    void cambiarEvitaUnaFijaQueYaSalio()
    {
        const QVector<Pregunta> banco = bancoInventado(2, 6);
        int veces = 0;
        for (quint32 semilla = 1; semilla <= 60; ++semilla) {
            ConfigExamen c = configuracion(semilla);
            c.preguntasPorTema = {{2, 4}};
            c.nombresModelos = {QStringLiteral("Uno")};
            ResultadoGeneracion r = Generador::generar(banco, c);
            bool estaLaT26 = false;
            for (const PreguntaExamen &pe : r.modelos.at(0).preguntas)
                estaLaT26 = estaLaT26 || pe.pregunta.codigoCompleto() == QStringLiteral("T2-6");
            if (estaLaT26)
                continue;
            ++veces;
            QVERIFY(Generador::cambiarPregunta(banco, c, r, 0, 0, {}));
            QVERIFY(r.modelos.at(0).preguntas.at(0).pregunta.codigoCompleto() != QStringLiteral("T2-6"));
        }
        QVERIFY(veces > 10);
    }

    // Al cambiar una pregunta por otra que ya ha salido, también se barajan sus opciones, aunque no se pida.
    void cambiarPorUnaYaSalida()
    {
        const QVector<Pregunta> banco = bancoInventado();
        for (quint32 semilla = 1; semilla <= 30; ++semilla) {
            ConfigExamen c = configuracion(semilla);
            ResultadoGeneracion r = Generador::generar(banco, c);
            for (const Pregunta &p : banco)
                c.salidas.insert(p.codigoCompleto(), {QStringLiteral("Junio 2025"), {p.correcta}});
            const int indice = int(semilla % r.modelos.at(0).preguntas.size());
            QVERIFY(Generador::cambiarPregunta(banco, c, r, 0, indice, {}));
            const PreguntaExamen &nueva = r.modelos.at(0).preguntas.at(indice);
            if (!Estilo::tieneOpcionesDependientes(nueva.pregunta))
                QVERIFY2(nueva.correcta() != nueva.pregunta.correcta, qPrintable(nueva.pregunta.codigoCompleto()));
        }
    }

    // Versiones de un examen: cada una es un examen más, con las mismas preguntas, siempre por temas. La A, como el
    // examen sin versiones; las demás, con las de cada tema en otro orden y las opciones barajadas (repartida la
    // correcta). Los exámenes sin versiones salen igual que si ningún otro las tuviera.
    void versionesConLasMismasPreguntasPorTemas()
    {
        QVector<Pregunta> banco = bancoInventado();
        for (Pregunta &p : banco)
            p.old = false;   // sin preguntas que ya hayan salido: así la correcta siempre se puede repartir por igual
        auto ids = [](const Modelo &m) {
            QVector<int> res;
            for (const PreguntaExamen &pe : m.preguntas)
                res << pe.pregunta.id;
            return res;
        };
        for (quint32 semilla = 1; semilla <= 30; ++semilla) {
            ConfigExamen c = configuracion(semilla);
            const ResultadoGeneracion sinVersiones = Generador::generar(banco, c);
            c.versiones = {3, 1, 2};
            c.versionesConNumeros = semilla % 2;
            const ResultadoGeneracion r = Generador::generar(banco, c);
            const QString a = Generador::etiquetaVersion(0, c.versionesConNumeros);
            const QString b = Generador::etiquetaVersion(1, c.versionesConNumeros);
            const QString cc = Generador::etiquetaVersion(2, c.versionesConNumeros);
            QStringList nombres;
            for (const Modelo &m : r.modelos)
                nombres << m.nombre;
            const QStringList esperados{QStringLiteral("Uno ") + a, QStringLiteral("Uno ") + b, QStringLiteral("Uno ") + cc,
                                        QStringLiteral("Dos"), QStringLiteral("Tres ") + a, QStringLiteral("Tres ") + b};
            QCOMPARE(nombres, esperados);
            QCOMPARE(Generador::nombresExamenes(c), esperados);
            QCOMPARE(r.modelos.at(1).base, QStringLiteral("Uno"));
            QCOMPARE(r.modelos.at(1).version, b);
            QVERIFY(r.modelos.at(3).version.isEmpty() && r.modelos.at(3).base.isEmpty());
            QCOMPARE(Generador::versionesDe(r, 1), (QVector<int>{0, 1, 2}));
            QCOMPARE(Generador::versionesDe(r, 3), QVector<int>{3});
            QCOMPARE(Generador::versionesDe(r, 5), (QVector<int>{4, 5}));
            // La A y el examen sin versiones, como sin versiones.
            for (auto [conVersiones, sin] : {std::pair{0, 0}, std::pair{3, 1}, std::pair{4, 2}}) {
                QCOMPARE(ids(r.modelos.at(conVersiones)), ids(sinVersiones.modelos.at(sin)));
                for (int i = 0; i < r.modelos.at(conVersiones).preguntas.size(); ++i)
                    QCOMPARE(r.modelos.at(conVersiones).preguntas.at(i).orden, sinVersiones.modelos.at(sin).preguntas.at(i).orden);
            }
            for (const QVector<int> &grupo : {QVector<int>{0, 1, 2}, QVector<int>{4, 5}}) {
                QVector<int> deLaA = ids(r.modelos.at(grupo.first()));
                std::sort(deLaA.begin(), deLaA.end());
                QVector<QVector<int>> ordenes;
                for (int v : grupo) {
                    const Modelo &m = r.modelos.at(v);
                    QVector<int> estas = ids(m);
                    QVERIFY2(!ordenes.contains(estas), qPrintable(m.nombre + QStringLiteral(": el mismo orden que otra versión")));
                    ordenes << estas;
                    std::sort(estas.begin(), estas.end());
                    QCOMPARE(estas, deLaA);   // las mismas preguntas
                    QVERIFY2(std::is_sorted(m.preguntas.begin(), m.preguntas.end(), [](const PreguntaExamen &x, const PreguntaExamen &y) {
                                 return x.pregunta.temaNumero < y.pregunta.temaNumero;
                             }),
                             qPrintable(m.nombre + QStringLiteral(": no va por temas")));
                    bool hayFijas = false;
                    for (const PreguntaExamen &pe : m.preguntas) {
                        QCOMPARE(pe.opcion(pe.correcta()), pe.pregunta.opciones.at(pe.pregunta.correcta));
                        hayFijas = hayFijas || Estilo::tieneOpcionesDependientes(pe.pregunta);
                    }
                    if (v != grupo.first() && !hayFijas) {
                        QVERIFY(m.barajadas);
                        const std::array<int, 4> cuenta = m.correctasPorLetra();
                        QVERIFY(*std::max_element(cuenta.begin(), cuenta.end()) - *std::min_element(cuenta.begin(), cuenta.end()) <= 1);
                    }
                }
            }
            // Con la misma semilla, las mismas versiones.
            QCOMPARE(firma(Generador::generar(banco, c)), firma(r));
        }
    }

    // Con orden aleatorio, las versiones siguen yendo por temas, y también la A se desordena dentro de cada tema y
    // lleva las opciones barajadas, aunque no se pida.
    void versionesConOrdenAleatorio()
    {
        QVector<Pregunta> banco = bancoInventado();
        for (Pregunta &p : banco)
            p.old = false;
        bool algunaDesordenada = false;
        for (quint32 semilla = 1; semilla <= 30; ++semilla) {
            ConfigExamen c = configuracion(semilla);
            c.nombresModelos = {QStringLiteral("Uno")};
            c.versiones = {3};
            c.ordenAleatorio = true;
            for (const Modelo &m : Generador::generar(banco, c).modelos) {
                QVERIFY(std::is_sorted(m.preguntas.begin(), m.preguntas.end(), [](const PreguntaExamen &x, const PreguntaExamen &y) {
                    return x.pregunta.temaNumero < y.pregunta.temaNumero;
                }));
                QVERIFY(m.barajadas);
                algunaDesordenada = algunaDesordenada
                                    || !std::is_sorted(m.preguntas.begin(), m.preguntas.end(), [](const PreguntaExamen &x, const PreguntaExamen &y) {
                                           return x.pregunta.antesQue(y.pregunta);
                                       });
            }
        }
        QVERIFY(algunaDesordenada);   // dentro de los temas, no por número
    }

    // Al cambiar una pregunta de una versión, se cambia en todas, cada una en su sitio.
    void cambiarEnUnaVersionCambiaEnTodas()
    {
        const QVector<Pregunta> banco = bancoInventado();
        for (quint32 semilla = 1; semilla <= 20; ++semilla) {
            ConfigExamen c = configuracion(semilla);
            c.versiones = {3};
            ResultadoGeneracion r = Generador::generar(banco, c);
            const int indice = int(semilla % r.modelos.at(1).preguntas.size());
            const Pregunta quitada = r.modelos.at(1).preguntas.at(indice).pregunta;
            QVERIFY(Generador::cambiarPregunta(banco, c, r, 1, indice, {}));
            const Pregunta nueva = r.modelos.at(1).preguntas.at(indice).pregunta;
            QVERIFY(nueva.id != quitada.id);
            for (int v : {0, 1, 2}) {
                int veces = 0;
                for (const PreguntaExamen &pe : r.modelos.at(v).preguntas) {
                    QVERIFY(pe.pregunta.id != quitada.id);
                    veces += pe.pregunta.id == nueva.id ? 1 : 0;
                }
                QCOMPARE(veces, 1);
            }
        }
    }
};

QTEST_MAIN(PruebaGenerador)
#include "prueba_generador.moc"
