#pragma once

#include <QColor>
#include <QString>

class QPalette;

// Los colores del programa, cada uno con su significado y en un solo sitio.
//
// Hay tres clases, según sobre qué fondo van:
//  - Tintas de la interfaz (tinta(), css()): texto de color sobre el fondo de las ventanas, que es claro u oscuro
//    según el tema del escritorio. Cada una tiene dos tonos, y con los dos se lee bien (contraste de 4,5:1 o más,
//    salvo Inactiva, que es de lo que no cuenta).
//  - Recuadros (fondoAtencion(), recuadroAtencion()…): llevan su fondo y su tinta, así que se leen igual con
//    cualquier tema.
//  - Colores del papel (kPapel…): los de los exámenes y las fichas de las preguntas, que se ven y se imprimen siempre
//    sobre blanco, también con el tema oscuro (VisorAmpliable pone el papel: ver papel()).
namespace Paleta {

enum Tinta {
    Secundario,    // notas y aclaraciones: lo que acompaña, sin dejar de leerse
    Inactiva,      // lo que no cuenta: las preguntas anuladas u obsoletas de la lista
    Atencion,      // algo que conviene mirar: un aviso, «sin verificar»
    Informacion,   // un dato que se destaca: la nota de la práctica
    Verificada,    // la pregunta la ha revisado una persona (azul, para que no se confunda con «correcta»)
    Correcta,      // la respuesta correcta
    Error,         // la respuesta incorrecta, el tiempo que se acaba
};

// Si la interfaz tiene el fondo oscuro (el tema del escritorio).
bool oscura();
// La tinta para el fondo de la interfaz: el que haya, o el claro o el oscuro que se diga.
QColor tinta(Tinta t);
QColor tinta(Tinta t, bool sobreOscuro);
// «color: #rrggbb», para una hoja de estilo.
QString css(Tinta t);

// Fondo y tinta de lo que pide atención (ámbar), de lo que informa (azul) y de lo que ha fallado (rojo), para una hoja
// de estilo. Los recuadros llevan además su borde y su margen interior.
QString fondoAtencion();
QString fondoError();
QString recuadroAtencion();
QString recuadroInformacion();
QColor colorFondoAtencion();
QColor colorTintaAtencion();

// El papel: fondo blanco y tinta negra, sea cual sea el tema, con los enlaces en un azul que se lee sobre blanco.
QPalette papel(const QPalette &base);

// Sobre el papel (los exámenes y las fichas): valen para el HTML, y sin la almohadilla para el Word y el LaTeX.
constexpr const char *kPapelCorrecta = "#0a7d32";      // la respuesta correcta y lo que la acompaña
constexpr const char *kPapelVerificada = "#1d4ed8";    // «✓ verificada»
constexpr const char *kPapelSinVerificar = "#b45309";  // «● sin verificar»
constexpr const char *kPapelSecundario = "#555555";    // la semilla, lo que se propone en la vista vacía
constexpr const char *kPapelFondoCodigo = "#eeeeee";   // los bloques de código
constexpr const char *kPapelBordeCodigo = "#808080";

} // namespace Paleta
