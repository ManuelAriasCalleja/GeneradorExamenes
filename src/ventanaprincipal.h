#pragma once

#include "basedatos.h"
#include "dialogopregunta.h"
#include "generador.h"
#include "maquetador.h"

#include <QHash>
#include <QMainWindow>
#include <QMap>
#include <QSet>
#include <QVector>

class FiltroPreguntas;
class ModeloPreguntas;
class QAction;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QPoint;
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
    void exportarMoodle();
    void actualizarJsonBanco();
    void cambiarJsonAutomatico(bool activar);
    void gestionarTemas();
    void revisarEstilo();
    void mostrarAyuda();
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
    void registrarAnteriores();
    void practicar();
    void cambiarPregunta();   // la del número elegido bajo la vista
    void deshacerCambio();

    // Banco de preguntas
    void filtrar();
    void seleccionCambiada();
    void nuevaPregunta();
    void editarPregunta();
    void duplicarPregunta();
    void eliminarPregunta();

private:
    QWidget *crearPestanaExamen();
    QWidget *crearGrupoConvocatoria();
    QWidget *crearGrupoExamenes();
    QWidget *crearGrupoTemas();
    QWidget *crearGrupoPuntuacion();
    QWidget *crearGrupoOpciones();
    QWidget *crearPanelResultados();
    QWidget *crearPestanaBanco();
    void crearMenus();
    void recargarDatos(int seleccionarId = 0);
    // Tras un cambio del banco hecho desde el programa (pregunta o tema guardado o borrado): reescribe el JSON
    // enlazado si se pidió mantenerlo al día (y antes del cambio coincidía) y recarga los datos.
    void bancoEditado(int seleccionarId = 0);
    // Compara el banco con su JSON enlazado y enseña (u oculta) el aviso de la barra de estado.
    void comprobarJson();
    void reconstruirTemas();
    void leerConfiguracion();
    void guardarConfiguracion();
    // Ajustes que se guardan tal cual en la configuración: su clave y el control que los muestra. El valor por
    // defecto es el que tiene el control al crearlo.
    struct Ajuste
    {
        QString clave;
        QWidget *control;
    };
    QVector<Ajuste> ajustes() const;
    bool hayExamen() const;
    Pregunta preguntaSeleccionada(bool *ok);
    // Abre la pregunta en el editor y la guarda. Con 'boton', el editor tiene además ese botón («Guardar y
    // siguiente»…). Devuelve lo que se ha pedido al guardar (Cerrar si no se ha guardado).
    DialogoPregunta::Despues editar(Pregunta p, DialogoPregunta::Despues boton = DialogoPregunta::Cerrar,
                                    const QString &texto = QString());
    // Muestra m_resultado (recién generado o cargado del historial); 'origen' va delante de la semilla.
    void presentarResultado(const QString &origen = QString());
    // Avisos, nivel de la maqueta y examen mostrado, según el resultado y la casilla «Letra grande».
    void actualizarMaqueta();
    // Enseña (en la línea gris y en el botón de registrar) si los exámenes están exportados y sin registrar.
    void actualizarRegistro();
    // Antes de sustituir o descartar los exámenes de la ventana (generar otros, cargar del historial, abrir otro
    // banco, salir): si se han exportado y no se han registrado, pregunta si registrarlos. Devuelve false si se
    // prefiere no seguir.
    bool puedenDescartarse();
    void cargarRegistrados(const QVector<int> &examenes);
    // Cambia la pregunta 'indice' del examen que se ve por otra (ver Generador::cambiarPregunta).
    void cambiar(int indice);
    // Menú del botón derecho en la vista del examen: el normal, con «Cambiar la pregunta N por otra» si se pulsa
    // sobre una pregunta y «Deshacer» si hay algún cambio.
    void menuVista(const QPoint &enVista);
    // Pregunta (desde 0) del examen que está en el punto 'enVista' de la vista, o -1 (cabecera, instrucciones).
    int preguntaEn(const QPoint &enVista) const;
    // Si se evitan, las preguntas de las últimas convocatorias registradas. Los exámenes 'examenes' de la convocatoria
    // de la ventana no cuentan: son los que se generan, que sustituirán a los registrados con su nombre.
    QHash<QString, int> usoReciente(const QStringList &examenes);
    DatosConvocatoria datosConvocatoria() const;
    ConfigExamen configuracionExamen() const;
    QString nombreBaseFichero() const;
    QStringList nombresModelos() const;
    QListWidgetItem *anadirExamen(const QString &nombre, bool marcado);
    QVector<Modelo> modelosDe(const QVector<int> &indices) const;
    // Nivel de la maqueta: el de letra grande si está marcada la casilla; si no, el de compactación con el que los
    // modelos generados caben en sus páginas (ver Maquetador).
    int nivelMaqueta() const;
    // Exporta los exámenes que se elijan en el diálogo, cada fichero con 'escribir' (ver exportacion.h); 'nota' se
    // añade al mensaje del final. Con 'casillaSoluciones', el diálogo tiene la casilla «Con soluciones» con ese texto,
    // marcada si lo está 'soluciones', y es ella la que decide.
    using Escritor = bool (*)(const QString &ruta, const QVector<Modelo> &modelos, const DatosConvocatoria &datos,
                              bool soluciones, int nivel);
    void exportar(const QString &titulo, const QString &nombreBase, const QString &extension, Escritor escribir,
                  bool soluciones, const QString &nota = QString(), const QString &casillaSoluciones = QString());
    void informarExportacion(const QStringList &escritos, const QStringList &fallidos, const QString &nota = QString());
    // Enseña en la barra de estado el botón que abre la carpeta donde se acaba de exportar (vacía: lo quita).
    void ofrecerCarpeta(const QString &carpeta);

    BaseDatos m_bd;
    QVector<Tema> m_temas;
    QVector<Pregunta> m_preguntas;
    int m_idGuardada = 0;                   // la última pregunta guardada en el editor
    ResultadoGeneracion m_resultado;
    QString m_origenResultado;              // «Registrada el …» si viene del historial
    bool m_semillaFija = false;             // m_resultado se generó con la semilla escrita en Opciones
    // Si m_resultado se ha exportado (o impreso) y si está registrado tal como está: si se ha exportado y no está
    // registrado, se pregunta antes de descartarlo.
    bool m_exportados = false;
    bool m_registrados = false;
    QString m_infoBase;                     // la línea gris, sin el aviso de que no está registrado
    // Para cambiar preguntas: la configuración con que se generó m_resultado (o, si viene del historial, la de la
    // ventana al cargarlo); los grupos quitados de cada examen, que no se vuelven a proponer mientras haya otros; y
    // los cambios hechos, para deshacerlos.
    ConfigExamen m_configResultado;
    QHash<int, QSet<QString>> m_quitadas;
    struct CambioPregunta
    {
        ResultadoGeneracion antes;
        int modelo;
        int indice;
    };
    QVector<CambioPregunta> m_cambios;
    QPushButton *m_avisoJson;               // en la barra de estado: el JSON enlazado no coincide con el banco
    QPushButton *m_abrirCarpeta;            // en la barra de estado, tras exportar: abre la carpeta de los ficheros
    QAction *m_accionJsonAuto;
    bool m_jsonCoincide = false;            // el JSON enlazado existe y coincide con el banco

    // Pestaña de examen
    QLineEdit *m_asignatura;
    QComboBox *m_convocatoria;
    QSpinBox *m_anio;
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
    QCheckBox *m_letraGrande;
    QLabel *m_correctasPorLetra;
    QLabel *m_avisos;
    QLabel *m_infoGeneracion;
    QTextBrowser *m_visorExamen;
    QList<QPushButton *> m_botonesExamen;
    QPushButton *m_botonRegistrar;
    QSpinBox *m_preguntaACambiar;
    QPushButton *m_deshacerCambio;

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
