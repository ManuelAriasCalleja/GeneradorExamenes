#include "basedatos.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <algorithm>

namespace {
QString nombreConexion(const BaseDatos *bd)
{
    return QStringLiteral("preguntas_%1").arg(quintptr(bd), 0, 16);
}

// Un QString nulo se guardaría como NULL; las columnas de texto son NOT NULL.
QString texto(const QString &s)
{
    return s.isNull() ? QStringLiteral("") : s;
}

// Pregunta <-> JSON con el formato de herramientas/importar_docx.py (sin el tema, que va aparte).
QJsonObject preguntaAJson(const Pregunta &p)
{
    QJsonObject jp;
    jp[QStringLiteral("codigo")] = p.codigo();
    jp[QStringLiteral("numero")] = p.numero;
    jp[QStringLiteral("variante")] = p.variante;
    jp[QStringLiteral("old")] = p.old;
    jp[QStringLiteral("estado")] = p.estado;
    jp[QStringLiteral("revisar")] = p.revisar;
    jp[QStringLiteral("enunciado")] = p.enunciado;
    jp[QStringLiteral("opciones")] = QJsonArray::fromStringList(p.opciones);
    jp[QStringLiteral("correcta")] = QString(Pregunta::letraMayuscula(p.correcta));
    jp[QStringLiteral("referencia")] = p.referencia;
    jp[QStringLiteral("explicacion")] = p.explicacion;
    jp[QStringLiteral("notas")] = p.notas;
    jp[QStringLiteral("pseudo_con")] = p.pseudoCon.isEmpty() ? QJsonValue() : QJsonValue(p.pseudoCon);
    jp[QStringLiteral("imagen")] = p.imagen.isEmpty() ? QJsonValue() : QJsonValue(QString::fromLatin1(p.imagen.toBase64()));
    return jp;
}

Pregunta preguntaDeJson(const QJsonObject &jp)
{
    Pregunta p;
    p.numero = jp.value(QStringLiteral("numero")).toInt();
    p.variante = jp.value(QStringLiteral("variante")).toString();
    p.old = jp.value(QStringLiteral("old")).toBool();
    p.estado = jp.value(QStringLiteral("estado")).toString(QStringLiteral("activa"));
    p.revisar = jp.value(QStringLiteral("revisar")).toBool();
    p.enunciado = jp.value(QStringLiteral("enunciado")).toString();
    const QJsonArray ops = jp.value(QStringLiteral("opciones")).toArray();
    for (int i = 0; i < 4; ++i)
        p.opciones[i] = ops.at(i).toString();
    const QString correcta = jp.value(QStringLiteral("correcta")).toString().toUpper();
    p.correcta = correcta.isEmpty() ? 0 : qBound(0, correcta.at(0).unicode() - 'A', 3);
    p.referencia = jp.value(QStringLiteral("referencia")).toString();
    p.explicacion = jp.value(QStringLiteral("explicacion")).toString();
    p.notas = jp.value(QStringLiteral("notas")).toString();
    p.pseudoCon = jp.value(QStringLiteral("pseudo_con")).toString();
    p.imagen = QByteArray::fromBase64(jp.value(QStringLiteral("imagen")).toString().toLatin1());
    return p;
}

using ClaveConvocatoria = QPair<int, QString>;   // año y convocatoria

// Mes de una convocatoria que se llama como un mes («Junio», «Septiembre (reserva)»...); 0 si no.
int mesDe(const QString &convocatoria)
{
    static const QStringList meses{
        QStringLiteral("enero"), QStringLiteral("febrero"), QStringLiteral("marzo"), QStringLiteral("abril"),
        QStringLiteral("mayo"), QStringLiteral("junio"), QStringLiteral("julio"), QStringLiteral("agosto"),
        QStringLiteral("septiembre"), QStringLiteral("octubre"), QStringLiteral("noviembre"), QStringLiteral("diciembre")};
    const QString c = convocatoria.trimmed().toLower();
    for (int i = 0; i < meses.size(); ++i)
        if (c.startsWith(meses.at(i)))
            return i + 1;
    return 0;
}

// Convocatorias de la más reciente a la más antigua: por año, por mes y, si no se sabe el mes, por el último
// examen registrado de cada una ('ultimoId').
void ordenarConvocatorias(QList<ClaveConvocatoria> &claves, const QHash<ClaveConvocatoria, int> &ultimoId)
{
    std::sort(claves.begin(), claves.end(), [&](const ClaveConvocatoria &a, const ClaveConvocatoria &b) {
        if (a.first != b.first)
            return a.first > b.first;
        if (mesDe(a.second) != mesDe(b.second))
            return mesDe(a.second) > mesDe(b.second);
        return ultimoId.value(a) > ultimoId.value(b);
    });
}
} // namespace

