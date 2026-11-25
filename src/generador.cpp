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

bool usable(const Pregunta &p, bool incluirParaRevisar, bool soloVerificadas)
{
    return p.activa() && (incluirParaRevisar || !p.revisar) && (!soloVerificadas || p.verificada);
}

// Letras (bits 0 a 3: a, b, c, d) en que puede ir la correcta de 'p': todas, salvo que ya haya salido (ver
// ConfigExamen::salidas). Entonces, las que no tuvo nunca o, si las tuvo todas, las que no tuvo la última vez; y si
// está marcada «old», las que no tiene en el banco.
int letrasPermitidas(const Pregunta &p, const ConfigExamen &config)
{
    constexpr int todas = 0b1111;
    const auto salida = config.salidas.constFind(p.codigoCompleto());
    if (salida != config.salidas.cend() && !salida->letras.isEmpty()) {
        int nunca = todas;
        for (int l : salida->letras)
            nunca &= ~(1 << l);
        return nunca ? nunca : todas & ~(1 << salida->letras.first());
    }
    return p.old ? todas & ~(1 << p.correcta) : todas;
}

bool yaSalio(const Pregunta &p, const ConfigExamen &config)
{
    return letrasPermitidas(p, config) != 0b1111;
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
// b), c) y d) y, si se puede, sin caer tres veces seguidas en la misma letra. Con 'todas' a false, solo las de las
// preguntas que ya han salido: las demás se quedan como están, y cuentan con la letra que tienen. La correcta de las
// que ya han salido va en una de sus letras permitidas (letrasPermitidas()). Las preguntas con opciones que se
// refieren a otras («Todas las anteriores»...) se quedan como están.
void repartirCorrectas(QVector<PreguntaExamen> &preguntas, QRandomGenerator &rng, const ConfigExamen &config, bool todas)
{
    QVector<int> barajables;
    std::array<int, 4> cuenta{};
    for (int i = 0; i < preguntas.size(); ++i) {
        const Pregunta &p = preguntas.at(i).pregunta;
        if (!Estilo::tieneOpcionesDependientes(p) && (todas || yaSalio(p, config)))
            barajables << i;
        else
            ++cuenta[preguntas.at(i).correcta()];
    }
    if (barajables.isEmpty())
        return;

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
            const int l = j < barajables.size() && barajables.at(j) == i ? asignadas.at(j++) : preguntas.at(i).correcta();
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

    // Las que ya han salido, con la correcta en una de sus letras permitidas: si le ha tocado otra, se cambia por la de
    // otra pregunta a la que le valga la suya (mejor si no alarga las rachas) y, si no hay ninguna, se le da la
    // permitida que menos sale. Sin usar 'rng': sin preguntas que ya hayan salido, todo queda como antes, y con la
    // misma semilla salen los mismos exámenes.
    QVector<int> permitidas;
    for (int i : std::as_const(barajables))
        permitidas << letrasPermitidas(preguntas.at(i).pregunta, config);
    auto vale = [&](int j, int letra) { return (permitidas.at(j) & (1 << letra)) != 0; };
    for (int j = 0; j < barajables.size(); ++j) {
        if (vale(j, letras.at(j)))
            continue;
        const int racha = qMax(2, rachaMasLarga(letras));
        int primera = -1, sinAlargar = -1;
        for (int k = 0; k < barajables.size() && sinAlargar < 0; ++k) {
            if (k == j || !vale(j, letras.at(k)) || !vale(k, letras.at(j)))
                continue;
            if (primera < 0)
                primera = k;
            QVector<int> otras = letras;
            std::swap(otras[j], otras[k]);
            if (rachaMasLarga(otras) <= racha)
                sinAlargar = k;
        }
        const int k = sinAlargar >= 0 ? sinAlargar : primera;
        if (k >= 0) {
            std::swap(letras[j], letras[k]);
            continue;
        }
        int mejor = -1;
        for (int l = 0; l < 4; ++l)
            if (vale(j, l) && (mejor < 0 || cuenta[l] < cuenta[mejor]))
                mejor = l;
        --cuenta[letras.at(j)];
        ++cuenta[mejor];
        letras[j] = mejor;
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
        if (!usable(p, config.incluirParaRevisar, config.soloVerificadas))
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
    // de la variante; correcta fija (sus opciones no se barajan) en una letra en que ya estuvo; y, si se barajan las
    // opciones, correcta fija en otra letra que la de la quitada.
    const int letra = quitada.correcta();
    const bool barajadas = m_config.barajarOpciones || modelos.at(modelo).barajadas;
    using Coste = std::array<int, 7>;
    auto coste = [&](const Pregunta &v) {
        const QString r = raiz(tema, v.numero);
        const bool fija = Estilo::tieneOpcionesDependientes(v);
        return Coste{quitados.contains(r) ? 1 : 0, m_config.noRepetirEntreModelos ? enOtros.value(r) : 0,
                     m_usoAnteriorGrupo.value(r), m_idsUsados.contains(v.id) ? 1 : 0, usoAnterior(v),
                     fija && !(letrasPermitidas(v, m_config) & (1 << v.correcta)) ? 1 : 0,
                     barajadas && fija && v.correcta != letra ? 1 : 0};
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
        //: Va detrás de «Examen», pregunta N: (por eso empieza en minúscula). %2 es el código de una pregunta (T1-7a).
        aviso = Generador::tr("no queda ninguna otra pregunta del tema %1; %2 es (o es variante de) una de las ya quitadas.")
                    .arg(tema).arg(elegida.codigoCompleto());
    else if (minimo[1])
        aviso = Generador::tr("%1 está también en otro examen de la convocatoria: no queda ninguna otra del tema %2.")
                    .arg(elegida.codigoCompleto()).arg(tema);
    else if (minimo[2] >= 2)
        aviso = Generador::tr("%1 salió en una de las últimas convocatorias registradas: no queda ninguna otra del "
                              "tema %2 sin usar.").arg(elegida.codigoCompleto()).arg(tema);
    else if (minimo[2] == 1)
        //: «old» es la marca de las preguntas que ya han salido en exámenes (la casilla «Ya ha salido en exámenes (old)»).
        aviso = Generador::tr("%1 está marcada «old»: no queda ninguna otra del tema %2 sin usar.")
                    .arg(elegida.codigoCompleto()).arg(tema);
    return elegida;
}

QStringList Seleccion::avisosUsoAnterior() const
{
    QStringList avisos;
    for (auto it = m_usadasAntes.cbegin(); it != m_usadasAntes.cend(); ++it)
        avisos << (it.value() >= 2 ? Generador::tr("Tema %1: no quedan bastantes preguntas sin usar en las últimas "
                                                   "convocatorias registradas; se repiten algunas (o variantes suyas).")
                                   : Generador::tr("Tema %1: no quedan bastantes preguntas sin la marca «old»; "
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

// «Nacional Original B»: el nombre de una versión.
QString nombreConVersion(const QString &base, const QString &version)
{
    return base + QLatin1Char(' ') + version;
}

// Cambia el orden de las preguntas dentro de cada tema, sin mezclar los temas (van seguidas las de cada uno).
void barajarDentroDeCadaTema(QVector<PreguntaExamen> &preguntas, QRandomGenerator &rng)
{
    for (int inicio = 0; inicio < preguntas.size();) {
        int fin = inicio + 1;
        while (fin < preguntas.size() && preguntas.at(fin).pregunta.temaNumero == preguntas.at(inicio).pregunta.temaNumero)
            ++fin;
        std::shuffle(preguntas.begin() + inicio, preguntas.begin() + fin, rng);
        inicio = fin;
    }
}

// Las 'n' versiones de 'modelo' (ver ConfigExamen::versiones), con sus preguntas ya elegidas, ordenadas y barajadas
// como las de un examen sin versiones. Siempre por temas: las del tema 1, las del 2… La A, como el modelo; con orden
// aleatorio, con las de cada tema en el orden al azar en que ya estaban y las opciones barajadas. Las demás, con las
// de cada tema en otro orden (si se puede, distinto del de las otras versiones) y las opciones barajadas.
QVector<Modelo> versionesDelModelo(const Modelo &modelo, int n, const ConfigExamen &config, QRandomGenerator &rng)
{
    Modelo a = modelo;
    a.base = modelo.nombre;
    if (config.ordenAleatorio) {
        std::stable_sort(a.preguntas.begin(), a.preguntas.end(), [](const PreguntaExamen &x, const PreguntaExamen &y) {
            return x.pregunta.temaNumero < y.pregunta.temaNumero;
        });
        repartirCorrectas(a.preguntas, rng, config, true);
        a.barajadas = true;
    }
    auto orden = [](const Modelo &m) {
        QVector<int> ids;
        for (const PreguntaExamen &pe : m.preguntas)
            ids << pe.pregunta.id;
        return ids;
    };
    QVector<Modelo> versiones;
    QVector<QVector<int>> ordenes;
    for (int v = 0; v < n; ++v) {
        Modelo m = a;
        m.version = Generador::etiquetaVersion(v, config.versionesConNumeros);
        m.nombre = nombreConVersion(a.base, m.version);
        if (v > 0) {
            for (int intento = 0; intento < 50; ++intento) {
                barajarDentroDeCadaTema(m.preguntas, rng);
                if (!ordenes.contains(orden(m)))
                    break;
            }
            repartirCorrectas(m.preguntas, rng, config, true);
            m.barajadas = true;
        }
        ordenes << orden(m);
        versiones << m;
    }
    return versiones;
}

// Comienzo de los avisos sobre un examen entero («Nacional Original»: …). Con él se reconocen al cambiar una de sus
// preguntas, así que el aviso y la búsqueda lo toman de aquí, en el idioma que sea.
QString inicioAvisoExamen(const QString &nombre)
{
    //: Comienzo de un aviso sobre un examen entero: %1 es su nombre. Detrás va el aviso («la respuesta correcta es…»).
    return Generador::tr("«%1»: ").arg(nombre);
}

// Comienzo de los avisos sobre una pregunta de un examen («Nacional Original», pregunta 7: …), con el que se
// reconocen al cambiarla.
QString inicioAvisoPregunta(const QString &nombre, int indice)
{
    //: Comienzo de un aviso sobre una pregunta: %1 es el nombre del examen y %2, el número de la pregunta en él.
    return Generador::tr("«%1», pregunta %2: ").arg(nombre).arg(indice + 1);
}

// Aviso si la pregunta 'indice' del examen ya ha salido y su correcta va en una letra en que estuvo, porque sus
// opciones remiten a otras y no se pueden barajar; vacío si no.
QString avisoNoBarajable(const Modelo &modelo, int indice, const ConfigExamen &config)
{
    const PreguntaExamen &pe = modelo.preguntas.at(indice);
    if (letrasPermitidas(pe.pregunta, config) & (1 << pe.correcta()))
        return QString();
    const QString codigo = pe.pregunta.codigoCompleto();
    const QString letra(Pregunta::letra(pe.correcta()));
    const auto salida = config.salidas.constFind(codigo);
    if (salida != config.salidas.cend())
        //: Va detrás de «Examen», pregunta N: . %1 es el código de una pregunta (T1-7a), %2 una convocatoria
        //: («Junio 2025») y %3 una letra (a, b, c o d).
        return inicioAvisoPregunta(modelo.nombre, indice)
               + Generador::tr("%1 ya salió en %2 con la correcta en la %3), y sus opciones no se pueden barajar "
                               "porque remiten a otras.").arg(codigo, salida->convocatoriaConLetra(pe.correcta()), letra);
    //: Va detrás de «Examen», pregunta N: . «old» es la marca de las preguntas que ya han salido en exámenes. %1 es el
    //: código de una pregunta (T1-7a) y %2 una letra (a, b, c o d).
    return inicioAvisoPregunta(modelo.nombre, indice)
           + Generador::tr("%1 está marcada «old» y sus opciones no se pueden barajar porque remiten a otras: la "
                           "correcta sigue en la %2), como en el banco.").arg(codigo, letra);
}

// Aviso si la correcta queda muy desigualmente repartida: una letra con dos o más de las que le tocarían.
QString avisoReparto(const Modelo &modelo, bool barajadas)
{
    const std::array<int, 4> cuenta = modelo.correctasPorLetra();
    const int n = modelo.preguntas.size();
    const int masRepetida = int(std::max_element(cuenta.begin(), cuenta.end()) - cuenta.begin());
    if (n < 4 || cuenta[masRepetida] <= (n + 3) / 4 + 1)
        return QString();
    //: Va detrás de «Nombre del examen»: . %1 es una letra (a, b, c o d); %4 está vacío o es «; marque «Barajar las opciones»…».
    return inicioAvisoExamen(modelo.nombre)
           + Generador::tr("la respuesta correcta es la %1) en %2 de %3 preguntas%4.")
        .arg(QString(Pregunta::letra(masRepetida)))
        .arg(cuenta[masRepetida])
        .arg(n)
        //: «Barajar las opciones» es la casilla de la ventana principal que reparte la correcta entre a)–d).
        .arg(barajadas ? QString() : Generador::tr("; marque «Barajar las opciones» para repartirla"));
}

// Pone 'nueva' en lugar de la pregunta 'indice' de 'm' (ver Generador::cambiarPregunta()) y pone al día los avisos de
// ese examen que pueden haber cambiado: el del reparto de la correcta («Nacional Original»: …) y los de ese sitio
// («Nacional Original», pregunta 7: …), con 'aviso' si no está vacío.
void ponerEnSuSitio(Modelo &m, int indice, const Pregunta &nueva, const QString &aviso, const ConfigExamen &config,
                    QStringList &avisos, QRandomGenerator &rng)
{
    PreguntaExamen &pe = m.preguntas[indice];
    int letra = pe.correcta();
    // Barajadas, la correcta va a la letra de la quitada, que deja el reparto como estaba (si estaba bien repartida,
    // es una de las que menos salen en el resto del examen). Si no lo es (la quitada tenía la correcta fija, porque sus
    // opciones remiten a otras), a una de las que menos salen: la letra de la nueva no cambia nada más del examen.
    // Si la nueva ya ha salido, se barajan aunque no se pida, y la correcta va a una de sus letras permitidas.
    const bool barajadas = config.barajarOpciones || m.barajadas;
    const bool barajar = !Estilo::tieneOpcionesDependientes(nueva) && (barajadas || yaSalio(nueva, config));
    if (barajar) {
        std::array<int, 4> cuenta{};
        for (int i = 0; i < m.preguntas.size(); ++i)
            if (i != indice)
                ++cuenta[m.preguntas.at(i).correcta()];
        const int permitidas = letrasPermitidas(nueva, config);
        int minimo = int(m.preguntas.size());
        for (int l = 0; l < 4; ++l)
            if (permitidas & (1 << l))
                minimo = qMin(minimo, cuenta[l]);
        if (!(permitidas & (1 << letra)) || cuenta[letra] != minimo) {
            QVector<int> menos;
            for (int l = 0; l < 4; ++l)
                if ((permitidas & (1 << l)) && cuenta[l] == minimo)
                    menos << l;
            letra = menos.at(rng.bounded(int(menos.size())));
        }
    }
    pe = PreguntaExamen();
    pe.pregunta = nueva;
    if (barajar)
        ponerCorrectaEn(pe, letra, rng);

    const QString delReparto = inicioAvisoExamen(m.nombre);
    const QString delSitio = inicioAvisoPregunta(m.nombre, indice);
    avisos.removeIf([&](const QString &a) { return a.startsWith(delReparto) || a.startsWith(delSitio); });
    if (!aviso.isEmpty())
        avisos << delSitio + aviso;
    const QString noBarajable = avisoNoBarajable(m, indice, config);
    if (!noBarajable.isEmpty())
        avisos << noBarajable;
    const QString reparto = avisoReparto(m, barajadas);
    if (!reparto.isEmpty())
        avisos << reparto;
}

} // namespace

int ConfigExamen::numVersiones(int modelo) const
{
    return qBound(1, versiones.value(modelo, 1), Generador::kMaxVersiones);
}

QString Generador::etiquetaVersion(int indice, bool conNumeros)
{
    return conNumeros ? QString::number(indice + 1) : QString(QChar(QLatin1Char('A').unicode() + indice));
}

QStringList Generador::nombresExamenes(const ConfigExamen &config)
{
    QStringList nombres;
    for (int i = 0; i < config.nombresModelos.size(); ++i) {
        const int n = config.numVersiones(i);
        if (n == 1)
            nombres << config.nombresModelos.at(i);
        else
            for (int v = 0; v < n; ++v)
                nombres << nombreConVersion(config.nombresModelos.at(i), etiquetaVersion(v, config.versionesConNumeros));
    }
    return nombres;
}

QVector<int> Generador::versionesDe(const ResultadoGeneracion &resultado, int modelo)
{
    if (modelo < 0 || modelo >= resultado.modelos.size())
        return {};
    const Modelo &m = resultado.modelos.at(modelo);
    if (m.version.isEmpty())
        return {modelo};
    QVector<int> res;
    for (int i = 0; i < resultado.modelos.size(); ++i)
        if (!resultado.modelos.at(i).version.isEmpty() && resultado.modelos.at(i).base == m.base)
            res << i;
    return res;
}

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

    // En este examen y, si tiene versiones, en todas, cada una en el sitio en que la tenga.
    const Pregunta quitada = resultado.modelos.at(modelo).preguntas.at(indice).pregunta;
    for (int v : versionesDe(resultado, modelo)) {
        Modelo &m = resultado.modelos[v];
        int sitio = v == modelo ? indice : -1;
        for (int i = 0; sitio < 0 && i < m.preguntas.size(); ++i)
            if (m.preguntas.at(i).pregunta.id == quitada.id && m.preguntas.at(i).pregunta.codigoCompleto() == quitada.codigoCompleto())
                sitio = i;
        if (sitio >= 0)
            ponerEnSuSitio(m, sitio, nueva, aviso, config, resultado.avisos, rng);
    }
    return true;
}

int Generador::ponerCorregida(ResultadoGeneracion &resultado, const ConfigExamen &config, const Pregunta &antes,
                              const Pregunta &corregida)
{
    QRandomGenerator rng(QRandomGenerator::global()->generate());
    const bool dependientes = Estilo::tieneOpcionesDependientes(corregida);
    const int permitidas = letrasPermitidas(corregida, config);
    int sitios = 0;
    for (Modelo &m : resultado.modelos) {
        bool enEste = false;
        for (int i = 0; i < m.preguntas.size(); ++i) {
            PreguntaExamen &pe = m.preguntas[i];
            if (pe.pregunta.id != antes.id || pe.pregunta.codigoCompleto() != antes.codigoCompleto())
                continue;
            const QString avisoAntes = avisoNoBarajable(m, i, config);
            const int letra = pe.correcta();
            const bool barajadas = config.barajarOpciones || m.barajadas || pe.orden != std::array<int, 4>{{0, 1, 2, 3}};
            const bool otraCorrecta = corregida.correcta != pe.pregunta.correcta;
            pe.pregunta = corregida;
            if (dependientes)
                pe.orden = {{0, 1, 2, 3}};
            else if (otraCorrecta && barajadas)
                ponerCorrectaEn(pe, letra, rng);
            // Si ya ha salido, la correcta, en una de sus letras permitidas: la de antes, si lo es, o la que menos sale
            // en el resto del examen.
            if (!dependientes && !(permitidas & (1 << pe.correcta()))) {
                int nueva = letra;
                if (!(permitidas & (1 << nueva))) {
                    std::array<int, 4> cuenta{};
                    for (int k = 0; k < m.preguntas.size(); ++k)
                        if (k != i)
                            ++cuenta[m.preguntas.at(k).correcta()];
                    nueva = -1;
                    for (int l = 0; l < 4; ++l)
                        if ((permitidas & (1 << l)) && (nueva < 0 || cuenta[l] < cuenta[nueva]))
                            nueva = l;
                }
                ponerCorrectaEn(pe, nueva, rng);
            }
            // El aviso de si no se puede barajar y va donde ya estuvo, al día.
            const QString avisoAhora = avisoNoBarajable(m, i, config);
            if (avisoAhora != avisoAntes) {
                resultado.avisos.removeAll(avisoAntes);
                if (!avisoAhora.isEmpty())
                    resultado.avisos << avisoAhora;
            }
            enEste = true;
            ++sitios;
        }
        if (!enEste)
            continue;
        const QString delReparto = inicioAvisoExamen(m.nombre);
        resultado.avisos.removeIf([&](const QString &a) { return a.startsWith(delReparto); });
        const QString reparto = avisoReparto(m, config.barajarOpciones || m.barajadas);
        if (!reparto.isEmpty())
            resultado.avisos << reparto;
    }
    return sitios;
}

int Generador::ponerVerificacion(ResultadoGeneracion &resultado, const Pregunta &p)
{
    int sitios = 0;
    for (Modelo &m : resultado.modelos) {
        for (PreguntaExamen &pe : m.preguntas) {
            if (pe.pregunta.id != p.id || pe.pregunta.codigoCompleto() != p.codigoCompleto())
                continue;
            pe.pregunta.verificada = p.verificada;
            pe.pregunta.verificadores = p.verificadores;
            ++sitios;
        }
    }
    return sitios;
}

QMap<int, int> Generador::gruposDisponibles(const QVector<Pregunta> &banco, bool incluirParaRevisar, bool soloVerificadas)
{
    QMap<int, QSet<int>> grupos;
    for (const Pregunta &p : banco)
        if (usable(p, incluirParaRevisar, soloVerificadas))
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
    // Para lo que se añadió después (barajar las opciones de las que ya han salido, sin «Barajar las opciones»), otra
    // serie de números al azar, sacada de la misma semilla: así 'rng', y con ella las preguntas que se eligen con cada
    // semilla, sigue como antes.
    QRandomGenerator rngExtra(res.semilla ^ 0x9e3779b9u);
    // Y otra para las versiones: así, con una semilla, los exámenes sin versiones salen igual tengan o no versiones
    // los demás.
    QRandomGenerator rngVersiones(res.semilla ^ 0x7f4a7c15u);
    Seleccion seleccion(banco, config, rng);
    QSet<int> temasAvisoRepeticion, temasAvisoFaltan;

    for (int k = 0; k < config.nombresModelos.size(); ++k) {
        Modelo modelo;
        modelo.nombre = config.nombresModelos.at(k);
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
                res.avisos << (config.soloVerificadas
                                   ? tr("Tema %1: se pidieron %2 preguntas pero solo hay %3 distintas verificadas disponibles.")
                                   : tr("Tema %1: se pidieron %2 preguntas pero solo hay %3 distintas disponibles."))
                                  .arg(tema).arg(pedidas).arg(elegidas.size());
            }
            if (repetida && config.noRepetirEntreModelos && !temasAvisoRepeticion.contains(tema)) {
                temasAvisoRepeticion.insert(tema);
                res.avisos << tr("Tema %1: no hay preguntas suficientes para no repetir entre exámenes; "
                                 "a partir del examen «%2» se repiten algunas.")
                                  .arg(tema).arg(modelo.nombre);
            }
        }
        ordenar(modelo.preguntas, config.ordenAleatorio, rng);
        if (config.barajarOpciones)
            repartirCorrectas(modelo.preguntas, rng, config, true);
        else
            repartirCorrectas(modelo.preguntas, rngExtra, config, false);   // solo las que ya han salido
        modelo.barajadas = config.barajarOpciones;
        const int n = config.numVersiones(k);
        for (const Modelo &m : n > 1 ? versionesDelModelo(modelo, n, config, rngVersiones) : QVector<Modelo>{modelo}) {
            const QString aviso = avisoReparto(m, m.barajadas);
            if (!aviso.isEmpty())
                res.avisos << aviso;
            for (int i = 0; i < m.preguntas.size(); ++i) {
                const QString noBarajable = avisoNoBarajable(m, i, config);
                if (!noBarajable.isEmpty())
                    res.avisos << noBarajable;
            }
            res.modelos.append(m);
        }
    }
    res.avisos << seleccion.avisosUsoAnterior();
    return res;
}
