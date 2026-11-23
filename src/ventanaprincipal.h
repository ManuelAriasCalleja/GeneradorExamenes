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
#include <functional>

class QAction;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGridLayout;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QPoint;
class QPrinter;
class QPushButton;
class QSpinBox;
class QTextBrowser;
class VisorAmpliable;
class ZonaAmpliable;
class GrupoPlegable;
class PanelConfiguracion;
class PestanaBanco;

class VentanaPrincipal : public QMainWindow
{
    Q_OBJECT
public:
    explicit VentanaPrincipal(QWidget *parent = nullptr);
    ~VentanaPrincipal() override;

    // La versión del programa y, en el ejecutable (main.cpp pone en la aplicación las propiedades «fechaCompilacion»
    // y «commitCompilacion»), cuándo se compiló y de qué commit de git: lo que dice «Acerca de».
    static QString versionYCompilacion();

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
    void comprobarFicheroJson();   // Archivo → Comprobar un JSON…
    void guardarJsonEjemplo();
    void exportarJson();
    void exportarMoodle();
    void actualizarJsonBanco();
    void cambiarJsonAutomatico(bool activar);
    void gestionarTemas();
    void revisarEstilo();
    void mostrarAyuda();
    void acercaDe();

    void cambiarIdioma(const QString &codigo);   // de la interfaz (Editar → Idioma); vacío: el del sistema