BaseDatos::BaseDatos() = default;

BaseDatos::~BaseDatos()
{
    cerrar();
}

bool BaseDatos::abierta() const
{
    return m_db.isValid() && m_db.isOpen();
}

void BaseDatos::cerrar()
{
    if (m_db.isValid()) {
        m_db.close();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(nombreConexion(this));
    }
    m_ruta.clear();
}

bool BaseDatos::abrir(const QString &ruta)
{
    cerrar();
    QDir().mkpath(QFileInfo(ruta).absolutePath());
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), nombreConexion(this));
    m_db.setDatabaseName(ruta);
    if (!m_db.open()) {
        m_error = m_db.lastError().text();
        cerrar();
        return false;
    }
    m_ruta = ruta;
    ejecutar(QStringLiteral("PRAGMA foreign_keys = ON"));
    return crearEsquema();
}

bool BaseDatos::ejecutar(const QString &sql)
{
    QSqlQuery q(m_db);
    if (!q.exec(sql)) {
        m_error = q.lastError().text();
        return false;
    }
    return true;
}

bool BaseDatos::crearEsquema()
{
    return ejecutar(QStringLiteral(
               "CREATE TABLE IF NOT EXISTS temas ("
               " id INTEGER PRIMARY KEY AUTOINCREMENT,"
               " numero INTEGER NOT NULL UNIQUE,"
               " titulo TEXT NOT NULL,"
               " autor TEXT NOT NULL DEFAULT '')"))
        && ejecutar(QStringLiteral(
               "CREATE TABLE IF NOT EXISTS preguntas ("
               " id INTEGER PRIMARY KEY AUTOINCREMENT,"
               " tema_id INTEGER NOT NULL REFERENCES temas(id) ON DELETE CASCADE,"
               " numero INTEGER NOT NULL,"
               " variante TEXT NOT NULL DEFAULT '',"
               " old INTEGER NOT NULL DEFAULT 0,"
               " estado TEXT NOT NULL DEFAULT 'activa' CHECK (estado IN ('activa','anulada','obsoleta')),"
               " revisar INTEGER NOT NULL DEFAULT 0,"
               " enunciado TEXT NOT NULL,"
               " opcion_a TEXT NOT NULL, opcion_b TEXT NOT NULL,"
               " opcion_c TEXT NOT NULL, opcion_d TEXT NOT NULL,"
               " correcta TEXT NOT NULL CHECK (correcta IN ('A','B','C','D')),"
               " referencia TEXT NOT NULL DEFAULT '',"
               " explicacion TEXT NOT NULL DEFAULT '',"
               " notas TEXT NOT NULL DEFAULT '',"
               " pseudo_con TEXT NOT NULL DEFAULT '',"
               " imagen BLOB,"
               " UNIQUE (tema_id, numero, variante))"))
        && ejecutar(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_preguntas_tema ON preguntas(tema_id, numero)"))
        // Historial de convocatorias ('registrado' es la fecha y hora en ISO 8601; 'orden', el de las opciones
        // mostradas, p. ej. "2031"; 'pregunta', la copia en JSON de la pregunta tal como salió).
        && ejecutar(QStringLiteral(
               "CREATE TABLE IF NOT EXISTS examenes_registrados ("
               " id INTEGER PRIMARY KEY AUTOINCREMENT,"
               " anio INTEGER NOT NULL,"
               " convocatoria TEXT NOT NULL,"
               " nombre TEXT NOT NULL,"
               " registrado TEXT NOT NULL,"
               " semilla INTEGER NOT NULL,"
               " asignatura TEXT NOT NULL,"
               " instrucciones TEXT NOT NULL,"
               " duracion INTEGER NOT NULL,"
               " puntos_acierto REAL NOT NULL,"
               " puntos_fallo REAL NOT NULL,"
               " UNIQUE (anio, convocatoria, nombre))"))
        && ejecutar(QStringLiteral(
               "CREATE TABLE IF NOT EXISTS preguntas_registradas ("
               " examen_id INTEGER NOT NULL REFERENCES examenes_registrados(id) ON DELETE CASCADE,"
               " posicion INTEGER NOT NULL,"
               " codigo TEXT NOT NULL,"
               " orden TEXT NOT NULL,"
               " pregunta TEXT NOT NULL,"
               " PRIMARY KEY (examen_id, posicion))"))
        && ejecutar(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_registradas_codigo ON preguntas_registradas(codigo)"));
}

QVector<Tema> BaseDatos::temas()
{
    QVector<Tema> res;
    QSqlQuery q(m_db);
    q.exec(QStringLiteral("SELECT id, numero, titulo, autor FROM temas ORDER BY numero"));
    while (q.next()) {
        Tema t;
        t.id = q.value(0).toInt();
        t.numero = q.value(1).toInt();
        t.titulo = q.value(2).toString();
        t.autor = q.value(3).toString();
        res.append(t);
    }
    return res;
}

bool BaseDatos::guardarTema(Tema &tema)
{
    QSqlQuery q(m_db);
    if (tema.id == 0) {
        q.prepare(QStringLiteral("INSERT INTO temas (numero, titulo, autor) VALUES (?, ?, ?)"));
    } else {
        q.prepare(QStringLiteral("UPDATE temas SET numero = ?, titulo = ?, autor = ? WHERE id = ?"));
    }
    q.addBindValue(tema.numero);
    q.addBindValue(tema.titulo.isNull() ? QStringLiteral("") : tema.titulo);
    q.addBindValue(tema.autor.isNull() ? QStringLiteral("") : tema.autor);
    if (tema.id != 0)
        q.addBindValue(tema.id);
    if (!q.exec()) {
        m_error = q.lastError().text();
        return false;
    }
    if (tema.id == 0)
        tema.id = q.lastInsertId().toInt();
    return true;
}

bool BaseDatos::guardarTemas(QVector<Tema> &nuevos, const QVector<int> &borrados)
{
    QHash<int, int> numeroAntes;   // id del tema -> número que tenía
    for (const Tema &t : temas())
        numeroAntes.insert(t.id, t.numero);
    auto fallar = [this]() {
        m_db.rollback();
        return false;
    };
    m_db.transaction();
    for (int id : borrados)
        if (!borrarTema(id))
            return fallar();
    // Se renumeran en dos pasos para no chocar con la restricción UNIQUE si se intercambian números.
    for (const Tema &t : std::as_const(nuevos)) {
        if (t.id) {
            Tema provisional = t;
            provisional.numero = -t.numero;
            if (!guardarTema(provisional))
                return fallar();
        }
    }
    QVector<Tema> guardados = nuevos;
    QHash<int, int> cambio;   // número antes -> número después
    for (Tema &t : guardados) {
        if (t.id && numeroAntes.contains(t.id) && numeroAntes.value(t.id) != t.numero)
            cambio.insert(numeroAntes.value(t.id), t.numero);
        if (!guardarTema(t))
            return fallar();
    }

    if (!cambio.isEmpty()) {
        auto renumerar = [&cambio](const QString &codigo) {
            static const QRegularExpression re(QStringLiteral("^T(\\d+)-"), QRegularExpression::CaseInsensitiveOption);
            const QRegularExpressionMatch m = re.match(codigo.trimmed());
            if (!m.hasMatch() || !cambio.contains(m.captured(1).toInt()))
                return codigo;
            return QStringLiteral("T%1-").arg(cambio.value(m.captured(1).toInt())) + codigo.trimmed().mid(m.capturedEnd());
        };
        QSqlQuery q(m_db);
        QVector<QPair<int, QString>> enlaces;
        q.exec(QStringLiteral("SELECT id, pseudo_con FROM preguntas WHERE pseudo_con <> ''"));
        while (q.next())
            if (renumerar(q.value(1).toString()) != q.value(1).toString())
                enlaces.append({q.value(0).toInt(), renumerar(q.value(1).toString())});
        for (const auto &e : enlaces) {
            q.prepare(QStringLiteral("UPDATE preguntas SET pseudo_con = ? WHERE id = ?"));
            q.addBindValue(e.second);
            q.addBindValue(e.first);
            if (!q.exec()) {
                m_error = q.lastError().text();
                return fallar();
            }
        }
        struct Registrada
        {
            int examen, posicion;
            QString codigo;
        };
        QVector<Registrada> registradas;
        q.exec(QStringLiteral("SELECT examen_id, posicion, codigo FROM preguntas_registradas"));
        while (q.next())
            if (renumerar(q.value(2).toString()) != q.value(2).toString())
                registradas.append({q.value(0).toInt(), q.value(1).toInt(), renumerar(q.value(2).toString())});
        for (const Registrada &r : registradas) {
            q.prepare(QStringLiteral("UPDATE preguntas_registradas SET codigo = ? WHERE examen_id = ? AND posicion = ?"));
            q.addBindValue(r.codigo);
            q.addBindValue(r.examen);
            q.addBindValue(r.posicion);
            if (!q.exec()) {
                m_error = q.lastError().text();
                return fallar();
            }
        }
    }
    if (!m_db.commit()) {
        m_error = m_db.lastError().text();
        return fallar();
    }
    nuevos = guardados;
    return true;
}

bool BaseDatos::borrarTema(int id)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM temas WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        m_error = q.lastError().text();
        return false;
    }
    return true;
}

