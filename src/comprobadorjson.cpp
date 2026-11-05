#include "comprobadorjson.h"
#include "idioma.h"

#include <QCoreApplication>
#include <QFile>
#include <QHash>
#include <QImage>
#include <QRegularExpression>
#include <QStringDecoder>
#include <vector>

namespace ComprobadorJson {

namespace {

// Textos traducibles, todos en el contexto «ComprobadorJson»: Texto::tr() en las clases, Texto::tr() en las funciones.
struct Texto
{
    Q_DECLARE_TR_FUNCTIONS(ComprobadorJson)
};

// ---------------------------------------------------------------------------------------------
// Lectura del JSON, con la línea y la columna de cada valor (QJsonDocument no las da).

struct Valor
{
    enum Tipo { Nulo, Booleano, Numero, Texto, Lista, Objeto };
    Tipo tipo = Nulo;
    int linea = 0;
    int columna = 0;
    bool booleano = false;
    double numero = 0;
    bool entero = false;           // número sin parte decimal ni exponente
    QString texto;                 // de un texto; del número, tal como está escrito
    std::vector<Valor> hijos;      // elementos de la lista, o valores de los campos del objeto
    QStringList claves;            // nombres de los campos del objeto, en el orden de 'hijos'

    const Valor *campo(const QString &clave) const
    {
        const int i = claves.lastIndexOf(clave);
        return i < 0 ? nullptr : &hijos[size_t(i)];
    }
};

// Un campo repetido en un objeto: con QJsonDocument (y al importar) solo valdría el último.
struct Repetido
{
    int linea, columna, lineaAnterior;
    QString clave;
};

class Lector
{
    Q_DECLARE_TR_FUNCTIONS(ComprobadorJson)

public:
    explicit Lector(const QString &texto)
        : m_s(texto)
    {
    }

    bool leer(Valor &raiz)
    {
        saltarEspacios();
        if (fin())
            return fallar(tr("El fichero está vacío."));
        if (!valor(raiz, 0))
            return false;
        saltarEspacios();
        if (!fin())
            return fallar(tr("Sobra texto después del final del JSON: ¿hay una llave «}» de más, o dos objetos seguidos?"));
        return true;
    }

    QString error;
    int lineaError = 0;
    int columnaError = 0;
    QVector<Repetido> repetidos;

private:
    const QString &m_s;
    qsizetype m_i = 0;
    int m_linea = 1;
    int m_columna = 1;

    bool fin() const { return m_i >= m_s.size(); }
    QChar actual() const { return fin() ? QChar() : m_s.at(m_i); }
    void avanzar()
    {
        if (m_s.at(m_i) == QLatin1Char('\n')) {
            ++m_linea;
            m_columna = 1;
        } else {
            ++m_columna;
        }
        ++m_i;
    }
    void saltarEspacios()
    {
        while (!fin() && (actual() == QLatin1Char(' ') || actual() == QLatin1Char('\t') || actual() == QLatin1Char('\n')
                          || actual() == QLatin1Char('\r')))
            avanzar();
    }
    bool fallar(const QString &texto) { return fallarEn(m_linea, m_columna, texto); }
    bool fallarEn(int linea, int columna, const QString &texto)
    {
        if (error.isEmpty()) {
            error = texto;
            lineaError = linea;
            columnaError = columna;
        }
        return false;
    }
    // La palabra que empieza aquí (para citarla en el error).
    QString palabra() const
    {
        qsizetype j = m_i;
        while (j < m_s.size() && j - m_i < 30 && (m_s.at(j).isLetterOrNumber() || m_s.at(j) == QLatin1Char('_')))
            ++j;
        return m_s.mid(m_i, qMax<qsizetype>(1, j - m_i));
    }
    // Errores en el sitio de un valor que no empieza bien.
    bool valorNoValido()
    {
        const QChar c = actual();
        if (c == QLatin1Char('\''))
            return fallar(tr("Los textos y los nombres de los campos van entre comillas dobles (\"), no simples (')."));
        if (c == QChar(0x201C) || c == QChar(0x201D) || c == QChar(0x00AB) || c == QChar(0x00BB))
            return fallar(tr("Los textos van entre comillas rectas (\"), no tipográficas (“ ” « »): suele pasar al copiar "
                             "el JSON de un procesador de textos."));
        if (c == QLatin1Char('/'))
            return fallar(tr("En JSON no se pueden poner comentarios."));
        if (c == QLatin1Char(','))
            return fallar(tr("Sobra una coma, o falta un valor antes de ella."));
        if (c == QLatin1Char('}') || c == QLatin1Char(']'))
            return fallar(tr("Falta un valor antes de «%1».").arg(c));
        return fallar(tr("«%1» no es un valor válido: los textos van entre comillas dobles, y los demás valores son números, "
                         "true, false, null, listas [ … ] u objetos { … }.").arg(palabra()));
    }

