#include "ventanaprincipal.h"
#include "paleta.h"
#include "dialogoverificadores.h"
#include "dialogoverificar.h"
#include "panelverificacion.h"
#include "dialogoanteriores.h"
#include "dialogoayuda.h"
#include "dialogocomprobacion.h"
#include "dialogoestilo.h"
#include "dialogoexportar.h"
#include "dialogohistorial.h"
#include "dialogomoodle.h"
#include "dialogopractica.h"
#include "dialogopregunta.h"
#include "dialogotemas.h"
#include "exportacion.h"
#include "idioma.h"
#include "panelconfiguracion.h"
#include "pestanabanco.h"
#include "utilidades.h"
#include "visorampliable.h"
#include "zonaampliable.h"
#include "grupoplegable.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDate>
#include <QDesktopServices>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIntValidator>
#include <QKeySequence>
#include <QLabel>
#include <QProgressBar>
#include <QLineEdit>
#include <QListWidget>
#include <QDateTime>
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QProcess>
#include <QPlainTextEdit>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableView>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextDocument>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>
#include <memory>

namespace {

// Zona con desplazamiento vertical que es siempre tan ancha como lo que lleva dentro, para que se vea entero sin
// redimensionar: con la letra, el idioma, los temas y los repartos que haya (se vuelve a medir cada vez que cambian).
class ZonaVertical : public QScrollArea
{
public:
    QSize minimumSizeHint() const override { return conAnchoDelContenido(QScrollArea::minimumSizeHint()); }
    QSize sizeHint() const override { return conAnchoDelContenido(QScrollArea::sizeHint()); }

protected:
    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (o == widget() && e->type() == QEvent::LayoutRequest)
            updateGeometry();
        return QScrollArea::eventFilter(o, e);
    }

private:
    // Con sitio para la barra de desplazamiento, aunque no se vea: así cabe también cuando aparece.
    QSize conAnchoDelContenido(QSize s) const
    {
        if (widget())
            s.setWidth(widget()->minimumSizeHint().width() + verticalScrollBar()->sizeHint().width() + 2 * frameWidth());
        return s;
    }
};

// Carpetas de los diálogos (las guardan ellos en la configuración) que van con cada banco: la de las exportaciones
// (exámenes y Moodle) y la de los PDF de los exámenes anteriores.
const QStringList kCarpetasDelBanco{QStringLiteral("exportar/carpeta"), QStringLiteral("anteriores/carpeta")};

// Claves de la configuración de cada usuario (QSettings) que se usan en más de un sitio.
const QString kClaveBanco = QStringLiteral("bd/ruta");                    // el último banco que se abrió
const QString kClaveGeometria = QStringLiteral("ventana/geometria");
const QString kClaveEscalaVista = QStringLiteral("vista/escala");         // el tamaño de la vista del examen
const QString kClaveDuracionPractica = QStringLiteral("practica/duracion");

} // namespace

VentanaPrincipal::VentanaPrincipal(QWidget *parent)
    : QMainWindow(parent)
{
    actualizarTitulo();
    auto *pestanas = new QTabWidget;
    pestanas->addTab(crearPestanaExamen(), tr("Generar examen"));
    // La pestaña del banco pide a la ventana lo que afecta a lo demás: recargar el banco tras un cambio (y poner al día
    // su JSON), llevar la verificación a los exámenes y los avisos de la barra de estado.
    m_banco = new PestanaBanco(&m_bd);
    connect(m_banco, &PestanaBanco::bancoEditado, this, &VentanaPrincipal::bancoEditado);
    connect(m_banco, &PestanaBanco::verificacionCambiada, this, &VentanaPrincipal::verificacionCambiada);
    connect(m_banco, &PestanaBanco::aviso, this, [this](const QString &texto, int ms) { statusBar()->showMessage(texto, ms); });
    m_zonaBanco = crearZona(m_banco, QStringLiteral("vista/escalaBanco"));
    m_banco->ponerZona(m_zonaBanco);
    pestanas->addTab(m_banco, tr("Banco de preguntas"));
    setCentralWidget(pestanas);
    crearMenus();
    // Ctrl++, Ctrl+- y Ctrl+0, en toda la ventana: amplían la zona en la que está el puntero del ratón, como Ctrl +
    // rueda. Las teclas son las de las acciones del visor, que se quedan sin ellas (solo para su menú).
    const QList<QAction *> delVisor = m_visorExamen->accionesEscala();
    const int pasos[] = {1, -1, 0};
    for (int i = 0; i < delVisor.size(); ++i) {
        auto *a = new QAction(this);
        a->setShortcuts(delVisor.at(i)->shortcuts());
        connect(a, &QAction::triggered, this, [this, n = pasos[i]] { cambiarEscala(n); });
        addAction(a);
    }
    m_visorExamen->quitarAtajos();
    // Aviso de que el JSON del banco no coincide con la base de datos; pulsándolo se actualiza.
    m_avisoJson = new QPushButton;
    m_avisoJson->setFlat(true);
    m_avisoJson->setCursor(Qt::PointingHandCursor);
    m_avisoJson->setStyleSheet(Paleta::recuadroAtencion() + QStringLiteral("; padding: 1px 6px"));
    m_avisoJson->hide();
    connect(m_avisoJson, &QPushButton::clicked, this, &VentanaPrincipal::actualizarJsonBanco);
    statusBar()->addPermanentWidget(m_avisoJson);
    m_abrirCarpeta = new QPushButton(tr("Abrir la carpeta"));
    m_abrirCarpeta->setFlat(true);
    m_abrirCarpeta->setCursor(Qt::PointingHandCursor);
    m_abrirCarpeta->setStyleSheet(Paleta::recuadroInformacion() + QStringLiteral("; padding: 1px 6px"));
    m_abrirCarpeta->hide();
    connect(m_abrirCarpeta, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_abrirCarpeta->property("carpeta").toString()));
    });
    statusBar()->addPermanentWidget(m_abrirCarpeta);
    leerConfiguracion();
}

VentanaPrincipal::~VentanaPrincipal()
{
}

// ---------------------------------------------------------------------------------------------
// Construcción de la interfaz

QWidget *VentanaPrincipal::crearPestanaExamen()
{
    // --- Panel de configuración. Avisa de lo que afecta a lo demás: el nombre de la asignatura (va en el título de la
    // ventana) y el idioma se guardan en el banco, y hay opciones que cambian la maqueta de los exámenes que se ven.
    m_panel = new PanelConfiguracion(&m_bd);
    connect(m_panel, &PanelConfiguracion::asignaturaGuardada, this, &VentanaPrincipal::actualizarTitulo);
    connect(m_panel, &PanelConfiguracion::bancoEditado, this, [this]() { bancoEditado(); });
    connect(m_panel, &PanelConfiguracion::maquetaCambiada, this, [this]() {
        if (hayExamen())
            actualizarMaqueta();
    });

    auto *scroll = new ZonaVertical;
    scroll->setWidget(m_panel);
    scroll->setWidgetResizable(true);

    // El botón, debajo y fuera de la zona con desplazamiento: siempre a la vista, aunque la pantalla sea baja.
    auto *bGenerar = new QPushButton(tr("Generar exámenes"));
    bGenerar->setMinimumHeight(36);
    QFont f = bGenerar->font();
    f.setBold(true);
    bGenerar->setFont(f);
    connect(bGenerar, &QPushButton::clicked, this, &VentanaPrincipal::generar);
    auto *izquierda = new QWidget;
    auto *li = new QVBoxLayout(izquierda);
    li->setContentsMargins(0, 0, 0, 0);
    li->addWidget(scroll, 1);
    li->addWidget(bGenerar);

    auto *divisor = new QSplitter;
    divisor->addWidget(izquierda);
    divisor->addWidget(crearPanelResultados());
    divisor->setStretchFactor(1, 1);
    divisor->setSizes({430, 800});
    m_zonaPanel = crearZona(izquierda, QStringLiteral("vista/escalaPanel"));
    return divisor;
}