int BaseDatos::contarPreguntas(int temaId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM preguntas WHERE tema_id = ?"));
    q.addBindValue(temaId);
    if (q.exec() && q.next())
        return q.value(0).toInt();
    return 0;
}

QVector<Pregunta> BaseDatos::preguntas()
{
    QVector<Pregunta> res;
    QSqlQuery q(m_db);
    q.exec(QStringLiteral(
        "SELECT p.id, p.tema_id, t.numero, p.numero, p.variante, p.old, p.estado, p.revisar,"
        " p.enunciado, p.opcion_a, p.opcion_b, p.opcion_c, p.opcion_d, p.correcta,"
        " p.referencia, p.explicacion, p.notas, p.pseudo_con, p.imagen"
        " FROM preguntas p JOIN temas t ON t.id = p.tema_id"
        " ORDER BY t.numero, p.numero, p.variante"));
    while (q.next()) {
        Pregunta p;
        p.id = q.value(0).toInt();
        p.temaId = q.value(1).toInt();
        p.temaNumero = q.value(2).toInt();
        p.numero = q.value(3).toInt();
        p.variante = q.value(4).toString();
        p.old = q.value(5).toBool();
        p.estado = q.value(6).toString();
        p.revisar = q.value(7).toBool();
        p.enunciado = q.value(8).toString();
        for (int i = 0; i < 4; ++i)
            p.opciones[i] = q.value(9 + i).toString();
        p.correcta = q.value(13).toString().at(0).unicode() - 'A';
        p.referencia = q.value(14).toString();
        p.explicacion = q.value(15).toString();
        p.notas = q.value(16).toString();
        p.pseudoCon = q.value(17).toString();
        p.imagen = q.value(18).toByteArray();
        res.append(p);
    }
    return res;
}

