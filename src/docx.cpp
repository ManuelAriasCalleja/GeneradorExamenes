#include "docx.h"
#include "enunciado.h"
#include "maqueta.h"
#include "resaltado.h"
#include "zip.h"

#include <QBuffer>
#include <QDateTime>
#include <QHash>
#include <QImage>
#include <QLocale>
#include <variant>

namespace Docx {

namespace {

using namespace Maqueta;

// Medidas de Word: twips (veinteavos de punto) en los párrafos y las tablas, medios puntos en la letra y EMU en las
// imágenes. La maqueta mide en píxeles a 96 ppp (un píxel, 0,75 puntos).
int twips(qreal px)
{
    return qRound(px * 15);
}

int twipsDePuntos(qreal puntos)
{
    return qRound(puntos * 20);
}

qint64 emu(qreal px)
{
    return qRound64(px * 9525);
}

// La página: A4 con los márgenes de Maquetador::configurarPagina(), 18 mm a los lados y 15 arriba; abajo, los 12 mm y
// el pie, que en Word va en el margen.
constexpr int kAnchoPagina = 11906;
constexpr int kAltoPagina = 16838;
constexpr int kMargenLados = 1020;
constexpr int kMargenArriba = 850;
constexpr int kMargenAbajo = 964;
constexpr int kDistanciaPie = 340;
constexpr int kAnchoTexto = kAnchoPagina - 2 * kMargenLados;   // 174 mm, como Maqueta::kAnchoLinea
const int kSangria = twips(kSangriaOpciones);                   // la de las opciones, las imágenes, el código y las tablas
constexpr int kMuyBajo = 20;                                    // alto (1 punto) de los párrafos que solo separan

// Letras: Arial (en LibreOffice, Liberation Sans, que mide lo mismo) y Courier New para el código.
const char *const kFuente = "Arial";
const char *const kFuenteCodigo = "Courier New";
const char *const kNegro = "000000";
const char *const kGris = "555555";

QString verde()
{
    return QString::fromLatin1(Maquetador::kColorCorrecta).mid(1).toUpper();
}

const char *const kEspacios =
    "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" "
    "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
    "xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing\" "
    "xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
    "xmlns:pic=\"http://schemas.openxmlformats.org/drawingml/2006/picture\"";
const char *const kCabeceraXml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";

// Texto para el XML: escapado y sin los caracteres de control que el XML no admite.
QString xml(const QString &texto)
{
    QString r;
    r.reserve(texto.size());
    for (const QChar c : texto) {
        switch (c.unicode()) {
        case '&': r += QStringLiteral("&amp;"); break;
        case '<': r += QStringLiteral("&lt;"); break;
        case '>': r += QStringLiteral("&gt;"); break;
        case '"': r += QStringLiteral("&quot;"); break;
        default:
            if ((c.unicode() < 0x20 && c != QLatin1Char('\t') && c != QLatin1Char('\n')) || c.unicode() == 0xFFFE
                || c.unicode() == 0xFFFF)
                break;
            r += c;
        }
    }
    return r;
}

// Formato de un trozo de texto.
struct Letra
{
    qreal puntos = 0;      // 0: la del documento (la del examen)
    bool negrita = false;
    bool cursiva = false;
    bool codigo = false;   // de ancho fijo
    QString color;         // «0A7D32»; vacío: negro
};

Letra letra(qreal puntos, bool negrita = false, bool cursiva = false, const QString &color = QString())
{
    Letra l;
    l.puntos = puntos;
    l.negrita = negrita;
    l.cursiva = cursiva;
    l.color = color;
    return l;
}

// En el orden que pide el esquema de Word.
QString propiedades(const Letra &l)
{
    QString r;
    if (l.codigo)
        r += QStringLiteral("<w:rFonts w:ascii=\"%1\" w:hAnsi=\"%1\" w:cs=\"%1\"/>").arg(QLatin1String(kFuenteCodigo));
    if (l.negrita)
        r += QStringLiteral("<w:b/><w:bCs/>");
    if (l.cursiva)
        r += QStringLiteral("<w:i/><w:iCs/>");
    if (!l.color.isEmpty())
        r += QStringLiteral("<w:color w:val=\"%1\"/>").arg(l.color);
    if (l.puntos > 0)
        r += QStringLiteral("<w:sz w:val=\"%1\"/><w:szCs w:val=\"%1\"/>").arg(qRound(l.puntos * 2));
    return r;
}

// Texto con su letra: los saltos de línea pasan a ser <w:br/> y los tabuladores, <w:tab/>.
QString trozo(const QString &texto, const Letra &l = Letra())
{
    if (texto.isEmpty())
        return QString();
    QString r = QStringLiteral("<w:r>");
    const QString rpr = propiedades(l);
    if (!rpr.isEmpty())
        r += QStringLiteral("<w:rPr>") + rpr + QStringLiteral("</w:rPr>");
    const QStringList lineas = texto.split(QLatin1Char('\n'));
    for (int i = 0; i < lineas.size(); ++i) {
        if (i > 0)
            r += QStringLiteral("<w:br/>");
        const QStringList partes = lineas.at(i).split(QLatin1Char('\t'));
        for (int j = 0; j < partes.size(); ++j) {
            if (j > 0)
                r += QStringLiteral("<w:tab/>");
            if (!partes.at(j).isEmpty())
                r += QStringLiteral("<w:t xml:space=\"preserve\">") + xml(partes.at(j)) + QStringLiteral("</w:t>");
        }
    }
    return r + QStringLiteral("</w:r>");
}

// Texto de una pregunta con su letra; su código en línea (entre comillas invertidas: ver enunciado.h), con la misma
// pero de ancho fijo y sin cursiva.
QString textoPregunta(const QString &t, const Letra &l = Letra())
{
    QString r;
    for (const Enunciado::Trozo &parte : Enunciado::trozos(t)) {
        Letra letraParte = l;
        if (parte.codigo) {
            letraParte.codigo = true;
            letraParte.cursiva = false;
        }
        r += trozo(parte.texto, letraParte);
    }
    return r;
}

const QString kSalto = QStringLiteral("<w:r><w:br/></w:r>");

struct Parrafo
{
    QString contenido;           // los <w:r>: texto, saltos e imágenes
    int antes = 0;               // espacio encima y debajo, en twips
    int despues = 0;
    int izquierda = 0;           // sangría, en twips
    int colgante = 0;            // sangría francesa (las líneas que siguen a la primera)
    int tabulador = 0;           // un tabulador en esa posición (0: ninguno)
    QString alineacion;          // «right», «center»; vacío: a la izquierda
    int interlineadoExacto = 0;  // en twips; 0: sencillo
    Letra marca;                 // la letra de la marca de párrafo (da el alto a los vacíos)
    bool seguir = false;         // en la misma página que lo que le sigue, y sin partirse
    QString seccion;             // <w:sectPr> si es el último de una sección

