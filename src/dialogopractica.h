#pragma once

#include "generador.h"
#include "maquetador.h"

#include <QDialog>
#include <QVector>

class QButtonGroup;
class QLabel;
class QPushButton;
class QScrollArea;
class QSpinBox;
class QTimer;

// Hacer un modelo de examen en pantalla y corregirlo. Se elige la duración y el reloj empieza al pulsar «Empezar»;
// hasta entonces no se puede contestar.
class DialogoPractica : public QDialog
{
    Q_OBJECT
public:
    DialogoPractica(const Modelo &modelo, const DatosConvocatoria &datos, int duracionMinutos, QWidget *parent = nullptr);

    // La duración elegida, para recordarla la próxima vez.
    int duracionMinutos() const;

protected:
    bool eventFilter(QObject *objeto, QEvent *evento) override;

private slots:
    void empezar();
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

    QWidget *crearPregunta(int numero, const PreguntaExamen &pe);

    Modelo m_modelo;
    DatosConvocatoria m_datos;
    QVector<Fila> m_filas;
    QSpinBox *m_duracion;
    QPushButton *m_empezar;
    QLabel *m_reloj;
    QLabel *m_nota;
    QScrollArea *m_preguntas;
    QPushButton *m_corregir;
    QTimer *m_temporizador;
    int m_segundosRestantes = 0;
    bool m_empezado = false;
    bool m_corregido = false;
};