bool BaseDatos::guardarPregunta(Pregunta &p)
{
    QSqlQuery q(m_db);
    const QString campos = QStringLiteral(
        "tema_id, numero, variante, old, estado, revisar, enunciado, opcion_a, opcion_b, opcion_c, opcion_d,"
        " correcta, referencia, explicacion, notas, pseudo_con, imagen");
    if (p.id == 0) {
        q.prepare(QStringLiteral("INSERT INTO preguntas (%1) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)").arg(campos));
    } else {
        q.prepare(QStringLiteral(
            "UPDATE preguntas SET tema_id=?, numero=?, variante=?, old=?, estado=?, revisar=?, enunciado=?,"
            " opcion_a=?, opcion_b=?, opcion_c=?, opcion_d=?, correcta=?, referencia=?, explicacion=?,"
            " notas=?, pseudo_con=?, imagen=? WHERE id=?"));
    }
    q.addBindValue(p.temaId);
    q.addBindValue(p.numero);
    q.addBindValue(texto(p.variante));
    q.addBindValue(p.old ? 1 : 0);
    q.addBindValue(texto(p.estado));
    q.addBindValue(p.revisar ? 1 : 0);
    q.addBindValue(texto(p.enunciado));
    for (int i = 0; i < 4; ++i)
        q.addBindValue(texto(p.opciones.value(i)));
    q.addBindValue(QString(Pregunta::letraMayuscula(p.correcta)));
    q.addBindValue(texto(p.referencia));
    q.addBindValue(texto(p.explicacion));
    q.addBindValue(texto(p.notas));
    q.addBindValue(texto(p.pseudoCon));
    q.addBindValue(p.imagen.isEmpty() ? QVariant(QMetaType(QMetaType::QByteArray)) : QVariant(p.imagen));
    if (p.id != 0)
        q.addBindValue(p.id);
    if (!q.exec()) {
        m_error = q.lastError().text();
        if (m_error.contains(QLatin1String("UNIQUE")))
            m_error = QStringLiteral("Ya existe una pregunta %1 en ese tema.").arg(p.codigo());
        return false;
    }
    if (p.id == 0)
        p.id = q.lastInsertId().toInt();
    return true;
}

