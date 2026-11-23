#include "latex.h"
#include "enunciado.h"
#include "idioma.h"
#include "maqueta.h"
#include "resaltado.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <QStringList>

using namespace Maqueta;

namespace {

QString letraExplicacionLatex(int n)
{
    if (n == Maquetador::kLetraGrande)
        return QString();
    return QLatin1String(nivel(n).letraLatex) == QLatin1String("\\small") ? QStringLiteral("\\footnotesize")
                                                                          : QStringLiteral("\\small");
}

// Preámbulo para pdflatex. \empezarmodelo reinicia la numeración de páginas y cambia el pie, para que cada
// modelo diga «Nacional Original · Junio 2026: página / total» como en el PDF.
const char *const kPreambuloLatex = R"tex(\documentclass[10pt,a4paper]{article}
\usepackage[T1]{fontenc}
\usepackage[utf8]{inputenc}
\usepackage[spanish,es-noshorthands]{babel}
\usepackage{lmodern}
\usepackage{helvet}
\renewcommand{\familydefault}{\sfdefault}
\usepackage[a4paper,left=18mm,right=18mm,top=15mm,bottom=12mm,includefoot,footskip=8mm]{geometry}
\usepackage{xcolor}
\usepackage{graphicx,array,tabularx,listings,fancyhdr,amssymb}
\definecolor{verde}{HTML}{0A7D32}
% Símbolos que pdflatex no reconoce por sí solo (válidos también dentro de los bloques de código).
\DeclareUnicodeCharacter{03BC}{\textmu}
\DeclareUnicodeCharacter{2264}{\ensuremath{\leq}}
\DeclareUnicodeCharacter{2265}{\ensuremath{\geq}}
\DeclareUnicodeCharacter{2260}{\ensuremath{\neq}}
\DeclareUnicodeCharacter{2248}{\ensuremath{\approx}}
\DeclareUnicodeCharacter{2192}{\ensuremath{\rightarrow}}
\DeclareUnicodeCharacter{2190}{\ensuremath{\leftarrow}}
\DeclareUnicodeCharacter{2212}{\ensuremath{-}}
\DeclareUnicodeCharacter{221A}{\ensuremath{\surd}}
\DeclareUnicodeCharacter{221E}{\ensuremath{\infty}}
\DeclareUnicodeCharacter{03B1}{\ensuremath{\alpha}}
\DeclareUnicodeCharacter{03B2}{\ensuremath{\beta}}
\DeclareUnicodeCharacter{03C0}{\ensuremath{\pi}}
\DeclareUnicodeCharacter{03A9}{\ensuremath{\Omega}}
\DeclareUnicodeCharacter{2022}{\textbullet}
\DeclareUnicodeCharacter{2713}{\checkmark}
% Las letras griegas, los símbolos de lógica, de conjuntos y de flechas, y los caracteres invisibles de formato (que no
% se escriben).
\DeclareUnicodeCharacter{03B1}{\ensuremath{\alpha}}\DeclareUnicodeCharacter{03B2}{\ensuremath{\beta}}\DeclareUnicodeCharacter{03B3}{\ensuremath{\gamma}}\DeclareUnicodeCharacter{03B4}{\ensuremath{\delta}}
\DeclareUnicodeCharacter{03B5}{\ensuremath{\varepsilon}}\DeclareUnicodeCharacter{03B6}{\ensuremath{\zeta}}\DeclareUnicodeCharacter{03B7}{\ensuremath{\eta}}\DeclareUnicodeCharacter{03B8}{\ensuremath{\theta}}
\DeclareUnicodeCharacter{03B9}{\ensuremath{\iota}}\DeclareUnicodeCharacter{03BA}{\ensuremath{\kappa}}\DeclareUnicodeCharacter{03BB}{\ensuremath{\lambda}}\DeclareUnicodeCharacter{03BC}{\ensuremath{\mu}}
\DeclareUnicodeCharacter{03BD}{\ensuremath{\nu}}\DeclareUnicodeCharacter{03BE}{\ensuremath{\xi}}\DeclareUnicodeCharacter{03BF}{\ensuremath{o}}\DeclareUnicodeCharacter{03C0}{\ensuremath{\pi}}
\DeclareUnicodeCharacter{03C1}{\ensuremath{\rho}}\DeclareUnicodeCharacter{03C2}{\ensuremath{\varsigma}}\DeclareUnicodeCharacter{03C3}{\ensuremath{\sigma}}\DeclareUnicodeCharacter{03C4}{\ensuremath{\tau}}
\DeclareUnicodeCharacter{03C5}{\ensuremath{\upsilon}}\DeclareUnicodeCharacter{03C6}{\ensuremath{\varphi}}\DeclareUnicodeCharacter{03C7}{\ensuremath{\chi}}\DeclareUnicodeCharacter{03C8}{\ensuremath{\psi}}
\DeclareUnicodeCharacter{03C9}{\ensuremath{\omega}}\DeclareUnicodeCharacter{0391}{\ensuremath{\mathrm{A}}}\DeclareUnicodeCharacter{0392}{\ensuremath{\mathrm{B}}}\DeclareUnicodeCharacter{0393}{\ensuremath{\Gamma}}
\DeclareUnicodeCharacter{0394}{\ensuremath{\Delta}}\DeclareUnicodeCharacter{0395}{\ensuremath{\mathrm{E}}}\DeclareUnicodeCharacter{0396}{\ensuremath{\mathrm{Z}}}\DeclareUnicodeCharacter{0397}{\ensuremath{\mathrm{H}}}
\DeclareUnicodeCharacter{0398}{\ensuremath{\Theta}}\DeclareUnicodeCharacter{0399}{\ensuremath{\mathrm{I}}}\DeclareUnicodeCharacter{039A}{\ensuremath{\mathrm{K}}}\DeclareUnicodeCharacter{039B}{\ensuremath{\Lambda}}
\DeclareUnicodeCharacter{039C}{\ensuremath{\mathrm{M}}}\DeclareUnicodeCharacter{039D}{\ensuremath{\mathrm{N}}}\DeclareUnicodeCharacter{039E}{\ensuremath{\Xi}}\DeclareUnicodeCharacter{039F}{\ensuremath{\mathrm{O}}}
\DeclareUnicodeCharacter{03A0}{\ensuremath{\Pi}}\DeclareUnicodeCharacter{03A1}{\ensuremath{\mathrm{P}}}\DeclareUnicodeCharacter{03A3}{\ensuremath{\Sigma}}\DeclareUnicodeCharacter{03A4}{\ensuremath{\mathrm{T}}}
\DeclareUnicodeCharacter{03A5}{\ensuremath{\Upsilon}}\DeclareUnicodeCharacter{03A6}{\ensuremath{\Phi}}\DeclareUnicodeCharacter{03A7}{\ensuremath{\mathrm{X}}}\DeclareUnicodeCharacter{03A8}{\ensuremath{\Psi}}
\DeclareUnicodeCharacter{03A9}{\ensuremath{\Omega}}
\DeclareUnicodeCharacter{2227}{\ensuremath{\land}}\DeclareUnicodeCharacter{2228}{\ensuremath{\lor}}\DeclareUnicodeCharacter{2295}{\ensuremath{\oplus}}\DeclareUnicodeCharacter{2297}{\ensuremath{\otimes}}
\DeclareUnicodeCharacter{2261}{\ensuremath{\equiv}}\DeclareUnicodeCharacter{2200}{\ensuremath{\forall}}\DeclareUnicodeCharacter{2203}{\ensuremath{\exists}}\DeclareUnicodeCharacter{2208}{\ensuremath{\in}}
\DeclareUnicodeCharacter{2209}{\ensuremath{\notin}}\DeclareUnicodeCharacter{2282}{\ensuremath{\subset}}\DeclareUnicodeCharacter{2286}{\ensuremath{\subseteq}}\DeclareUnicodeCharacter{2283}{\ensuremath{\supset}}
\DeclareUnicodeCharacter{2287}{\ensuremath{\supseteq}}\DeclareUnicodeCharacter{2229}{\ensuremath{\cap}}\DeclareUnicodeCharacter{222A}{\ensuremath{\cup}}\DeclareUnicodeCharacter{2205}{\ensuremath{\emptyset}}
\DeclareUnicodeCharacter{2211}{\ensuremath{\sum}}\DeclareUnicodeCharacter{220F}{\ensuremath{\prod}}\DeclareUnicodeCharacter{222B}{\ensuremath{\int}}\DeclareUnicodeCharacter{2202}{\ensuremath{\partial}}
\DeclareUnicodeCharacter{226A}{\ensuremath{\ll}}\DeclareUnicodeCharacter{226B}{\ensuremath{\gg}}\DeclareUnicodeCharacter{223C}{\ensuremath{\sim}}\DeclareUnicodeCharacter{2245}{\ensuremath{\cong}}
\DeclareUnicodeCharacter{22A5}{\ensuremath{\perp}}\DeclareUnicodeCharacter{221D}{\ensuremath{\propto}}\DeclareUnicodeCharacter{230A}{\ensuremath{\lfloor}}\DeclareUnicodeCharacter{230B}{\ensuremath{\rfloor}}
\DeclareUnicodeCharacter{2308}{\ensuremath{\lceil}}\DeclareUnicodeCharacter{2309}{\ensuremath{\rceil}}\DeclareUnicodeCharacter{2194}{\ensuremath{\leftrightarrow}}\DeclareUnicodeCharacter{21D2}{\ensuremath{\Rightarrow}}
\DeclareUnicodeCharacter{21D0}{\ensuremath{\Leftarrow}}\DeclareUnicodeCharacter{21D4}{\ensuremath{\Leftrightarrow}}\DeclareUnicodeCharacter{2191}{\ensuremath{\uparrow}}\DeclareUnicodeCharacter{2193}{\ensuremath{\downarrow}}
\DeclareUnicodeCharacter{22C5}{\ensuremath{\cdot}}\DeclareUnicodeCharacter{2218}{\ensuremath{\circ}}\DeclareUnicodeCharacter{221A}{\ensuremath{\surd}}\DeclareUnicodeCharacter{2714}{\checkmark}
\DeclareUnicodeCharacter{2717}{\ensuremath{\times}}\DeclareUnicodeCharacter{2718}{\ensuremath{\times}}\DeclareUnicodeCharacter{2605}{\ensuremath{\bigstar}}\DeclareUnicodeCharacter{2610}{\ensuremath{\square}}
\DeclareUnicodeCharacter{200B}{}\DeclareUnicodeCharacter{200C}{}\DeclareUnicodeCharacter{200D}{}\DeclareUnicodeCharacter{200E}{}\DeclareUnicodeCharacter{200F}{}\DeclareUnicodeCharacter{202A}{}\DeclareUnicodeCharacter{202B}{}
\DeclareUnicodeCharacter{202C}{}\DeclareUnicodeCharacter{202D}{}\DeclareUnicodeCharacter{202E}{}\DeclareUnicodeCharacter{2060}{}\DeclareUnicodeCharacter{FEFF}{}\DeclareUnicodeCharacter{00AD}{}
% Cualquier otro carácter que no conozca (un emoji, un alfabeto que no es el latino…) sale como «?» en un recuadro, en
% lugar de parar la compilación.
\makeatletter\def\UTFviii@undefined@err#1{\fbox{?}}\makeatother
\newcolumntype{Y}{>{\raggedright\arraybackslash}X}
\setlength{\parindent}{0pt}
\pagestyle{fancy}
\fancyhf{}
\renewcommand{\headrulewidth}{0pt}
\newcommand{\pie}{}
\newcommand{\finmodelo}{}
\fancyfoot[R]{\footnotesize\pie: \thepage{} / \pageref{\finmodelo}}
\newcommand{\empezarmodelo}[2]{\clearpage\setcounter{page}{1}\renewcommand{\pie}{#1}\renewcommand{\finmodelo}{#2}}
\newcommand{\sangria}{\hspace*{8mm}}
\newcommand{\enunciado}[1]{{\raggedright\bfseries\itshape #1\par}}
\newcommand{\opcion}[2]{#1)~#2}
\newcommand{\opcioncorrecta}[2]{\textcolor{verde}{\textbf{#1)~#2~\checkmark}}}
% Separación entre preguntas y tamaño de letra del examen (los fija el nivel de compactación tras \begin{document}).
\newlength{\seppregunta}\setlength{\seppregunta}{6pt}
\newcommand{\letraexamen}{\normalsize}
% Cada pregunta va en un bloque que no se parte entre dos páginas.
\newenvironment{pregunta}{\par\addvspace{\seppregunta}\noindent\begin{minipage}{\linewidth}}{\end{minipage}\par}
\newsavebox{\cajaestructura}
% Código de los enunciados, como en el PDF: en un recuadro gris (el marco y el fondo de listings) del ancho de su línea
% más larga (como mucho, el de la línea del examen, y entonces se parten las líneas que no caben), resaltado según su
% lenguaje: palabras clave en negrita, comentarios en cursiva y cadenas en granate.
% \begin{codigo}[opciones de listings]{caracteres de la línea más larga}{caracteres de los números de línea y su
% separación, o 0}. No se mete en una caja (minipage, lrbox): listings dejaría mal las letras con tilde del texto que
% sigue. Los caracteres que no son ASCII van como texto LaTeX entre (*@ y @*), salvo las letras con tilde, diéresis o
% eñe, que las cambia listings.
\definecolor{fondocodigo}{HTML}{EEEEEE}
\definecolor{bordecodigo}{HTML}{808080}
\definecolor{codigoclave}{HTML}{1F3A93}
\definecolor{codigocomentario}{HTML}{666666}
\definecolor{codigocadena}{HTML}{8B1A1A}
\definecolor{codigonumero}{HTML}{808080}
\lstset{basicstyle=\ttfamily, columns=fullflexible, keepspaces=true, showstringspaces=false, tabsize=4,
  breaklines=true, keywordstyle=\bfseries\color{codigoclave},
  commentstyle=\itshape\color{codigocomentario}, stringstyle=\color{codigocadena},
  numberstyle=\ttfamily\color{codigonumero}, escapeinside={(*@}{@*)}, frame=single, framesep=4pt,
  framerule=0.4pt, rulecolor=\color{bordecodigo}, backgroundcolor=\color{fondocodigo}, aboveskip=4pt, belowskip=4pt,
  literate={á}{{\'a}}1 {é}{{\'e}}1 {í}{{\'i}}1 {ó}{{\'o}}1 {ú}{{\'u}}1 {Á}{{\'A}}1 {É}{{\'E}}1 {Í}{{\'I}}1
    {Ó}{{\'O}}1 {Ú}{{\'U}}1 {ü}{{\"u}}1 {Ü}{{\"U}}1 {ñ}{{\~n}}1 {Ñ}{{\~N}}1}
\lstdefinelanguage{JSON}{morestring=[b]", morekeywords={true,false,null}, sensitive=true}
\lstdefinelanguage{YAML}{morecomment=[l]\#, morestring=[b]", morestring=[b]', morekeywords={true,false,null,yes,no},
  sensitive=false}
\newlength{\anchocaracter}
\newlength{\anchocodigo}
\newlength{\anchonumeros}
\lstnewenvironment{codigo}[3][]{%
  \settowidth{\anchocaracter}{\ttfamily 0}%
  \setlength{\anchocodigo}{#2\anchocaracter}%
  \setlength{\anchonumeros}{#3\anchocaracter}%
  \ifdim\anchocodigo>\dimexpr\linewidth-8mm-8.8pt-\anchonumeros\relax
    \setlength{\anchocodigo}{\dimexpr\linewidth-8mm-8.8pt-\anchonumeros\relax}\fi
  \lstset{framexleftmargin=\anchonumeros, xleftmargin=\dimexpr8mm+4.4pt+\anchonumeros\relax,
    linewidth=\dimexpr8mm+8.8pt+\anchonumeros+\anchocodigo\relax, #1}}{}
)tex";

// Sin los caracteres de control (invisibles, salvo el tabulador y los saltos de línea), que pdflatex no admite: con uno
// solo, el documento no compila. Llegan, por ejemplo, al copiar de Word (U+000B, su salto de línea manual).
bool esDeControl(QChar c)
{
    const char16_t u = c.unicode();
    return (u < 0x20 && u != '\t' && u != '\n' && u != '\r') || (u >= 0x7F && u <= 0x9F);
}

// Si el texto tiene algo que se ve: no solo espacios, caracteres de control, de formato (invisibles, como U+200B) o
// marcas que se combinan con la letra anterior.
bool tieneAlgoVisible(const QString &texto)
{
    for (const QChar c : texto)
        if (!c.isSpace() && !esDeControl(c) && c.category() != QChar::Other_Format && c.category() != QChar::Mark_NonSpacing
            && c.category() != QChar::Mark_Enclosing)
            return true;
    return false;
}

QString imagenLatex(const Pregunta &p, const QString &carpeta, int n)
{
    const int ancho = anchoImagen(p, n);
    if (ancho == 0)
        return QString();
    return QStringLiteral("\\par\\smallskip\\sangria\\includegraphics[width=%1cm]{%2/%3}\\par\\smallskip\n")
        .arg(QString::number(ancho * 2.54 / 96, 'f', 2), carpeta, Maquetador::nombreImagen(p));
}

// Texto de una pregunta con su código en línea (entre comillas invertidas: ver enunciado.h) en letra de ancho fijo y
// sin cursiva.
QString textoLatex(const QString &texto)
{
    QString tex;
    for (const Enunciado::Trozo &t : Enunciado::trozos(texto))
        tex += t.codigo ? QStringLiteral("\\texttt{\\upshape %1}").arg(Latex::escapar(t.texto)) : Latex::escapar(t.texto);
    return tex;
}

QString opcionLatex(int i, const QString &texto, bool marcada)
{
    return QStringLiteral("\\%1{%2}{%3}")
        .arg(marcada ? QStringLiteral("opcioncorrecta") : QStringLiteral("opcion"), QString(Pregunta::letra(i)),
             textoLatex(texto));
}

// Equivalente a opcionesConImagenHtml(): las opciones, una por línea, y la imagen a su derecha, con su borde de
// arriba a la altura del de la primera opción.
QString opcionesConImagenLatex(const PreguntaExamen &pe, bool soluciones, const QString &carpeta, int n)
{
    QStringList opciones;
    for (int i = 0; i < 4; ++i)
        opciones << opcionLatex(i, pe.opcion(i), soluciones && i == pe.correcta());
    return QStringLiteral("\\par\\vspace{1pt}\n\\noindent\\sangria\\begin{tabular}[t]{@{}l@{}}\n%1\n\\end{tabular}"
                          "\\hspace{%2mm}\\raisebox{\\dimexpr\\ht\\strutbox-\\height}{\\includegraphics[width=%3cm]{%4/%5}}\\par\n")
        .arg(opciones.join(QStringLiteral(" \\\\\n")), QString::number(kSeparacionImagen * 25.4 / 96, 'f', 1),
             QString::number(anchoImagen(pe.pregunta, n) * 2.54 / 96, 'f', 2), carpeta, Maquetador::nombreImagen(pe.pregunta));
}

// Mismo reparto que opcionesHtml: en una fila, en dos columnas (a c / b d) o una por línea.
QString opcionesLatex(const PreguntaExamen &pe, bool soluciones, int n)
{
    const int filas = filasOpciones(pe, n);
    const int correcta = pe.correcta();
    auto op = [&](int i) { return opcionLatex(i, pe.opcion(i), soluciones && i == correcta); };
    const QString finFila = QStringLiteral(" \\\\\n");
    const QString sep = QStringLiteral(" & ");
    QString columnas;
    QString cuerpo;
    if (filas == 1) {
        columnas = QStringLiteral("YYYY");
        cuerpo = op(0) + sep + op(1) + sep + op(2) + sep + op(3) + finFila;
    } else if (filas == 2) {
        columnas = QStringLiteral("YY");
        for (int fila = 0; fila < 2; ++fila)
            cuerpo += op(fila) + sep + op(fila + 2) + finFila;
    } else {
        columnas = QStringLiteral("Y");
        for (int i = 0; i < 4; ++i)
            cuerpo += op(i) + finFila;
    }
    return QStringLiteral("\\par\\vspace{1pt}\n\\begin{tabularx}{\\linewidth}{@{\\sangria}%1@{}}\n%2\\end{tabularx}\n")
        .arg(columnas, cuerpo);
}

// Equivalente a codigoHtml(): el entorno codigo del preámbulo, con el lenguaje en listings y los números de línea si los
// lleva. Los tabuladores pasan a ser cuatro espacios.
QString codigoLatex(const Enunciado::Bloque &b)
{
    static const QString conLiterate = QStringLiteral("áéíóúÁÉÍÓÚüÜñÑ");
    QStringList lineas;
    int maximo = 1;
    for (QString linea : b.lineas) {
        linea.replace(QLatin1Char('\t'), QStringLiteral("    "));
        linea.removeIf(esDeControl);
        // Los caracteres que van como texto LaTeX («→») pueden ser más anchos que los demás: cuentan por dos.
        int ancho = 0;
        QString escrita;
        for (const QChar c : linea) {
            const bool comoTexto = c.unicode() >= 128 && !conLiterate.contains(c);
            escrita += comoTexto ? QStringLiteral("(*@%1@*)").arg(c) : QString(c);
            ancho += comoTexto ? 2 : 1;
        }
        maximo = qMax(maximo, ancho);
        lineas << escrita;
    }
    QStringList opciones;
    const QString lenguaje = Resaltado::enListings(Resaltado::lenguaje(b.lenguaje));
    // Entre llaves si lleva corchetes («[CORBA]IDL»), que cerrarían antes de tiempo las opciones de \begin{codigo}.
    if (!lenguaje.isEmpty())
        opciones << QStringLiteral("language=") + (lenguaje.contains(QLatin1Char('[')) ? QLatin1Char('{') + lenguaje + QLatin1Char('}') : lenguaje);
    // En XML y HTML, el nombre de cada etiqueta en negrita, como las palabras clave (la opción solo existe con ellos).
    if (lenguaje == QLatin1String("XML") || lenguaje == QLatin1String("HTML"))
        opciones << QStringLiteral("markfirstintag=true");
    // Los números de línea, a la izquierda y dentro del recuadro: sus cifras y un espacio.
    int numeros = 0;
    if (b.numerado) {
        numeros = QString::number(qMax(1, int(b.lineas.size()))).size() + 1;
        opciones << QStringLiteral("numbers=left") << QStringLiteral("numbersep=\\anchocaracter");
    }
    return QStringLiteral("\\begin{codigo}[%1]{%2}{%3}\n%4\n\\end{codigo}\n")
        .arg(opciones.join(QLatin1Char(',')), QString::number(maximo), QString::number(numeros), lineas.join(QLatin1Char('\n')));
}

// Equivalente a explicacionHtml: la explicación y la referencia de la pregunta, debajo de las opciones.
QString explicacionLatex(const Pregunta &p, const DatosConvocatoria &d, int n)
{
    QStringList partes;
    auto parte = [&](const char *etiqueta, const QString &texto) {
        if (texto.trimmed().isEmpty())
            return;
        QStringList lineas;
        for (const QString &linea : texto.trimmed().split(QLatin1Char('\n')))
            lineas << textoLatex(linea);
        partes << QStringLiteral("\\textcolor{verde}{\\textbf{%1}} %2")
                      .arg(Latex::escapar(tx(d, etiqueta)), lineas.join(QStringLiteral("\\newline ")));
    };
    parte(QT_TRANSLATE_NOOP("Examen", "Explicación:"), p.explicacion);
    parte(QT_TRANSLATE_NOOP("Examen", "Referencia:"), p.referencia);
    if (partes.isEmpty())
        return QString();
    return QStringLiteral("\\par\\vspace{2pt}{%1\\leftskip=8mm\\noindent %2\\par}\n")
        .arg(letraExplicacionLatex(n), partes.join(QStringLiteral("\\par\n\\noindent ")));
}

// Equivalente a Maquetador::enunciadoHtml: párrafos en negrita cursiva, bloques ``` y tablas | a | b |.
QString enunciadoLatex(const QString &texto, const QString &prefijo)
{
    QString tex;
    bool prefijoPuesto = prefijo.isEmpty();
    auto ponerPrefijoSolo = [&]() {
        if (!prefijoPuesto) {
            tex += QStringLiteral("\\enunciado{%1}\n").arg(prefijo);
            prefijoPuesto = true;
        }
    };
    for (const Enunciado::Bloque &b : Enunciado::bloques(texto)) {
        switch (b.tipo) {
        case Enunciado::Bloque::Parrafo: {
            // Sin las líneas que no tienen nada que se vea: un \\ sin nada delante («There's no line here to end») es un
            // error de LaTeX. Y tras cada \\, \relax: si la línea siguiente empezara por «[» (una lista de Python) o «*»,
            // LaTeX los tomaría por un argumento del \\ («\\[1cm]», «\\*»).
            QStringList lineas;
            for (const QString &linea : b.lineas)
                if (tieneAlgoVisible(linea))
                    lineas << textoLatex(linea);
            if (lineas.isEmpty())
                break;
            QString contenido = lineas.join(QStringLiteral("\\\\\\relax\n"));
            if (!prefijoPuesto) {
                contenido = prefijo + contenido;
                prefijoPuesto = true;
            }
            tex += QStringLiteral("\\enunciado{%1}\n").arg(contenido);
            break;
        }
        case Enunciado::Bloque::Tabla: {
            ponerPrefijoSolo();
            int columnas = 0;
            for (const QStringList &fila : b.filas)
                columnas = qMax(columnas, int(fila.size()));
            tex += QStringLiteral("\\par\\smallskip\\sangria\\begin{tabular}{|%1}\\hline\n").arg(QStringLiteral("l|").repeated(columnas));
            for (const QStringList &fila : b.filas) {
                QStringList celdas;
                for (const QString &c : fila)
                    celdas << textoLatex(c);
                while (celdas.size() < columnas)
                    celdas << QString();
                tex += celdas.join(QStringLiteral(" & ")) + QStringLiteral(" \\\\ \\hline\n");
            }
            tex += QStringLiteral("\\end{tabular}\\par\\smallskip\n");
            break;
        }
        case Enunciado::Bloque::Codigo:
            ponerPrefijoSolo();
            tex += codigoLatex(b);
            break;
        }
    }
    ponerPrefijoSolo();
    return tex;
}

// Con letra grande, la cabecera y las instrucciones van en \LARGE y el título en \huge.
QString cabeceraLatex(const DatosConvocatoria &d, const QString &derecha, int n = 0)
{
    // Como en cabeceraHtml, sin las líneas vacías: un \\ sin nada delante es un error de LaTeX («There's no line here
    // to end»), que pasaba con un banco sin asignatura.
    const bool grande = n == Maquetador::kLetraGrande;
    QStringList izquierda;
    if (!asignaturaCabecera(d).isEmpty())
        izquierda << QStringLiteral("{%1\\bfseries %2}").arg(grande ? QStringLiteral("\\huge") : QStringLiteral("\\Large"),
                                                          Latex::escapar(asignaturaCabecera(d)));
    if (!convocatoriaCabecera(d).isEmpty())
        izquierda << Latex::escapar(convocatoriaCabecera(d));
    if (izquierda.isEmpty() && derecha.isEmpty())
        return QString();
    return QStringLiteral("\\noindent\\begin{minipage}[t]{0.58\\linewidth}\n%1\n\\end{minipage}\\hfill\n"
                          "\\begin{minipage}[t]{0.4\\linewidth}\\raggedleft\n%2\n\\end{minipage}\\par\n")
        .arg(izquierda.join(QStringLiteral("\\\\\n")), derecha);
}

QString cuerpoExamenLatex(const Modelo &modelo, const DatosConvocatoria &d, bool soluciones,
                          const QString &carpetaImagenes, int n)
{
    QString derecha;
    if (soluciones)
        derecha = QStringLiteral("\\textcolor{verde}{\\bfseries %1}").arg(Latex::escapar(tx(d, QT_TRANSLATE_NOOP("Examen", "CON SOLUCIONES"))));
    // Con letra grande, todo el modelo (cabecera e instrucciones incluidas) va en \letraexamen.
    QString tex = n == Maquetador::kLetraGrande ? QStringLiteral("{\\letraexamen\n") : QString();
    tex += cabeceraLatex(d, derecha, n);

    QStringList instrucciones;
    for (const QString &linea : d.instrucciones.split(QLatin1Char('\n')))
        if (!linea.trimmed().isEmpty())
            instrucciones << Latex::escapar(linea);
    // Recuadro de instrucciones: el texto y, justo debajo, la puntuación en una línea (sin líneas en blanco, para que
    // sea bajo).
    instrucciones << Latex::escapar(textoPuntuacion(d));
    tex += QStringLiteral(
               "\\par\\smallskip\n{\\setlength{\\fboxsep}{4pt}\\noindent\\fbox{\\begin{minipage}{\\dimexpr\\linewidth-2\\fboxsep-2\\fboxrule\\relax}\n"
               "%1\n\\end{minipage}}}\\par\\smallskip\n")
               .arg(instrucciones.join(QStringLiteral("\\par\n")));

    if (n != Maquetador::kLetraGrande)
        tex += QStringLiteral("{\\letraexamen\n");
    for (int i = 0; i < modelo.preguntas.size(); ++i) {
        const PreguntaExamen &pe = modelo.preguntas.at(i);
        QString prefijo = QStringLiteral("%1.- ").arg(i + 1);
        if (soluciones)
            prefijo += QStringLiteral("\\textcolor{verde}{\\upshape[%1]} ").arg(Latex::escapar(pe.pregunta.codigoCompleto()));
        tex += QStringLiteral("\\begin{pregunta}\n");
        tex += enunciadoLatex(pe.pregunta.enunciado, prefijo);
        if (imagenAlLado(pe, n)) {
            tex += opcionesConImagenLatex(pe, soluciones, carpetaImagenes, n);
        } else {
            tex += imagenLatex(pe.pregunta, carpetaImagenes, n);
            tex += opcionesLatex(pe, soluciones, n);
        }
        if (soluciones)
            tex += explicacionLatex(pe.pregunta, d, n);
        tex += QStringLiteral("\\end{pregunta}\n");
    }
    tex += QStringLiteral("\\par}\n");
    return tex;
}

QString solucionesLatex(const QVector<Modelo> &modelos, const DatosConvocatoria &d)
{
    // A la derecha, el título y, debajo, la semilla con que se generaron los exámenes (no va en ellos).
    QString derecha = QStringLiteral("{\\LARGE\\bfseries %1}").arg(Latex::escapar(tx(d, QT_TRANSLATE_NOOP("Examen", "Soluciones"))));
    if (!textoSemilla(d).isEmpty())
        derecha += QStringLiteral("\\\\\n{\\scriptsize\\color{darkgray}%1}").arg(Latex::escapar(textoSemilla(d)));
    QString tex = cabeceraLatex(d, derecha);

    // Estructura: qué preguntas de cada tema entran en cada modelo. Si la tabla cabe en la línea, con su ancho natural;
    // si no (muchas preguntas o muchos temas), del ancho de la página, con las celdas partiendo sus líneas tras las
    // comas, como en el PDF. Se mide en LaTeX (\sbox) para no depender de paquetes que no son los básicos.
    const QList<int> temas = temasDe(modelos);
    QString cuerpo = QStringLiteral("\\hline\n\\textbf{%1}").arg(Latex::escapar(tx(d, QT_TRANSLATE_NOOP("Examen", "Examen"))));
    for (int t : temas)
        cuerpo += QStringLiteral(" & \\textbf{T%1}").arg(t);
    cuerpo += QStringLiteral(" \\\\ \\hline\n");
    for (const Modelo &m : modelos) {
        cuerpo += QStringLiteral("\\textbf{") + Latex::escapar(m.nombre) + QLatin1Char('}');
        for (int t : temas)
            cuerpo += QStringLiteral(" & ") + Latex::escapar(codigosDelTema(m, t).join(QStringLiteral(", ")));
        cuerpo += QStringLiteral(" \\\\ \\hline\n");
    }
    tex += QStringLiteral("\\subsection*{%1}\n").arg(Latex::escapar(tx(d, QT_TRANSLATE_NOOP("Examen", "Estructura de la convocatoria"))));
    tex += QStringLiteral("\\sbox{\\cajaestructura}{\\begin{tabular}{|l|%1}%2\\end{tabular}}%\n"
                          "\\ifdim\\wd\\cajaestructura>\\linewidth\n"
                          "\\begin{tabularx}{\\linewidth}{|l|%3}%2\\end{tabularx}\n"
                          "\\else\\usebox{\\cajaestructura}\\fi\n")
               .arg(QStringLiteral("l|").repeated(temas.size()), cuerpo, QStringLiteral("Y|").repeated(temas.size()));

    // Plantilla de corrección de cada modelo («1 C», «2 A»…, en filas de 10) y, debajo, código, respuesta y referencia de
    // cada pregunta.
    for (const Modelo &m : modelos) {
        const int n = m.preguntas.size();
        tex += QStringLiteral("\\subsection*{%1}\n")
                   .arg(Latex::escapar(tx(d, QT_TRANSLATE_NOOP("Examen", "Plantilla de corrección: %1")).arg(m.nombre)));
        if (n == 0)
            continue;
        tex += QStringLiteral("\\begin{tabular}{|*{%1}{>{\\centering\\arraybackslash}p{9mm}|}}\n").arg(qMin(10, n));
        tex += QStringLiteral("\\cline{1-%1}\n").arg(qMin(10, n));
        for (int inicio = 0; inicio < n; inicio += 10) {
            const int fin = qMin(inicio + 10, n);
            QStringList celdas;
            for (int i = inicio; i < fin; ++i)
                celdas << QStringLiteral("%1~\\textbf{%2}").arg(QString::number(i + 1), QString(Pregunta::letraMayuscula(m.preguntas.at(i).correcta())));
            tex += celdas.join(QStringLiteral(" & ")) + QStringLiteral(" \\\\ \\cline{1-%1}\n").arg(fin - inicio);
        }
        tex += QStringLiteral("\\end{tabular}\n\\par\\smallskip\n")
               + Latex::escapar(tx(d, QT_TRANSLATE_NOOP("Examen", "Respuestas correctas: %1")).arg(Maquetador::textoCorrectasPorLetra(m)))
               + QStringLiteral("\\par\\smallskip\n{\\footnotesize\n"
                              "\\begin{tabularx}{\\linewidth}{@{}r@{\\hspace{1em}}l@{\\hspace{1em}}l@{\\hspace{1em}}Y@{}}\n");
        for (int i = 0; i < n; ++i) {
            const PreguntaExamen &pe = m.preguntas.at(i);
            tex += QStringLiteral("%1. & %2 & \\textbf{%3} & ").arg(i + 1).arg(Latex::escapar(pe.pregunta.codigoCompleto())).arg(Pregunta::letraMayuscula(pe.correcta()))
                   + Latex::escapar(pe.pregunta.referencia) + QStringLiteral(" \\\\\n");
        }
        tex += QStringLiteral("\\end{tabularx}}\n");
    }
    return tex;
}

} // namespace

QString Latex::escapar(const QString &texto)
{
    QString r;
    r.reserve(texto.size() + texto.size() / 8);
    for (const QChar c : texto) {
        if (esDeControl(c))
            continue;
        switch (c.unicode()) {
        case '\\': r += QStringLiteral("\\textbackslash{}"); break;
        case '{': case '}': case '$': case '&': case '#': case '_': case '%':
            r += QLatin1Char('\\');
            r += c;
            break;
        case '~': r += QStringLiteral("\\textasciitilde{}"); break;
        case '^': r += QStringLiteral("\\textasciicircum{}"); break;
        // Comillas, < > y ` sueltos: evitan ligaduras («<<» → «, «?`» → ¿...) y los atajos de babel.
        case '"': r += QStringLiteral("\\textquotedbl{}"); break;
        case '\'': r += QStringLiteral("\\textquotesingle{}"); break;
        case '`': r += QStringLiteral("\\textasciigrave{}"); break;
        case '<': r += QStringLiteral("\\textless{}"); break;
        case '>': r += QStringLiteral("\\textgreater{}"); break;
        case '|': r += QStringLiteral("\\textbar{}"); break;
        case '-':
            if (r.endsWith(QLatin1Char('-')))
                r += QStringLiteral("{}");   // «--» y «---» no deben convertirse en rayas
            r += c;
            break;
        default:
            r += c;
        }
    }
    return r;
}

QString Latex::examenes(const QVector<Modelo> &modelos, const DatosConvocatoria &d, bool soluciones,
                                  const QString &carpetaImagenes, int n)
{
    // La semilla, que no va en los exámenes: en un comentario del principio y, al compilar con pdflatex, en las
    // propiedades del PDF (solo con los caracteres que no hay que escapar en ellas).
    const QString semilla = textoSemilla(d);
    QString tex = QStringLiteral("% ") + programa(d) + QLatin1Char('\n');
    tex += QString::fromUtf8(kPreambuloLatex);
    // babel, en el idioma del examen.
    if (d.idioma == QLatin1String("en"))
        tex.replace(QStringLiteral("\\usepackage[spanish,es-noshorthands]{babel}"), QStringLiteral("\\usepackage[english]{babel}"));
    if (!semilla.isEmpty())
        tex += QStringLiteral("\\ifdefined\\pdfinfo\\pdfinfo{/Subject (%1)}\\fi\n")
                   .arg(QString(semilla).remove(QRegularExpression(QStringLiteral("[^A-Za-z0-9 :.,-]"))));
    tex += QStringLiteral("\\begin{document}\n");
    // Nivel de compactación: separación entre preguntas y letra. Con letra grande, enunciados sin cursiva.
    tex += QStringLiteral("\\setlength{\\seppregunta}{%1pt}\\renewcommand{\\letraexamen}{%2}\n")
               .arg(QLatin1String(nivel(n).separacionLatex), QLatin1String(nivel(n).letraLatex));
    if (n == Maquetador::kLetraGrande || !d.enunciadoEnCursiva)
        tex += QStringLiteral("\\renewcommand{\\enunciado}[1]{{\\raggedright\\bfseries #1\\par}}\n");
    if (soluciones) {
        tex += QStringLiteral("\n\\empezarmodelo{") + Latex::escapar(textoPie(tx(d, QT_TRANSLATE_NOOP("Examen", "Soluciones")), d))
               + QStringLiteral("}{fin:soluciones}\n");
        tex += solucionesLatex(modelos, d);
        // En un párrafo propio: detrás de la tabla, que ocupa todo el ancho, pasaría a otra línea, y con la página
        // llena, a una página más, vacía salvo el pie.
        tex += QStringLiteral("\\par\\label{fin:soluciones}\n");
    }
    for (int i = 0; i < modelos.size(); ++i) {
        const QString etiqueta = QStringLiteral("fin:modelo%1").arg(i + 1);
        tex += QStringLiteral("\n\\empezarmodelo{") + Latex::escapar(textoPie(modelos.at(i).nombre, d))
               + QStringLiteral("}{") + etiqueta + QStringLiteral("}\n");
        tex += cuerpoExamenLatex(modelos.at(i), d, soluciones, carpetaImagenes, n);
        tex += QStringLiteral("\\label{") + etiqueta + QStringLiteral("}\n");
    }
    return tex + QStringLiteral("\\end{document}\n");
}
