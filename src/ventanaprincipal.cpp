#include "ventanaprincipal.h"
#include "dialogoestilo.h"
#include "dialogoexportar.h"
#include "dialogohistorial.h"
#include "dialogopractica.h"
#include "dialogopregunta.h"
#include "dialogotemas.h"
#include "exportacion.h"
#include "modelopreguntas.h"
#include "utilidades.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDate>
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
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableView>
#include <QTextBrowser>
#include <QTextDocument>
#include <QVBoxLayout>

namespace {

// Reparto habitual de preguntas por tema (exámenes de 10 preguntas por parte y 20 en total).
const QMap<int, int> kRepartoHabitual{{1, 3}, {2, 2}, {3, 3}, {4, 2}, {5, 2}, {6, 3}, {7, 2}, {8, 3}};
// Reparto inicial (sin configuración guardada): como en los exámenes de ejemplo, 10 preguntas de T1–T4.
const QMap<int, int> kRepartoPorDefecto{{1, 3}, {2, 2}, {3, 3}, {4, 2}};

// Lista de exámenes la primera vez que se abre el programa; luego se recuerda la que se haya hecho.
const QStringList kExamenesPorDefecto{QStringLiteral("Nacional Original"), QStringLiteral("Nacional Reserva")};

// Lee de la configuración el valor de un control (si no está, se queda con el que tiene) y el valor que hay que
// guardar de él, según su tipo.
void leerAjuste(const QSettings &s, const QString &clave, QWidget *control)
{
    if (auto *c = qobject_cast<QCheckBox *>(control))
        c->setChecked(s.value(clave, c->isChecked()).toBool());
    else if (auto *n = qobject_cast<QSpinBox *>(control))
        n->setValue(s.value(clave, n->value()).toInt());
    else if (auto *d = qobject_cast<QDoubleSpinBox *>(control))
        d->setValue(s.value(clave, d->value()).toDouble());
    else if (auto *l = qobject_cast<QLineEdit *>(control))
        l->setText(s.value(clave, l->text()).toString());
    else if (auto *t = qobject_cast<QPlainTextEdit *>(control))
        t->setPlainText(s.value(clave, t->toPlainText()).toString());
    else if (auto *b = qobject_cast<QComboBox *>(control))
        b->setCurrentText(s.value(clave, b->currentText()).toString());
}

QVariant valorAjuste(const QWidget *control)
{
    if (auto *c = qobject_cast<const QCheckBox *>(control))
        return c->isChecked();
    if (auto *n = qobject_cast<const QSpinBox *>(control))
        return n->value();
    if (auto *d = qobject_cast<const QDoubleSpinBox *>(control))
        return d->value();
    if (auto *l = qobject_cast<const QLineEdit *>(control))
        return l->text();
    if (auto *t = qobject_cast<const QPlainTextEdit *>(control))
        return t->toPlainText();
    if (auto *b = qobject_cast<const QComboBox *>(control))
        return b->currentText().trimmed();
    return QVariant();
}

} // namespace

VentanaPrincipal::VentanaPrincipal(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Generador de exámenes - Fundamentos de Informática"));
    auto *pestanas = new QTabWidget;
    pestanas->addTab(crearPestanaExamen(), QStringLiteral("Generar examen"));
    pestanas->addTab(crearPestanaBanco(), QStringLiteral("Banco de preguntas"));
    setCentralWidget(pestanas);
    crearMenus();
    leerConfiguracion();
}

VentanaPrincipal::~VentanaPrincipal() = default;

// ---------------------------------------------------------------------------------------------
// Construcción de la interfaz

QWidget *VentanaPrincipal::crearPestanaExamen()
{
    // --- Panel de configuración
    auto *config = new QWidget;
    auto *lc = new QVBoxLayout(config);

    lc->addWidget(crearGrupoConvocatoria());
    lc->addWidget(crearGrupoExamenes());
    lc->addWidget(crearGrupoTemas());
    lc->addWidget(crearGrupoPuntuacion());
    lc->addWidget(crearGrupoOpciones());
    auto *bGenerar = new QPushButton(QStringLiteral("Generar exámenes"));
    bGenerar->setMinimumHeight(36);
    QFont f = bGenerar->font();
    f.setBold(true);
    bGenerar->setFont(f);
    connect(bGenerar, &QPushButton::clicked, this, &VentanaPrincipal::generar);
    lc->addWidget(bGenerar);
    lc->addStretch();

    auto *scroll = new QScrollArea;
    scroll->setWidget(config);
    scroll->setWidgetResizable(true);
    scroll->setMinimumWidth(400);

    auto *divisor = new QSplitter;
    divisor->addWidget(scroll);
    divisor->addWidget(crearPanelResultados());
    divisor->setStretchFactor(1, 1);
    divisor->setSizes({430, 800});
    return divisor;
}

QWidget *VentanaPrincipal::crearGrupoConvocatoria()
{
    auto *gConv = new QGroupBox(QStringLiteral("Convocatoria"));
    auto *fConv = new QFormLayout(gConv);
    m_asignatura = new QLineEdit(QStringLiteral("Fundamentos de Informática"));
    m_convocatoria = new QComboBox;
    m_convocatoria->setEditable(true);
    m_convocatoria->addItems({QStringLiteral("Febrero"), QStringLiteral("Junio"), QStringLiteral("Septiembre")});
    m_anio = new QSpinBox;
    m_anio->setRange(2000, 2100);
    m_anio->setValue(QDate::currentDate().year());
    m_duracion = new QSpinBox;
    m_duracion->setRange(5, 300);
    m_duracion->setSuffix(QStringLiteral(" min"));
    m_duracion->setValue(60);
    m_instrucciones = new QPlainTextEdit(DatosConvocatoria().instrucciones);
    m_instrucciones->setMaximumHeight(52);
    m_instrucciones->setTabChangesFocus(true);
    fConv->addRow(QStringLiteral("Asignatura:"), m_asignatura);
    fConv->addRow(QStringLiteral("Convocatoria:"), m_convocatoria);
    fConv->addRow(QStringLiteral("Año:"), m_anio);
    fConv->addRow(QStringLiteral("Duración:"), m_duracion);
    fConv->addRow(QStringLiteral("Instrucciones:"), m_instrucciones);
    return gConv;
}

