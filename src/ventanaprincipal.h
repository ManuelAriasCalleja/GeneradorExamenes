#pragma once

#include "basedatos.h"
#include "generador.h"
#include "maquetador.h"

#include <QMainWindow>
#include <QMap>
#include <QVector>

class FiltroPreguntas;
class ModeloPreguntas;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QPrinter;
class QPushButton;
class QSpinBox;
class QTableView;
class QTextBrowser;

class VentanaPrincipal : public QMainWindow
{
    Q_OBJECT
public:
    explicit VentanaPrincipal(QWidget *parent = nullptr);
    ~VentanaPrincipal() override;

    bool abrirBaseDatos(const QString &ruta);
    // Al arrancar: abre el banco indicado en la línea de órdenes o, si no, el último que se abrió (lo guarda el
    // fichero de configuración). Si no lo encuentra, pregunta cuál abrir o si crear uno. Devuelve false si se
    // prefiere salir.
    bool abrirBancoInicial(const QString &rutaIndicada);

protected:
    void closeEvent(QCloseEvent *evento) override;

private slots:
    // Archivo
    void abrirOtraBaseDatos();
    void nuevaBaseDatos();
    void importarJson();
    void exportarJson();
    void gestionarTemas();
    void acercaDe();

    // Generar examen
    void aplicarReparto(int primerTema, int ultimoTema);
    void actualizarTotal();
    void actualizarPuntuacion();
    void nuevoExamen();
    void quitarExamen();
    void generar();
    void mostrarModelo();
    void vistaPreviaImpresion();
    void exportarPdf();
    void exportarSolucionesPdf();
    void exportarHtml();
    void exportarLatex();
    void registrarConvocatoria();
    void historialConvocatorias();
    void practicar();

    // Banco de preguntas
    void filtrar();
    void seleccionCambiada();
    void nuevaPregunta();
    void editarPregunta();
    void duplicarPregunta();
    void eliminarPregunta();

private:
    QWidget *crearPestanaExamen();
    QWidget *crearPestanaBanco();
    void crearMenus();
    void recargarDatos(int seleccionarId = 0);
    void reconstruirTemas();
    void leerConfiguracion();
    void guardarConfiguracion();
    void ponerDocumento(QTextBrowser *visor, QTextDocument *doc);
    bool hayExamen() const;
    Pregunta preguntaSeleccionada(bool *ok);
    void editar(Pregunta p);
    // Muestra m_resultado (recién generado o cargado del historial); 'origen' va delante de la semilla.
    void presentarResultado(const QString &origen = QString());
    void cargarRegistrados(const QVector<int> &examenes);
    DatosConvocatoria datosConvocatoria() const;
    ConfigExamen configuracionExamen() const;
    QString nombreBaseFichero() const;
    QStringList nombresModelos() const;
    QListWidgetItem *anadirExamen(const QString &nombre, bool marcado);
    QVector<Modelo> modelosDe(const QVector<int> &indices) const;
    // Nivel de compactación de la maqueta con el que los modelos generados caben en sus páginas (ver Maquetador).
    int nivelCompactacion() const;
    bool escribirHtml(const QString &ruta, const QVector<Modelo> &modelos, int nivel);
    bool escribirLatex(const QString &ruta, const QVector<Modelo> &modelos, int nivel);
    void informarExportacion(const QStringList &escritos, const QStringList &fallidos, const QString &nota = QString());

    BaseDatos m_bd;
    QVector<Tema> m_temas;
    QVector<Pregunta> m_preguntas;
    ResultadoGeneracion m_resultado;

    // Pestaña de examen
    QLineEdit *m_asignatura;
    QComboBox *m_convocatoria;
    QSpinBox *m_anio;
    QSpinBox *m_duracion;
    QListWidget *m_listaExamenes;           // nombres de los exámenes; se generan los marcados
    QPlainTextEdit *m_instrucciones;
    QGridLayout *m_rejillaTemas;
    QWidget *m_contenedorTemas;
    QMap<int, QSpinBox *> m_cantidades;     // número de tema -> preguntas
    QMap<int, int> m_cantidadesGuardadas;
    QLabel *m_total;
    QCheckBox *m_puntuacionAuto;
    QDoubleSpinBox *m_acierto;
    QDoubleSpinBox *m_fallo;
    QCheckBox *m_noRepetir;
    QCheckBox *m_evitarRecientes;
    QSpinBox *m_numRecientes;
    QCheckBox *m_evitarOld;
    QCheckBox *m_incluirRevisar;
    QCheckBox *m_ordenAleatorio;
    QCheckBox *m_barajarOpciones;
    QLineEdit *m_semilla;
    QComboBox *m_selectorModelo;
    QCheckBox *m_verSoluciones;
    QLabel *m_correctasPorLetra;
    QLabel *m_avisos;
    QLabel *m_infoGeneracion;
    QTextBrowser *m_visorExamen;
    QList<QPushButton *> m_botonesExamen;

    // Pestaña del banco
    QComboBox *m_filtroTema;
    QComboBox *m_filtroEstado;
    QLineEdit *m_filtroTexto;
    QLabel *m_cuenta;
    QTableView *m_tabla;
    ModeloPreguntas *m_modelo;
    FiltroPreguntas *m_filtro;
    QTextBrowser *m_ficha;
};
