#pragma once

#include "maquetador.h"
#include "pregunta.h"

#include <QCoreApplication>
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
    QString nombre;                  // p. ej. "Nacional Original"; con versiones, «Nacional Original B»
    QString version;                 // la de un examen con versiones («B»); vacía si no
    QDateTime registrado;
    quint32 semilla = 0;
    DatosConvocatoria datos;         // convocatoria, año, asignatura, instrucciones y puntuación
    int numPreguntas = 0;

    // El nombre del examen de la lista: sin la versión, si tiene («Nacional Original»).
    QString base() const
    {
        return !version.isEmpty() && nombre.endsWith(QLatin1Char(' ') + version) ? nombre.chopped(version.size() + 1) : nombre;
    }
};

// Lo que ha pasado con la verificación de las preguntas al importar un JSON (ver BaseDatos::importarJson() e
// importarVerificaciones()). Importar nunca quita la verificación de una pregunta que no ha cambiado.
struct ResumenVerificacion
{
    int conservadas = 0;     // verificadas en el banco que el JSON da por no verificadas, sin cambios: lo siguen
    int anadidas = 0;        // preguntas con alguna verificación (o persona) del JSON que no tenían en el banco
    int quitadas = 0;        // verificadas en el banco que han cambiado y el JSON no da por verificadas
    int yaEstaban = 0;       // al importar solo las verificaciones: el banco ya las tenía todas
    QStringList distintas;   // … las que el JSON da por verificadas y en el banco tienen otro contenido (no se tocan)
    QStringList noEstan;     // … las que el JSON da por verificadas y no están en el banco
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
    Q_DECLARE_TR_FUNCTIONS(BaseDatos)

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
    // Guarda la pregunta con su verificación: si no está verificada, sin verificadores; los que no estén en la lista
    // de verificadores del banco se añaden.
    bool guardarPregunta(Pregunta &p);
    bool borrarPregunta(int id);

    // Verificación de las preguntas por personas (ver Pregunta::verificada). Los verificadores del banco, por orden
    // alfabético; cada pregunta dice cuáles la han verificado.
    QStringList verificadores();
    bool anadirVerificador(const QString &nombre);
    // Cambia el nombre en la lista y en las preguntas que había verificado.
    bool cambiarNombreVerificador(const QString &antes, const QString &despues);
    // Lo quita de la lista y de las preguntas que había verificado, que siguen verificadas.
    bool borrarVerificador(const QString &nombre);
    // Cuántas preguntas ha verificado.
    int preguntasVerificadasPor(const QString &nombre);
    // Pone solo la verificación de una pregunta (o de varias, de una vez), sin tocar lo demás.
    bool guardarVerificacion(int preguntaId, bool verificada, const QStringList &verificadores);
    bool guardarVerificaciones(const QVector<Pregunta> &preguntas);
    // Los que verificaron la última pregunta que se marcó como verificada: se proponen para la siguiente.
    QStringList ultimosVerificadores();
    bool guardarUltimosVerificadores(const QStringList &nombres);

