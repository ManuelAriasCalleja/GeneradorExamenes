#pragma once

#include "generador.h"
#include "maquetador.h"
#include "pregunta.h"

#include <QMap>
#include <QStringList>
#include <QVariantMap>
#include <QVector>
#include <QWidget>
#include <functional>

class BaseDatos;
class GrupoPlegable;
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
class QSpinBox;

// El panel izquierdo de la pestaña «Generar examen»: lo que se elige antes de generar. La convocatoria (con la
// asignatura y el idioma de los exámenes, que se guardan en el banco), la lista de exámenes, las preguntas de cada
// tema (con los repartos guardados en el banco), la puntuación y las opciones.
//
// La ventana le pregunta lo elegido (datos(), configuracion()…) y le da los temas y las preguntas del banco
// (ponerDatos()). Lo que aquí se cambia y afecta a lo demás se avisa con señales.
class PanelConfiguracion : public QWidget
{
    Q_OBJECT
public:
    explicit PanelConfiguracion(BaseDatos *bd, QWidget *parent = nullptr);
    ~PanelConfiguracion() override;

    // --- Lo elegido
    // Los datos de la convocatoria para la maqueta. Sin la semilla, que es de los exámenes generados.
    DatosConvocatoria datos() const;
    // Con qué generar: el reparto, los exámenes marcados y las opciones. Sin el uso reciente de las preguntas.
    ConfigExamen configuracion() const;
    // Solo los exámenes marcados, con sus versiones (los campos de ConfigExamen de los exámenes).
    ConfigExamen configuracionExamenes() const;
    QString idiomaExamen() const;
    // Los nombres de la lista de exámenes y, en 'marcados', los marcados.
    QStringList examenes(QStringList *marcados = nullptr) const;
    bool evitaRecientes() const;
    int numRecientes() const;
    bool limitaPaginas() const;
    QString semilla() const;     // vacía: aleatoria
    void quitarSemilla();

    // --- El banco
    // Tras abrir un banco (o cambiarlo desde fuera del panel): su asignatura, su idioma y el último reparto que se usó
    // con él. Si no hay ninguno, el que guardaban en la configuración las versiones anteriores o el primero del banco.
    void ponerAsignaturaDelBanco();
    void ponerIdiomaDelBanco();
    void ponerUltimoReparto();
    // Los temas y las preguntas del banco, recién leídos: rehace las casillas de los temas y los botones de los repartos.
    void ponerDatos(const QVector<Tema> &temas, const QVector<Pregunta> &preguntas);
    // Lo que va con el banco abierto y no se guarda al momento: el nombre de la asignatura, si se está cambiando, y el
    // reparto del panel (el último usado). Antes de cerrarlo (al salir o abrir otro).
    void guardarEstadoBanco();
    // Los datos de una convocatoria registrada pasan al panel, para que lo que se exporte salga como entonces.
    void ponerConvocatoria(const DatosConvocatoria &d);

