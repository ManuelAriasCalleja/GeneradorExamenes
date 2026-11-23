#pragma once

#include "pregunta.h"

#include <QStringList>
#include <QVector>
#include <QWidget>

class BaseDatos;
class QCheckBox;
class QGridLayout;
class QLabel;

// Verificación de las preguntas por personas (Pregunta::verificada): las preguntas pueden estar redactadas por una IA
// y alguien tiene que revisarlas antes de ponerlas en un examen.

// La casilla «Verificada por humanos» y una casilla por cada verificador del banco, con un botón para añadirlos o
// quitarlos. Para verificar con pocos clics: al marcar «Verificada por humanos» sin nadie marcado, se marcan los que
// verificaron la última (BaseDatos::ultimosVerificadores()), que suelen ser los mismos; al marcar a alguien, se marca
// también «Verificada»; al desmarcarla, se desmarcan todos (y si se vuelve a marcar, vuelven los que había).
class PanelVerificacion : public QWidget
{
    Q_OBJECT
public:
    // Sin 'conCasilla', solo los verificadores (para marcar varias preguntas a la vez).
    explicit PanelVerificacion(BaseDatos *bd, bool conCasilla = true, QWidget *parent = nullptr);

    // La verificación de la pregunta.
    void ponerPregunta(const Pregunta &p);
    bool verificada() const;
    QStringList verificadores() const;
    // Si es lo que tenía la pregunta de ponerPregunta().
    bool sinCambios() const;
    // La marca como verificada; si no hay nadie marcado, por los últimos.
    void marcarVerificada();
    // Marca a estos (los que estén en la lista).
    void marcar(const QStringList &nombres);
    // Si está verificada por alguien, los recuerda como los últimos que han verificado (para proponerlos en la
    // siguiente).
    void recordar();
    // Si el banco tiene verificadores.
    bool hayVerificadores() const { return !m_casillas.isEmpty(); }

signals:
    void cambiado();
    // Se han añadido, quitado o cambiado de nombre verificadores (con el botón del panel).
    void verificadoresCambiados();

private:
    void construir(const QStringList &marcados);
    void gestionar();
    void verificadaCambiada(bool marcada);
    void personaCambiada(bool marcada);

    BaseDatos *m_bd;
    QCheckBox *m_verificada = nullptr;
    QGridLayout *m_rejilla;
    QVector<QCheckBox *> m_casillas;
    QLabel *m_ninguno;
    bool m_inicialVerificada = false;
    QStringList m_inicialVerificadores;
    QStringList m_antes;    // los marcados antes de desmarcar «Verificada»
    bool m_cambiando = false;
};