QWidget *VentanaPrincipal::crearPanelResultados()
{
    auto *resultados = new QWidget;
    auto *lr = new QVBoxLayout(resultados);
    auto *barra = new QHBoxLayout;
    m_selectorModelo = new QComboBox;
    m_selectorModelo->setMinimumWidth(110);
    m_selectorModelo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_verSoluciones = new QCheckBox(tr("Mostrar soluciones"));
    //: Versión del examen en letra grande, adaptada para estudiantes con discapacidad visual
    m_letraGrande = new QCheckBox(tr("Letra grande"));
    m_letraGrande->setToolTip(tr("Versión adaptada para estudiantes con discapacidad visual: letra de 16 pt, una "
                                 "opción por línea (enunciados sin cursiva), más separación e imágenes más grandes, sin límite de páginas.\n"
                                 "Vale para la vista, la impresión y las exportaciones; los ficheros llevan "
                                 "«-letra-grande» en el nombre."));
    m_infoGeneracion = new QLabel;
    m_infoGeneracion->setStyleSheet(Paleta::css(Paleta::Secundario));
    m_infoGeneracion->setTextInteractionFlags(Qt::TextSelectableByMouse);
    //: Delante de la lista para elegir cuál de los exámenes generados se ve
    barra->addWidget(new QLabel(tr("Examen:")));
    barra->addWidget(m_selectorModelo);
    barra->addWidget(m_verSoluciones);
    barra->addWidget(m_letraGrande);
    m_correctasPorLetra = new QLabel;
    m_correctasPorLetra->setToolTip(tr("Cuántas veces es correcta cada letra en este examen"));
    barra->addSpacing(12);
    barra->addWidget(m_correctasPorLetra);
    barra->addStretch();
    lr->addLayout(barra);
    // Semilla, origen (si se ha cargado del historial) y nivel de la maqueta, en su propia línea porque no caben en
    // la de arriba. Si la ventana es estrecha se corta, en lugar de ensancharla.
    m_infoGeneracion->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    lr->addWidget(m_infoGeneracion);
    m_avisos = new QLabel;
    m_avisos->setWordWrap(true);
    m_avisos->setObjectName(QStringLiteral("avisosExamen"));
    m_avisos->setStyleSheet(Paleta::recuadroAtencion());
    m_avisos->setTextFormat(Qt::RichText);
    m_avisos->hide();
    // Lo que pide una decisión se queda aquí, junto al examen, hasta que se resuelve (en la barra de estado se va a
    // los pocos segundos y es fácil no verlo), con el remedio a un clic cuando lo hay.
    connect(m_avisos, &QLabel::linkActivated, this, [this](const QString &enlace) {
        if (enlace == QLatin1String("quitar-semilla"))
            m_panel->quitarSemilla();
    });
    connect(m_panel, &PanelConfiguracion::semillaCambiada, this, [this]() {
        if (hayExamen() && m_semillaFija)
            actualizarMaqueta();
    });
    lr->addWidget(m_avisos);
    m_visorExamen = new VisorAmpliable;
    m_visorExamen->setObjectName(QStringLiteral("visorExamen"));
    m_visorExamen->setHtml(QStringLiteral("<p style=\"color:%1\">%2</p>")
                               .arg(QLatin1String(Paleta::kPapelSecundario),
                                    tr("Elija cuántas preguntas quiere de cada tema y pulse <b>Generar exámenes</b>.")));
    // El tamaño de la vista (Ctrl + rueda, Ctrl++, Ctrl+-, Ctrl+0) se recuerda de una vez a otra.
    m_visorExamen->ponerEscala(QSettings().value(kClaveEscalaVista, 1.0).toDouble());
    connect(m_visorExamen, &VisorAmpliable::escalaCambiada, this, [this](qreal escala) {
        QSettings().setValue(kClaveEscalaVista, escala);
        //: En la barra de estado, al ampliar o reducir la vista del examen; %1 es el porcentaje (125)
        statusBar()->showMessage(tr("Vista del examen al %1 %").arg(qRound(escala * 100)), 3000);
    });
    m_visorExamen->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_visorExamen, &QWidget::customContextMenuRequested, this, &VentanaPrincipal::menuVista);
    lr->addWidget(m_visorExamen, 1);
    // Dos filas de botones (exportar; cambiar preguntas, registrar y practicar) para que la ventana quepa en
    // pantallas pequeñas.
    auto *exportar = new QHBoxLayout;
    auto *otras = new QHBoxLayout;
    auto boton = [&](QHBoxLayout *fila, const QString &texto, void (VentanaPrincipal::*slot)()) {
        auto *b = new QPushButton(texto);
        b->setEnabled(false);
        connect(b, &QPushButton::clicked, this, slot);
        fila->addWidget(b);
        m_botonesExamen << b;
        return b;
    };
    boton(exportar, tr("Imprimir…"), &VentanaPrincipal::vistaPreviaImpresion);
    boton(exportar, tr("Exportar PDF…"), &VentanaPrincipal::exportarPdf);
    boton(exportar, tr("Soluciones PDF…"), &VentanaPrincipal::exportarSolucionesPdf);
    boton(exportar, tr("Exportar HTML…"), &VentanaPrincipal::exportarHtml);
    boton(exportar, tr("Exportar LaTeX…"), &VentanaPrincipal::exportarLatex);
    //: Botón: guarda los exámenes en un documento de Word (.docx)
    boton(exportar, tr("Exportar Word…"), &VentanaPrincipal::exportarDocx);
    exportar->addStretch();
    m_preguntaACambiar = new QSpinBox;
    m_preguntaACambiar->setEnabled(false);
    m_preguntaACambiar->setToolTip(tr("Número de la pregunta que se quiere editar o cambiar, en el examen que se ve.\n"
                                      "También se puede pulsar con el botón derecho sobre la pregunta."));
    //: Delante del número de la pregunta del examen que se quiere editar o cambiar
    otras->addWidget(new QLabel(tr("Pregunta:")));
    otras->addWidget(m_preguntaACambiar);
    //: Botón: abre en el editor esa pregunta del examen
    boton(otras, tr("Editar…"), &VentanaPrincipal::editarPreguntaExamen)
        ->setToolTip(tr("Abre la pregunta en el editor. Lo que se guarde queda en el banco y se ve ya en el examen, "
                        "con las opciones en el mismo orden."));
    //: Botón: recorre las preguntas del examen, desde esa, para marcarlas como verificadas por personas
    boton(otras, tr("Verificar…"), &VentanaPrincipal::verificarPreguntaExamen)
        ->setToolTip(tr("Recorre las preguntas del examen, desde esta, para marcarlas como verificadas por personas: "
                        "«Verificada, siguiente» la marca con los mismos verificadores que la anterior."));
    //: Botón: cambia esa pregunta del examen por otra del mismo tema
    boton(otras, tr("Cambiar por otra"), &VentanaPrincipal::cambiarPregunta)
        ->setToolTip(tr("Sustituye la pregunta por otra del mismo tema, en el mismo sitio y con la correcta en la "
                        "misma letra, con las mismas reglas que al generar. El resto de los exámenes no cambia."));
    m_deshacerCambio = new QPushButton(tr("Deshacer el cambio"));
    m_deshacerCambio->setEnabled(false);
    connect(m_deshacerCambio, &QPushButton::clicked, this, &VentanaPrincipal::deshacerCambio);
    otras->addWidget(m_deshacerCambio);
    otras->addStretch();
    m_botonRegistrar = boton(otras, tr("Registrar convocatoria…"), &VentanaPrincipal::registrarConvocatoria);
    boton(otras, tr("Practicar este examen…"), &VentanaPrincipal::practicar);
    lr->addLayout(exportar);
    lr->addLayout(otras);

    connect(m_selectorModelo, &QComboBox::currentIndexChanged, this, &VentanaPrincipal::mostrarModelo);
    connect(m_verSoluciones, &QCheckBox::toggled, this, &VentanaPrincipal::mostrarModelo);
    connect(m_letraGrande, &QCheckBox::toggled, this, &VentanaPrincipal::actualizarMaqueta);
    return resultados;
}

ZonaAmpliable *VentanaPrincipal::crearZona(QWidget *raiz, const QString &clave)
{
    // El tamaño de cada zona se recuerda de una vez a otra, como el de la vista del examen.
    auto *zona = new ZonaAmpliable(raiz, this);
    zona->ponerEscala(QSettings().value(clave, 1.0).toDouble());
    connect(zona, &ZonaAmpliable::escalaCambiada, this, [this, clave](qreal escala) {
        QSettings().setValue(clave, escala);
        //: En la barra de estado, al ampliar o reducir un panel de la ventana; %1 es el porcentaje (125)
        statusBar()->showMessage(tr("Tamaño de la letra al %1 %").arg(qRound(escala * 100)), 3000);
    });
    return zona;
}

void VentanaPrincipal::cambiarEscala(int pasos)
{
    const QWidget *bajoElPuntero = QApplication::widgetAt(QCursor::pos());
    const auto *pestanas = qobject_cast<QTabWidget *>(centralWidget());
    ZonaAmpliable *zona = nullptr;
    if (m_zonaPanel->contiene(bajoElPuntero))
        zona = m_zonaPanel;
    else if (m_zonaBanco->contiene(bajoElPuntero) || (pestanas && pestanas->currentIndex() == 1))
        zona = m_zonaBanco;
    if (zona) {
        if (pasos > 0)
            zona->ampliar();
        else if (pasos < 0)
            zona->reducir();
        else
            zona->tamanoNormal();
    } else if (pasos > 0) {
        m_visorExamen->ampliar();
    } else if (pasos < 0) {
        m_visorExamen->reducir();
    } else {
        m_visorExamen->tamanoNormal();
    }
}

void VentanaPrincipal::crearMenus()
{
    QMenu *archivo = menuBar()->addMenu(tr("&Archivo"));
    archivo->addAction(tr("&Abrir banco de preguntas…"), this, &VentanaPrincipal::abrirOtraBaseDatos);
    archivo->addAction(tr("&Nuevo banco de preguntas…"), this, &VentanaPrincipal::nuevaBaseDatos);
    archivo->addSeparator();
    archivo->addAction(tr("&Importar preguntas (JSON)…"), this, &VentanaPrincipal::importarJson);
    //: Menú Archivo. El & marca la letra del atajo: no puede ser la de otra orden del menú.
    archivo->addAction(tr("Importar las &verificaciones (JSON)…"), this, &VentanaPrincipal::importarVerificaciones)
        ->setToolTip(tr("Añade las verificaciones de un JSON (exportado de otra copia del banco, en otro ordenador) a "
                        "las preguntas que son iguales en este banco, sin cambiar nada más ni quitar ninguna"));
    archivo->addAction(tr("&Comprobar un JSON…"), this, &VentanaPrincipal::comprobarFicheroJson)
        ->setToolTip(tr("Dice los errores de un JSON de preguntas, con su línea, sin importarlo"));
    archivo->addAction(tr("Guardar un JSON de e&jemplo…"), this, &VentanaPrincipal::guardarJsonEjemplo)
        ->setToolTip(tr("Un JSON con unas preguntas inventadas, para ver cómo se escribe uno"));
    archivo->addAction(tr("&Exportar preguntas (JSON)…"), this, &VentanaPrincipal::exportarJson);
    archivo->addAction(tr("Exportar a &Moodle (XML)…"), this, &VentanaPrincipal::exportarMoodle);
    archivo->addSeparator();
    QAction *actualizarJson = archivo->addAction(tr("Actuali&zar el JSON del banco…"), this, &VentanaPrincipal::actualizarJsonBanco);
    actualizarJson->setToolTip(tr("Sobrescribe el JSON enlazado con el banco (la primera vez pide cuál es) con el "
                                  "contenido de la base de datos"));
    m_accionJsonAuto = archivo->addAction(tr("&Mantener el JSON del banco al día"));
    m_accionJsonAuto->setCheckable(true);
    m_accionJsonAuto->setToolTip(tr("Tras cada cambio en el banco (pregunta o tema guardado o borrado) se reescribe "
                                    "el JSON enlazado, si antes coincidía con la base de datos. Un JSON que ya no "
                                    "coincida (por ejemplo, recién generado desde el .docx) no se toca: se avisa."));
    archivo->setToolTipsVisible(true);
    connect(m_accionJsonAuto, &QAction::toggled, this, &VentanaPrincipal::cambiarJsonAutomatico);
    archivo->addSeparator();
    archivo->addAction(tr("&Historial de convocatorias…"), this, &VentanaPrincipal::historialConvocatorias);
    archivo->addAction(tr("&Registrar exámenes anteriores…"), this, &VentanaPrincipal::registrarAnteriores)
        ->setToolTip(tr("Registra en el historial los exámenes de una convocatoria hecha fuera del programa (de "
                        "sus PDF o su texto), para que se eviten sus preguntas al generar"));
    archivo->addSeparator();
    // En macOS, «Salir» y «Acerca de…» van en el menú de la aplicación. Qt los reconoce por su texto en inglés: en
    // castellano hay que decírselo.
    archivo->addAction(tr("&Salir"), QKeySequence::Quit, this, &QWidget::close)->setMenuRole(QAction::QuitRole);

    QMenu *editar = menuBar()->addMenu(tr("&Editar"));
    editar->addAction(tr("&Temas…"), this, &VentanaPrincipal::gestionarTemas);
    editar->addAction(tr("&Nueva pregunta…"), QKeySequence::New, m_banco, &PestanaBanco::nuevaPregunta);
    editar->addAction(tr("&Revisar el estilo del banco…"), this, &VentanaPrincipal::revisarEstilo);
    //: Menú: la lista de personas que verifican las preguntas del banco
    editar->addAction(tr("&Verificadores…"), m_banco, &PestanaBanco::gestionarVerificadores);
    editar->addSeparator();
    //: Menú para elegir el idioma de la interfaz del programa (menús, diálogos, ayuda)
    QMenu *menuIdioma = editar->addMenu(tr("&Idioma"));
    auto *grupoIdioma = new QActionGroup(menuIdioma);
    auto anadirIdioma = [&](const QString &texto, const QString &codigo) {
        QAction *a = menuIdioma->addAction(texto);
        a->setCheckable(true);
        a->setChecked(codigo.isEmpty() ? !Idioma::disponible(Idioma::elegido()) : Idioma::elegido() == codigo);
        a->setData(codigo);
        grupoIdioma->addAction(a);
        connect(a, &QAction::triggered, this, [this, codigo]() { cambiarIdioma(codigo); });
    };
    //: Opción del menú Idioma: el idioma del sistema; %1 es su nombre («Español», «English»)
    anadirIdioma(tr("El del sistema (%1)").arg(Idioma::nombre(Idioma::delSistema())), QString());
    menuIdioma->addSeparator();
    for (const Idioma::Disponible &d : Idioma::disponibles())
        anadirIdioma(d.nombre, d.codigo);

    QMenu *ayuda = menuBar()->addMenu(tr("A&yuda"));
    ayuda->addAction(tr("&Manual de uso"), QKeySequence::HelpContents, this, &VentanaPrincipal::mostrarAyuda);
    ayuda->addSeparator();
    ayuda->addAction(tr("&Acerca de…"), this, &VentanaPrincipal::acercaDe)->setMenuRole(QAction::AboutRole);
}

