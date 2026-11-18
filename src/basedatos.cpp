#include "basedatos.h"
#include "comprobadorjson.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <algorithm>

namespace {
// Versión del esquema de la base de datos (PRAGMA user_version): ver BaseDatos::actualizarEsquema().
constexpr int kVersionEsquema = 2;

QString nombreConexion(const BaseDatos *bd)
{
    return QStringLiteral("preguntas_%1").arg(quintptr(bd), 0, 16);
}

// Un texto como se guarda: sin los U+FEFF del principio (la marca de orden de bytes del UTF-16, invisible), que SQLite
// quita de uno en uno cada vez que guarda (el controlador de Qt le pasa los textos en UTF-16), con lo que un texto no se
// quedaría igual al pasar por el JSON y volver; y nunca nulo, que se guardaría como NULL (las columnas son NOT NULL).
QString texto(const QString &s)
{
    qsizetype inicio = 0;
    while (inicio < s.size() && s.at(inicio) == QChar(0xFEFF))
        ++inicio;
    return s.isNull() ? QStringLiteral("") : s.mid(inicio);
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
    // La verificación, siempre: un JSON que no la dice (uno escrito a mano o por una IA) no cambia la del banco (ver
    // importarJson()), pero uno del programa sí.
    jp[QStringLiteral("verificada")] = p.verificada;
    if (p.verificada)
        jp[QStringLiteral("verificadores")] = QJsonArray::fromStringList(p.verificadores);
    return jp;
}

// Lo que se compara entre el banco y su JSON: la pregunta entera menos los id (que cambian de una base de datos a
// otra).
QString huella(const Pregunta &p)
{
    QStringList verificadores = p.verificadores;
    verificadores.sort(Qt::CaseInsensitive);
    return QStringList{p.enunciado, p.opciones.join(QLatin1Char('|')), QString::number(p.correcta), p.referencia, p.estado,
                       QString::number(p.old), QString::number(p.revisar), p.pseudoCon, p.explicacion, p.notas,
                       QString::fromLatin1(p.imagen.toBase64()), QString::number(p.verificada),
                       verificadores.join(QLatin1Char('|'))}
        .join(QLatin1Char('#'));
}

QString huella(const Tema &t)
{
    return QStringList{t.titulo, t.autor, t.formatoReferencia}.join(QLatin1Char('#'));
}

// Lo que hay en un JSON con el formato de herramientas/importar_docx.py.
struct ContenidoJson
{
    bool diceAsignatura = false;      // si el JSON lleva el nombre de la asignatura (aunque sea vacío)
    QString asignatura;
    QString idioma;                   // el de los exámenes; vacío si el JSON no lo dice
    QVector<Tema> temas;              // sin id
    QSet<int> temasSinFormato;        // números de los temas que no dicen la forma de su referencia
    QSet<int> temasSinTitulo;         // … su título
    QSet<int> temasSinAutor;          // … su autor
    QVector<Pregunta> preguntas;      // con el número de su tema, sin id
    QSet<QString> sinVerificacion;    // códigos (T1-7a) de las preguntas que no dicen si están verificadas
    QStringList verificadores;        // la lista del banco, si el JSON la dice
};

// Si falla, deja el motivo en 'error'.
bool leerJson(const QString &ruta, ContenidoJson *contenido, QString *error);

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
    p.verificada = jp.value(QStringLiteral("verificada")).toBool();
    if (p.verificada)
        for (const QJsonValue &v : jp.value(QStringLiteral("verificadores")).toArray())
            if (!v.toString().trimmed().isEmpty())
                p.verificadores << v.toString().trimmed();
    return p;
}

bool leerJson(const QString &ruta, ContenidoJson *contenido, QString *error)
{
    QFile f(ruta);
    if (!f.open(QIODevice::ReadOnly)) {
        *error = BaseDatos::tr("No se puede abrir %1: %2").arg(ruta, f.errorString());
        return false;
    }
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (doc.isNull()) {
        *error = BaseDatos::tr("JSON no válido: %1").arg(err.errorString());
        return false;
    }
    contenido->diceAsignatura = doc.object().contains(QStringLiteral("asignatura"));
    contenido->asignatura = doc.object().value(QStringLiteral("asignatura")).toString().trimmed();
    contenido->idioma = doc.object().value(QStringLiteral("idioma")).toString().trimmed();
    for (const QJsonValue &v : doc.object().value(QStringLiteral("verificadores")).toArray())
        if (!v.toString().trimmed().isEmpty())
            contenido->verificadores << v.toString().trimmed();
    const QJsonArray jtemas = doc.object().value(QStringLiteral("temas")).toArray();
    for (const QJsonValue &vt : jtemas) {
        const QJsonObject jt = vt.toObject();
        Tema t;
        t.numero = jt.value(QStringLiteral("numero")).toInt();
        t.titulo = jt.value(QStringLiteral("titulo")).toString();
        t.autor = jt.value(QStringLiteral("autor")).toString();
        if (!jt.contains(QStringLiteral("titulo")))
            contenido->temasSinTitulo.insert(t.numero);
        if (!jt.contains(QStringLiteral("autor")))
            contenido->temasSinAutor.insert(t.numero);
        if (jt.contains(QStringLiteral("formato_referencia")))
            t.formatoReferencia = jt.value(QStringLiteral("formato_referencia")).toString();
        else
            contenido->temasSinFormato.insert(t.numero);
        contenido->temas.append(t);
        for (const QJsonValue &vp : jt.value(QStringLiteral("preguntas")).toArray()) {
            Pregunta p = preguntaDeJson(vp.toObject());
            p.temaNumero = t.numero;
            if (!vp.toObject().contains(QStringLiteral("verificada")))
                contenido->sinVerificacion.insert(p.codigoCompleto());
            contenido->preguntas.append(p);
        }
    }
    return true;
}

