#include "examenesanteriores.h"
#include "maquetador.h"
#include "parecidas.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <algorithm>

#ifdef LEER_PDF
#include <QPdfDocument>
#include <QPdfSelection>
#endif

namespace ExamenesAnteriores {

namespace {

// Comienzo de una pregunta en el texto: dónde empieza la línea, dónde acaba su número («7.-») y el número.
struct Inicio
{
    qsizetype linea;
    qsizetype finNumero;
    int numero;
};

QVector<Inicio> inicios(const QString &texto, const QRegularExpression &re, qsizetype desde = 0, qsizetype hasta = -1)
{
    QVector<Inicio> res;
    for (auto it = re.globalMatch(texto, desde); it.hasNext();) {
        const QRegularExpressionMatch m = it.next();
        if (hasta >= 0 && m.capturedStart() >= hasta)
            break;
        res.append({m.capturedStart(), m.capturedEnd(), m.captured(1).toInt()});
    }
    return res;
}

// De los comienzos posibles, los que siguen la numeración: el primero, el 1; cada uno, con un número algo mayor que
// el anterior (puede faltar alguno, que se busca después).
QVector<Inicio> enOrden(const QVector<Inicio> &candidatos)
{
    QVector<Inicio> res;
    for (const Inicio &i : candidatos)
        if (res.isEmpty() ? i.numero == 1 : i.numero > res.last().numero && i.numero <= res.last().numero + 3)
            res << i;
    return res;
}

// Enunciado y opciones de una pregunta (el texto que sigue a su número). Las opciones empiezan por «a)»...«d)», en
// una columna (a b c d) o en dos (a c / b d): cada una llega hasta la siguiente marca, sea de la letra que sea.
Leida leerPregunta(const QString &bloque)
{
    static const QRegularExpression marca(QStringLiteral("(?:^|(?<=\\s))([a-dA-D])\\)[ \\t]*"));
    struct Marca
    {
        int letra;
        qsizetype inicio, fin;
    };
    QVector<Marca> marcas;
    for (auto it = marca.globalMatch(bloque); it.hasNext();) {
        const QRegularExpressionMatch m = it.next();
        const int letra = m.captured(1).toLower().at(0).unicode() - 'a';
        // La primera «a)» abre las opciones; de las demás letras, la primera que haya después.
        if ((marcas.isEmpty() && letra == 0)
            || (!marcas.isEmpty() && std::none_of(marcas.cbegin(), marcas.cend(), [letra](const Marca &x) { return x.letra == letra; })))
            marcas.append({letra, m.capturedStart(), m.capturedEnd()});
    }
    Leida l;
    l.enunciado = (marcas.isEmpty() ? bloque : bloque.left(marcas.first().inicio)).simplified();
    std::sort(marcas.begin(), marcas.end(), [](const Marca &a, const Marca &b) { return a.inicio < b.inicio; });
    for (int i = 0; i < marcas.size(); ++i) {
        const qsizetype fin = i + 1 < marcas.size() ? marcas.at(i + 1).inicio : bloque.size();
        l.opciones[marcas.at(i).letra] = bloque.mid(marcas.at(i).fin, fin - marcas.at(i).fin).simplified();
    }
    return l;
}

// Para comparar nombres de examen: sin tildes, mayúsculas ni signos.
QString comparable(const QString &s)
{
    QString t = s.toLower().normalized(QString::NormalizationForm_D);
    static const QRegularExpression marcas(QStringLiteral("\\p{Mn}"));
    static const QRegularExpression signos(QStringLiteral("[^a-z0-9]+"));
    t.remove(marcas);
    t.replace(signos, QStringLiteral(" "));
    return t.trimmed();
}

} // namespace

QVector<Leida> separar(const QString &original)
{
    QString texto = original;
    texto.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    for (QChar &c : texto) {
        if (c == QLatin1Char('\r') || c == QChar::ParagraphSeparator || c == QChar::LineSeparator)
            c = QLatin1Char('\n');
        else if (c == QChar::Nbsp)
            c = QLatin1Char(' ');
    }
    // Los pies de página («Nacional Original · Febrero 2026: 1 / 2»), que si no se pegan a la última opción.
    static const QRegularExpression pie(QStringLiteral("^[^\\n]*:[ \\t]*\\d+[ \\t]*/[ \\t]*\\d+[ \\t]*$"),
                                        QRegularExpression::MultilineOption);
    texto.remove(pie);

    // Primero «7.-», que es como se numeran; si no, «7.» o «7)».
    static const QRegularExpression conGuion(QStringLiteral("^[ \\t]*(\\d{1,3})[ \\t]*\\.[ \\t]*-[ \\t]*"),
                                             QRegularExpression::MultilineOption);
    static const QRegularExpression sinGuion(QStringLiteral("^[ \\t]*(\\d{1,3})[ \\t]*[.)][ \\t]+"),
                                             QRegularExpression::MultilineOption);
    QVector<Inicio> preguntas = enOrden(inicios(texto, conGuion));
    if (preguntas.size() < 2) {
        const QVector<Inicio> otras = enOrden(inicios(texto, sinGuion));
        if (otras.size() > preguntas.size())
            preguntas = otras;
    }
    // Los números que faltan entre dos encontrados, aunque no lleven puntuación: «7 Texto…».
    for (int k = 0; k + 1 < preguntas.size(); ++k) {
        const Inicio a = preguntas.at(k), b = preguntas.at(k + 1);
        qsizetype desde = a.finNumero;
        QVector<Inicio> encontrados;
        for (int n = a.numero + 1; n < b.numero; ++n) {
            const QRegularExpression suelto(QStringLiteral("^[ \\t]*(%1)(?!\\d)[ \\t]*[.)\\-]*[ \\t]+").arg(n),
                                            QRegularExpression::MultilineOption);
            const QVector<Inicio> i = inicios(texto, suelto, desde, b.linea);
            if (i.isEmpty())
                break;
            encontrados << i.first();
            desde = i.first().finNumero;
        }
        for (int j = 0; j < encontrados.size(); ++j)
            preguntas.insert(k + 1 + j, encontrados.at(j));
        k += encontrados.size();
    }

    QVector<Leida> res;
    for (int k = 0; k < preguntas.size(); ++k) {
        const qsizetype fin = k + 1 < preguntas.size() ? preguntas.at(k + 1).linea : texto.size();
        res << leerPregunta(texto.mid(preguntas.at(k).finNumero, fin - preguntas.at(k).finNumero));
    }
    return res;
}

std::array<int, 4> ordenDeOpciones(const Leida &leida, const Pregunta &p)
{
    double parecido[4][4];
    int leidas = 0;
    for (int i = 0; i < 4; ++i) {
        leidas += leida.opciones.value(i).isEmpty() ? 0 : 1;
        for (int j = 0; j < 4; ++j)
            parecido[i][j] = Parecidas::parecidoTextos(leida.opciones.value(i), p.opciones.value(j));
    }
    // De las 24 maneras de emparejarlas, la que más se parece en total (a igualdad, la primera: su orden).
    std::array<int, 4> orden{{0, 1, 2, 3}}, mejor = orden;
    double maximo = -1;
    do {
        double suma = 0;
        for (int i = 0; i < 4; ++i)
            suma += parecido[i][orden[i]];
        if (suma > maximo + 1e-9) {
            maximo = suma;
            mejor = orden;
        }
    } while (std::next_permutation(orden.begin(), orden.end()));
    // Si las opciones no se han podido leer bien, no se sabe el orden.
    return leidas > 0 && maximo >= 0.5 * leidas ? mejor : std::array<int, 4>{{0, 1, 2, 3}};
}

QVector<Reconocida> reconocer(const QVector<Leida> &leidas, const QVector<Pregunta> &banco)
{
    const Parecidas::Indice indice(banco);
    QVector<Reconocida> res;
    for (const Leida &l : leidas) {
        Reconocida r;
        r.leida = l;
        Pregunta buscada;
        buscada.enunciado = l.enunciado;
        buscada.opciones = l.opciones;
        const QVector<Parecidas::Parecida> parecidas = indice.masParecidas(buscada, 20, 0.0);
        if (!parecidas.isEmpty()) {
            const Parecidas::Parecida &mejor = parecidas.first();
            r.pregunta.pregunta = mejor.pregunta;
            r.pregunta.orden = ordenDeOpciones(l, mejor.pregunta);
            r.parecido = mejor.parecido;
            // Si otra pregunta de otro número se parece casi lo mismo, puede ser esa (sus variantes no importan:
            // se evitan juntas).
            for (const Parecidas::Parecida &p : parecidas) {
                if (p.pregunta.grupo() != mejor.pregunta.grupo()) {
                    r.otra = p.pregunta.codigoCompleto();
                    r.parecidoOtra = p.parecido;
                    break;
                }
            }
            r.dudosa = r.parecido < kMinimoReconocida || r.parecido - r.parecidoOtra < kVentajaMinima;
        }
        res << r;
    }
    return res;
}

DatosFichero datosDeFichero(const QString &ruta, const QStringList &conocidos)
{
    DatosFichero d;
    static const QRegularExpression separadores(QStringLiteral("[_\\s]+"));
    QStringList partes = QFileInfo(ruta).completeBaseName().split(separadores, Qt::SkipEmptyParts);
    static const QRegularExpression anio(QStringLiteral("^(19|20)\\d\\d$"));
    static const QRegularExpression numero(QStringLiteral("^\\d{1,2}$"));
    if (!partes.isEmpty() && anio.match(partes.first()).hasMatch())
        d.anio = partes.takeFirst().toInt();
    // El mes, en número («02») o con su nombre («Febrero»).
    if (!partes.isEmpty()) {
        const QString p = partes.first();
        const int mes = numero.match(p).hasMatch() ? p.toInt() : DatosConvocatoria::mesDe(p);
        const QString nombreMes = DatosConvocatoria::nombreDeMes(mes);
        if (!nombreMes.isEmpty() && (numero.match(p).hasMatch() || p.compare(nombreMes, Qt::CaseInsensitive) == 0)) {
            d.convocatoria = nombreMes;
            partes.removeFirst();
        }
    }
    if (!partes.isEmpty() && numero.match(partes.first()).hasMatch())
        partes.removeFirst();   // número de orden del examen: «01»
    // Lo que no es del nombre: «ex_fi» (examen de FI) y lo que añade el programa al exportar.
    static const QStringList sobran{QStringLiteral("ex"), QStringLiteral("fi"), QStringLiteral("soluciones"),
                                    QStringLiteral("letra"), QStringLiteral("grande")};
    partes.removeIf([](const QString &p) { return sobran.contains(p.toLower()); });
    static const QHash<QString, QString> abreviaturas{
        {QStringLiteral("ue"), QStringLiteral("UE")}, {QStringLiteral("ae"), QStringLiteral("América Este")},
        {QStringLiteral("ao"), QStringLiteral("América Oeste")}, {QStringLiteral("ccpp"), QStringLiteral("Centros Penitenciarios")}};
    QStringList palabras;
    for (const QString &p : std::as_const(partes)) {
        if (abreviaturas.contains(p.toLower()))
            palabras << abreviaturas.value(p.toLower());
        else
            palabras << (p == p.toLower() ? p.left(1).toUpper() + p.mid(1) : p);
    }
    d.nombre = palabras.join(QLatin1Char(' '));

    // El de la lista de exámenes que le corresponde: el mismo sin tildes ni signos, o el único que empieza igual.
    const QString buscado = comparable(d.nombre);
    QStringList empiezan;
    for (const QString &c : conocidos) {
        const QString k = comparable(c);
        if (k == buscado) {
            d.nombre = c;
            return d;
        }
        if (!buscado.isEmpty() && k.startsWith(buscado + QLatin1Char(' ')))
            empiezan << c;
    }
    if (empiezan.size() == 1)
        d.nombre = empiezan.first();
    return d;
}

bool puedeLeerPdf()
{
#ifdef LEER_PDF
    return true;
#else
    return false;
#endif
}

QString textoDePdf(const QString &ruta, QString *error)
{
#ifdef LEER_PDF
    QPdfDocument pdf(nullptr);
    if (pdf.load(ruta) != QPdfDocument::Error::None) {
        if (error)
            *error = QCoreApplication::translate("ExamenesAnteriores", "No se pudo leer el PDF.");
        return QString();
    }
    QStringList paginas;
    for (int i = 0; i < pdf.pageCount(); ++i)
        paginas << pdf.getAllText(i).text();
    return paginas.join(QLatin1Char('\n'));
#else
    Q_UNUSED(ruta)
    if (error)
        *error = QCoreApplication::translate("ExamenesAnteriores", "Esta versión del programa no lee PDF: abra el examen, copie su texto y péguelo.");
    return QString();
#endif
}

} // namespace ExamenesAnteriores
