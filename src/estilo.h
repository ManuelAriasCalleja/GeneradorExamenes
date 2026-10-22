#pragma once

#include "pregunta.h"

#include <QString>
#include <QVector>

// Comprobación automática de las pautas de redacción de las preguntas (las de la guía de estilo del banco, que
// está fuera del repositorio). Son avisos, no errores: puede haber excepciones justificadas.
namespace Estilo {

struct Aviso
{
    enum Parte { Enunciado, Opciones, Referencia };
    Parte parte;
    QString texto;   // una frase, sin citar el texto de la pregunta
};

QString nombreParte(Aviso::Parte parte);

QVector<Aviso> revisar(const Pregunta &p);

// Opción que remite a las demás («Todas las anteriores», «a y b»...): con ella no se pueden barajar las opciones.
bool opcionDependiente(const QString &opcion);
bool tieneOpcionesDependientes(const Pregunta &p);

} // namespace Estilo
