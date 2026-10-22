#pragma once

#include "pregunta.h"

#include <QSqlDatabase>
#include <QString>
#include <QVector>

// Acceso a la base de datos SQLite de temas y preguntas.
class BaseDatos
{
public:
    BaseDatos();
    ~BaseDatos();

    // Abre (o crea) la base de datos. Si está vacía la rellena con las preguntas incluidas en el programa.
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

    static QString rutaJsonIncluido() { return QStringLiteral(":/datos/preguntas_fi.json"); }

private:
    bool crearEsquema();
    bool ejecutar(const QString &sql);
    int idTema(int numero, const QString &titulo, const QString &autor, bool actualizar);

    QSqlDatabase m_db;
    QString m_ruta;
    QString m_error;
};
