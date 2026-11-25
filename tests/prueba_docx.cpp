// Exportación a Word (.docx): el ZIP y las partes del documento.

#include "datos_prueba.h"
#include "docx.h"
#include "exportacion.h"
#include "generador.h"
#include "resaltado.h"
#include "zip.h"

#include <QFile>
#include <QTemporaryDir>
#include <QXmlStreamReader>
#include <QtEndian>
#include <QtTest>

namespace {

// Un banco inventado con de todo: código numerado y resaltado, una tabla y caracteres que hay que escapar en el XML
// (tema 1), imágenes (tema 2) y explicaciones.
QVector<Pregunta> bancoCompleto()
{
    QVector<Pregunta> banco = bancoInventado();
    for (Pregunta &p : banco) {
        if (p.temaNumero == 1)
            p.enunciado = QStringLiteral("Si a < b && c > \"d\", ¿qué escribe este código inventado?\n"
                                         "```java numerado\nint x = 1; // comentario\n\tString s = \"hola\";\n\n```\n"
                                         "| Columna | Otra |\n| 1 | 2 |");
        if (p.temaNumero == 2)
            p.imagen = imagenInventada();
        p.explicacion = QStringLiteral("Explicación inventada.");
    }
    return banco;
}

QVector<Modelo> examenes(const QVector<Pregunta> &banco, int modelos = 2)
{
    ConfigExamen c;
    c.preguntasPorTema = {{1, 2}, {2, 2}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1}, {8, 1}};
    for (int i = 0; i < modelos; ++i)
        c.nombresModelos << QStringLiteral("Examen %1").arg(i + 1);
    c.semilla = 5;
    return Generador::generar(banco, c).modelos;
}

DatosConvocatoria convocatoria()
{
    DatosConvocatoria d;
    d.asignatura = QStringLiteral("Asignatura inventada");
    d.convocatoria = QStringLiteral("Junio");
    d.anio = 2026;
    d.semilla = 4321;
    return d;
}

// Las entradas de un ZIP, leyendo sus cabeceras locales una tras otra.
struct Entrada
{
    QByteArray datos;   // como están guardados
    quint16 metodo = 0;
    quint32 crc = 0;
};

QList<QPair<QString, Entrada>> entradas(const QByteArray &zip)
{
    QList<QPair<QString, Entrada>> res;
    qsizetype i = 0;
    while (i + 30 <= zip.size() && qFromLittleEndian<quint32>(zip.constData() + i) == 0x04034b50) {
        const char *c = zip.constData() + i;
        Entrada e;
        e.metodo = qFromLittleEndian<quint16>(c + 8);
        e.crc = qFromLittleEndian<quint32>(c + 14);
        const quint32 guardado = qFromLittleEndian<quint32>(c + 18);
        const quint16 nombre = qFromLittleEndian<quint16>(c + 26);
        const quint16 extra = qFromLittleEndian<quint16>(c + 28);
        e.datos = zip.mid(i + 30 + nombre + extra, guardado);
        res << qMakePair(QString::fromUtf8(zip.mid(i + 30, nombre)), e);
        i += 30 + nombre + extra + guardado;
    }
    return res;
}

// Descomprime los datos de una entrada con deflate, que tienen que dar 'esperado': con la cabecera de zlib delante y
// su suma de comprobación (la de 'esperado') detrás, los descomprime qUncompress.
QByteArray inflar(const QByteArray &deflate, const QByteArray &esperado)
{
    char b[4];
    QByteArray zlib;
    qToBigEndian(quint32(esperado.size()), b);
    zlib.append(b, 4);
    zlib.append("\x78\x9c", 2);
    zlib += deflate;
    quint32 a = 1, s = 0;
    for (const char c : esperado) {
        a = (a + quint8(c)) % 65521;
        s = (s + a) % 65521;
    }
    qToBigEndian(quint32(s << 16 | a), b);
    zlib.append(b, 4);
    return qUncompress(zlib);
}

bool bienFormado(const QByteArray &xml, QString *error)
{
    QXmlStreamReader r(xml);
    while (!r.atEnd())
        r.readNext();
    *error = r.errorString();
    return !r.hasError();
}

} // namespace

class PruebaDocx : public QObject
{
    Q_OBJECT

private slots:
    void crc32()
    {
        QCOMPARE(EscritorZip::crc32(QByteArray("123456789")), 0xCBF43926u);   // el valor de comprobación del CRC-32
        QCOMPARE(EscritorZip::crc32(QByteArray()), 0u);
    }

