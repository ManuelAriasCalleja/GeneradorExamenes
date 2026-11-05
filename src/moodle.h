#pragma once

#include "pregunta.h"

#include <QString>
#include <QVector>

// Exportación del banco a un cuestionario de Moodle (formato «Moodle XML», el que importa Banco de preguntas →
// Importar), para que los estudiantes practiquen en el curso virtual: preguntas de opción múltiple con una sola
// respuesta, en una categoría por tema dentro de la de la asignatura («$course$/top/<asignatura>/Tema 1: …»), con la
// referencia al libro y la explicación como realimentación general y las imágenes incrustadas. Las opciones se barajan
// en Moodle salvo en las preguntas con opciones del tipo «Todas las anteriores». Las anuladas y obsoletas no se exportan.
namespace Moodle {

struct Opciones
{
    QString asignatura;                 // categoría raíz, bajo $course$/top; sin ella, los temas van directamente allí
    QString idioma = QStringLiteral("es");   // de los textos (categorías de los temas, realimentación): el del banco
    bool incluirParaRevisar = true;     // también las marcadas para revisar
    bool penalizarFallos = true;        // las respuestas erróneas restan el 30 % de la pregunta, como en el examen
};

// Las preguntas que se exportan (las activas, con o sin las de revisar), en el orden del banco.
QVector<Pregunta> seleccion(const QVector<Pregunta> &preguntas, const Opciones &opciones);
// El XML del cuestionario con esas preguntas; 'temas' da el nombre de la categoría de cada una.
QString xml(const QVector<Tema> &temas, const QVector<Pregunta> &preguntas, const Opciones &opciones);
// Lo escribe en 'ruta' (en UTF-8); si no puede, devuelve false y deja el motivo en 'error'.
bool exportar(const QString &ruta, const QVector<Tema> &temas, const QVector<Pregunta> &preguntas, const Opciones &opciones,
              QString *error);

} // namespace Moodle
