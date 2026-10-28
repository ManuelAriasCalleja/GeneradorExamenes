#pragma once

#include "maquetador.h"
#include "pregunta.h"

#include <QDateTime>
#include <QHash>
#include <QMap>
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
    DatosConvocatoria datos;         // convocatoria, año, asignatura, instrucciones y puntuación
    int numPreguntas = 0;
};

// Reparto de preguntas por tema guardado en el banco, con un nombre: es uno de los botones («T1–T4») que lo ponen.
struct Reparto
{
    QString nombre;
    QMap<int, int> preguntas;        // número de tema -> preguntas
};

// Acceso a la base de datos SQLite de temas y preguntas.
class BaseDatos
{
public:
    BaseDatos();
    ~BaseDatos();

    // Abre la base de datos (si no existe, la crea vacía). Los bancos de antes de que el banco llevase el nombre de
    // la asignatura (todos, de Fundamentos de la informática) se ponen al día: ver actualizarEsquema().
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
    // se borra. Las preguntas con el mismo tema, número y variante se actualizan. El nombre de la asignatura del
    // JSON solo se toma si el banco aún no tiene; la forma de la referencia de un tema, si el JSON no la dice (los
    // de importar_docx.py no la dicen), es la que ya tenía el tema con ese número.
    int importarJson(const QString &ruta, bool reemplazar, QString *copia = nullptr);
    bool exportarJson(const QString &ruta);

    // Guarda una copia de la base de datos en la carpeta «copias» junto a ella, con la fecha y la hora en el nombre
    // (copias/preguntas_2026-10-22_153045.db), y devuelve su ruta; vacía si falla (el motivo, en ultimoError()).
    QString copiaSeguridad();

    // Ajustes del banco, que van con él en la base de datos: el valor de una clave ('porDefecto' si no está).
    QString ajuste(const QString &clave, const QString &porDefecto = QString());
    bool guardarAjuste(const QString &clave, const QString &valor);

    // Nombre de la asignatura del banco: va en la cabecera de los exámenes. Vacío si aún no se ha puesto.
    QString asignatura();
    bool guardarAsignatura(const QString &nombre);
    // Repartos de preguntas por tema guardados en el banco, en el orden de sus botones.
    QVector<Reparto> repartos();
    bool guardarRepartos(const QVector<Reparto> &repartos);
    // El último reparto usado (número de tema -> preguntas); vacío si no se ha guardado ninguno.
    QMap<int, int> ultimoReparto();
    bool guardarUltimoReparto(const QMap<int, int> &reparto);

    // JSON enlazado con el banco: su copia en texto (preguntas_fi.json), que es el último JSON del que se importó
    // todo o el elegido en «Actualizar el JSON del banco». El programa avisa cuando no coinciden y, si se pide,
    // lo reescribe tras cada cambio. La ruta se guarda relativa a la carpeta de la base de datos, para que siga
    // valiendo si se copian juntos a otro ordenador.
    QString jsonEnlazado();   // ruta absoluta; vacía si no hay
    bool enlazarJson(const QString &ruta);
    bool jsonAutomatico();
    bool fijarJsonAutomatico(bool automatico);
    // En qué se diferencia el banco de un JSON, sin contar los id: «3 preguntas distintas: T1-7, T2-3a, T4-1»,
    // «1 pregunta solo en la base de datos: T5-2», «2 preguntas solo en el JSON: …», «1 tema con otro título, autor
    // o forma de la referencia: T3», «La asignatura tiene otro nombre en el JSON: «…»». Lo que el JSON no dice (la
    // asignatura o la forma de la referencia de un tema) no cuenta. Vacío si coinciden; si el fichero no se puede
    // leer, una sola línea que lo dice.
    QStringList diferenciasConJson(const QString &ruta);

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
    // Pone al día un banco de una versión anterior del programa (PRAGMA user_version). En la 1, el banco lleva el
    // nombre de la asignatura, sus repartos y la forma de la referencia de cada tema; a los bancos de antes, que
    // eran todos de Fundamentos de la informática (los que tienen temas), se les ponen los de esa asignatura.
    bool actualizarEsquema();
    // Sentencia con sus valores (los «?», en orden); si falla, deja el error en m_error. En 'id' deja el de la fila
    // insertada.
    bool ejecutar(const QString &sql, const QVariantList &valores = {}, int *id = nullptr);
    // Consulta con sus valores, para recorrerla con next(); si falla, no da filas y deja el error en m_error.
    QSqlQuery consultar(const QString &sql, const QVariantList &valores = {});
    bool ejecutarEn(QSqlQuery &q, const QString &sql, const QVariantList &valores);
    // Id del tema con el número de 't' (si no hay, lo crea); con 'actualizar', le pone el título, el autor y la forma
    // de la referencia de 't'.
    int idTema(const Tema &t, bool actualizar);

    QSqlDatabase m_db;
    QString m_ruta;
    QString m_error;
};