    bool valor(Valor &v, int profundidad)
    {
        saltarEspacios();
        v.linea = m_linea;
        v.columna = m_columna;
        if (fin())
            return fallar(tr("El fichero se acaba antes de tiempo: falta un valor."));
        if (profundidad > 100)
            return fallar(tr("Hay demasiados niveles de { } o [ ] uno dentro de otro."));
        const QChar c = actual();
        if (c == QLatin1Char('{'))
            return objeto(v, profundidad);
        if (c == QLatin1Char('['))
            return lista(v, profundidad);
        if (c == QLatin1Char('"')) {
            v.tipo = Valor::Texto;
            return cadena(v.texto);
        }
        if (c == QLatin1Char('-') || c.isDigit())
            return numero(v);
        for (const auto &[literal, tipo, booleano] : {std::tuple{"true", Valor::Booleano, true},
                                                      std::tuple{"false", Valor::Booleano, false},
                                                      std::tuple{"null", Valor::Nulo, false}}) {
            if (palabra() == QLatin1String(literal)) {
                v.tipo = tipo;
                v.booleano = booleano;
                for (size_t k = 0; k < qstrlen(literal); ++k)
                    avanzar();
                return true;
            }
        }
        if (palabra().compare(QLatin1String("true"), Qt::CaseInsensitive) == 0
            || palabra().compare(QLatin1String("false"), Qt::CaseInsensitive) == 0
            || palabra().compare(QLatin1String("null"), Qt::CaseInsensitive) == 0)
            return fallar(tr("«%1» se escribe en minúsculas: true, false o null.").arg(palabra()));
        return valorNoValido();
    }

    bool objeto(Valor &v, int profundidad)
    {
        v.tipo = Valor::Objeto;
        avanzar();   // {
        saltarEspacios();
        if (actual() == QLatin1Char('}')) {
            avanzar();
            return true;
        }
        QHash<QString, int> lineaDe;
        while (true) {
            saltarEspacios();
            if (fin())
                return fallar(tr("Falta un «}»: el objeto que empieza en la línea %1 no se cierra.").arg(v.linea));
            if (actual() != QLatin1Char('"')) {
                if (actual() == QLatin1Char('}') && !v.claves.isEmpty())
                    return fallar(tr("Sobra la coma antes de «}»."));
                if (actual() == QLatin1Char('\'') || actual() == QChar(0x201C) || actual() == QChar(0x00AB) || actual() == QLatin1Char('/'))
                    return valorNoValido();
                if (actual().isLetter())
                    return fallar(tr("El nombre del campo va entre comillas dobles: \"%1\".").arg(palabra()));
                return fallar(tr("Aquí tendría que ir el nombre de un campo, entre comillas dobles."));
            }
            const int lineaClave = m_linea, columnaClave = m_columna;
            QString clave;
            if (!cadena(clave))
                return false;
            saltarEspacios();
            if (actual() != QLatin1Char(':'))
                return fallar(tr("Falta «:» después de \"%1\".").arg(clave));
            avanzar();
            v.hijos.emplace_back();
            if (!valor(v.hijos.back(), profundidad + 1))
                return false;
            if (lineaDe.contains(clave))
                repetidos.append({lineaClave, columnaClave, lineaDe.value(clave), clave});
            lineaDe.insert(clave, lineaClave);
            v.claves << clave;
            saltarEspacios();
            if (fin())
                return fallar(tr("Falta un «}»: el objeto que empieza en la línea %1 no se cierra.").arg(v.linea));
            if (actual() == QLatin1Char(',')) {
                avanzar();
                continue;
            }
            if (actual() == QLatin1Char('}')) {
                avanzar();
                return true;
            }
            if (actual() == QLatin1Char('"'))
                return fallar(tr("Falta una coma antes de este campo."));
            if (actual() == QLatin1Char(']'))
                return fallar(tr("Aquí se cierra con «]» algo que se abrió con «{» en la línea %1.").arg(v.linea));
            return fallar(tr("Después del valor de \"%1\" tiene que ir una coma o «}».").arg(clave));
        }
    }

