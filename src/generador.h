#pragma once

#include "pregunta.h"

#include <QHash>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QVector>
#include <array>

struct ConfigExamen
{
    QMap<int, int> preguntasPorTema;    // número de tema -> número de preguntas
    QStringList nombresModelos;         // un modelo por nombre (p. ej. "Nacional Original"), en este orden
    bool noRepetirEntreModelos = true;  // dentro de una misma convocatoria
    bool incluirParaRevisar = true;     // preguntas marcadas MEJORAR/REFINAR...
    bool ordenAleatorio = false;        // si no, por tema y número (como en los exámenes oficiales)
    bool barajarOpciones = false;       // repartiendo la correcta por igual entre a), b), c) y d)
    // Preguntas ya usadas, que se dejan para el final (con sus variantes y pseudorrepetidas): primero las nunca
    // usadas, luego las «old» (si se pide) y por último las de las convocatorias registradas, de la más antigua
    // a la más reciente.
    QHash<QString, int> usoReciente;    // código (T1-7a) -> última convocatoria en que salió (1 = la más reciente)
    bool evitarOld = false;
    quint32 semilla = 0;                // 0 = aleatoria
};

struct PreguntaExamen
{
    Pregunta pregunta;
    std::array<int, 4> orden{{0, 1, 2, 3}};  // opción mostrada i -> opción original orden[i]

    QString opcion(int i) const { return pregunta.opciones.value(orden[i]); }
    int correcta() const
    {
        for (int i = 0; i < 4; ++i)
            if (orden[i] == pregunta.correcta)
                return i;
        return 0;
    }
};

struct Modelo
{
    QString nombre;                     // p. ej. "Nacional Original"
    QVector<PreguntaExamen> preguntas;

    // Cuántas veces es correcta cada letra (a, b, c, d) tal como se muestran las opciones.
    std::array<int, 4> correctasPorLetra() const
    {
        std::array<int, 4> cuenta{};
        for (const PreguntaExamen &pe : preguntas)
            ++cuenta[pe.correcta()];
        return cuenta;
    }
};

struct ResultadoGeneracion
{
    QVector<Modelo> modelos;
    QStringList avisos;
    quint32 semilla = 0;
};

// Elige preguntas al azar respetando:
//  - el número de preguntas pedido por tema;
//  - que no haya dos variantes de la misma pregunta (3a/3b) ni pseudorrepetidas entre temas en un modelo;
//  - opcionalmente, que no se repitan preguntas entre los modelos de la misma convocatoria.
class Generador
{
public:
    static ResultadoGeneracion generar(const QVector<Pregunta> &banco, const ConfigExamen &config);

    // Cambia la pregunta 'indice' del examen 'modelo' por otra del mismo tema, elegida con las reglas de 'config'
    // como al generar: nunca una variante o pseudorrepetida de las demás del examen; si no se quieren repetir,
    // mejor una que no esté en los otros exámenes; y mejor las que no han salido antes o hace más tiempo. Las de
    // los grupos 'descartados' («T2-5», ya quitados de ese examen) solo vuelven si no queda otra. La nueva va en el
    // mismo sitio y, si se barajan las opciones, con la correcta en la misma letra, así que el resto no cambia.
    // Pone al día los avisos de ese examen. Devuelve false, sin cambiar nada, si no hay ninguna que pueda ir.
    static bool cambiarPregunta(const QVector<Pregunta> &banco, const ConfigExamen &config, ResultadoGeneracion &resultado,
                                int modelo, int indice, const QSet<QString> &descartados);

    // Número de preguntas distintas (sin contar variantes) disponibles por tema.
    static QMap<int, int> gruposDisponibles(const QVector<Pregunta> &banco, bool incluirParaRevisar);
};