QWidget *VentanaPrincipal::crearGrupoExamenes()
{
    auto *gExamenes = new QGroupBox(QStringLiteral("Exámenes"));
    auto *le = new QVBoxLayout(gExamenes);
    m_listaExamenes = new QListWidget;
    m_listaExamenes->setToolTip(QStringLiteral("Se genera un examen por cada nombre marcado, en este orden.\n"
                                               "Doble clic para cambiar el nombre; arrastre para cambiar el orden."));
    m_listaExamenes->setDragDropMode(QAbstractItemView::InternalMove);
    m_listaExamenes->setMaximumHeight(150);
    le->addWidget(m_listaExamenes);
    auto *botonesExamenes = new QHBoxLayout;
    auto botonExamenes = [&](const QString &texto, auto accion) {
        auto *b = new QPushButton(texto);
        connect(b, &QPushButton::clicked, this, accion);
        botonesExamenes->addWidget(b);
    };
    auto marcarTodos = [this](bool marcar) {
        for (int i = 0; i < m_listaExamenes->count(); ++i)
            m_listaExamenes->item(i)->setCheckState(marcar ? Qt::Checked : Qt::Unchecked);
    };
    botonExamenes(QStringLiteral("Añadir"), &VentanaPrincipal::nuevoExamen);
    botonExamenes(QStringLiteral("Quitar"), &VentanaPrincipal::quitarExamen);
    botonExamenes(QStringLiteral("Todos"), [marcarTodos]() { marcarTodos(true); });
    botonExamenes(QStringLiteral("Ninguno"), [marcarTodos]() { marcarTodos(false); });
    le->addLayout(botonesExamenes);
    return gExamenes;
}

QWidget *VentanaPrincipal::crearGrupoTemas()
{
    auto *gTemas = new QGroupBox(QStringLiteral("Preguntas por tema"));
    auto *lt = new QVBoxLayout(gTemas);
    m_contenedorTemas = new QWidget;
    m_rejillaTemas = new QGridLayout(m_contenedorTemas);
    m_rejillaTemas->setContentsMargins(0, 0, 0, 0);
    lt->addWidget(m_contenedorTemas);
    auto *repartos = new QHBoxLayout;
    auto botonReparto = [&](const QString &texto, const QString &ayuda, int desde, int hasta) {
        auto *b = new QPushButton(texto);
        b->setToolTip(ayuda);
        connect(b, &QPushButton::clicked, this, [this, desde, hasta]() { aplicarReparto(desde, hasta); });
        repartos->addWidget(b);
    };
    botonReparto(QStringLiteral("T1–T4"), QStringLiteral("Primera parte: 3, 2, 3, 2 preguntas"), 1, 4);
    botonReparto(QStringLiteral("T5–T8"), QStringLiteral("Segunda parte: 2, 3, 2, 3 preguntas"), 5, 8);
    botonReparto(QStringLiteral("T1–T8"), QStringLiteral("Examen total de 20 preguntas"), 1, 8);
    botonReparto(QStringLiteral("Ninguna"), QStringLiteral("Poner todos los temas a 0"), 0, -1);
    lt->addLayout(repartos);
    m_total = new QLabel;
    lt->addWidget(m_total);
    return gTemas;
}

QWidget *VentanaPrincipal::crearGrupoPuntuacion()
{
    auto *gPunt = new QGroupBox(QStringLiteral("Puntuación"));
    auto *fPunt = new QFormLayout(gPunt);
    m_puntuacionAuto = new QCheckBox(QStringLiteral("Automática (10 puntos; cada fallo resta el 30 %)"));
    m_puntuacionAuto->setChecked(true);
    m_acierto = new QDoubleSpinBox;
    m_acierto->setRange(0, 10);
    m_acierto->setSingleStep(0.05);
    m_acierto->setPrefix(QStringLiteral("+ "));
    m_fallo = new QDoubleSpinBox;
    m_fallo->setRange(0, 10);
    m_fallo->setSingleStep(0.05);
    m_fallo->setPrefix(QStringLiteral("- "));
    m_acierto->setValue(DatosConvocatoria().puntosAcierto);
    m_fallo->setValue(DatosConvocatoria().puntosFallo);
    fPunt->addRow(m_puntuacionAuto);
    fPunt->addRow(QStringLiteral("Acierto:"), m_acierto);
    fPunt->addRow(QStringLiteral("Fallo:"), m_fallo);
    connect(m_puntuacionAuto, &QCheckBox::toggled, this, &VentanaPrincipal::actualizarPuntuacion);
    return gPunt;
}

QWidget *VentanaPrincipal::crearGrupoOpciones()
{
    auto *gOpc = new QGroupBox(QStringLiteral("Opciones"));
    auto *fOpc = new QFormLayout(gOpc);
    m_noRepetir = new QCheckBox(QStringLiteral("No repetir preguntas entre exámenes"));
    m_evitarRecientes = new QCheckBox(QStringLiteral("Evitar las preguntas de las últimas"));
    m_evitarRecientes->setToolTip(QStringLiteral("Deja para el final las preguntas (y sus variantes) que salieron en las últimas "
                                                 "convocatorias registradas, empezando por las más recientes"));
    m_numRecientes = new QSpinBox;
    m_numRecientes->setRange(1, 99);
    m_numRecientes->setValue(3);
    m_numRecientes->setSuffix(QStringLiteral(" convocatorias"));
    connect(m_evitarRecientes, &QCheckBox::toggled, m_numRecientes, &QWidget::setEnabled);
    m_evitarRecientes->setChecked(true);
    m_evitarOld = new QCheckBox(QStringLiteral("Evitar también las marcadas «old»"));
    m_evitarOld->setToolTip(QStringLiteral("Deja para el final (aunque antes que las de las convocatorias registradas) las "
                                           "preguntas que ya salieron en exámenes anteriores al historial"));
    m_evitarOld->setChecked(true);
    auto *filaRecientes = new QHBoxLayout;
    filaRecientes->addWidget(m_evitarRecientes);
    filaRecientes->addWidget(m_numRecientes);
    filaRecientes->addStretch();
    m_noRepetir->setChecked(true);
    m_incluirRevisar = new QCheckBox(QStringLiteral("Incluir preguntas marcadas para revisar"));
    m_incluirRevisar->setChecked(true);
    m_ordenAleatorio = new QCheckBox(QStringLiteral("Orden aleatorio (si no, por tema y número)"));
    m_barajarOpciones = new QCheckBox(QStringLiteral("Barajar las opciones (la correcta, repartida entre a)–d))"));
    m_barajarOpciones->setToolTip(QStringLiteral("En cada examen, la correcta sale en a), b), c) y d) por igual y nunca tres veces "
                                                 "seguidas en la misma letra (si se puede).\nNo se barajan las preguntas con "
                                                 "opciones del tipo «Todas las anteriores»."));
    m_semilla = new QLineEdit;
    m_semilla->setValidator(new QIntValidator(0, 2147483647, m_semilla));
    m_semilla->setPlaceholderText(QStringLiteral("aleatoria"));
    m_semilla->setToolTip(QStringLiteral("Con la misma semilla y la misma configuración se obtienen los mismos exámenes"));
    fOpc->addRow(m_noRepetir);
    fOpc->addRow(filaRecientes);
    fOpc->addRow(m_evitarOld);
    fOpc->addRow(m_incluirRevisar);
    fOpc->addRow(m_ordenAleatorio);
    fOpc->addRow(m_barajarOpciones);
    fOpc->addRow(QStringLiteral("Semilla:"), m_semilla);
    return gOpc;
}

