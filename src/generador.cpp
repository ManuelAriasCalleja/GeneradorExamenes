#include "generador.h"

#include <QHash>
#include <QRandomGenerator>
#include <QRegularExpression>
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

QString claveGrupo(int tema, int numero)
{
    return QStringLiteral("T%1-%2").arg(tema).arg(numero);
}

// Las opciones que se refieren a otras ("Todas las anteriores", "a y b"...) no se pueden barajar.
bool opcionesDependientes(const Pregunta &p)
{
    static const QRegularExpression re(
        QStringLiteral("anterior|todas las|ninguna de|ambas|las dos|\\b[a-d]\\)? y [a-d]\\b"),
        QRegularExpression::CaseInsensitiveOption);
    for (const QString &o : p.opciones)
        if (re.match(o).hasMatch())
            return true;
    return false;
}

bool usable(const Pregunta &p, bool incluirParaRevisar)
{
    return p.activa() && (incluirParaRevisar || !p.revisar);
}

// Baraja las opciones de las preguntas de un examen de modo que la correcta quede repartida por igual entre a),
// b), c) y d) y, si se puede, sin caer tres veces seguidas en la misma letra. Las preguntas con opciones que se
// refieren a otras («Todas las anteriores»...) se quedan como están.
void repartirCorrectas(QVector<PreguntaExamen> &preguntas, QRandomGenerator &rng)
{
    QVector<int> barajables;
    std::array<int, 4> cuenta{};
    for (int i = 0; i < preguntas.size(); ++i) {
        if (opcionesDependientes(preguntas.at(i).pregunta))
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

    // En cada pregunta, la correcta en su letra y las otras tres opciones al azar en las demás.
    for (int j = 0; j < barajables.size(); ++j) {
        PreguntaExamen &pe = preguntas[barajables.at(j)];
        std::array<int, 3> otras{};
        for (int o = 0, k = 0; o < 4; ++o)
            if (o != pe.pregunta.correcta)
                otras[k++] = o;
        std::shuffle(otras.begin(), otras.end(), rng);
        for (int pos = 0, k = 0; pos < 4; ++pos)
            pe.orden[pos] = pos == letras.at(j) ? pe.pregunta.correcta : otras[k++];
    }
}

} // namespace

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

    // tema -> número de pregunta -> variantes
    QMap<int, QMap<int, QVector<Pregunta>>> porTema;
    ConjuntosDisjuntos pseudo;
    for (const Pregunta &p : banco) {
        if (!usable(p, config.incluirParaRevisar))
            continue;
        porTema[p.temaNumero][p.numero].append(p);
        if (!p.pseudoCon.trimmed().isEmpty())
            pseudo.unir(p.grupo(), p.pseudoCon.trimmed().toUpper());
    }

    // Uso anterior de cada pregunta: 0 = nunca; 1 = marcada «old» (si se evitan); 2 o más = salió en una
    // convocatoria registrada (cuanto más reciente, más alto). Cada grupo de variantes y pseudorrepetidas toma el
    // de su pregunta más usada.
    int masAntigua = 0;
    for (int a : config.usoReciente)
        masAntigua = qMax(masAntigua, a);
    auto usoAnteriorPregunta = [&](const Pregunta &p) {
        const int a = config.usoReciente.value(p.codigoCompleto());
        if (a > 0)
            return 2 + masAntigua - a;
        return config.evitarOld && p.old ? 1 : 0;
    };
    QHash<QString, int> usoAnterior;
    for (const QMap<int, QVector<Pregunta>> &grupos : std::as_const(porTema))
        for (const QVector<Pregunta> &variantes : grupos)
            for (const Pregunta &p : variantes) {
                const QString raiz = pseudo.raiz(p.grupo());
                usoAnterior[raiz] = qMax(usoAnterior.value(raiz), usoAnteriorPregunta(p));
            }
    QMap<int, int> usadasAntes;        // tema -> mayor uso anterior de las preguntas elegidas

    QHash<QString, int> usosGrupo;     // veces que se ha usado cada grupo en la convocatoria
    QSet<int> idsUsados;               // preguntas concretas ya usadas
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
            const QMap<int, QVector<Pregunta>> grupos = porTema.value(tema);

            QVector<int> numeros = grupos.keys();
            std::shuffle(numeros.begin(), numeros.end(), rng);
            // Primero las que aún no han salido en esta convocatoria (si no se quieren repetir) y, de ellas, las
            // que no se han usado antes o hace más tiempo.
            std::stable_sort(numeros.begin(), numeros.end(), [&](int a, int b) {
                const QString ra = pseudo.raiz(claveGrupo(tema, a)), rb = pseudo.raiz(claveGrupo(tema, b));
                if (config.noRepetirEntreModelos && usosGrupo.value(ra) != usosGrupo.value(rb))
                    return usosGrupo.value(ra) < usosGrupo.value(rb);
                return usoAnterior.value(ra) < usoAnterior.value(rb);
            });

            int elegidas = 0;
            bool repetida = false;
            for (int num : numeros) {
                if (elegidas == pedidas)
                    break;
                const QString raiz = pseudo.raiz(claveGrupo(tema, num));
                if (usadosModelo.contains(raiz))
                    continue;   // pseudorrepetida con otra ya elegida en este modelo
                if (usosGrupo.value(raiz) > 0)
                    repetida = true;
                usadosModelo.insert(raiz);
                usosGrupo[raiz] += 1;
                if (usoAnterior.value(raiz) > 0)
                    usadasAntes[tema] = qMax(usadasAntes.value(tema), usoAnterior.value(raiz));

                // Variante al azar entre las mejores: las que no se han usado en esta convocatoria y, de ellas,
                // las que no han salido antes o hace más tiempo.
                const QVector<Pregunta> variantes = grupos.value(num);
                auto coste = [&](const Pregunta &v) {
                    return std::make_pair(idsUsados.contains(v.id) ? 1 : 0, usoAnteriorPregunta(v));
                };
                std::pair<int, int> mejor = coste(variantes.first());
                for (const Pregunta &v : variantes)
                    mejor = std::min(mejor, coste(v));
                QVector<Pregunta> candidatas;
                for (const Pregunta &v : variantes)
                    if (coste(v) == mejor)
                        candidatas.append(v);

                PreguntaExamen pe;
                pe.pregunta = candidatas.at(rng.bounded(qsizetype(candidatas.size())));
                idsUsados.insert(pe.pregunta.id);
                modelo.preguntas.append(pe);
                ++elegidas;
            }

            if (elegidas < pedidas && !temasAvisoFaltan.contains(tema)) {
                temasAvisoFaltan.insert(tema);
                res.avisos << QStringLiteral("Tema %1: se pidieron %2 preguntas pero solo hay %3 distintas disponibles.")
                                  .arg(tema).arg(pedidas).arg(elegidas);
            }
            if (repetida && config.noRepetirEntreModelos && !temasAvisoRepeticion.contains(tema)) {
                temasAvisoRepeticion.insert(tema);
                res.avisos << QStringLiteral("Tema %1: no hay preguntas suficientes para no repetir entre modelos; "
                                             "a partir del examen «%2» se repiten algunas.")
                                  .arg(tema).arg(modelo.nombre);
            }
        }

        if (config.ordenAleatorio) {
            std::shuffle(modelo.preguntas.begin(), modelo.preguntas.end(), rng);
        } else {
            std::sort(modelo.preguntas.begin(), modelo.preguntas.end(), [](const PreguntaExamen &a, const PreguntaExamen &b) {
                const Pregunta &x = a.pregunta, &y = b.pregunta;
                if (x.temaNumero != y.temaNumero)
                    return x.temaNumero < y.temaNumero;
                if (x.numero != y.numero)
                    return x.numero < y.numero;
                return x.variante < y.variante;
            });
        }
        if (config.barajarOpciones)
            repartirCorrectas(modelo.preguntas, rng);

        // Aviso si la correcta queda muy desigualmente repartida: una letra con dos o más de las que le tocarían.
        const std::array<int, 4> cuenta = modelo.correctasPorLetra();
        const int n = modelo.preguntas.size();
        const int masRepetida = int(std::max_element(cuenta.begin(), cuenta.end()) - cuenta.begin());
        if (n >= 4 && cuenta[masRepetida] > (n + 3) / 4 + 1)
            res.avisos << QStringLiteral("«%1»: la respuesta correcta es la %2) en %3 de %4 preguntas%5.")
                              .arg(modelo.nombre, QString(Pregunta::letra(masRepetida)))
                              .arg(cuenta[masRepetida])
                              .arg(n)
                              .arg(config.barajarOpciones ? QString() : QStringLiteral("; marque «Barajar las opciones» para repartirla"));
        res.modelos.append(modelo);
    }

    for (auto it = usadasAntes.cbegin(); it != usadasAntes.cend(); ++it)
        res.avisos << (it.value() >= 2 ? QStringLiteral("Tema %1: no quedan bastantes preguntas sin usar en las últimas "
                                                         "convocatorias registradas; se repiten algunas (o variantes suyas).")
                                       : QStringLiteral("Tema %1: no quedan bastantes preguntas sin la marca «old»; "
                                                         "se usan algunas (o variantes suyas)."))
                          .arg(it.key());
    return res;
}
