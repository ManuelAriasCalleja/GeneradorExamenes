#pragma once

#include "dialogopregunta.h"
#include "pregunta.h"

#include <QVector>
#include <QWidget>

class BaseDatos;
class FiltroPreguntas;
class ModeloPreguntas;
class QComboBox;
class QLabel;
class QLineEdit;
class QPoint;
class QProgressBar;
class QTableView;
class VisorAmpliable;
class ZonaAmpliable;

// La pestaña «Banco de preguntas» de la ventana: la lista de las preguntas con sus filtros, la ficha de la
// seleccionada y lo que se hace con ellas (crearlas, editarlas, duplicarlas, eliminarlas y verificarlas).
//
// Las preguntas y los temas se los da la ventana (ponerDatos()), que es quien los lee del banco. Cuando aquí se cambia
// algo del banco, se avisa con bancoEditado() o verificacionCambiada(): la ventana lo recarga y vuelve a llamar a
// ponerDatos() antes de que la señal termine (la conexión es directa), así que tras emitirla los datos ya son los
// nuevos.
class PestanaBanco : public QWidget
{
    Q_OBJECT
public:
    explicit PestanaBanco(BaseDatos *bd, QWidget *parent = nullptr);

    // Los temas y las preguntas del banco, recién leídos. Los filtros se quedan como están.
    void ponerDatos(const QVector<Tema> &temas, const QVector<Pregunta> &preguntas);
    // Selecciona en la lista la pregunta 'id' (si los filtros la dejan ver).
    void seleccionar(int id);
    // La zona con la que se amplían juntas la lista y la ficha (la crea la ventana, que es quien reparte Ctrl++…).
    void ponerZona(ZonaAmpliable *zona);
    // Pone en la ficha la pregunta seleccionada, tal como está ahora (con los exámenes en que ha salido).
    void actualizarFicha();
    // Abre la pregunta en el editor y la guarda. Con 'boton', el editor tiene además ese botón («Guardar y
    // siguiente»…). Devuelve lo que se ha pedido al guardar (Cerrar si no se ha guardado). En 'guardada' deja la
    // pregunta guardada (con id 0 si no se ha guardado).
    DialogoPregunta::Despues editar(Pregunta p, DialogoPregunta::Despues boton = DialogoPregunta::Cerrar,
                                    const QString &texto = QString(), Pregunta *guardada = nullptr);

public slots:
    void nuevaPregunta();
    void editarPregunta();
    void duplicarPregunta();
    void eliminarPregunta();
    void verificarPregunta();   // las de la lista, desde la seleccionada
    void verificarLista();      // marca como verificadas todas las de la lista
    void gestionarVerificadores();

signals:
    // Se ha guardado o borrado una pregunta: hay que recargar el banco (y seleccionar 'seleccionarId', si no es 0).
    void bancoEditado(int seleccionarId);
    // Ha cambiado la verificación de preguntas (ver VentanaPrincipal::verificacionCambiada()).
    void verificacionCambiada(int seleccionarId, bool conJson = true);
    // Algo que decir en la barra de estado, durante 'ms' milisegundos.
    void aviso(const QString &texto, int ms);

private:
    void filtrar();
    void ponerCuenta();
    Pregunta preguntaSeleccionada(bool *ok);
    // Las preguntas de la lista: las que cumplen los filtros, en el orden en que se ven.
    QVector<Pregunta> preguntasDeLaLista() const;
    // Menú del botón derecho en la lista de preguntas.
    void menuTabla(const QPoint &donde);

    BaseDatos *m_bd;
    QVector<Tema> m_temas;
    QVector<Pregunta> m_preguntas;
    int m_idGuardada = 0;                   // la última pregunta guardada en el editor
    bool m_filtroTemaRecordado = false;     // ya se ha puesto en el filtro el tema de la última vez
    QComboBox *m_filtroTema;
    QComboBox *m_filtroEstado;
    QLineEdit *m_filtroTexto;
    QLabel *m_cuenta;
    QProgressBar *m_barraVerificadas;   // de las preguntas de la lista, cuántas están verificadas
    QTableView *m_tabla;
    ModeloPreguntas *m_modelo;
    FiltroPreguntas *m_filtro;
    VisorAmpliable *m_ficha;
};
