#include "basedatos.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
QString nombreConexion(const BaseDatos *bd)
{
    return QStringLiteral("preguntas_%1").arg(quintptr(bd), 0, 16);
}
}

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
    if (!crearEsquema())
        return false;

    QSqlQuery q(m_db);
    // Base de datos vacía: se rellena con las preguntas incluidas en el ejecutable, si las lleva (no están en git).
    if (QFile::exists(rutaJsonIncluido()) && q.exec(QStringLiteral("SELECT COUNT(*) FROM preguntas")) && q.next()
        && q.value(0).toInt() == 0) {
        if (importarJson(rutaJsonIncluido(), false) < 0)
            return false;
    }
    return true;
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
        && ejecutar(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_preguntas_tema ON preguntas(tema_id, numero)"));
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
    // Un QString nulo se guardaría como NULL; las columnas de texto son NOT NULL.
    auto texto = [](const QString &s) { return s.isNull() ? QStringLiteral("") : s; };
    q.addBindValue(p.temaId);
    q.addBindValue(p.numero);
    q.addBindValue(texto(p.variante));
    q.addBindValue(p.old ? 1 : 0);
    q.addBindValue(texto(p.estado));
    q.addBindValue(p.revisar ? 1 : 0);
    q.addBindValue(texto(p.enunciado));
    for (int i = 0; i < 4; ++i)
        q.addBindValue(texto(p.opciones.value(i)));
    q.addBindValue(QString(QChar('A' + p.correcta)));
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
            const QJsonObject jp = vp.toObject();
            Pregunta p;
            p.temaId = temaId;
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
        for (const Pregunta &p : todas) {
            if (p.temaId != t.id)
                continue;
            QJsonObject jp;
            jp[QStringLiteral("codigo")] = p.codigo();
            jp[QStringLiteral("numero")] = p.numero;
            jp[QStringLiteral("variante")] = p.variante;
            jp[QStringLiteral("old")] = p.old;
            jp[QStringLiteral("estado")] = p.estado;
            jp[QStringLiteral("revisar")] = p.revisar;
            jp[QStringLiteral("enunciado")] = p.enunciado;
            jp[QStringLiteral("opciones")] = QJsonArray::fromStringList(p.opciones);
            jp[QStringLiteral("correcta")] = QString(QChar('A' + p.correcta));
            jp[QStringLiteral("referencia")] = p.referencia;
            jp[QStringLiteral("explicacion")] = p.explicacion;
            jp[QStringLiteral("notas")] = p.notas;
            jp[QStringLiteral("pseudo_con")] = p.pseudoCon.isEmpty() ? QJsonValue() : QJsonValue(p.pseudoCon);
            jp[QStringLiteral("imagen")] = p.imagen.isEmpty() ? QJsonValue() : QJsonValue(QString::fromLatin1(p.imagen.toBase64()));
            jpreguntas.append(jp);
        }
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
