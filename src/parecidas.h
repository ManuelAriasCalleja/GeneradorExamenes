#pragma once

#include "pregunta.h"

#include <QHash>
#include <QString>
#include <QVector>

// Búsqueda de preguntas parecidas a una dada, para que una pregunta nueva que se parece mucho a otra se guarde
// como su variante (o se enlace con ella si es de otro tema) en lugar de con un número nuevo.
//
// El parecido es el coseno entre los vectores TF-IDF de los trozos de cuatro letras del enunciado (que cuenta
// doble) y de las opciones, sin tildes ni palabras vacías. Los trozos, mejor que las palabras enteras, hacen que
// se parezcan «PROM» y «EPROM» o «interrupción» e «interrupciones». Calibrado con las variantes y pseudorrepetidas
// de los dos bancos: la variante de una pregunta sale entre las tres más parecidas en ocho de cada diez casos.
namespace Parecidas {

// A partir de este parecido se muestra una pregunta como parecida, y a partir de este otro se pregunta, al guardar
// una pregunta nueva, si es una variante.
constexpr double kMinimoMostrar = 0.30;
constexpr double kMinimoPreguntar = 0.45;

struct Parecida
{
    Pregunta pregunta;
    double parecido;   // de 0 a 1
};

class Indice
{
public:
    explicit Indice(const QVector<Pregunta> &banco);

    // Las 'cuantas' preguntas más parecidas a 'p' con al menos 'minimo' de parecido, de más a menos parecida. No
    // cuentan las de su mismo grupo (mismo tema y número: sus variantes) ni las ya enlazadas como pseudorrepetidas.
    QVector<Parecida> masParecidas(const Pregunta &p, int cuantas, double minimo) const;

private:
    using Vector = QHash<QString, double>;
    Vector vector(const Pregunta &p) const;

    QVector<Pregunta> m_banco;
    QHash<QString, double> m_idf;
    QVector<Vector> m_vectores;
};

// Letra libre para una variante nueva del grupo (tema y número): la primera que no se usa. Una pregunta sin letra
// cuenta como la «a», para que la nueva no sea otra «a».
QString siguienteVariante(const QVector<Pregunta> &banco, int temaId, int numero);

} // namespace Parecidas