bool BaseDatos::borrarPregunta(int id)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM preguntas WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        m_error = q.lastError().text();
        return false;
    }
    return true;
}

int BaseDatos::idTema(int numero, const QString &titulo, const QString &autor, bool actualizar)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id FROM temas WHERE numero = ?"));
    q.addBindValue(numero);
    if (q.exec() && q.next()) {
        const int id = q.value(0).toInt();
        if (actualizar) {
            QSqlQuery u(m_db);
            u.prepare(QStringLiteral("UPDATE temas SET titulo = ?, autor = ? WHERE id = ?"));
            u.addBindValue(titulo);
            u.addBindValue(autor.isNull() ? QStringLiteral("") : autor);
            u.addBindValue(id);
            u.exec();
        }
        return id;
    }
    Tema t;
    t.numero = numero;
    t.titulo = titulo;
    t.autor = autor;
    return guardarTema(t) ? t.id : 0;
}

int BaseDatos::importarJson(const QString &ruta, bool reemplazar)
{
    QFile f(ruta);
    if (!f.open(QIODevice::ReadOnly)) {
        m_error = QStringLiteral("No se puede abrir %1: %2").arg(ruta, f.errorString());
        return -1;
    }
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (doc.isNull()) {
        m_error = QStringLiteral("JSON no válido: %1").arg(err.errorString());
        return -1;
    }

    m_db.transaction();
    if (reemplazar && !(ejecutar(QStringLiteral("DELETE FROM preguntas")) && ejecutar(QStringLiteral("DELETE FROM temas")))) {
        m_db.rollback();
        return -1;
    }

    int n = 0;
    const QJsonArray temas = doc.object().value(QStringLiteral("temas")).toArray();
    for (const QJsonValue &vt : temas) {
        const QJsonObject jt = vt.toObject();
        const int temaId = idTema(jt.value(QStringLiteral("numero")).toInt(),
                                  jt.value(QStringLiteral("titulo")).toString(),
                                  jt.value(QStringLiteral("autor")).toString(), true);
        if (temaId == 0) {
            m_db.rollback();
            return -1;
        }
        for (const QJsonValue &vp : jt.value(QStringLiteral("preguntas")).toArray()) {
            Pregunta p = preguntaDeJson(vp.toObject());
            p.temaId = temaId;

            // Si ya existe (mismo tema, número y variante) se actualiza.
            QSqlQuery q(m_db);
            q.prepare(QStringLiteral("SELECT id FROM preguntas WHERE tema_id = ? AND numero = ? AND variante = ?"));
            q.addBindValue(temaId);
            q.addBindValue(p.numero);
            q.addBindValue(p.variante);
            if (q.exec() && q.next())
                p.id = q.value(0).toInt();
            if (!guardarPregunta(p)) {
                m_error = QStringLiteral("Tema %1, pregunta %2: %3").arg(jt.value(QStringLiteral("numero")).toInt()).arg(p.codigo(), m_error);
                m_db.rollback();
                return -1;
            }
            ++n;
        }
    }
    m_db.commit();
    return n;
}

bool BaseDatos::exportarJson(const QString &ruta)
{
    const QVector<Pregunta> todas = preguntas();
    QJsonArray jtemas;
    for (const Tema &t : temas()) {
        QJsonArray jpreguntas;
        for (const Pregunta &p : todas)
            if (p.temaId == t.id)
                jpreguntas.append(preguntaAJson(p));
        QJsonObject jt;
        jt[QStringLiteral("numero")] = t.numero;
        jt[QStringLiteral("titulo")] = t.titulo;
        jt[QStringLiteral("autor")] = t.autor;
        jt[QStringLiteral("preguntas")] = jpreguntas;
        jtemas.append(jt);
    }
    QJsonObject raiz;
    raiz[QStringLiteral("asignatura")] = QStringLiteral("Fundamentos de Informática");
    raiz[QStringLiteral("temas")] = jtemas;

    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_error = f.errorString();
        return false;
    }
    f.write(QJsonDocument(raiz).toJson(QJsonDocument::Indented));
    return true;
}

