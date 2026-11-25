// Base de datos: preguntas, JSON, transacciones, historial de convocatorias, temas, asignatura y repartos, y bancos de
// versiones anteriores.

#include "basedatos.h"
#include "datos_prueba.h"
#include "generador.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>
#include <functional>

namespace {

// Pregunta como texto, sin los id (que cambian de una base de datos a otra), para comparar.
QString describir(const Pregunta &p)
{
    return QStringList{p.codigoCompleto(), p.enunciado, p.opciones.join(QLatin1Char('|')), QString::number(p.correcta),
                       p.referencia, p.estado, QString::number(p.old), QString::number(p.revisar), p.pseudoCon, p.explicacion,
                       p.notas, QString::fromLatin1(p.imagen.toBase64()), QString::number(p.verificada),
                       p.verificadores.join(QLatin1Char('|'))}
        .join(QLatin1Char('#'));
}

QStringList describir(const QVector<Pregunta> &preguntas)
{
    QStringList res;
    for (const Pregunta &p : preguntas)
        res << describir(p);
    return res;
}

QStringList describir(const QVector<Tema> &temas)
{
    QStringList res;
    for (const Tema &t : temas)
        res << QStringList{QString::number(t.numero), t.titulo, t.autor, t.formatoReferencia}.join(QLatin1Char('#'));
    return res;
}

QStringList formatos(const QVector<Tema> &temas)
{
    QStringList res;
    for (const Tema &t : temas)
        res << t.formatoReferencia;
    return res;
}

// Cambia un JSON de banco con 'cambiar'.
bool cambiarJson(const QString &ruta, const std::function<void(QJsonObject &)> &cambiar)
{
    QFile f(ruta);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    QJsonObject raiz = QJsonDocument::fromJson(f.readAll()).object();
    f.close();
    cambiar(raiz);
    return f.open(QIODevice::WriteOnly | QIODevice::Truncate) && f.write(QJsonDocument(raiz).toJson()) > 0;
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

    // Los U+FEFF (la marca de orden de bytes, invisible) del principio de un texto no se guardan: SQLite los quitaría de
    // uno en uno cada vez, y el texto cambiaría al pasar por el JSON (lo encontró una sonda, ver docs/revision_errores.md).
    void marcaDeOrdenAlPrincipio()
    {
        BaseDatos bd;
        QVERIFY2(abrirConBanco(bd, 1, 2), qPrintable(bd.ultimoError()));
        const QString marca(QChar(0xFEFF));
        Pregunta p = bd.preguntas().first();
        p.opciones[1] = marca + marca + QStringLiteral("Opción inventada con dos marcas delante.");
        p.enunciado = marca + QStringLiteral("Enunciado inventado") + marca + QStringLiteral(":");   // la de dentro se queda
        QVERIFY(bd.guardarPregunta(p));
        QCOMPARE(bd.preguntas().first().opciones.at(1), QStringLiteral("Opción inventada con dos marcas delante."));
        QCOMPARE(bd.preguntas().first().enunciado, QStringLiteral("Enunciado inventado") + marca + QStringLiteral(":"));
        const QString json = m_dir.filePath(QStringLiteral("marca.json"));
        QVERIFY(bd.exportarJson(json));
        QVERIFY(bd.importarJson(json, false) > 0);
        QVERIFY(bd.diferenciasConJson(json).isEmpty());
        QCOMPARE(bd.preguntas().first().opciones.at(1), QStringLiteral("Opción inventada con dos marcas delante."));
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
        QVERIFY(origen.guardarAsignatura(QStringLiteral("Asignatura inventada")));
        const QString json = m_dir.filePath(QStringLiteral("banco.json"));
        QVERIFY(origen.exportarJson(json));
        BaseDatos destino;
        QVERIFY(destino.abrir(m_dir.filePath(QStringLiteral("importado.db"))));
        QCOMPARE(destino.importarJson(json, false), int(origen.preguntas().size()));
        QCOMPARE(describir(destino.preguntas()), describir(origen.preguntas()));
        QCOMPARE(describir(destino.temas()), describir(origen.temas()));   // con la forma de la referencia
        QCOMPARE(destino.asignatura(), QStringLiteral("Asignatura inventada"));   // el banco nuevo no tenía
        // Importar otra vez actualiza en lugar de duplicar.
        QCOMPARE(destino.importarJson(json, false), int(origen.preguntas().size()));
        QCOMPARE(destino.preguntas().size(), origen.preguntas().size());
    }

    void asignaturaYRepartos()
    {
        // Un banco nuevo aún no es de ninguna asignatura ni tiene repartos.
        BaseDatos bd;
        QVERIFY2(abrirConBanco(bd, 4, 5), qPrintable(bd.ultimoError()));
        QVERIFY(bd.asignatura().isEmpty());
        QVERIFY(bd.repartos().isEmpty());
        QVERIFY(bd.ultimoReparto().isEmpty());
        QVERIFY(bd.guardarAsignatura(QStringLiteral("  Asignatura inventada ")));
        QVERIFY(bd.guardarRepartos({{QStringLiteral("T1–T2"), {{1, 3}, {2, 2}}}, {QStringLiteral("Todo"), {{1, 1}, {2, 1}, {3, 1}, {4, 1}}}}));
        QVERIFY(bd.guardarUltimoReparto({{1, 2}, {2, 0}, {3, 4}, {4, 0}}));
        const QString ruta = bd.ruta();
        bd.cerrar();
        QVERIFY(bd.abrir(ruta));
        QCOMPARE(bd.asignatura(), QStringLiteral("Asignatura inventada"));
        const QVector<Reparto> repartos = bd.repartos();
        QCOMPARE(repartos.size(), 2);
        QCOMPARE(repartos.at(0).nombre, QStringLiteral("T1–T2"));
        QCOMPARE(repartos.at(0).preguntas, (QMap<int, int>{{1, 3}, {2, 2}}));
        QCOMPARE(repartos.at(1).nombre, QStringLiteral("Todo"));
        QCOMPARE(bd.ultimoReparto(), (QMap<int, int>{{1, 2}, {2, 0}, {3, 4}, {4, 0}}));
    }

    void opcionesDelBanco()
    {
        BaseDatos bd;
        QVERIFY2(abrirConBanco(bd, 2, 5), qPrintable(bd.ultimoError()));
        QVERIFY(bd.opciones().isEmpty());
        const QVariantMap opciones{
            {QStringLiteral("examen/numRecientes"), 7},
            {QStringLiteral("examen/evitarRecientes"), true},
            {QStringLiteral("examen/fallo"), 0.25},
            {QStringLiteral("examen/instrucciones"), QStringLiteral("Instrucciones inventadas.\nSegunda línea.")},
            {QStringLiteral("examenes/nombres"), QStringList{QStringLiteral("Primero"), QStringLiteral("Segundo")}},
            {QStringLiteral("examenes/marcados"), QStringList{QStringLiteral("Segundo")}},
            {QStringLiteral("exportar/carpeta"), QStringLiteral("../exportados")},
        };
        QVERIFY(bd.guardarOpciones(opciones));
        // Una convocatoria registrada, para ver que «Reemplazar todo» no se lleva ni las opciones ni el historial.
        Modelo m;
        m.nombre = QStringLiteral("Primero");
        for (const Pregunta &p : bd.preguntas().mid(0, 2))
            m.preguntas.append({p, {{0, 1, 2, 3}}});
        DatosConvocatoria datos;
        datos.convocatoria = QStringLiteral("Junio");
        datos.anio = 2030;
        QVERIFY(bd.registrarExamenes({m}, datos, 1));
        const QString json = m_dir.filePath(QStringLiteral("opciones.json"));
        QVERIFY(bd.exportarJson(json));
        QVERIFY(bd.importarJson(json, true) > 0);
        const QString ruta = bd.ruta();
        bd.cerrar();
        QVERIFY(bd.abrir(ruta));
        const QVariantMap leidas = bd.opciones();
        QCOMPARE(leidas.keys(), opciones.keys());
        QCOMPARE(leidas.value(QStringLiteral("examen/numRecientes")).toInt(), 7);
        QCOMPARE(leidas.value(QStringLiteral("examen/evitarRecientes")).toBool(), true);
        QCOMPARE(leidas.value(QStringLiteral("examen/fallo")).toDouble(), 0.25);
        QCOMPARE(leidas.value(QStringLiteral("examen/instrucciones")).toString(), QStringLiteral("Instrucciones inventadas.\nSegunda línea."));
        QCOMPARE(leidas.value(QStringLiteral("examenes/nombres")).toStringList(), (QStringList{QStringLiteral("Primero"), QStringLiteral("Segundo")}));
        QCOMPARE(leidas.value(QStringLiteral("examenes/marcados")).toStringList(), QStringList{QStringLiteral("Segundo")});
        QCOMPARE(leidas.value(QStringLiteral("exportar/carpeta")).toString(), QStringLiteral("../exportados"));
        QCOMPARE(bd.examenesRegistrados().size(), 1);
        QCOMPARE(bd.usoReciente(1, 2031, QStringLiteral("Febrero"), {}).size(), 2);
    }

    void bancoAntiguoSePoneAlDia()
    {
        // Bancos de una versión anterior (sin PRAGMA user_version): sin la forma de la referencia de los temas, ni
        // asignatura, ni repartos. Uno con nueve temas y otro sin ninguno.
        auto crear = [](const QString &ruta, int temas) {
            {
                QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("antiguo"));
                db.setDatabaseName(ruta);
                if (!db.open())
                    return false;
                QSqlQuery q(db);
                if (!q.exec(QStringLiteral("CREATE TABLE temas (id INTEGER PRIMARY KEY AUTOINCREMENT, numero INTEGER NOT NULL UNIQUE,"
                                           " titulo TEXT NOT NULL, autor TEXT NOT NULL DEFAULT '')")))
                    return false;
                for (int t = 1; t <= temas; ++t)
                    if (!q.exec(QStringLiteral("INSERT INTO temas (numero, titulo) VALUES (%1, 'Tema inventado %1')").arg(t)))
                        return false;
            }
            QSqlDatabase::removeDatabase(QStringLiteral("antiguo"));
            return true;
        };
        const QString ruta = m_dir.filePath(QStringLiteral("antiguo.db"));
        const QString vacio = m_dir.filePath(QStringLiteral("antiguo_vacio.db"));
        QVERIFY(crear(ruta, 9));
        QVERIFY(crear(vacio, 0));

        // El que tiene temas era de Fundamentos de la informática: su nombre, sus repartos y la forma de la referencia
        // de sus ocho temas.
        BaseDatos bd;
        QVERIFY2(bd.abrir(ruta), qPrintable(bd.ultimoError()));
        QCOMPARE(bd.asignatura(), QStringLiteral("Fundamentos de la informática"));
        QStringList nombres;
        for (const Reparto &r : bd.repartos())
            nombres << r.nombre;
        QCOMPARE(nombres, (QStringList{QStringLiteral("T1–T4"), QStringLiteral("T5–T8"), QStringLiteral("T1–T8")}));
        QCOMPARE(bd.repartos().at(0).preguntas, (QMap<int, int>{{1, 3}, {2, 2}, {3, 3}, {4, 2}}));
        QCOMPARE(bd.repartos().at(1).preguntas, (QMap<int, int>{{5, 2}, {6, 3}, {7, 2}, {8, 3}}));
        QCOMPARE(bd.repartos().at(2).preguntas.size(), 8);
        QCOMPARE(formatos(bd.temas()), (QStringList{QStringLiteral("vease"), QStringLiteral("pag"), QStringLiteral("vease"),
                                                    QStringLiteral("pag"), QStringLiteral("pag"), QStringLiteral("vease"),
                                                    QStringLiteral("pag"), QStringLiteral("vease"), QString()}));
        Pregunta p = bancoInventado(1, 1).first();
        p.id = 0;
        QVERIFY2(bd.guardarPregunta(p), qPrintable(bd.ultimoError()));

        // Ya está al día: lo que se cambie después no se vuelve a poner.
        QVERIFY(bd.guardarAsignatura(QStringLiteral("Otra asignatura inventada")));
        QVERIFY(bd.guardarRepartos({}));
        bd.cerrar();
        QVERIFY(bd.abrir(ruta));
        QCOMPARE(bd.asignatura(), QStringLiteral("Otra asignatura inventada"));
        QVERIFY(bd.repartos().isEmpty());

        // El que no tiene temas aún no es de ninguna asignatura.
        QVERIFY2(bd.abrir(vacio), qPrintable(bd.ultimoError()));
        QVERIFY(bd.asignatura().isEmpty());
        QVERIFY(bd.repartos().isEmpty());
    }

