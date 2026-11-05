#pragma once

#include <QString>
#include <QVector>

// Idiomas del programa. Hay dos, independientes:
//  - el de la interfaz (menús, diálogos, avisos), que elige cada usuario en Editar → Idioma y se guarda en su
//    configuración; por defecto, el del sistema si hay traducción y, si no, el castellano;
//  - el de los exámenes (y de lo que se exporta a Moodle), que va con cada banco (BaseDatos::idioma()), porque
//    depende de la asignatura y no de quien usa el programa.
// El castellano es el idioma de origen de los textos: los demás están en traducciones/GeneradorExamenes_<código>.ts
// (Qt Linguist), que se compilan dentro del programa.
namespace Idioma {

constexpr const char *kOrigen = "es";

struct Disponible
{
    QString codigo;   // "es", "en"
    QString nombre;   // en su propio idioma: «Español», «English»
};

QVector<Disponible> disponibles();
QString nombre(const QString &codigo);   // el código si no es uno de los disponibles
bool disponible(const QString &codigo);

// El elegido en Editar → Idioma (vacío: el del sistema) y el que se usa de verdad.
QString elegido();
void elegir(const QString &codigo);
QString delSistema();      // el primero de los del sistema que esté disponible; si no, el castellano
QString interfaz();        // elegido() o, si está vacío o ya no está disponible, delSistema()

// Pone la interfaz en ese idioma: las traducciones del programa y las de Qt (botones de los diálogos estándar) y el
// formato de números y fechas. Se llama al arrancar; cambiarlo con el programa abierto no traduce lo que ya está a
// la vista, por eso el idioma nuevo se aplica al volver a abrirlo.
void instalar(const QString &codigo);

// Texto de los exámenes (o de Moodle) en el idioma 'idioma' del banco. 'texto' es el original en castellano,
// marcado para lupdate con QT_TRANSLATE_NOOP("Examen", "…"): todos los textos de los exámenes van en el contexto
// «Examen» de las traducciones. Si no hay traducción, el original.
QString examen(const QString &idioma, const char *texto);

// La ayuda (Ayuda → Manual de uso) en ese idioma, si la hay; si no, la castellana.
QString ayuda(const QString &codigo);

} // namespace Idioma
