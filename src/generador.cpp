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
            if (config.noRepetirEntreModelos) {
                // Primero las que aún no han salido en esta convocatoria.
                std::stable_sort(numeros.begin(), numeros.end(), [&](int a, int b) {
                    return usosGrupo.value(pseudo.raiz(claveGrupo(tema, a)))
                         < usosGrupo.value(pseudo.raiz(claveGrupo(tema, b)));
                });
            }

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

                // Variante al azar, prefiriendo las que no se han usado todavía.
                QVector<Pregunta> variantes = grupos.value(num);
                QVector<Pregunta> nuevas;
                for (const Pregunta &v : variantes)
                    if (!idsUsados.contains(v.id))
                        nuevas.append(v);
                if (!nuevas.isEmpty())
                    variantes = nuevas;

                PreguntaExamen pe;
                pe.pregunta = variantes.at(rng.bounded(qsizetype(variantes.size())));
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
    return res;
}