    void verificadoresYVerificacion()
    {
        BaseDatos bd;
        QVERIFY2(abrirConBanco(bd, 2, 3), qPrintable(bd.ultimoError()));
        QVERIFY(bd.verificadores().isEmpty());
        QVERIFY(bd.anadirVerificador(QStringLiteral("Persona Uno")));
        QVERIFY(bd.anadirVerificador(QStringLiteral("  Otra Persona ")));
        QVERIFY(!bd.anadirVerificador(QStringLiteral("persona uno")));   // sin distinguir mayúsculas
        QVERIFY(!bd.anadirVerificador(QStringLiteral(" ")));
        QCOMPARE(bd.verificadores(), (QStringList{QStringLiteral("Otra Persona"), QStringLiteral("Persona Uno")}));

        // Al guardar una pregunta verificada, sus verificadores (los que no estaban, se añaden), por orden alfabético.
        Pregunta p = bd.preguntas().first();
        QVERIFY(!p.verificada);
        p.verificada = true;
        p.verificadores = {QStringLiteral("Persona Uno"), QStringLiteral("Alguien Nuevo")};
        QVERIFY2(bd.guardarPregunta(p), qPrintable(bd.ultimoError()));
        QCOMPARE(bd.preguntas().first().verificadores, (QStringList{QStringLiteral("Alguien Nuevo"), QStringLiteral("Persona Uno")}));
        QVERIFY(bd.preguntas().first().verificada);
        QCOMPARE(bd.verificadores().size(), 3);
        QCOMPARE(bd.preguntasVerificadasPor(QStringLiteral("Persona Uno")), 1);
        // Sin verificar, sin verificadores.
        p.verificada = false;
        QVERIFY(bd.guardarPregunta(p));
        QVERIFY(bd.preguntas().first().verificadores.isEmpty());
        QCOMPARE(bd.preguntasVerificadasPor(QStringLiteral("Persona Uno")), 0);

        // Solo la verificación, sin tocar lo demás; de una o de varias.
        const int id = bd.preguntas().at(1).id;
        QVERIFY(bd.guardarVerificacion(id, true, {QStringLiteral("Otra Persona")}));
        QCOMPARE(bd.preguntas().at(1).verificadores, QStringList{QStringLiteral("Otra Persona")});
        QCOMPARE(bd.preguntas().at(1).enunciado, bancoInventado(2, 3).at(1).enunciado);
        QVector<Pregunta> varias = bd.preguntas().mid(2, 2);
        for (Pregunta &v : varias) {
            v.verificada = true;
            v.verificadores = {QStringLiteral("Persona Uno"), QStringLiteral("Otra Persona")};
        }
        QVERIFY(bd.guardarVerificaciones(varias));
        QCOMPARE(bd.preguntasVerificadasPor(QStringLiteral("Persona Uno")), 2);

        // Los últimos que han verificado, para proponerlos en la siguiente; con los nombres cambiados y sin los quitados.
        QVERIFY(bd.guardarUltimosVerificadores({QStringLiteral("Persona Uno"), QStringLiteral("Otra Persona")}));
        QVERIFY(!bd.cambiarNombreVerificador(QStringLiteral("Persona Uno"), QStringLiteral("otra persona")));
        QVERIFY(bd.cambiarNombreVerificador(QStringLiteral("Persona Uno"), QStringLiteral("Persona Primera")));
        QCOMPARE(bd.preguntas().at(2).verificadores, (QStringList{QStringLiteral("Otra Persona"), QStringLiteral("Persona Primera")}));
        QCOMPARE(bd.ultimosVerificadores(), (QStringList{QStringLiteral("Persona Primera"), QStringLiteral("Otra Persona")}));
        // Quitar a alguien no quita la verificación de sus preguntas.
        QVERIFY(bd.borrarVerificador(QStringLiteral("Otra Persona")));
        QCOMPARE(bd.ultimosVerificadores(), QStringList{QStringLiteral("Persona Primera")});
        QVERIFY(bd.preguntas().at(1).verificada);
        QVERIFY(bd.preguntas().at(1).verificadores.isEmpty());
        QCOMPARE(bd.preguntas().at(2).verificadores, QStringList{QStringLiteral("Persona Primera")});
        // Al borrar una pregunta, su verificación se va con ella.
        QVERIFY(bd.borrarPregunta(bd.preguntas().at(2).id));
        QCOMPARE(bd.preguntasVerificadasPor(QStringLiteral("Persona Primera")), 1);
    }

