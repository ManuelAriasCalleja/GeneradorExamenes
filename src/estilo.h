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

// Formas de la referencia al libro que se pueden pedir a las preguntas de un tema (Tema::formatoReferencia), con lo
// que se ve al elegirlas: la primera, vacía, es «cualquiera».
QVector<QPair<QString, QString>> formatosReferencia();

// 'temas', los del banco: dicen qué forma lleva la referencia de cada uno (si no está el de la pregunta, cualquiera).
// 'idioma', el de las preguntas (BaseDatos::idioma()): muchas pautas son del castellano (¿?, «NO», comillas, las
// formas «Véase la página…», enunciados que acaban en dos puntos y opciones en punto); en otro idioma solo se
// comprueban las demás (longitud, mayúsculas, opciones iguales o que remiten a las demás, la correcta más larga,
// que haya referencia, comillas del código y erratas).
QVector<Aviso> revisar(const Pregunta &p, const QVector<Tema> &temas = {}, const QString &idioma = QStringLiteral("es"));

// Opción que remite a las demás («Todas las anteriores», «a y b», «All of the above»...): con ella no se pueden
// barajar las opciones. Se reconocen en castellano y en inglés, sea cual sea el idioma del banco.
bool opcionDependiente(const QString &opcion);
bool tieneOpcionesDependientes(const Pregunta &p);

} // namespace Estilo