// Transacción que se deshace sola si no se llega a confirmar (al salir de una función por un error).
class Transaccion
{
public:
    explicit Transaccion(QSqlDatabase &db)
        : m_db(db)
    {
        m_db.transaction();
    }
    ~Transaccion()
    {
        if (!m_confirmada)
            m_db.rollback();
    }
    bool confirmar()
    {
        m_confirmada = m_db.commit();
        return m_confirmada;
    }

private:
    QSqlDatabase &m_db;
    bool m_confirmada = false;
};

// Reparto (número de tema -> preguntas) <-> JSON, como se guarda en los ajustes del banco: {"1": 3, "2": 2}.
QJsonObject repartoAJson(const QMap<int, int> &reparto)
{
    QJsonObject o;
    for (auto it = reparto.cbegin(); it != reparto.cend(); ++it)
        o[QString::number(it.key())] = it.value();
    return o;
}

QMap<int, int> repartoDeJson(const QJsonObject &o)
{
    QMap<int, int> reparto;
    for (auto it = o.constBegin(); it != o.constEnd(); ++it)
        reparto.insert(it.key().toInt(), it.value().toInt());
    return reparto;
}

using ClaveConvocatoria = QPair<int, QString>;   // año y convocatoria

// Convocatorias de la más reciente a la más antigua: por año, por mes y, si no se sabe el mes, por el último
// examen registrado de cada una ('ultimoId').
void ordenarConvocatorias(QList<ClaveConvocatoria> &claves, const QHash<ClaveConvocatoria, int> &ultimoId)
{
    std::sort(claves.begin(), claves.end(), [&](const ClaveConvocatoria &a, const ClaveConvocatoria &b) {
        if (a.first != b.first)
            return a.first > b.first;
        const int mesA = DatosConvocatoria::mesDe(a.second), mesB = DatosConvocatoria::mesDe(b.second);
        if (mesA != mesB)
            return mesA > mesB;
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
    return crearEsquema() && actualizarEsquema();
}

bool BaseDatos::ejecutar(const QString &sql, const QVariantList &valores, int *id)
{
    QSqlQuery q(m_db);
    if (!ejecutarEn(q, sql, valores))
        return false;
    if (id)
        *id = q.lastInsertId().toInt();
    return true;
}

QSqlQuery BaseDatos::consultar(const QString &sql, const QVariantList &valores)
{
    QSqlQuery q(m_db);
    ejecutarEn(q, sql, valores);
    return q;
}

bool BaseDatos::ejecutarEn(QSqlQuery &q, const QString &sql, const QVariantList &valores)
{
    bool ok;
    if (valores.isEmpty()) {
        ok = q.exec(sql);
    } else {
        q.prepare(sql);
        for (const QVariant &v : valores)
            q.addBindValue(v);
        ok = q.exec();
    }
    if (!ok)
        m_error = q.lastError().text();
    return ok;
}

bool BaseDatos::crearEsquema()
{
    return ejecutar(QStringLiteral(
               "CREATE TABLE IF NOT EXISTS temas ("
               " id INTEGER PRIMARY KEY AUTOINCREMENT,"
               " numero INTEGER NOT NULL UNIQUE,"
               " titulo TEXT NOT NULL,"
               " autor TEXT NOT NULL DEFAULT '',"
               " formato_referencia TEXT NOT NULL DEFAULT '')"))
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
               " verificada INTEGER NOT NULL DEFAULT 0,"
               " UNIQUE (tema_id, numero, variante))"))
        && ejecutar(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_preguntas_tema ON preguntas(tema_id, numero)"))
        // Las personas que verifican las preguntas y quién ha verificado cada una.
        && ejecutar(QStringLiteral("CREATE TABLE IF NOT EXISTS verificadores ("
                                   " id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                   " nombre TEXT NOT NULL UNIQUE)"))
        && ejecutar(QStringLiteral(
               "CREATE TABLE IF NOT EXISTS verificaciones ("
               " pregunta_id INTEGER NOT NULL REFERENCES preguntas(id) ON DELETE CASCADE,"
               " verificador_id INTEGER NOT NULL REFERENCES verificadores(id) ON DELETE CASCADE,"
               " PRIMARY KEY (pregunta_id, verificador_id))"))
        // Ajustes que van con el banco (asignatura, repartos, json_enlazado...).
        && ejecutar(QStringLiteral("CREATE TABLE IF NOT EXISTS ajustes (clave TEXT PRIMARY KEY, valor TEXT NOT NULL)"))
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
               " duracion INTEGER NOT NULL,"   // ya no se usa (la duración es del diálogo de práctica); se guarda 0
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

bool BaseDatos::actualizarEsquema()
{
    int version = 0;
    bool tieneFormato = false;
    bool tieneVerificada = false;
    {   // Las consultas se cierran antes de la transacción.
        QSqlQuery q = consultar(QStringLiteral("PRAGMA user_version"));
        if (q.next())
            version = q.value(0).toInt();
        q = consultar(QStringLiteral("PRAGMA table_info(temas)"));
        while (q.next())
            tieneFormato = tieneFormato || q.value(1).toString() == QLatin1String("formato_referencia");
        q = consultar(QStringLiteral("PRAGMA table_info(preguntas)"));
        while (q.next())
            tieneVerificada = tieneVerificada || q.value(1).toString() == QLatin1String("verificada");
    }
    if (version >= kVersionEsquema)
        return true;
    Transaccion transaccion(m_db);
    if (!tieneFormato && !ejecutar(QStringLiteral("ALTER TABLE temas ADD COLUMN formato_referencia TEXT NOT NULL DEFAULT ''")))
        return false;
    if (!tieneVerificada && !ejecutar(QStringLiteral("ALTER TABLE preguntas ADD COLUMN verificada INTEGER NOT NULL DEFAULT 0")))
        return false;
    // Lo que el programa daba por hecho cuando solo era para Fundamentos de la informática: el nombre, los repartos
    // de sus exámenes (la primera parte, la segunda y las dos juntas) y la forma de la referencia según el autor de
    // cada tema (ver Estilo::formatosReferencia()). Un banco sin temas aún no es de ninguna asignatura.
    const QVector<Tema> temasAntes = temas();
    if (version < 1 && !temasAntes.isEmpty()) {
        const QMap<int, int> primera{{1, 3}, {2, 2}, {3, 3}, {4, 2}}, segunda{{5, 2}, {6, 3}, {7, 2}, {8, 3}};
        QMap<int, int> todo = primera;
        todo.insert(segunda);
        if (!guardarAsignatura(QStringLiteral("Fundamentos de la informática"))
            || !guardarRepartos({{QStringLiteral("T1–T4"), primera}, {QStringLiteral("T5–T8"), segunda}, {QStringLiteral("T1–T8"), todo}}))
            return false;
        const QSet<int> vease{1, 3, 6, 8}, pag{2, 4, 5, 7};
        for (const Tema &t : temasAntes) {
            const QString formato = vease.contains(t.numero) ? QStringLiteral("vease")
                                    : pag.contains(t.numero) ? QStringLiteral("pag")
                                                             : QString();
            if (!formato.isEmpty() && !ejecutar(QStringLiteral("UPDATE temas SET formato_referencia = ? WHERE id = ?"), {formato, t.id}))
                return false;
        }
    }
    if (!ejecutar(QStringLiteral("PRAGMA user_version = %1").arg(kVersionEsquema)))
        return false;
    if (!transaccion.confirmar()) {
        m_error = m_db.lastError().text();
        return false;
    }
    return true;
}

QVector<Tema> BaseDatos::temas()
{
    QVector<Tema> res;
    QSqlQuery q = consultar(QStringLiteral("SELECT id, numero, titulo, autor, formato_referencia FROM temas ORDER BY numero"));
    while (q.next()) {
        Tema t;
        t.id = q.value(0).toInt();
        t.numero = q.value(1).toInt();
        t.titulo = q.value(2).toString();
        t.autor = q.value(3).toString();
        t.formatoReferencia = q.value(4).toString();
        res.append(t);
    }
    return res;
}

bool BaseDatos::guardarTema(Tema &tema)
{
    const QVariantList valores{tema.numero, texto(tema.titulo), texto(tema.autor), texto(tema.formatoReferencia)};
    if (tema.id != 0)
        return ejecutar(QStringLiteral("UPDATE temas SET numero = ?, titulo = ?, autor = ?, formato_referencia = ? WHERE id = ?"),
                        valores + QVariantList{tema.id});
    return ejecutar(QStringLiteral("INSERT INTO temas (numero, titulo, autor, formato_referencia) VALUES (?, ?, ?, ?)"), valores, &tema.id);
}

bool BaseDatos::guardarTemas(QVector<Tema> &nuevos, const QVector<int> &borrados)
{
    QHash<int, int> numeroAntes;   // id del tema -> número que tenía
    for (const Tema &t : temas())
        numeroAntes.insert(t.id, t.numero);
    Transaccion transaccion(m_db);
    for (int id : borrados)
        if (!borrarTema(id))
            return false;
    // Se renumeran en dos pasos para no chocar con la restricción UNIQUE si se intercambian números.
    for (const Tema &t : std::as_const(nuevos)) {
        if (t.id) {
            Tema provisional = t;
            provisional.numero = -t.numero;
            if (!guardarTema(provisional))
                return false;
        }
    }
    QVector<Tema> guardados = nuevos;
    QHash<int, int> cambio;   // número antes -> número después
    for (Tema &t : guardados) {
        if (t.id && numeroAntes.contains(t.id) && numeroAntes.value(t.id) != t.numero)
            cambio.insert(numeroAntes.value(t.id), t.numero);
        if (!guardarTema(t))
            return false;
    }

    if (!cambio.isEmpty()) {
        auto renumerar = [&cambio](const QString &codigo) {
            static const QRegularExpression re(QStringLiteral("^T(\\d+)-"), QRegularExpression::CaseInsensitiveOption);
            const QRegularExpressionMatch m = re.match(codigo.trimmed());
            if (!m.hasMatch() || !cambio.contains(m.captured(1).toInt()))
                return codigo;
            return QStringLiteral("T%1-").arg(cambio.value(m.captured(1).toInt())) + codigo.trimmed().mid(m.capturedEnd());
        };
        QVector<QPair<int, QString>> enlaces;
        QSqlQuery q = consultar(QStringLiteral("SELECT id, pseudo_con FROM preguntas WHERE pseudo_con <> ''"));
        while (q.next())
            if (renumerar(q.value(1).toString()) != q.value(1).toString())
                enlaces.append({q.value(0).toInt(), renumerar(q.value(1).toString())});
        for (const auto &e : enlaces)
            if (!ejecutar(QStringLiteral("UPDATE preguntas SET pseudo_con = ? WHERE id = ?"), {e.second, e.first}))
                return false;
        struct Registrada
        {
            int examen, posicion;
            QString codigo;
        };
        QVector<Registrada> registradas;
        q = consultar(QStringLiteral("SELECT examen_id, posicion, codigo FROM preguntas_registradas"));
        while (q.next())
            if (renumerar(q.value(2).toString()) != q.value(2).toString())
                registradas.append({q.value(0).toInt(), q.value(1).toInt(), renumerar(q.value(2).toString())});
        for (const Registrada &r : registradas)
            if (!ejecutar(QStringLiteral("UPDATE preguntas_registradas SET codigo = ? WHERE examen_id = ? AND posicion = ?"),
                          {r.codigo, r.examen, r.posicion}))
                return false;
    }
    // Los repartos van por número de tema: se renumeran igual, sin los temas borrados (y sin los que se queden vacíos).
    if (!cambio.isEmpty() || !borrados.isEmpty()) {
        QHash<int, int> numeroNuevo;   // número antes -> número después, de los temas que siguen
        for (const Tema &t : std::as_const(guardados))
            if (numeroAntes.contains(t.id))
                numeroNuevo.insert(numeroAntes.value(t.id), t.numero);
        auto renumerar = [&numeroNuevo](const QMap<int, int> &reparto) {
            QMap<int, int> res;
            for (auto it = reparto.cbegin(); it != reparto.cend(); ++it)
                if (numeroNuevo.contains(it.key()))
                    res.insert(numeroNuevo.value(it.key()), it.value());
            return res;
        };
        QVector<Reparto> renumerados;
        for (Reparto r : repartos()) {
            r.preguntas = renumerar(r.preguntas);
            if (!r.preguntas.isEmpty())
                renumerados << r;
        }
        if (!guardarRepartos(renumerados) || !guardarUltimoReparto(renumerar(ultimoReparto())))
            return false;
    }
    if (!transaccion.confirmar()) {
        m_error = m_db.lastError().text();
        return false;
    }
    nuevos = guardados;
    return true;
}

bool BaseDatos::borrarTema(int id)
{
    return ejecutar(QStringLiteral("DELETE FROM temas WHERE id = ?"), {id});
}

int BaseDatos::contarPreguntas(int temaId)
{
    QSqlQuery q = consultar(QStringLiteral("SELECT COUNT(*) FROM preguntas WHERE tema_id = ?"), {temaId});
    return q.next() ? q.value(0).toInt() : 0;
}

QVector<Pregunta> BaseDatos::preguntas()
{
    QVector<Pregunta> res;
    QSqlQuery q = consultar(QStringLiteral(
        "SELECT p.id, p.tema_id, t.numero, p.numero, p.variante, p.old, p.estado, p.revisar,"
        " p.enunciado, p.opcion_a, p.opcion_b, p.opcion_c, p.opcion_d, p.correcta,"
        " p.referencia, p.explicacion, p.notas, p.pseudo_con, p.imagen, p.verificada"
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
        p.verificada = q.value(19).toBool();
        res.append(p);
    }
    // Quiénes ha verificado cada una, por orden alfabético.
    QHash<int, QStringList> verificadoresDe;
    q = consultar(QStringLiteral("SELECT c.pregunta_id, v.nombre FROM verificaciones c JOIN verificadores v ON v.id = c.verificador_id"
                                 " ORDER BY v.nombre COLLATE NOCASE"));
    while (q.next())
        verificadoresDe[q.value(0).toInt()] << q.value(1).toString();
    for (Pregunta &p : res)
        p.verificadores = verificadoresDe.value(p.id);
    return res;
}

bool BaseDatos::guardarPregunta(Pregunta &p)
{
    QVariantList valores{p.temaId, p.numero, texto(p.variante), p.old ? 1 : 0, texto(p.estado), p.revisar ? 1 : 0, texto(p.enunciado)};
    for (int i = 0; i < 4; ++i)
        valores << texto(p.opciones.value(i));
    valores << QString(Pregunta::letraMayuscula(p.correcta)) << texto(p.referencia) << texto(p.explicacion) << texto(p.notas)
            << texto(p.pseudoCon) << (p.imagen.isEmpty() ? QVariant(QMetaType(QMetaType::QByteArray)) : QVariant(p.imagen))
            << (p.verificada ? 1 : 0);
    const bool ok = p.id == 0
                        ? ejecutar(QStringLiteral("INSERT INTO preguntas (tema_id, numero, variante, old, estado, revisar, enunciado,"
                                                  " opcion_a, opcion_b, opcion_c, opcion_d, correcta, referencia, explicacion, notas,"
                                                  " pseudo_con, imagen, verificada) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"),
                                   valores, &p.id)
                        : ejecutar(QStringLiteral("UPDATE preguntas SET tema_id=?, numero=?, variante=?, old=?, estado=?, revisar=?,"
                                                  " enunciado=?, opcion_a=?, opcion_b=?, opcion_c=?, opcion_d=?, correcta=?,"
                                                  " referencia=?, explicacion=?, notas=?, pseudo_con=?, imagen=?, verificada=?"
                                                  " WHERE id=?"),
                                   valores << p.id);
    if (!ok && m_error.contains(QLatin1String("UNIQUE")))
        m_error = tr("Ya existe una pregunta %1 en ese tema.").arg(p.codigo());
    if (!p.verificada)
        p.verificadores.clear();
    return ok && ponerVerificadores(p.id, p.verificadores);
}

// ---------------------------------------------------------------------------------------------
// Verificación

int BaseDatos::idVerificador(const QString &nombre)
{
    {
        QSqlQuery q = consultar(QStringLiteral("SELECT id FROM verificadores WHERE nombre = ?"), {nombre});
        if (q.next())
            return q.value(0).toInt();
    }
    int id = 0;
    return ejecutar(QStringLiteral("INSERT INTO verificadores (nombre) VALUES (?)"), {nombre}, &id) ? id : 0;
}

bool BaseDatos::ponerVerificadores(int preguntaId, const QStringList &nombres)
{
    if (!ejecutar(QStringLiteral("DELETE FROM verificaciones WHERE pregunta_id = ?"), {preguntaId}))
        return false;
    for (const QString &nombre : nombres) {
        if (texto(nombre).trimmed().isEmpty())
            continue;
        const int id = idVerificador(texto(nombre).trimmed());
        if (id == 0 || !ejecutar(QStringLiteral("INSERT OR IGNORE INTO verificaciones (pregunta_id, verificador_id) VALUES (?, ?)"),
                                 {preguntaId, id}))
            return false;
    }
    return true;
}

QStringList BaseDatos::verificadores()
{
    QStringList res;
    QSqlQuery q = consultar(QStringLiteral("SELECT nombre FROM verificadores ORDER BY nombre COLLATE NOCASE"));
    while (q.next())
        res << q.value(0).toString();
    return res;
}

bool BaseDatos::anadirVerificador(const QString &nombre)
{
    const QString n = nombre.trimmed();
    if (n.isEmpty()) {
        m_error = tr("Falta el nombre.");
        return false;
    }
    if (verificadores().contains(n, Qt::CaseInsensitive)) {
        m_error = tr("Ya hay alguien que se llama «%1».").arg(n);
        return false;
    }
    return idVerificador(n) != 0;
}

bool BaseDatos::cambiarNombreVerificador(const QString &antes, const QString &despues)
{
    const QString n = despues.trimmed();
    if (n.isEmpty()) {
        m_error = tr("Falta el nombre.");
        return false;
    }
    for (const QString &otro : verificadores()) {
        if (otro != antes && otro.compare(n, Qt::CaseInsensitive) == 0) {
            m_error = tr("Ya hay alguien que se llama «%1».").arg(n);
            return false;
        }
    }
    QStringList ultimos = ultimosVerificadores();
    if (ultimos.contains(antes)) {
        ultimos.replace(ultimos.indexOf(antes), n);
        if (!guardarUltimosVerificadores(ultimos))
            return false;
    }
    return ejecutar(QStringLiteral("UPDATE verificadores SET nombre = ? WHERE nombre = ?"), {n, antes});
}

bool BaseDatos::borrarVerificador(const QString &nombre)
{
    QStringList ultimos = ultimosVerificadores();
    if (ultimos.removeAll(nombre) && !guardarUltimosVerificadores(ultimos))
        return false;
    return ejecutar(QStringLiteral("DELETE FROM verificadores WHERE nombre = ?"), {nombre});
}

int BaseDatos::preguntasVerificadasPor(const QString &nombre)
{
    QSqlQuery q = consultar(QStringLiteral("SELECT COUNT(*) FROM verificaciones c JOIN verificadores v ON v.id = c.verificador_id"
                                           " WHERE v.nombre = ?"),
                            {nombre});
    return q.next() ? q.value(0).toInt() : 0;
}

bool BaseDatos::guardarVerificacion(int preguntaId, bool verificada, const QStringList &verificadores)
{
    Pregunta p;
    p.id = preguntaId;
    p.verificada = verificada;
    p.verificadores = verificadores;
    return guardarVerificaciones({p});
}

bool BaseDatos::guardarVerificaciones(const QVector<Pregunta> &preguntas)
{
    Transaccion transaccion(m_db);
    for (const Pregunta &p : preguntas)
        if (!ejecutar(QStringLiteral("UPDATE preguntas SET verificada = ? WHERE id = ?"), {p.verificada ? 1 : 0, p.id})
            || !ponerVerificadores(p.id, p.verificada ? p.verificadores : QStringList()))
            return false;
    if (!transaccion.confirmar()) {
        m_error = m_db.lastError().text();
        return false;
    }
    return true;
}

QStringList BaseDatos::ultimosVerificadores()
{
    QStringList res;
    const QStringList existentes = verificadores();
    for (const QJsonValue &v : QJsonDocument::fromJson(ajuste(QStringLiteral("ultimos_verificadores")).toUtf8()).array())
        if (existentes.contains(v.toString()))
            res << v.toString();
    return res;
}

bool BaseDatos::guardarUltimosVerificadores(const QStringList &nombres)
{
    return guardarAjuste(QStringLiteral("ultimos_verificadores"),
                         QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(nombres)).toJson(QJsonDocument::Compact)));
}

bool BaseDatos::borrarPregunta(int id)
{
    return ejecutar(QStringLiteral("DELETE FROM preguntas WHERE id = ?"), {id});
}

int BaseDatos::idTema(const Tema &t, bool actualizar)
{
    QSqlQuery q = consultar(QStringLiteral("SELECT id FROM temas WHERE numero = ?"), {t.numero});
    if (q.next()) {
        const int id = q.value(0).toInt();
        if (actualizar
            && !ejecutar(QStringLiteral("UPDATE temas SET titulo = ?, autor = ?, formato_referencia = ? WHERE id = ?"),
                         {texto(t.titulo), texto(t.autor), texto(t.formatoReferencia), id}))
            return 0;
        return id;
    }
    Tema nuevo = t;
    nuevo.id = 0;
    return guardarTema(nuevo) ? nuevo.id : 0;
}

QString BaseDatos::copiaSeguridad()
{
    const QFileInfo info(m_ruta);
    const QDir carpeta(info.absolutePath() + QStringLiteral("/copias"));
    if (!carpeta.mkpath(QStringLiteral("."))) {
        m_error = tr("No se puede crear la carpeta %1").arg(carpeta.absolutePath());
        return {};
    }
    const QString base = info.completeBaseName() + QLatin1Char('_')
                         + QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HHmmss"));
    const QString extension = info.suffix().isEmpty() ? QStringLiteral("db") : info.suffix();
    QString destino = carpeta.filePath(QStringLiteral("%1.%2").arg(base, extension));
    for (int i = 2; QFileInfo::exists(destino); ++i)
        destino = carpeta.filePath(QStringLiteral("%1_%2.%3").arg(base).arg(i).arg(extension));
    // VACUUM INTO copia la base de datos entera y coherente aunque esté abierta (SQLite ≥ 3.27). No admite
    // transacción abierta ni que el destino exista.
    if (!ejecutar(QStringLiteral("VACUUM INTO ?"), {destino}))
        return {};
    return destino;
}

int BaseDatos::importarJson(const QString &ruta, bool reemplazar, QString *copia)
{
    if (copia)
        copia->clear();
    ContenidoJson contenido;
    if (!leerJson(ruta, &contenido, &m_error))
        return -1;
    // Los temas del JSON que no dicen su título, su autor o la forma de su referencia se quedan con lo que tenía el
    // tema con su número (así un JSON con solo las preguntas nuevas de un tema no le borra nada).
    QHash<int, Tema> antes;
    for (const Tema &t : temas())
        antes.insert(t.numero, t);
    for (Tema &t : contenido.temas) {
        if (contenido.temasSinFormato.contains(t.numero))
            t.formatoReferencia = antes.value(t.numero).formatoReferencia;
        if (contenido.temasSinTitulo.contains(t.numero))
            t.titulo = antes.value(t.numero).titulo;
        if (contenido.temasSinAutor.contains(t.numero))
            t.autor = antes.value(t.numero).autor;
    }

    // La verificación de las que no la dicen: la que tenían en el banco, si no ha cambiado lo que se ve de ellas.
    QHash<QString, Pregunta> enElBanco;   // código (T1-7a) -> pregunta
    for (const Pregunta &p : preguntas())
        enElBanco.insert(p.codigoCompleto(), p);
    for (Pregunta &p : contenido.preguntas) {
        if (!contenido.sinVerificacion.contains(p.codigoCompleto()))
            continue;
        const auto it = enElBanco.constFind(p.codigoCompleto());
        const bool mantener = it != enElBanco.constEnd() && it->mismoContenido(p);
        p.verificada = mantener && it->verificada;
        p.verificadores = mantener ? it->verificadores : QStringList();
    }

    // Antes de borrar el banco, copia de seguridad (si hay algo que borrar: las preguntas van con sus temas). Si
    // la copia falla, no se toca nada.
    QString rutaCopia;
    if (reemplazar) {
        bool hayTemas = true;
        {   // La consulta se cierra antes de la copia: VACUUM no admite sentencias a medias.
            QSqlQuery q = consultar(QStringLiteral("SELECT COUNT(*) FROM temas"));
            hayTemas = !(q.next() && q.value(0).toInt() == 0);
        }
        if (hayTemas) {
            rutaCopia = copiaSeguridad();
            if (rutaCopia.isEmpty()) {
                m_error = tr("No se ha podido guardar la copia de seguridad del banco, así que no se ha borrado nada:\n%1").arg(m_error);
                return -1;
            }
        }
    }
    // Si la importación falla no cambia nada, y la copia sobra.
    const auto fallo = [&rutaCopia]() {
        if (!rutaCopia.isEmpty())
            QFile::remove(rutaCopia);
        return -1;
    };

    Transaccion transaccion(m_db);
    if (reemplazar && !(ejecutar(QStringLiteral("DELETE FROM preguntas")) && ejecutar(QStringLiteral("DELETE FROM temas"))))
        return fallo();

    if (asignatura().isEmpty() && !contenido.asignatura.isEmpty() && !guardarAsignatura(contenido.asignatura))
        return fallo();
    // El idioma, como la asignatura: solo si el banco aún no lo dice.
    if (ajuste(QStringLiteral("idioma")).isEmpty() && !contenido.idioma.isEmpty() && !guardarIdioma(contenido.idioma))
        return fallo();
    // Los verificadores del JSON se añaden a los del banco (los de las preguntas, al guardarlas).
    for (const QString &nombre : std::as_const(contenido.verificadores))
        if (idVerificador(nombre) == 0)
            return fallo();
    QHash<int, int> idDelTema;   // número de tema -> id
    for (const Tema &t : std::as_const(contenido.temas)) {
        const int temaId = idTema(t, true);
        if (temaId == 0)
            return fallo();
        idDelTema.insert(t.numero, temaId);
    }
    int n = 0;
    for (Pregunta p : std::as_const(contenido.preguntas)) {
        p.temaId = idDelTema.value(p.temaNumero);
        // Si ya existe (mismo tema, número y variante) se actualiza.
        QSqlQuery q = consultar(QStringLiteral("SELECT id FROM preguntas WHERE tema_id = ? AND numero = ? AND variante = ?"),
                                {p.temaId, p.numero, p.variante});
        if (q.next())
            p.id = q.value(0).toInt();
        if (!guardarPregunta(p)) {
            m_error = tr("Tema %1, pregunta %2: %3").arg(p.temaNumero).arg(p.codigo(), m_error);
            return fallo();
        }
        ++n;
    }
    if (!transaccion.confirmar()) {
        m_error = m_db.lastError().text();
        return fallo();
    }
    if (copia)
        *copia = rutaCopia;
    return n;
}

QString BaseDatos::ajuste(const QString &clave, const QString &porDefecto)
{
    QSqlQuery q = consultar(QStringLiteral("SELECT valor FROM ajustes WHERE clave = ?"), {clave});
    return q.next() ? q.value(0).toString() : porDefecto;
}

bool BaseDatos::guardarAjuste(const QString &clave, const QString &valor)
{
    return ejecutar(QStringLiteral("INSERT OR REPLACE INTO ajustes (clave, valor) VALUES (?, ?)"), {clave, valor});
}

QString BaseDatos::asignatura()
{
    return ajuste(QStringLiteral("asignatura"));
}

bool BaseDatos::guardarAsignatura(const QString &nombre)
{
    return guardarAjuste(QStringLiteral("asignatura"), nombre.trimmed());
}

QString BaseDatos::idioma()
{
    return ajuste(QStringLiteral("idioma"), QStringLiteral("es"));
}

bool BaseDatos::guardarIdioma(const QString &codigo)
{
    return guardarAjuste(QStringLiteral("idioma"), codigo);
}

QVector<Reparto> BaseDatos::repartos()
{
    // [{"nombre": "T1–T4", "preguntas": {"1": 3, "2": 2, "3": 3, "4": 2}}, …]
    QVector<Reparto> res;
    for (const QJsonValue &v : QJsonDocument::fromJson(ajuste(QStringLiteral("repartos")).toUtf8()).array()) {
        const QJsonObject o = v.toObject();
        res.append({o.value(QStringLiteral("nombre")).toString(), repartoDeJson(o.value(QStringLiteral("preguntas")).toObject())});
    }
    return res;
}

bool BaseDatos::guardarRepartos(const QVector<Reparto> &repartos)
{
    QJsonArray a;
    for (const Reparto &r : repartos)
        a.append(QJsonObject{{QStringLiteral("nombre"), r.nombre}, {QStringLiteral("preguntas"), repartoAJson(r.preguntas)}});
    return guardarAjuste(QStringLiteral("repartos"), QString::fromUtf8(QJsonDocument(a).toJson(QJsonDocument::Compact)));
}

QMap<int, int> BaseDatos::ultimoReparto()
{
    return repartoDeJson(QJsonDocument::fromJson(ajuste(QStringLiteral("reparto")).toUtf8()).object());
}

bool BaseDatos::guardarUltimoReparto(const QMap<int, int> &reparto)
{
    return guardarAjuste(QStringLiteral("reparto"), QString::fromUtf8(QJsonDocument(repartoAJson(reparto)).toJson(QJsonDocument::Compact)));
}

QVariantMap BaseDatos::opciones()
{
    // {"examen/numRecientes": 3, "examenes/nombres": ["Nacional Original", …], …}
    return QJsonDocument::fromJson(ajuste(QStringLiteral("opciones")).toUtf8()).object().toVariantMap();
}

bool BaseDatos::guardarOpciones(const QVariantMap &opciones)
{
    return guardarAjuste(QStringLiteral("opciones"),
                         QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(opciones)).toJson(QJsonDocument::Compact)));
}