    void zip()
    {
        const QByteArray texto = QByteArray("Texto inventado que se repite. ").repeated(50);
        const QByteArray binario("\x89PNG\r\n\x1a\n", 8);
        EscritorZip z;
        z.anadir(QStringLiteral("carpeta/texto.xml"), texto);
        z.anadir(QStringLiteral("binario.png"), binario, false);
        const QByteArray zip = z.terminar();
        // Al final, el fin del directorio central, con las dos entradas.
        const qsizetype fin = zip.size() - 22;
        QCOMPARE(qFromLittleEndian<quint32>(zip.constData() + fin), 0x06054b50u);
        QCOMPARE(qFromLittleEndian<quint16>(zip.constData() + fin + 10), quint16(2));
        const auto e = entradas(zip);
        QCOMPARE(e.size(), 2);
        QCOMPARE(e.at(0).first, QStringLiteral("carpeta/texto.xml"));
        QCOMPARE(e.at(0).second.metodo, quint16(8));   // deflate
        QVERIFY(e.at(0).second.datos.size() < texto.size() / 10);
        QCOMPARE(inflar(e.at(0).second.datos, texto), texto);
        QCOMPARE(e.at(0).second.crc, EscritorZip::crc32(texto));
        QCOMPARE(e.at(1).first, QStringLiteral("binario.png"));
        QCOMPARE(e.at(1).second.metodo, quint16(0));   // tal cual
        QCOMPARE(e.at(1).second.datos, binario);
    }

    void documento()
    {
        const QVector<Modelo> modelos = examenes(bancoCompleto());
        const QMap<QString, QByteArray> p = Docx::partes(modelos, convocatoria(), false);
        for (auto it = p.cbegin(); it != p.cend(); ++it) {
            if (it.key().endsWith(QLatin1String(".png")))
                continue;
            QString error;
            QVERIFY2(bienFormado(it.value(), &error), qPrintable(it.key() + QStringLiteral(": ") + error));
        }
        const QString doc = QString::fromUtf8(p.value(QStringLiteral("word/document.xml")));
        // Una sección por examen, cada una con su pie y la numeración desde 1.
        QCOMPARE(doc.count(QStringLiteral("<w:sectPr>")), 2);
        QCOMPARE(doc.count(QStringLiteral("<w:pgNumType w:start=\"1\"/>")), 2);
        QCOMPARE(doc.count(QStringLiteral("<w:footerReference")), 2);
        const QString pie = QString::fromUtf8(p.value(QStringLiteral("word/footer2.xml")));
        QVERIFY(pie.contains(QStringLiteral("Asignatura inventada · Examen 2 · Junio 2026: ")));
        // La página y el total: la página del marcador del final de la sección.
        QVERIFY(pie.contains(QStringLiteral("w:instr=\" PAGE \"")) && pie.contains(QStringLiteral("w:instr=\" PAGEREF _FinSeccion2 \"")));
        QVERIFY(doc.contains(QStringLiteral("<w:bookmarkStart w:id=\"2\" w:name=\"_FinSeccion2\"/>")));
        QVERIFY(p.value(QStringLiteral("[Content_Types].xml")).contains("/word/footer2.xml"));
        // Cabecera, instrucciones con la puntuación en una línea y las preguntas, con su número.
        QVERIFY(doc.contains(QStringLiteral(">Asignatura inventada<")));
        QVERIFY(doc.contains(QStringLiteral("Respuesta correcta: +1,00 puntos · incorrecta: −0,30 puntos · en blanco: 0 puntos")));
        QVERIFY(doc.contains(QStringLiteral(">10.- <")));
        QVERIFY(doc.contains(QStringLiteral("a) Primera opción inventada.")));
        // El código, en Courier New, con los números de línea y el resaltado; la tabla y el texto, escapados.
        QVERIFY(doc.contains(QStringLiteral("Courier New")));
        QVERIFY(doc.contains(QStringLiteral("<w:shd w:val=\"clear\" w:color=\"auto\" w:fill=\"EEEEEE\"/>")));
        QVERIFY(doc.contains(QStringLiteral("<w:color w:val=\"%1\"/>").arg(QLatin1String(Resaltado::kColorClave))));
        QVERIFY(doc.contains(QStringLiteral("    String s = ")));   // el tabulador, cuatro espacios
        QVERIFY(doc.contains(QStringLiteral("a &lt; b &amp;&amp; c &gt; &quot;d&quot;")));
        QVERIFY(doc.contains(QStringLiteral(">Columna<")));
        // Las imágenes, dentro: cada una distinta, una vez.
        QVERIFY(p.contains(QStringLiteral("word/media/imagen1.png")));
        QVERIFY(!p.contains(QStringLiteral("word/media/imagen2.png")));
        QVERIFY(doc.contains(QStringLiteral("r:embed=\"rIdImagen1\"")));
        QVERIFY(p.value(QStringLiteral("word/_rels/document.xml.rels")).contains("Target=\"media/imagen1.png\""));
        // Sin soluciones: ni la semilla ni las respuestas, que van en las propiedades y en la hoja de soluciones.
        QVERIFY(!doc.contains(QStringLiteral("4321")));
        QVERIFY(!doc.contains(QStringLiteral("CON SOLUCIONES")));
        QVERIFY(!doc.contains(QStringLiteral("Explicación inventada")));
        QVERIFY(p.value(QStringLiteral("docProps/core.xml")).contains("<dc:description>Semilla: 4321</dc:description>"));
        QVERIFY(p.value(QStringLiteral("docProps/core.xml")).contains("<dc:title>Asignatura inventada · Junio 2026</dc:title>"));
        QVERIFY(p.value(QStringLiteral("docProps/app.xml")).contains("GeneradorExamenes "));
    }

