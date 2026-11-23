#pragma once

#include "pregunta.h"

#include <QDialog>
#include <QHash>
#include <QStringList>
#include <QVector>
#include <QWidget>
#include <functional>

class BaseDatos;
class QCheckBox;
class QGridLayout;
class QLabel;
class QListWidget;
class QPushButton;
class VisorAmpliable;

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

// La lista de los verificadores del banco: añadirlos, cambiarles el nombre y quitarlos (los cambios se guardan al
// momento).
class DialogoVerificadores : public QDialog
{
    Q_OBJECT
public:
    explicit DialogoVerificadores(BaseDatos *bd, QWidget *parent = nullptr);

    bool hayCambios() const { return m_cambios; }
    // Los nombres cambiados: antes -> después.
    QHash<QString, QString> renombrados() const { return m_renombrados; }

private:
    void actualizar(const QString &seleccionar = QString());
    QString seleccionado() const;
    void anadir();
    void cambiarNombre();
    void quitar();

    BaseDatos *m_bd;
    QListWidget *m_lista;
    QPushButton *m_cambiarNombre;
    QPushButton *m_quitar;
    bool m_cambios = false;
    QHash<QString, QString> m_renombrados;
};

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

    BaseDatos *m_bd;
    QVector<Pregunta> m_preguntas;
    QVector<Tema> m_temas;
    std::function<Pregunta(const Pregunta &)> m_editar;
    int m_actual = -1;
    QHash<int, int> m_cambiadas;   // id -> índice en m_preguntas
    bool m_editadas = false;

    QLabel *m_posicion;
    VisorAmpliable *m_ficha;
    PanelVerificacion *m_panel;
    QLabel *m_aviso;
    QPushButton *m_anterior;
    QPushButton *m_siguiente;
    QPushButton *m_verificarYSeguir;
    QPushButton *m_botonEditar;
};