// ---------------------------------------------------------------------------------------------
// Base de datos

bool VentanaPrincipal::abrirBaseDatos(const QString &ruta)
{
    // Las carpetas de la configuración son las del último banco abierto: las toma el que no tenga las suyas solo si
    // es ese (el primero que se abre tras pasar a esta versión, que guarda las opciones en el banco).
    const QString ultimo = QSettings().value(kClaveBanco).toString();
    const bool carpetasPropias = ultimo.isEmpty() || QFileInfo(ruta).absoluteFilePath() != QFileInfo(ultimo).absoluteFilePath();
    if (m_bd.abierta()) {
        m_panel->guardarEstadoBanco();
        guardarConfiguracion();
    }
    if (!m_bd.abrir(ruta)) {
        QMessageBox::critical(this, windowTitle(),
                              tr("No se pudo abrir la base de datos\n%1\n\n%2").arg(ruta, m_bd.ultimoError()));
        return false;
    }
    m_resultado = ResultadoGeneracion();
    m_cambios.clear();
    m_quitadas.clear();
    m_selectorModelo->clear();
    m_correctasPorLetra->clear();
    for (QPushButton *b : m_botonesExamen)
        b->setEnabled(false);
    m_preguntaACambiar->setEnabled(false);
    m_deshacerCambio->setEnabled(false);
    actualizarRegistro();
    m_panel->ponerAsignaturaDelBanco();
    m_panel->ponerIdiomaDelBanco();
    actualizarTitulo();
    aplicarOpcionesDelBanco(m_bd.opciones(), carpetasPropias);
    m_panel->ponerUltimoReparto();
    recargarDatos();
    QSettings().setValue(kClaveBanco, ruta);
    // Ya, para que la configuración tenga las opciones de este banco y él las suyas aunque el programa no se cierre bien.
    guardarConfiguracion();
    return true;
}

bool VentanaPrincipal::abrirBancoInicial(const QString &rutaIndicada)
{
    const QString ruta = rutaIndicada.isEmpty() ? QSettings().value(kClaveBanco).toString()
                                                : QFileInfo(rutaIndicada).absoluteFilePath();
    // El de la línea de órdenes se crea si no existe; el de la configuración, no (se habrá movido).
    if (!ruta.isEmpty() && (!rutaIndicada.isEmpty() || QFileInfo::exists(ruta)) && abrirBaseDatos(ruta))
        return true;
    while (!m_bd.abierta()) {
        QMessageBox caja(QMessageBox::Question, windowTitle(),
                         ruta.isEmpty() ? tr("Elija el banco de preguntas (fichero .db) con el que quiere trabajar.")
                                        : tr("No se encuentra el banco de preguntas\n%1\n\n"
                                             "Elija dónde está o cree uno nuevo.").arg(QDir::toNativeSeparators(ruta)),
                         QMessageBox::NoButton, this);
        QPushButton *abrir = caja.addButton(tr("Abrir banco…"), QMessageBox::AcceptRole);
        QPushButton *nuevo = caja.addButton(tr("Nuevo banco…"), QMessageBox::ActionRole);
        caja.addButton(tr("Salir"), QMessageBox::RejectRole);
        caja.exec();
        if (caja.clickedButton() == abrir)
            abrirOtraBaseDatos();
        else if (caja.clickedButton() == nuevo)
            nuevaBaseDatos();
        else
            return false;
    }
    return true;
}

void VentanaPrincipal::abrirOtraBaseDatos()
{
    if (!puedenDescartarse())
        return;
    const QString ruta = QFileDialog::getOpenFileName(this, tr("Abrir banco de preguntas"),
                                                      carpetaBancos(m_bd.ruta(), QSettings().value(kClaveBanco).toString()),
                                                      tr("Bases de datos SQLite (*.db *.sqlite);;Todos (*)"));
    if (!ruta.isEmpty())
        abrirBaseDatos(ruta);
}

void VentanaPrincipal::nuevaBaseDatos()
{
    if (!puedenDescartarse())
        return;
    const QString ruta = QFileDialog::getSaveFileName(this, tr("Nuevo banco de preguntas"),
                                                      carpetaBancos(m_bd.ruta(), QSettings().value(kClaveBanco).toString())
                                                          + QStringLiteral("/preguntas_nueva.db"),
                                                      tr("Bases de datos SQLite (*.db)"));
    if (ruta.isEmpty())
        return;
    if (QFileInfo::exists(ruta)) {
        if (ruta == m_bd.ruta()) {
            QMessageBox::warning(this, windowTitle(), tr("Esa base de datos está abierta; elija otro nombre."));
            return;
        }
        QFile::remove(ruta);
    }
    if (!abrirBaseDatos(ruta))
        return;
    QString asignatura;
    if (pedirTexto(this, tr("Nuevo banco de preguntas"),
                   tr("Nombre de la asignatura del banco (va en la cabecera de los exámenes):"), &asignatura)
        && !asignatura.isEmpty()) {
        m_bd.guardarAsignatura(asignatura);
        m_panel->ponerAsignaturaDelBanco();
        actualizarTitulo();
    }
    if (confirmar(this, windowTitle(), tr("El banco nuevo está vacío. ¿Quiere importar ahora preguntas de un JSON?")))
        importarJson();
}

void VentanaPrincipal::importarJson()
{
    const QString ruta = QFileDialog::getOpenFileName(this, tr("Importar preguntas"), QString(),
                                                      QStringLiteral("JSON (*.json)"));
    if (ruta.isEmpty())
        return;
    // Antes, la comprobación: con errores no se importa; con avisos, si se quiere.
    if (!ComprobadorJson::comprobarFichero(ruta, gruposDelBanco()).problemas.isEmpty()) {
        DialogoComprobacion comprobacion(ruta, gruposDelBanco(), DialogoComprobacion::AntesDeImportar, this);
        if (comprobacion.exec() != QDialog::Accepted)
            return;
    }
    QMessageBox caja(QMessageBox::Question, windowTitle(),
                     tr("¿Cómo quiere importar las preguntas?\n\n"
                        "Añadir/actualizar: las preguntas con el mismo tema, número y variante se sobrescriben; el resto se conservan.\n"
                        "Reemplazar todo: se borran antes todos los temas y preguntas.\n\n"
                        "La verificación de las preguntas que no cambian se conserva, aunque el JSON no la tenga. Antes "
                        "de importar se guarda una copia de seguridad de la base de datos en la carpeta «copias», junto "
                        "a ella."),
                     QMessageBox::NoButton, this);
    QPushButton *anadir = caja.addButton(tr("Añadir/actualizar"), QMessageBox::AcceptRole);
    QPushButton *reemplazar = caja.addButton(tr("Reemplazar todo"), QMessageBox::DestructiveRole);
    caja.addButton(tr("Cancelar"), QMessageBox::RejectRole);
    caja.exec();
    if (caja.clickedButton() != anadir && caja.clickedButton() != reemplazar)
        return;
    QString copia;
    ResumenVerificacion verificacion;
    const QString asignaturaAntes = m_bd.asignatura();
    const int n = m_bd.importarJson(ruta, caja.clickedButton() == reemplazar, &copia, &verificacion);
    if (n < 0) {
        QMessageBox::warning(this, windowTitle(), tr("Error al importar:\n%1").arg(m_bd.ultimoError()));
        return;
    }
    // El JSON del que viene todo el banco pasa a ser su copia en texto; si solo se han añadido preguntas, el que
    // hubiera enlazado sigue siéndolo.
    if (caja.clickedButton() == reemplazar || m_bd.jsonEnlazado().isEmpty())
        m_bd.enlazarJson(ruta);
    if (m_bd.asignatura() != asignaturaAntes) {   // el banco no tenía nombre y lo trae el JSON
        m_panel->ponerAsignaturaDelBanco();
        actualizarTitulo();
    }
    m_panel->ponerIdiomaDelBanco();   // por si el banco no decía el idioma y lo trae el JSON
    recargarDatos();
    QString mensaje = tr("Se han importado %1 preguntas.").arg(n);
    if (verificacion.conservadas > 0)
        mensaje += tr("\n\nVerificaciones conservadas (el JSON las da por no verificadas, pero no han cambiado): %1.")
                       .arg(verificacion.conservadas);
    if (verificacion.anadidas > 0)
        mensaje += tr("\n\nPreguntas con verificaciones nuevas del JSON: %1.").arg(verificacion.anadidas);
    if (verificacion.quitadas > 0)
        mensaje += tr("\n\nVerificaciones quitadas porque la pregunta ha cambiado: %1.").arg(verificacion.quitadas);
    if (!copia.isEmpty())
        mensaje += tr("\n\nAntes de importar se ha guardado una copia de seguridad del banco en:\n%1")
                       .arg(QDir::toNativeSeparators(copia));
    QMessageBox::information(this, windowTitle(), mensaje);
}

