// Búsqueda de preguntas parecidas y numeración de las preguntas nuevas.

#include "datos_prueba.h"
#include "parecidas.h"

#include <QtTest>

namespace {

Pregunta copiaDe(const QVector<Pregunta> &banco, const QString &codigo)
{
    for (const Pregunta &p : banco)
        if (p.codigoCompleto() == codigo) {
            Pregunta copia = p;
            copia.id = 0;
            return copia;
        }
    return Pregunta();
}

} // namespace

class PruebaParecidas : public QObject
{
    Q_OBJECT

private slots:
    void encuentraLaQueSeParece()
    {
        const QVector<Pregunta> banco = bancoVariado();
        const Parecidas::Indice indice(banco);
        // La misma pregunta, en otro tema y con otra palabra: «microprocesador» por «procesador».
        Pregunta nueva = copiaDe(banco, QStringLiteral("T1-1"));
        nueva.temaNumero = 3;
        nueva.numero = 9;
        nueva.enunciado = QStringLiteral("La memoria caché de un procesador sirve para:");
        const QVector<Parecidas::Parecida> res = indice.masParecidas(nueva, 3, Parecidas::kMinimoMostrar);
        QVERIFY(!res.isEmpty());
        QCOMPARE(res.first().pregunta.codigoCompleto(), QStringLiteral("T1-1"));
        QVERIFY(res.first().parecido >= Parecidas::kMinimoPreguntar);
        for (int i = 1; i < res.size(); ++i)
            QVERIFY(res.at(i - 1).parecido >= res.at(i).parecido);
    }

    void unaDistintaNoSeParece()
    {
        const Parecidas::Indice indice(bancoVariado());
        Pregunta nueva;
        nueva.temaNumero = 2;
        nueva.numero = 9;
        nueva.enunciado = QStringLiteral("El protocolo que envía el correo electrónico saliente es:");
        nueva.opciones = {QStringLiteral("SMTP."), QStringLiteral("FTP."), QStringLiteral("DHCP."), QStringLiteral("ARP.")};
        QVERIFY(indice.masParecidas(nueva, 3, Parecidas::kMinimoMostrar).isEmpty());
    }

    void noCuentanSusVariantesNiSusEnlaces()
    {
        const QVector<Pregunta> banco = bancoVariado();
        const Parecidas::Indice indice(banco);
        Pregunta variante = copiaDe(banco, QStringLiteral("T1-1"));
        variante.variante = QStringLiteral("b");
        for (const Parecidas::Parecida &p : indice.masParecidas(variante, 10, 0.0))
            QVERIFY(p.pregunta.grupo() != QLatin1String("T1-1"));

        Pregunta enlazada = copiaDe(banco, QStringLiteral("T1-1"));
        enlazada.temaNumero = 3;
        enlazada.numero = 9;
        enlazada.pseudoCon = QStringLiteral("t1-1 ");
        for (const Parecidas::Parecida &p : indice.masParecidas(enlazada, 10, 0.0))
            QVERIFY(p.pregunta.grupo() != QLatin1String("T1-1"));

        // Ni una del banco que ya está enlazada con la nueva.
        QVector<Pregunta> conEnlace = banco;
        conEnlace[0].pseudoCon = QStringLiteral("T3-9");
        for (const Parecidas::Parecida &p : Parecidas::Indice(conEnlace).masParecidas(enlazada, 10, 0.0))
            QVERIFY(p.pregunta.grupo() != QLatin1String("T1-1"));
    }

    void cuantasYMinimo()
    {
        const QVector<Pregunta> banco = bancoVariado();
        const Parecidas::Indice indice(banco);
        Pregunta nueva = copiaDe(banco, QStringLiteral("T2-1"));
        nueva.temaNumero = 4;
        nueva.numero = 9;
        QCOMPARE(indice.masParecidas(nueva, 2, 0.0).size(), 2);
        QCOMPARE(indice.masParecidas(nueva, 20, 0.0).size(), banco.size());   // la original también: es de otro tema
        for (const Parecidas::Parecida &p : indice.masParecidas(nueva, 20, 0.2))
            QVERIFY(p.parecido >= 0.2 && p.parecido <= 1.0 + 1e-9);
    }

    void siguienteNumero()
    {
        QVector<Pregunta> banco = bancoInventado(3, 10);
        QCOMPARE(::siguienteNumero(banco, 2), 11);
        QCOMPARE(::siguienteNumero(banco, 7), 1);   // tema sin preguntas
    }

    void siguienteVariante()
    {
        QVector<Pregunta> banco = bancoInventado(1, 10);   // T1-4 y T1-8 tienen a, b, c
        QCOMPARE(::siguienteVariante(banco, 1, 4), QStringLiteral("d"));
        QCOMPARE(::siguienteVariante(banco, 1, 5), QStringLiteral("b"));   // la que no tiene letra cuenta como «a»
        QVector<Pregunta> llena;
        for (char l = 'a'; l <= 'z'; ++l) {
            Pregunta p;
            p.temaId = 1;
            p.numero = 5;
            p.variante = QString(QLatin1Char(l));
            llena << p;
        }
        QVERIFY(::siguienteVariante(llena, 1, 5).isEmpty());
        llena.removeAt(10);
        QCOMPARE(::siguienteVariante(llena, 1, 5), QStringLiteral("k"));
    }

    void nombreTema()
    {
        const QVector<Tema> temas = temasInventados(3);
        QCOMPARE(::nombreTema(temas, 2), QStringLiteral("Tema 2: Tema inventado 2"));
        QVERIFY(::nombreTema(temas, 9).isEmpty());
    }
};

QTEST_MAIN(PruebaParecidas)
#include "prueba_parecidas.moc"
