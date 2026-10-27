#pragma once

#include "generador.h"

#include <QString>
#include <QStringList>
#include <QVector>
#include <array>

// Exámenes hechos fuera del programa (los de las convocatorias anteriores a usarlo), para registrarlos en el
// historial y que sus preguntas se eviten al generar los siguientes: se separa el texto de cada examen en preguntas
// y se reconoce cada una en el banco con el índice de parecidas.
namespace ExamenesAnteriores {

// Una pregunta tal como está en el texto del examen.
struct Leida
{
    QString enunciado;
    QStringList opciones = QStringList(4, QString());   // a), b), c) y d); vacías las que no se encuentren
};

// Las preguntas del texto de un examen, numeradas «1.-», «2.-»... (o «1.», «1)») al principio de una línea, cada una
// con sus opciones «a)»...«d)» en una columna o en dos. Lo de antes de la primera (cabecera, instrucciones) y los
// pies de página («…: 1 / 2») no cuentan. Si a algún número le falta la puntuación («7 Texto…» entre la 6.- y la
// 8.-), se busca igualmente.
QVector<Leida> separar(const QString &texto);

struct Reconocida
{
    Leida leida;
    PreguntaExamen pregunta;   // la del banco (id 0 si no hay ninguna) y el orden en que salieron sus opciones
    double parecido = 0;       // de 0 a 1
    bool dudosa = true;        // poco parecida, o casi tan parecida como otra pregunta de otro número
    QString otra;              // la más parecida de otro número, y cuánto
    double parecidoOtra = 0;
};

// Mínimo parecido para dar una pregunta por reconocida sin dudas, y ventaja mínima sobre la siguiente más parecida
// de otro número. Con los once exámenes de una convocatoria hecha a mano, leídos de sus PDF, las 110 preguntas
// pasan del 0,7 (y casi todas del 0,9) con mucha ventaja.
constexpr double kMinimoReconocida = 0.6;
constexpr double kVentajaMinima = 0.15;

// La pregunta del banco más parecida a cada una de las leídas, con el orden en que salieron sus opciones.
QVector<Reconocida> reconocer(const QVector<Leida> &leidas, const QVector<Pregunta> &banco);

// Orden en que salieron las opciones de 'p' en 'leida' (opción mostrada i -> opción original orden[i]). Si las de
// 'leida' no se parecen bastante a las de 'p', las deja en su orden.
std::array<int, 4> ordenDeOpciones(const Leida &leida, const Pregunta &p);

// Convocatoria, año y nombre del examen que dice el nombre de su fichero: «2026_02_01_nacional_original_ex_fi.pdf»
// es Febrero de 2026, «Nacional Original»; «2026_Junio_UE_Reserva.pdf», Junio de 2026, «UE Reserva». El nombre se
// cambia por el de 'conocidos' (los de la lista de exámenes) que le corresponde, si hay uno solo: «ae_original»
// es «América Este Original» y «ue_original», «UE-Original y Guinea». Lo que no diga se queda vacío (el año, 0).
struct DatosFichero
{
    QString convocatoria;
    int anio = 0;
    QString nombre;
};
DatosFichero datosDeFichero(const QString &ruta, const QStringList &conocidos = QStringList());

// Si el programa puede leer PDF (se ha compilado con Qt PDF).
bool puedeLeerPdf();
// El texto de un PDF; vacío, con el motivo en 'error', si no se puede leer.
QString textoDePdf(const QString &ruta, QString *error = nullptr);

} // namespace ExamenesAnteriores
