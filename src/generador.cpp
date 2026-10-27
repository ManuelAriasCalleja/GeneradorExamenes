#include "generador.h"
#include "estilo.h"

#include <QHash>
#include <QRandomGenerator>
#include <QSet>
#include <algorithm>

namespace {

// Conjuntos disjuntos para tratar como una sola pregunta las pseudorrepetidas entre temas (T3-5 ~ T1-7).
class ConjuntosDisjuntos
{
public:
    QString raiz(const QString &k)
    {
        QString r = k;
        while (m_padre.contains(r) && m_padre.value(r) != r)
            r = m_padre.value(r);
        return r;
    }
    void unir(const QString &a, const QString &b)
    {
        const QString ra = raiz(a), rb = raiz(b);
        if (ra != rb)
            m_padre.insert(ra, rb);
    }

private:
    QHash<QString, QString> m_padre;
};

bool usable(const Pregunta &p, bool incluirParaRevisar)
{
    return p.activa() && (incluirParaRevisar || !p.revisar);
}

// Pone la correcta de la pregunta en la letra 'letra' y las otras tres opciones al azar en las demás.
void ponerCorrectaEn(PreguntaExamen &pe, int letra, QRandomGenerator &rng)
{
    std::array<int, 3> otras{};
    for (int o = 0, k = 0; o < 4; ++o)
        if (o != pe.pregunta.correcta)
            otras[k++] = o;
    std::shuffle(otras.begin(), otras.end(), rng);
    for (int pos = 0, k = 0; pos < 4; ++pos)
        pe.orden[pos] = pos == letra ? pe.pregunta.correcta : otras[k++];
}

// Baraja las opciones de las preguntas de un examen de modo que la correcta quede repartida por igual entre a),
// b), c) y d) y, si se puede, sin caer tres veces seguidas en la misma letra. Las preguntas con opciones que se
// refieren a otras («Todas las anteriores»...) se quedan como están.
void repartirCorrectas(QVector<PreguntaExamen> &preguntas, QRandomGenerator &rng)
{
    QVector<int> barajables;
    std::array<int, 4> cuenta{};
    for (int i = 0; i < preguntas.size(); ++i) {
        if (Estilo::tieneOpcionesDependientes(preguntas.at(i).pregunta))
            ++cuenta[preguntas.at(i).pregunta.correcta];
        else
            barajables << i;
    }

    // Letra de la correcta para cada pregunta barajable: cada vez, la que menos ha salido (a igualdad, al azar).
    QVector<int> letras;
    for (int k = 0; k < barajables.size(); ++k) {
        const int minimo = *std::min_element(cuenta.begin(), cuenta.end());
        QVector<int> menos;
        for (int l = 0; l < 4; ++l)
            if (cuenta[l] == minimo)
                menos << l;
        const int l = menos.at(rng.bounded(qsizetype(menos.size())));
        ++cuenta[l];
        letras << l;
    }

    // Se reparten en un orden al azar; si salen tres seguidas iguales, se prueba con otros.
    auto rachaMasLarga = [&](const QVector<int> &asignadas) {
        int racha = 0, masLarga = 0, anterior = -1;
        for (int i = 0, j = 0; i < preguntas.size(); ++i) {
            const int l = j < barajables.size() && barajables.at(j) == i ? asignadas.at(j++) : preguntas.at(i).pregunta.correcta;
            racha = l == anterior ? racha + 1 : 1;
            anterior = l;
            masLarga = qMax(masLarga, racha);
        }
        return masLarga;
    };
    std::shuffle(letras.begin(), letras.end(), rng);
    for (int intento = 0; intento < 200 && rachaMasLarga(letras) > 2; ++intento) {
        QVector<int> otras = letras;
        std::shuffle(otras.begin(), otras.end(), rng);
        if (rachaMasLarga(otras) < rachaMasLarga(letras))
            letras = otras;
    }

    for (int j = 0; j < barajables.size(); ++j)
        ponerCorrectaEn(preguntas[barajables.at(j)], letras.at(j), rng);
}

// Estado de la elección de las preguntas de una convocatoria: las preguntas que se pueden usar, por tema y número;
// los grupos enlazados como pseudorrepetidos; cuánto se usó antes cada uno, y lo que ya se ha elegido.
class Seleccion
{
public:
    Seleccion(const QVector<Pregunta> &banco, const ConfigExamen &config, QRandomGenerator &rng);