QString BaseDatos::jsonEnlazado()
{
    const QString relativa = ajuste(QStringLiteral("json_enlazado"));
    if (relativa.isEmpty())
        return {};
    return QDir::cleanPath(QDir(QFileInfo(m_ruta).absolutePath()).absoluteFilePath(relativa));
}

bool BaseDatos::enlazarJson(const QString &ruta)
{
    const QDir carpeta(QFileInfo(m_ruta).absolutePath());
    return guardarAjuste(QStringLiteral("json_enlazado"), carpeta.relativeFilePath(QFileInfo(ruta).absoluteFilePath()));
}

bool BaseDatos::jsonAutomatico()
{
    return ajuste(QStringLiteral("json_automatico")) == QLatin1String("1");
}

bool BaseDatos::fijarJsonAutomatico(bool automatico)
{
    return guardarAjuste(QStringLiteral("json_automatico"), automatico ? QStringLiteral("1") : QStringLiteral("0"));
}

QStringList BaseDatos::diferenciasConJson(const QString &ruta)
{
    ContenidoJson json;
    QString error;
    if (!leerJson(ruta, &json, &error))
        return {error};

    // «3 preguntas distintas: T1-7, T2-3a, T4-1» (como mucho ocho códigos; luego, puntos suspensivos).
    auto linea = [](const QStringList &codigos, const QString &singular, const QString &plural) {
        if (codigos.isEmpty())
            return QString();
        QStringList muestra = codigos.mid(0, 8);
        if (codigos.size() > 8)
            muestra << QStringLiteral("…");
        return QStringLiteral("%1 %2: %3").arg(codigos.size()).arg(codigos.size() == 1 ? singular : plural, muestra.join(QStringLiteral(", ")));
    };
    QStringList diferencias;

    // Las preguntas del JSON que no dicen si están verificadas cuentan con la verificación del banco: no es una
    // diferencia (como al importarlas, que la conservan).
    QHash<QString, Pregunta> delJson;   // código completo -> pregunta
    for (const Pregunta &p : std::as_const(json.preguntas))
        delJson.insert(p.codigoCompleto(), p);
    QStringList distintas, soloBd, soloJson;
    for (const Pregunta &p : preguntas()) {
        const QString codigo = p.codigoCompleto();
        if (!delJson.contains(codigo)) {
            soloBd << codigo;
            continue;
        }
        Pregunta enJson = delJson.take(codigo);
        if (json.sinVerificacion.contains(codigo)) {
            enJson.verificada = p.verificada;
            enJson.verificadores = p.verificadores;
        }
        if (huella(enJson) != huella(p))
            distintas << codigo;
    }
    QHash<QString, QString> huellasJson;   // las que quedan, solo en el JSON
    for (auto it = delJson.cbegin(); it != delJson.cend(); ++it)
        huellasJson.insert(it.key(), QString());
    soloJson = huellasJson.keys();
    std::sort(soloJson.begin(), soloJson.end());
    diferencias << linea(distintas, tr("pregunta distinta"), tr("preguntas distintas"))
                << linea(soloBd, tr("pregunta solo en la base de datos"), tr("preguntas solo en la base de datos"))
                << linea(soloJson, tr("pregunta solo en el JSON"), tr("preguntas solo en el JSON"));

    QHash<int, Tema> temasDelJson;   // número -> tema
    for (const Tema &t : std::as_const(json.temas))
        temasDelJson.insert(t.numero, t);
    QStringList temasDistintos, temasSoloBd, temasSoloJson;
    for (const Tema &t : temas()) {
        const QString codigo = QStringLiteral("T%1").arg(t.numero);
        if (!temasDelJson.contains(t.numero)) {
            temasSoloBd << codigo;
            continue;
        }
        Tema temaJson = temasDelJson.take(t.numero);
        if (json.temasSinFormato.contains(t.numero))
            temaJson.formatoReferencia = t.formatoReferencia;
        if (huella(temaJson) != huella(t))
            temasDistintos << codigo;
    }
    QList<int> numerosSoloJson = temasDelJson.keys();
    std::sort(numerosSoloJson.begin(), numerosSoloJson.end());
    for (int numero : numerosSoloJson)
        temasSoloJson << QStringLiteral("T%1").arg(numero);
    diferencias << linea(temasDistintos, tr("tema con otro título, autor o forma de la referencia"),
                         tr("temas con otro título, autor o forma de la referencia"))
                << linea(temasSoloBd, tr("tema solo en la base de datos"), tr("temas solo en la base de datos"))
                << linea(temasSoloJson, tr("tema solo en el JSON"), tr("temas solo en el JSON"));
    if (json.diceAsignatura && json.asignatura != asignatura())
        diferencias << (json.asignatura.isEmpty() ? tr("En el JSON la asignatura no tiene nombre")
                                                  : tr("La asignatura tiene otro nombre en el JSON: «%1»").arg(json.asignatura));
    if (!json.idioma.isEmpty() && json.idioma != idioma())
        //: %1 es el código del idioma de los exámenes que dice el JSON («es», «en»…).
        diferencias << tr("Los exámenes están en otro idioma en el JSON: «%1»").arg(json.idioma);
    diferencias.removeAll(QString());
    return diferencias;
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
        jt[QStringLiteral("formato_referencia")] = t.formatoReferencia;
        jt[QStringLiteral("preguntas")] = jpreguntas;
        jtemas.append(jt);
    }
    QJsonObject raiz;
    raiz[QStringLiteral("$schema")] = QLatin1String(ComprobadorJson::kEsquema);
    raiz[QStringLiteral("asignatura")] = asignatura();
    raiz[QStringLiteral("idioma")] = idioma();
    if (!verificadores().isEmpty())
        raiz[QStringLiteral("verificadores")] = QJsonArray::fromStringList(verificadores());
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
    const QString ahora = QDateTime::currentDateTime().toString(Qt::ISODate);
    Transaccion transaccion(m_db);
    for (const Modelo &m : modelos) {
        int examenId = 0;
        if (!ejecutar(QStringLiteral("DELETE FROM examenes_registrados WHERE anio = ? AND convocatoria = ? AND nombre = ?"),
                      {datos.anio, texto(datos.convocatoria), texto(m.nombre)})
            || !ejecutar(QStringLiteral("INSERT INTO examenes_registrados (anio, convocatoria, nombre, registrado, semilla,"
                                        " asignatura, instrucciones, duracion, puntos_acierto, puntos_fallo)"
                                        " VALUES (?,?,?,?,?,?,?,?,?,?)"),
                         {datos.anio, texto(datos.convocatoria), texto(m.nombre), ahora, qint64(semilla), texto(datos.asignatura),
                          texto(datos.instrucciones), 0 /* duracion: columna antigua, ya sin uso */, datos.puntosAcierto, datos.puntosFallo},
                         &examenId))
            return false;
        for (int i = 0; i < m.preguntas.size(); ++i) {
            const PreguntaExamen &pe = m.preguntas.at(i);
            QJsonObject jp = preguntaAJson(pe.pregunta);
            jp[QStringLiteral("id")] = pe.pregunta.id;
            jp[QStringLiteral("tema")] = pe.pregunta.temaNumero;
            QString orden;
            for (int k : pe.orden)
                orden += QString::number(k);
            if (!ejecutar(QStringLiteral("INSERT INTO preguntas_registradas (examen_id, posicion, codigo, orden, pregunta) VALUES (?,?,?,?,?)"),
                          {examenId, i, pe.pregunta.codigoCompleto(), orden,
                           QString::fromUtf8(QJsonDocument(jp).toJson(QJsonDocument::Compact))}))
                return false;
        }
    }
    if (!transaccion.confirmar()) {
        m_error = m_db.lastError().text();
        return false;
    }
    return true;
}