void VentanaPrincipal::importarVerificaciones()
{
    const QString ruta = QFileDialog::getOpenFileName(this, tr("Importar las verificaciones de un JSON"), QString(),
                                                      QStringLiteral("JSON (*.json)"));
    if (ruta.isEmpty())
        return;
    ResumenVerificacion r;
    const int n = m_bd.importarVerificaciones(ruta, &r);
    if (n < 0) {
        QMessageBox::warning(this, windowTitle(), tr("No se pudieron importar las verificaciones:\n%1").arg(m_bd.ultimoError()));
        return;
    }
    // Los códigos de las que no se han tocado, para revisarlas a mano (los primeros, si son muchos).
    auto codigos = [](const QStringList &lista) {
        constexpr int kMaximo = 20;
        QString texto = lista.mid(0, kMaximo).join(QStringLiteral(", "));
        if (lista.size() > kMaximo)
            texto += QStringLiteral(", …");
        return texto;
    };
    QString mensaje = tr("Preguntas con verificaciones nuevas: %1.").arg(n);
    if (r.yaEstaban > 0)
        mensaje += tr("\n\nYa tenían esas verificaciones: %1.").arg(r.yaEstaban);
    if (!r.distintas.isEmpty())
        mensaje += tr("\n\nNo se han tocado porque en el banco no son iguales que en el JSON (%1): %2.")
                       .arg(r.distintas.size()).arg(codigos(r.distintas));
    if (!r.noEstan.isEmpty())
        mensaje += tr("\n\nNo están en el banco (%1): %2.").arg(r.noEstan.size()).arg(codigos(r.noEstan));
    QMessageBox::information(this, windowTitle(), mensaje);
    if (n > 0)
        verificacionCambiada(0);
}

void VentanaPrincipal::comprobarFicheroJson()
{
    const QString ruta = QFileDialog::getOpenFileName(this, tr("Comprobar un JSON de preguntas"), QString(),
                                                      QStringLiteral("JSON (*.json)"));
    if (ruta.isEmpty())
        return;
    DialogoComprobacion dialogo(ruta, gruposDelBanco(), DialogoComprobacion::Informe, this);
    dialogo.exec();
}

void VentanaPrincipal::guardarJsonEjemplo()
{
    // El de la interfaz: el ejemplo en inglés si está en inglés.
    const QString origen = Idioma::interfaz() == QLatin1String("en") ? QStringLiteral(":/recursos/ejemplo_banco_en.json")
                                                                     : QStringLiteral(":/recursos/ejemplo_banco.json");
    const QString ruta = QFileDialog::getSaveFileName(this, tr("Guardar un JSON de ejemplo"),
                                                      QDir(carpetaBancos(m_bd.ruta(), QString())).filePath(tr("ejemplo_preguntas.json")),
                                                      QStringLiteral("JSON (*.json)"));
    if (ruta.isEmpty())
        return;
    QFile::remove(ruta);   // el diálogo ya ha preguntado si se sustituye
    QFile f(origen);
    if (!f.copy(ruta) || !QFile::setPermissions(ruta, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther)) {
        QMessageBox::warning(this, windowTitle(), tr("No se pudo guardar el ejemplo en\n%1").arg(QDir::toNativeSeparators(ruta)));
        return;
    }
    QMessageBox::information(this, windowTitle(),
                             tr("Se ha guardado el ejemplo en\n%1\n\nÁbralo con un editor de textos; en uno que "
                                "entienda los esquemas JSON, como VS Code, se ven los errores mientras se escribe. El formato "
                                "está explicado en la ayuda (F1, «Formato del JSON»).").arg(QDir::toNativeSeparators(ruta)));
}

QSet<QString> VentanaPrincipal::gruposDelBanco() const
{
    QSet<QString> grupos;
    for (const Pregunta &p : m_preguntas)
        grupos.insert(p.grupo());
    return grupos;
}

void VentanaPrincipal::exportarJson()
{
    const QString ruta = QFileDialog::getSaveFileName(this, tr("Exportar preguntas"),
                                                      QStringLiteral("preguntas_%1.json").arg(QDate::currentDate().toString(Qt::ISODate)),
                                                      QStringLiteral("JSON (*.json)"));
    if (ruta.isEmpty())
        return;
    if (!m_bd.exportarJson(ruta))
        QMessageBox::warning(this, windowTitle(), m_bd.ultimoError());
    else
        statusBar()->showMessage(tr("Preguntas exportadas a %1").arg(ruta), 5000);
}

void VentanaPrincipal::exportarMoodle()
{
    DialogoMoodle dialogo(m_preguntas, m_panel->datos().asignatura, this);
    if (dialogo.exec() != QDialog::Accepted)
        return;
    guardarConfiguracion();   // la carpeta, en el banco
    Moodle::Opciones opciones = dialogo.opciones();
    opciones.idioma = m_panel->idiomaExamen();
    QString error;
    if (!Moodle::exportar(dialogo.ruta(), m_temas, m_preguntas, opciones, &error)) {
        QMessageBox::warning(this, windowTitle(), error);
        return;
    }
    statusBar()->showMessage(tr("%1 preguntas exportadas a %2 (en Moodle: Banco de preguntas → Importar, formato «Moodle XML»)")
                                 .arg(Moodle::seleccion(m_preguntas, opciones).size())
                                 .arg(QDir::toNativeSeparators(dialogo.ruta())),
                             10000);
    ofrecerCarpeta(QFileInfo(dialogo.ruta()).absolutePath());
}

void VentanaPrincipal::actualizarJsonBanco()
{
    QString json = m_bd.jsonEnlazado();
    if (json.isEmpty()) {
        json = QFileDialog::getSaveFileName(this, tr("JSON del banco de preguntas"),
                                            QFileInfo(m_bd.ruta()).absolutePath() + QStringLiteral("/preguntas.json"),
                                            QStringLiteral("JSON (*.json)"), nullptr, QFileDialog::DontConfirmOverwrite);
        if (json.isEmpty())
            return;
        m_bd.enlazarJson(json);
    }
    if (QFileInfo::exists(json)) {
        const QStringList diferencias = m_bd.diferenciasConJson(json);
        if (diferencias.isEmpty()) {
            comprobarJson();
            QMessageBox::information(this, windowTitle(), tr("El JSON del banco ya coincide con la base de datos:\n%1")
                                                              .arg(QDir::toNativeSeparators(json)));
            return;
        }
        if (!confirmar(this, tr("Actualizar el JSON del banco"),
                       tr("El JSON del banco no coincide con la base de datos:\n%1\n\n%2\n\n"
                          "¿Sobrescribirlo con el contenido de la base de datos?")
                           .arg(QDir::toNativeSeparators(json), diferencias.join(QLatin1Char('\n'))))) {
            comprobarJson();
            return;
        }
    }
    if (!m_bd.exportarJson(json))
        QMessageBox::warning(this, windowTitle(), tr("No se pudo escribir el JSON:\n%1").arg(m_bd.ultimoError()));
    else
        statusBar()->showMessage(tr("JSON del banco actualizado: %1").arg(QDir::toNativeSeparators(json)), 8000);
    comprobarJson();
}

void VentanaPrincipal::cambiarJsonAutomatico(bool activar)
{
    m_bd.fijarJsonAutomatico(activar);
    // Para mantenerlo al día hay que partir de un JSON igual que el banco: se elige (si no lo hay) y se actualiza.
    if (activar && !m_jsonCoincide)
        actualizarJsonBanco();
    comprobarJson();
}

void VentanaPrincipal::comprobarJson()
{
    m_accionJsonAuto->blockSignals(true);
    m_accionJsonAuto->setChecked(m_bd.jsonAutomatico());
    m_accionJsonAuto->blockSignals(false);
    const QString json = m_bd.jsonEnlazado();
    const QStringList diferencias = json.isEmpty() ? QStringList() : m_bd.diferenciasConJson(json);
    m_jsonCoincide = !json.isEmpty() && diferencias.isEmpty();
    m_avisoJson->setVisible(!diferencias.isEmpty());
    if (diferencias.isEmpty())
        return;
    m_avisoJson->setText(tr("El JSON del banco no coincide con la base de datos: actualizarlo…"));
    m_avisoJson->setToolTip(tr("%1\n\n%2\n\nPulse para sobrescribir el JSON con el contenido de la base de datos.")
                                .arg(QDir::toNativeSeparators(json), diferencias.join(QLatin1Char('\n'))));
}

void VentanaPrincipal::bancoEditado(int seleccionarId)
{
    if (m_bd.jsonAutomatico() && m_jsonCoincide) {
        const QString json = m_bd.jsonEnlazado();
        if (!json.isEmpty() && !m_bd.exportarJson(json))
            QMessageBox::warning(this, windowTitle(), tr("No se pudo actualizar el JSON del banco:\n%1").arg(m_bd.ultimoError()));
    }
    recargarDatos(seleccionarId);
}

void VentanaPrincipal::gestionarTemas()
{
    DialogoTemas d(&m_bd, this);
    if (d.exec() == QDialog::Accepted)
        bancoEditado();
}

void VentanaPrincipal::revisarEstilo()
{
    DialogoEstilo dialogo([this]() { return m_preguntas; },
                          [this](int id) {
                              for (const Pregunta &p : std::as_const(m_preguntas))
                                  if (p.id == id)   // editar() recarga m_preguntas: no se sigue recorriendo
                                      return m_banco->editar(p, DialogoPregunta::Siguiente, tr("Guardar y siguiente con avisos"))
                                             == DialogoPregunta::Siguiente;
                              return false;
                          },
                          m_bd.ruta(), m_temas, m_panel->idiomaExamen(), this);
    dialogo.exec();
}

void VentanaPrincipal::mostrarAyuda()
{
    // Por el capítulo de la pestaña que se está viendo.
    const auto *pestanas = qobject_cast<QTabWidget *>(centralWidget());
    DialogoAyuda::mostrar(this, pestanas && pestanas->currentIndex() == 1 ? QStringLiteral("banco") : QStringLiteral("generar"));
}

QString VentanaPrincipal::versionYCompilacion()
{
    const QString version = QStringLiteral(VERSION_APLICACION);
    const QDateTime fecha = qApp->property("fechaCompilacion").toDateTime();
    if (!fecha.isValid())
        return version;
    const QString cuando = QLocale().toString(fecha, QLocale::ShortFormat);
    const QString commit = qApp->property("commitCompilacion").toString();
    if (commit.isEmpty())
        //: En «Acerca de», tras «Versión»: %1 es la versión (1.1) y %2, la fecha y la hora
        return tr("%1 (compilado el %2)").arg(version, cuando);
    //: En «Acerca de», tras «Versión»: %1 es la versión, %2 la fecha y la hora y %3 el commit de git (383615a)
    return tr("%1 (compilado el %2, commit %3)").arg(version, cuando, commit);
}