    // Las 'pedidas' preguntas de un tema para un examen. 'usadosModelo' son los grupos ya elegidos en ese examen,
    // que no se repiten en él; 'repetida' se pone a true si hay que repetir un grupo de otro examen.
    QVector<PreguntaExamen> elegirDelTema(int tema, int pedidas, QSet<QString> &usadosModelo, bool &repetida);
    // Avisos de los temas en que no han quedado bastantes preguntas sin usar antes.
    QStringList avisosUsoAnterior() const;
    // La pregunta que va en lugar de la 'indice' del examen 'modelo' (ver Generador::cambiarPregunta), o una con id 0
    // si no hay ninguna. En 'aviso', por qué no es de las deseables, si es el caso.
    Pregunta sustituta(const QVector<Modelo> &modelos, int modelo, int indice, const QSet<QString> &descartados,
                       QString &aviso);

private:
    // Uso anterior de una pregunta: 0 = nunca; 1 = marcada «old» (si se evitan); 2 o más = salió en una
    // convocatoria registrada (cuanto más reciente, más alto).
    int usoAnterior(const Pregunta &p) const;
    // Variante al azar entre las mejores: las que no se han usado en esta convocatoria y, de ellas, las que no han
    // salido antes o hace más tiempo.
    Pregunta elegirVariante(const QVector<Pregunta> &variantes);
    QString raiz(int tema, int numero) { return m_pseudo.raiz(Pregunta::claveGrupo(tema, numero)); }