QWidget *VentanaPrincipal::crearPanelResultados()
{
    auto *resultados = new QWidget;
    auto *lr = new QVBoxLayout(resultados);
    auto *barra = new QHBoxLayout;
    m_selectorModelo = new QComboBox;
    m_selectorModelo->setMinimumWidth(110);
    m_selectorModelo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_verSoluciones = new QCheckBox(QStringLiteral("Mostrar soluciones"));
    m_infoGeneracion = new QLabel;
    m_infoGeneracion->setStyleSheet(QStringLiteral("color: gray"));
    m_infoGeneracion->setTextInteractionFlags(Qt::TextSelectableByMouse);
    barra->addWidget(new QLabel(QStringLiteral("Examen:")));
    barra->addWidget(m_selectorModelo);
    barra->addWidget(m_verSoluciones);
    m_correctasPorLetra = new QLabel;
    m_correctasPorLetra->setToolTip(QStringLiteral("Cuántas veces es correcta cada letra en este examen"));
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
    m_avisos->setStyleSheet(QLatin1String(kEstiloAviso));
    m_avisos->hide();
    lr->addWidget(m_avisos);
    m_visorExamen = new QTextBrowser;
    m_visorExamen->setHtml(QStringLiteral("<p style=\"color:gray\">Elija cuántas preguntas quiere de cada tema y pulse "
                                          "<b>Generar exámenes</b>.</p>"));
    lr->addWidget(m_visorExamen, 1);
    // Dos filas de botones (exportar; registrar y practicar) para que la ventana quepa en pantallas pequeñas.
    auto *exportar = new QHBoxLayout;
    auto *otras = new QHBoxLayout;
    auto boton = [&](QHBoxLayout *fila, const QString &texto, void (VentanaPrincipal::*slot)()) {
        auto *b = new QPushButton(texto);
        b->setEnabled(false);
        connect(b, &QPushButton::clicked, this, slot);
        fila->addWidget(b);
        m_botonesExamen << b;
    };
    boton(exportar, QStringLiteral("Imprimir…"), &VentanaPrincipal::vistaPreviaImpresion);
    boton(exportar, QStringLiteral("Exportar PDF…"), &VentanaPrincipal::exportarPdf);
    boton(exportar, QStringLiteral("Soluciones PDF…"), &VentanaPrincipal::exportarSolucionesPdf);
    boton(exportar, QStringLiteral("Exportar HTML…"), &VentanaPrincipal::exportarHtml);
    boton(exportar, QStringLiteral("Exportar LaTeX…"), &VentanaPrincipal::exportarLatex);
    exportar->addStretch();
    otras->addStretch();
    boton(otras, QStringLiteral("Registrar convocatoria…"), &VentanaPrincipal::registrarConvocatoria);
    boton(otras, QStringLiteral("Practicar este examen…"), &VentanaPrincipal::practicar);
    lr->addLayout(exportar);
    lr->addLayout(otras);

    connect(m_selectorModelo, &QComboBox::currentIndexChanged, this, &VentanaPrincipal::mostrarModelo);
    connect(m_verSoluciones, &QCheckBox::toggled, this, &VentanaPrincipal::mostrarModelo);
    return resultados;
}

QWidget *VentanaPrincipal::crearPestanaBanco()
{
    auto *w = new QWidget;
    auto *l = new QVBoxLayout(w);

    auto *filtros = new QHBoxLayout;
    m_filtroTema = new QComboBox;
    m_filtroEstado = new QComboBox;
    m_filtroEstado->addItem(QStringLiteral("Todas"), FiltroPreguntas::Todas);
    m_filtroEstado->addItem(QStringLiteral("Activas"), FiltroPreguntas::Activas);
    m_filtroEstado->addItem(QStringLiteral("Anuladas"), FiltroPreguntas::Anuladas);
    m_filtroEstado->addItem(QStringLiteral("Obsoletas"), FiltroPreguntas::Obsoletas);
    m_filtroEstado->addItem(QStringLiteral("Para revisar"), FiltroPreguntas::ParaRevisar);
    m_filtroEstado->addItem(QStringLiteral("Con imagen"), FiltroPreguntas::ConImagen);
    m_filtroTexto = new QLineEdit;
    m_filtroTexto->setPlaceholderText(QStringLiteral("Buscar texto o código (T1-22b)…"));
    m_filtroTexto->setClearButtonEnabled(true);
    m_cuenta = new QLabel;
    filtros->addWidget(new QLabel(QStringLiteral("Tema:")));
    filtros->addWidget(m_filtroTema, 1);
    filtros->addWidget(new QLabel(QStringLiteral("Estado:")));
    filtros->addWidget(m_filtroEstado);
    filtros->addWidget(m_filtroTexto, 2);
    filtros->addWidget(m_cuenta);
    l->addLayout(filtros);

    m_modelo = new ModeloPreguntas(this);
    m_filtro = new FiltroPreguntas(this);
    m_filtro->setSourceModel(m_modelo);
    m_tabla = new QTableView;
    m_tabla->setModel(m_filtro);
    m_tabla->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabla->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tabla->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tabla->setSortingEnabled(true);
    m_tabla->sortByColumn(ModeloPreguntas::ColCodigo, Qt::AscendingOrder);
    m_tabla->verticalHeader()->hide();
    m_tabla->setWordWrap(false);
    m_tabla->horizontalHeader()->setSectionResizeMode(ModeloPreguntas::ColEnunciado, QHeaderView::Stretch);
    m_tabla->setColumnWidth(ModeloPreguntas::ColTema, 50);
    m_tabla->setColumnWidth(ModeloPreguntas::ColCodigo, 80);
    m_tabla->setColumnWidth(ModeloPreguntas::ColEstado, 130);
    m_tabla->setColumnWidth(ModeloPreguntas::ColCorrecta, 50);
    m_ficha = new QTextBrowser;

    auto *divisor = new QSplitter;
    divisor->addWidget(m_tabla);
    divisor->addWidget(m_ficha);
    divisor->setStretchFactor(0, 3);
    divisor->setStretchFactor(1, 2);
    l->addWidget(divisor, 1);

    auto *botones = new QHBoxLayout;
    auto boton = [&](const QString &texto, void (VentanaPrincipal::*slot)()) {
        auto *b = new QPushButton(texto);
        connect(b, &QPushButton::clicked, this, slot);
        botones->addWidget(b);
    };
    boton(QStringLiteral("Nueva…"), &VentanaPrincipal::nuevaPregunta);
    boton(QStringLiteral("Editar…"), &VentanaPrincipal::editarPregunta);
    boton(QStringLiteral("Duplicar como variante…"), &VentanaPrincipal::duplicarPregunta);
    boton(QStringLiteral("Eliminar"), &VentanaPrincipal::eliminarPregunta);
    botones->addStretch();
    l->addLayout(botones);

    connect(m_filtroTema, &QComboBox::currentIndexChanged, this, &VentanaPrincipal::filtrar);
    connect(m_filtroEstado, &QComboBox::currentIndexChanged, this, &VentanaPrincipal::filtrar);
    connect(m_filtroTexto, &QLineEdit::textChanged, this, &VentanaPrincipal::filtrar);
    connect(m_tabla->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &VentanaPrincipal::seleccionCambiada);
    connect(m_tabla, &QTableView::doubleClicked, this, &VentanaPrincipal::editarPregunta);
    return w;
}

