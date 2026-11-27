#pragma once

#include "pregunta.h"

#include <QCoreApplication>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QVector>
#include <array>

// Cómo salió una pregunta en los exámenes registrados en el historial. Si vuelve a salir, sus opciones se barajan
// siempre, con la correcta en otra letra (ver ConfigExamen::salidas).
struct SalidaAnterior
{
    QString convocatoria;   // la más reciente en que salió («Junio 2025»), para los avisos
    QVector<int> letras;    // letra de la correcta (0..3) cada vez que salió, de la más reciente a la más antigua
    QStringList convocatorias;   // en qué convocatoria salió cada vez, como 'letras' (si falta, 'convocatoria')

    // La convocatoria más reciente en que la correcta estuvo en 'letra'.
    QString convocatoriaConLetra(int letra) const
    {
        const int i = int(letras.indexOf(letra));
        return i >= 0 && i < convocatorias.size() ? convocatorias.at(i) : convocatoria;
    }
};

struct ConfigExamen
{
    QMap<int, int> preguntasPorTema;    // número de tema -> número de preguntas
    QStringList nombresModelos;         // un modelo por nombre (p. ej. "Nacional Original"), en este orden
    // Versiones de cada modelo de nombresModelos (las que falten: 1, sin versiones). Con varias, cada versión es un
    // examen más («Nacional Original A», «… B»…), con las mismas preguntas, siempre por temas (las del tema 1, las del
    // 2…): la A, como saldría el modelo; las demás, con las preguntas de cada tema en otro orden y las opciones
    // barajadas. Con ordenAleatorio, también la A se desordena dentro de cada tema y lleva las opciones barajadas.
    QVector<int> versiones;
    bool versionesConNumeros = false;   // las versiones, «A, B, C…» o «1, 2, 3…»
    int numVersiones(int modelo) const;
    bool noRepetirEntreModelos = true;  // dentro de una misma convocatoria
    bool incluirParaRevisar = true;     // preguntas marcadas MEJORAR/REFINAR...
    bool soloVerificadas = false;       // solo las verificadas por personas (Pregunta::verificada)
    bool ordenAleatorio = false;        // si no, por tema y número (como en los exámenes oficiales)
    bool barajarOpciones = false;       // repartiendo la correcta por igual entre a), b), c) y d)
    // Preguntas ya usadas, que se dejan para el final (con sus variantes y pseudorrepetidas): primero las nunca
    // usadas, luego las «old» (si se pide) y por último las de las convocatorias registradas, de la más antigua
    // a la más reciente.
    QHash<QString, int> usoReciente;    // código (T1-7a) -> última convocatoria en que salió (1 = la más reciente)
    bool evitarOld = false;
    // Preguntas que ya han salido en exámenes registrados (sin contar los que se están generando de nuevo), por
    // código (T1-7a). Las que vuelven a salir, y las marcadas «old», llevan siempre las opciones barajadas, aunque no
    // se pida, y la correcta en una letra en que no estuvo (o, si estuvo en todas, en otra que la última vez; de las
    // «old» no se sabe cómo salieron: en otra que en el banco). Las que tienen opciones que remiten a otras no se
    // pueden barajar: se avisa si la correcta va donde estuvo.
    QHash<QString, SalidaAnterior> salidas;
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
    QString nombre;                     // p. ej. "Nacional Original"; con versiones, «Nacional Original B»
    // Con versiones (ConfigExamen::versiones), el nombre del examen de la lista («Nacional Original») y la etiqueta de
    // esta («B», «2»); vacíos si no tiene.
    QString base;
    QString version;
    bool barajadas = false;             // las opciones de todas sus preguntas se barajaron (salvo las que no se puede)
    QVector<PreguntaExamen> preguntas;

    // El nombre del examen de la lista del que sale: el suyo o, si es una versión, el de todas («Nacional Original»).
    QString nombreEnLaLista() const { return version.isEmpty() ? nombre : base; }

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
    // Cargados del historial: si se habían cambiado preguntas a mano antes de registrarlos (la semilla ya no los da).
    bool cambiadosAMano = false;
};

// Elige preguntas al azar respetando:
//  - el número de preguntas pedido por tema;
//  - que no haya dos variantes de la misma pregunta (3a/3b) ni pseudorrepetidas entre temas en un modelo;
//  - opcionalmente, que no se repitan preguntas entre los modelos de la misma convocatoria.
class Generador
{
    Q_DECLARE_TR_FUNCTIONS(Generador)

public:
    static ResultadoGeneracion generar(const QVector<Pregunta> &banco, const ConfigExamen &config);