// ---------------------------------------------------------------------------------------------
// Historial de convocatorias

bool BaseDatos::registrarExamenes(const QVector<Modelo> &modelos, const DatosConvocatoria &datos, quint32 semilla)
{
    auto fallar = [this](const QSqlQuery &q) {
        m_error = q.lastError().text();
        m_db.rollback();
        return false;
    };
    const QString ahora = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_db.transaction();
    for (const Modelo &m : modelos) {
        QSqlQuery q(m_db);
        q.prepare(QStringLiteral("DELETE FROM examenes_registrados WHERE anio = ? AND convocatoria = ? AND nombre = ?"));
        q.addBindValue(datos.anio);
        q.addBindValue(texto(datos.convocatoria));
        q.addBindValue(texto(m.nombre));
        if (!q.exec())
            return fallar(q);
        q.prepare(QStringLiteral(
            "INSERT INTO examenes_registrados (anio, convocatoria, nombre, registrado, semilla, asignatura, instrucciones,"
            " duracion, puntos_acierto, puntos_fallo) VALUES (?,?,?,?,?,?,?,?,?,?)"));
        q.addBindValue(datos.anio);
        q.addBindValue(texto(datos.convocatoria));
        q.addBindValue(texto(m.nombre));
        q.addBindValue(ahora);
        q.addBindValue(qint64(semilla));
        q.addBindValue(texto(datos.asignatura));
        q.addBindValue(texto(datos.instrucciones));
        q.addBindValue(datos.duracionMinutos);
        q.addBindValue(datos.puntosAcierto);
        q.addBindValue(datos.puntosFallo);
        if (!q.exec())
            return fallar(q);
        const int examenId = q.lastInsertId().toInt();
        for (int i = 0; i < m.preguntas.size(); ++i) {
            const PreguntaExamen &pe = m.preguntas.at(i);
            QJsonObject jp = preguntaAJson(pe.pregunta);
            jp[QStringLiteral("id")] = pe.pregunta.id;
            jp[QStringLiteral("tema")] = pe.pregunta.temaNumero;
            QString orden;
            for (int k : pe.orden)
                orden += QString::number(k);
            q.prepare(QStringLiteral(
                "INSERT INTO preguntas_registradas (examen_id, posicion, codigo, orden, pregunta) VALUES (?,?,?,?,?)"));
            q.addBindValue(examenId);
            q.addBindValue(i);
            q.addBindValue(pe.pregunta.codigoCompleto());
            q.addBindValue(orden);
            q.addBindValue(QString::fromUtf8(QJsonDocument(jp).toJson(QJsonDocument::Compact)));
            if (!q.exec())
                return fallar(q);
        }
    }
    m_db.commit();
    return true;
}

QVector<ExamenRegistrado> BaseDatos::examenesRegistrados()
{
    QVector<ExamenRegistrado> res;
    QHash<ClaveConvocatoria, int> ultimoId;
    QSqlQuery q(m_db);
    q.exec(QStringLiteral(
        "SELECT e.id, e.nombre, e.registrado, e.semilla, e.anio, e.convocatoria, e.asignatura, e.instrucciones,"
        " e.duracion, e.puntos_acierto, e.puntos_fallo,"
        " (SELECT COUNT(*) FROM preguntas_registradas p WHERE p.examen_id = e.id)"
        " FROM examenes_registrados e ORDER BY e.id"));
    while (q.next()) {
        ExamenRegistrado e;
        e.id = q.value(0).toInt();
        e.nombre = q.value(1).toString();
        e.registrado = QDateTime::fromString(q.value(2).toString(), Qt::ISODate);
        e.semilla = quint32(q.value(3).toLongLong());
        e.datos.anio = q.value(4).toInt();
        e.datos.convocatoria = q.value(5).toString();
        e.datos.asignatura = q.value(6).toString();
        e.datos.instrucciones = q.value(7).toString();
        e.datos.duracionMinutos = q.value(8).toInt();
        e.datos.puntosAcierto = q.value(9).toDouble();
        e.datos.puntosFallo = q.value(10).toDouble();
        e.numPreguntas = q.value(11).toInt();
        ultimoId[ClaveConvocatoria(e.datos.anio, e.datos.convocatoria)] = e.id;
        res.append(e);
    }

    // Las convocatorias, de la más reciente a la más antigua; dentro de cada una, los exámenes en el orden en que
    // se registraron.
    QList<ClaveConvocatoria> orden = ultimoId.keys();
    ordenarConvocatorias(orden, ultimoId);
    std::stable_sort(res.begin(), res.end(), [&](const ExamenRegistrado &a, const ExamenRegistrado &b) {
        return orden.indexOf(ClaveConvocatoria(a.datos.anio, a.datos.convocatoria))
             < orden.indexOf(ClaveConvocatoria(b.datos.anio, b.datos.convocatoria));
    });
    return res;
}

