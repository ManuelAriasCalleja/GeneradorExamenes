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
                if (config.barajarOpciones && !opcionesDependientes(pe.pregunta))
                    std::shuffle(pe.orden.begin(), pe.orden.end(), rng);
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
