#pragma once

#include "pregunta.h"

#include <QDialog>
#include <QVector>

class BaseDatos;
class QTableWidget;

// Alta, baja y renombrado de temas.
class DialogoTemas : public QDialog
{
    Q_OBJECT
public:
    DialogoTemas(BaseDatos *bd, QWidget *parent = nullptr);

public slots:
    void accept() override;

private slots:
    void anadir();
    void eliminar();

private:
    void anadirFila(const Tema &t, int numPreguntas);

    BaseDatos *m_bd;
    QTableWidget *m_tabla;
    QVector<int> m_borrados;
};
