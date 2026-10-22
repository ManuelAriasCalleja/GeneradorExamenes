#include "parecidas.h"

#include <QRegularExpression>
#include <QSet>
#include <QtMath>
#include <algorithm>

namespace Parecidas {

namespace {

// Palabras que no dicen nada del contenido de una pregunta (sin tildes, como quedan al normalizar).
const QSet<QString> &palabrasVacias()
{
    static const QSet<QString> vacias = [] {
        QSet<QString> s;
        for (const QString &p : QStringLiteral(
                 "a al algo algun alguna algunas alguno algunos ante antes como con contra cual cuales cuando de del desde "
                 "donde dos el ella ellas ellos en entre era es esa esas ese eso esos esta estas este esto estos fue ha han "
                 "hay la las le les lo los mas me mi muy no nos o otra otras otro otros para pero poco por porque que se sea "
                 "segun ser si sin sobre son su sus tambien tan tiene tienen todo todos tras un una unas uno unos y ya "
                 "siguiente siguientes indique afirmacion cierta falso verdadero puede pueden suele hace mediante cuya cuyo "
                 "dicho dicha").split(QLatin1Char(' ')))
            s.insert(p);
        return s;
    }();
    return vacias;
}

// Trozos de cuatro letras de las palabras de un texto, sin tildes ni mayúsculas: «_bus», «bus_»...
QStringList trozos(const QString &texto)
{
    QString t = texto.toLower().normalized(QString::NormalizationForm_D);
    static const QRegularExpression marcas(QStringLiteral("\\p{Mn}"));
    t.remove(marcas);
    static const QRegularExpression palabra(QStringLiteral("[a-z0-9]+"));
    QStringList res;
    for (auto it = palabra.globalMatch(t); it.hasNext();) {
        const QString w = it.next().captured();
        if (palabrasVacias().contains(w))
            continue;
        const QString marcada = QLatin1Char('_') + w + QLatin1Char('_');
        if (marcada.size() < 4)
            res << marcada;
        for (int i = 0; i + 4 <= marcada.size(); ++i)
            res << marcada.mid(i, 4);
    }
    return res;
}

QHash<QString, double> cuentas(const Pregunta &p)
{
    QHash<QString, double> c;
    for (const QString &t : trozos(p.enunciado))
        c[t] += 2;   // el enunciado cuenta doble
    for (const QString &t : trozos(p.opciones.join(QLatin1Char(' '))))
        c[t] += 1;
    return c;
}

double coseno(const QHash<QString, double> &a, const QHash<QString, double> &b)
{
    const QHash<QString, double> &menor = a.size() < b.size() ? a : b;
    const QHash<QString, double> &mayor = a.size() < b.size() ? b : a;
    double suma = 0;
    for (auto it = menor.cbegin(); it != menor.cend(); ++it)
        suma += it.value() * mayor.value(it.key());
    return suma;
}

} // namespace

Indice::Indice(const QVector<Pregunta> &banco)
    : m_banco(banco)
{
    QVector<QHash<QString, double>> todas;
    QHash<QString, int> enCuantas;
    for (const Pregunta &p : banco) {
        todas << cuentas(p);
        for (auto it = todas.last().cbegin(); it != todas.last().cend(); ++it)
            ++enCuantas[it.key()];
    }
    const double n = banco.size();
    for (auto it = enCuantas.cbegin(); it != enCuantas.cend(); ++it)
        m_idf.insert(it.key(), qLn((n + 1) / (it.value() + 1)) + 1);
    for (const Pregunta &p : banco)
        m_vectores << vector(p);
}

Indice::Vector Indice::vector(const Pregunta &p) const
{
    // Los trozos que no aparecen en el banco pesan como los más raros.
    const double idfNuevo = qLn(m_banco.size() + 1.0) + 1;
    Vector v = cuentas(p);
    double norma = 0;
    for (auto it = v.begin(); it != v.end(); ++it) {
        it.value() *= m_idf.value(it.key(), idfNuevo);
        norma += it.value() * it.value();
    }
    norma = qSqrt(norma);
    if (norma > 0)
        for (auto it = v.begin(); it != v.end(); ++it)
            it.value() /= norma;
    return v;
}

QVector<Parecida> Indice::masParecidas(const Pregunta &p, int cuantas, double minimo) const
{
    const Vector v = vector(p);
    const QString grupo = p.grupo();
    const QString enlazada = p.pseudoCon.trimmed().toUpper();
    QVector<Parecida> res;
    for (int i = 0; i < m_banco.size(); ++i) {
        const Pregunta &q = m_banco.at(i);
        if (q.id == p.id || q.grupo() == grupo || q.grupo() == enlazada || q.pseudoCon.trimmed().toUpper() == grupo)
            continue;
        const double parecido = coseno(v, m_vectores.at(i));
        if (parecido >= minimo)
            res.append({q, parecido});
    }
    std::sort(res.begin(), res.end(), [](const Parecida &a, const Parecida &b) { return a.parecido > b.parecido; });
    if (res.size() > cuantas)
        res.resize(cuantas);
    return res;
}

QString siguienteVariante(const QVector<Pregunta> &banco, int temaId, int numero)
{
    QSet<QString> usadas;
    for (const Pregunta &q : banco)
        if (q.temaId == temaId && q.numero == numero)
            usadas.insert(q.variante.isEmpty() ? QStringLiteral("a") : q.variante);
    for (char letra = 'a'; letra <= 'z'; ++letra)
        if (!usadas.contains(QString(QLatin1Char(letra))))
            return QString(QLatin1Char(letra));
    return QString();
}

} // namespace Parecidas
