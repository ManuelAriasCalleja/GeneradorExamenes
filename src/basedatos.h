#pragma once

#include "maquetador.h"
#include "pregunta.h"

#include <QDateTime>
#include <QHash>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

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
    int contarPreguntas(int temaId);

    QVector<Pregunta> preguntas();
    bool guardarPregunta(Pregunta &p);
    bool borrarPregunta(int id);

    // Importa un JSON con el formato de herramientas/importar_docx.py.
    // Si 'reemplazar' es true borra antes todas las preguntas y temas.
    // Las preguntas con el mismo tema, número y variante se actualizan.
    int importarJson(const QString &ruta, bool reemplazar);
    bool exportarJson(const QString &ruta);

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
    bool ejecutar(const QString &sql);
    int idTema(int numero, const QString &titulo, const QString &autor, bool actualizar);

    QSqlDatabase m_db;
    QString m_ruta;
    QString m_error;
};