    // --- La configuración
    // Las opciones del panel, en la configuración de cada usuario (QSettings).
    void leerConfiguracion();
    void guardarConfiguracion();
    // Las que van con cada banco (BaseDatos::opciones()): las de ajustes() y la lista de exámenes. Las que un banco
    // no tenga (uno de una versión anterior) se quedan como están.
    QVariantMap opciones() const;
    void aplicarOpciones(const QVariantMap &opciones);

signals:
    // Se ha guardado en el banco el nombre de la asignatura.
    void asignaturaGuardada();
    // Se ha guardado en el banco algo que va en su JSON (la asignatura o el idioma).
    void bancoEditado();
    // Ha cambiado algo que se ve en la maqueta de los exámenes ya generados (la cabecera, el pie, el ajuste a páginas).
    void maquetaCambiada();
    void semillaCambiada();

private:
    QWidget *crearGrupoConvocatoria();
    QWidget *crearGrupoExamenes();
    QWidget *crearGrupoTemas();
    QWidget *crearGrupoPuntuacion();
    QWidget *crearGrupoOpciones();
    void guardarAsignatura();     // si se ha cambiado en el panel, en el banco
    void cambiarIdiomaExamen();   // el elegido en el panel, en el banco
    // Al cambiar el idioma de los exámenes (de 'antes' al del panel), lo que se propone en él: las convocatorias de la
    // lista, la elegida si es una de ellas y las instrucciones si son las de por defecto.
    void adaptarAlIdiomaExamen(const QString &antes);
    void reconstruirTemas();
    // Botones de los repartos guardados en el banco.
    void reconstruirRepartos();
    // Menú del botón derecho de un reparto del banco (en 'donde', en coordenadas de la pantalla): sustituirlo por el
    // del panel o quitarlo.
    void menuReparto(const QString &nombre, const QPoint &donde);
    // Las preguntas de cada tema en el panel (solo los temas con alguna).
    QMap<int, int> repartoActual() const;
    void aplicarReparto(const QMap<int, int> &reparto);
    void guardarReparto();        // el del panel, como un reparto del banco con nombre (un botón más)
    void actualizarTotal();
    void actualizarPuntuacion();
    QListWidgetItem *anadirExamen(const QString &nombre, bool marcado, int versiones = 1);
    void nuevoExamen();
    void quitarExamen();
    // Sube (-1) o baja (+1) en la lista el examen elegido.
    void moverExamen(int desplazamiento);
    // Las versiones de los exámenes de la lista que tienen más de una: nombre -> cuántas.
    QVariantMap versionesDeLaLista() const;
    bool versionesConNumeros() const;   // las versiones, «1, 2, 3…»; si no, «A, B, C…»
    // El título del recuadro de la lista de exámenes: «Exámenes: 2 modelos de 3 seleccionados.».
    void actualizarCuentaExamenes();
    // Lo que casi nunca se cambia va plegado, con un resumen de lo elegido en la cabecera.
    void resumirPuntuacion();
    void resumirOpciones();
    // Ajustes que se guardan tal cual en la configuración: su clave y el control que los muestra. El valor por
    // defecto es el que tiene el control al crearlo.
    struct Ajuste
    {
        QString clave;
        QWidget *control;
    };
    QVector<Ajuste> ajustes() const;
    // Las opciones del recuadro «Opciones», en el orden en que se ven: la clave con que se guarda (vacía: no se
    // guarda), su control y lo que dice el resumen del recuadro plegado cuando no está como viene de entrada (vacío
    // si lo está; sin función, el resumen no la nombra).
    struct Opcion
    {
        QString clave;
        QWidget *control;
        std::function<QString()> cambio;
    };
    QVector<Opcion> m_opciones;

    BaseDatos *m_bd;
    QVector<Tema> m_temas;
    QVector<Pregunta> m_preguntas;
    QLineEdit *m_asignatura;
    QComboBox *m_idiomaExamen;              // el del banco (BaseDatos::idioma())
    QComboBox *m_convocatoria;
    QSpinBox *m_anio;
    QListWidget *m_listaExamenes;           // nombres de los exámenes; se generan los marcados
    QGroupBox *m_grupoExamenes;             // su recuadro, que dice cuántos hay marcados
    QSpinBox *m_versiones;                  // las del examen elegido en la lista
    QWidget *m_numeracion;                  // «A, B, C…» o «1, 2, 3…»: un botón de opción por forma (con su «valor»)
    QPlainTextEdit *m_instrucciones;
    // Si van la asignatura y la convocatoria en la cabecera y en el pie de página de los exámenes.
    QCheckBox *m_asignaturaEnCabecera;
    QCheckBox *m_convocatoriaEnCabecera;
    QCheckBox *m_versionEnCabecera;         // «Versión B», en los exámenes con versiones
    QCheckBox *m_asignaturaEnPie;
    QCheckBox *m_convocatoriaEnPie;
    QGridLayout *m_rejillaTemas;
    QWidget *m_contenedorTemas;
    QMap<int, QSpinBox *> m_cantidades;     // número de tema -> preguntas
    QMap<int, int> m_cantidadesGuardadas;
    QGridLayout *m_rejillaRepartos;         // botones de los repartos del banco y «Ninguna»
    QLabel *m_total;
    QWidget *m_puntuacion;                  // los botones de las formas de puntuar (DatosConvocatoria::Puntuacion)
    QButtonGroup *m_formasPuntuacion;       // los mismos, con la forma como id
    QDoubleSpinBox *m_acierto;
    QDoubleSpinBox *m_fallo;
    QCheckBox *m_noRepetir;
    QCheckBox *m_evitarRecientes;
    QSpinBox *m_numRecientes;
    QCheckBox *m_evitarOld;
    QCheckBox *m_incluirRevisar;
    QCheckBox *m_soloVerificadas;           // generar solo con preguntas verificadas por personas
    QCheckBox *m_ordenAleatorio;
    QCheckBox *m_barajarOpciones;
    QCheckBox *m_limitarPaginas;            // compactar la maqueta para que quepa en una página por cada 10 preguntas
    QCheckBox *m_enunciadoEnCursiva;        // si no, los enunciados van solo en negrita
    QLineEdit *m_semilla;
    GrupoPlegable *m_plegablePuntuacion = nullptr;
    GrupoPlegable *m_plegableOpciones = nullptr;
};