    bool lista(Valor &v, int profundidad)
    {
        v.tipo = Valor::Lista;
        avanzar();   // [
        saltarEspacios();
        if (actual() == QLatin1Char(']')) {
            avanzar();
            return true;
        }
        while (true) {
            saltarEspacios();
            if (actual() == QLatin1Char(']') && !v.hijos.empty())
                return fallar(tr("Sobra la coma antes de «]»."));
            v.hijos.emplace_back();
            if (!valor(v.hijos.back(), profundidad + 1))
                return false;
            saltarEspacios();
            if (fin())
                return fallar(tr("Falta un «]»: la lista que empieza en la línea %1 no se cierra.").arg(v.linea));
            if (actual() == QLatin1Char(',')) {
                avanzar();
                continue;
            }
            if (actual() == QLatin1Char(']')) {
                avanzar();
                return true;
            }
            if (actual() == QLatin1Char('}'))
                return fallar(tr("Aquí se cierra con «}» algo que se abrió con «[» en la línea %1.").arg(v.linea));
            return fallar(tr("Falta una coma entre dos elementos de la lista."));
        }
    }

    bool cadena(QString &texto)
    {
        const int linea = m_linea, columna = m_columna;
        avanzar();   // "
        while (true) {
            if (fin())
                return fallarEn(linea, columna, tr("Falta la comilla que cierra el texto que empieza aquí."));
            const QChar c = actual();
            if (c == QLatin1Char('"')) {
                avanzar();
                return true;
            }
            if (c == QLatin1Char('\n') || c == QLatin1Char('\r'))
                return fallar(tr("Salto de línea dentro de un texto: en JSON se escribe \\n. (O falta la comilla que "
                                 "cierra el texto que empieza en la línea %1, columna %2.)").arg(linea).arg(columna));
            if (c.unicode() < 0x20)
                return fallar(tr("Carácter de control (código %1) dentro de un texto: en JSON no se permite; un tabulador "
                                 "se escribe \\t.").arg(c.unicode()));
            if (c != QLatin1Char('\\')) {
                texto += c;
                avanzar();
                continue;
            }
            const int lineaBarra = m_linea, columnaBarra = m_columna;
            avanzar();   // barra
            if (fin())
                return fallarEn(linea, columna, tr("Falta la comilla que cierra el texto que empieza aquí."));
            const QChar e = actual();
            static const QHash<QChar, QChar> escapes{{QLatin1Char('"'), QLatin1Char('"')}, {QLatin1Char('\\'), QLatin1Char('\\')},
                                                     {QLatin1Char('/'), QLatin1Char('/')},   {QLatin1Char('b'), QLatin1Char('\b')},
                                                     {QLatin1Char('f'), QLatin1Char('\f')},  {QLatin1Char('n'), QLatin1Char('\n')},
                                                     {QLatin1Char('r'), QLatin1Char('\r')},  {QLatin1Char('t'), QLatin1Char('\t')}};
            if (escapes.contains(e)) {
                texto += escapes.value(e);
                avanzar();
            } else if (e == QLatin1Char('u')) {
                avanzar();
                const QString hex = m_s.mid(m_i, 4);
                bool ok = hex.size() == 4;
                const uint codigo = ok ? hex.toUInt(&ok, 16) : 0;
                if (!ok)
                    return fallarEn(lineaBarra, columnaBarra, tr("Después de \\u van cuatro cifras hexadecimales (\\u00e1)."));
                texto += QChar(char16_t(codigo));
                for (int k = 0; k < 4; ++k)
                    avanzar();
            } else {
                return fallarEn(lineaBarra, columnaBarra, tr("«\\%1» no vale dentro de un texto: una barra invertida se escribe \\\\, y un salto de "
                                 "línea, \\n.").arg(e));
            }
        }
    }