    void verificacionEnElJson()
    {
        BaseDatos origen;
        QVERIFY2(abrirConBanco(origen, 2, 3), qPrintable(origen.ultimoError()));
        QVERIFY(origen.anadirVerificador(QStringLiteral("Sin Preguntas")));
        QVERIFY(origen.guardarVerificacion(origen.preguntas().first().id, true, {QStringLiteral("Persona Uno")}));
        const QString json = m_dir.filePath(QStringLiteral("verificacion.json"));
        QVERIFY(origen.exportarJson(json));
        {
            QFile f(json);
            QVERIFY(f.open(QIODevice::ReadOnly));
            const QJsonObject raiz = QJsonDocument::fromJson(f.readAll()).object();
            QCOMPARE(raiz.value(QStringLiteral("verificadores")).toArray().size(), 2);
            const QJsonArray preguntas = raiz.value(QStringLiteral("temas")).toArray().first().toObject().value(QStringLiteral("preguntas")).toArray();
            QCOMPARE(preguntas.at(0).toObject().value(QStringLiteral("verificada")), QJsonValue(true));
            QCOMPARE(preguntas.at(0).toObject().value(QStringLiteral("verificadores")).toArray().first().toString(), QStringLiteral("Persona Uno"));
            QCOMPARE(preguntas.at(1).toObject().value(QStringLiteral("verificada")), QJsonValue(false));   // dicho siempre
            QVERIFY(!preguntas.at(1).toObject().contains(QStringLiteral("verificadores")));
        }
        // Ida y vuelta: la verificación y los verificadores (también los que aún no han verificado nada).
        BaseDatos destino;
        QVERIFY(destino.abrir(m_dir.filePath(QStringLiteral("verificacion.db"))));
        QVERIFY(destino.importarJson(json, false) > 0);
        QCOMPARE(describir(destino.preguntas()), describir(origen.preguntas()));
        QCOMPARE(destino.verificadores(), origen.verificadores());
        QVERIFY(destino.diferenciasConJson(json).isEmpty());

        // Un JSON que no dice la verificación (escrito a mano o por una IA): las que no cambian la conservan, y las que
        // cambian quedan sin verificar; tampoco es una diferencia con el banco.
        QVERIFY(destino.guardarVerificacion(destino.preguntas().at(1).id, true, {QStringLiteral("Persona Uno")}));
        QVERIFY(cambiarJson(json, [](QJsonObject &raiz) {
            raiz.remove(QStringLiteral("verificadores"));
            QJsonArray temas = raiz.value(QStringLiteral("temas")).toArray();
            QJsonObject tema = temas.at(0).toObject();
            QJsonArray preguntas = tema.value(QStringLiteral("preguntas")).toArray();
            for (QJsonValueRef v : preguntas) {
                QJsonObject p = v.toObject();
                p.remove(QStringLiteral("verificada"));
                p.remove(QStringLiteral("verificadores"));
                v = p;
            }
            QJsonObject segunda = preguntas.at(1).toObject();
            segunda[QStringLiteral("enunciado")] = QStringLiteral("Enunciado inventado cambiado por otro:");
            preguntas[1] = segunda;
            tema[QStringLiteral("preguntas")] = preguntas;
            temas[0] = tema;
            raiz[QStringLiteral("temas")] = temas;
        }));
        QCOMPARE(destino.diferenciasConJson(json), QStringList{QStringLiteral("1 pregunta distinta: T1-2")});
        for (bool reemplazar : {false, true}) {
            QVERIFY(destino.guardarVerificacion(destino.preguntas().at(1).id, true, {QStringLiteral("Persona Uno")}));
            QVERIFY(destino.importarJson(json, reemplazar) > 0);
            const QVector<Pregunta> despues = destino.preguntas();
            QVERIFY2(despues.at(0).verificada, reemplazar ? "reemplazando" : "actualizando");
            QCOMPARE(despues.at(0).verificadores, QStringList{QStringLiteral("Persona Uno")});
            // La T1-2 la primera vez ha cambiado; la segunda, ya no (el banco tiene el texto del JSON).
            QCOMPARE(despues.at(1).verificada, reemplazar);
            QVERIFY(!despues.at(2).verificada);
        }
        // Si el JSON dice que no está verificada, no lo está.
        QVERIFY(destino.guardarVerificacion(destino.preguntas().at(1).id, true, {}));
        QVERIFY(destino.exportarJson(json));
        QVERIFY(destino.guardarVerificacion(destino.preguntas().at(2).id, true, {QStringLiteral("Persona Uno")}));
        QCOMPARE(destino.diferenciasConJson(json), QStringList{QStringLiteral("1 pregunta distinta: T1-3")});
        QVERIFY(destino.importarJson(json, false) > 0);
        QVERIFY(!destino.preguntas().at(2).verificada);
    }