    const ConfigExamen &m_config;
    QRandomGenerator &m_rng;
    QMap<int, QMap<int, QVector<Pregunta>>> m_porTema;   // tema -> número de pregunta -> variantes
    ConjuntosDisjuntos m_pseudo;
    int m_masAntigua = 0;                    // antigüedad de la convocatoria más antigua de usoReciente
    QHash<QString, int> m_usoAnteriorGrupo;  // el de la pregunta más usada de cada grupo y sus pseudorrepetidas
    QMap<int, int> m_usadasAntes;            // tema -> mayor uso anterior de las preguntas elegidas
    QHash<QString, int> m_usosGrupo;         // veces que se ha usado cada grupo en la convocatoria
    QSet<int> m_idsUsados;                   // preguntas concretas ya usadas
};

Seleccion::Seleccion(const QVector<Pregunta> &banco, const ConfigExamen &config, QRandomGenerator &rng)
    : m_config(config), m_rng(rng)
{
    for (const Pregunta &p : banco) {
        if (!usable(p, config.incluirParaRevisar))
            continue;
        m_porTema[p.temaNumero][p.numero].append(p);
        if (!p.pseudoCon.trimmed().isEmpty())
            m_pseudo.unir(p.grupo(), p.pseudoCon.trimmed().toUpper());
    }
    for (int a : config.usoReciente)
        m_masAntigua = qMax(m_masAntigua, a);
    for (const QMap<int, QVector<Pregunta>> &grupos : std::as_const(m_porTema))
        for (const QVector<Pregunta> &variantes : grupos)
            for (const Pregunta &p : variantes) {
                const QString r = m_pseudo.raiz(p.grupo());
                m_usoAnteriorGrupo[r] = qMax(m_usoAnteriorGrupo.value(r), usoAnterior(p));
            }
}

int Seleccion::usoAnterior(const Pregunta &p) const
{
    const int a = m_config.usoReciente.value(p.codigoCompleto());
    if (a > 0)
        return 2 + m_masAntigua - a;
    return m_config.evitarOld && p.old ? 1 : 0;
}

QVector<PreguntaExamen> Seleccion::elegirDelTema(int tema, int pedidas, QSet<QString> &usadosModelo, bool &repetida)
{
    const QMap<int, QVector<Pregunta>> grupos = m_porTema.value(tema);
    QVector<int> numeros = grupos.keys();
    std::shuffle(numeros.begin(), numeros.end(), m_rng);
    // Primero las que aún no han salido en esta convocatoria (si no se quieren repetir) y, de ellas, las que no se
    // han usado antes o hace más tiempo.
    std::stable_sort(numeros.begin(), numeros.end(), [&](int a, int b) {
        const QString ra = raiz(tema, a), rb = raiz(tema, b);
        if (m_config.noRepetirEntreModelos && m_usosGrupo.value(ra) != m_usosGrupo.value(rb))
            return m_usosGrupo.value(ra) < m_usosGrupo.value(rb);
        return m_usoAnteriorGrupo.value(ra) < m_usoAnteriorGrupo.value(rb);
    });

    QVector<PreguntaExamen> elegidas;
    for (int num : numeros) {
        if (elegidas.size() == pedidas)
            break;
        const QString r = raiz(tema, num);
        if (usadosModelo.contains(r))
            continue;   // pseudorrepetida con otra ya elegida en este examen
        if (m_usosGrupo.value(r) > 0)
            repetida = true;
        usadosModelo.insert(r);
        m_usosGrupo[r] += 1;
        if (m_usoAnteriorGrupo.value(r) > 0)
            m_usadasAntes[tema] = qMax(m_usadasAntes.value(tema), m_usoAnteriorGrupo.value(r));
        PreguntaExamen pe;
        pe.pregunta = elegirVariante(grupos.value(num));
        elegidas << pe;
    }
    return elegidas;
}

Pregunta Seleccion::elegirVariante(const QVector<Pregunta> &variantes)
{
    auto coste = [this](const Pregunta &v) { return std::make_pair(m_idsUsados.contains(v.id) ? 1 : 0, usoAnterior(v)); };
    std::pair<int, int> mejor = coste(variantes.first());
    for (const Pregunta &v : variantes)
        mejor = std::min(mejor, coste(v));
    QVector<Pregunta> candidatas;
    for (const Pregunta &v : variantes)
        if (coste(v) == mejor)
            candidatas.append(v);
    const Pregunta elegida = candidatas.at(m_rng.bounded(qsizetype(candidatas.size())));
    m_idsUsados.insert(elegida.id);
    return elegida;
}

Pregunta Seleccion::sustituta(const QVector<Modelo> &modelos, int modelo, int indice, const QSet<QString> &descartados,
                              QString &aviso)
{
    const PreguntaExamen &quitada = modelos.at(modelo).preguntas.at(indice);
    const int tema = quitada.pregunta.temaNumero;
    // Grupos de las demás preguntas del examen (no pueden volver a salir en él), veces que está cada grupo en los
    // otros exámenes y grupos ya quitados de este examen, la quitada incluida.
    QSet<QString> enElExamen, quitados;
    QHash<QString, int> enOtros;
    for (int m = 0; m < modelos.size(); ++m) {
        for (int i = 0; i < modelos.at(m).preguntas.size(); ++i) {
            const Pregunta &p = modelos.at(m).preguntas.at(i).pregunta;
            const QString r = raiz(p.temaNumero, p.numero);
            if (m != modelo) {
                enOtros[r] += 1;
                m_idsUsados.insert(p.id);
            } else if (i != indice) {
                enElExamen.insert(r);
            }
        }
    }
    quitados.insert(raiz(tema, quitada.pregunta.numero));
    for (const QString &g : descartados)
        quitados.insert(m_pseudo.raiz(g));

    // Coste de cada candidata, de lo que más pesa a lo que menos: ya quitada de este examen; repetida en otro
    // examen (si no se quieren repetir); uso anterior de su grupo; variante ya usada en otro examen; uso anterior
    // de la variante; y, si se barajan las opciones, correcta fija en otra letra que la de la quitada.
    const int letra = quitada.correcta();
    using Coste = std::array<int, 6>;
    auto coste = [&](const Pregunta &v) {
        const QString r = raiz(tema, v.numero);
        return Coste{quitados.contains(r) ? 1 : 0, m_config.noRepetirEntreModelos ? enOtros.value(r) : 0,
                     m_usoAnteriorGrupo.value(r), m_idsUsados.contains(v.id) ? 1 : 0, usoAnterior(v),
                     m_config.barajarOpciones && Estilo::tieneOpcionesDependientes(v) && v.correcta != letra ? 1 : 0};
    };
    // Las mejores variantes de cada grupo y, de los grupos, los de menor coste: se elige un grupo al azar (todos con
    // la misma probabilidad, tengan las variantes que tengan) y una de sus mejores variantes.
    QVector<QVector<Pregunta>> mejores;
    Coste minimo{};
    const QMap<int, QVector<Pregunta>> grupos = m_porTema.value(tema);
    for (auto it = grupos.cbegin(); it != grupos.cend(); ++it) {
        if (enElExamen.contains(raiz(tema, it.key())))
            continue;
        QVector<Pregunta> variantes;
        Coste mejorDelGrupo{};
        for (const Pregunta &v : it.value()) {
            if (v.id == quitada.pregunta.id)
                continue;
            const Coste c = coste(v);
            if (variantes.isEmpty() || c < mejorDelGrupo) {
                mejorDelGrupo = c;
                variantes.clear();
            }
            if (c == mejorDelGrupo)
                variantes << v;
        }
        if (variantes.isEmpty())
            continue;
        if (mejores.isEmpty() || mejorDelGrupo < minimo) {
            minimo = mejorDelGrupo;
            mejores.clear();
        }
        if (mejorDelGrupo == minimo)
            mejores << variantes;
    }
    if (mejores.isEmpty())
        return Pregunta();
    const QVector<Pregunta> &grupo = mejores.at(m_rng.bounded(qsizetype(mejores.size())));
    const Pregunta elegida = grupo.at(m_rng.bounded(qsizetype(grupo.size())));

    if (minimo[0])
        aviso = QStringLiteral("no queda ninguna otra pregunta del tema %1; %2 es (o es variante de) una de las ya quitadas.")
                    .arg(tema).arg(elegida.codigoCompleto());
    else if (minimo[1])
        aviso = QStringLiteral("%1 está también en otro examen de la convocatoria: no queda ninguna otra del tema %2.")
                    .arg(elegida.codigoCompleto()).arg(tema);
    else if (minimo[2] >= 2)
        aviso = QStringLiteral("%1 salió en una de las últimas convocatorias registradas: no queda ninguna otra del "
                               "tema %2 sin usar.").arg(elegida.codigoCompleto()).arg(tema);
    else if (minimo[2] == 1)
        aviso = QStringLiteral("%1 está marcada «old»: no queda ninguna otra del tema %2 sin usar.")
                    .arg(elegida.codigoCompleto()).arg(tema);
    return elegida;
}

QStringList Seleccion::avisosUsoAnterior() const
{
    QStringList avisos;
    for (auto it = m_usadasAntes.cbegin(); it != m_usadasAntes.cend(); ++it)
        avisos << (it.value() >= 2 ? QStringLiteral("Tema %1: no quedan bastantes preguntas sin usar en las últimas "
                                                     "convocatorias registradas; se repiten algunas (o variantes suyas).")
                                   : QStringLiteral("Tema %1: no quedan bastantes preguntas sin la marca «old»; "
                                                     "se usan algunas (o variantes suyas)."))
                      .arg(it.key());
    return avisos;
}

// Por tema y número, como en los exámenes oficiales, o al azar.
void ordenar(QVector<PreguntaExamen> &preguntas, bool alAzar, QRandomGenerator &rng)
{
    if (alAzar)
        std::shuffle(preguntas.begin(), preguntas.end(), rng);
    else
        std::sort(preguntas.begin(), preguntas.end(),
                  [](const PreguntaExamen &a, const PreguntaExamen &b) { return a.pregunta.antesQue(b.pregunta); });
}

// Aviso si la correcta queda muy desigualmente repartida: una letra con dos o más de las que le tocarían.
QString avisoReparto(const Modelo &modelo, bool barajadas)
{
    const std::array<int, 4> cuenta = modelo.correctasPorLetra();
    const int n = modelo.preguntas.size();
    const int masRepetida = int(std::max_element(cuenta.begin(), cuenta.end()) - cuenta.begin());
    if (n < 4 || cuenta[masRepetida] <= (n + 3) / 4 + 1)
        return QString();
    return QStringLiteral("«%1»: la respuesta correcta es la %2) en %3 de %4 preguntas%5.")
        .arg(modelo.nombre, QString(Pregunta::letra(masRepetida)))
        .arg(cuenta[masRepetida])
        .arg(n)
        .arg(barajadas ? QString() : QStringLiteral("; marque «Barajar las opciones» para repartirla"));
}

} // namespace

