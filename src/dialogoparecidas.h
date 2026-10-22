#pragma once

#include "parecidas.h"

#include <QDialog>
#include <QVector>

class QListWidget;
class QPushButton;
class QTextBrowser;

// Al guardar una pregunta nueva que se parece mucho a otras del banco: las muestra, con su ficha al lado para
// compararlas, y deja guardarla como variante de la elegida (si es del mismo tema), enlazarla con ella como
// pseudorrepetida (si es de otro), guardarla como pregunta nueva o seguir editando.
class DialogoParecidas : public QDialog
{
    Q_OBJECT
public:
    enum Decision { SeguirEditando, ComoVariante, ComoPseudorrepetida, ComoNueva };

    DialogoParecidas(const Pregunta &nueva, const QVector<Parecidas::Parecida> &parecidas, const QVector<Tema> &temas,
                     QWidget *parent = nullptr);

    Decision decision() const { return m_decision; }
    Pregunta elegida() const;

private:
    void mostrarElegida();
    void decidir(Decision decision);

    Pregunta m_nueva;
    QVector<Parecidas::Parecida> m_parecidas;
    QVector<Tema> m_temas;
    Decision m_decision = SeguirEditando;
    QListWidget *m_lista;
    QTextBrowser *m_ficha;
    QPushButton *m_variante;
    QPushButton *m_pseudorrepetida;
};
