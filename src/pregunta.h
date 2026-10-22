#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

// Un tema (capítulo) del libro.
struct Tema
{
    int id = 0;
    int numero = 0;
    QString titulo;
    QString autor;

    QString nombre() const { return QStringLiteral("Tema %1: %2").arg(numero).arg(titulo); }
};

// Una pregunta tipo test de la base de datos.
// Las preguntas con el mismo tema y número (3a, 3b...) son variantes "pseudorrepetidas":
// nunca deben aparecer dos de ellas en el mismo examen.
struct Pregunta
{
    int id = 0;
    int temaId = 0;
    int temaNumero = 0;
    int numero = 0;
    QString variante;               // "", "a", "b", ...
    bool old = false;               // ya ha salido en convocatorias anteriores
    QString estado = QStringLiteral("activa");   // activa, anulada, obsoleta
    bool revisar = false;           // marcada para mejorar/refinar
    QString enunciado;              // texto; admite bloques ``` y filas de tabla | a | b |
    QStringList opciones = QStringList(4, QString());
    int correcta = 0;               // 0..3 -> a..d
    QString referencia;             // p. ej. "Véase la página 47 del libro de teoría."
    QString explicacion;
    QString notas;
    QString pseudoCon;              // otra pregunta equivalente de otro tema, p. ej. "T1-7"
    QByteArray imagen;              // PNG opcional (diagramas)

    QString codigo() const { return QString::number(numero) + variante; }
    QString codigoCompleto() const { return QStringLiteral("T%1-%2").arg(temaNumero).arg(codigo()); }
    QString grupo() const { return QStringLiteral("T%1-%2").arg(temaNumero).arg(numero); }
    bool activa() const { return estado == QLatin1String("activa"); }
    static QChar letra(int i) { return QChar('a' + i); }
};