Modelo BaseDatos::modeloRegistrado(int examenId)
{
    Modelo m;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT nombre FROM examenes_registrados WHERE id = ?"));
    q.addBindValue(examenId);
    if (q.exec() && q.next())
        m.nombre = q.value(0).toString();
    q.prepare(QStringLiteral("SELECT orden, pregunta FROM preguntas_registradas WHERE examen_id = ? ORDER BY posicion"));
    q.addBindValue(examenId);
    q.exec();
    while (q.next()) {
        const QJsonObject jp = QJsonDocument::fromJson(q.value(1).toString().toUtf8()).object();
        PreguntaExamen pe;
        pe.pregunta = preguntaDeJson(jp);
        pe.pregunta.id = jp.value(QStringLiteral("id")).toInt();
        pe.pregunta.temaNumero = jp.value(QStringLiteral("tema")).toInt();
        const QString orden = q.value(0).toString();
        for (int k = 0; k < 4 && k < orden.size(); ++k)
            pe.orden[k] = qBound(0, orden.at(k).digitValue(), 3);
        m.preguntas.append(pe);
    }
    return m;
}

bool BaseDatos::borrarExamenRegistrado(int examenId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM examenes_registrados WHERE id = ?"));
    q.addBindValue(examenId);
    if (!q.exec()) {
        m_error = q.lastError().text();
        return false;
    }
    return true;
}

QHash<QString, int> BaseDatos::usoReciente(int cuantas, int anio, const QString &convocatoria, const QStringList &excluidos)
{
    struct Uso
    {
        QString codigo;
        ClaveConvocatoria convocatoria;
    };
    QVector<Uso> usos;
    QHash<ClaveConvocatoria, int> ultimoId;
    QSqlQuery q(m_db);
    q.exec(QStringLiteral(
        "SELECT p.codigo, e.anio, e.convocatoria, e.nombre, e.id"
        " FROM preguntas_registradas p JOIN examenes_registrados e ON e.id = p.examen_id"));
    while (q.next()) {
        const ClaveConvocatoria clave(q.value(1).toInt(), q.value(2).toString());
        if (clave == ClaveConvocatoria(anio, convocatoria) && excluidos.contains(q.value(3).toString()))
            continue;
        ultimoId[clave] = qMax(ultimoId.value(clave), q.value(4).toInt());
        usos.append({q.value(0).toString(), clave});
    }

    QList<ClaveConvocatoria> orden = ultimoId.keys();
    ordenarConvocatorias(orden, ultimoId);
    QHash<ClaveConvocatoria, int> antiguedad;
    for (int i = 0; i < qMin(cuantas, int(orden.size())); ++i)
        antiguedad.insert(orden.at(i), i + 1);

    QHash<QString, int> res;
    for (const Uso &u : std::as_const(usos)) {
        const int a = antiguedad.value(u.convocatoria);
        if (a > 0 && (!res.contains(u.codigo) || a < res.value(u.codigo)))
            res.insert(u.codigo, a);
    }
    return res;
}

QStringList BaseDatos::usosDe(const QString &codigo)
{
    QStringList res;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT e.convocatoria, e.anio, e.nombre FROM preguntas_registradas p JOIN examenes_registrados e ON e.id = p.examen_id"
        " WHERE p.codigo = ? ORDER BY e.anio DESC, e.id DESC"));
    q.addBindValue(codigo);
    q.exec();
    while (q.next())
        res << QStringLiteral("%1 %2 · %3").arg(q.value(0).toString()).arg(q.value(1).toInt()).arg(q.value(2).toString());
    return res;
}