bool Generador::cambiarPregunta(const QVector<Pregunta> &banco, const ConfigExamen &config, ResultadoGeneracion &resultado,
                                int modelo, int indice, const QSet<QString> &descartados)
{
    if (modelo < 0 || modelo >= resultado.modelos.size() || indice < 0 || indice >= resultado.modelos.at(modelo).preguntas.size())
        return false;
    QRandomGenerator rng(QRandomGenerator::global()->generate());
    Seleccion seleccion(banco, config, rng);
    QString aviso;
    const Pregunta nueva = seleccion.sustituta(resultado.modelos, modelo, indice, descartados, aviso);
    if (nueva.id == 0)
        return false;

    Modelo &m = resultado.modelos[modelo];
    PreguntaExamen &pe = m.preguntas[indice];
    const int letra = pe.correcta();
    pe = PreguntaExamen();
    pe.pregunta = nueva;
    if (config.barajarOpciones && !Estilo::tieneOpcionesDependientes(nueva))
        ponerCorrectaEn(pe, letra, rng);

    // Los avisos de este examen que pueden haber cambiado: el del reparto de la correcta («Nacional Original»: …)
    // y el de la pregunta de este sitio («Nacional Original», pregunta 7: …).
    const QString delReparto = QStringLiteral("«%1»:").arg(m.nombre);
    const QString delSitio = QStringLiteral("«%1», pregunta %2: ").arg(m.nombre).arg(indice + 1);
    resultado.avisos.removeIf([&](const QString &a) { return a.startsWith(delReparto) || a.startsWith(delSitio); });
    if (!aviso.isEmpty())
        resultado.avisos << delSitio + aviso;
    const QString reparto = avisoReparto(m, config.barajarOpciones);
    if (!reparto.isEmpty())
        resultado.avisos << reparto;
    return true;
}