void VentanaPrincipal::crearMenus()
{
    QMenu *archivo = menuBar()->addMenu(QStringLiteral("&Archivo"));
    archivo->addAction(QStringLiteral("&Abrir banco de preguntas…"), this, &VentanaPrincipal::abrirOtraBaseDatos);
    archivo->addAction(QStringLiteral("&Nuevo banco de preguntas…"), this, &VentanaPrincipal::nuevaBaseDatos);
    archivo->addSeparator();
    archivo->addAction(QStringLiteral("&Importar preguntas (JSON)…"), this, &VentanaPrincipal::importarJson);
    archivo->addAction(QStringLiteral("&Exportar preguntas (JSON)…"), this, &VentanaPrincipal::exportarJson);
    archivo->addSeparator();
    archivo->addAction(QStringLiteral("&Historial de convocatorias…"), this, &VentanaPrincipal::historialConvocatorias);
    archivo->addSeparator();
    archivo->addAction(QStringLiteral("&Salir"), QKeySequence::Quit, this, &QWidget::close);

    QMenu *editar = menuBar()->addMenu(QStringLiteral("&Editar"));
    editar->addAction(QStringLiteral("&Temas…"), this, &VentanaPrincipal::gestionarTemas);
    editar->addAction(QStringLiteral("&Nueva pregunta…"), QKeySequence::New, this, &VentanaPrincipal::nuevaPregunta);
    editar->addAction(QStringLiteral("&Revisar el estilo del banco…"), this, &VentanaPrincipal::revisarEstilo);

    QMenu *ayuda = menuBar()->addMenu(QStringLiteral("A&yuda"));
    ayuda->addAction(QStringLiteral("&Acerca de…"), this, &VentanaPrincipal::acercaDe);
}

// ---------------------------------------------------------------------------------------------
// Base de datos

bool VentanaPrincipal::abrirBaseDatos(const QString &ruta)
{
    if (!m_bd.abrir(ruta)) {
        QMessageBox::critical(this, windowTitle(),
                              QStringLiteral("No se pudo abrir la base de datos\n%1\n\n%2").arg(ruta, m_bd.ultimoError()));
        return false;
    }
    m_resultado = ResultadoGeneracion();
    m_selectorModelo->clear();
    m_correctasPorLetra->clear();
    for (QPushButton *b : m_botonesExamen)
        b->setEnabled(false);
    recargarDatos();
    QSettings().setValue(QStringLiteral("bd/ruta"), ruta);
    return true;
}

