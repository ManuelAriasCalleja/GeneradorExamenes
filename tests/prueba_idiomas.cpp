// Idiomas: la interfaz en inglés (la traducción va dentro del programa), los textos de los exámenes y de Moodle en
// el idioma del banco, los meses de las convocatorias, las pautas de estilo que dependen del idioma y que la
// traducción esté completa.

#include "datos_prueba.h"
#include "estilo.h"
#include "generador.h"
#include "idioma.h"
#include "maquetador.h"
#include "moodle.h"

#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QXmlStreamReader>
#include <QtTest>

namespace {

Modelo modelo()
{
    ConfigExamen c;
    c.preguntasPorTema = {{1, 2}, {2, 2}};
    c.nombresModelos << QStringLiteral("Modelo inventado");
    c.semilla = 7;
    return Generador::generar(bancoInventado(2, 6), c).modelos.first();
}

DatosConvocatoria convocatoria(const QString &idioma)
{
    DatosConvocatoria d = DatosConvocatoria::proxima(QDate(2026, 10, 27), idioma);
    d.asignatura = QStringLiteral("Asignatura inventada");
    return d;
}

QVector<Estilo::Aviso> avisos(const QString &enunciado, const QStringList &opciones, const QString &referencia, const QString &idioma)
{
    Pregunta p;
    p.temaNumero = 1;
    p.enunciado = enunciado;
    p.opciones = opciones;
    p.referencia = referencia;
    return Estilo::revisar(p, {}, idioma);
}

QStringList textos(const QVector<Estilo::Aviso> &avisos)
{
    QStringList res;
    for (const Estilo::Aviso &a : avisos)
        res << a.texto;
    return res;
}

} // namespace

class PruebaIdiomas : public QObject
{
    Q_OBJECT

private slots:
    void cleanup()
    {
        Idioma::instalar(QStringLiteral("es"));
    }

    void disponibles()
    {
        QVERIFY(Idioma::disponible(QStringLiteral("es")));
        QVERIFY(Idioma::disponible(QStringLiteral("en")));
        QVERIFY(!Idioma::disponible(QStringLiteral("fr")));
        QCOMPARE(Idioma::nombre(QStringLiteral("en")), QStringLiteral("English"));
        QCOMPARE(Idioma::nombre(QStringLiteral("es")), QStringLiteral("Español"));
        QVERIFY(Idioma::disponible(Idioma::delSistema()));
    }

    // La traducción al inglés está dentro del programa y cambia los textos de la interfaz; con el castellano, los de origen.
    void interfazEnIngles()
    {
        Idioma::instalar(QStringLiteral("en"));
        QCOMPARE(QCoreApplication::translate("VentanaPrincipal", "&Archivo"), QStringLiteral("&File"));
        QCOMPARE(QCoreApplication::translate("Tema", "Tema %1: %2"), QStringLiteral("Topic %1: %2"));
        QCOMPARE(QCoreApplication::translate("ComprobadorJson", "aviso"), QStringLiteral("warning"));
        Tema t;
        t.numero = 3;
        t.titulo = QStringLiteral("Inventado");
        QCOMPARE(t.nombre(), QStringLiteral("Topic 3: Inventado"));
        QCOMPARE(QLocale().language(), QLocale::English);
        Idioma::instalar(QStringLiteral("es"));
        QCOMPARE(QCoreApplication::translate("VentanaPrincipal", "&Archivo"), QStringLiteral("&Archivo"));
        QCOMPARE(t.nombre(), QStringLiteral("Tema 3: Inventado"));
        QCOMPARE(QLocale().language(), QLocale::Spanish);
    }

    // Los textos de los exámenes van en el idioma del banco, sin depender del de la interfaz.
    void textosDelExamen()
    {
        QCOMPARE(Idioma::examen(QStringLiteral("en"), QT_TRANSLATE_NOOP("Examen", "Soluciones")), QStringLiteral("Solutions"));
        QCOMPARE(Idioma::examen(QStringLiteral("es"), QT_TRANSLATE_NOOP("Examen", "Soluciones")), QStringLiteral("Soluciones"));
        // Un idioma sin traducción: el original.
        QCOMPARE(Idioma::examen(QStringLiteral("fr"), QT_TRANSLATE_NOOP("Examen", "Soluciones")), QStringLiteral("Soluciones"));
        // Con la interfaz en inglés, un examen en castellano sigue en castellano.
        Idioma::instalar(QStringLiteral("en"));
        QCOMPARE(Idioma::examen(QStringLiteral("es"), QT_TRANSLATE_NOOP("Examen", "Soluciones")), QStringLiteral("Soluciones"));
    }