QMap<int, int> Generador::gruposDisponibles(const QVector<Pregunta> &banco, bool incluirParaRevisar)
{
    QMap<int, QSet<int>> grupos;
    for (const Pregunta &p : banco)
        if (usable(p, incluirParaRevisar))
            grupos[p.temaNumero].insert(p.numero);
    QMap<int, int> res;
    for (auto it = grupos.cbegin(); it != grupos.cend(); ++it)
        res.insert(it.key(), it.value().size());
    return res;
}

ResultadoGeneracion Generador::generar(const QVector<Pregunta> &banco, const ConfigExamen &config)
{
    ResultadoGeneracion res;
    res.semilla = config.semilla;
    while (res.semilla == 0)
        res.semilla = QRandomGenerator::global()->generate();
    QRandomGenerator rng(res.semilla);
    Seleccion seleccion(banco, config, rng);
    QSet<int> temasAvisoRepeticion, temasAvisoFaltan;

    for (const QString &nombre : config.nombresModelos) {
        Modelo modelo;
        modelo.nombre = nombre;
        QSet<QString> usadosModelo;
        for (auto it = config.preguntasPorTema.cbegin(); it != config.preguntasPorTema.cend(); ++it) {
            const int tema = it.key();
            const int pedidas = it.value();
            if (pedidas <= 0)
                continue;
            bool repetida = false;
            const QVector<PreguntaExamen> elegidas = seleccion.elegirDelTema(tema, pedidas, usadosModelo, repetida);
            modelo.preguntas += elegidas;
            if (elegidas.size() < pedidas && !temasAvisoFaltan.contains(tema)) {
                temasAvisoFaltan.insert(tema);
                res.avisos << QStringLiteral("Tema %1: se pidieron %2 preguntas pero solo hay %3 distintas disponibles.")
                                  .arg(tema).arg(pedidas).arg(elegidas.size());
            }
            if (repetida && config.noRepetirEntreModelos && !temasAvisoRepeticion.contains(tema)) {
                temasAvisoRepeticion.insert(tema);
                res.avisos << QStringLiteral("Tema %1: no hay preguntas suficientes para no repetir entre exámenes; "
                                             "a partir del examen «%2» se repiten algunas.")
                                  .arg(tema).arg(modelo.nombre);
            }
        }
        ordenar(modelo.preguntas, config.ordenAleatorio, rng);
        if (config.barajarOpciones)
            repartirCorrectas(modelo.preguntas, rng);
        const QString aviso = avisoReparto(modelo, config.barajarOpciones);
        if (!aviso.isEmpty())
            res.avisos << aviso;
        res.modelos.append(modelo);
    }
    res.avisos << seleccion.avisosUsoAnterior();
    return res;
}
