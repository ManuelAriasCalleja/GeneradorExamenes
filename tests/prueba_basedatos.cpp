// Base de datos: preguntas, JSON, transacciones, historial de convocatorias y temas.

#include "basedatos.h"
#include "datos_prueba.h"
#include "generador.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

namespace {

// Pregunta como texto, sin los id (que cambian de una base de datos a otra), para comparar.
QString describir(const Pregunta &p)
{
    return QStringList{p.codigoCompleto(), p.enunciado, p.opciones.join(QLatin1Char('|')), QString::number(p.correcta),
                       p.referencia, p.estado, QString::number(p.old), QString::number(p.revisar), p.pseudoCon, p.explicacion,
                       p.notas, QString::fromLatin1(p.imagen.toBase64())}
        .join(QLatin1Char('#'));
}

QStringList describir(const QVector<Pregunta> &preguntas)
{
    QStringList res;
    for (const Pregunta &p : preguntas)
        res << describir(p);
    return res;
}

} // namespace

class PruebaBaseDatos : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    int m_contador = 0;

    // Base de datos nueva, con los temas y las preguntas del banco inventado (la T4-1 con imagen).
    bool abrirConBanco(BaseDatos &bd, int temas = 8, int grupos = 20)
    {
        QVector<Pregunta> banco = bancoInventado(temas, grupos);
        for (Pregunta &p : banco)
            if (p.codigoCompleto() == QLatin1String("T4-1"))
                p.imagen = imagenInventada();
        return bd.abrir(m_dir.filePath(QStringLiteral("banco%1.db").arg(++m_contador))) && llenarBaseDatos(bd, temas, banco);
    }