    void mesesYConvocatorias()
    {
        QCOMPARE(DatosConvocatoria::ordinarias(), (QStringList{QStringLiteral("Febrero"), QStringLiteral("Junio"), QStringLiteral("Septiembre")}));
        QCOMPARE(DatosConvocatoria::ordinarias(QStringLiteral("en")),
                 (QStringList{QStringLiteral("February"), QStringLiteral("June"), QStringLiteral("September")}));
        QCOMPARE(DatosConvocatoria::nombreDeMes(6, QStringLiteral("en")), QStringLiteral("June"));
        QCOMPARE(DatosConvocatoria::mesDe(QStringLiteral("June (reserve)")), 6);
        QCOMPARE(DatosConvocatoria::mesDe(QStringLiteral("Septiembre")), 9);
        QCOMPARE(DatosConvocatoria::mesDe(QStringLiteral("Extraordinaria")), 0);
        const DatosConvocatoria en = convocatoria(QStringLiteral("en"));
        QCOMPARE(en.convocatoria, QStringLiteral("February"));
        QCOMPARE(en.anio, 2027);
        QCOMPARE(en.idioma, QStringLiteral("en"));
        QVERIFY2(en.instrucciones.startsWith(QStringLiteral("Mark the correct answer")), qPrintable(en.instrucciones));
        QCOMPARE(DatosConvocatoria::instruccionesPorDefecto(QStringLiteral("es")), DatosConvocatoria().instrucciones);
        QCOMPARE(Maquetador::formatoPuntos(0.3, QStringLiteral("en")), QStringLiteral("0.30"));
        QCOMPARE(Maquetador::formatoPuntos(0.3), QStringLiteral("0,30"));
    }

    void examenEnIngles()
    {
        const Modelo m = modelo();
        const DatosConvocatoria en = convocatoria(QStringLiteral("en"));
        const QString html = Maquetador::examenHtml(m, en, true);
        for (const QString &texto : {QStringLiteral("WITH SOLUTIONS"), QStringLiteral("Question answered correctly:"),
                                     QStringLiteral("+ 1.00 points"), QStringLiteral("- 0.30 points"), QStringLiteral("February 2027")})
            QVERIFY2(html.contains(texto), qPrintable(texto));
        QVERIFY(!html.contains(QStringLiteral("puntos")));
        const QString soluciones = Maquetador::solucionesHtml({m}, en);
        QVERIFY(soluciones.contains(QStringLiteral("Solutions")));
        QVERIFY(soluciones.contains(QStringLiteral("Session structure")));
        QVERIFY(soluciones.contains(QStringLiteral("Answer key: Modelo inventado")));
        QVERIFY(html.contains(QStringLiteral("Reference:</span>")));
        const QString tex = Maquetador::examenesLatex({m}, en, true, QStringLiteral("imagenes"));
        QVERIFY(tex.contains(QStringLiteral("\\usepackage[english]{babel}")));
        QVERIFY(tex.contains(QStringLiteral("Question not answered:")));
        // El mismo examen en castellano, como siempre.
        const DatosConvocatoria es = convocatoria(QStringLiteral("es"));
        QVERIFY(Maquetador::examenHtml(m, es, true).contains(QStringLiteral("Pregunta contestada correctamente:")));
        QVERIFY(Maquetador::examenesLatex({m}, es, true, QStringLiteral("imagenes")).contains(QStringLiteral("[spanish,es-noshorthands]{babel}")));
    }

    void moodleEnIngles()
    {
        Moodle::Opciones o;
        o.asignatura = QStringLiteral("Inventada");
        o.idioma = QStringLiteral("en");
        const QString xml = Moodle::xml(temasInventados(2), bancoInventado(2, 3), o);
        QVERIFY(xml.contains(QStringLiteral("Correct answer.")));
        QVERIFY(xml.contains(QStringLiteral("$course$/top/Inventada/Topic 1: ")));
        QVERIFY(!xml.contains(QStringLiteral("Respuesta correcta.")));
    }

    // Las pautas del castellano (¿?, dos puntos, punto final en las opciones, formas de la referencia) solo en
    // castellano; las demás, en cualquier idioma.
    void estiloSegunElIdioma()
    {
        const QStringList opciones{QStringLiteral("Mars"), QStringLiteral("Venus"), QStringLiteral("Mars"), QStringLiteral("Earth")};
        const QStringList es = textos(avisos(QStringLiteral("Which planet is red?"), opciones, QStringLiteral("See page 3."), QStringLiteral("es")));
        const QStringList en = textos(avisos(QStringLiteral("Which planet is red?"), opciones, QStringLiteral("See page 3."), QStringLiteral("en")));
        QVERIFY(es.join(QLatin1Char('\n')).contains(QStringLiteral("«¿»")));
        QVERIFY(es.join(QLatin1Char('\n')).contains(QStringLiteral("debe terminar en punto")));
        QVERIFY2(!en.join(QLatin1Char('\n')).contains(QStringLiteral("«¿»")), qPrintable(en.join(QLatin1Char('\n'))));
        QVERIFY2(!en.join(QLatin1Char('\n')).contains(QStringLiteral("terminar en punto")), qPrintable(en.join(QLatin1Char('\n'))));
        QVERIFY2(!en.join(QLatin1Char('\n')).contains(QStringLiteral("forma habitual")), qPrintable(en.join(QLatin1Char('\n'))));
        // Las opciones iguales y la falta de referencia valen en cualquier idioma.
        QVERIFY(en.join(QLatin1Char('\n')).contains(QStringLiteral("son iguales")));
        QVERIFY(textos(avisos(QStringLiteral("Which planet is red?"), opciones, QString(), QStringLiteral("en")))
                    .contains(QStringLiteral("Falta la referencia al libro.")));
    }

