#pragma once

#include "parecidas.h"
#include "pregunta.h"

#include <QDialog>
#include <QVector>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QRadioButton;
class QSpinBox;
class QTextBrowser;
class QTimer;

// Alta y edición de una pregunta, con vista previa, avisos de estilo y las preguntas más parecidas del banco
// ('banco', todas las preguntas, para buscarlas y para dar letra a una variante nueva).
class DialogoPregunta : public QDialog
{
    Q_OBJECT
public:
    DialogoPregunta(const QVector<Tema> &temas, const QVector<Pregunta> &banco, const Pregunta &pregunta,
                    QWidget *parent = nullptr);

    Pregunta pregunta() const;

public slots:
    void accept() override;
    void reject() override;   // con cambios sin guardar, pregunta antes de descartarlos (también con Esc o al cerrar)

private slots:
    void actualizarVistaPrevia();
    void actualizarParecidas();
    void cargarImagen();
    void quitarImagen();

private:
    QGroupBox *crearIdentificacion(const Pregunta &pregunta);
    QGroupBox *crearTextos(const Pregunta &pregunta);
    QGroupBox *crearImagen();
    QGroupBox *crearReferencia(const Pregunta &pregunta);
    QGroupBox *crearVistaPrevia();
    void conectar(const Pregunta &pregunta);
    void mostrarImagen();
    // Al guardar una pregunta nueva con un número que no existe y que se parece mucho a otras, propone hacerla
    // variante de una (o enlazarla como pseudorrepetida). Devuelve false si se prefiere seguir editando.
    bool decidirSiEsVariante();

    QVector<Tema> m_temas;
    QVector<Pregunta> m_banco;
    Parecidas::Indice m_indice;
    Pregunta m_original;
    Pregunta m_inicial;   // como estaba en el formulario al abrirlo, para saber si hay cambios
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
    QLabel *m_parecidas;
    QTimer *m_temporizadorParecidas;   // para no buscar en todo el banco con cada tecla
};
