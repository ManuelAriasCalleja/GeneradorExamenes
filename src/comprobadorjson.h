#pragma once

#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

// Comprobación de un JSON de preguntas (el formato de Archivo → Importar preguntas, que se describe en la ayuda y en
// recursos/banco.schema.json) antes de importarlo: dice todos los problemas a la vez, cada uno con su línea y su
// columna y la pregunta en la que está («tema 3, pregunta 12b»). Los errores impiden importar (el JSON está mal
// escrito, falta algo que hace falta, un valor no es válido o se perdería algo al importar); los avisos no (un campo
// desconocido, que se ignora; una pseudorrepetida con una pregunta que no está…).
namespace ComprobadorJson {

// Dirección del esquema JSON (recursos/banco.schema.json, publicado en GitHub): con ella en el campo «$schema», los
// editores como VS Code avisan de los errores y completan los campos mientras se escribe el JSON.
constexpr const char *kEsquema = "https://raw.githubusercontent.com/ManuelAriasCalleja/GeneradorExamenes/main/recursos/banco.schema.json";

struct Problema
{
    enum Gravedad { Error, Aviso };
    Gravedad gravedad = Error;
    int linea = 0;      // desde 1; 0 si es del fichero entero
    int columna = 0;    // desde 1
    QString donde;      // «tema 3, pregunta 12b»; vacío si no es de una pregunta ni de un tema
    QString texto;      // una frase
};

struct Resultado
{
    QVector<Problema> problemas;   // en el orden del fichero
    int temas = 0;
    int preguntas = 0;
    bool leido = false;            // el JSON está bien escrito (aunque tenga otros errores)

    int errores() const;
    int avisos() const;
    bool valido() const { return errores() == 0; }
};

// 'gruposDelBanco', los grupos de preguntas («T1-7») del banco en el que se va a importar, si se sabe: una
// pseudorrepetida con uno de ellos no es un aviso aunque no esté en el JSON.
Resultado comprobar(const QByteArray &json, const QSet<QString> &gruposDelBanco = {});
Resultado comprobarFichero(const QString &ruta, const QSet<QString> &gruposDelBanco = {});

// «Sin errores ni avisos: 120 preguntas en 8 temas.», «2 errores y 1 aviso (120 preguntas en 8 temas).»
QString resumen(const Resultado &r);
// Una línea por problema, como las de los compiladores (que entienden muchos editores): «ruta:12:5: error: tema 3,
// pregunta 12b: Tiene 3 opciones…», y al final el resumen.
QString informe(const Resultado &r, const QString &ruta);
QString nombreGravedad(Problema::Gravedad g);

// Campos que se entienden en cada nivel del JSON (los demás son avisos). Los usa también la prueba que compara el
// comprobador con el esquema JSON.
QStringList camposRaiz();
QStringList camposTema();
QStringList camposPregunta();

} // namespace ComprobadorJson