bool VentanaPrincipal::abrirBancoInicial(const QString &rutaIndicada)
{
    const QString ruta = rutaIndicada.isEmpty() ? QSettings().value(QStringLiteral("bd/ruta")).toString()
                                                : QFileInfo(rutaIndicada).absoluteFilePath();
    // El de la línea de órdenes se crea si no existe; el de la configuración, no (se habrá movido).
    if (!ruta.isEmpty() && (!rutaIndicada.isEmpty() || QFileInfo::exists(ruta)) && abrirBaseDatos(ruta))
        return true;
    while (!m_bd.abierta()) {
        QMessageBox caja(QMessageBox::Question, windowTitle(),
                         ruta.isEmpty() ? QStringLiteral("Elija el banco de preguntas (fichero .db) con el que quiere trabajar.")
                                        : QStringLiteral("No se encuentra el banco de preguntas\n%1\n\n"
                                                         "Elija dónde está o cree uno nuevo.").arg(QDir::toNativeSeparators(ruta)),
                         QMessageBox::NoButton, this);
        QPushButton *abrir = caja.addButton(QStringLiteral("Abrir banco…"), QMessageBox::AcceptRole);
        QPushButton *nuevo = caja.addButton(QStringLiteral("Nuevo banco…"), QMessageBox::ActionRole);
        caja.addButton(QStringLiteral("Salir"), QMessageBox::RejectRole);
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
    const QString ruta = QFileDialog::getOpenFileName(this, QStringLiteral("Abrir banco de preguntas"),
                                                      QFileInfo(m_bd.ruta()).absolutePath(),
                                                      QStringLiteral("Bases de datos SQLite (*.db *.sqlite);;Todos (*)"));
    if (!ruta.isEmpty())
        abrirBaseDatos(ruta);
}

void VentanaPrincipal::nuevaBaseDatos()
{
    const QString ruta = QFileDialog::getSaveFileName(this, QStringLiteral("Nuevo banco de preguntas"),
                                                      QFileInfo(m_bd.ruta()).absolutePath() + QStringLiteral("/preguntas_nueva.db"),
                                                      QStringLiteral("Bases de datos SQLite (*.db)"));
    if (ruta.isEmpty())
        return;
    if (QFileInfo::exists(ruta)) {
        if (ruta == m_bd.ruta()) {
            QMessageBox::warning(this, windowTitle(), QStringLiteral("Esa base de datos está abierta; elija otro nombre."));
            return;
        }
        QFile::remove(ruta);
    }
    if (abrirBaseDatos(ruta)
        && confirmar(this, windowTitle(), QStringLiteral("El banco nuevo está vacío. ¿Quiere importar ahora preguntas de un JSON?")))
        importarJson();
}

void VentanaPrincipal::importarJson()
{
    const QString ruta = QFileDialog::getOpenFileName(this, QStringLiteral("Importar preguntas"), QString(),
                                                      QStringLiteral("JSON (*.json)"));
    if (ruta.isEmpty())
        return;
    QMessageBox caja(QMessageBox::Question, windowTitle(),
                     QStringLiteral("¿Cómo quiere importar las preguntas?\n\n"
                                    "Añadir/actualizar: las preguntas con el mismo tema, número y variante se sobrescriben; el resto se conservan.\n"
                                    "Reemplazar todo: se borran antes todos los temas y preguntas."),
                     QMessageBox::NoButton, this);
    QPushButton *anadir = caja.addButton(QStringLiteral("Añadir/actualizar"), QMessageBox::AcceptRole);
    QPushButton *reemplazar = caja.addButton(QStringLiteral("Reemplazar todo"), QMessageBox::DestructiveRole);
    caja.addButton(QStringLiteral("Cancelar"), QMessageBox::RejectRole);
    caja.exec();
    if (caja.clickedButton() != anadir && caja.clickedButton() != reemplazar)
        return;
    const int n = m_bd.importarJson(ruta, caja.clickedButton() == reemplazar);
    if (n < 0) {
        QMessageBox::warning(this, windowTitle(), QStringLiteral("Error al importar:\n%1").arg(m_bd.ultimoError()));
        return;
    }
    recargarDatos();
    QMessageBox::information(this, windowTitle(), QStringLiteral("Se han importado %1 preguntas.").arg(n));
}

void VentanaPrincipal::exportarJson()
{
    const QString ruta = QFileDialog::getSaveFileName(this, QStringLiteral("Exportar preguntas"),
                                                      QStringLiteral("preguntas_%1.json").arg(QDate::currentDate().toString(Qt::ISODate)),
                                                      QStringLiteral("JSON (*.json)"));
    if (ruta.isEmpty())
        return;
    if (!m_bd.exportarJson(ruta))
        QMessageBox::warning(this, windowTitle(), m_bd.ultimoError());
    else
        statusBar()->showMessage(QStringLiteral("Preguntas exportadas a %1").arg(ruta), 5000);
}

void VentanaPrincipal::gestionarTemas()
{
    DialogoTemas d(&m_bd, this);
    if (d.exec() == QDialog::Accepted)
        recargarDatos();
}

void VentanaPrincipal::revisarEstilo()
{
    DialogoEstilo dialogo([this]() { return m_preguntas; },
                          [this](int id) {
                              for (const Pregunta &p : std::as_const(m_preguntas)) {
                                  if (p.id == id) {
                                      editar(p);   // recarga m_preguntas: no se sigue recorriendo
                                      return;
                                  }
                              }
                          },
                          m_bd.ruta(), this);
    dialogo.exec();
}

void VentanaPrincipal::acercaDe()
{
    QMessageBox caja(this);
    caja.setWindowTitle(QStringLiteral("Acerca de"));
    const QPixmap foto = QPixmap(QStringLiteral(":/recursos/manuel-arias-calleja.jpg"))
                             .scaledToWidth(160, Qt::SmoothTransformation);
    caja.setIconPixmap(foto);
    caja.setTextFormat(Qt::RichText);
    caja.setText(QStringLiteral("<h3>Generador de exámenes de Fundamentos de Informática</h3>"
                                "<p>Versión %1</p>"
                                "<p>Desarrollado por Manuel Arias Calleja</p>"
                                "<p>Genera exámenes tipo test eligiendo al azar preguntas de la base de datos, "
                                "con el número de preguntas que se indique de cada tema.</p>"
                                "<p>Nunca pone en un mismo examen dos variantes de una pregunta (3a/3b) ni "
                                "pseudorrepetidas, y puede evitar que se repitan preguntas entre exámenes.</p>"
                                "<p>Base de datos: <tt>%2</tt></p><p>Configuración: <tt>%3</tt></p><p>Qt %4</p>")
                     .arg(QStringLiteral(VERSION_APLICACION), QDir::toNativeSeparators(m_bd.ruta()).toHtmlEscaped(),
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

    const int temaFiltro = m_filtroTema->currentData().toInt();
    m_filtroTema->blockSignals(true);
    m_filtroTema->clear();
    m_filtroTema->addItem(QStringLiteral("Todos los temas"), 0);
    for (const Tema &t : m_temas)
        m_filtroTema->addItem(t.nombre(), t.numero);
    m_filtroTema->setCurrentIndex(qMax(0, m_filtroTema->findData(temaFiltro)));
    m_filtroTema->blockSignals(false);

    m_modelo->establecer(m_preguntas);
    filtrar();
    reconstruirTemas();

    if (seleccionarId) {
        const int fila = m_modelo->filaDeId(seleccionarId);
        const QModelIndex i = m_filtro->mapFromSource(m_modelo->index(fila, 0));
        if (i.isValid()) {
            m_tabla->setCurrentIndex(i);
            m_tabla->scrollTo(i);
        }
    }
    int activas = 0;
    for (const Pregunta &p : m_preguntas)
        activas += p.activa() ? 1 : 0;
    statusBar()->showMessage(QStringLiteral("Base de datos: %1  ·  %2 temas, %3 preguntas (%4 activas)")
                                 .arg(QDir::toNativeSeparators(m_bd.ruta()))
                                 .arg(m_temas.size()).arg(m_preguntas.size()).arg(activas));
}

// Rejilla de "preguntas por tema" de la pestaña de examen.
void VentanaPrincipal::reconstruirTemas()
{
    for (auto it = m_cantidades.cbegin(); it != m_cantidades.cend(); ++it)
        m_cantidadesGuardadas[it.key()] = it.value()->value();
    QLayoutItem *item;
    while ((item = m_rejillaTemas->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    m_cantidades.clear();

    const QMap<int, int> disponibles = Generador::gruposDisponibles(m_preguntas, m_incluirRevisar->isChecked());
    int fila = 0;
    for (const Tema &t : m_temas) {
        auto *nombre = new QLabel(QStringLiteral("T%1  %2").arg(t.numero).arg(t.titulo));
        nombre->setToolTip(t.nombre() + (t.autor.isEmpty() ? QString() : QStringLiteral(" (%1)").arg(t.autor)));
        auto *spin = new QSpinBox;
        spin->setRange(0, qMax(0, disponibles.value(t.numero)));
        spin->setValue(m_cantidadesGuardadas.value(t.numero, 0));
        auto *max = new QLabel(QStringLiteral("de %1").arg(disponibles.value(t.numero)));
        max->setStyleSheet(QStringLiteral("color: gray"));
        max->setToolTip(QStringLiteral("Preguntas distintas disponibles (las variantes a, b, c... cuentan como una)"));
        m_rejillaTemas->addWidget(nombre, fila, 0);
        m_rejillaTemas->addWidget(spin, fila, 1);
        m_rejillaTemas->addWidget(max, fila, 2);
        connect(spin, &QSpinBox::valueChanged, this, &VentanaPrincipal::actualizarTotal);
        m_cantidades.insert(t.numero, spin);
        ++fila;
    }
    m_rejillaTemas->setColumnStretch(0, 1);
    actualizarTotal();
}

// ---------------------------------------------------------------------------------------------
// Configuración

void VentanaPrincipal::leerConfiguracion()
{
    QSettings s;
    restoreGeometry(s.value(QStringLiteral("ventana/geometria")).toByteArray());
    for (const Ajuste &a : ajustes())
        leerAjuste(s, a.clave, a.control);
    const QStringList examenes = s.value(QStringLiteral("examenes/nombres"), kExamenesPorDefecto).toStringList();
    const QStringList marcados = s.value(QStringLiteral("examenes/marcados"), examenes).toStringList();
    for (const QString &nombre : examenes)
        anadirExamen(nombre, marcados.contains(nombre));
    const QVariantMap cantidades = s.value(QStringLiteral("examen/temas")).toMap();
    if (cantidades.isEmpty()) {
        m_cantidadesGuardadas = kRepartoPorDefecto;
    } else {
        for (auto it = cantidades.cbegin(); it != cantidades.cend(); ++it)
            m_cantidadesGuardadas.insert(it.key().toInt(), it.value().toInt());
    }
    connect(m_incluirRevisar, &QCheckBox::toggled, this, &VentanaPrincipal::reconstruirTemas);
    actualizarPuntuacion();
}

void VentanaPrincipal::guardarConfiguracion()
{
    QSettings s;
    s.setValue(QStringLiteral("ventana/geometria"), saveGeometry());
    for (const Ajuste &a : ajustes())
        s.setValue(a.clave, valorAjuste(a.control));
    QStringList examenes, marcados;
    for (int i = 0; i < m_listaExamenes->count(); ++i) {
        const QListWidgetItem *item = m_listaExamenes->item(i);
        const QString nombre = item->text().trimmed();
        if (nombre.isEmpty())
            continue;
        examenes << nombre;
        if (item->checkState() == Qt::Checked)
            marcados << nombre;
    }
    s.setValue(QStringLiteral("examenes/nombres"), examenes);
    s.setValue(QStringLiteral("examenes/marcados"), marcados);
    QVariantMap cantidades;
    for (auto it = m_cantidades.cbegin(); it != m_cantidades.cend(); ++it)
        cantidades.insert(QString::number(it.key()), it.value()->value());
    s.setValue(QStringLiteral("examen/temas"), cantidades);
}

QVector<VentanaPrincipal::Ajuste> VentanaPrincipal::ajustes() const
{
    // En este orden se leen: «Automática» antes que los puntos, para que se queden los guardados.
    return {
        {QStringLiteral("examen/asignatura"), m_asignatura},
        {QStringLiteral("examen/duracion"), m_duracion},
        {QStringLiteral("examen/instrucciones"), m_instrucciones},
        {QStringLiteral("examen/anio"), m_anio},
        {QStringLiteral("examen/nombreConvocatoria"), m_convocatoria},
        {QStringLiteral("examen/noRepetir"), m_noRepetir},
        {QStringLiteral("examen/evitarRecientes"), m_evitarRecientes},
        {QStringLiteral("examen/numRecientes"), m_numRecientes},
        {QStringLiteral("examen/evitarOld"), m_evitarOld},
        {QStringLiteral("examen/incluirRevisar"), m_incluirRevisar},
        {QStringLiteral("examen/ordenAleatorio"), m_ordenAleatorio},
        {QStringLiteral("examen/barajarOpciones"), m_barajarOpciones},
        {QStringLiteral("examen/puntuacionAuto"), m_puntuacionAuto},
        {QStringLiteral("examen/acierto"), m_acierto},
        {QStringLiteral("examen/fallo"), m_fallo},
    };
}

void VentanaPrincipal::closeEvent(QCloseEvent *evento)
{
    guardarConfiguracion();
    evento->accept();
}

// ---------------------------------------------------------------------------------------------
// Generación de exámenes

void VentanaPrincipal::aplicarReparto(int primerTema, int ultimoTema)
{
    for (auto it = m_cantidades.begin(); it != m_cantidades.end(); ++it) {
        const int t = it.key();
        it.value()->setValue(t >= primerTema && t <= ultimoTema ? kRepartoHabitual.value(t, 2) : 0);
    }
}

void VentanaPrincipal::actualizarTotal()
{
    int total = 0;
    for (QSpinBox *s : std::as_const(m_cantidades))
        total += s->value();
    m_total->setText(QStringLiteral("<b>Total: %1 preguntas</b>").arg(total));
    actualizarPuntuacion();
}

void VentanaPrincipal::actualizarPuntuacion()
{
    const bool automatica = m_puntuacionAuto->isChecked();
    m_acierto->setEnabled(!automatica);
    m_fallo->setEnabled(!automatica);
    if (!automatica)
        return;
    int total = 0;
    for (QSpinBox *s : std::as_const(m_cantidades))
        total += s->value();
    if (total == 0)
        return;
    const double acierto = 10.0 / total;
    m_acierto->setValue(acierto);
    m_fallo->setValue(qRound(acierto * 0.3 * 100) / 100.0);
}

QListWidgetItem *VentanaPrincipal::anadirExamen(const QString &nombre, bool marcado)
{
    auto *item = new QListWidgetItem(nombre, m_listaExamenes);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable
                   | Qt::ItemIsDragEnabled);
    item->setCheckState(marcado ? Qt::Checked : Qt::Unchecked);
    return item;
}

void VentanaPrincipal::nuevoExamen()
{
    QListWidgetItem *item = anadirExamen(QStringLiteral("Nuevo examen"), true);
    m_listaExamenes->setCurrentItem(item);
    m_listaExamenes->editItem(item);
}

void VentanaPrincipal::quitarExamen()
{
    QListWidgetItem *item = m_listaExamenes->currentItem();
    if (item && confirmar(this, windowTitle(), QStringLiteral("¿Quitar «%1» de la lista de exámenes?").arg(item->text())))
        delete item;
}

DatosConvocatoria VentanaPrincipal::datosConvocatoria() const
{
    DatosConvocatoria d;
    d.asignatura = m_asignatura->text().trimmed();
    d.convocatoria = m_convocatoria->currentText().trimmed();
    d.anio = m_anio->value();
    d.duracionMinutos = m_duracion->value();
    d.instrucciones = m_instrucciones->toPlainText().trimmed();
    d.puntosAcierto = m_acierto->value();
    d.puntosFallo = m_fallo->value();
    return d;
}

ConfigExamen VentanaPrincipal::configuracionExamen() const
{
    ConfigExamen c;
    for (auto it = m_cantidades.cbegin(); it != m_cantidades.cend(); ++it)
        if (it.value()->value() > 0)
            c.preguntasPorTema.insert(it.key(), it.value()->value());
    for (int i = 0; i < m_listaExamenes->count(); ++i)
        if (m_listaExamenes->item(i)->checkState() == Qt::Checked)
            c.nombresModelos << m_listaExamenes->item(i)->text().trimmed();
    c.noRepetirEntreModelos = m_noRepetir->isChecked();
    c.incluirParaRevisar = m_incluirRevisar->isChecked();
    c.ordenAleatorio = m_ordenAleatorio->isChecked();
    c.barajarOpciones = m_barajarOpciones->isChecked();
    c.evitarOld = m_evitarOld->isChecked();
    c.semilla = m_semilla->text().toUInt();
    return c;
}

void VentanaPrincipal::generar()
{
    ConfigExamen config = configuracionExamen();
    if (config.preguntasPorTema.isEmpty()) {
        QMessageBox::information(this, windowTitle(), QStringLiteral("Indique cuántas preguntas quiere de cada tema."));
        return;
    }
    QString error;
    if (config.nombresModelos.isEmpty())
        error = QStringLiteral("Marque en la lista los exámenes que quiere generar.");
    else if (config.nombresModelos.contains(QString()))
        error = QStringLiteral("Hay un examen marcado sin nombre.");
    else if (QSet<QString>(config.nombresModelos.cbegin(), config.nombresModelos.cend()).size() != config.nombresModelos.size())
        error = QStringLiteral("Hay dos exámenes marcados con el mismo nombre.");
    if (!error.isEmpty()) {
        QMessageBox::information(this, windowTitle(), error);
        return;
    }
    // Los exámenes que se generan de nuevo no cuentan como usados: sustituirán a los registrados con su nombre.
    if (m_evitarRecientes->isChecked())
        config.usoReciente = m_bd.usoReciente(m_numRecientes->value(), m_anio->value(),
                                              m_convocatoria->currentText().trimmed(), config.nombresModelos);
    m_resultado = Generador::generar(m_preguntas, config);
    guardarConfiguracion();
    presentarResultado();
}

void VentanaPrincipal::presentarResultado(const QString &origen)
{
    m_selectorModelo->blockSignals(true);
    m_selectorModelo->clear();
    for (const Modelo &m : std::as_const(m_resultado.modelos))
        m_selectorModelo->addItem(m.nombre);
    m_selectorModelo->blockSignals(false);

    QStringList avisos = m_resultado.avisos;
    bool caben = true;
    const int nivel = hayExamen() ? Maquetador::nivelParaCaber(m_resultado.modelos, datosConvocatoria(), &caben) : 0;
    if (!caben) {
        const int paginas = Maquetador::paginasObjetivo(m_resultado.modelos.first().preguntas.size());
        avisos << QStringLiteral("Algún examen no cabe en %1 ni con la maqueta más compacta: acorte las instrucciones o quite preguntas.")
                      .arg(paginas == 1 ? QStringLiteral("una página") : QStringLiteral("%1 páginas").arg(paginas));
    }
    m_avisos->setText(avisos.join(QLatin1Char('\n')));
    m_avisos->setVisible(!avisos.isEmpty());
    QString info = origen.isEmpty() ? QString() : origen + QStringLiteral("  ·  ");
    info += QStringLiteral("Semilla: %1").arg(m_resultado.semilla);
    if (nivel > 0)
        info += QStringLiteral("  ·  Maqueta compacta (nivel %1 de %2)").arg(nivel).arg(Maquetador::numNiveles() - 1);
    m_infoGeneracion->setText(info);
    m_infoGeneracion->setToolTip(QStringLiteral("Escriba esta semilla en Opciones para volver a obtener los mismos exámenes.\n"
                                                "La maqueta se compacta (opciones en dos columnas, menos separación, letra menor) "
                                                "hasta que cada examen cabe en una página por cada 10 preguntas."));
    for (QPushButton *b : std::as_const(m_botonesExamen))
        b->setEnabled(hayExamen());
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
    m_correctasPorLetra->setText(QStringLiteral("Correctas: ") + Maquetador::textoCorrectasPorLetra(m_resultado.modelos.at(i)));
    const QString html = Maquetador::examenHtml(m_resultado.modelos.at(i), datosConvocatoria(), m_verSoluciones->isChecked(),
                                                nivelCompactacion());
    ponerDocumento(m_visorExamen, Maquetador::crearDocumento(html, Maquetador::preguntasDe(m_resultado.modelos), m_visorExamen));
}

QString VentanaPrincipal::nombreBaseFichero() const
{
    // «2026_Junio»: el año delante, para que los ficheros de varios años queden ordenados.
    return limpiarNombreFichero(QStringLiteral("%1 %2").arg(m_anio->value()).arg(m_convocatoria->currentText().trimmed()));
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

int VentanaPrincipal::nivelCompactacion() const
{
    return hayExamen() ? Maquetador::nivelParaCaber(m_resultado.modelos, datosConvocatoria()) : 0;
}

void VentanaPrincipal::informarExportacion(const QStringList &escritos, const QStringList &fallidos, const QString &nota)
{
    if (!fallidos.isEmpty())
        QMessageBox::warning(this, windowTitle(), QStringLiteral("No se pudieron escribir estos ficheros:\n%1").arg(fallidos.join(QLatin1Char('\n'))));
    if (escritos.isEmpty())
        return;
    QString mensaje = escritos.size() == 1
                          ? QStringLiteral("Fichero guardado en %1").arg(QDir::toNativeSeparators(escritos.first()))
                          : QStringLiteral("%1 ficheros guardados en %2").arg(escritos.size()).arg(QDir::toNativeSeparators(QFileInfo(escritos.first()).absolutePath()));
    if (!nota.isEmpty())
        mensaje += QStringLiteral(" (%1)").arg(nota);
    statusBar()->showMessage(mensaje, 8000);
}

void VentanaPrincipal::vistaPreviaImpresion()
{
    if (!hayExamen())
        return;
    QPrinter printer(QPrinter::HighResolution);
    Maquetador::configurarPagina(printer);
    QPrintPreviewDialog vista(&printer, this);
    vista.setWindowTitle(QStringLiteral("Vista previa de impresión"));
    vista.resize(1000, 900);
    const DatosConvocatoria d = datosConvocatoria();
    const int nivel = nivelCompactacion();
    connect(&vista, &QPrintPreviewDialog::paintRequested, this,
            [this, d, nivel](QPrinter *p) { Maquetador::imprimirExamenes(p, m_resultado.modelos, d, false, nivel); });
    vista.exec();
}

void VentanaPrincipal::exportar(const QString &titulo, const QString &nombreBase, const QString &extension,
                                Escritor escribir, bool soluciones, const QString &nota)
{
    if (!hayExamen())
        return;
    DialogoExportar dialogo(titulo, nombresModelos(), nombreBase, extension, this);
    if (dialogo.exec() != QDialog::Accepted)
        return;
    const DatosConvocatoria d = datosConvocatoria();
    const int nivel = nivelCompactacion();
    QStringList escritos, fallidos;
    for (const DialogoExportar::Fichero &f : dialogo.ficheros())
        (escribir(f.ruta, modelosDe(f.modelos), d, soluciones, nivel) ? escritos : fallidos) << f.ruta;
    informarExportacion(escritos, fallidos, nota);
}

// Cada examen marcado en el diálogo se guarda en su propio fichero (y, si se marca, otro con todos juntos).
void VentanaPrincipal::exportarPdf()
{
    exportar(QStringLiteral("Exportar exámenes a PDF"), nombreBaseFichero(), QStringLiteral("pdf"), &Exportacion::pdf, false);
}

// Igual que exportarPdf(), pero cada fichero lleva delante la hoja de soluciones de sus exámenes.
void VentanaPrincipal::exportarSolucionesPdf()
{
    exportar(QStringLiteral("Exportar soluciones a PDF"), nombreBaseFichero() + QStringLiteral("_soluciones"),
             QStringLiteral("pdf"), &Exportacion::pdf, true);
}

void VentanaPrincipal::exportarHtml()
{
    exportar(QStringLiteral("Exportar a HTML (editable en Word / LibreOffice)"), nombreBaseFichero(), QStringLiteral("html"),
             &Exportacion::html, m_verSoluciones->isChecked());
}

void VentanaPrincipal::exportarLatex()
{
    exportar(QStringLiteral("Exportar a LaTeX"), nombreBaseFichero(), QStringLiteral("tex"), &Exportacion::latex,
             m_verSoluciones->isChecked(), QStringLiteral("compílelos dos veces con pdflatex"));
}

void VentanaPrincipal::registrarConvocatoria()
{
    if (!hayExamen())
        return;
    const DatosConvocatoria d = datosConvocatoria();
    if (d.convocatoria.isEmpty()) {
        QMessageBox::information(this, windowTitle(), QStringLiteral("Indique la convocatoria (Febrero, Junio…)."));
        return;
    }
    const QStringList nombres = nombresModelos();
    QStringList yaRegistrados;
    for (const ExamenRegistrado &e : m_bd.examenesRegistrados())
        if (e.datos.anio == d.anio && e.datos.convocatoria == d.convocatoria && nombres.contains(e.nombre))
            yaRegistrados << e.nombre;
    QString texto = QStringLiteral("Se guardarán en el historial los exámenes de %1: %2.\n\n"
                                   "Al generar, se evitarán sus preguntas, y desde Archivo → Historial de convocatorias "
                                   "se podrán volver a cargar tal como están.")
                        .arg(d.convocatoriaYAnio(), nombres.join(QStringLiteral(", ")));
    if (!yaRegistrados.isEmpty())
        texto += QStringLiteral("\n\nYa había registrados con el mismo nombre, y se sustituirán: %1.")
                     .arg(yaRegistrados.join(QStringLiteral(", ")));
    if (!confirmar(this, QStringLiteral("Registrar convocatoria"), texto))
        return;
    if (!m_bd.registrarExamenes(m_resultado.modelos, d, m_resultado.semilla)) {
        QMessageBox::warning(this, windowTitle(), QStringLiteral("No se pudo registrar la convocatoria:\n%1").arg(m_bd.ultimoError()));
        return;
    }
    statusBar()->showMessage(QStringLiteral("Registrados en el historial los exámenes de %1").arg(d.convocatoriaYAnio()), 8000);
    seleccionCambiada();   // la ficha de la pregunta dice en qué exámenes ha salido
}

void VentanaPrincipal::historialConvocatorias()
{
    DialogoHistorial dialogo(&m_bd, this);
    if (dialogo.exec() == QDialog::Accepted)
        cargarRegistrados(dialogo.examenesElegidos());
    seleccionCambiada();   // por si se ha eliminado algún examen
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
    m_resultado = resultado;

    // Los datos de la convocatoria pasan a la ventana, para que lo que se exporte salga como cuando se registró.
    const DatosConvocatoria &d = primero->datos;
    m_asignatura->setText(d.asignatura);
    m_convocatoria->setCurrentText(d.convocatoria);
    m_anio->setValue(d.anio);
    m_duracion->setValue(d.duracionMinutos);
    m_instrucciones->setPlainText(d.instrucciones);
    if (qAbs(m_acierto->value() - d.puntosAcierto) > 0.001 || qAbs(m_fallo->value() - d.puntosFallo) > 0.001) {
        m_puntuacionAuto->setChecked(false);
        m_acierto->setValue(d.puntosAcierto);
        m_fallo->setValue(d.puntosFallo);
    }
    if (auto *pestanas = qobject_cast<QTabWidget *>(centralWidget()))
        pestanas->setCurrentIndex(0);
    presentarResultado(QStringLiteral("Registrada el %1").arg(QLocale().toString(primero->registrado.date(), QLocale::ShortFormat)));
}

void VentanaPrincipal::practicar()
{
    const int i = m_selectorModelo->currentIndex();
    if (i < 0 || i >= m_resultado.modelos.size())
        return;
    DialogoPractica d(m_resultado.modelos.at(i), datosConvocatoria(), this);
    d.exec();
}

// ---------------------------------------------------------------------------------------------
// Banco de preguntas

void VentanaPrincipal::filtrar()
{
    m_filtro->setTema(m_filtroTema->currentData().toInt());
    m_filtro->setEstado(FiltroPreguntas::FiltroEstado(m_filtroEstado->currentData().toInt()));
    m_filtro->setTexto(m_filtroTexto->text());
    const int n = m_filtro->rowCount();
    m_cuenta->setText(n == 1 ? QStringLiteral("1 pregunta") : QStringLiteral("%1 preguntas").arg(n));
    seleccionCambiada();
}

Pregunta VentanaPrincipal::preguntaSeleccionada(bool *ok)
{
    const QModelIndex i = m_filtro->mapToSource(m_tabla->currentIndex());
    *ok = i.isValid();
    return i.isValid() ? m_modelo->pregunta(i.row()) : Pregunta();
}

void VentanaPrincipal::seleccionCambiada()
{
    bool ok;
    const Pregunta p = preguntaSeleccionada(&ok);
    if (!ok) {
        m_ficha->clear();
        return;
    }
    QString html = Maquetador::fichaPreguntaHtml(p, nombreTema(m_temas, p.temaId));
    const QStringList usos = m_bd.usosDe(p.codigoCompleto());
    if (!usos.isEmpty())
        html.replace(QStringLiteral("</body>"), QStringLiteral("<p><b>Ha salido en:</b> %1</p></body>")
                                                    .arg(Maquetador::escapar(usos.join(QStringLiteral("; ")))));
    ponerDocumento(m_ficha, Maquetador::crearDocumento(html, {p}, m_ficha));
}

void VentanaPrincipal::editar(Pregunta p)
{
    if (m_temas.isEmpty()) {
        QMessageBox::information(this, windowTitle(), QStringLiteral("Primero cree algún tema (Editar → Temas)."));
        return;
    }
    DialogoPregunta d(m_temas, m_preguntas, p, this);
    while (d.exec() == QDialog::Accepted) {
        Pregunta nueva = d.pregunta();
        if (m_bd.guardarPregunta(nueva)) {
            recargarDatos(nueva.id);
            return;
        }
        QMessageBox::warning(this, windowTitle(), QStringLiteral("No se pudo guardar la pregunta:\n%1").arg(m_bd.ultimoError()));
    }
}

void VentanaPrincipal::nuevaPregunta()
{
    Pregunta p;
    const int temaFiltro = m_filtroTema->currentData().toInt();
    for (const Tema &t : std::as_const(m_temas))
        if (t.numero == temaFiltro || (temaFiltro == 0 && p.temaId == 0))
            p.temaId = t.id, p.temaNumero = t.numero;
    p.numero = siguienteNumero(m_preguntas, p.temaId);
    editar(p);
}

void VentanaPrincipal::editarPregunta()
{
    bool ok;
    const Pregunta p = preguntaSeleccionada(&ok);
    if (ok)
        editar(p);
}

void VentanaPrincipal::duplicarPregunta()
{
    bool ok;
    Pregunta p = preguntaSeleccionada(&ok);
    if (!ok)
        return;
    // Nueva variante del mismo número: siguiente letra libre.
    p.id = 0;
    p.variante = siguienteVariante(m_preguntas, p.temaId, p.numero);
    if (p.variante.isEmpty()) {
        QMessageBox::information(this, windowTitle(), QStringLiteral("%1 ya tiene variantes de la a a la z.").arg(p.grupo()));
        return;
    }
    p.old = false;
    editar(p);
}

void VentanaPrincipal::eliminarPregunta()
{
    bool ok;
    const Pregunta p = preguntaSeleccionada(&ok);
    if (!ok)
        return;
    if (!confirmar(this, windowTitle(),
                   QStringLiteral("¿Eliminar definitivamente la pregunta %1?\n\n%2\n\n"
                                  "(Si solo quiere que deje de salir en los exámenes, edítela y márquela como anulada.)")
                       .arg(p.codigoCompleto(), p.enunciado.left(200))))
        return;
    if (!m_bd.borrarPregunta(p.id))
        QMessageBox::warning(this, windowTitle(), m_bd.ultimoError());
    recargarDatos();
}