    // «Todas las anteriores» y compañía, en castellano y en inglés: con ellas no se barajan las opciones.
    void opcionesQueRemitenALasDemas()
    {
        for (const QString &o : {QStringLiteral("Todas las anteriores."), QStringLiteral("All of the above."), QStringLiteral("None of the above"),
                                 QStringLiteral("Both of the above."), QStringLiteral("a and b."), QStringLiteral("All are correct."),
                                 QStringLiteral("Neither.")})
            QVERIFY2(Estilo::opcionDependiente(o), qPrintable(o));
        for (const QString &o : {QStringLiteral("Mars."), QStringLiteral("All planets orbit the Sun."), QStringLiteral("Above the clouds.")})
            QVERIFY2(!Estilo::opcionDependiente(o), qPrintable(o));
    }

    // La ayuda en inglés está dentro del programa y tiene los mismos apartados (anclas) y enlaces que la castellana,
    // a la que van F1 y el botón «Ayuda» de cada diálogo. Si falla tras cambiar ayuda.html, hay que cambiar igual
    // ayuda_en.html.
    void ayudaEnIngles()
    {
        QCOMPARE(Idioma::ayuda(QStringLiteral("en")), QStringLiteral("qrc:/recursos/ayuda_en.html"));
        QCOMPARE(Idioma::ayuda(QStringLiteral("es")), QStringLiteral("qrc:/recursos/ayuda.html"));
        QCOMPARE(Idioma::ayuda(QStringLiteral("fr")), QStringLiteral("qrc:/recursos/ayuda.html"));
        auto leer = [](const QString &ruta) {
            QFile f(ruta);
            return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
        };
        const QString es = leer(QStringLiteral(":/recursos/ayuda.html"));
        const QString en = leer(QStringLiteral(":/recursos/ayuda_en.html"));
        QVERIFY(!es.isEmpty() && !en.isEmpty());
        auto todas = [](const QString &html, const QString &patron) {
            QStringList res;
            for (const QRegularExpressionMatch &m : QRegularExpression(patron).globalMatch(html))
                res << m.captured(1);
            return res;
        };
        const QStringList anclas = todas(es, QStringLiteral("<a name=\"([^\"]+)\""));
        QVERIFY(anclas.size() > 30);
        QCOMPARE(todas(en, QStringLiteral("<a name=\"([^\"]+)\"")), anclas);
        QCOMPARE(todas(en, QStringLiteral("href=\"#([^\"]+)\"")), todas(es, QStringLiteral("href=\"#([^\"]+)\"")));
        for (const QString &destino : todas(en, QStringLiteral("href=\"#([^\"]+)\"")))
            QVERIFY2(anclas.contains(destino), qPrintable(destino));
        QVERIFY(en.contains(QStringLiteral("JSON format")));
    }

    // La traducción al inglés está completa: ningún texto sin traducir. Si falla tras añadir textos al programa:
    // «cmake --build build --target update_translations» y traducirlos con Qt Linguist (ver el README).
    void traduccionCompleta()
    {
        QFile f(QStringLiteral(RUTA_PROYECTO "/traducciones/GeneradorExamenes_en.ts"));
        QVERIFY(f.open(QIODevice::ReadOnly));
        QXmlStreamReader x(&f);
        QString contexto, origen;
        QStringList sinTraducir;
        int mensajes = 0;
        while (!x.atEnd()) {
            x.readNext();
            if (!x.isStartElement())
                continue;
            if (x.name() == QLatin1String("name"))
                contexto = x.readElementText();
            else if (x.name() == QLatin1String("source"))
                origen = x.readElementText();
            else if (x.name() == QLatin1String("translation")) {
                ++mensajes;
                const bool sinTerminar = x.attributes().value(QLatin1String("type")) == QLatin1String("unfinished");
                if (sinTerminar || x.readElementText().isEmpty())
                    sinTraducir << contexto + QStringLiteral(": ") + origen;
            }
        }
        QVERIFY(!x.hasError());
        QVERIFY(mensajes > 600);
        QVERIFY2(sinTraducir.isEmpty(), qPrintable(sinTraducir.join(QLatin1Char('\n'))));
    }
};

QTEST_MAIN(PruebaIdiomas)
#include "prueba_idiomas.moc"