    // Cambia la pregunta 'indice' del examen 'modelo' por otra del mismo tema, elegida con las reglas de 'config'
    // como al generar: nunca una variante o pseudorrepetida de las demás del examen; si no se quieren repetir,
    // mejor una que no esté en los otros exámenes; y mejor las que no han salido antes o hace más tiempo. Las de
    // los grupos 'descartados' («T2-5», ya quitados de ese examen) solo vuelven si no queda otra. La nueva va en el
    // mismo sitio y, si se barajan las opciones, con la correcta en la misma letra (o en una de las que menos salen en
    // el examen, si la quitada no se barajaba y su letra no es de esas), así que el resto no cambia.
    // Si el examen tiene versiones, la cambia en todas, cada una en el sitio en que la tenga. Pone al día los avisos de
    // esos exámenes. Devuelve false, sin cambiar nada, si no hay ninguna que pueda ir.
    static bool cambiarPregunta(const QVector<Pregunta> &banco, const ConfigExamen &config, ResultadoGeneracion &resultado,
                                int modelo, int indice, const QSet<QString> &descartados);

    // Pone en los exámenes 'corregida', la pregunta 'antes' después de editarla en el banco: en cada sitio en que esté
    // 'antes' (la misma pregunta: el mismo id y el mismo código). Las opciones se quedan en el orden en que salieron,
    // salvo que la correcta sea otra y estuvieran barajadas (entonces la nueva correcta va a la letra de la anterior,
    // como al cambiar una pregunta) o que ahora haya alguna que remita a las demás (entonces van sin barajar, como al
    // generar). Pone al día los avisos del reparto de la correcta. Devuelve en cuántos sitios estaba.
    static int ponerCorregida(ResultadoGeneracion &resultado, const ConfigExamen &config, const Pregunta &antes,
                              const Pregunta &corregida);

    // Pone en los exámenes la verificación de 'p' (en cada sitio en que esté la misma pregunta: el mismo id y el mismo
    // código), sin cambiar nada más. Devuelve en cuántos sitios estaba.
    static int ponerVerificacion(ResultadoGeneracion &resultado, const Pregunta &p);

    // Versiones: como mucho, una por letra; su etiqueta (la 0, «A» o «1») y los nombres de los exámenes que salen de
    // 'config', con sus versiones («Uno A», «Uno B», «Dos»).
    static constexpr int kMaxVersiones = 26;
    static QString etiquetaVersion(int indice, bool conNumeros);
    static QStringList nombresExamenes(const ConfigExamen &config);
    // Los exámenes de 'resultado' que son versiones del mismo que 'modelo' (él incluido), en orden; solo él si no tiene.
    static QVector<int> versionesDe(const ResultadoGeneracion &resultado, int modelo);

    // Para exámenes ya generados (o cargados del historial): los de 'resultado' (por su nombre en la lista,
    // Modelo::nombreEnLaLista()) a los que 'config' pide más versiones (ConfigExamen::versiones, por ese nombre) de las
    // que tienen. Y añadírselas sin cambiar las que ya están: un examen sin versiones pasa a ser la A tal cual (solo
    // cambia su nombre, «Nacional Original A»), aunque lleve las preguntas en orden aleatorio, y las demás salen de ella
    // como al generar, siempre por temas, con las preguntas de cada tema en otro orden y las opciones barajadas. Las que
    // se añaden a un examen que ya tenía versiones llevan la numeración de las suyas. Todas las de un examen van juntas,
    // donde estaba la primera.
    // Con la serie de números al azar de las versiones de generar(): a unos exámenes generados sin orden aleatorio, y
    // que no han cambiado, se les añaden las mismas que si se hubieran pedido al generarlos. Pone los avisos de las
    // nuevas y, en los del examen que pasa a ser la A, su nombre nuevo. Devuelve los nombres en la lista de los
    // exámenes a los que se ha añadido alguna.
    static QStringList faltanVersiones(const ResultadoGeneracion &resultado, const ConfigExamen &config);
    static QStringList anadirVersiones(ResultadoGeneracion &resultado, const ConfigExamen &config);

    // Número de preguntas distintas (sin contar variantes) disponibles por tema.
    static QMap<int, int> gruposDisponibles(const QVector<Pregunta> &banco, bool incluirParaRevisar, bool soloVerificadas = false);
};
