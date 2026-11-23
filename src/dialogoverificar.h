#pragma once

#include "atencionalverificar.h"
#include "pregunta.h"

#include <QDialog>
#include <QHash>
#include <QVector>
#include <functional>

class BaseDatos;
class PanelVerificacion;
class QButtonGroup;
class QCheckBox;
class QLabel;
class QProgressBar;
class QPushButton;
class VisorAmpliable;

// Recorre unas preguntas (las de la lista del banco o las de un examen) para verificarlas una tras otra: se ve la ficha
// de cada una (con la respuesta correcta, la explicación y la referencia) y su verificación. «Verificada, siguiente»
// (Intro) la marca como verificada por los de la anterior y pasa a la siguiente; lo que se cambie se guarda al pasar a
// otra o al cerrar.
class DialogoVerificar : public QDialog
{
    Q_OBJECT
public:
    // 'preguntas', tal como están en el banco, en el orden en que se recorren, desde 'inicial'. 'editar' abre una en el
    // editor de preguntas y devuelve cómo ha quedado (con id 0 si no se ha guardado).
    DialogoVerificar(BaseDatos *bd, const QVector<Pregunta> &preguntas, int inicial, const QVector<Tema> &temas,
                     std::function<Pregunta(const Pregunta &)> editar = {}, QWidget *parent = nullptr);

    // Las preguntas que han cambiado (su verificación o, si se han editado, también lo demás), como han quedado.
    QVector<Pregunta> cambiadas() const;
    int actual() const { return m_actual; }
    // Si se ha guardado alguna con el editor.
    bool hayEditadas() const { return m_editadas; }

    // Cada cuánto se propone una pausa, en milisegundos (las pruebas lo acortan). Ver AtencionAlVerificar.
    void ponerPausaCada(int ms) { m_atencion.ponerPausaCada(ms); }

signals:
    // Se acaba de guardar en el banco la verificación de la pregunta 'id': quien ha abierto el diálogo puede poner al
    // día lo que se ve detrás (la lista, el examen) sin esperar a que se cierre.
    void verificacionGuardada(int id);

public slots:
    void done(int resultado) override;

private:
    void mostrar(int indice);
    // Guarda la verificación de la que se ve si ha cambiado; false si no se ha podido.
    bool guardar();
    void ir(int indice);
    void verificarYSeguir();
    void editarActual();
    // Cuántas de las preguntas que se recorren están ya verificadas, en número y en la barra.
    void ponerProgreso();
    bool conAvisos() const;
    // Con la solución oculta o a la vista: la ficha, el botón de Intro y lo que se puede marcar.
    void ponerOculta(bool oculta);
    // Enseña la solución y la compara con la respuesta que se haya dado.
    void mostrarSolucion();

    BaseDatos *m_bd;
    QVector<Pregunta> m_preguntas;
    QVector<Tema> m_temas;
    std::function<Pregunta(const Pregunta &)> m_editar;
    int m_actual = -1;
    QHash<int, int> m_cambiadas;   // id -> índice en m_preguntas
    bool m_editadas = false;
    AtencionAlVerificar m_atencion;   // los avisos de prisa y de pausa, que se quitan con la casilla
    QCheckBox *m_avisos;
    // Resolver la pregunta antes de ver la solución: si se ve desde el principio, se tiende a darla por buena.
    bool m_oculta = false;         // la solución de la pregunta actual
    QCheckBox *m_resolver;
    QWidget *m_filaRespuesta;
    QButtonGroup *m_respuesta;

    QLabel *m_posicion;
    QLabel *m_progreso;
    QProgressBar *m_barra;
    VisorAmpliable *m_ficha;
    PanelVerificacion *m_panel;
    QLabel *m_aviso;
    QPushButton *m_anterior;
    QPushButton *m_siguiente;
    QPushButton *m_verificarYSeguir;
    QPushButton *m_botonEditar;
};