    bool numero(Valor &v)
    {
        static const QRegularExpression re(QStringLiteral("-?(0|[1-9][0-9]*)(\\.[0-9]+)?([eE][+-]?[0-9]+)?"));
        qsizetype j = m_i;
        while (j < m_s.size() && (m_s.at(j).isDigit() || QStringLiteral("+-.eE").contains(m_s.at(j))))
            ++j;
        const QString escrito = m_s.mid(m_i, j - m_i);
        const auto m = re.match(escrito, 0, QRegularExpression::NormalMatch, QRegularExpression::AnchorAtOffsetMatchOption);
        if (!m.hasMatch() || m.capturedLength() != escrito.size())
            return fallar(tr("Número mal escrito: «%1».").arg(escrito));
        v.tipo = Valor::Numero;
        v.texto = escrito;
        v.numero = escrito.toDouble();
        v.entero = m.captured(2).isEmpty() && m.captured(3).isEmpty();
        for (qsizetype k = 0; k < escrito.size(); ++k)
            avanzar();
        return true;
    }
};

// ---------------------------------------------------------------------------------------------
// Comprobación del contenido

QString tipoDe(const Valor &v)
{
    switch (v.tipo) {
    case Valor::Nulo: return Texto::tr("null");
    case Valor::Booleano: return v.booleano ? Texto::tr("true") : Texto::tr("false");
    case Valor::Numero: return Texto::tr("el número %1").arg(v.texto);
    case Valor::Texto: return Texto::tr("el texto «%1»").arg(v.texto.size() > 40 ? v.texto.left(40) + QStringLiteral("…") : v.texto);
    case Valor::Lista: return Texto::tr("una lista");
    case Valor::Objeto: return Texto::tr("un objeto");
    }
    return QString();
}

// Distancia de edición, para sugerir el campo que se quería escribir.
int distancia(const QString &a, const QString &b)
{
    QVector<int> fila(b.size() + 1);
    for (int j = 0; j <= b.size(); ++j)
        fila[j] = j;
    for (int i = 1; i <= a.size(); ++i) {
        int diagonal = fila[0];
        fila[0] = i;
        for (int j = 1; j <= b.size(); ++j) {
            const int arriba = fila[j];
            fila[j] = qMin(qMin(fila[j] + 1, fila[j - 1] + 1), diagonal + (a.at(i - 1).toLower() == b.at(j - 1).toLower() ? 0 : 1));
            diagonal = arriba;
        }
    }
    return fila[b.size()];
}

class Comprobacion
{
    Q_DECLARE_TR_FUNCTIONS(ComprobadorJson)

public:
    Resultado r;
    QSet<QString> gruposDelBanco;

    void comprobarRaiz(const Valor &raiz)
    {
        if (raiz.tipo != Valor::Objeto) {
            error(raiz, QString(), tr("El JSON tiene que ser un objeto { … } con la lista de temas en «temas», y es %1.").arg(tipoDe(raiz)));
            return;
        }
        desconocidos(raiz, camposRaiz(), QString());
        texto(raiz, QStringLiteral("$schema"), QString(), false);
        texto(raiz, QStringLiteral("asignatura"), QString(), false);
        texto(raiz, QStringLiteral("origen"), QString(), false);
        if (const Valor *idioma = texto(raiz, QStringLiteral("idioma"), QString(), false); idioma && !Idioma::disponible(idioma->texto)) {
            QStringList codigos;
            for (const Idioma::Disponible &d : Idioma::disponibles())
                codigos << QStringLiteral("\"%1\" (%2)").arg(d.codigo, d.nombre);
            aviso(*idioma, QString(), tr("El idioma «%1» no es ninguno de los del programa (%2): los exámenes saldrían en castellano.")
                                          .arg(idioma->texto, codigos.join(QStringLiteral(", "))));
        }
        const Valor *temas = raiz.campo(QStringLiteral("temas"));
        if (!temas) {
            error(raiz, QString(), tr("Falta el campo «temas», la lista de los temas con sus preguntas."));
            return;
        }
        if (temas->tipo != Valor::Lista) {
            error(*temas, QString(), tr("«temas» tiene que ser una lista [ … ] de temas, y es %1.").arg(tipoDe(*temas)));
            return;
        }
        if (temas->hijos.empty())
            aviso(*temas, QString(), tr("La lista de temas está vacía: no hay nada que importar."));
        QHash<int, int> lineaDelTema;
        for (size_t i = 0; i < temas->hijos.size(); ++i)
            comprobarTema(temas->hijos[i], int(i) + 1, lineaDelTema);
        // Las pseudorrepetidas, cuando ya se conocen todas las preguntas del JSON.
        for (const auto &[valor, donde] : std::as_const(m_pseudos))
            if (!m_grupos.contains(valor->texto) && !gruposDelBanco.contains(valor->texto))
                aviso(*valor, donde, gruposDelBanco.isEmpty()
                                         ? tr("Es pseudorrepetida con %1, que no está en este JSON (puede estar ya en el banco).").arg(valor->texto)
                                         : tr("Es pseudorrepetida con %1, que no está ni en este JSON ni en el banco.").arg(valor->texto));
    }

private:
    QSet<QString> m_grupos;                                  // «T1-7» de las preguntas del JSON
    QVector<QPair<const Valor *, QString>> m_pseudos;       // pseudo_con y dónde