QVector<ExamenRegistrado> BaseDatos::examenesRegistrados()
{
    QVector<ExamenRegistrado> res;
    QHash<ClaveConvocatoria, int> ultimoId;
    QSqlQuery q = consultar(QStringLiteral(
        "SELECT e.id, e.nombre, e.registrado, e.semilla, e.anio, e.convocatoria, e.asignatura, e.instrucciones,"
        " e.puntos_acierto, e.puntos_fallo,"
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
        e.datos.puntosAcierto = q.value(8).toDouble();
        e.datos.puntosFallo = q.value(9).toDouble();
        e.numPreguntas = q.value(10).toInt();
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
    QSqlQuery q = consultar(QStringLiteral("SELECT nombre FROM examenes_registrados WHERE id = ?"), {examenId});
    if (q.next())
        m.nombre = q.value(0).toString();
    q = consultar(QStringLiteral("SELECT orden, pregunta FROM preguntas_registradas WHERE examen_id = ? ORDER BY posicion"), {examenId});
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
    return ejecutar(QStringLiteral("DELETE FROM examenes_registrados WHERE id = ?"), {examenId});
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
    QSqlQuery q = consultar(QStringLiteral(
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
    QSqlQuery q = consultar(QStringLiteral("SELECT e.convocatoria, e.anio, e.nombre FROM preguntas_registradas p"
                                           " JOIN examenes_registrados e ON e.id = p.examen_id"
                                           " WHERE p.codigo = ? ORDER BY e.anio DESC, e.id DESC"),
                            {codigo});
    while (q.next())
        res << QStringLiteral("%1 %2 · %3").arg(q.value(0).toString()).arg(q.value(1).toInt()).arg(q.value(2).toString());
    return res;
}
