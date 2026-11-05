// Comprobador de los JSON de preguntas: errores de escritura con su línea y columna, campos que faltan o están mal,
// avisos, el informe, los ejemplos y el esquema JSON (que tiene que decir lo mismo que el comprobador).

#include "basedatos.h"
#include "comprobadorjson.h"
#include "datos_prueba.h"

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

using ComprobadorJson::Problema;
using ComprobadorJson::Resultado;

namespace {

Resultado comprobar(const QByteArray &json, const QSet<QString> &grupos = {})
{
    return ComprobadorJson::comprobar(json, grupos);
}

// El primer problema cuyo texto contiene 'trozo' (nullptr si no hay).
const Problema *buscar(const Resultado &r, const QString &trozo)
{
    for (const Problema &p : r.problemas)
        if (p.texto.contains(trozo))
            return &p;
    return nullptr;
}

QString describir(const Resultado &r)
{
    return ComprobadorJson::informe(r, QStringLiteral("prueba.json"));
}

// Una pregunta inventada correcta, como texto JSON, con 'extra' añadido a sus campos.
QByteArray pregunta(int numero, const QByteArray &extra = QByteArray(), const QByteArray &variante = "")
{
    return "{\"numero\": " + QByteArray::number(numero) + ", \"variante\": \"" + variante
           + "\", \"enunciado\": \"Enunciado inventado:\", \"opciones\": [\"Uno.\", \"Dos.\", \"Tres.\", \"Cuatro.\"], "
             "\"correcta\": \"B\""
           + (extra.isEmpty() ? QByteArray() : ", " + extra) + "}";
}

QByteArray banco(const QByteArray &preguntas, const QByteArray &extraTema = QByteArray())
{
    return "{\n\"asignatura\": \"Inventada\",\n\"temas\": [\n{\"numero\": 1, \"titulo\": \"Uno\"" + extraTema
           + ", \"preguntas\": [\n" + preguntas + "\n]}\n]\n}\n";
}

QString pngEnBase64()
{
    QImage img(4, 4, QImage::Format_RGB32);
    img.fill(Qt::red);
    QByteArray datos;
    QBuffer b(&datos);
    b.open(QIODevice::WriteOnly);
    img.save(&b, "PNG");
    return QString::fromLatin1(datos.toBase64());
}

QStringList propiedades(const QJsonObject &o)
{
    QStringList res = o.value(QStringLiteral("properties")).toObject().keys();
    res.sort();
    return res;
}

QStringList ordenadas(QStringList l)
{
    l.sort();
    return l;
}

} // namespace

class PruebaComprobador : public QObject
{
    Q_OBJECT

private slots:
    void jsonCorrecto()
    {
        const Resultado r = comprobar(banco(pregunta(1) + ",\n" + pregunta(2, "\"referencia\": null, \"pseudo_con\": null")));
        QVERIFY2(r.problemas.isEmpty(), qPrintable(describir(r)));
        QVERIFY(r.leido && r.valido());
        QCOMPARE(r.temas, 1);
        QCOMPARE(r.preguntas, 2);
        QCOMPARE(ComprobadorJson::resumen(r), QStringLiteral("Sin errores ni avisos: 2 preguntas en 1 tema."));
    }