    void problema(Problema::Gravedad g, int linea, int columna, const QString &donde, const QString &texto)
    {
        r.problemas.append({g, linea, columna, donde, texto});
    }
    void error(const Valor &v, const QString &donde, const QString &texto) { problema(Problema::Error, v.linea, v.columna, donde, texto); }
    void aviso(const Valor &v, const QString &donde, const QString &texto) { problema(Problema::Aviso, v.linea, v.columna, donde, texto); }

    void desconocidos(const Valor &objeto, const QStringList &conocidos, const QString &donde)
    {
        for (size_t i = 0; i < objeto.hijos.size(); ++i) {
            const QString &clave = objeto.claves.at(int(i));
            if (conocidos.contains(clave))
                continue;
            QString parecido;
            for (const QString &c : conocidos)
                if (distancia(clave, c) <= qMax(1, int(c.size()) / 4) && (parecido.isEmpty() || distancia(clave, c) < distancia(clave, parecido)))
                    parecido = c;
            aviso(objeto.hijos[i], donde,
                  parecido.isEmpty() ? tr("El campo «%1» no es de este formato: no se usa.").arg(clave)
                                     : tr("El campo «%1» no es de este formato: no se usa. ¿Quería decir «%2»?").arg(clave, parecido));
        }
    }

    // Un campo que tiene que ser un texto (o null, que cuenta como vacío). Devuelve el valor si lo es.
    const Valor *texto(const Valor &objeto, const QString &clave, const QString &donde, bool obligatorio)
    {
        const Valor *v = objeto.campo(clave);
        if (!v) {
            if (obligatorio)
                error(objeto, donde, tr("Falta el campo «%1».").arg(clave));
            return nullptr;
        }
        if (v->tipo == Valor::Nulo && !obligatorio)
            return nullptr;
        if (v->tipo != Valor::Texto) {
            error(*v, donde, tr("«%1» tiene que ser un texto, entre comillas dobles, y es %2.").arg(clave, tipoDe(*v)));
            return nullptr;
        }
        return v;
    }

    const Valor *booleano(const Valor &objeto, const QString &clave, const QString &donde)
    {
        const Valor *v = objeto.campo(clave);
        if (v && v->tipo != Valor::Booleano) {
            error(*v, donde, tr("«%1» tiene que ser true o false (sin comillas), y es %2.").arg(clave, tipoDe(*v)));
            return nullptr;
        }
        return v;
    }

    // Número entero mayor que 0, obligatorio. Devuelve 0 si no lo es.
    int numero(const Valor &objeto, const QString &donde)
    {
        const Valor *v = objeto.campo(QStringLiteral("numero"));
        if (!v) {
            error(objeto, donde, tr("Falta el campo «numero»."));
            return 0;
        }
        if (v->tipo != Valor::Numero || !v->entero || v->numero < 1 || v->numero > 1e6) {
            error(*v, donde, tr("«numero» tiene que ser un número entero mayor que 0, sin comillas, y es %1.").arg(tipoDe(*v)));
            return 0;
        }
        return int(v->numero);
    }