    QString xml() const
    {
        QString p;
        if (seguir)
            p += QStringLiteral("<w:keepNext/><w:keepLines/>");
        if (tabulador > 0)
            p += QStringLiteral("<w:tabs><w:tab w:val=\"left\" w:pos=\"%1\"/></w:tabs>").arg(tabulador);
        p += QStringLiteral("<w:spacing w:before=\"%1\" w:after=\"%2\"").arg(antes).arg(despues);
        if (interlineadoExacto > 0)
            p += QStringLiteral(" w:line=\"%1\" w:lineRule=\"exact\"").arg(interlineadoExacto);
        p += QStringLiteral("/>");
        if (izquierda > 0 || colgante > 0) {
            p += QStringLiteral("<w:ind w:left=\"%1\"").arg(izquierda);
            if (colgante > 0)
                p += QStringLiteral(" w:hanging=\"%1\"").arg(colgante);
            p += QStringLiteral("/>");
        }
        if (!alineacion.isEmpty())
            p += QStringLiteral("<w:jc w:val=\"%1\"/>").arg(alineacion);
        const QString rpr = propiedades(marca);
        if (!rpr.isEmpty())
            p += QStringLiteral("<w:rPr>") + rpr + QStringLiteral("</w:rPr>");
        p += seccion;
        return QStringLiteral("<w:p><w:pPr>") + p + QStringLiteral("</w:pPr>") + contenido + QStringLiteral("</w:p>");
    }
};

Parrafo parrafo(const QString &contenido, const QString &alineacion = QString())
{
    Parrafo p;
    p.contenido = contenido;
    p.alineacion = alineacion;
    return p;
}

// Párrafo vacío de 'alto' twips, para separar (dos tablas seguidas se juntarían en una).
Parrafo separacion(int alto = kMuyBajo)
{
    Parrafo p;
    p.interlineadoExacto = alto;
    p.marca.puntos = 1;
    return p;
}

struct Celda
{
    int ancho = 0;                  // en twips
    QVector<Parrafo> parrafos;
    QString fondo;                  // color de fondo; vacío: ninguno
};

Celda celda(int ancho, const QVector<Parrafo> &parrafos = {})
{
    Celda c;
    c.ancho = ancho;
    c.parrafos = parrafos;
    return c;
}

// Tabla de anchos fijos.
struct Tabla
{
    QVector<QVector<Celda>> filas;
    int sangria = 0;                // en twips
    QString borde;                  // color de los bordes; vacío: sin bordes
    int margenIzquierda = 0;        // de las celdas, en twips
    int margenDerecha = 0;
    int margenVertical = 0;
    bool juntas = false;            // las filas, en la misma página

    QString xml(bool seguir) const
    {
        QVector<int> rejilla;
        for (const QVector<Celda> &fila : filas) {
            if (fila.size() > rejilla.size()) {
                rejilla.clear();
                for (const Celda &c : fila)
                    rejilla << c.ancho;
            }
        }
        int total = 0;
        for (int ancho : rejilla)
            total += ancho;
        QString t = QStringLiteral("<w:tbl><w:tblPr><w:tblW w:w=\"%1\" w:type=\"dxa\"/>").arg(total);
        if (sangria > 0)
            t += QStringLiteral("<w:tblInd w:w=\"%1\" w:type=\"dxa\"/>").arg(sangria);
        t += QStringLiteral("<w:tblBorders>");
        for (const char *lado : {"top", "left", "bottom", "right", "insideH", "insideV"}) {
            if (borde.isEmpty())
                t += QStringLiteral("<w:%1 w:val=\"nil\"/>").arg(QLatin1String(lado));
            else
                t += QStringLiteral("<w:%1 w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"%2\"/>").arg(QLatin1String(lado), borde);
        }
        t += QStringLiteral("</w:tblBorders><w:tblLayout w:type=\"fixed\"/>"
                            "<w:tblCellMar><w:top w:w=\"%1\" w:type=\"dxa\"/><w:left w:w=\"%2\" w:type=\"dxa\"/>"
                            "<w:bottom w:w=\"%1\" w:type=\"dxa\"/><w:right w:w=\"%3\" w:type=\"dxa\"/></w:tblCellMar>"
                            "</w:tblPr><w:tblGrid>")
                 .arg(margenVertical)
                 .arg(margenIzquierda)
                 .arg(margenDerecha);
        for (int ancho : rejilla)
            t += QStringLiteral("<w:gridCol w:w=\"%1\"/>").arg(ancho);
        t += QStringLiteral("</w:tblGrid>");
        for (int f = 0; f < filas.size(); ++f) {
            // Las filas de una tabla que va junta siguen con la siguiente; la última, con lo que siga a la tabla.
            const bool seguirFila = f + 1 < filas.size() ? juntas : seguir;
            t += QStringLiteral("<w:tr><w:trPr>");
            if (filas.at(f).size() < rejilla.size())
                t += QStringLiteral("<w:gridAfter w:val=\"%1\"/>").arg(rejilla.size() - filas.at(f).size());
            t += QStringLiteral("<w:cantSplit/></w:trPr>");
            for (const Celda &c : filas.at(f)) {
                t += QStringLiteral("<w:tc><w:tcPr><w:tcW w:w=\"%1\" w:type=\"dxa\"/>").arg(c.ancho);
                if (!c.fondo.isEmpty())
                    t += QStringLiteral("<w:shd w:val=\"clear\" w:color=\"auto\" w:fill=\"%1\"/>").arg(c.fondo);
                t += QStringLiteral("</w:tcPr>");
                // Una celda tiene que tener al menos un párrafo.
                QVector<Parrafo> parrafos = c.parrafos.isEmpty() ? QVector<Parrafo>{Parrafo()} : c.parrafos;
                for (Parrafo &p : parrafos) {
                    p.seguir = p.seguir || seguirFila;
                    t += p.xml();
                }
                t += QStringLiteral("</w:tc>");
            }
            t += QStringLiteral("</w:tr>");
        }
        return t + QStringLiteral("</w:tbl>");
    }
};

using Bloque = std::variant<Parrafo, Tabla>;

// El documento a medida que se escribe: el cuerpo, los pies de cada sección y las imágenes.
class Documento
{
public:
    // Añade los bloques; si van 'juntos', en la misma página (cada uno sigue con el siguiente).
    void anadir(const QVector<Bloque> &bloques, bool juntos)
    {
        // Si la sección empieza por una tabla, delante, un párrafo: LibreOffice no vuelve a numerar las páginas desde 1
        // en una sección que empieza por una tabla.
        if (m_inicioSeccion && !bloques.isEmpty() && std::holds_alternative<Tabla>(bloques.first()))
            m_cuerpo += separacion().xml();
        m_inicioSeccion = false;
        for (int i = 0; i < bloques.size(); ++i) {
            const bool seguir = juntos && i + 1 < bloques.size();
            if (const Parrafo *p = std::get_if<Parrafo>(&bloques.at(i))) {
                Parrafo copia = *p;
                copia.seguir = copia.seguir || seguir;
                m_cuerpo += copia.xml();
                m_tablaAlFinal = false;
            } else {
                if (m_tablaAlFinal) {
                    Parrafo s = separacion();
                    s.seguir = juntos;
                    m_cuerpo += s.xml();
                }
                m_cuerpo += std::get<Tabla>(bloques.at(i)).xml(seguir);
                m_tablaAlFinal = true;
            }
        }
    }

