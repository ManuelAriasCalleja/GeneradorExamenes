#pragma once

#include "pregunta.h"

#include <QDialog>
#include <QVector>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QRadioButton;
class QSpinBox;
class QTextBrowser;

// Alta y edición de una pregunta, con vista previa.
class DialogoPregunta : public QDialog
{
    Q_OBJECT
public:
    DialogoPregunta(const QVector<Tema> &temas, const Pregunta &pregunta, QWidget *parent = nullptr);

    Pregunta pregunta() const;

public slots:
    void accept() override;

private slots:
    void actualizarVistaPrevia();
    void cargarImagen();
    void quitarImagen();

private:
    void mostrarImagen();

    QVector<Tema> m_temas;
    Pregunta m_original;
    QByteArray m_imagen;

    QComboBox *m_tema;
    QSpinBox *m_numero;
    QLineEdit *m_variante;
    QComboBox *m_estado;
    QCheckBox *m_old;
    QCheckBox *m_revisar;
    QLineEdit *m_pseudo;
    QPlainTextEdit *m_enunciado;
    QLineEdit *m_opciones[4];
    QRadioButton *m_correcta[4];
    QLineEdit *m_referencia;
    QPlainTextEdit *m_explicacion;
    QLineEdit *m_notas;
    QLabel *m_vistaImagen;
    QTextBrowser *m_vistaPrevia;
    QLabel *m_avisosEstilo;
};