    // Importa un JSON con el formato de herramientas/importar_docx.py.
    // Antes de importar guarda una copia de seguridad de la base de datos (ver copiaSeguridad()), si tiene algo; en
    // 'copia', si se da, deja su ruta, o vacía si no se ha hecho (banco sin temas). Si 'reemplazar' es true borra
    // antes todas las preguntas y temas. Si la importación falla, la base de datos no cambia y la copia se borra. Las
    // preguntas con el mismo tema, número y variante se actualizan. El nombre de la asignatura del JSON solo se toma
    // si el banco aún no tiene; la forma de la referencia de un tema, si el JSON no la dice (los de importar_docx.py
    // no la dicen), es la que ya tenía el tema con ese número. La verificación de una pregunta: si su contenido no ha
    // cambiado, la que tenía en el banco la del mismo código, con las personas que añada el JSON (importar nunca la
    // quita: el JSON puede ser de antes de verificarla); si ha cambiado, o es nueva, la del JSON, si la dice. Lo que
    // ha pasado con ella, en 'verificacion', si se da. También al reemplazarlo todo.
    int importarJson(const QString &ruta, bool reemplazar, QString *copia = nullptr,
                     ResumenVerificacion *verificacion = nullptr);
    // Solo las verificaciones de un JSON (exportado de otra copia del banco, en otro ordenador): las que da por
    // verificadas se añaden a las preguntas del banco con el mismo código y el mismo contenido, juntando las personas.
    // No cambia nada más ni quita ninguna verificación. Devuelve a cuántas preguntas se ha añadido algo (lo demás, en
    // 'resumen'), o -1 si no se puede leer el JSON o guardar (el motivo, en ultimoError()).
    int importarVerificaciones(const QString &ruta, ResumenVerificacion *resumen = nullptr);
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
    // Idioma de los textos de los exámenes y de Moodle («es», «en»: ver Idioma::examen()). Va con el banco, como la
    // asignatura; los bancos que no lo dicen están en castellano.
    QString idioma();
    bool guardarIdioma(const QString &codigo);
    // Repartos de preguntas por tema guardados en el banco, en el orden de sus botones.
    QVector<Reparto> repartos();
    bool guardarRepartos(const QVector<Reparto> &repartos);
    // El último reparto usado (número de tema -> preguntas); vacío si no se ha guardado ninguno.
    QMap<int, int> ultimoReparto();
    bool guardarUltimoReparto(const QMap<int, int> &reparto);
    // Opciones de la ventana que van con el banco, para que cada asignatura tenga las suyas: las de los exámenes
    // (instrucciones, opciones al elegir las preguntas, puntuación), la lista de exámenes de la convocatoria y las
    // carpetas de las exportaciones. Clave de la configuración («examen/numRecientes») -> valor; vacío si el banco
    // aún no tiene (los de las versiones anteriores).
    QVariantMap opciones();
    bool guardarOpciones(const QVariantMap &opciones);

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
    // Registra los exámenes; sustituye los que ya haya con el mismo año, convocatoria y nombre, y también los de esa
    // convocatoria que se llamen como alguno de 'sustituidos' (de los mismos exámenes de la lista, con o sin versiones).
    bool registrarExamenes(const QVector<Modelo> &modelos, const DatosConvocatoria &datos, quint32 semilla,
                           const QStringList &sustituidos = {});
    // Todos los exámenes registrados, de la convocatoria más reciente a la más antigua.
    QVector<ExamenRegistrado> examenesRegistrados();
    Modelo modeloRegistrado(int examenId);
    bool borrarExamenRegistrado(int examenId);
    // Para cada pregunta (por código) que haya salido en alguna de las 'cuantas' últimas convocatorias, cuál fue
    // la última: 1 = la más reciente. No cuentan los exámenes de 'anio' y 'convocatoria' que se llamen como
    // alguno de 'excluidos': son los que se están generando de nuevo y sustituirán a los registrados.
    QHash<QString, int> usoReciente(int cuantas, int anio, const QString &convocatoria, const QStringList &excluidos);
    // Cómo salió cada pregunta (por código) en todos los exámenes registrados, sean de la convocatoria que sean: en qué
    // letra estuvo la correcta cada vez (ver ConfigExamen::salidas). Como en usoReciente(), no cuentan los exámenes de
    // 'anio' y 'convocatoria' que se llamen como alguno de 'excluidos'.
    QHash<QString, SalidaAnterior> salidasAnteriores(int anio, const QString &convocatoria, const QStringList &excluidos);
    // Exámenes registrados en que ha salido una pregunta: «Junio 2026 · Nacional Original».
    QStringList usosDe(const QString &codigo);

private:
    bool crearEsquema();
    // Pone al día un banco de una versión anterior del programa (PRAGMA user_version). En la 1, el banco lleva el
    // nombre de la asignatura, sus repartos y la forma de la referencia de cada tema; a los bancos de antes, que
    // eran todos de Fundamentos de la informática (los que tienen temas), se les ponen los de esa asignatura. En la 2,
    // las preguntas dicen si están verificadas (ninguna lo está). En la 3, los exámenes registrados dicen su versión
    // (ninguno tiene), y en la 4, si se barajaron sus opciones (ninguno: lo que diga la ventana, como antes).
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
    // Id del verificador con ese nombre (si no está, lo añade); 0 si falla.
    int idVerificador(const QString &nombre);
    bool ponerVerificadores(int preguntaId, const QStringList &nombres);

    QSqlDatabase m_db;
    QString m_ruta;
    QString m_error;
};