    void documentoConSoluciones()
    {
        const QVector<Modelo> modelos = examenes(bancoCompleto());
        const QMap<QString, QByteArray> p = Docx::partes(modelos, convocatoria(), true);
        QString error;
        QVERIFY2(bienFormado(p.value(QStringLiteral("word/document.xml")), &error), qPrintable(error));
        const QString doc = QString::fromUtf8(p.value(QStringLiteral("word/document.xml")));
        // La hoja de soluciones, en su sección, y los dos exámenes.
        QCOMPARE(doc.count(QStringLiteral("<w:sectPr>")), 3);
        QVERIFY(QString::fromUtf8(p.value(QStringLiteral("word/footer1.xml"))).contains(QStringLiteral("Asignatura inventada · Soluciones · Junio 2026: ")));
        for (const QString &texto : {QStringLiteral("Semilla: 4321"), QStringLiteral("Estructura de la convocatoria"),
                                     QStringLiteral("Plantilla de corrección: Examen 2"), QStringLiteral("CON SOLUCIONES"),
                                     QStringLiteral("Explicación inventada."), QString(QChar(0x2714))})
            QVERIFY2(doc.contains(texto), qPrintable(texto));
        QCOMPARE(doc.count(QStringLiteral("Semilla: 4321")), 1);   // en la hoja, no en los exámenes
    }

    void cabeceraYPie()
    {
        const QVector<Modelo> modelos = examenes(bancoInventado(), 1);
        DatosConvocatoria d = convocatoria();
        d.asignaturaEnCabecera = false;
        d.convocatoriaEnPie = false;
        QMap<QString, QByteArray> p = Docx::partes(modelos, d, false);
        QString doc = QString::fromUtf8(p.value(QStringLiteral("word/document.xml")));
        QVERIFY(!doc.contains(QStringLiteral("Asignatura inventada")));
        QVERIFY(doc.contains(QStringLiteral(">Junio 2026<")));
        QVERIFY(QString::fromUtf8(p.value(QStringLiteral("word/footer1.xml"))).contains(QStringLiteral(">Asignatura inventada · Examen 1: <")));
        // Sin nada arriba, no hay cabecera: el documento empieza por el recuadro de las instrucciones.
        d.convocatoriaEnCabecera = false;
        p = Docx::partes(modelos, d, false);
        doc = QString::fromUtf8(p.value(QStringLiteral("word/document.xml")));
        QVERIFY(!doc.contains(QStringLiteral("Junio 2026")));
        const QString cuerpo = doc.section(QStringLiteral("<w:body>"), 1);
        QVERIFY(!cuerpo.section(QStringLiteral("<w:tbl>"), 0, 0).contains(QStringLiteral("<w:t ")));
        QVERIFY(cuerpo.section(QStringLiteral("</w:tbl>"), 0, 0).contains(QStringLiteral("Seleccione")));
    }

    // La versión del examen («Versión B»), arriba a la derecha, si se pide.
    void versionEnLaCabecera()
    {
        QVector<Modelo> modelos = examenes(bancoInventado(), 1);
        modelos[0].base = modelos[0].nombre;
        modelos[0].version = QStringLiteral("B");
        DatosConvocatoria d = convocatoria();
        QString doc = QString::fromUtf8(Docx::partes(modelos, d, false).value(QStringLiteral("word/document.xml")));
        QVERIFY(!doc.contains(QStringLiteral("Versión B")));
        d.versionEnCabecera = true;
        doc = QString::fromUtf8(Docx::partes(modelos, d, false).value(QStringLiteral("word/document.xml")));
        QVERIFY(doc.contains(QStringLiteral(">Versión B<")));
    }