void VentanaPrincipal::acercaDe()
{
    QMessageBox caja(this);
    caja.setWindowTitle(tr("Acerca de"));
    const QPixmap foto = QPixmap(QStringLiteral(":/recursos/manuel-arias-calleja.jpg"))
                             .scaledToWidth(160, Qt::SmoothTransformation);
    caja.setIconPixmap(foto);
    caja.setTextFormat(Qt::RichText);
    caja.setText(tr("<h3>Generador de exámenes</h3>"
                    "<p>Versión %1</p>"
                    "<p>Desarrollado por Manuel Arias Calleja</p>"
                    "<p>Genera exámenes tipo test eligiendo al azar preguntas de la base de datos, "
                    "con el número de preguntas que se indique de cada tema.</p>"
                    "<p>Nunca pone en un mismo examen dos variantes de una pregunta (3a/3b) ni "
                    "pseudorrepetidas, y puede evitar que se repitan preguntas entre exámenes.</p>"
                    "<p>Base de datos: <tt>%2</tt></p><p>Configuración: <tt>%3</tt></p><p>Qt %4</p>")
                     .arg(versionYCompilacion().toHtmlEscaped(), QDir::toNativeSeparators(m_bd.ruta()).toHtmlEscaped(),
                          QDir::toNativeSeparators(QSettings().fileName()).toHtmlEscaped(), QString::fromLatin1(qVersion())));
    caja.setStandardButtons(QMessageBox::Ok);
    // QMessageBox elige el ancho del texto y lo deja muy estrecho (hasta el nombre se partía en dos líneas).
    // Respeta el mínimo de su rejilla, así que se le añade un espaciador del ancho de la foto más unos
    // 55 caracteres de texto.
    if (auto *rejilla = qobject_cast<QGridLayout *>(caja.layout())) {
        const QFontMetrics fm(caja.font());
        rejilla->addItem(new QSpacerItem(foto.width() + 55 * fm.averageCharWidth(), 0,
                                         QSizePolicy::Minimum, QSizePolicy::Fixed),
                         rejilla->rowCount(), 0, 1, rejilla->columnCount());
    }
    caja.exec();
}

void VentanaPrincipal::recargarDatos(int seleccionarId)
{
    m_temas = m_bd.temas();
    m_preguntas = m_bd.preguntas();

    m_banco->ponerDatos(m_temas, m_preguntas);
    m_panel->ponerDatos(m_temas, m_preguntas);

    if (seleccionarId)
        m_banco->seleccionar(seleccionarId);
    int activas = 0, verificadas = 0;
    for (const Pregunta &p : m_preguntas) {
        activas += p.activa() ? 1 : 0;
        verificadas += p.verificada ? 1 : 0;
    }
    statusBar()->showMessage(tr("Base de datos: %1  ·  %2 temas, %3 preguntas (%4 activas, %5 verificadas)")
                                 .arg(QDir::toNativeSeparators(m_bd.ruta()))
                                 .arg(m_temas.size()).arg(m_preguntas.size()).arg(activas).arg(verificadas));
    comprobarJson();
}

// ---------------------------------------------------------------------------------------------
// Configuración

// La configuración guarda las opciones del último banco abierto; al abrir uno, se cambian por las suyas (ver
// aplicarOpcionesDelBanco()). Un banco que aún no tiene se queda con las que haya.
void VentanaPrincipal::leerConfiguracion()
{
    QSettings s;
    restoreGeometry(s.value(kClaveGeometria).toByteArray());
    m_panel->leerConfiguracion();
}

void VentanaPrincipal::guardarConfiguracion()
{
    QSettings s;
    s.setValue(kClaveGeometria, saveGeometry());
    m_panel->guardarConfiguracion();
    if (m_bd.abierta())
        m_bd.guardarOpciones(opcionesDelBanco());
}

QVariantMap VentanaPrincipal::opcionesDelBanco() const
{
    QVariantMap opciones = m_panel->opciones();
    const QDir carpetaBanco = QFileInfo(m_bd.ruta()).absoluteDir();
    QSettings s;
    for (const QString &clave : kCarpetasDelBanco) {
        const QString carpeta = QDir::fromNativeSeparators(s.value(clave).toString().trimmed());
        if (carpeta.isEmpty())
            continue;
        const QString relativa = carpetaBanco.relativeFilePath(carpeta);
        opciones.insert(clave, relativa.isEmpty() ? QStringLiteral(".") : relativa);
    }
    return opciones;
}

void VentanaPrincipal::aplicarOpcionesDelBanco(const QVariantMap &opciones, bool carpetasPropias)
{
    m_panel->aplicarOpciones(opciones);
    // Las carpetas, en la configuración, que es donde las buscan los diálogos.
    const QDir carpetaBanco = QFileInfo(m_bd.ruta()).absoluteDir();
    QSettings s;
    for (const QString &clave : kCarpetasDelBanco) {
        if (opciones.contains(clave))
            s.setValue(clave, QDir::toNativeSeparators(QDir::cleanPath(carpetaBanco.absoluteFilePath(opciones.value(clave).toString()))));
        else if (carpetasPropias)
            s.setValue(clave, QDir::toNativeSeparators(carpetaBanco.absolutePath()));
    }
}

void VentanaPrincipal::closeEvent(QCloseEvent *evento)
{
    if (!puedenDescartarse()) {
        evento->ignore();
        return;
    }
    guardarConfiguracion();
    m_panel->guardarEstadoBanco();
    evento->accept();
}

// ---------------------------------------------------------------------------------------------
// Generación de exámenes

void VentanaPrincipal::actualizarTitulo()
{
    const QString asignatura = m_bd.abierta() ? m_bd.asignatura() : QString();
    setWindowTitle(asignatura.isEmpty() ? tr("Generador de exámenes")
                                        : tr("Generador de exámenes - %1").arg(asignatura));
}

void VentanaPrincipal::cambiarIdioma(const QString &codigo)
{
    const QString antes = Idioma::interfaz();   // el que se ve
    Idioma::elegir(codigo);
    const QString ahora = Idioma::preferido();  // el que tendrá al volver a abrirlo
    if (ahora == antes)
        return;
    // Lo que ya está a la vista no se traduce: el idioma nuevo se aplica al volver a abrir el programa.
    QMessageBox caja(QMessageBox::Information, windowTitle(),
                     //: %1 es el nombre del idioma elegido, en ese idioma («English»)
                     tr("El programa estará en %1 la próxima vez que se abra.").arg(Idioma::nombre(ahora)),
                     QMessageBox::NoButton, this);
    QPushButton *reiniciar = caja.addButton(tr("Reiniciar ahora"), QMessageBox::AcceptRole);
    caja.addButton(tr("Más tarde"), QMessageBox::RejectRole);
    caja.exec();
    if (caja.clickedButton() == reiniciar && close())
        QProcess::startDetached(QCoreApplication::applicationFilePath(), {});
}

DatosConvocatoria VentanaPrincipal::datosConvocatoria() const
{
    DatosConvocatoria d = m_panel->datos();
    // La de los exámenes que se ven, para las soluciones y las propiedades de los ficheros.
    if (hayExamen()) {
        d.semilla = m_resultado.semilla;
        d.cambiadosAMano = !m_cambios.isEmpty();
    }
    return d;
}

void VentanaPrincipal::generar()
{
    ConfigExamen config = m_panel->configuracion();
    if (config.preguntasPorTema.isEmpty()) {
        QMessageBox::information(this, windowTitle(), tr("Indique cuántas preguntas quiere de cada tema."));
        return;
    }
    // Los exámenes que salen, con las versiones de cada uno («Nacional Original A», «… B»).
    const QStringList examenes = Generador::nombresExamenes(config);
    QString repetido;
    for (int i = 0; i < examenes.size() && repetido.isEmpty(); ++i)
        if (examenes.indexOf(examenes.at(i), i + 1) > 0)
            repetido = examenes.at(i);
    QString error;
    if (config.nombresModelos.isEmpty())
        error = tr("Marque en la lista los exámenes que quiere generar.");
    else if (config.nombresModelos.contains(QString()))
        error = tr("Hay un examen marcado sin nombre.");
    else if (!repetido.isEmpty())
        //: %1 es el nombre de un examen, con su versión si tiene («Nacional Original B»)
        error = tr("Hay dos exámenes marcados con el mismo nombre: «%1».").arg(repetido);
    if (!error.isEmpty()) {
        QMessageBox::information(this, windowTitle(), error);
        return;
    }
    if (!puedenDescartarse())
        return;
    // Los registrados de estos exámenes no cuentan como salidos: se generan de nuevo, aunque ahora tengan otras versiones.
    const QStringList excluidos = examenes + registradosDe(config.nombresModelos);
    config.usoReciente = usoReciente(excluidos);
    config.salidas = salidasAnteriores(excluidos);
    m_resultado = Generador::generar(m_preguntas, config);
    m_semillaFija = config.semilla != 0;   // se avisa en la franja de avisos (actualizarMaqueta())
    if (m_panel->evitaRecientes() && m_bd.examenesRegistrados().isEmpty())
        m_resultado.avisos << tr("No hay ninguna convocatoria registrada en el historial, así que no se evitan las "
                                 "preguntas de las anteriores. Si se hicieron fuera del programa, regístrelas con "
                                 "Archivo → Registrar exámenes anteriores…");
    m_configResultado = config;
    guardarConfiguracion();
    presentarResultado();
}

QStringList VentanaPrincipal::registradosDe(const QStringList &deLaLista)
{
    const DatosConvocatoria d = m_panel->datos();
    QStringList res;
    for (const ExamenRegistrado &e : m_bd.examenesRegistrados())
        if (e.datos.anio == d.anio && e.datos.convocatoria == d.convocatoria && deLaLista.contains(e.base()))
            res << e.nombre;
    return res;
}

QHash<QString, SalidaAnterior> VentanaPrincipal::salidasAnteriores(const QStringList &examenes)
{
    const DatosConvocatoria d = m_panel->datos();
    return m_bd.salidasAnteriores(d.anio, d.convocatoria, examenes);
}

QHash<QString, int> VentanaPrincipal::usoReciente(const QStringList &examenes)
{
    if (!m_panel->evitaRecientes())
        return {};
    const DatosConvocatoria d = m_panel->datos();
    return m_bd.usoReciente(m_panel->numRecientes(), d.anio, d.convocatoria, examenes);
}

void VentanaPrincipal::presentarResultado(const QString &origen)
{
    m_origenResultado = origen;
    m_selectorModelo->blockSignals(true);
    m_selectorModelo->clear();
    for (const Modelo &m : std::as_const(m_resultado.modelos))
        m_selectorModelo->addItem(m.nombre);
    m_selectorModelo->blockSignals(false);
    for (QPushButton *b : std::as_const(m_botonesExamen))
        b->setEnabled(hayExamen());
    m_preguntaACambiar->setEnabled(hayExamen());
    m_cambios.clear();
    m_quitadas.clear();
    m_deshacerCambio->setEnabled(false);
    ofrecerCarpeta(QString());
    m_exportados = false;
    m_registrados = !origen.isEmpty();   // cargados del historial
    actualizarMaqueta();
}

