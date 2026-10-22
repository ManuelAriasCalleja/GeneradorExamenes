#include "pregunta.h"

#include <QSet>

QString nombreTema(const QVector<Tema> &temas, int temaId)
{
    for (const Tema &t : temas)
        if (t.id == temaId)
            return t.nombre();
    return QString();
}

int siguienteNumero(const QVector<Pregunta> &banco, int temaId)
{
    int maximo = 0;
    for (const Pregunta &q : banco)
        if (q.temaId == temaId)
            maximo = qMax(maximo, q.numero);
    return maximo + 1;
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
