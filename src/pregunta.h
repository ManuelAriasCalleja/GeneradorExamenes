#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

// Un tema (capítulo) del libro.
struct Tema
{
    int id = 0;
    int numero = 0;
    QString titulo;
    QString autor;
    // Forma de la referencia al libro en las preguntas del tema, que depende de su autor (la comprueba Estilo):
    // "vease", "pag" o vacía (cualquiera). Ver Estilo::formatosReferencia().
    QString formatoReferencia;

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
    // Grupo de variantes, «T3-5»: es también lo que se escribe en «pseudorrepetida con».
    QString grupo() const { return claveGrupo(temaNumero, numero); }
    static QString claveGrupo(int tema, int numero) { return QStringLiteral("T%1-%2").arg(tema).arg(numero); }
    bool activa() const { return estado == QLatin1String("activa"); }
    static QChar letra(int i) { return QChar('a' + i); }            // de la opción: a, b, c, d
    static QChar letraMayuscula(int i) { return QChar('A' + i); }   // de la respuesta en soluciones y plantillas: A, B, C, D

    // Orden del banco y de los exámenes: tema, número y variante.
    bool antesQue(const Pregunta &otra) const
    {
        if (temaNumero != otra.temaNumero)
            return temaNumero < otra.temaNumero;
        if (numero != otra.numero)
            return numero < otra.numero;
        return variante < otra.variante;
    }
};

// Nombre del tema con ese id («Tema 3: Sistemas operativos»), o vacío si no está.
QString nombreTema(const QVector<Tema> &temas, int temaId);
// Numeración de una pregunta nueva en el banco: el primer número libre de un tema, y la primera letra libre para
// una variante nueva de un grupo (vacía si están usadas de la «a» a la «z»; una pregunta sin letra cuenta como la
// «a», para que la nueva no sea otra «a»).
int siguienteNumero(const QVector<Pregunta> &banco, int temaId);
QString siguienteVariante(const QVector<Pregunta> &banco, int temaId, int numero);