    void bancoSinVerificacionSePoneAlDia()
    {
        // Un banco de la versión 1 del esquema: las preguntas no dicen si están verificadas.
        const QString ruta = m_dir.filePath(QStringLiteral("version1.db"));
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("version1"));
            db.setDatabaseName(ruta);
            QVERIFY(db.open());
            QSqlQuery q(db);
            QVERIFY(q.exec(QStringLiteral("CREATE TABLE temas (id INTEGER PRIMARY KEY AUTOINCREMENT, numero INTEGER NOT NULL UNIQUE,"
                                          " titulo TEXT NOT NULL, autor TEXT NOT NULL DEFAULT '',"
                                          " formato_referencia TEXT NOT NULL DEFAULT '')")));
            QVERIFY(q.exec(QStringLiteral(
                "CREATE TABLE preguntas (id INTEGER PRIMARY KEY AUTOINCREMENT,"
                " tema_id INTEGER NOT NULL REFERENCES temas(id) ON DELETE CASCADE, numero INTEGER NOT NULL,"
                " variante TEXT NOT NULL DEFAULT '', old INTEGER NOT NULL DEFAULT 0, estado TEXT NOT NULL DEFAULT 'activa',"
                " revisar INTEGER NOT NULL DEFAULT 0, enunciado TEXT NOT NULL, opcion_a TEXT NOT NULL, opcion_b TEXT NOT NULL,"
                " opcion_c TEXT NOT NULL, opcion_d TEXT NOT NULL, correcta TEXT NOT NULL, referencia TEXT NOT NULL DEFAULT '',"
                " explicacion TEXT NOT NULL DEFAULT '', notas TEXT NOT NULL DEFAULT '', pseudo_con TEXT NOT NULL DEFAULT '',"
                " imagen BLOB, UNIQUE (tema_id, numero, variante))")));
            QVERIFY(q.exec(QStringLiteral("INSERT INTO temas (numero, titulo) VALUES (1, 'Tema inventado')")));
            QVERIFY(q.exec(QStringLiteral("INSERT INTO preguntas (tema_id, numero, enunciado, opcion_a, opcion_b, opcion_c, opcion_d,"
                                          " correcta) VALUES (1, 1, 'Enunciado inventado:', 'Una.', 'Dos.', 'Tres.', 'Cuatro.', 'A')")));
            QVERIFY(q.exec(QStringLiteral("PRAGMA user_version = 1")));
        }
        QSqlDatabase::removeDatabase(QStringLiteral("version1"));
        BaseDatos bd;
        QVERIFY2(bd.abrir(ruta), qPrintable(bd.ultimoError()));
        Pregunta p = bd.preguntas().first();
        QVERIFY(!p.verificada);
        p.verificada = true;
        p.verificadores = {QStringLiteral("Persona Uno")};
        QVERIFY2(bd.guardarPregunta(p), qPrintable(bd.ultimoError()));
        bd.cerrar();
        QVERIFY(bd.abrir(ruta));
        QCOMPARE(bd.preguntas().first().verificadores, QStringList{QStringLiteral("Persona Uno")});
        QCOMPARE(bd.ajuste(QStringLiteral("asignatura")), QString());   // la versión 1 ya estaba: no se toca
    }

    void importarConservaLoQueElJsonNoDice()
    {
        // Un JSON como los de importar_docx.py: sin la forma de la referencia de los temas, y con otro nombre.
        BaseDatos origen;
        QVERIFY2(abrirConBanco(origen, 3, 4), qPrintable(origen.ultimoError()));
        const QString json = m_dir.filePath(QStringLiteral("sin_formato.json"));
        QVERIFY(origen.exportarJson(json));
        QVERIFY(cambiarJson(json, [](QJsonObject &raiz) {
            raiz[QStringLiteral("asignatura")] = QStringLiteral("Otra asignatura inventada");
            QJsonArray temas = raiz.value(QStringLiteral("temas")).toArray();
            for (QJsonValueRef t : temas) {
                QJsonObject tema = t.toObject();
                tema.remove(QStringLiteral("formato_referencia"));
                t = tema;
            }
            raiz[QStringLiteral("temas")] = temas;
        }));

        // Al reemplazar todo, cada tema se queda con la forma que tenía, y el banco con su nombre.
        BaseDatos bd;
        QVERIFY2(abrirConBanco(bd, 3, 4), qPrintable(bd.ultimoError()));
        QVector<Tema> temas = bd.temas();
        temas[0].formatoReferencia = QStringLiteral("pag");
        QVERIFY(bd.guardarTemas(temas, {}));
        QVERIFY(bd.guardarAsignatura(QStringLiteral("Asignatura inventada")));
        QVERIFY(bd.importarJson(json, true) > 0);
        QCOMPARE(formatos(bd.temas()), (QStringList{QStringLiteral("pag"), QStringLiteral("pag"), QStringLiteral("vease")}));
        QCOMPARE(bd.asignatura(), QStringLiteral("Asignatura inventada"));
        // Lo que el JSON no dice no es una diferencia; el nombre, sí.
        QCOMPARE(bd.diferenciasConJson(json), QStringList{QStringLiteral("La asignatura tiene otro nombre en el JSON: «Otra asignatura inventada»")});

        // Un banco sin nombre toma el del JSON; sus temas nuevos, sin forma de la referencia.
        BaseDatos nuevo;
        QVERIFY(nuevo.abrir(m_dir.filePath(QStringLiteral("sin_nombre.db"))));
        QVERIFY(nuevo.importarJson(json, false) > 0);
        QCOMPARE(nuevo.asignatura(), QStringLiteral("Otra asignatura inventada"));
        QCOMPARE(formatos(nuevo.temas()), (QStringList{QString(), QString(), QString()}));
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

    void jsonEnlazadoYDiferencias()
    {
        BaseDatos bd;
        QVERIFY2(abrirConBanco(bd, 3, 10), qPrintable(bd.ultimoError()));
        QVERIFY(bd.jsonEnlazado().isEmpty());
        QVERIFY(!bd.jsonAutomatico());
        // No se puede leer: una sola línea que lo dice.
        QCOMPARE(bd.diferenciasConJson(m_dir.filePath(QStringLiteral("no_existe.json"))).size(), 1);

        // Enlazado, guardado relativo a la base de datos y recuperado al volver a abrirla.
        const QString json = m_dir.filePath(QStringLiteral("enlazado.json"));
        QVERIFY(bd.exportarJson(json));
        QVERIFY(bd.enlazarJson(json));
        QVERIFY(bd.fijarJsonAutomatico(true));
        QCOMPARE(bd.ajuste(QStringLiteral("json_enlazado")), QStringLiteral("enlazado.json"));
        const QString ruta = bd.ruta();
        bd.cerrar();
        QVERIFY(bd.abrir(ruta));
        QCOMPARE(bd.jsonEnlazado(), QFileInfo(json).absoluteFilePath());
        QVERIFY(bd.jsonAutomatico());
        QVERIFY(bd.diferenciasConJson(json).isEmpty());

        // Una pregunta cambiada, otra borrada y otra nueva en la base de datos; un tema renombrado.
        QVector<Pregunta> preguntas = bd.preguntas();
        Pregunta cambiada = preguntas.at(2);
        cambiada.explicacion = QStringLiteral("Explicación añadida.");
        QVERIFY(bd.guardarPregunta(cambiada));
        QVERIFY(bd.borrarPregunta(preguntas.at(5).id));
        Pregunta nueva = preguntas.at(7);
        nueva.id = 0;
        nueva.numero = 99;
        QVERIFY(bd.guardarPregunta(nueva));
        QVector<Tema> temas = bd.temas();
        temas[1].titulo = QStringLiteral("Otro título");
        QVERIFY(bd.guardarTemas(temas, {}));
        const QStringList diferencias = bd.diferenciasConJson(json);
        QCOMPARE(diferencias.size(), 4);
        QCOMPARE(diferencias.at(0), QStringLiteral("1 pregunta distinta: %1").arg(cambiada.codigoCompleto()));
        QCOMPARE(diferencias.at(1), QStringLiteral("1 pregunta solo en la base de datos: %1").arg(nueva.codigoCompleto()));
        QCOMPARE(diferencias.at(2), QStringLiteral("1 pregunta solo en el JSON: %1").arg(preguntas.at(5).codigoCompleto()));
        QCOMPARE(diferencias.at(3), QStringLiteral("1 tema con otro título, autor o forma de la referencia: T2"));
        // Al reescribir el JSON vuelven a coincidir.
        QVERIFY(bd.exportarJson(json));
        QVERIFY(bd.diferenciasConJson(json).isEmpty());
        // También cuentan la forma de la referencia de un tema y el nombre de la asignatura.
        temas = bd.temas();
        temas[2].formatoReferencia.clear();
        QVERIFY(bd.guardarTemas(temas, {}));
        QVERIFY(bd.guardarAsignatura(QStringLiteral("Asignatura inventada")));
        QCOMPARE(bd.diferenciasConJson(json), (QStringList{QStringLiteral("1 tema con otro título, autor o forma de la referencia: T3"),
                                                           QStringLiteral("En el JSON la asignatura no tiene nombre")}));
        QVERIFY(bd.exportarJson(json));
        QVERIFY(bd.diferenciasConJson(json).isEmpty());

        // Con muchas diferencias, la lista de códigos se acorta.
        for (Pregunta p : bd.preguntas())
            if (p.temaNumero == 1) {
                p.notas = QStringLiteral("nota");
                QVERIFY(bd.guardarPregunta(p));
            }
        const QStringList muchas = bd.diferenciasConJson(json);
        QCOMPARE(muchas.size(), 1);
        QVERIFY2(muchas.first().startsWith(QStringLiteral("14 preguntas distintas: ")) && muchas.first().endsWith(QStringLiteral(", …")),
                 qPrintable(muchas.first()));
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

    // Cómo salió cada pregunta en todos los exámenes registrados: la letra de la correcta cada vez, de la convocatoria
    // más reciente a la más antigua, y la más reciente. Sin contar los exámenes que se van a generar de nuevo.
    void historialSalidasAnteriores()
    {
        BaseDatos bd;
        QVERIFY2(abrirConBanco(bd), qPrintable(bd.ultimoError()));
        ConfigExamen c;
        c.preguntasPorTema = {{1, 3}};
        c.nombresModelos = {QStringLiteral("Uno")};
        c.semilla = 5;
        Modelo m = Generador::generar(bd.preguntas(), c).modelos.first();
        const QString codigo = m.preguntas.first().pregunta.codigoCompleto();
        auto registrar = [&](const QString &convocatoria, int anio, const std::array<int, 4> &orden) {
            m.preguntas[0].orden = orden;
            DatosConvocatoria d;
            d.convocatoria = convocatoria;
            d.anio = anio;
            return bd.registrarExamenes({m}, d, 5);
        };
        QVERIFY(registrar(QStringLiteral("Junio"), 2025, {{0, 1, 2, 3}}));
        QVERIFY(registrar(QStringLiteral("Febrero"), 2026, {{1, 2, 3, 0}}));
        QVERIFY(registrar(QStringLiteral("Septiembre"), 2024, {{3, 2, 1, 0}}));
        const int correcta = m.preguntas.first().pregunta.correcta;
        auto letra = [correcta](const std::array<int, 4> &orden) {
            return int(std::find(orden.begin(), orden.end(), correcta) - orden.begin());
        };
        const QHash<QString, SalidaAnterior> salidas = bd.salidasAnteriores(2026, QStringLiteral("Junio"), {});
        QCOMPARE(salidas.value(codigo).convocatoria, QStringLiteral("Febrero 2026"));
        QCOMPARE(salidas.value(codigo).letras, (QVector<int>{letra({{1, 2, 3, 0}}), letra({{0, 1, 2, 3}}), letra({{3, 2, 1, 0}})}));
        QCOMPARE(salidas.size(), 3);   // las tres preguntas del examen
        // El examen «Uno» de Febrero 2026 se va a generar de nuevo: no cuenta.
        const SalidaAnterior sinFebrero = bd.salidasAnteriores(2026, QStringLiteral("Febrero"), {QStringLiteral("Uno")}).value(codigo);
        QCOMPARE(sinFebrero.convocatoria, QStringLiteral("Junio 2025"));
        QCOMPARE(sinFebrero.letras, (QVector<int>{letra({{0, 1, 2, 3}}), letra({{3, 2, 1, 0}})}));
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

        QVERIFY(bd.guardarRepartos({{QStringLiteral("Tercero y cuarto"), {{3, 1}, {4, 2}}}}));
        QVERIFY(bd.guardarUltimoReparto({{1, 1}, {3, 3}, {4, 4}}));

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
        // Los repartos siguen pidiendo las preguntas a los mismos temas.
        QCOMPARE(bd.repartos().first().preguntas, (QMap<int, int>{{4, 1}, {3, 2}}));
        QCOMPARE(bd.ultimoReparto(), (QMap<int, int>{{1, 1}, {4, 3}, {3, 4}}));
    }

    void borrarTemaBorraSusPreguntas()
    {
        BaseDatos bd;
        QVERIFY2(abrirConBanco(bd, 3, 10), qPrintable(bd.ultimoError()));
        QVERIFY(bd.guardarRepartos({{QStringLiteral("Segundo"), {{2, 3}}}, {QStringLiteral("Todos"), {{1, 1}, {2, 2}, {3, 3}}}}));
        QVERIFY(bd.guardarUltimoReparto({{1, 1}, {2, 2}, {3, 3}}));
        QVector<Tema> temas = bd.temas();
        const int borrado = temas.takeAt(1).id;
        QVERIFY(bd.guardarTemas(temas, {borrado}));
        QCOMPARE(bd.temas().size(), 2);
        QCOMPARE(bd.contarPreguntas(borrado), 0);
        for (const Pregunta &p : bd.preguntas())
            QVERIFY(p.temaNumero != 2);
        // Los repartos pierden el tema, y el que solo tenía ese, desaparece.
        QCOMPARE(bd.repartos().size(), 1);
        QCOMPARE(bd.repartos().first().preguntas, (QMap<int, int>{{1, 1}, {3, 3}}));
        QCOMPARE(bd.ultimoReparto(), (QMap<int, int>{{1, 1}, {3, 3}}));
    }
};

QTEST_MAIN(PruebaBaseDatos)
#include "prueba_basedatos.moc"