void VentanaPrincipal::actualizarMaqueta()
{
    QStringList avisos = m_resultado.avisos;
    QString info = m_origenResultado.isEmpty() ? QString() : m_origenResultado + QStringLiteral("  ·  ");
    //: Línea de información del examen generado: semilla del generador de números aleatorios
    info += tr("Semilla: %1").arg(m_resultado.semilla);
    //: Se añade tras «Semilla: %1» (con el espacio delante)
    if (m_semillaFija)
        info += tr(" (fija: cada vez salen los mismos exámenes)");
    //: Se añade a la línea de información del examen (con el espacio delante)
    if (!m_cambios.isEmpty())
        info += m_cambios.size() == 1 ? tr(" (con una pregunta cambiada a mano)")
                                      : tr(" (con %1 preguntas cambiadas a mano)").arg(m_cambios.size());
    if (m_letraGrande->isChecked()) {
        //: Se añade a la línea de información del examen (con el separador delante)
        info += tr("  ·  Letra grande (versión adaptada, sin límite de páginas)");
    } else if (!m_panel->limitaPaginas()) {
        //: Se añade a la línea de información del examen (con el separador delante): no se ajusta a una página por cada 10 preguntas
        info += tr("  ·  Sin límite de páginas");
    } else {
        bool caben = true;
        const int nivel = hayExamen() ? Maquetador::nivelParaCaber(m_resultado.modelos, datosConvocatoria(), &caben) : 0;
        if (!caben) {
            const int paginas = Maquetador::paginasObjetivo(m_resultado.modelos.first().preguntas.size());
            //: %1 es «una página» o «N páginas»
            avisos << tr("Algún examen no cabe en %1 ni con la maqueta más compacta: acorte las instrucciones o quite preguntas.")
                          .arg(paginas == 1 ? tr("una página") : tr("%1 páginas").arg(paginas));
        }
        if (nivel > 0)
            //: Se añade a la línea de información del examen (con el separador delante)
            info += tr("  ·  Maqueta compacta (nivel %1 de %2)").arg(nivel).arg(Maquetador::numNiveles() - 1);
    }
    // Las preguntas que han dejado de estar activas (al editarlas desde el examen) siguen en él: hasta que se cambien.
    for (const Modelo &m : m_resultado.modelos)
        for (int i = 0; i < m.preguntas.size(); ++i)
            if (!m.preguntas.at(i).pregunta.activa())
                //: Aviso junto al examen; %1 es el nombre del examen, %2 el número de la pregunta en él y %3 su código (T2-15b)
                avisos << tr("«%1», pregunta %2 (%3): ya no está activa. Si no debe salir, cámbiela por otra.")
                              .arg(m.nombre).arg(i + 1).arg(m.preguntas.at(i).pregunta.codigoCompleto());
    QStringList lineas;
    // Mientras la semilla siga escrita: al generar otra vez saldrían los mismos.
    if (hayExamen() && m_semillaFija && !m_panel->semilla().isEmpty())
        lineas << tr("Generados con la semilla fija %1: cada vez salen los mismos exámenes.").arg(m_resultado.semilla).toHtmlEscaped()
                      + QStringLiteral(" <a href=\"quitar-semilla\">%1</a>")
                            //: Enlace del aviso de la semilla fija: borra el campo «Semilla» de las opciones
                            .arg(tr("Quitar la semilla").toHtmlEscaped());
    for (const QString &aviso : std::as_const(avisos))
        lineas << aviso.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    m_avisos->setText(lineas.join(QStringLiteral("<br>")));
    m_avisos->setVisible(!lineas.isEmpty());
    m_infoBase = info;
    actualizarRegistro();
    m_infoGeneracion->setToolTip(tr("Escriba esta semilla en Opciones para volver a obtener los mismos exámenes.\n"
                                    "Si se han cambiado preguntas a mano, ya no los reproduce: regístrelos para "
                                    "conservarlos.\n"
                                    "Con «Ajustar a una página por cada 10 preguntas» (en Opciones), la maqueta se "
                                    "compacta (opciones en dos columnas, menos separación, letra menor) hasta que cada "
                                    "examen cabe en sus páginas; con «Letra grande» no se compacta."));
    mostrarModelo();
}

bool VentanaPrincipal::hayExamen() const
{
    return !m_resultado.modelos.isEmpty() && !m_resultado.modelos.first().preguntas.isEmpty();
}

void VentanaPrincipal::mostrarModelo()
{
    const int i = m_selectorModelo->currentIndex();
    if (i < 0 || i >= m_resultado.modelos.size())
        return;
    //: Cuántas veces es correcta cada letra en el examen; %1 es la lista: «A 3 · B 2 · C 3 · D 2»
    m_correctasPorLetra->setText(tr("Correctas: %1").arg(Maquetador::textoCorrectasPorLetra(m_resultado.modelos.at(i))));
    m_preguntaACambiar->setRange(1, qMax(1, int(m_resultado.modelos.at(i).preguntas.size())));
    // Con soluciones, también la plantilla de corrección, que en los ficheros va en la hoja de soluciones. Y la marca de
    // las verificadas y las que no, que solo va en la vista.
    QString html = Maquetador::examenHtml(m_resultado.modelos.at(i), datosConvocatoria(), m_verSoluciones->isChecked(),
                                          nivelMaqueta(), true, true);
    // Y al final, lo que sale al pie de cada página (aquí, el de la última): la vista no tiene páginas.
    html.replace(QStringLiteral("</body>"), Maquetador::pieVistaHtml(m_resultado.modelos.at(i), datosConvocatoria(),
                                                                     m_verSoluciones->isChecked(), nivelMaqueta())
                                                + QStringLiteral("</body>"));
    m_visorExamen->ponerDocumento(Maquetador::crearDocumento(html, Maquetador::preguntasDe(m_resultado.modelos), m_visorExamen));
}

QString VentanaPrincipal::nombreBaseFichero() const
{
    // «2026-Junio»: el año delante, para que los ficheros de varios años queden ordenados; la versión adaptada,
    // «2026-Junio-letra-grande», para no pisar la normal.
    const QString base = limpiarNombreFichero(QStringLiteral("%1 %2").arg(m_panel->datos().anio).arg(m_panel->datos().convocatoria));
    return m_letraGrande->isChecked() ? base + QStringLiteral("-letra-grande") : base;
}

QStringList VentanaPrincipal::nombresDeLaLista() const
{
    QStringList nombres;
    for (const Modelo &m : m_resultado.modelos) {
        const QString nombre = m.version.isEmpty() ? m.nombre : m.base;
        if (!nombres.contains(nombre))
            nombres << nombre;
    }
    return nombres;
}

QStringList VentanaPrincipal::nombresModelos() const
{
    QStringList nombres;
    for (const Modelo &m : m_resultado.modelos)
        nombres << m.nombre;
    return nombres;
}

QVector<Modelo> VentanaPrincipal::modelosDe(const QVector<int> &indices) const
{
    QVector<Modelo> res;
    for (int i : indices)
        if (i >= 0 && i < m_resultado.modelos.size())
            res << m_resultado.modelos.at(i);
    return res;
}

int VentanaPrincipal::nivelMaqueta() const
{
    if (m_letraGrande->isChecked())
        return Maquetador::kLetraGrande;
    if (!m_panel->limitaPaginas())
        return 0;
    return hayExamen() ? Maquetador::nivelParaCaber(m_resultado.modelos, datosConvocatoria()) : 0;
}

void VentanaPrincipal::informarExportacion(const QStringList &escritos, const QStringList &fallidos, const QString &nota)
{
    if (!fallidos.isEmpty())
        QMessageBox::warning(this, windowTitle(), tr("No se pudieron escribir estos ficheros:\n%1").arg(fallidos.join(QLatin1Char('\n'))));
    if (escritos.isEmpty())
        return;
    QString mensaje = escritos.size() == 1
                          ? tr("Fichero guardado en %1").arg(QDir::toNativeSeparators(escritos.first()))
                          : tr("%1 ficheros guardados en %2").arg(escritos.size()).arg(QDir::toNativeSeparators(QFileInfo(escritos.first()).absolutePath()));
    if (!nota.isEmpty())
        mensaje += QStringLiteral(" (%1)").arg(nota);
    statusBar()->showMessage(mensaje, 8000);
    ofrecerCarpeta(QFileInfo(escritos.first()).absolutePath());
    m_exportados = true;
    actualizarRegistro();
}

void VentanaPrincipal::ofrecerCarpeta(const QString &carpeta)
{
    m_abrirCarpeta->setProperty("carpeta", carpeta);
    m_abrirCarpeta->setToolTip(QDir::toNativeSeparators(carpeta));
    m_abrirCarpeta->setVisible(!carpeta.isEmpty());
}

void VentanaPrincipal::vistaPreviaImpresion()
{
    if (!hayExamen())
        return;
    QPrinter printer(QPrinter::HighResolution);
    Maquetador::configurarPagina(printer);
    QPrintPreviewDialog vista(&printer, this);
    vista.setWindowTitle(tr("Vista previa de impresión"));
    vista.resize(1000, 900);
    const DatosConvocatoria d = datosConvocatoria();
    const int nivel = nivelMaqueta();
    connect(&vista, &QPrintPreviewDialog::paintRequested, this,
            [this, d, nivel](QPrinter *p) { Maquetador::imprimirExamenes(p, m_resultado.modelos, d, false, nivel); });
    if (vista.exec() == QDialog::Accepted) {   // se ha impreso
        m_exportados = true;
        actualizarRegistro();
    }
}

void VentanaPrincipal::actualizarRegistro()
{
    const bool pendiente = hayExamen() && m_exportados && !m_registrados;
    //: Se añade a la línea de información de los exámenes (con el separador delante)
    m_infoGeneracion->setText(pendiente ? m_infoBase + tr("  ·  Exportados y sin registrar") : m_infoBase);
    m_botonRegistrar->setStyleSheet(pendiente ? Paleta::fondoAtencion() : QString());
    m_botonRegistrar->setToolTip(pendiente ? tr("Los exámenes se han exportado pero no se han registrado: si son "
                                                "los definitivos, regístrelos para que sus preguntas se eviten "
                                                "en las convocatorias siguientes.")
                                           : QString());
}

bool VentanaPrincipal::puedenDescartarse()
{
    if (!hayExamen() || !m_exportados || m_registrados)
        return true;
    QMessageBox caja(QMessageBox::Warning, windowTitle(),
                     tr("Los exámenes de %1 se han exportado, pero no se han registrado en el historial. Si sigue, "
                        "se perderán y sus preguntas no se evitarán en las convocatorias siguientes.\n\n"
                        "Si son los definitivos, regístrelos antes.").arg(datosConvocatoria().convocatoriaYAnio()),
                     QMessageBox::NoButton, this);
    QPushButton *registrar = caja.addButton(tr("Registrarlos…"), QMessageBox::AcceptRole);
    QPushButton *descartar = caja.addButton(tr("No registrarlos"), QMessageBox::DestructiveRole);
    caja.setDefaultButton(caja.addButton(tr("Cancelar"), QMessageBox::RejectRole));
    caja.exec();
    if (caja.clickedButton() == registrar) {
        registrarConvocatoria();
        return m_registrados;
    }
    return caja.clickedButton() == descartar;
}