    void comprobarTema(const Valor &t, int posicion, QHash<int, int> &lineaDelTema)
    {
        QString donde = tr("tema n.º %1 de la lista").arg(posicion);
        if (t.tipo != Valor::Objeto) {
            error(t, donde, tr("Cada tema tiene que ser un objeto { … }, y este es %1.").arg(tipoDe(t)));
            return;
        }
        ++r.temas;
        const int n = numero(t, donde);
        if (n > 0) {
            donde = tr("tema %1").arg(n);
            if (lineaDelTema.contains(n))
                error(*t.campo(QStringLiteral("numero")), donde, tr("El tema %1 está repetido (el otro, en la línea %2).").arg(n).arg(lineaDelTema.value(n)));
            lineaDelTema.insert(n, t.linea);
        }
        desconocidos(t, camposTema(), donde);
        const Valor *titulo = texto(t, QStringLiteral("titulo"), donde, false);
        if (!t.campo(QStringLiteral("titulo")))
            aviso(t, donde, tr("El tema no tiene «titulo»: si ya está en el banco, conserva el suyo; si no, se queda sin título."));
        else if (titulo && titulo->texto.trimmed().isEmpty())
            aviso(*titulo, donde, tr("El título del tema está vacío."));
        texto(t, QStringLiteral("autor"), donde, false);
        if (const Valor *formato = texto(t, QStringLiteral("formato_referencia"), donde, false)) {
            static const QStringList validos{QString(), QStringLiteral("vease"), QStringLiteral("pag")};
            if (!validos.contains(formato->texto))
                error(*formato, donde, tr("«formato_referencia» tiene que ser \"vease\", \"pag\" o \"\" (cualquiera), y es «%1».").arg(formato->texto));
        }
        const Valor *preguntas = t.campo(QStringLiteral("preguntas"));
        if (!preguntas) {
            error(t, donde, tr("Falta el campo «preguntas», la lista de las preguntas del tema."));
            return;
        }
        if (preguntas->tipo != Valor::Lista) {
            error(*preguntas, donde, tr("«preguntas» tiene que ser una lista [ … ], y es %1.").arg(tipoDe(*preguntas)));
            return;
        }
        if (preguntas->hijos.empty())
            aviso(*preguntas, donde, tr("El tema no tiene preguntas."));
        QHash<QString, int> lineaDelCodigo;
        for (size_t i = 0; i < preguntas->hijos.size(); ++i)
            comprobarPregunta(preguntas->hijos[i], n, int(i) + 1, lineaDelCodigo);
    }