    // El código dentro del texto (entre comillas invertidas): con la letra de ancho fijo y sin cursiva, en el enunciado
    // (que sigue en negrita) y en las opciones.
    void codigoEnLinea()
    {
        QVector<Pregunta> banco = bancoInventado(1, 1);
        banco[0].enunciado = QStringLiteral("Texto inventado con `nombre_1` dentro:");
        banco[0].opciones[0] = QStringLiteral("La opción `valor` inventada.");
        const QVector<Modelo> modelos = examenes(banco, 1);
        const QString documento = QString::fromUtf8(Docx::partes(modelos, convocatoria(), false).value(QStringLiteral("word/document.xml")));
        QVERIFY(!documento.contains(QLatin1Char('`')));
        const QRegularExpression enEnunciado(QStringLiteral("<w:rPr><w:rFonts w:ascii=\"Courier New\"[^>]*/><w:b/><w:bCs/></w:rPr><w:t xml:space=\"preserve\">nombre_1</w:t>"));
        QVERIFY2(enEnunciado.match(documento).hasMatch(), qPrintable(documento.mid(documento.indexOf(QStringLiteral("nombre_1")) - 300, 400)));
        const QRegularExpression enOpcion(QStringLiteral("<w:rPr><w:rFonts w:ascii=\"Courier New\"[^>]*/></w:rPr><w:t xml:space=\"preserve\">valor</w:t>"));
        QVERIFY2(enOpcion.match(documento).hasMatch(), qPrintable(documento.mid(documento.indexOf(QStringLiteral(">valor<")) - 300, 400)));
    }

    void letraGrande()
    {
        const QVector<Modelo> modelos = examenes(bancoCompleto(), 1);
        const QByteArray normal = Docx::partes(modelos, convocatoria(), false).value(QStringLiteral("word/styles.xml"));
        const QByteArray grande = Docx::partes(modelos, convocatoria(), false, Maquetador::kLetraGrande).value(QStringLiteral("word/styles.xml"));
        QVERIFY(normal.contains("<w:sz w:val=\"21\"/>"));   // 10,5 puntos
        QVERIFY(grande.contains("<w:sz w:val=\"32\"/>"));   // 16 puntos
        // Con letra grande, los enunciados en negrita sin cursiva.
        const QString enunciado = QStringLiteral("<w:b/><w:bCs/><w:i/><w:iCs/></w:rPr><w:t xml:space=\"preserve\">1.- ");
        QVERIFY(QString::fromUtf8(Docx::partes(modelos, convocatoria(), false).value(QStringLiteral("word/document.xml"))).contains(enunciado));
        QVERIFY(!QString::fromUtf8(Docx::partes(modelos, convocatoria(), false, Maquetador::kLetraGrande)
                                       .value(QStringLiteral("word/document.xml")))
                     .contains(QStringLiteral("<w:i/><w:iCs/></w:rPr><w:t xml:space=\"preserve\">1.- ")));
        // Y con la letra normal, si se pide (la casilla «Enunciados en cursiva», desmarcada).
        DatosConvocatoria sinCursiva = convocatoria();
        sinCursiva.enunciadoEnCursiva = false;
        const QString documento = QString::fromUtf8(Docx::partes(modelos, sinCursiva, false).value(QStringLiteral("word/document.xml")));
        QVERIFY(!documento.contains(enunciado));
        QVERIFY(documento.contains(QStringLiteral("<w:b/><w:bCs/></w:rPr><w:t xml:space=\"preserve\">1.- ")));
    }

    void fichero()
    {
        const QVector<Modelo> modelos = examenes(bancoCompleto());
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("examen.docx"));
        QVERIFY(Exportacion::docx(ruta, modelos, convocatoria(), true, 0));
        QFile f(ruta);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const auto e = entradas(f.readAll());
        // Las partes, con [Content_Types].xml la primera, y las imágenes sin comprimir.
        const QMap<QString, QByteArray> p = Docx::partes(modelos, convocatoria(), true);
        QCOMPARE(e.size(), p.size());
        QCOMPARE(e.first().first, QStringLiteral("[Content_Types].xml"));
        for (const auto &entrada : e) {
            QVERIFY2(p.contains(entrada.first), qPrintable(entrada.first));
            if (entrada.first.endsWith(QLatin1String(".png")))
                QCOMPARE(entrada.second.datos, p.value(entrada.first));
            else if (entrada.first != QLatin1String("docProps/core.xml"))   // lleva la hora
                QCOMPARE(inflar(entrada.second.datos, p.value(entrada.first)), p.value(entrada.first));
        }
    }
};

QTEST_MAIN(PruebaDocx)
#include "prueba_docx.moc"