void VentanaPrincipal::exportar(const QString &titulo, const QString &extension, Escritor escribir, bool soluciones,
                                const QString &nota, const QString &casillaSoluciones)
{
    if (!hayExamen())
        return;
    DialogoExportar dialogo(titulo, nombresModelos(), nombreBaseFichero(), extension, soluciones, this);
    if (!casillaSoluciones.isEmpty())
        dialogo.ofrecerSoluciones(casillaSoluciones);
    if (dialogo.exec() != QDialog::Accepted)
        return;
    guardarConfiguracion();   // la carpeta, en el banco
    if (!casillaSoluciones.isEmpty())
        soluciones = dialogo.conSoluciones();
    const DatosConvocatoria d = datosConvocatoria();
    const int nivel = nivelMaqueta();
    QStringList escritos, fallidos;
    for (const DialogoExportar::Fichero &f : dialogo.ficheros())
        (escribir(f.ruta, modelosDe(f.modelos), d, soluciones, nivel) ? escritos : fallidos) << f.ruta;
    informarExportacion(escritos, fallidos, nota);
}

// Cada examen marcado en el diálogo se guarda en su propio fichero (y, si se marca, otro con todos juntos).
void VentanaPrincipal::exportarPdf()
{
    exportar(tr("Exportar exámenes a PDF"), QStringLiteral("pdf"), &Exportacion::pdf, false);
}

// Igual que exportarPdf(), pero cada fichero lleva delante la hoja de soluciones de sus exámenes.
void VentanaPrincipal::exportarSolucionesPdf()
{
    exportar(tr("Exportar soluciones a PDF"), QStringLiteral("pdf"), &Exportacion::pdf, true);
}

// La casilla del diálogo de exportar a HTML y a LaTeX: con ella llevan lo mismo que el PDF de soluciones.
QString VentanaPrincipal::textoConSoluciones()
{
    return tr("Con soluciones (plantilla de corrección y, en cada pregunta, la correcta, la explicación y la referencia)");
}

void VentanaPrincipal::exportarHtml()
{
    exportar(tr("Exportar a HTML (editable en Word / LibreOffice)"), QStringLiteral("html"), &Exportacion::html,
             m_verSoluciones->isChecked(), QString(), textoConSoluciones());
}

void VentanaPrincipal::exportarLatex()
{
    exportar(tr("Exportar a LaTeX"), QStringLiteral("tex"), &Exportacion::latex,
             m_verSoluciones->isChecked(), tr("compílelos dos veces con pdflatex"), textoConSoluciones());
}

void VentanaPrincipal::exportarDocx()
{
    exportar(tr("Exportar a Word (.docx)"), QStringLiteral("docx"), &Exportacion::docx, m_verSoluciones->isChecked(),
             QString(), textoConSoluciones());
}

void VentanaPrincipal::registrarConvocatoria()
{
    if (!hayExamen())
        return;
    const DatosConvocatoria d = datosConvocatoria();
    if (d.convocatoria.isEmpty()) {
        QMessageBox::information(this, windowTitle(), tr("Indique la convocatoria (Febrero, Junio…)."));
        return;
    }
    const QStringList nombres = nombresModelos();
    // Los ya registrados de estos exámenes: con el mismo nombre o, con versiones o sin ellas, del mismo de la lista.
    QStringList yaRegistrados;
    for (const ExamenRegistrado &e : m_bd.examenesRegistrados())
        if (e.datos.anio == d.anio && e.datos.convocatoria == d.convocatoria && nombres.contains(e.nombre))
            yaRegistrados << e.nombre;
    for (const QString &nombre : registradosDe(nombresDeLaLista()))
        if (!yaRegistrados.contains(nombre))
            yaRegistrados << nombre;
    QString texto = tr("Se guardarán en el historial los exámenes de %1: %2.\n\n"
                       "Al generar, se evitarán sus preguntas, y desde Archivo → Historial de convocatorias "
                       "se podrán volver a cargar tal como están.")
                        .arg(d.convocatoriaYAnio(), nombres.join(QStringLiteral(", ")));
    if (!yaRegistrados.isEmpty())
        texto += tr("\n\nYa había registrados de estos exámenes (con el mismo nombre, o con otras versiones), y se "
                    "sustituirán: %1.").arg(yaRegistrados.join(QStringLiteral(", ")));
    if (!confirmar(this, tr("Registrar convocatoria"), texto))
        return;
    if (!m_bd.registrarExamenes(m_resultado.modelos, d, m_resultado.semilla, yaRegistrados)) {
        QMessageBox::warning(this, windowTitle(), tr("No se pudo registrar la convocatoria:\n%1").arg(m_bd.ultimoError()));
        return;
    }
    statusBar()->showMessage(tr("Registrados en el historial los exámenes de %1").arg(d.convocatoriaYAnio()), 8000);
    m_registrados = true;
    actualizarRegistro();
    m_banco->actualizarFicha();   // la ficha de la pregunta dice en qué exámenes ha salido
}

void VentanaPrincipal::historialConvocatorias()
{
    DialogoHistorial dialogo(&m_bd, this);
    if (dialogo.exec() == QDialog::Accepted && puedenDescartarse())
        cargarRegistrados(dialogo.examenesElegidos());
    m_banco->actualizarFicha();   // por si se ha eliminado algún examen
}

void VentanaPrincipal::registrarAnteriores()
{
    DialogoAnteriores dialogo(&m_bd, m_preguntas, datosConvocatoria(), m_panel->examenes(), this);
    const bool registrados = dialogo.exec() == QDialog::Accepted;
    guardarConfiguracion();   // la carpeta de los PDF, en el banco
    if (!registrados)
        return;
    statusBar()->showMessage(tr("Registrados en el historial los exámenes de %1").arg(dialogo.convocatoriaYAnio()), 8000);
    m_banco->actualizarFicha();   // la ficha de la pregunta dice en qué exámenes ha salido
}

void VentanaPrincipal::cargarRegistrados(const QVector<int> &examenes)
{
    const QVector<ExamenRegistrado> registrados = m_bd.examenesRegistrados();
    ResultadoGeneracion resultado;
    const ExamenRegistrado *primero = nullptr;
    for (int id : examenes) {
        for (const ExamenRegistrado &e : registrados) {
            if (e.id != id)
                continue;
            if (!primero)
                primero = &e;
            resultado.modelos << m_bd.modeloRegistrado(id);
        }
    }
    if (!primero)
        return;
    resultado.semilla = primero->semilla;
    // La copia guarda la verificación de cuando se registró: la de ahora está en el banco.
    ponerVerificacionDelBanco(resultado);
    m_resultado = resultado;

    // Los datos de la convocatoria pasan a la ventana, para que lo que se exporte salga como cuando se registró.
    const DatosConvocatoria &d = primero->datos;
    m_panel->ponerConvocatoria(d);
    // Para cambiar preguntas de los exámenes cargados: las opciones de la ventana.
    m_semillaFija = false;
    m_configResultado = m_panel->configuracion();
    m_configResultado.nombresModelos = nombresModelos();
    const QStringList excluidos = m_configResultado.nombresModelos + registradosDe(nombresDeLaLista());
    m_configResultado.usoReciente = usoReciente(excluidos);
    m_configResultado.salidas = salidasAnteriores(excluidos);
    if (auto *pestanas = qobject_cast<QTabWidget *>(centralWidget()))
        pestanas->setCurrentIndex(0);
    //: Origen de los exámenes cargados del historial: la convocatoria se registró el día %1
    presentarResultado(tr("Registrada el %1").arg(QLocale().toString(primero->registrado.date(), QLocale::ShortFormat)));
}

void VentanaPrincipal::cambiarPregunta()
{
    cambiar(m_preguntaACambiar->value() - 1);
}

void VentanaPrincipal::cambiar(int indice)
{
    const int modelo = m_selectorModelo->currentIndex();
    if (modelo < 0 || modelo >= m_resultado.modelos.size() || indice < 0 || indice >= m_resultado.modelos.at(modelo).preguntas.size())
        return;
    const QString nombre = m_resultado.modelos.at(modelo).nombre;
    const Pregunta quitada = m_resultado.modelos.at(modelo).preguntas.at(indice).pregunta;
    const ResultadoGeneracion antes = m_resultado;
    if (!Generador::cambiarPregunta(m_preguntas, m_configResultado, m_resultado, modelo, indice, m_quitadas.value(modelo))) {
        QMessageBox::information(this, windowTitle(),
                                 tr("No hay ninguna otra pregunta del tema %1 que pueda ir en lugar de la %2 (%3): "
                                    "las demás son variantes o pseudorrepetidas de otras del examen.")
                                     .arg(quitada.temaNumero).arg(indice + 1).arg(quitada.codigoCompleto()));
        return;
    }
    m_cambios << CambioPregunta{antes, modelo, indice};
    m_registrados = false;   // ya no son como los registrados
    for (int version : Generador::versionesDe(m_resultado, modelo))   // se ha cambiado en todas
        m_quitadas[version].insert(quitada.grupo());
    m_deshacerCambio->setEnabled(true);
    actualizarMaqueta();
    m_preguntaACambiar->setValue(indice + 1);
    m_visorExamen->scrollToAnchor(QStringLiteral("q%1").arg(indice + 1));
    //: %1 es el nombre del examen, %2 el número de la pregunta en él, %3 y %4 los códigos de la nueva y de la quitada (T2-15b)
    statusBar()->showMessage(tr("«%1», pregunta %2: %3 en lugar de %4")
                                 .arg(nombre).arg(indice + 1)
                                 .arg(m_resultado.modelos.at(modelo).preguntas.at(indice).pregunta.codigoCompleto(),
                                      quitada.codigoCompleto()),
                             10000);
}

void VentanaPrincipal::editarPreguntaExamen()
{
    editarDelExamen(m_preguntaACambiar->value() - 1);
}