    // Termina la sección (un examen o la hoja de soluciones): empieza en página nueva, con la numeración desde 1, y
    // lleva su pie «texto: página / total». El total es la página de un marcador al final de la sección, como en
    // LaTeX: el campo SECTIONPAGES de Word, que lo da directamente, no lo entiende LibreOffice.
    void terminarSeccion(const QString &textoPie, bool ultima)
    {
        m_pies << pie(textoPie, marcadorFin(m_pies.size() + 1));
        const QString seccion = QStringLiteral(
                                    "<w:sectPr><w:footerReference w:type=\"default\" r:id=\"rIdPie%1\"/><w:type w:val=\"nextPage\"/>"
                                    "<w:pgSz w:w=\"%2\" w:h=\"%3\"/>"
                                    "<w:pgMar w:top=\"%4\" w:right=\"%5\" w:bottom=\"%6\" w:left=\"%5\" w:header=\"%7\" w:footer=\"%7\" w:gutter=\"0\"/>"
                                    "<w:pgNumType w:start=\"1\"/><w:cols w:space=\"708\"/></w:sectPr>")
                                    .arg(m_pies.size())
                                    .arg(kAnchoPagina)
                                    .arg(kAltoPagina)
                                    .arg(kMargenArriba)
                                    .arg(kMargenLados)
                                    .arg(kMargenAbajo)
                                    .arg(kDistanciaPie);
        // Las secciones terminan con un párrafo casi invisible con el marcador (Word pide un párrafo detrás de una tabla).
        // Las que no son la última, con otro con su <w:sectPr> (LibreOffice pasaría el marcador de ese a la sección
        // siguiente); el de la última va al final del cuerpo.
        Parrafo fin = separacion();
        fin.contenido = QStringLiteral("<w:bookmarkStart w:id=\"%1\" w:name=\"%2\"/><w:bookmarkEnd w:id=\"%1\"/>")
                            .arg(m_pies.size())
                            .arg(marcadorFin(m_pies.size()));
        m_cuerpo += fin.xml();
        if (!ultima) {
            Parrafo salto = separacion();
            salto.seccion = seccion;
            m_cuerpo += salto.xml();
        }
        m_tablaAlFinal = false;
        m_inicioSeccion = true;
        if (ultima)
            m_cuerpo += seccion;
    }

    // La imagen de la pregunta, del tamaño de la maqueta, en un trozo de texto.
    QString imagen(const Pregunta &p, int n)
    {
        const QSize tamano = tamanoImagen(p, n);
        if (tamano.isEmpty())
            return QString();
        // Cada imagen distinta, una vez (en PNG, aunque se guardara en otro formato).
        int numero = m_numeroImagen.value(p.imagen);
        if (numero == 0) {
            QImage img;
            img.loadFromData(p.imagen);
            QByteArray png;
            QBuffer buffer(&png);
            buffer.open(QIODevice::WriteOnly);
            img.save(&buffer, "PNG");
            m_imagenes << png;
            numero = m_imagenes.size();
            m_numeroImagen.insert(p.imagen, numero);
        }
        const int dibujo = ++m_dibujos;
        return QStringLiteral(
                   "<w:r><w:drawing><wp:inline distT=\"0\" distB=\"0\" distL=\"0\" distR=\"0\">"
                   "<wp:extent cx=\"%1\" cy=\"%2\"/><wp:effectExtent l=\"0\" t=\"0\" r=\"0\" b=\"0\"/>"
                   "<wp:docPr id=\"%3\" name=\"%4\"/>"
                   "<wp:cNvGraphicFramePr><a:graphicFrameLocks noChangeAspect=\"1\"/></wp:cNvGraphicFramePr>"
                   "<a:graphic><a:graphicData uri=\"http://schemas.openxmlformats.org/drawingml/2006/picture\">"
                   "<pic:pic><pic:nvPicPr><pic:cNvPr id=\"%3\" name=\"%4\"/><pic:cNvPicPr/></pic:nvPicPr>"
                   "<pic:blipFill><a:blip r:embed=\"rIdImagen%5\"/><a:stretch><a:fillRect/></a:stretch></pic:blipFill>"
                   "<pic:spPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"%1\" cy=\"%2\"/></a:xfrm>"
                   "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom></pic:spPr></pic:pic>"
                   "</a:graphicData></a:graphic></wp:inline></w:drawing></w:r>")
            .arg(QString::number(emu(tamano.width())), QString::number(emu(tamano.height())), QString::number(dibujo),
                 xml(p.codigoCompleto()), QString::number(numero));
    }

