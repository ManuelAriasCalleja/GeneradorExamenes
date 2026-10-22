#pragma once

#include "generador.h"
#include "maquetador.h"

#include <QDialog>
#include <QVector>

class QButtonGroup;
class QLabel;
class QPushButton;
class QTimer;

// Hacer un modelo de examen en pantalla y corregirlo.
class DialogoPractica : public QDialog
{
    Q_OBJECT
public:
    DialogoPractica(const Modelo &modelo, const DatosConvocatoria &datos, QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *objeto, QEvent *evento) override;

private slots:
    void corregir();
    void tic();

private:
    struct Fila
    {
        QButtonGroup *grupo = nullptr;
        QLabel *opciones[4] = {};
        QLabel *resultado = nullptr;
        QWidget *blanco = nullptr;
    };

    Modelo m_modelo;
    DatosConvocatoria m_datos;
    QVector<Fila> m_filas;
    QLabel *m_reloj;
    QLabel *m_nota;
    QPushButton *m_corregir;
    QTimer *m_temporizador;
    int m_segundosRestantes;
    bool m_corregido = false;
};