void VentanaPrincipal::editarDelExamen(int indice)
{
    const int modelo = m_selectorModelo->currentIndex();
    if (modelo < 0 || modelo >= m_resultado.modelos.size() || indice < 0 || indice >= m_resultado.modelos.at(modelo).preguntas.size())
        return;
    const Pregunta copia = m_resultado.modelos.at(modelo).preguntas.at(indice).pregunta;
    // Se edita la del banco, que puede ser más nueva que la copia del examen (si se ha corregido después, o si el
    // examen viene del historial).
    const Pregunta enBanco = preguntaDelBanco(copia);
    if (enBanco.id == 0) {
        QMessageBox::information(this, windowTitle(), tr("La pregunta %1 ya no está en el banco: no se puede editar.")
                                                          .arg(copia.codigoCompleto()));
        return;
    }
    Pregunta guardada;
    m_banco->editar(enBanco, DialogoPregunta::Cerrar, QString(), &guardada);
    if (guardada.id == 0)
        return;

    const int sitios = corregirEnExamenes(copia, guardada);
    actualizarMaqueta();
    m_preguntaACambiar->setValue(indice + 1);
    m_visorExamen->scrollToAnchor(QStringLiteral("q%1").arg(indice + 1));
    //: %1 es el código de la pregunta (T2-15b)
    QString mensaje = sitios > 1 ? tr("Pregunta %1 guardada en el banco y corregida en los %2 exámenes en que está.")
                                       .arg(guardada.codigoCompleto()).arg(sitios)
                                 : tr("Pregunta %1 guardada en el banco y corregida en el examen.").arg(guardada.codigoCompleto());
    if (!guardada.activa())
        //: Se añade al mensaje de pregunta guardada (con el espacio delante) si se ha marcado como anulada u obsoleta
        mensaje += tr(" Ya no está activa: si no debe salir, cámbiela por otra.");
    statusBar()->showMessage(mensaje, 10000);
}

Pregunta VentanaPrincipal::preguntaDelBanco(const Pregunta &copia) const
{
    for (const Pregunta &p : m_preguntas)
        if (p.id == copia.id && p.codigoCompleto() == copia.codigoCompleto())
            return p;
    for (const Pregunta &p : m_preguntas)
        if (p.codigoCompleto() == copia.codigoCompleto())
            return p;
    for (const Pregunta &p : m_preguntas)
        if (p.id == copia.id)
            return p;
    return Pregunta();
}

int VentanaPrincipal::corregirEnExamenes(const Pregunta &antes, const Pregunta &guardada)
{
    // Si cambia lo que se ve en el examen, ya no es como el registrado.
    if (guardada.codigoCompleto() != antes.codigoCompleto() || !guardada.mismoContenido(antes))
        m_registrados = false;
    const int sitios = Generador::ponerCorregida(m_resultado, m_configResultado, antes, guardada);
    // También en los exámenes de antes de cada cambio de pregunta: al deshacerlo, la pregunta vuelve corregida.
    for (CambioPregunta &cambio : m_cambios)
        Generador::ponerCorregida(cambio.antes, m_configResultado, antes, guardada);
    return sitios;
}

void VentanaPrincipal::verificarPreguntaExamen()
{
    verificarDelExamen(m_preguntaACambiar->value() - 1);
}

void VentanaPrincipal::verificarDelExamen(int indice)
{
    const int modelo = m_selectorModelo->currentIndex();
    if (modelo < 0 || modelo >= m_resultado.modelos.size())
        return;
    // Las del examen que se ve, en su orden, como están en el banco (las que ya no están, no).
    QVector<Pregunta> preguntas;
    QVector<int> enExamen;   // número (desde 0) en el examen de cada una
    int inicial = 0;
    const QVector<PreguntaExamen> &delExamen = m_resultado.modelos.at(modelo).preguntas;
    for (int i = 0; i < delExamen.size(); ++i) {
        const Pregunta p = preguntaDelBanco(delExamen.at(i).pregunta);
        if (p.id == 0)
            continue;
        if (i <= indice)
            inicial = int(preguntas.size());
        preguntas << p;
        enExamen << i;
    }
    if (preguntas.isEmpty())
        return;
    // Lo que se corrija con el editor se pone también en los exámenes, como con «Editar…».
    DialogoVerificar d(&m_bd, preguntas, inicial, m_temas,
                       [this](const Pregunta &p) {
                           Pregunta guardada;
                           m_banco->editar(p, DialogoPregunta::Cerrar, QString(), &guardada);
                           if (guardada.id != 0)
                               corregirEnExamenes(p, guardada);
                           return guardada;
                       },
                       this);
    //: Título del diálogo de verificar; %1 es el nombre del examen («Nacional Original»)
    d.setWindowTitle(tr("Verificar las preguntas de «%1»").arg(m_resultado.modelos.at(modelo).nombre));
    // Cada pregunta que se verifica se ve ya en el examen, detrás del diálogo, sin esperar a cerrarlo.
    connect(&d, &DialogoVerificar::verificacionGuardada, this, [&](int id) {
        verificacionCambiada(id, false);
        for (int i = 0; i < preguntas.size(); ++i)
            if (preguntas.at(i).id == id)
                m_visorExamen->scrollToAnchor(QStringLiteral("q%1").arg(enExamen.at(i) + 1));
    });
    d.exec();
    if (!d.cambiadas().isEmpty())
        verificacionCambiada(preguntas.value(d.actual()).id);
    if (d.hayEditadas())
        actualizarMaqueta();
    else
        mostrarModelo();
    const int numero = enExamen.value(d.actual()) + 1;
    m_preguntaACambiar->setValue(numero);
    m_visorExamen->scrollToAnchor(QStringLiteral("q%1").arg(numero));
}

void VentanaPrincipal::verificacionCambiada(int seleccionarId, bool conJson)
{
    if (conJson)
        bancoEditado(seleccionarId);
    else
        recargarDatos(seleccionarId);
    // La verificación de las preguntas de los exámenes (y de los de antes de cada cambio), la del banco.
    ponerVerificacionDelBanco(m_resultado);
    for (CambioPregunta &cambio : m_cambios)
        ponerVerificacionDelBanco(cambio.antes);
    if (hayExamen())
        mostrarModelo();
}

void VentanaPrincipal::ponerVerificacionDelBanco(ResultadoGeneracion &resultado) const
{
    for (Modelo &m : resultado.modelos) {
        for (PreguntaExamen &pe : m.preguntas) {
            const Pregunta enBanco = preguntaDelBanco(pe.pregunta);
            if (enBanco.id != 0 && enBanco.mismoContenido(pe.pregunta)) {
                pe.pregunta.verificada = enBanco.verificada;
                pe.pregunta.verificadores = enBanco.verificadores;
            }
        }
    }
}

void VentanaPrincipal::deshacerCambio()
{
    if (m_cambios.isEmpty())
        return;
    const CambioPregunta cambio = m_cambios.takeLast();
    m_resultado = cambio.antes;
    m_registrados = false;
    m_deshacerCambio->setEnabled(!m_cambios.isEmpty());
    // Se vuelve al examen y la pregunta del cambio.
    m_selectorModelo->blockSignals(true);
    m_selectorModelo->setCurrentIndex(cambio.modelo);
    m_selectorModelo->blockSignals(false);
    actualizarMaqueta();
    m_preguntaACambiar->setValue(cambio.indice + 1);
    m_visorExamen->scrollToAnchor(QStringLiteral("q%1").arg(cambio.indice + 1));
    statusBar()->showMessage(tr("Deshecho el cambio de la pregunta %1 de «%2»")
                                 .arg(cambio.indice + 1).arg(m_resultado.modelos.at(cambio.modelo).nombre),
                             8000);
}

int VentanaPrincipal::preguntaEn(const QPoint &enVista) const
{
    // Cada pregunta empieza con un ancla «qN» (ver Maquetador): la primera que haya hacia atrás.
    for (QTextBlock b = m_visorExamen->cursorForPosition(enVista).block(); b.isValid(); b = b.previous()) {
        for (auto it = b.begin(); !it.atEnd(); ++it) {
            for (const QString &ancla : it.fragment().charFormat().anchorNames()) {
                bool ok = false;
                const int n = ancla.startsWith(QLatin1Char('q')) ? ancla.mid(1).toInt(&ok) : 0;
                if (ok)
                    return n - 1;
            }
        }
    }
    return -1;
}

void VentanaPrincipal::menuVista(const QPoint &enVista)
{
    std::unique_ptr<QMenu> menu(m_visorExamen->createStandardContextMenu());
    QAction *primera = menu->actions().value(0);
    QAction *editar = nullptr;
    QAction *verificar = nullptr;
    QAction *cambiar = nullptr;
    QAction *deshacer = nullptr;
    const int modelo = m_selectorModelo->currentIndex();
    const int indice = hayExamen() && modelo >= 0 && modelo < m_resultado.modelos.size() ? preguntaEn(enVista) : -1;
    if (indice >= 0 && indice < m_resultado.modelos.at(modelo).preguntas.size()) {
        //: %1 es el número de la pregunta en el examen y %2 su código (T2-15b)
        editar = new QAction(tr("Editar la pregunta %1 (%2)…")
                                 .arg(indice + 1).arg(m_resultado.modelos.at(modelo).preguntas.at(indice).pregunta.codigoCompleto()),
                             menu.get());
        menu->insertAction(primera, editar);
        //: %1 es el número de la pregunta en el examen y %2 su código (T2-15b)
        verificar = new QAction(tr("Verificar la pregunta %1 (%2)…")
                                    .arg(indice + 1).arg(m_resultado.modelos.at(modelo).preguntas.at(indice).pregunta.codigoCompleto()),
                                menu.get());
        menu->insertAction(primera, verificar);
        cambiar = new QAction(tr("Cambiar la pregunta %1 por otra del tema %2")
                                  .arg(indice + 1).arg(m_resultado.modelos.at(modelo).preguntas.at(indice).pregunta.temaNumero),
                              menu.get());
        menu->insertAction(primera, cambiar);
    }
    if (!m_cambios.isEmpty()) {
        deshacer = new QAction(tr("Deshacer el último cambio de pregunta"), menu.get());
        menu->insertAction(primera, deshacer);
    }
    if (editar || cambiar || deshacer)
        menu->insertSeparator(primera);
    menu->addSeparator();
    menu->addActions(m_visorExamen->accionesEscala());
    QAction *elegida = menu->exec(m_visorExamen->viewport()->mapToGlobal(enVista));
    if (elegida && elegida == editar)
        editarDelExamen(indice);
    else if (elegida && elegida == verificar)
        verificarDelExamen(indice);
    else if (elegida && elegida == cambiar)
        this->cambiar(indice);
    else if (elegida && elegida == deshacer)
        deshacerCambio();
}

void VentanaPrincipal::practicar()
{
    const int i = m_selectorModelo->currentIndex();
    if (i < 0 || i >= m_resultado.modelos.size())
        return;
    // La duración se recuerda de una vez a otra (antes era un campo de la convocatoria: se hereda su valor).
    QSettings ajustes;
    const int duracion = ajustes.value(kClaveDuracionPractica,
                                       ajustes.value(QStringLiteral("examen/duracion"), 60)).toInt();
    DialogoPractica d(m_resultado.modelos.at(i), datosConvocatoria(), duracion, this);
    d.exec();
    ajustes.setValue(kClaveDuracionPractica, d.duracionMinutos());
}
