#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// Resaltado del código de los enunciados («```java»…, ver Enunciado): las palabras clave en negrita, los comentarios en
// cursiva y las cadenas en otro color, con colores oscuros para que se distinga también impreso en blanco y negro.
// En la vista, el PDF, el HTML y Moodle lo hace resaltar(); en LaTeX, el paquete listings, con los mismos estilos (las
// reglas de los dos se parecen, aunque no son idénticas).
namespace Resaltado {

enum Estilo { Normal, Clave, Comentario, Cadena };

struct Trozo
{
    QString texto;
    Estilo estilo = Normal;
};

// Colores de cada estilo (los mismos en HTML y en LaTeX), sin «#».
constexpr const char *kColorClave = "1F3A93";        // azul oscuro, en negrita
constexpr const char *kColorComentario = "666666";   // gris, en cursiva
constexpr const char *kColorCadena = "8B1A1A";       // granate
constexpr const char *kColorNumero = "808080";       // los números de línea

// Los lenguajes que se resaltan, con el nombre que se escribe tras las ```: c, dtd, html, idl (el de CORBA), java,
// json, python, sql, xml y yaml.
QStringList lenguajes();
// Otros nombres que se aceptan para ellos: «xsd», «xsl», «xslt», «wsdl» y «soap» son xml; «yml», yaml.
QStringList alias();
// El lenguaje que se resalta con ese nombre (en minúsculas): uno de lenguajes(), el de un alias, o vacío si es
// «texto» (código sin resaltar) o desconocido.
QString lenguaje(const QString &nombre);
// Si el nombre es de un lenguaje conocido, de un alias o «texto».
bool conocido(const QString &nombre);
// Nombre del lenguaje en listings: «Java», «XML»… (las DTD, «XML», que tiene sus palabras clave; JSON y YAML los define
// el preámbulo de los exámenes).
QString enListings(const QString &lenguaje);

// Las líneas de código en trozos con su estilo. Los comentarios de varias líneas siguen en las siguientes. Sin
// lenguaje (o uno desconocido), cada línea en un solo trozo normal.
QVector<QVector<Trozo>> resaltar(const QStringList &lineas, const QString &lenguaje);
// Las mismas líneas en HTML (escapadas, cada trozo con estilo en un <span> con su estilo), para ponerlas en un <pre>.
QStringList html(const QStringList &lineas, const QString &lenguaje);

} // namespace Resaltado