    // Generar examen
    void generar();
    void mostrarModelo();
    void vistaPreviaImpresion();
    void exportarPdf();
    void exportarSolucionesPdf();
    void exportarHtml();
    void exportarLatex();
    void exportarDocx();
    void registrarConvocatoria();
    void historialConvocatorias();
    void registrarAnteriores();
    void practicar();
    void editarPreguntaExamen();   // la del número elegido bajo la vista
    void verificarPreguntaExamen();   // desde la del número elegido bajo la vista
    void cambiarPregunta();   // la del número elegido bajo la vista
    void deshacerCambio();

private:
    QWidget *crearPestanaExamen();
    QWidget *crearPanelResultados();
    void crearMenus();
    void recargarDatos(int seleccionarId = 0);
    // Tras un cambio del banco hecho desde el programa (pregunta o tema guardado o borrado): reescribe el JSON
    // enlazado si se pidió mantenerlo al día (y antes del cambio coincidía) y recarga los datos.
    void bancoEditado(int seleccionarId = 0);
    // Compara el banco con su JSON enlazado y enseña (u oculta) el aviso de la barra de estado.
    void comprobarJson();
    // «Generador de exámenes - <asignatura del banco>».
    void actualizarTitulo();
    void leerConfiguracion();
    // Guarda las opciones de la ventana en la configuración y, las que van con él (ver opcionesDelBanco()), en el banco
    // abierto. Al generar, al exportar, al cambiar de banco y al salir.
    void guardarConfiguracion();
    // Las opciones de la ventana que van con cada banco (BaseDatos::opciones()): las del panel de configuración
    // (PanelConfiguracion::opciones()) y las carpetas de las exportaciones, relativas a la del banco (como el JSON enlazado).
    QVariantMap opcionesDelBanco() const;
    // Pone en la ventana las opciones de un banco. Las que no tenga (un banco de una versión anterior) se quedan como
    // están, salvo las carpetas si 'carpetasPropias': entonces, la del banco, para no exportar los exámenes de una
    // asignatura en la carpeta de otra.
    void aplicarOpcionesDelBanco(const QVariantMap &opciones, bool carpetasPropias);
    bool hayExamen() const;
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
    // Abre en el editor la pregunta 'indice' del examen que se ve y, si se guarda, la corrige también en los exámenes
    // (ver Generador::ponerCorregida). Solo así cambian las preguntas de los exámenes generados al editarlas: los
    // cambios hechos en la pestaña del banco no les llegan, como a los registrados en el historial.
    void editarDelExamen(int indice);
    // La pregunta del banco que corresponde a la copia de un examen: la del mismo id y código; si no está, la del mismo
    // código (que no cambia al reimportar el JSON), y si tampoco, la del mismo id. Con id 0 si ya no está.
    Pregunta preguntaDelBanco(const Pregunta &copia) const;
    // Pone en los exámenes la pregunta 'guardada', que era 'antes' (ver Generador::ponerCorregida()). Devuelve en
    // cuántos sitios de los exámenes estaba.
    int corregirEnExamenes(const Pregunta &antes, const Pregunta &guardada);
    // Recorre las preguntas del examen que se ve para verificarlas, desde la 'indice' (ver DialogoVerificar).
    void verificarDelExamen(int indice);
    // Tras cambiar la verificación de preguntas: pone al día el JSON enlazado, la lista (seleccionando 'seleccionarId')
    // y la verificación de las preguntas de los exámenes, que se ve en sus marcas.
    // Pone al día la lista y los exámenes con la verificación del banco. Sin 'conJson' (mientras se va verificando,
    // pregunta a pregunta) no se reescribe el JSON del banco: se hace una vez, al acabar.
    void verificacionCambiada(int seleccionarId, bool conJson = true);
    // Pone en las preguntas de 'resultado' la verificación que tienen en el banco. Las que ya no dicen lo mismo que en el
    // banco (un examen del historial, si se han corregido después) se quedan con la suya: la del banco no certifica su texto.
    void ponerVerificacionDelBanco(ResultadoGeneracion &resultado) const;
    // Menú del botón derecho en la vista del examen: el normal, con «Editar la pregunta N» y «Cambiar la pregunta N
    // por otra» si se pulsa sobre una pregunta, «Deshacer» si hay algún cambio y los de ampliar y reducir la vista.
    void menuVista(const QPoint &enVista);
    // Amplía (pasos > 0), reduce (< 0) o deja a su tamaño normal (0) la zona en la que está el puntero del ratón; si
    // no está en ninguna, la vista del examen o el banco de preguntas, según la pestaña.
    void cambiarEscala(int pasos);
    ZonaAmpliable *crearZona(QWidget *raiz, const QString &clave);
    // Pregunta (desde 0) del examen que está en el punto 'enVista' de la vista, o -1 (cabecera, instrucciones).
    int preguntaEn(const QPoint &enVista) const;
    // Si se evitan, las preguntas de las últimas convocatorias registradas. Los exámenes 'examenes' de la convocatoria
    // de la ventana no cuentan: son los que se generan, que sustituirán a los registrados con su nombre.
    QHash<QString, int> usoReciente(const QStringList &examenes);
    DatosConvocatoria datosConvocatoria() const;
    QSet<QString> gruposDelBanco() const;   // «T1-7» de las preguntas del banco abierto
    QString nombreBaseFichero() const;
    QStringList nombresModelos() const;
    QVector<Modelo> modelosDe(const QVector<int> &indices) const;
    // Nivel de la maqueta: el de letra grande si está marcada la casilla; si no, el de compactación con el que los
    // modelos generados caben en sus páginas (ver Maquetador), o el normal si no hay que ajustarlos a ellas.
    int nivelMaqueta() const;
    static QString textoConSoluciones();
    // Exporta los exámenes que se elijan en el diálogo, cada fichero con 'escribir' (ver exportacion.h), con los nombres
    // que empiezan por nombreBaseFichero(); 'nota' se añade al mensaje del final. Con 'casillaSoluciones', el diálogo
    // tiene la casilla «Con soluciones» con ese texto, marcada si lo está 'soluciones', y es ella la que decide.
    using Escritor = bool (*)(const QString &ruta, const QVector<Modelo> &modelos, const DatosConvocatoria &datos,
                              bool soluciones, int nivel);
    void exportar(const QString &titulo, const QString &extension, Escritor escribir, bool soluciones,
                  const QString &nota = QString(), const QString &casillaSoluciones = QString());
    void informarExportacion(const QStringList &escritos, const QStringList &fallidos, const QString &nota = QString());
    // Enseña en la barra de estado el botón que abre la carpeta donde se acaba de exportar (vacía: lo quita).
    void ofrecerCarpeta(const QString &carpeta);

    BaseDatos m_bd;
    QVector<Tema> m_temas;
    QVector<Pregunta> m_preguntas;
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
    PanelConfiguracion *m_panel = nullptr;  // lo que se elige antes de generar
    QComboBox *m_selectorModelo;
    QCheckBox *m_verSoluciones;
    QCheckBox *m_letraGrande;
    QLabel *m_correctasPorLetra;
    QLabel *m_avisos;
    QLabel *m_infoGeneracion;
    VisorAmpliable *m_visorExamen;
    QList<QPushButton *> m_botonesExamen;
    QPushButton *m_botonRegistrar;
    QSpinBox *m_preguntaACambiar;
    QPushButton *m_deshacerCambio;

    PestanaBanco *m_banco = nullptr;        // la pestaña «Banco de preguntas»
    // Lo que se amplía con Ctrl + rueda, Ctrl++, Ctrl+- y Ctrl+0, cada cosa con su tamaño: la vista del examen
    // (m_visorExamen), el panel izquierdo de «Generar examen» y el banco de preguntas (la lista y la ficha).
    ZonaAmpliable *m_zonaPanel = nullptr;
    ZonaAmpliable *m_zonaBanco = nullptr;
};
