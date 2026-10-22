#pragma once

#include "moodle.h"

#include <QDialog>
#include <QVector>

class QCheckBox;
class QLabel;
class QLineEdit;

// Exportar el banco a un cuestionario de Moodle (ver moodle.h): el fichero y las opciones.
class DialogoMoodle : public QDialog
{
    Q_OBJECT
public:
    DialogoMoodle(const QVector<Pregunta> &preguntas, const QString &asignatura, QWidget *parent = nullptr);

    QString ruta() const;
    Moodle::Opciones opciones() const;

public slots:
    void accept() override;

private:
    void elegirFichero();
    void actualizarCuenta();

    QVector<Pregunta> m_preguntas;
    QString m_asignatura;
    QLineEdit *m_ruta;
    QCheckBox *m_incluirRevisar;
    QCheckBox *m_penalizar;
    QLabel *m_cuenta;
};
