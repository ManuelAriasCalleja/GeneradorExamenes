#pragma once

#include <QElapsedTimer>

struct Pregunta;

// La atención al verificar preguntas: hacerlo con muchas seguidas acaba siendo pulsar Intro por inercia. Lleva la
// cuenta del tiempo que se ve cada pregunta y del que se lleva sin parar, y dice cuándo avisar; los avisos los enseña
// DialogoVerificar (y se quitan con su casilla).
class AtencionAlVerificar
{
public:
    // Lo mínimo que se tarda en leer la pregunta, en milisegundos, según sus palabras.
    static int tiempoParaLeer(const Pregunta &p);
    // Cada cuánto se propone una pausa, en milisegundos (25 minutos; las pruebas lo acortan).
    void ponerPausaCada(int ms) { m_pausaCada = ms; }

    // Se empieza a verificar.
    void empezar();
    // Se acaba de enseñar una pregunta.
    void preguntaMostrada();
    // Si la pregunta 'indice', que se quiere marcar como verificada, se ha visto menos de lo que se tarda en leerla.
    // Solo lo dice una vez de cada pregunta: a la siguiente se da por revisada.
    bool demasiadoDeprisa(int indice, const Pregunta &p);
    // Se va a marcar como verificada la que se ve: si se ha estado un rato sin tocar nada, eso ya ha sido una pausa.
    void contarDescanso();
    // Se ha guardado una pregunta como verificada.
    void contarVerificada() { ++m_desdeLaPausa; }
    // Si toca proponer una pausa: entonces deja en 'minutos' y en 'verificadas' lo que se lleva sin parar y empieza a
    // contar de nuevo.
    bool tocaPausa(int *minutos, int *verificadas);

private:
    QElapsedTimer m_vista;         // desde que se ve la pregunta actual
    QElapsedTimer m_sinParar;      // desde que se empezó, o desde la última pausa (propuesta o hecha)
    int m_desdeLaPausa = 0;        // las verificadas desde entonces
    int m_avisadaDePrisa = -1;     // índice de la pregunta de la que ya se ha avisado que se marcaba muy deprisa
    int m_pausaCada = 25 * 60 * 1000;
};