    void comprobarPregunta(const Valor &p, int tema, int posicion, QHash<QString, int> &lineaDelCodigo)
    {
        const QString temaTexto = tema > 0 ? QString::number(tema) : QStringLiteral("?");
        QString donde = tr("tema %1, pregunta n.º %2 de la lista").arg(temaTexto).arg(posicion);
        if (p.tipo != Valor::Objeto) {
            error(p, donde, tr("Cada pregunta tiene que ser un objeto { … }, y esta es %1.").arg(tipoDe(p)));
            return;
        }
        ++r.preguntas;
        const int n = numero(p, donde);
        QString variante;
        if (const Valor *v = texto(p, QStringLiteral("variante"), donde, false)) {
            static const QRegularExpression letra(QStringLiteral("^[a-z]?$"));
            if (letra.match(v->texto).hasMatch())
                variante = v->texto;
            else
                error(*v, donde, tr("«variante» tiene que ser una letra minúscula (\"a\", \"b\"…) o \"\" si no tiene variantes, y es «%1».").arg(v->texto));
        }
        if (n > 0) {
            const QString codigo = QString::number(n) + variante;
            donde = tr("tema %1, pregunta %2").arg(temaTexto, codigo);
            if (lineaDelCodigo.contains(codigo))
                error(p, donde, tr("La pregunta %1 está repetida en el tema (la otra, en la línea %2).").arg(codigo).arg(lineaDelCodigo.value(codigo)));
            lineaDelCodigo.insert(codigo, p.linea);
            if (tema > 0)
                m_grupos.insert(QStringLiteral("T%1-%2").arg(tema).arg(n));
            if (const Valor *c = texto(p, QStringLiteral("codigo"), donde, false); c && c->texto != codigo)
                aviso(*c, donde, tr("«codigo» dice «%1», pero el número y la variante dan %2: al importar cuentan estos.").arg(c->texto, codigo));
        } else {
            texto(p, QStringLiteral("codigo"), donde, false);
        }
        desconocidos(p, camposPregunta(), donde);
        texto(p, QStringLiteral("etiqueta_original"), donde, false);
        booleano(p, QStringLiteral("old"), donde);
        booleano(p, QStringLiteral("revisar"), donde);
        if (const Valor *e = texto(p, QStringLiteral("estado"), donde, false)) {
            static const QStringList estados{QStringLiteral("activa"), QStringLiteral("anulada"), QStringLiteral("obsoleta")};
            if (!estados.contains(e->texto))
                error(*e, donde, tr("«estado» tiene que ser \"activa\", \"anulada\" u \"obsoleta\", y es «%1».").arg(e->texto));
        }
        if (const Valor *e = texto(p, QStringLiteral("enunciado"), donde, true); e && e->texto.trimmed().isEmpty())
            error(*e, donde, tr("El enunciado está vacío."));
        comprobarOpciones(p, donde);
        comprobarCorrecta(p, donde);
        texto(p, QStringLiteral("referencia"), donde, false);
        texto(p, QStringLiteral("explicacion"), donde, false);
        texto(p, QStringLiteral("notas"), donde, false);
        if (const Valor *s = texto(p, QStringLiteral("pseudo_con"), donde, false); s && !s->texto.isEmpty()) {
            static const QRegularExpression grupo(QStringLiteral("^T\\d+-\\d+$"));
            if (!grupo.match(s->texto).hasMatch())
                error(*s, donde, tr("«pseudo_con» tiene que ser el código de otra pregunta sin la letra de la variante, como "
                                    "\"T1-7\" (tema 1, número 7), y es «%1».").arg(s->texto));
            else
                m_pseudos.append({s, donde});
        }
        if (const Valor *img = texto(p, QStringLiteral("imagen"), donde, false); img && !img->texto.isEmpty()) {
            const auto datos = QByteArray::fromBase64Encoding(img->texto.toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
            if (!datos || QImage::fromData(*datos).isNull())
                error(*img, donde, tr("«imagen» no se puede leer: tiene que ser una imagen PNG o JPG codificada en base64 "
                                      "(o null si no tiene)."));
        }
    }

    void comprobarOpciones(const Valor &p, const QString &donde)
    {
        const Valor *ops = p.campo(QStringLiteral("opciones"));
        if (!ops) {
            error(p, donde, tr("Falta el campo «opciones», la lista de las cuatro opciones."));
            return;
        }
        if (ops->tipo != Valor::Lista) {
            error(*ops, donde, tr("«opciones» tiene que ser una lista [ … ] de cuatro textos, y es %1.").arg(tipoDe(*ops)));
            return;
        }
        const int n = int(ops->hijos.size());
        if (n == 0)
            error(*ops, donde, tr("La lista de opciones está vacía: tienen que ser cuatro (a, b, c y d)."));
        else if (n == 1)
            error(*ops, donde, tr("Tiene una opción, y tienen que ser cuatro (a, b, c y d)."));
        else if (n != 4)
            error(*ops, donde, tr("Tiene %1 opciones, y tienen que ser cuatro (a, b, c y d).").arg(n));
        for (int i = 0; i < qMin(n, 4); ++i) {
            const Valor &o = ops->hijos[size_t(i)];
            const QChar letra = QChar(u'a' + i);
            if (o.tipo != Valor::Texto)
                error(o, donde, tr("La opción %1) tiene que ser un texto, entre comillas dobles, y es %2.").arg(letra).arg(tipoDe(o)));
            else if (o.texto.trimmed().isEmpty())
                error(o, donde, tr("La opción %1) está vacía.").arg(letra));
            else
                for (int j = 0; j < i; ++j) {
                    const Valor &otra = ops->hijos[size_t(j)];
                    if (otra.tipo == Valor::Texto && otra.texto.trimmed().compare(o.texto.trimmed(), Qt::CaseInsensitive) == 0)
                        aviso(o, donde, tr("Las opciones %1) y %2) son iguales.").arg(QChar(u'a' + j)).arg(letra));
                }
        }
    }

    void comprobarCorrecta(const Valor &p, const QString &donde)
    {
        const Valor *c = p.campo(QStringLiteral("correcta"));
        if (!c) {
            error(p, donde, tr("Falta el campo «correcta», la letra de la respuesta correcta (\"A\", \"B\", \"C\" o \"D\")."));
            return;
        }
        static const QRegularExpression letra(QStringLiteral("^[A-Da-d]$"));
        if (c->tipo != Valor::Texto || !letra.match(c->texto).hasMatch())
            error(*c, donde, tr("«correcta» tiene que ser la letra de la respuesta correcta, entre comillas: \"A\", \"B\", \"C\" "
                                "o \"D\", y es %1.").arg(tipoDe(*c)));
    }
};

QString cantidad(int n, const QString &uno, const QString &varios)
{
    return n == 1 ? uno : varios.arg(n);
}

} // namespace

int Resultado::errores() const
{
    int n = 0;
    for (const Problema &p : problemas)
        n += p.gravedad == Problema::Error;
    return n;
}

int Resultado::avisos() const
{
    return int(problemas.size()) - errores();
}

QStringList camposRaiz()
{
    return {QStringLiteral("$schema"), QStringLiteral("asignatura"), QStringLiteral("idioma"), QStringLiteral("origen"),
            QStringLiteral("temas")};
}

QStringList camposTema()
{
    return {QStringLiteral("numero"), QStringLiteral("titulo"), QStringLiteral("autor"), QStringLiteral("formato_referencia"),
            QStringLiteral("preguntas")};
}

QStringList camposPregunta()
{
    return {QStringLiteral("numero"),     QStringLiteral("variante"),  QStringLiteral("codigo"),
            QStringLiteral("etiqueta_original"), QStringLiteral("old"), QStringLiteral("estado"),
            QStringLiteral("revisar"),    QStringLiteral("enunciado"), QStringLiteral("opciones"),
            QStringLiteral("correcta"),   QStringLiteral("referencia"), QStringLiteral("explicacion"),
            QStringLiteral("notas"),      QStringLiteral("pseudo_con"), QStringLiteral("imagen")};
}

Resultado comprobar(const QByteArray &json, const QSet<QString> &gruposDelBanco)
{
    Resultado r;
    QStringDecoder utf8(QStringDecoder::Utf8);
    QString texto = utf8.decode(json);
    if (utf8.hasError()) {
        r.problemas.append({Problema::Error, 0, 0, QString(),
                            Texto::tr("El fichero no está en UTF-8: guárdelo con esa codificación (en el Bloc de notas de Windows, "
                               "«Guardar como» → Codificación: UTF-8).")});
        return r;
    }
    if (texto.startsWith(QChar(0xFEFF)))
        texto.remove(0, 1);
    Lector lector(texto);
    Valor raiz;
    if (!lector.leer(raiz)) {
        r.problemas.append({Problema::Error, lector.lineaError, lector.columnaError, QString(), lector.error});
        return r;
    }
    r.leido = true;
    Comprobacion c;
    c.gruposDelBanco = gruposDelBanco;
    for (const Repetido &rep : std::as_const(lector.repetidos))
        c.r.problemas.append({Problema::Error, rep.linea, rep.columna, QString(),
                              Texto::tr("El campo «%1» está dos veces en el mismo objeto (la otra, en la línea %2): solo valdría "
                                 "el último.").arg(rep.clave).arg(rep.lineaAnterior)});
    c.comprobarRaiz(raiz);
    r.problemas = c.r.problemas;
    r.temas = c.r.temas;
    r.preguntas = c.r.preguntas;
    std::stable_sort(r.problemas.begin(), r.problemas.end(), [](const Problema &a, const Problema &b) {
        return a.linea != b.linea ? a.linea < b.linea : a.columna < b.columna;
    });
    return r;
}

Resultado comprobarFichero(const QString &ruta, const QSet<QString> &gruposDelBanco)
{
    QFile f(ruta);
    if (!f.open(QIODevice::ReadOnly)) {
        Resultado r;
        r.problemas.append({Problema::Error, 0, 0, QString(), Texto::tr("No se puede abrir el fichero: %1").arg(f.errorString())});
        return r;
    }
    return comprobar(f.readAll(), gruposDelBanco);
}

QString nombreGravedad(Problema::Gravedad g)
{
    return g == Problema::Error ? Texto::tr("error") : Texto::tr("aviso");
}

QString resumen(const Resultado &r)
{
    if (!r.leido && r.problemas.size() == 1)
        return Texto::tr("El JSON no se puede leer.");
    const QString contenido = Texto::tr("%1 en %2").arg(cantidad(r.preguntas, Texto::tr("1 pregunta"), Texto::tr("%1 preguntas")),
                                                  cantidad(r.temas, Texto::tr("1 tema"), Texto::tr("%1 temas")));
    if (r.problemas.isEmpty())
        return Texto::tr("Sin errores ni avisos: %1.").arg(contenido);
    QStringList partes;
    if (r.errores() > 0)
        partes << cantidad(r.errores(), Texto::tr("1 error"), Texto::tr("%1 errores"));
    if (r.avisos() > 0)
        partes << cantidad(r.avisos(), Texto::tr("1 aviso"), Texto::tr("%1 avisos"));
    return Texto::tr("%1 (%2).").arg(partes.join(Texto::tr(" y ")), contenido);
}

QString informe(const Resultado &r, const QString &ruta)
{
    QStringList lineas;
    for (const Problema &p : r.problemas) {
        QString linea = ruta;
        if (p.linea > 0)
            linea += QStringLiteral(":%1:%2").arg(p.linea).arg(p.columna);
        linea += QStringLiteral(": ") + nombreGravedad(p.gravedad) + QStringLiteral(": ");
        if (!p.donde.isEmpty())
            linea += p.donde + QStringLiteral(": ");
        lineas << linea + p.texto;
    }
    lineas << resumen(r);
    return lineas.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

} // namespace ComprobadorJson