    // Los errores de escritura dicen dónde están, línea y columna, y qué hacer.
    void erroresDeEscritura_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<int>("linea");
        QTest::addColumn<int>("columna");
        QTest::addColumn<QString>("trozo");
        QTest::newRow("vacío") << QByteArray("  \n") << 2 << 1 << "vacío";
        QTest::newRow("coma de más en un objeto") << QByteArray("{\n  \"temas\": [],\n}") << 3 << 1 << "Sobra la coma antes de «}»";
        QTest::newRow("coma de más en una lista") << QByteArray("{\"temas\": [1, 2,\n ]}") << 2 << 2 << "Sobra la coma antes de «]»";
        QTest::newRow("falta una coma entre campos") << QByteArray("{\"a\": 1\n \"b\": 2}") << 2 << 2 << "Falta una coma antes de este campo";
        QTest::newRow("falta una coma en una lista") << QByteArray("{\"temas\": [1 2]}") << 1 << 14 << "Falta una coma entre dos elementos";
        QTest::newRow("comillas simples") << QByteArray("{'temas': []}") << 1 << 2 << "comillas dobles";
        QTest::newRow("comillas tipográficas") << QByteArray("{\"temas\": [{\"titulo\": \xe2\x80\x9cUno\xe2\x80\x9d}]}") << 1 << 23 << "tipográficas";
        QTest::newRow("nombre sin comillas") << QByteArray("{temas: []}") << 1 << 2 << "\"temas\"";
        QTest::newRow("falta los dos puntos") << QByteArray("{\"temas\" []}") << 1 << 10 << "Falta «:»";
        QTest::newRow("texto sin cerrar") << QByteArray("{\"temas\": [{\"titulo\": \"Uno}]}") << 1 << 23 << "comilla que cierra";
        QTest::newRow("salto de línea en un texto") << QByteArray("{\"a\": \"uno\ndos\"}") << 1 << 11 << "\\n";
        QTest::newRow("escape no válido") << QByteArray("{\"a\": \"C:\\carpeta\"}") << 1 << 10 << "barra invertida";
        QTest::newRow("comentario") << QByteArray("{\n// nota\n\"temas\": []}") << 2 << 1 << "comentarios";
        QTest::newRow("palabra suelta") << QByteArray("{\"temas\": hola}") << 1 << 11 << "«hola» no es un valor válido";
        QTest::newRow("True en mayúsculas") << QByteArray("{\"old\": True}") << 1 << 9 << "minúsculas";
        QTest::newRow("número mal escrito") << QByteArray("{\"numero\": 01}") << 1 << 12 << "Número mal escrito";
        QTest::newRow("objeto sin cerrar") << QByteArray("{\n\"temas\": [\n{\"numero\": 1\n") << 4 << 1 << "línea 3 no se cierra";
        QTest::newRow("cierre cruzado") << QByteArray("{\"temas\": [1}") << 1 << 13 << "se abrió con «[»";
        QTest::newRow("sobra texto") << QByteArray("{\"temas\": []}\n}") << 2 << 1 << "Sobra texto";
    }
    void erroresDeEscritura()
    {
        QFETCH(QByteArray, json);
        QFETCH(int, linea);
        QFETCH(int, columna);
        QFETCH(QString, trozo);
        const Resultado r = comprobar(json);
        QVERIFY(!r.leido);
        QCOMPARE(r.problemas.size(), 1);
        const Problema &p = r.problemas.first();
        QVERIFY2(p.texto.contains(trozo), qPrintable(p.texto));
        QCOMPARE(p.gravedad, Problema::Error);
        QCOMPARE(p.linea, linea);
        QCOMPARE(p.columna, columna);
    }

    void codificacion()
    {
        const Resultado latin1 = comprobar(QByteArray("{\"asignatura\": \"Matem\xe1ticas\", \"temas\": []}"));
        QVERIFY(buscar(latin1, QStringLiteral("UTF-8")));
        // Con BOM (como lo guarda el Bloc de notas), vale.
        const Resultado bom = comprobar(QByteArray("\xef\xbb\xbf") + banco(pregunta(1)));
        QVERIFY2(bom.problemas.isEmpty(), qPrintable(describir(bom)));
    }

    // Lo que falta o tiene un valor que no vale: error, en la pregunta y la línea que toca.
    void erroresDeContenido_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<QString>("trozo");
        QTest::addColumn<QString>("donde");
        QTest::addColumn<int>("linea");
        const QByteArray sinOpciones = "{\"numero\": 3, \"enunciado\": \"Inventado:\", \"correcta\": \"A\"}";
        QTest::newRow("no es un objeto") << QByteArray("[1, 2]") << "tiene que ser un objeto" << QString() << 1;
        QTest::newRow("sin temas") << QByteArray("{\"asignatura\": \"X\"}") << "Falta el campo «temas»" << QString() << 1;
        QTest::newRow("temas no es lista") << QByteArray("{\"temas\": {}}") << "«temas» tiene que ser una lista" << QString() << 1;
        QTest::newRow("tema sin número") << QByteArray("{\"temas\": [{\"titulo\": \"X\", \"preguntas\": []}]}")
                                         << "Falta el campo «numero»" << "tema n.º 1 de la lista" << 1;
        QTest::newRow("número de tema en texto") << QByteArray("{\"temas\": [{\"numero\": \"1\", \"titulo\": \"X\", \"preguntas\": []}]}")
                                                 << "número entero mayor que 0" << "tema n.º 1 de la lista" << 1;
        QTest::newRow("tema repetido") << QByteArray("{\"temas\": [\n{\"numero\": 1, \"titulo\": \"X\", \"preguntas\": []},\n"
                                                     "{\"numero\": 1, \"titulo\": \"Y\", \"preguntas\": []}]}")
                                       << "El tema 1 está repetido (el otro, en la línea 2)" << "tema 1" << 3;
        QTest::newRow("tema sin preguntas") << QByteArray("{\"temas\": [{\"numero\": 2, \"titulo\": \"X\"}]}")
                                            << "Falta el campo «preguntas»" << "tema 2" << 1;
        QTest::newRow("formato de referencia") << banco(pregunta(1), ", \"formato_referencia\": \"pagina\"")
                                               << "«formato_referencia» tiene que ser" << "tema 1" << 4;
        QTest::newRow("sin opciones") << banco(sinOpciones) << "Falta el campo «opciones»" << "tema 1, pregunta 3" << 5;
        QTest::newRow("tres opciones") << banco("{\"numero\": 3, \"enunciado\": \"Inventado:\", \"correcta\": \"A\",\n"
                                                "\"opciones\": [\"Uno.\", \"Dos.\", \"Tres.\"]}")
                                       << "Tiene 3 opciones, y tienen que ser cuatro" << "tema 1, pregunta 3" << 6;
        QTest::newRow("opción vacía") << banco(pregunta(1).replace("\"Tres.\"", "\" \"")) << "La opción c) está vacía"
                                      << "tema 1, pregunta 1" << 5;
        QTest::newRow("opción no es texto") << banco(pregunta(1).replace("\"Tres.\"", "3")) << "La opción c) tiene que ser un texto"
                                            << "tema 1, pregunta 1" << 5;
        QTest::newRow("correcta E") << banco(pregunta(1).replace("\"B\"", "\"E\"")) << "«correcta» tiene que ser la letra"
                                    << "tema 1, pregunta 1" << 5;
        QTest::newRow("correcta número") << banco(pregunta(1).replace("\"B\"", "2")) << "y es el número 2" << "tema 1, pregunta 1" << 5;
        QTest::newRow("sin correcta") << banco(pregunta(1).replace(", \"correcta\": \"B\"", "")) << "Falta el campo «correcta»"
                                      << "tema 1, pregunta 1" << 5;
        QTest::newRow("enunciado vacío") << banco(pregunta(1).replace("Enunciado inventado:", "  ")) << "El enunciado está vacío"
                                         << "tema 1, pregunta 1" << 5;
        QTest::newRow("estado") << banco(pregunta(1, "\"estado\": \"borrada\"")) << "«estado» tiene que ser" << "tema 1, pregunta 1" << 5;
        QTest::newRow("old en texto") << banco(pregunta(1, "\"old\": \"sí\"")) << "«old» tiene que ser true o false"
                                      << "tema 1, pregunta 1" << 5;
        QTest::newRow("variante") << banco(pregunta(1, QByteArray(), "B")) << "«variante» tiene que ser una letra minúscula"
                                  << "tema 1, pregunta n.º 1 de la lista" << 5;
        QTest::newRow("pregunta repetida") << banco(pregunta(4, QByteArray(), "a") + ",\n" + pregunta(4, QByteArray(), "a"))
                                           << "La pregunta 4a está repetida en el tema (la otra, en la línea 5)"
                                           << "tema 1, pregunta 4a" << 6;
        QTest::newRow("pseudo_con mal escrito") << banco(pregunta(1, "\"pseudo_con\": \"T1-7a\"")) << "«pseudo_con» tiene que ser"
                                                << "tema 1, pregunta 1" << 5;
        QTest::newRow("imagen que no es imagen") << banco(pregunta(1, "\"imagen\": \"bm8gZXMgdW5hIGltYWdlbg==\""))
                                                 << "«imagen» no se puede leer" << "tema 1, pregunta 1" << 5;
        QTest::newRow("imagen que no es base64") << banco(pregunta(1, "\"imagen\": \"imagenes/t1.png\""))
                                                 << "«imagen» no se puede leer" << "tema 1, pregunta 1" << 5;
        QTest::newRow("campo repetido") << QByteArray("{\"temas\": [],\n \"temas\": []}") << "está dos veces en el mismo objeto (la otra, en la línea 1)"
                                        << QString() << 2;
    }
    void erroresDeContenido()
    {
        QFETCH(QByteArray, json);
        QFETCH(QString, trozo);
        QFETCH(QString, donde);
        QFETCH(int, linea);
        const Resultado r = comprobar(json);
        QVERIFY(r.leido);
        QVERIFY(!r.valido());
        const Problema *p = buscar(r, trozo);
        QVERIFY2(p, qPrintable(describir(r)));
        QCOMPARE(p->gravedad, Problema::Error);
        QCOMPARE(p->donde, donde);
        QCOMPARE(p->linea, linea);
    }

    // Lo que no impide importar: avisos.
    void avisos()
    {
        const Resultado r = comprobar(banco(pregunta(1, "\"enunciaado\": \"x\", \"pseudo_con\": \"T5-2\", \"codigo\": \"7\"") + ",\n"
                                            + pregunta(2).replace("\"Cuatro.\"", "\"uno.\""),
                                            ", \"extra\": 1"));
        QVERIFY2(r.valido(), qPrintable(describir(r)));
        const Problema *errata = buscar(r, QStringLiteral("«enunciaado»"));
        QVERIFY(errata);
        QCOMPARE(errata->gravedad, Problema::Aviso);
        QVERIFY2(errata->texto.contains(QStringLiteral("¿Quería decir «enunciado»?")), qPrintable(errata->texto));
        QVERIFY(buscar(r, QStringLiteral("El campo «extra» no es de este formato")));
        QVERIFY(buscar(r, QStringLiteral("pseudorrepetida con T5-2, que no está en este JSON")));
        QVERIFY(buscar(r, QStringLiteral("«codigo» dice «7», pero el número y la variante dan 1")));
        QVERIFY(buscar(r, QStringLiteral("Las opciones a) y d) son iguales")));
        QCOMPARE(r.avisos(), 5);

        // Si la pseudorrepetida está en el banco en el que se importa, no hay aviso; tampoco si está en el JSON.
        const QByteArray conPseudo = banco(pregunta(1, "\"pseudo_con\": \"T5-2\"") + ",\n" + pregunta(2, "\"pseudo_con\": \"T1-1\""));
        QCOMPARE(comprobar(conPseudo, {QStringLiteral("T5-2")}).problemas.size(), 0);
        QCOMPARE(comprobar(conPseudo).avisos(), 1);

        // Un tema sin título (si ya está en el banco, conserva el suyo), sin preguntas o una lista vacía de temas.
        QVERIFY(buscar(comprobar("{\"temas\": [{\"numero\": 1, \"preguntas\": [" + pregunta(1) + "]}]}"), QStringLiteral("no tiene «titulo»")));
        QVERIFY(buscar(comprobar("{\"temas\": [{\"numero\": 1, \"titulo\": \"X\", \"preguntas\": []}]}"), QStringLiteral("no tiene preguntas")));
        QVERIFY(buscar(comprobar("{\"temas\": []}"), QStringLiteral("La lista de temas está vacía")));
        // Un idioma que no es de los del programa.
        const Resultado idioma = comprobar("{\"idioma\": \"fr\", \"temas\": []}");
        QVERIFY(idioma.valido());
        QVERIFY(buscar(idioma, QStringLiteral("El idioma «fr» no es ninguno de los del programa")));
    }

    void imagenValida()
    {
        const Resultado r = comprobar(banco(pregunta(1, "\"imagen\": \"" + pngEnBase64().toLatin1() + "\"")));
        QVERIFY2(r.problemas.isEmpty(), qPrintable(describir(r)));
    }

    // Todos los problemas a la vez, en el orden del fichero, y el informe como el de un compilador.
    void informe()
    {
        const Resultado r = comprobar(banco(pregunta(1).replace("\"B\"", "\"E\"") + ",\n" + pregunta(2, "\"estado\": \"x\", \"rara\": 1")));
        QCOMPARE(r.errores(), 2);
        QCOMPARE(r.avisos(), 1);
        for (int i = 1; i < r.problemas.size(); ++i)
            QVERIFY(r.problemas.at(i - 1).linea <= r.problemas.at(i).linea);
        const QStringList lineas = ComprobadorJson::informe(r, QStringLiteral("p.json")).trimmed().split(QLatin1Char('\n'));
        QCOMPARE(lineas.size(), 4);
        QVERIFY2(lineas.at(0).startsWith(QStringLiteral("p.json:5:")), qPrintable(lineas.at(0)));
        QVERIFY2(lineas.at(0).contains(QStringLiteral(": error: tema 1, pregunta 1: «correcta»")), qPrintable(lineas.at(0)));
        QVERIFY2(lineas.at(2).contains(QStringLiteral(": aviso: tema 1, pregunta 2: El campo «rara»")), qPrintable(lineas.at(2)));
        QCOMPARE(lineas.last(), QStringLiteral("2 errores y 1 aviso (2 preguntas en 1 tema)."));
    }

    // Los ejemplos (Archivo → Guardar un JSON de ejemplo…) están bien y se importan.
    void ejemplos_data()
    {
        QTest::addColumn<QString>("ruta");
        QTest::addColumn<QString>("idioma");
        QTest::newRow("castellano") << QStringLiteral(":/recursos/ejemplo_banco.json") << QStringLiteral("es");
        QTest::newRow("inglés") << QStringLiteral(":/recursos/ejemplo_banco_en.json") << QStringLiteral("en");
    }
    void ejemplos()
    {
        QFETCH(QString, ruta);
        QFETCH(QString, idioma);
        const Resultado r = ComprobadorJson::comprobarFichero(ruta);
        QVERIFY2(r.problemas.isEmpty(), qPrintable(describir(r)));
        QCOMPARE(r.temas, 2);
        QCOMPARE(r.preguntas, 8);
        QTemporaryDir dir;
        const QString json = dir.filePath(QStringLiteral("ejemplo.json"));
        QVERIFY(QFile::copy(ruta, json));
        BaseDatos bd;
        QVERIFY(bd.abrir(dir.filePath(QStringLiteral("banco.db"))));
        QCOMPARE(bd.importarJson(json, false), 8);
        QCOMPARE(bd.idioma(), idioma);
        QCOMPARE(bd.temas().size(), 2);
        // Y lo que se exporta de un banco también está bien (lleva el esquema y el idioma).
        const QString exportado = dir.filePath(QStringLiteral("exportado.json"));
        QVERIFY(bd.exportarJson(exportado));
        const Resultado e = ComprobadorJson::comprobarFichero(exportado);
        QVERIFY2(e.problemas.isEmpty(), qPrintable(describir(e)));
        QFile f(exportado);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QJsonObject raiz = QJsonDocument::fromJson(f.readAll()).object();
        QCOMPARE(raiz.value(QStringLiteral("$schema")).toString(), QLatin1String(ComprobadorJson::kEsquema));
        QCOMPARE(raiz.value(QStringLiteral("idioma")).toString(), idioma);
    }

    // El esquema JSON (para los editores) dice los mismos campos y los mismos obligatorios que el comprobador.
    void esquemaComoElComprobador()
    {
        QFile f(QStringLiteral(":/recursos/banco.schema.json"));
        QVERIFY(f.open(QIODevice::ReadOnly));
        QJsonParseError error;
        const QJsonObject esquema = QJsonDocument::fromJson(f.readAll(), &error).object();
        QVERIFY2(error.error == QJsonParseError::NoError, qPrintable(error.errorString()));
        QCOMPARE(esquema.value(QStringLiteral("$id")).toString(), QLatin1String(ComprobadorJson::kEsquema));
        const QJsonObject defs = esquema.value(QStringLiteral("$defs")).toObject();
        const QJsonObject tema = defs.value(QStringLiteral("tema")).toObject();
        const QJsonObject preg = defs.value(QStringLiteral("pregunta")).toObject();
        QCOMPARE(propiedades(esquema), ordenadas(ComprobadorJson::camposRaiz()));
        QCOMPARE(propiedades(tema), ordenadas(ComprobadorJson::camposTema()));
        QCOMPARE(propiedades(preg), ordenadas(ComprobadorJson::camposPregunta()));
        auto obligatorios = [](const QJsonObject &o) {
            QStringList res;
            for (const QJsonValue &v : o.value(QStringLiteral("required")).toArray())
                res << v.toString();
            res.sort();
            return res;
        };
        QCOMPARE(obligatorios(esquema), QStringList{QStringLiteral("temas")});
        QCOMPARE(obligatorios(tema), (QStringList{QStringLiteral("numero"), QStringLiteral("preguntas")}));
        QCOMPARE(obligatorios(preg), ordenadas({QStringLiteral("numero"), QStringLiteral("enunciado"), QStringLiteral("opciones"),
                                                QStringLiteral("correcta")}));
    }

    // Un JSON con solo las preguntas nuevas de un tema (sin título ni autor) no le borra nada al tema del banco.
    void importarSinTituloNiAutor()
    {
        QTemporaryDir dir;
        BaseDatos bd;
        QVERIFY(bd.abrir(dir.filePath(QStringLiteral("banco.db"))));
        Tema t;
        t.numero = 1;
        t.titulo = QStringLiteral("Título inventado");
        t.autor = QStringLiteral("Autor inventado");
        t.formatoReferencia = QStringLiteral("pag");
        QVERIFY(bd.guardarTema(t));
        const QString json = dir.filePath(QStringLiteral("nuevas.json"));
        QFile f(json);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("{\"temas\": [{\"numero\": 1, \"preguntas\": [" + pregunta(5) + "]}]}");
        f.close();
        QVERIFY(ComprobadorJson::comprobarFichero(json).valido());
        QCOMPARE(bd.importarJson(json, false), 1);
        const QVector<Tema> temas = bd.temas();
        QCOMPARE(temas.size(), 1);
        QCOMPARE(temas.first().titulo, QStringLiteral("Título inventado"));
        QCOMPARE(temas.first().autor, QStringLiteral("Autor inventado"));
        QCOMPARE(temas.first().formatoReferencia, QStringLiteral("pag"));
    }
};

QTEST_MAIN(PruebaComprobador)
#include "prueba_comprobador.moc"
