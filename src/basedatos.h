#pragma once

#include "maquetador.h"
#include "pregunta.h"

#include <QDateTime>
#include <QHash>
#include <QSqlDatabase>
#include <QString>
#include <QVariant>
#include <QVector>

class QSqlQuery;

// Examen guardado en el historial de convocatorias.
struct ExamenRegistrado
{
    int id = 0;
    QString nombre;                  // p. ej. "Nacional Original"
    QDateTime registrado;
    quint32 semilla = 0;
    DatosConvocatoria datos;         // convocatoria, año, asignatura, instrucciones, duración y puntuación
    int numPreguntas = 0;
};

// Acceso a la base de datos SQLite de temas y preguntas.
class BaseDatos
{
public:
    BaseDatos();
    ~BaseDatos();

    // Abre la base de datos (si no existe, la crea vacía).
    bool abrir(const QString &ruta);
    void cerrar();
    bool abierta() const;
    QString ruta() const { return m_ruta; }
    QString ultimoError() const { return m_error; }

    QVector<Tema> temas();
    bool guardarTema(Tema &tema);
    bool borrarTema(int id);
    // Los cambios del diálogo de temas, de una vez (si algo falla, no cambia nada): borra los temas de 'borrados'
    // (con sus preguntas) y guarda los de 'nuevos', a los que no tenían id se lo pone. Si cambia el número de un tema,
    // actualiza los códigos que lo llevan escrito: los enlaces «pseudorrepetida con» (T3-5) y los del historial.
    bool guardarTemas(QVector<Tema> &nuevos, const QVector<int> &borrados);
    int contarPreguntas(int temaId);

    QVector<Pregunta> preguntas();
    bool guardarPregunta(Pregunta &p);
    bool borrarPregunta(int id);

    // Importa un JSON con el formato de herramientas/importar_docx.py.
    // Si 'reemplazar' es true borra antes todas las preguntas y temas, tras guardar una copia de seguridad de la
    // base de datos (ver copiaSeguridad()); en 'copia', si se da, deja la ruta de esa copia, o vacía si no se ha
    // hecho (banco sin temas, o añadir/actualizar). Si la importación falla, la base de datos no cambia y la copia
    // se borra. Las preguntas con el mismo tema, número y variante se actualizan.
    int importarJson(const QString &ruta, bool reemplazar, QString *copia = nullptr);
    bool exportarJson(const QString &ruta);

    // Guarda una copia de la base de datos en la carpeta «copias» junto a ella, con la fecha y la hora en el nombre
    // (copias/preguntas_2026-10-22_153045.db), y devuelve su ruta; vacía si falla (el motivo, en ultimoError()).
    QString copiaSeguridad();

    // Historial de convocatorias. Cada examen registrado guarda una copia de sus preguntas tal como salieron (con
    // el orden de las opciones), para poder volver a exportarlo aunque después se corrijan. Las preguntas se
    // enlazan con el banco por su código (T1-7a), que no cambia al reimportar el JSON.
    // Registra los exámenes; sustituye los que ya haya con el mismo año, convocatoria y nombre.
    bool registrarExamenes(const QVector<Modelo> &modelos, const DatosConvocatoria &datos, quint32 semilla);
    // Todos los exámenes registrados, de la convocatoria más reciente a la más antigua.
    QVector<ExamenRegistrado> examenesRegistrados();
    Modelo modeloRegistrado(int examenId);
    bool borrarExamenRegistrado(int examenId);
    // Para cada pregunta (por código) que haya salido en alguna de las 'cuantas' últimas convocatorias, cuál fue
    // la última: 1 = la más reciente. No cuentan los exámenes de 'anio' y 'convocatoria' que se llamen como
    // alguno de 'excluidos': son los que se están generando de nuevo y sustituirán a los registrados.
    QHash<QString, int> usoReciente(int cuantas, int anio, const QString &convocatoria, const QStringList &excluidos);
    // Exámenes registrados en que ha salido una pregunta: «Junio 2026 · Nacional Original».
    QStringList usosDe(const QString &codigo);

private:
    bool crearEsquema();
    // Sentencia con sus valores (los «?», en orden); si falla, deja el error en m_error. En 'id' deja el de la fila
    // insertada.
    bool ejecutar(const QString &sql, const QVariantList &valores = {}, int *id = nullptr);
    // Consulta con sus valores, para recorrerla con next(); si falla, no da filas y deja el error en m_error.
    QSqlQuery consultar(const QString &sql, const QVariantList &valores = {});
    bool ejecutarEn(QSqlQuery &q, const QString &sql, const QVariantList &valores);
    int idTema(int numero, const QString &titulo, const QString &autor, bool actualizar);

    QSqlDatabase m_db;
    QString m_ruta;
    QString m_error;
};