    QMap<QString, QByteArray> partes(const DatosConvocatoria &d, int n) const;

private:
    // Nombre del marcador del final de la sección 'n' (empieza por «_»: Word no lo enseña en su lista).
    static QString marcadorFin(int n)
    {
        return QStringLiteral("_FinSeccion%1").arg(n);
    }

    static QString pie(const QString &texto, const QString &marcador)
    {
        const Letra l = letra(8);
        auto campo = [&](const QString &instruccion) {
            return QStringLiteral("<w:fldSimple w:instr=\" %1 \">").arg(instruccion) + trozo(QStringLiteral("1"), l)
                   + QStringLiteral("</w:fldSimple>");
        };
        const Parrafo p = parrafo(trozo(texto + QStringLiteral(": "), l) + campo(QStringLiteral("PAGE")) + trozo(QStringLiteral(" / "), l)
                                      + campo(QStringLiteral("PAGEREF ") + marcador),
                                  QStringLiteral("right"));
        return QLatin1String(kCabeceraXml) + QStringLiteral("<w:ftr ") + QLatin1String(kEspacios) + QLatin1Char('>') + p.xml()
               + QStringLiteral("</w:ftr>");
    }

    QString m_cuerpo;
    QStringList m_pies;
    QVector<QByteArray> m_imagenes;
    QHash<QByteArray, int> m_numeroImagen;
    int m_dibujos = 0;
    bool m_tablaAlFinal = false;
    bool m_inicioSeccion = true;
};

// El idioma del texto (para la ortografía), como lo escribe Word: «es-ES».
QString idiomaWord(const QString &idioma)
{
    if (idioma.isEmpty() || idioma == QLatin1String("es"))
        return QStringLiteral("es-ES");
    return QLocale(idioma).name().replace(QLatin1Char('_'), QLatin1Char('-'));
}

QMap<QString, QByteArray> Documento::partes(const DatosConvocatoria &d, int n) const
{
    QMap<QString, QByteArray> res;
    auto poner = [&](const QString &ruta, const QString &contenido) {
        res.insert(ruta, (QLatin1String(kCabeceraXml) + contenido).toUtf8());
    };

    QString tipos = QStringLiteral(
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Default Extension=\"png\" ContentType=\"image/png\"/>"
        "<Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>"
        "<Override PartName=\"/word/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml\"/>"
        "<Override PartName=\"/word/settings.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml\"/>"
        "<Override PartName=\"/docProps/core.xml\" ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>"
        "<Override PartName=\"/docProps/app.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/>");
    for (int i = 1; i <= m_pies.size(); ++i)
        tipos += QStringLiteral("<Override PartName=\"/word/footer%1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml\"/>")
                     .arg(i);
    poner(QStringLiteral("[Content_Types].xml"), tipos + QStringLiteral("</Types>"));

    poner(QStringLiteral("_rels/.rels"),
          QStringLiteral(
              "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
              "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"word/document.xml\"/>"
              "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/>"
              "<Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" Target=\"docProps/app.xml\"/>"
              "</Relationships>"));

    // Propiedades: el título y, en los comentarios y en el programa, la semilla.
    QString core = QStringLiteral(
        "<cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" "
        "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:dcterms=\"http://purl.org/dc/terms/\" "
        "xmlns:dcmitype=\"http://purl.org/dc/dcmitype/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">");
    if (!tituloDocumento(d).isEmpty())
        core += QStringLiteral("<dc:title>") + xml(tituloDocumento(d)) + QStringLiteral("</dc:title>");
    if (!textoSemilla(d).isEmpty())
        core += QStringLiteral("<dc:description>") + xml(textoSemilla(d)) + QStringLiteral("</dc:description>");
    core += QStringLiteral("<dcterms:created xsi:type=\"dcterms:W3CDTF\">%1</dcterms:created></cp:coreProperties>")
                .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    poner(QStringLiteral("docProps/core.xml"), core);
    poner(QStringLiteral("docProps/app.xml"),
          QStringLiteral("<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\">"
                         "<Application>%1</Application></Properties>")
              .arg(xml(programa(d))));

    QString relaciones = QStringLiteral(
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rIdEstilos\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>"
        "<Relationship Id=\"rIdAjustes\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings\" Target=\"settings.xml\"/>");
    for (int i = 1; i <= m_pies.size(); ++i)
        relaciones += QStringLiteral("<Relationship Id=\"rIdPie%1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer\" Target=\"footer%1.xml\"/>")
                          .arg(i);
    for (int i = 1; i <= m_imagenes.size(); ++i)
        relaciones += QStringLiteral("<Relationship Id=\"rIdImagen%1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" Target=\"media/imagen%1.png\"/>")
                          .arg(i);
    poner(QStringLiteral("word/_rels/document.xml.rels"), relaciones + QStringLiteral("</Relationships>"));

    // La letra del examen (la del nivel), en el idioma del examen; los párrafos, sin espacio entre ellos.
    poner(QStringLiteral("word/styles.xml"),
          QStringLiteral(
              "<w:styles %1><w:docDefaults><w:rPrDefault><w:rPr>"
              "<w:rFonts w:ascii=\"%2\" w:eastAsia=\"%2\" w:hAnsi=\"%2\" w:cs=\"%2\"/>"
              "<w:sz w:val=\"%3\"/><w:szCs w:val=\"%3\"/><w:lang w:val=\"%4\"/></w:rPr></w:rPrDefault>"
              "<w:pPrDefault><w:pPr><w:spacing w:after=\"0\" w:line=\"240\" w:lineRule=\"auto\"/></w:pPr></w:pPrDefault>"
              "</w:docDefaults>"
              "<w:style w:type=\"paragraph\" w:default=\"1\" w:styleId=\"Normal\"><w:name w:val=\"Normal\"/><w:qFormat/></w:style>"
              "<w:style w:type=\"character\" w:default=\"1\" w:styleId=\"DefaultParagraphFont\"><w:name w:val=\"Default Paragraph Font\"/>"
              "<w:uiPriority w:val=\"1\"/><w:semiHidden/><w:unhideWhenUsed/></w:style>"
              "<w:style w:type=\"table\" w:default=\"1\" w:styleId=\"TableNormal\"><w:name w:val=\"Normal Table\"/>"
              "<w:uiPriority w:val=\"99\"/><w:semiHidden/><w:unhideWhenUsed/><w:tblPr><w:tblInd w:w=\"0\" w:type=\"dxa\"/>"
              "<w:tblCellMar><w:top w:w=\"0\" w:type=\"dxa\"/><w:left w:w=\"108\" w:type=\"dxa\"/>"
              "<w:bottom w:w=\"0\" w:type=\"dxa\"/><w:right w:w=\"108\" w:type=\"dxa\"/></w:tblCellMar></w:tblPr></w:style>"
              "</w:styles>")
              .arg(QLatin1String(kEspacios), QLatin1String(kFuente), QString::number(qRound(nivel(n).fuentePt * 2)),
                   idiomaWord(d.idioma)));
    // Sin el modo de compatibilidad de las versiones antiguas de Word.
    poner(QStringLiteral("word/settings.xml"),
          QStringLiteral("<w:settings %1><w:defaultTabStop w:val=\"708\"/><w:characterSpacingControl w:val=\"doNotCompress\"/>"
                         "<w:compat><w:compatSetting w:name=\"compatibilityMode\" w:uri=\"http://schemas.microsoft.com/office/word\" w:val=\"15\"/>"
                         "</w:compat></w:settings>")
              .arg(QLatin1String(kEspacios)));

    poner(QStringLiteral("word/document.xml"),
          QStringLiteral("<w:document %1><w:body>").arg(QLatin1String(kEspacios)) + m_cuerpo
              + QStringLiteral("</w:body></w:document>"));
    for (int i = 0; i < m_pies.size(); ++i)
        res.insert(QStringLiteral("word/footer%1.xml").arg(i + 1), m_pies.at(i).toUtf8());
    for (int i = 0; i < m_imagenes.size(); ++i)
        res.insert(QStringLiteral("word/media/imagen%1.png").arg(i + 1), m_imagenes.at(i));
    return res;
}

// ---------------------------------------------------------------------------------------------
// Las partes del examen, como en el HTML de la maqueta (Maquetador::cuerpoExamen())

// Cabecera: a la izquierda, la asignatura (en grande) y la convocatoria, si se ponen; a la derecha, 'derecha'. Sin
// filas si no queda nada. 'puntos' es la letra de la convocatoria (0: la del examen).
Tabla cabecera(const DatosConvocatoria &d, const QVector<Parrafo> &derecha, int n, qreal puntos = 0)
{
    Tabla t;
    const int anchoIzquierda = kAnchoTexto * 6 / 10;
    Celda izquierda = celda(anchoIzquierda);
    if (!asignaturaCabecera(d).isEmpty())
        izquierda.parrafos << parrafo(trozo(asignaturaCabecera(d), letra(14 * escala(n), true)));
    if (!convocatoriaCabecera(d).isEmpty())
        izquierda.parrafos << parrafo(trozo(convocatoriaCabecera(d), letra(puntos)));
    if (izquierda.parrafos.isEmpty() && derecha.isEmpty())
        return t;
    Celda dcha = celda(kAnchoTexto - anchoIzquierda, derecha);
    for (Parrafo &p : dcha.parrafos)
        p.alineacion = QStringLiteral("right");
    t.filas = {{izquierda, dcha}};
    t.juntas = true;
    return t;
}

// Recuadro de instrucciones: el texto y, justo debajo, la puntuación en una línea.
Tabla instrucciones(const DatosConvocatoria &d)
{
    QVector<Parrafo> parrafos;
    if (!d.instrucciones.trimmed().isEmpty())
        parrafos << parrafo(trozo(d.instrucciones.trimmed()));
    parrafos << parrafo(trozo(textoPuntuacion(d)));
    Tabla t;
    t.filas = {{celda(kAnchoTexto, parrafos)}};
    t.borde = QLatin1String(kNegro);
    t.margenIzquierda = t.margenDerecha = t.margenVertical = twips(4);
    t.juntas = true;
    return t;
}

// Ancho de un texto de 'caracteres' caracteres en Arial de 'puntos' puntos, estimado (en twips).
int anchoTexto(qsizetype caracteres, qreal puntos)
{
    return qRound(caracteres * 0.55 * puntos * 20);
}

// Tabla de un enunciado («| a | b |»): con bordes, del ancho de su contenido (como mucho, el de la línea).
Tabla tablaEnunciado(const Enunciado::Bloque &b, int n)
{
    int columnas = 0;
    for (const QStringList &fila : b.filas)
        columnas = qMax(columnas, int(fila.size()));
    QVector<int> anchos(columnas, 0);
    const int margen = twips(3);
    for (const QStringList &fila : b.filas)
        for (int c = 0; c < fila.size(); ++c)
            anchos[c] = qMax(anchos[c], anchoTexto(fila.at(c).size(), nivel(n).fuentePt) + 2 * margen + 40);
    int total = 0;
    for (int ancho : anchos)
        total += ancho;
    const int disponible = kAnchoTexto - kSangria;
    if (total > disponible)
        for (int &ancho : anchos)
            ancho = ancho * disponible / total;
    Tabla t;
    for (const QStringList &fila : b.filas) {
        QVector<Celda> celdas;
        for (int c = 0; c < columnas; ++c)
            celdas << celda(anchos[c], {parrafo(textoPregunta(fila.value(c)))});
        t.filas << celdas;
    }
    t.sangria = kSangria;
    t.borde = QLatin1String(kNegro);
    t.margenIzquierda = t.margenDerecha = t.margenVertical = margen;
    t.juntas = true;
    return t;
}

// Código de un enunciado, como en el PDF: en un recuadro gris con borde, del ancho de su línea más larga (como mucho,
// el de la línea del examen, y entonces se parten las líneas), resaltado según su lenguaje y, si los lleva, con los
// números de línea en gris a la izquierda (las líneas partidas siguen a su derecha).
Tabla codigo(const Enunciado::Bloque &b, int n)
{
    QStringList lineas;
    int maximo = 1;
    for (QString linea : b.lineas) {
        linea.replace(QLatin1Char('\t'), QStringLiteral("    "));
        maximo = qMax(maximo, int(linea.size()));
        lineas << linea;
    }
    const QVector<QVector<Resaltado::Trozo>> resaltadas = Resaltado::resaltar(lineas, b.lenguaje);
    const qreal puntos = nivel(n).fuentePt;
    const int caracter = qRound(0.6 * puntos * 20);   // ancho de un carácter de Courier New, en twips
    const int cifras = b.numerado ? QString::number(qMax(1, int(lineas.size()))).size() : 0;
    const int anchoNumeros = b.numerado ? (cifras + 1) * caracter : 0;
    const int margen = twips(4);
    const int ancho = qMin(kAnchoTexto - kSangria, anchoNumeros + (maximo + 1) * caracter + 2 * margen);

    Letra normal;
    normal.codigo = true;
    auto letraDe = [&](Resaltado::Estilo estilo) {
        Letra l = normal;
        switch (estilo) {
        case Resaltado::Clave:
            l.negrita = true;
            l.color = QLatin1String(Resaltado::kColorClave);
            break;
        case Resaltado::Comentario:
            l.cursiva = true;
            l.color = QLatin1String(Resaltado::kColorComentario);
            break;
        case Resaltado::Cadena:
            l.color = QLatin1String(Resaltado::kColorCadena);
            break;
        case Resaltado::Normal:
            break;
        }
        return l;
    };
    Celda c = celda(ancho);
    c.fondo = QStringLiteral("EEEEEE");
    for (int i = 0; i < resaltadas.size(); ++i) {
        Parrafo p;
        p.marca = normal;
        if (b.numerado) {
            Letra gris = normal;
            gris.color = QLatin1String(Resaltado::kColorNumero);
            p.izquierda = p.colgante = p.tabulador = anchoNumeros;
            p.contenido = trozo(QString::number(i + 1).rightJustified(cifras), gris) + QStringLiteral("<w:r><w:tab/></w:r>");
        }
        for (const Resaltado::Trozo &t : resaltadas.at(i))
            p.contenido += trozo(t.texto, letraDe(t.estilo));
        c.parrafos << p;
    }
    Tabla t;
    t.filas = {{c}};
    t.sangria = kSangria;
    t.borde = QStringLiteral("808080");
    t.margenIzquierda = t.margenDerecha = t.margenVertical = margen;
    t.juntas = true;
    return t;
}

// «a) texto»; la correcta, en las soluciones, en verde y negrita, con una marca.
QString textoOpcion(const PreguntaExamen &pe, int i, bool marcada)
{
    const QString texto = QString(Pregunta::letra(i)) + QStringLiteral(") ") + pe.opcion(i);
    if (marcada)
        return textoPregunta(texto + QLatin1Char(' ') + QChar(0x2714), letra(0, true, false, verde()));
    return textoPregunta(texto);
}

// Opciones a)..d) repartidas en filas como en la maqueta (Maqueta::filasOpciones()).
Tabla opciones(const PreguntaExamen &pe, bool soluciones, int n)
{
    const int filas = filasOpciones(pe, n);
    const int disponible = kAnchoTexto - kSangria;
    auto op = [&](int i, int ancho) {
        return celda(ancho, {parrafo(textoOpcion(pe, i, soluciones && i == pe.correcta()))});
    };
    Tabla t;
    if (filas == 1) {
        t.filas = {{op(0, disponible / 4), op(1, disponible / 4), op(2, disponible / 4), op(3, disponible / 4)}};
    } else if (filas == 2) {
        for (int fila = 0; fila < 2; ++fila)
            t.filas << QVector<Celda>{op(fila, disponible / 2), op(fila + 2, disponible / 2)};
    } else {
        for (int i = 0; i < 4; ++i)
            t.filas << QVector<Celda>{op(i, disponible)};
    }
    t.sangria = kSangria;
    t.margenDerecha = twips(4);
    t.margenVertical = twips(1);
    t.juntas = true;
    return t;
}

// Las opciones, una por línea, y la imagen a su derecha (ver Maqueta::imagenAlLado()).
Tabla opcionesConImagen(Documento &doc, const PreguntaExamen &pe, bool soluciones, int n)
{
    const int anchoImagen = twips(tamanoImagen(pe.pregunta, n).width());
    const int separacionImagen = twips(kSeparacionImagen);
    qsizetype caracteres = 0;
    QVector<Parrafo> parrafos;
    for (int i = 0; i < 4; ++i) {
        caracteres = qMax(caracteres, pe.opcion(i).size() + 5);
        Parrafo p = parrafo(textoOpcion(pe, i, soluciones && i == pe.correcta()));
        p.despues = twips(2);
        parrafos << p;
    }
    const int disponible = kAnchoTexto - kSangria - separacionImagen - anchoImagen;
    const int anchoOpciones = qMin(disponible, anchoTexto(caracteres, nivel(n).fuentePt) + twips(4));
    Tabla t;
    t.filas = {{celda(anchoOpciones, parrafos), celda(separacionImagen), celda(anchoImagen, {parrafo(doc.imagen(pe.pregunta, n))})}};
    t.sangria = kSangria;
    t.juntas = true;
    return t;
}

// En las soluciones, debajo de las opciones: la explicación y la referencia, si las tiene.
QVector<Parrafo> explicacion(const Pregunta &p, const DatosConvocatoria &d, int n)
{
    const Letra normal = letra(puntosExplicacion(n));
    const Letra etiqueta = letra(puntosExplicacion(n), true, false, verde());
    QStringList partes;
    // La explicación admite código en línea (entre comillas invertidas); la referencia, no: es texto tal cual.
    auto parte = [&](const char *nombre, const QString &texto, bool conCodigo) {
        if (texto.trimmed().isEmpty())
            return;
        const QString contenido = QStringLiteral(" ") + texto.trimmed();
        partes << trozo(tx(d, nombre), etiqueta) + (conCodigo ? textoPregunta(contenido, normal) : trozo(contenido, normal));
    };
    parte(QT_TRANSLATE_NOOP("Examen", "Explicación:"), p.explicacion, true);
    parte(QT_TRANSLATE_NOOP("Examen", "Referencia:"), p.referencia, false);
    if (partes.isEmpty())
        return {};
    Parrafo par = parrafo(partes.join(kSalto));
    par.izquierda = kSangria;
    par.antes = twips(3);
    return {par};
}

// Una pregunta: el enunciado (con su número y, en las soluciones, su código), la imagen, las opciones y, en las
// soluciones, la explicación y la referencia.
QVector<Bloque> pregunta(Documento &doc, const PreguntaExamen &pe, int indice, const DatosConvocatoria &d, bool soluciones, int n)
{
    // Con letra grande, el enunciado va en negrita sin cursiva, que cuesta más de leer.
    const Letra enunciado = letra(0, true, n != Maquetador::kLetraGrande && d.enunciadoEnCursiva);
    QString prefijo = trozo(QStringLiteral("%1.- ").arg(indice + 1), enunciado);
    if (soluciones)
        prefijo += trozo(QStringLiteral("[%1] ").arg(pe.pregunta.codigoCompleto()), letra(0, true, false, verde()));
    QVector<Bloque> bloques;
    bool prefijoPuesto = false;
    // El prefijo va al principio del primer párrafo; si antes hay código o una tabla, en un párrafo propio.
    auto ponerPrefijoSolo = [&]() {
        if (!prefijoPuesto) {
            Parrafo p = parrafo(prefijo);
            p.despues = twips(2);
            bloques << p;
            prefijoPuesto = true;
        }
    };
    for (const Enunciado::Bloque &b : Enunciado::bloques(pe.pregunta.enunciado)) {
        switch (b.tipo) {
        case Enunciado::Bloque::Parrafo: {
            Parrafo p = parrafo((prefijoPuesto ? QString() : prefijo) + textoPregunta(b.lineas.join(QLatin1Char('\n')), enunciado));
            p.despues = twips(2);
            prefijoPuesto = true;
            bloques << p;
            break;
        }
        case Enunciado::Bloque::Tabla:
            ponerPrefijoSolo();
            bloques << tablaEnunciado(b, n);
            break;
        case Enunciado::Bloque::Codigo:
            ponerPrefijoSolo();
            bloques << codigo(b, n);
            break;
        }
    }
    ponerPrefijoSolo();
    std::get<Parrafo>(bloques.first()).antes = twips(indice == 0 ? nivel(n).separacionPx + 4 : nivel(n).separacionPx);

    if (imagenAlLado(pe, n)) {
        bloques << opcionesConImagen(doc, pe, soluciones, n);
    } else {
        const QString imagen = doc.imagen(pe.pregunta, n);
        if (!imagen.isEmpty()) {
            Parrafo p = parrafo(imagen);
            p.izquierda = kSangria;
            p.antes = p.despues = twips(4);
            bloques << p;
        }
        bloques << opciones(pe, soluciones, n);
    }
    if (soluciones)
        for (const Parrafo &p : explicacion(pe.pregunta, d, n))
            bloques << p;
    return bloques;
}

void examen(Documento &doc, const Modelo &m, const DatosConvocatoria &d, bool soluciones, int n)
{
    QVector<Parrafo> derecha;
    if (!Maquetador::textoVersion(m, d).isEmpty())
        derecha << parrafo(trozo(Maquetador::textoVersion(m, d), letra(14 * escala(n), true)));
    if (soluciones)
        derecha << parrafo(trozo(tx(d, QT_TRANSLATE_NOOP("Examen", "CON SOLUCIONES")), letra(0, true, false, verde())));
    QVector<Bloque> inicio;
    const Tabla cab = cabecera(d, derecha, n);
    if (!cab.filas.isEmpty())
        inicio << cab << separacion(twips(6));
    inicio << instrucciones(d);
    doc.anadir(inicio, false);
    for (int i = 0; i < m.preguntas.size(); ++i)
        doc.anadir(pregunta(doc, m.preguntas.at(i), i, d, soluciones, n), true);
}

// Hoja de soluciones, como Maquetador::solucionesHtml(): la estructura de la convocatoria y, de cada modelo, su
// plantilla de corrección y el código, la respuesta y la referencia de cada pregunta.
void hojaSoluciones(Documento &doc, const QVector<Modelo> &modelos, const DatosConvocatoria &d)
{
    const qreal puntos = 10;
    const Letra normal = letra(puntos);
    const Letra negrita = letra(puntos, true);
    QVector<Parrafo> derecha{parrafo(trozo(tx(d, QT_TRANSLATE_NOOP("Examen", "Soluciones")), letra(16, true)))};
    if (!textoSemilla(d).isEmpty())
        derecha << parrafo(trozo(textoSemilla(d), letra(8, false, false, QLatin1String(kGris))));
    QVector<Bloque> bloques;
    const Tabla cab = cabecera(d, derecha, 0, puntos);
    if (!cab.filas.isEmpty())
        bloques << cab;
    auto titulo = [&](const QString &texto) {
        Parrafo p = parrafo(trozo(texto, letra(12, true)));
        p.antes = twipsDePuntos(10);
        p.despues = twipsDePuntos(4);
        p.seguir = true;
        return p;
    };
    const int margen = twips(3);

    // Estructura: qué preguntas de cada tema entran en cada modelo.
    bloques << titulo(tx(d, QT_TRANSLATE_NOOP("Examen", "Estructura de la convocatoria")));
    const QList<int> temas = temasDe(modelos);
    qsizetype caracteres = tx(d, QT_TRANSLATE_NOOP("Examen", "Examen")).size();
    for (const Modelo &m : modelos)
        caracteres = qMax(caracteres, m.nombre.size());
    const int anchoNombre = qBound(1000, anchoTexto(caracteres, puntos) + 2 * margen + 60, kAnchoTexto / 3);
    const int anchoTema = temas.isEmpty() ? 0 : (kAnchoTexto - anchoNombre) / int(temas.size());
    Tabla estructura;
    QVector<Celda> encabezado{celda(anchoNombre, {parrafo(trozo(tx(d, QT_TRANSLATE_NOOP("Examen", "Examen")), negrita))})};
    for (int t : temas)
        encabezado << celda(anchoTema, {parrafo(trozo(QStringLiteral("T%1").arg(t), negrita), QStringLiteral("center"))});
    estructura.filas << encabezado;
    for (const Modelo &m : modelos) {
        QVector<Celda> fila{celda(anchoNombre, {parrafo(trozo(m.nombre, negrita))})};
        for (int t : temas)
            fila << celda(anchoTema, {parrafo(trozo(codigosDelTema(m, t).join(QStringLiteral(", ")), normal))});
        estructura.filas << fila;
    }
    estructura.borde = QLatin1String(kNegro);
    estructura.margenIzquierda = estructura.margenDerecha = estructura.margenVertical = margen;
    bloques << estructura;

    // Plantilla de corrección de cada modelo («1 C», «2 A»…, en filas de 10) y, debajo, código, respuesta y referencia
    // de cada pregunta.
    const Letra pequena = letra(9);
    for (const Modelo &m : modelos) {
        bloques << titulo(tx(d, QT_TRANSLATE_NOOP("Examen", "Plantilla de corrección: %1")).arg(m.nombre));
        if (m.preguntas.isEmpty())
            continue;
        Tabla plantilla;
        for (int inicio = 0; inicio < m.preguntas.size(); inicio += 10) {
            QVector<Celda> fila;
            for (int i = inicio; i < qMin(inicio + 10, int(m.preguntas.size())); ++i)
                fila << celda(twips(44), {parrafo(trozo(QString::number(i + 1) + QChar(0x00A0), normal)
                                                       + trozo(QString(Pregunta::letraMayuscula(m.preguntas.at(i).correcta())), negrita),
                                                   QStringLiteral("center"))});
            plantilla.filas << fila;
        }
        plantilla.borde = QLatin1String(kNegro);
        plantilla.margenIzquierda = plantilla.margenDerecha = plantilla.margenVertical = margen;
        plantilla.juntas = true;
        bloques << plantilla;
        Parrafo correctas = parrafo(trozo(tx(d, QT_TRANSLATE_NOOP("Examen", "Respuestas correctas: %1"))
                                              .arg(Maquetador::textoCorrectasPorLetra(m)),
                                          normal));
        correctas.antes = twips(2);
        correctas.despues = twips(4);
        bloques << correctas;
        Tabla referencias;
        const int anchos[] = {twips(28), twips(60), twips(22), kAnchoTexto - twips(28 + 60 + 22)};
        for (int i = 0; i < m.preguntas.size(); ++i) {
            const PreguntaExamen &pe = m.preguntas.at(i);
            referencias.filas << QVector<Celda>{
                celda(anchos[0], {parrafo(trozo(QStringLiteral("%1.").arg(i + 1), pequena), QStringLiteral("right"))}),
                celda(anchos[1], {parrafo(trozo(pe.pregunta.codigoCompleto(), pequena))}),
                celda(anchos[2], {parrafo(trozo(QString(Pregunta::letraMayuscula(pe.correcta())), letra(9, true)))}),
                celda(anchos[3], {parrafo(trozo(pe.pregunta.referencia, pequena))})};
        }
        referencias.margenIzquierda = referencias.margenDerecha = referencias.margenVertical = twips(2);
        bloques << referencias;
    }
    doc.anadir(bloques, false);
}

} // namespace

QMap<QString, QByteArray> partes(const QVector<Modelo> &modelos, const DatosConvocatoria &d, bool soluciones, int n)
{
    Documento doc;
    if (soluciones) {
        hojaSoluciones(doc, modelos, d);
        doc.terminarSeccion(textoPie(tx(d, QT_TRANSLATE_NOOP("Examen", "Soluciones")), d), modelos.isEmpty());
    }
    for (int i = 0; i < modelos.size(); ++i) {
        examen(doc, modelos.at(i), d, soluciones, n);
        doc.terminarSeccion(textoPie(modelos.at(i).nombre, d), i + 1 == modelos.size());
    }
    if (modelos.isEmpty() && !soluciones)
        doc.terminarSeccion(QString(), true);
    return doc.partes(d, n);
}

QByteArray examenes(const QVector<Modelo> &modelos, const DatosConvocatoria &d, bool soluciones, int n)
{
    // [Content_Types].xml, el primero; las imágenes, que ya van comprimidas, tal cual.
    const QMap<QString, QByteArray> p = partes(modelos, d, soluciones, n);
    EscritorZip zip;
    const QString tipos = QStringLiteral("[Content_Types].xml");
    zip.anadir(tipos, p.value(tipos));
    for (auto it = p.cbegin(); it != p.cend(); ++it)
        if (it.key() != tipos)
            zip.anadir(it.key(), it.value(), !it.key().endsWith(QLatin1String(".png")));
    return zip.terminar();
}

} // namespace Docx