private slots:
    void guardarYLeer()
    {
        BaseDatos bd;
        QVERIFY2(abrirConBanco(bd), qPrintable(bd.ultimoError()));
        const QVector<Pregunta> leidas = bd.preguntas();
        QVector<Pregunta> esperadas = bancoInventado();
        for (Pregunta &p : esperadas)
            if (p.codigoCompleto() == QLatin1String("T4-1"))
                p.imagen = imagenInventada();
        QCOMPARE(describir(leidas), describir(esperadas));

        Pregunta p = leidas.at(5);
        p.enunciado = QStringLiteral("Enunciado cambiado:");
        p.estado = QStringLiteral("anulada");
        QVERIFY(bd.guardarPregunta(p));
        QVERIFY(bd.borrarPregunta(leidas.at(6).id));
        const QVector<Pregunta> despues = bd.preguntas();
        QCOMPARE(despues.size(), leidas.size() - 1);
        QCOMPARE(despues.at(5).enunciado, QStringLiteral("Enunciado cambiado:"));
        QCOMPARE(despues.at(5).estado, QStringLiteral("anulada"));
    }

    void codigoRepetido()
    {
        BaseDatos bd;
        QVERIFY2(abrirConBanco(bd, 1, 5), qPrintable(bd.ultimoError()));
        Pregunta p = bd.preguntas().first();
        p.id = 0;
        QVERIFY(!bd.guardarPregunta(p));
        QCOMPARE(bd.ultimoError(), QStringLiteral("Ya existe una pregunta 1 en ese tema."));
    }

    void jsonIdaYVuelta()
    {
        BaseDatos origen;
        QVERIFY2(abrirConBanco(origen), qPrintable(origen.ultimoError()));
        const QString json = m_dir.filePath(QStringLiteral("banco.json"));
        QVERIFY(origen.exportarJson(json));
        BaseDatos destino;
        QVERIFY(destino.abrir(m_dir.filePath(QStringLiteral("importado.db"))));
        QCOMPARE(destino.importarJson(json, false), int(origen.preguntas().size()));
        QCOMPARE(describir(destino.preguntas()), describir(origen.preguntas()));
        // Importar otra vez actualiza en lugar de duplicar.
        QCOMPARE(destino.importarJson(json, false), int(origen.preguntas().size()));
        QCOMPARE(destino.preguntas().size(), origen.preguntas().size());
    }

    void importacionFallidaNoCambiaNada()
    {
        BaseDatos bd;
        QVERIFY2(abrirConBanco(bd, 2, 10), qPrintable(bd.ultimoError()));
        const QStringList antes = describir(bd.preguntas());
        const QString json = m_dir.filePath(QStringLiteral("roto.json"));
        QVERIFY(bd.exportarJson(json));
        // Una pregunta con un estado que no admite la base de datos, a mitad del fichero.
        QFile f(json);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QJsonObject raiz = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
        QJsonArray temas = raiz.value(QStringLiteral("temas")).toArray();
        QJsonObject tema = temas.at(1).toObject();
        QJsonArray preguntas = tema.value(QStringLiteral("preguntas")).toArray();
        QJsonObject rota = preguntas.at(3).toObject();
        rota[QStringLiteral("estado")] = QStringLiteral("rara");
        preguntas[3] = rota;
        tema[QStringLiteral("preguntas")] = preguntas;
        temas[1] = tema;
        raiz[QStringLiteral("temas")] = temas;
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write(QJsonDocument(raiz).toJson());
        f.close();

        const int copiasAntes = QDir(m_dir.filePath(QStringLiteral("copias"))).entryList(QDir::Files).size();
        QString copia = QStringLiteral("x");
        QCOMPARE(bd.importarJson(json, true, &copia), -1);   // reemplazar todo: borra antes los temas y las preguntas
        QVERIFY(bd.ultimoError().contains(QStringLiteral("CHECK")));
        QCOMPARE(describir(bd.preguntas()), antes);
        // Como no ha cambiado nada, la copia de seguridad que se hizo antes de borrar sobra y se quita.
        QVERIFY(copia.isEmpty());
        QCOMPARE(QDir(m_dir.filePath(QStringLiteral("copias"))).entryList(QDir::Files).size(), copiasAntes);
    }

    void reemplazarTodoHaceCopiaDeSeguridad()
    {
        BaseDatos bd;
        QVERIFY2(abrirConBanco(bd, 3, 10), qPrintable(bd.ultimoError()));
        const QStringList antes = describir(bd.preguntas());
        // Un JSON con otro banco, más pequeño.
        BaseDatos otro;
        QVERIFY2(abrirConBanco(otro, 2, 5), qPrintable(otro.ultimoError()));
        const QString json = m_dir.filePath(QStringLiteral("otro.json"));
        QVERIFY(otro.exportarJson(json));
        const QStringList nuevas = describir(otro.preguntas());

        QString copia;
        QVERIFY2(bd.importarJson(json, true, &copia) == nuevas.size(), qPrintable(bd.ultimoError()));
        QCOMPARE(describir(bd.preguntas()), nuevas);
        // La copia está en la carpeta «copias» junto a la base de datos, se llama como ella y tiene lo que había
        // antes de borrar.
        QVERIFY(!copia.isEmpty());
        const QFileInfo infoCopia(copia), infoBd(bd.ruta());
        QCOMPARE(infoCopia.absolutePath(), infoBd.absolutePath() + QStringLiteral("/copias"));
        QVERIFY2(infoCopia.fileName().startsWith(infoBd.completeBaseName() + QLatin1Char('_')), qPrintable(copia));
        QCOMPARE(infoCopia.suffix(), infoBd.suffix());
        BaseDatos restaurada;
        QVERIFY2(restaurada.abrir(copia), qPrintable(restaurada.ultimoError()));
        QCOMPARE(describir(restaurada.preguntas()), antes);
        QCOMPARE(restaurada.temas().size(), 3);

        // Otra copia en el mismo segundo no pisa la anterior.
        QString copia2;
        QCOMPARE(bd.importarJson(json, true, &copia2), nuevas.size());
        QVERIFY(!copia2.isEmpty());
        QVERIFY(copia2 != copia);
        QVERIFY(QFileInfo::exists(copia) && QFileInfo::exists(copia2));

        // Añadir/actualizar no borra nada y no hace copia; reemplazar un banco sin temas, tampoco.
        copia = QStringLiteral("x");
        QCOMPARE(bd.importarJson(json, false, &copia), nuevas.size());
        QVERIFY(copia.isEmpty());
        BaseDatos vacia;
        QVERIFY(vacia.abrir(m_dir.filePath(QStringLiteral("vacia.db"))));
        copia = QStringLiteral("x");
        QCOMPARE(vacia.importarJson(json, true, &copia), nuevas.size());
        QVERIFY(copia.isEmpty());
        QCOMPARE(describir(vacia.preguntas()), nuevas);
    }

    void historialRegistrarYCargar()
    {
        BaseDatos bd;
        QVERIFY2(abrirConBanco(bd), qPrintable(bd.ultimoError()));
        ConfigExamen c;
        c.preguntasPorTema = {{1, 3}, {2, 2}, {3, 3}, {4, 2}};
        c.nombresModelos = {QStringLiteral("Uno"), QStringLiteral("Dos")};
        c.semilla = 21;
        c.barajarOpciones = true;
        const ResultadoGeneracion r = Generador::generar(bd.preguntas(), c);
        DatosConvocatoria d;
        d.convocatoria = QStringLiteral("Junio");
        d.anio = 2026;
        d.instrucciones = QStringLiteral("Instrucciones inventadas.");
        QVERIFY(bd.registrarExamenes(r.modelos, d, r.semilla));

        const QVector<ExamenRegistrado> registrados = bd.examenesRegistrados();
        QCOMPARE(registrados.size(), 2);
        QCOMPARE(registrados.at(0).nombre, QStringLiteral("Uno"));
        QCOMPARE(registrados.at(0).semilla, quint32(21));
        QCOMPARE(registrados.at(0).numPreguntas, 10);
        QCOMPARE(registrados.at(0).datos.instrucciones, d.instrucciones);
        for (int i = 0; i < 2; ++i) {
            const Modelo m = bd.modeloRegistrado(registrados.at(i).id);
            QCOMPARE(m.nombre, r.modelos.at(i).nombre);
            QCOMPARE(m.preguntas.size(), r.modelos.at(i).preguntas.size());
            for (int k = 0; k < m.preguntas.size(); ++k) {
                QCOMPARE(m.preguntas.at(k).orden, r.modelos.at(i).preguntas.at(k).orden);
                QCOMPARE(describir(m.preguntas.at(k).pregunta), describir(r.modelos.at(i).preguntas.at(k).pregunta));
            }
        }
        const QString codigo = r.modelos.at(0).preguntas.at(0).pregunta.codigoCompleto();
        QCOMPARE(bd.usosDe(codigo), QStringList{QStringLiteral("Junio 2026 · Uno")});

        // Registrar otra vez sustituye; eliminar quita también sus preguntas.
        QVERIFY(bd.registrarExamenes(r.modelos, d, r.semilla));
        QCOMPARE(bd.examenesRegistrados().size(), 2);
        QVERIFY(bd.borrarExamenRegistrado(bd.examenesRegistrados().at(0).id));
        QCOMPARE(bd.examenesRegistrados().size(), 1);
        QVERIFY(bd.usosDe(codigo).isEmpty());
        QCOMPARE(bd.modeloRegistrado(registrados.at(0).id).preguntas.size(), 0);
    }

    void historialOrdenYUsoReciente()
    {
        BaseDatos bd;
        QVERIFY2(abrirConBanco(bd), qPrintable(bd.ultimoError()));
        auto registrar = [&](const QString &convocatoria, int anio, quint32 semilla) {
            ConfigExamen c;
            c.preguntasPorTema = {{1, 3}, {2, 2}};
            c.nombresModelos = {QStringLiteral("Uno")};
            c.semilla = semilla;
            const Modelo m = Generador::generar(bd.preguntas(), c).modelos.first();
            DatosConvocatoria d;
            d.convocatoria = convocatoria;
            d.anio = anio;
            bd.registrarExamenes({m}, d, semilla);
            return m;
        };
        const Modelo junio = registrar(QStringLiteral("Junio"), 2026, 1);
        const Modelo septiembre = registrar(QStringLiteral("Septiembre"), 2026, 2);
        registrar(QStringLiteral("Febrero"), 2026, 3);       // registrada después, pero anterior por el mes
        registrar(QStringLiteral("Extraordinaria"), 2025, 4);

        QStringList orden;
        for (const ExamenRegistrado &e : bd.examenesRegistrados())
            orden << e.datos.convocatoriaYAnio();
        QCOMPARE(orden, (QStringList{"Septiembre 2026", "Junio 2026", "Febrero 2026", "Extraordinaria 2025"}));

        const QHash<QString, int> uso = bd.usoReciente(2, 2027, QStringLiteral("Febrero"), {});
        for (const PreguntaExamen &pe : septiembre.preguntas)
            QCOMPARE(uso.value(pe.pregunta.codigoCompleto()), 1);
        for (const PreguntaExamen &pe : junio.preguntas)
            QVERIFY(uso.value(pe.pregunta.codigoCompleto()) >= 1 && uso.value(pe.pregunta.codigoCompleto()) <= 2);
        // Los exámenes que se van a generar de nuevo (mismo nombre, convocatoria y año) no cuentan.
        const QHash<QString, int> sinSeptiembre = bd.usoReciente(1, 2026, QStringLiteral("Septiembre"), {QStringLiteral("Uno")});
        for (const PreguntaExamen &pe : junio.preguntas)
            QCOMPARE(sinSeptiembre.value(pe.pregunta.codigoCompleto()), 1);
    }

    void temasIntercambiadosActualizanCodigos()
    {
        BaseDatos bd;
        QVERIFY2(abrirConBanco(bd), qPrintable(bd.ultimoError()));
        Pregunta enlazada = bd.preguntas().first();   // T1-1
        enlazada.pseudoCon = QStringLiteral("T3-2");
        QVERIFY(bd.guardarPregunta(enlazada));
        ConfigExamen c;
        c.preguntasPorTema = {{3, 3}};
        c.nombresModelos = {QStringLiteral("Uno")};
        c.semilla = 8;
        const Modelo m = Generador::generar(bd.preguntas(), c).modelos.first();
        DatosConvocatoria d;
        d.convocatoria = QStringLiteral("Junio");
        d.anio = 2026;
        QVERIFY(bd.registrarExamenes({m}, d, 8));
        const Pregunta registrada = m.preguntas.first().pregunta;   // del tema 3

        // Se intercambian los temas 3 y 4.
        QVector<Tema> temas = bd.temas();
        for (Tema &t : temas)
            t.numero = t.numero == 3 ? 4 : t.numero == 4 ? 3 : t.numero;
        QVERIFY2(bd.guardarTemas(temas, {}), qPrintable(bd.ultimoError()));

        const QVector<Pregunta> despues = bd.preguntas();
        QCOMPARE(despues.first().pseudoCon, QStringLiteral("T4-2"));   // sigue enlazada con la misma pregunta
        for (const Pregunta &p : despues)
            if (p.enunciado == registrada.enunciado)
                QCOMPARE(p.temaNumero, 4);
        const QString codigoNuevo = QStringLiteral("T4-") + registrada.codigo();
        QCOMPARE(bd.usosDe(codigoNuevo), QStringList{QStringLiteral("Junio 2026 · Uno")});
        QVERIFY(bd.usosDe(registrada.codigoCompleto()).isEmpty());
    }

    void borrarTemaBorraSusPreguntas()
    {
        BaseDatos bd;
        QVERIFY2(abrirConBanco(bd, 3, 10), qPrintable(bd.ultimoError()));
        QVector<Tema> temas = bd.temas();
        const int borrado = temas.takeAt(1).id;
        QVERIFY(bd.guardarTemas(temas, {borrado}));
        QCOMPARE(bd.temas().size(), 2);
        QCOMPARE(bd.contarPreguntas(borrado), 0);
        for (const Pregunta &p : bd.preguntas())
            QVERIFY(p.temaNumero != 2);
    }
};

QTEST_MAIN(PruebaBaseDatos)
#include "prueba_basedatos.moc"
