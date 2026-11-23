#include "ventanaprincipal.h"
#include "verificacion.h"
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
#include "modelopreguntas.h"
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

// Botones de reparto por fila.
constexpr int kRepartosPorFila = 4;

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

// Lo que pone un reparto, para su botón: «T1: 3, T2: 2, T3: 3, T4: 2 (10 preguntas)».
QString describirReparto(const QMap<int, int> &reparto)
{
    QStringList temas;
    int total = 0;
    for (auto it = reparto.cbegin(); it != reparto.cend(); ++it) {
        temas << QStringLiteral("T%1: %2").arg(it.key()).arg(it.value());
        total += it.value();
    }
    //: Texto de un reparto: %1 son los temas y su número de preguntas («T1: 3, T2: 2»), %2 el total
    return QCoreApplication::translate("VentanaPrincipal", "%1 (%2 preguntas)").arg(temas.join(QStringLiteral(", "))).arg(total);
}

// Nombre que se propone para guardar un reparto: «T1–T4» si son temas seguidos; si no, «T1, T3, T5».
QString nombreDeReparto(const QMap<int, int> &reparto)
{
    const QList<int> temas = reparto.keys();
    if (temas.size() > 1 && temas.last() - temas.first() == temas.size() - 1)
        return QStringLiteral("T%1–T%2").arg(temas.first()).arg(temas.last());
    QStringList nombres;
    for (int t : temas)
        nombres << QStringLiteral("T%1").arg(t);
    return nombres.join(QStringLiteral(", "));
}

// Lista de exámenes la primera vez que se abre el programa; luego se recuerda la que se haya hecho.
const QStringList kExamenesPorDefecto{QStringLiteral("Nacional Original"), QStringLiteral("Nacional Reserva")};

// Carpetas de los diálogos (las guardan ellos en la configuración) que van con cada banco: la de las exportaciones
// (exámenes y Moodle) y la de los PDF de los exámenes anteriores.
const QStringList kCarpetasDelBanco{QStringLiteral("exportar/carpeta"), QStringLiteral("anteriores/carpeta")};

// Pone en un control el valor guardado de un ajuste y da el valor que hay que guardar de él, según su tipo.
void ponerAjuste(QWidget *control, const QVariant &valor)
{
    if (auto *c = qobject_cast<QCheckBox *>(control))
        c->setChecked(valor.toBool());
    else if (auto *n = qobject_cast<QSpinBox *>(control))
        n->setValue(valor.toInt());
    else if (auto *d = qobject_cast<QDoubleSpinBox *>(control))
        d->setValue(valor.toDouble());
    else if (auto *l = qobject_cast<QLineEdit *>(control))
        l->setText(valor.toString());
    else if (auto *t = qobject_cast<QPlainTextEdit *>(control))
        t->setPlainText(valor.toString());
    else if (auto *b = qobject_cast<QComboBox *>(control))
        b->setCurrentText(valor.toString());
    else   // un grupo de botones de opción: el que lleva ese valor (ver valorAjuste())
        for (QRadioButton *r : control->findChildren<QRadioButton *>())
            if (r->property("valor").toString() == valor.toString())
                r->setChecked(true);
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
    // Un grupo de botones de opción: el valor (propiedad «valor») del marcado.
    for (const QRadioButton *r : control->findChildren<QRadioButton *>())
        if (r->isChecked())
            return r->property("valor");
    return QVariant();
}

// La puntuación se guarda con esta clave. Las versiones anteriores tenían en su lugar la casilla «Automática»
// («examen/puntuacionAuto»): marcada, cada fallo restaba el 30 %; sin marcar, los puntos eran los de la ventana.
const QString kClavePuntuacion = QStringLiteral("examen/puntuacion");
const QString kClavePuntuacionAntigua = QStringLiteral("examen/puntuacionAuto");

QString puntuacionAntigua(const QVariant &automatica)
{
    return automatica.toBool() ? QStringLiteral("resta30") : QStringLiteral("personalizada");
}

} // namespace

VentanaPrincipal::VentanaPrincipal(QWidget *parent)
    : QMainWindow(parent)
{
    actualizarTitulo();
    auto *pestanas = new QTabWidget;
    pestanas->addTab(crearPestanaExamen(), tr("Generar examen"));
    pestanas->addTab(crearPestanaBanco(), tr("Banco de preguntas"));
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
    m_avisoJson->setStyleSheet(QString::fromLatin1(kEstiloAviso) + QStringLiteral("; padding: 1px 6px"));
    m_avisoJson->hide();
    connect(m_avisoJson, &QPushButton::clicked, this, &VentanaPrincipal::actualizarJsonBanco);
    statusBar()->addPermanentWidget(m_avisoJson);
    m_abrirCarpeta = new QPushButton(tr("Abrir la carpeta"));
    m_abrirCarpeta->setFlat(true);
    m_abrirCarpeta->setCursor(Qt::PointingHandCursor);
    m_abrirCarpeta->setStyleSheet(QString::fromLatin1(kEstiloInformacion) + QStringLiteral("; padding: 1px 6px"));
    m_abrirCarpeta->hide();
    connect(m_abrirCarpeta, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_abrirCarpeta->property("carpeta").toString()));
    });
    statusBar()->addPermanentWidget(m_abrirCarpeta);
    leerConfiguracion();
}

VentanaPrincipal::~VentanaPrincipal()
{
    // Al destruirse, la lista de exámenes se vacía y lo avisa cuando su recuadro ya se está destruyendo: sin esto, se
    // pondría el título de un recuadro a medio destruir.
    m_listaExamenes->model()->disconnect(this);
}

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
    // La puntuación y las opciones se eligen una vez y casi nunca se cambian: plegadas, con lo elegido en el título,
    // dejan a la vista lo que se decide cada vez (la convocatoria, los exámenes y las preguntas por tema).
    m_plegablePuntuacion = new GrupoPlegable(qobject_cast<QGroupBox *>(crearGrupoPuntuacion()),
                                             QStringLiteral("vista/plegadoPuntuacion"), true);
    m_plegablePuntuacion->setObjectName(QStringLiteral("plegablePuntuacion"));
    m_plegableOpciones = new GrupoPlegable(qobject_cast<QGroupBox *>(crearGrupoOpciones()),
                                           QStringLiteral("vista/plegadoOpciones"), true);
    m_plegableOpciones->setObjectName(QStringLiteral("plegableOpciones"));
    lc->addWidget(m_plegablePuntuacion);
    lc->addWidget(m_plegableOpciones);
    resumirPuntuacion();
    resumirOpciones();
    lc->addStretch();

    auto *scroll = new ZonaVertical;
    scroll->setWidget(config);
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

QWidget *VentanaPrincipal::crearGrupoConvocatoria()
{
    //: Convocatoria de examen (Febrero, Junio, Septiembre…): datos de la cabecera de los exámenes
    auto *gConv = new QGroupBox(tr("Convocatoria"));
    auto *fConv = new QFormLayout(gConv);
    m_asignatura = new QLineEdit;
    m_asignatura->setPlaceholderText(tr("Nombre de la asignatura"));
    m_asignatura->setToolTip(tr("Va en la cabecera de los exámenes. Se guarda en el banco de preguntas: "
                                "cada banco tiene el de su asignatura."));
    connect(m_asignatura, &QLineEdit::editingFinished, this, &VentanaPrincipal::guardarAsignatura);
    m_idiomaExamen = new QComboBox;
    for (const Idioma::Disponible &d : Idioma::disponibles())
        m_idiomaExamen->addItem(d.nombre, d.codigo);
    m_idiomaExamen->setToolTip(tr("Idioma de lo que pone el programa en los exámenes (instrucciones propuestas, puntuación, "
                                  "soluciones, meses de las convocatorias) y en Moodle, y de las pautas de estilo que se "
                                  "comprueban. Se guarda en el banco de preguntas: cada banco tiene el de su asignatura."));
    connect(m_idiomaExamen, &QComboBox::currentIndexChanged, this, &VentanaPrincipal::cambiarIdiomaExamen);
    // La convocatoria y el año no se guardan en la configuración: al abrir se propone la próxima.
    const DatosConvocatoria proxima = DatosConvocatoria::proxima(QDate::currentDate());
    m_convocatoria = new QComboBox;
    m_convocatoria->setEditable(true);
    m_convocatoria->addItems(DatosConvocatoria::ordinarias());
    m_convocatoria->setCurrentText(proxima.convocatoria);
    m_anio = new QSpinBox;
    m_anio->setRange(2000, 2100);
    m_anio->setValue(proxima.anio);
    m_instrucciones = new QPlainTextEdit(DatosConvocatoria().instrucciones);
    m_instrucciones->setMaximumHeight(52);
    m_instrucciones->setTabChangesFocus(true);
    // Qué va arriba (en la cabecera de cada examen y de la hoja de soluciones) y abajo (en el pie de cada página).
    auto casilla = [this](const QString &texto, const QString &descripcion) {
        auto *c = new QCheckBox(texto);
        c->setToolTip(descripcion);
        c->setChecked(true);
        connect(c, &QCheckBox::toggled, this, [this]() {
            if (hayExamen())
                actualizarMaqueta();
        });
        return c;
    };
    //: Casilla: el nombre de la asignatura va en la cabecera (o en el pie) de los exámenes
    m_asignaturaEnCabecera = casilla(tr("Asignatura"), tr("Pone el nombre de la asignatura arriba, en la cabecera de los "
                                                          "exámenes y de la hoja de soluciones."));
    //: Casilla: la convocatoria y el año («Junio 2026») van en la cabecera (o en el pie) de los exámenes
    m_convocatoriaEnCabecera = casilla(tr("Convocatoria y año"), tr("Pone la convocatoria y el año arriba, en la cabecera "
                                                                    "de los exámenes y de la hoja de soluciones."));
    m_asignaturaEnPie = casilla(tr("Asignatura"), tr("Pone el nombre de la asignatura abajo, en el pie de cada página "
                                                     "(en el PDF, el LaTeX y el Word)."));
    m_convocatoriaEnPie = casilla(tr("Convocatoria y año"), tr("Pone la convocatoria y el año abajo, en el pie de cada "
                                                               "página (en el PDF, el LaTeX y el Word)."));
    auto fila = [](QCheckBox *a, QCheckBox *b) {
        auto *l = new QHBoxLayout;
        l->addWidget(a);
        l->addWidget(b);
        l->addStretch();
        return l;
    };
    fConv->addRow(tr("Asignatura:"), m_asignatura);
    fConv->addRow(tr("Idioma de los exámenes:"), m_idiomaExamen);
    fConv->addRow(tr("Convocatoria:"), m_convocatoria);
    fConv->addRow(tr("Año:"), m_anio);
    fConv->addRow(tr("Instrucciones:"), m_instrucciones);
    //: Delante de las casillas de lo que va en la cabecera de los exámenes (arriba de la primera página)
    fConv->addRow(tr("Cabecera:"), fila(m_asignaturaEnCabecera, m_convocatoriaEnCabecera));
    //: Delante de las casillas de lo que va en el pie de cada página de los exámenes
    fConv->addRow(tr("Pie de página:"), fila(m_asignaturaEnPie, m_convocatoriaEnPie));
    return gConv;
}

QWidget *VentanaPrincipal::crearGrupoExamenes()
{
    auto *gExamenes = m_grupoExamenes = new QGroupBox;
    auto *le = new QVBoxLayout(gExamenes);
    m_listaExamenes = new QListWidget;
    m_listaExamenes->setToolTip(tr("Se genera un examen por cada nombre marcado, en este orden.\n"
                                   "Doble clic para cambiar el nombre; arrastre para cambiar el orden."));
    m_listaExamenes->setDragDropMode(QAbstractItemView::InternalMove);
    m_listaExamenes->setMaximumHeight(150);
    le->addWidget(m_listaExamenes);
    // Cada botón tiene su atajo, Alt y la letra subrayada (sin las de los menús), que dice su descripción emergente.
    auto *botonesExamenes = new QHBoxLayout;
    auto botonExamenes = [&](const QString &texto, const QString &descripcion, auto accion) {
        auto *b = new QPushButton(texto);
        const QString atajo = QKeySequence::mnemonic(texto).toString(QKeySequence::NativeText);
        b->setToolTip(atajo.isEmpty() ? descripcion : QStringLiteral("%1 (%2)").arg(descripcion, atajo));
        connect(b, &QPushButton::clicked, this, accion);
        botonesExamenes->addWidget(b);
    };
    auto marcarTodos = [this](bool marcar) {
        for (int i = 0; i < m_listaExamenes->count(); ++i)
            m_listaExamenes->item(i)->setCheckState(marcar ? Qt::Checked : Qt::Unchecked);
    };
    //: Botón: añade un examen a la lista de exámenes que se generan. El & marca la letra del atajo (Alt+letra):
    //: no puede ser la de un menú ni la de otro de estos cuatro botones.
    botonExamenes(tr("Aña&dir"), tr("Añade un examen a la lista"), &VentanaPrincipal::nuevoExamen);
    //: Botón: quita de la lista el examen seleccionado. El & marca la letra del atajo (Alt+letra).
    botonExamenes(tr("&Quitar"), tr("Quita de la lista el examen seleccionado"), &VentanaPrincipal::quitarExamen);
    //: Botón: marca todos los exámenes de la lista. El & marca la letra del atajo (Alt+letra).
    botonExamenes(tr("&Todos"), tr("Marca todos los exámenes de la lista"), [marcarTodos]() { marcarTodos(true); });
    //: Botón: desmarca todos los exámenes de la lista. El & marca la letra del atajo (Alt+letra).
    botonExamenes(tr("&Ninguno"), tr("Desmarca todos los exámenes de la lista"), [marcarTodos]() { marcarTodos(false); });
    le->addLayout(botonesExamenes);
    // El título dice cuántos hay marcados: se pone al día al marcar o desmarcar, añadir, quitar o cargar la lista.
    const QAbstractItemModel *modelo = m_listaExamenes->model();
    connect(modelo, &QAbstractItemModel::dataChanged, this, &VentanaPrincipal::actualizarCuentaExamenes);
    connect(modelo, &QAbstractItemModel::rowsInserted, this, &VentanaPrincipal::actualizarCuentaExamenes);
    connect(modelo, &QAbstractItemModel::rowsRemoved, this, &VentanaPrincipal::actualizarCuentaExamenes);
    connect(modelo, &QAbstractItemModel::modelReset, this, &VentanaPrincipal::actualizarCuentaExamenes);
    actualizarCuentaExamenes();
    return gExamenes;
}

void VentanaPrincipal::actualizarCuentaExamenes()
{
    const int total = m_listaExamenes->count();
    int marcados = 0;
    for (int i = 0; i < total; ++i)
        marcados += m_listaExamenes->item(i)->checkState() == Qt::Checked ? 1 : 0;
    //: Título del recuadro de la lista de exámenes (modelos): cuántos están marcados, los que se generan, de cuántos hay
    m_grupoExamenes->setTitle(marcados == 1 ? tr("Exámenes: 1 modelo de %1 seleccionado.").arg(total)
                                            //: Título del recuadro de la lista de exámenes (modelos): cuántos están
                                            //: marcados, los que se generan, de cuántos hay
                                            : tr("Exámenes: %1 modelos de %2 seleccionados.").arg(marcados).arg(total));
}

QWidget *VentanaPrincipal::crearGrupoTemas()
{
    auto *gTemas = new QGroupBox(tr("Preguntas por tema"));
    auto *lt = new QVBoxLayout(gTemas);
    m_contenedorTemas = new QWidget;
    m_rejillaTemas = new QGridLayout(m_contenedorTemas);
    m_rejillaTemas->setContentsMargins(0, 0, 0, 0);
    lt->addWidget(m_contenedorTemas);
    m_rejillaRepartos = new QGridLayout;
    lt->addLayout(m_rejillaRepartos);
    auto *pie = new QHBoxLayout;
    m_total = new QLabel;
    pie->addWidget(m_total, 1);
    auto *guardar = new QPushButton(tr("Guardar reparto…"));
    guardar->setToolTip(tr("Guarda en el banco el número de preguntas de cada tema que hay ahora, con un nombre: "
                           "sale un botón que lo pone"));
    connect(guardar, &QPushButton::clicked, this, &VentanaPrincipal::guardarReparto);
    pie->addWidget(guardar);
    lt->addLayout(pie);
    return gTemas;
}

QWidget *VentanaPrincipal::crearGrupoPuntuacion()
{
    auto *gPunt = new QGroupBox(tr("Puntuación"));
    auto *lPunt = new QVBoxLayout(gPunt);
    // Las cuatro formas de puntuar. Cada botón lleva en «valor» lo que se guarda en la configuración (ver ponerAjuste()).
    m_puntuacion = new QWidget;
    auto *lFormas = new QVBoxLayout(m_puntuacion);
    lFormas->setContentsMargins(0, 0, 0, 0);
    m_formasPuntuacion = new QButtonGroup(m_puntuacion);
    const QString sobre10 = tr("El examen vale 10 puntos: cada acierto, 10 entre el número de preguntas.");
    auto forma = [&](DatosConvocatoria::Puntuacion id, const char *valor, const QString &texto, const QString &ayuda) {
        auto *b = new QRadioButton(texto);
        b->setProperty("valor", QString::fromLatin1(valor));
        b->setToolTip(ayuda);
        m_formasPuntuacion->addButton(b, id);
        lFormas->addWidget(b);
    };
    forma(DatosConvocatoria::Resta30, "resta30", tr("Cada fallo resta el 30 %"), sobre10);
    //: %1 es el porcentaje de un acierto que resta cada fallo para que contestar al azar dé 0 de media («33,3»)
    forma(DatosConvocatoria::RestaAlAzar, "restaAlAzar",
          tr("Cada fallo resta el %1 %").arg(QLocale().toString(100 * DatosConvocatoria::fraccionFalloAlAzar(), 'f', 1)),
          sobre10 + QLatin1Char('\n')
              + tr("Es lo justo para que contestar al azar dé 0 puntos de media: con cuatro opciones se acierta una vez "
                   "de cada cuatro, y ese acierto compensa los tres fallos."));
    forma(DatosConvocatoria::SinResta, "sinResta", tr("Los fallos no restan"), sobre10);
    //: Forma de puntuar: los puntos de cada acierto y de cada fallo los escribe quien hace el examen
    forma(DatosConvocatoria::Personalizada, "personalizada", tr("Personalizada"),
          tr("Los puntos de cada acierto y de cada fallo, los que se escriban abajo."));
    m_formasPuntuacion->button(DatosConvocatoria::Resta30)->setChecked(true);
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
    lPunt->addWidget(m_puntuacion);
    auto *puntos = new QHBoxLayout;
    //: Puntos que suma cada respuesta correcta
    puntos->addWidget(new QLabel(tr("Acierto:")));
    puntos->addWidget(m_acierto);
    puntos->addSpacing(12);
    //: Puntos que resta cada respuesta incorrecta
    puntos->addWidget(new QLabel(tr("Fallo:")));
    puntos->addWidget(m_fallo);
    puntos->addStretch();
    lPunt->addLayout(puntos);
    connect(m_formasPuntuacion, &QButtonGroup::idToggled, this, [this](int, bool marcado) {
        if (marcado)
            actualizarPuntuacion();
    });
    connect(m_formasPuntuacion, &QButtonGroup::idToggled, this, &VentanaPrincipal::resumirPuntuacion);
    connect(m_acierto, &QDoubleSpinBox::valueChanged, this, &VentanaPrincipal::resumirPuntuacion);
    connect(m_fallo, &QDoubleSpinBox::valueChanged, this, &VentanaPrincipal::resumirPuntuacion);
    return gPunt;
}

void VentanaPrincipal::resumirPuntuacion()
{
    if (!m_plegablePuntuacion)
        return;
    const QAbstractButton *forma = m_formasPuntuacion->checkedButton();
    QString resumen = forma ? forma->text() : QString();
    if (m_formasPuntuacion->checkedId() == DatosConvocatoria::Personalizada)
        //: Resumen de la puntuación personalizada: %1 y %2 son los puntos de cada acierto y de cada fallo
        resumen = tr("acierto +%1, fallo -%2").arg(QLocale().toString(m_acierto->value(), 'f', 2), QLocale().toString(m_fallo->value(), 'f', 2));
    m_plegablePuntuacion->ponerResumen(resumen);
}

// Lo que se aparta de lo habitual (lo que viene marcado de entrada), que es lo que conviene tener presente: sobre todo
// la semilla, con la que todas las generaciones salen iguales.
void VentanaPrincipal::resumirOpciones()
{
    if (!m_plegableOpciones)
        return;
    QStringList cambios;
    if (!m_noRepetir->isChecked())
        cambios << tr("se repiten entre exámenes");
    if (!m_evitarRecientes->isChecked())
        cambios << tr("sin evitar las recientes");
    else if (m_numRecientes->value() != 3)
        //: Resumen de las opciones: %1 es el número de convocatorias
        cambios << tr("evita las de las últimas %1").arg(m_numRecientes->value());
    if (!m_evitarOld->isChecked())
        cambios << tr("sin evitar las «old»");
    if (!m_incluirRevisar->isChecked())
        cambios << tr("sin las de revisar");
    if (m_soloVerificadas->isChecked())
        cambios << tr("solo verificadas");
    if (m_ordenAleatorio->isChecked())
        cambios << tr("orden aleatorio");
    if (m_barajarOpciones->isChecked())
        cambios << tr("opciones barajadas");
    if (!m_limitarPaginas->isChecked())
        cambios << tr("sin ajustar las páginas");
    if (!m_semilla->text().isEmpty())
        //: Resumen de las opciones: %1 es la semilla (un número)
        cambios << tr("semilla %1").arg(m_semilla->text());
    //: Resumen de las opciones cuando están todas como vienen de entrada
    m_plegableOpciones->ponerResumen(cambios.isEmpty() ? tr("las habituales") : cambios.join(QStringLiteral(", ")));
}

QWidget *VentanaPrincipal::crearGrupoOpciones()
{
    auto *gOpc = new QGroupBox(tr("Opciones"));
    auto *fOpc = new QFormLayout(gOpc);
    m_noRepetir = new QCheckBox(tr("No repetir preguntas entre exámenes"));
    //: Casilla seguida de un número y del sufijo « convocatorias»: «Evitar las preguntas de las últimas [3] convocatorias»
    m_evitarRecientes = new QCheckBox(tr("Evitar las preguntas de las últimas"));
    m_evitarRecientes->setToolTip(tr("Deja para el final las preguntas (y sus variantes) que salieron en las últimas "
                                     "convocatorias registradas, empezando por las más recientes"));
    m_numRecientes = new QSpinBox;
    m_numRecientes->setRange(1, 99);
    m_numRecientes->setValue(3);
    //: Sufijo del número: «Evitar las preguntas de las últimas [3] convocatorias» (con el espacio delante)
    m_numRecientes->setSuffix(tr(" convocatorias"));
    connect(m_evitarRecientes, &QCheckBox::toggled, m_numRecientes, &QWidget::setEnabled);
    m_evitarRecientes->setChecked(true);
    //: «old» es el nombre de la marca de las preguntas que ya salieron en exámenes anteriores al historial
    m_evitarOld = new QCheckBox(tr("Evitar también las marcadas «old»"));
    m_evitarOld->setToolTip(tr("Deja para el final (aunque antes que las de las convocatorias registradas) las "
                               "preguntas que ya salieron en exámenes anteriores al historial"));
    m_evitarOld->setChecked(true);
    auto *filaRecientes = new QHBoxLayout;
    filaRecientes->addWidget(m_evitarRecientes);
    filaRecientes->addWidget(m_numRecientes);
    filaRecientes->addStretch();
    m_noRepetir->setChecked(true);
    m_incluirRevisar = new QCheckBox(tr("Incluir preguntas marcadas para revisar"));
    m_incluirRevisar->setChecked(true);
    m_soloVerificadas = new QCheckBox(tr("Solo preguntas verificadas por humanos"));
    m_soloVerificadas->setToolTip(tr("Sin marcar, entran todas las preguntas; marcada, solo las que ha revisado alguien "
                                     "(«Verificada por humanos»). En la vista del examen, cada pregunta dice si está "
                                     "verificada."));
    m_ordenAleatorio = new QCheckBox(tr("Orden aleatorio (si no, por tema y número)"));
    m_barajarOpciones = new QCheckBox(tr("Barajar las opciones (la correcta, repartida entre a)–d))"));
    m_barajarOpciones->setToolTip(tr("En cada examen, la correcta sale en a), b), c) y d) por igual y nunca tres veces "
                                     "seguidas en la misma letra (si se puede).\nNo se barajan las preguntas con "
                                     "opciones del tipo «Todas las anteriores»."));
    m_limitarPaginas = new QCheckBox(tr("Ajustar a una página por cada 10 preguntas"));
    m_limitarPaginas->setToolTip(tr("Compacta la maqueta (opciones en dos columnas, menos separación, letra menor) hasta "
                                    "que cada examen quepa en una página por cada 10 preguntas, y avisa si no se consigue.\n"
                                    "Sin marcar, los exámenes van con la maqueta normal y ocupan las páginas que necesiten."));
    m_limitarPaginas->setChecked(true);
    connect(m_limitarPaginas, &QCheckBox::toggled, this, [this]() {
        if (hayExamen())
            actualizarMaqueta();
    });
    m_semilla = new QLineEdit;
    m_semilla->setValidator(new QIntValidator(0, 2147483647, m_semilla));
    //: Lo que pone el campo «Semilla» cuando está vacío (la semilla será aleatoria)
    m_semilla->setPlaceholderText(tr("aleatoria"));
    m_semilla->setClearButtonEnabled(true);
    m_semilla->setToolTip(tr("Con la misma semilla y la misma configuración se obtienen los mismos exámenes.\n"
                             "Vacía, cada vez salen otros."));
    // Con una semilla escrita, todas las generaciones salen iguales: que se note.
    connect(m_semilla, &QLineEdit::textChanged, this, [this](const QString &texto) {
        m_semilla->setStyleSheet(texto.isEmpty() ? QString() : QStringLiteral("background: #fff3cd; color: #664d03"));
    });
    fOpc->addRow(m_noRepetir);
    fOpc->addRow(filaRecientes);
    fOpc->addRow(m_evitarOld);
    fOpc->addRow(m_incluirRevisar);
    fOpc->addRow(m_soloVerificadas);
    fOpc->addRow(m_ordenAleatorio);
    fOpc->addRow(m_barajarOpciones);
    fOpc->addRow(m_limitarPaginas);
    //: Semilla del generador de números aleatorios
    fOpc->addRow(tr("Semilla:"), m_semilla);
    for (QCheckBox *c : {m_noRepetir, m_evitarRecientes, m_evitarOld, m_incluirRevisar, m_soloVerificadas, m_ordenAleatorio,
                         m_barajarOpciones, m_limitarPaginas})
        connect(c, &QCheckBox::toggled, this, &VentanaPrincipal::resumirOpciones);
    connect(m_numRecientes, &QSpinBox::valueChanged, this, &VentanaPrincipal::resumirOpciones);
    connect(m_semilla, &QLineEdit::textChanged, this, &VentanaPrincipal::resumirOpciones);
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
    m_verSoluciones = new QCheckBox(tr("Mostrar soluciones"));
    //: Versión del examen en letra grande, adaptada para estudiantes con discapacidad visual
    m_letraGrande = new QCheckBox(tr("Letra grande"));
    m_letraGrande->setToolTip(tr("Versión adaptada para estudiantes con discapacidad visual: letra de 16 pt, una "
                                 "opción por línea (enunciados sin cursiva), más separación e imágenes más grandes, sin límite de páginas.\n"
                                 "Vale para la vista, la impresión y las exportaciones; los ficheros llevan "
                                 "«-letra-grande» en el nombre."));
    m_infoGeneracion = new QLabel;
    m_infoGeneracion->setStyleSheet(QStringLiteral("color: gray"));
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
    m_avisos->setStyleSheet(QLatin1String(kEstiloAviso));
    m_avisos->hide();
    lr->addWidget(m_avisos);
    m_visorExamen = new VisorAmpliable;
    m_visorExamen->setObjectName(QStringLiteral("visorExamen"));
    m_visorExamen->setHtml(tr("<p style=\"color:gray\">Elija cuántas preguntas quiere de cada tema y pulse "
                              "<b>Generar exámenes</b>.</p>"));
    // El tamaño de la vista (Ctrl + rueda, Ctrl++, Ctrl+-, Ctrl+0) se recuerda de una vez a otra.
    m_visorExamen->ponerEscala(QSettings().value(QStringLiteral("vista/escala"), 1.0).toDouble());
    connect(m_visorExamen, &VisorAmpliable::escalaCambiada, this, [this](qreal escala) {
        QSettings().setValue(QStringLiteral("vista/escala"), escala);
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

QWidget *VentanaPrincipal::crearPestanaBanco()
{
    auto *w = new QWidget;
    auto *l = new QVBoxLayout(w);

    auto *filtros = new QHBoxLayout;
    m_filtroTema = new QComboBox;
    m_filtroEstado = new QComboBox;
    //: Filtro por estado de las preguntas (en femenino: «las preguntas»): todas
    m_filtroEstado->addItem(tr("Todas"), FiltroPreguntas::Todas);
    //: Filtro por estado de las preguntas: las activas
    m_filtroEstado->addItem(tr("Activas"), FiltroPreguntas::Activas);
    //: Filtro por estado de las preguntas: las anuladas
    m_filtroEstado->addItem(tr("Anuladas"), FiltroPreguntas::Anuladas);
    //: Filtro por estado de las preguntas: las obsoletas
    m_filtroEstado->addItem(tr("Obsoletas"), FiltroPreguntas::Obsoletas);
    //: Filtro por estado de las preguntas: las marcadas para revisar
    m_filtroEstado->addItem(tr("Para revisar"), FiltroPreguntas::ParaRevisar);
    //: Filtro por estado de las preguntas: las que tienen imagen
    m_filtroEstado->addItem(tr("Con imagen"), FiltroPreguntas::ConImagen);
    //: Filtro por estado de las preguntas: las revisadas por personas
    m_filtroEstado->addItem(tr("Verificadas"), FiltroPreguntas::Verificadas);
    //: Filtro por estado de las preguntas: las que aún no ha revisado nadie
    m_filtroEstado->addItem(tr("Sin verificar"), FiltroPreguntas::SinVerificar);
    m_filtroTexto = new QLineEdit;
    m_filtroTexto->setPlaceholderText(tr("Buscar texto o código (T1-22b)…"));
    m_filtroTexto->setClearButtonEnabled(true);
    m_cuenta = new QLabel;
    m_cuenta->setObjectName(QStringLiteral("cuentaBanco"));
    filtros->addWidget(new QLabel(tr("Tema:")));
    filtros->addWidget(m_filtroTema, 1);
    //: Estado de las preguntas (activa, anulada, obsoleta…), para filtrar la tabla
    filtros->addWidget(new QLabel(tr("Estado:")));
    filtros->addWidget(m_filtroEstado);
    filtros->addWidget(m_filtroTexto, 2);
    filtros->addWidget(m_cuenta);
    // La proporción de verificadas, de un vistazo: con el filtro de tema, el avance de ese tema.
    m_barraVerificadas = new QProgressBar;
    m_barraVerificadas->setObjectName(QStringLiteral("barraBanco"));
    m_barraVerificadas->setTextVisible(false);
    m_barraVerificadas->setFixedWidth(120);
    m_barraVerificadas->setMaximumHeight(12);
    filtros->addWidget(m_barraVerificadas);
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
    m_tabla->setColumnWidth(ModeloPreguntas::ColVerificada, 130);
    m_tabla->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tabla, &QWidget::customContextMenuRequested, this, &VentanaPrincipal::menuTabla);
    m_tabla->setColumnWidth(ModeloPreguntas::ColCorrecta, 50);
    m_ficha = new VisorAmpliable;
    m_ficha->setObjectName(QStringLiteral("fichaBanco"));
    m_ficha->quitarAtajos();

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
    //: Botón: nueva pregunta
    boton(tr("Nueva…"), &VentanaPrincipal::nuevaPregunta);
    //: Botón: editar la pregunta seleccionada
    boton(tr("Editar…"), &VentanaPrincipal::editarPregunta);
    //: Botón: recorre las preguntas de la lista, desde la seleccionada, para marcarlas como verificadas por personas
    boton(tr("Verificar…"), &VentanaPrincipal::verificarPregunta);
    boton(tr("Duplicar como variante…"), &VentanaPrincipal::duplicarPregunta);
    //: Botón: eliminar la pregunta seleccionada
    boton(tr("Eliminar"), &VentanaPrincipal::eliminarPregunta);
    botones->addStretch();
    l->addLayout(botones);

    // Los filtros de tema y de estado se recuerdan de una vez a otra, para seguir donde se dejó («Tema 4, sin
    // verificar»): los que elige el usuario, no los que pone el programa. El texto que se busca no se recuerda.
    m_filtroEstado->setCurrentIndex(qMax(0, m_filtroEstado->findData(QSettings().value(QStringLiteral("banco/filtroEstado"), 0))));
    connect(m_filtroTema, &QComboBox::activated, this,
            [this]() { QSettings().setValue(QStringLiteral("banco/filtroTema"), m_filtroTema->currentData()); });
    connect(m_filtroEstado, &QComboBox::activated, this,
            [this]() { QSettings().setValue(QStringLiteral("banco/filtroEstado"), m_filtroEstado->currentData()); });
    connect(m_filtroTema, &QComboBox::currentIndexChanged, this, &VentanaPrincipal::filtrar);
    connect(m_filtroEstado, &QComboBox::currentIndexChanged, this, &VentanaPrincipal::filtrar);
    connect(m_filtroTexto, &QLineEdit::textChanged, this, &VentanaPrincipal::filtrar);
    connect(m_tabla->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &VentanaPrincipal::seleccionCambiada);
    connect(m_tabla, &QTableView::doubleClicked, this, &VentanaPrincipal::editarPregunta);

    // La lista y la ficha se amplían juntas. El ancho de las columnas y el alto de las filas no cambian solos con la
    // letra: se ajustan aquí.
    m_zonaBanco = crearZona(w, QStringLiteral("vista/escalaBanco"));
    m_zonaBanco->anadirVisor(m_ficha);
    auto ajustarColumnas = [this, anterior = 1.0, alto = m_tabla->verticalHeader()->defaultSectionSize()](qreal escala) mutable {
        m_tabla->verticalHeader()->setDefaultSectionSize(qRound(alto * escala));
        for (int c = 0; c < m_tabla->model()->columnCount(); ++c)
            if (m_tabla->horizontalHeader()->sectionResizeMode(c) == QHeaderView::Interactive)
                m_tabla->setColumnWidth(c, qRound(m_tabla->columnWidth(c) * escala / anterior));
        anterior = escala;
    };
    ajustarColumnas(m_zonaBanco->escala());
    connect(m_zonaBanco, &ZonaAmpliable::escalaCambiada, this, ajustarColumnas);
    return w;
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
    archivo->addAction(tr("&Salir"), QKeySequence::Quit, this, &QWidget::close);

    QMenu *editar = menuBar()->addMenu(tr("&Editar"));
    editar->addAction(tr("&Temas…"), this, &VentanaPrincipal::gestionarTemas);
    editar->addAction(tr("&Nueva pregunta…"), QKeySequence::New, this, &VentanaPrincipal::nuevaPregunta);
    editar->addAction(tr("&Revisar el estilo del banco…"), this, &VentanaPrincipal::revisarEstilo);
    //: Menú: la lista de personas que verifican las preguntas del banco
    editar->addAction(tr("&Verificadores…"), this, &VentanaPrincipal::gestionarVerificadores);
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
    ayuda->addAction(tr("&Acerca de…"), this, &VentanaPrincipal::acercaDe);
}

// ---------------------------------------------------------------------------------------------
// Base de datos

bool VentanaPrincipal::abrirBaseDatos(const QString &ruta)
{
    // Las carpetas de la configuración son las del último banco abierto: las toma el que no tenga las suyas solo si
    // es ese (el primero que se abre tras pasar a esta versión, que guarda las opciones en el banco).
    const QString ultimo = QSettings().value(QStringLiteral("bd/ruta")).toString();
    const bool carpetasPropias = ultimo.isEmpty() || QFileInfo(ruta).absoluteFilePath() != QFileInfo(ultimo).absoluteFilePath();
    if (m_bd.abierta()) {
        guardarEstadoBanco();
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
    m_asignatura->setText(m_bd.asignatura());
    {
        const QString antes = idiomaExamen();
        {
            const QSignalBlocker bloqueo(m_idiomaExamen);
            m_idiomaExamen->setCurrentIndex(qMax(0, m_idiomaExamen->findData(m_bd.idioma())));
        }
        adaptarAlIdiomaExamen(antes);
    }
    actualizarTitulo();
    aplicarOpcionesDelBanco(m_bd.opciones(), carpetasPropias);
    // El último reparto usado con el banco. Si no hay ninguno, el que guardaban en la configuración las versiones
    // anteriores (uno para todos los bancos: se pasa al primero que se abre) o, si no, el primero de los del banco.
    m_cantidadesGuardadas = m_bd.ultimoReparto();
    if (m_cantidadesGuardadas.isEmpty()) {
        QSettings s;
        const QVariantMap antiguo = s.value(QStringLiteral("examen/temas")).toMap();
        s.remove(QStringLiteral("examen/temas"));
        for (auto it = antiguo.cbegin(); it != antiguo.cend(); ++it)
            m_cantidadesGuardadas.insert(it.key().toInt(), it.value().toInt());
        const QVector<Reparto> repartos = m_bd.repartos();
        if (m_cantidadesGuardadas.isEmpty() && !repartos.isEmpty())
            m_cantidadesGuardadas = repartos.first().preguntas;
    }
    m_cantidades.clear();   // las casillas del banco anterior, que no deben pasar sus números a este
    recargarDatos();
    QSettings().setValue(QStringLiteral("bd/ruta"), ruta);
    // Ya, para que la configuración tenga las opciones de este banco y él las suyas aunque el programa no se cierre bien.
    guardarConfiguracion();
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
                                                      carpetaBancos(m_bd.ruta(), QSettings().value(QStringLiteral("bd/ruta")).toString()),
                                                      tr("Bases de datos SQLite (*.db *.sqlite);;Todos (*)"));
    if (!ruta.isEmpty())
        abrirBaseDatos(ruta);
}

void VentanaPrincipal::nuevaBaseDatos()
{
    if (!puedenDescartarse())
        return;
    const QString ruta = QFileDialog::getSaveFileName(this, tr("Nuevo banco de preguntas"),
                                                      carpetaBancos(m_bd.ruta(), QSettings().value(QStringLiteral("bd/ruta")).toString())
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
        m_asignatura->setText(m_bd.asignatura());
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
                        "Reemplazar todo: se borran antes todos los temas y preguntas (antes de borrarlos se guarda "
                        "una copia de seguridad de la base de datos en la carpeta «copias», junto a ella)."),
                     QMessageBox::NoButton, this);
    QPushButton *anadir = caja.addButton(tr("Añadir/actualizar"), QMessageBox::AcceptRole);
    QPushButton *reemplazar = caja.addButton(tr("Reemplazar todo"), QMessageBox::DestructiveRole);
    caja.addButton(tr("Cancelar"), QMessageBox::RejectRole);
    caja.exec();
    if (caja.clickedButton() != anadir && caja.clickedButton() != reemplazar)
        return;
    QString copia;
    const QString asignaturaAntes = m_bd.asignatura();
    const int n = m_bd.importarJson(ruta, caja.clickedButton() == reemplazar, &copia);
    if (n < 0) {
        QMessageBox::warning(this, windowTitle(), tr("Error al importar:\n%1").arg(m_bd.ultimoError()));
        return;
    }
    // El JSON del que viene todo el banco pasa a ser su copia en texto; si solo se han añadido preguntas, el que
    // hubiera enlazado sigue siéndolo.
    if (caja.clickedButton() == reemplazar || m_bd.jsonEnlazado().isEmpty())
        m_bd.enlazarJson(ruta);
    if (m_bd.asignatura() != asignaturaAntes) {   // el banco no tenía nombre y lo trae el JSON
        m_asignatura->setText(m_bd.asignatura());
        actualizarTitulo();
    }
    if (m_bd.idioma() != idiomaExamen()) {        // el banco no decía el idioma y lo trae el JSON
        const QString antes = idiomaExamen();
        {
            const QSignalBlocker bloqueo(m_idiomaExamen);
            m_idiomaExamen->setCurrentIndex(qMax(0, m_idiomaExamen->findData(m_bd.idioma())));
        }
        adaptarAlIdiomaExamen(antes);
    }
    recargarDatos();
    QString mensaje = tr("Se han importado %1 preguntas.").arg(n);
    if (!copia.isEmpty())
        mensaje += tr("\n\nAntes de borrar el banco se ha guardado una copia de seguridad en:\n%1")
                       .arg(QDir::toNativeSeparators(copia));
    QMessageBox::information(this, windowTitle(), mensaje);
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
    DialogoMoodle dialogo(m_preguntas, m_asignatura->text().trimmed(), this);
    if (dialogo.exec() != QDialog::Accepted)
        return;
    guardarConfiguracion();   // la carpeta, en el banco
    Moodle::Opciones opciones = dialogo.opciones();
    opciones.idioma = idiomaExamen();
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
                                      return editar(p, DialogoPregunta::Siguiente, tr("Guardar y siguiente con avisos"))
                                             == DialogoPregunta::Siguiente;
                              return false;
                          },
                          m_bd.ruta(), m_temas, idiomaExamen(), this);
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

    int temaFiltro = m_filtroTema->currentData().toInt();
    // La primera vez, el tema que se dejó elegido la última vez que se usó el programa (si el banco lo tiene).
    if (!m_filtroTemaRecordado && !m_temas.isEmpty()) {
        temaFiltro = QSettings().value(QStringLiteral("banco/filtroTema"), 0).toInt();
        m_filtroTemaRecordado = true;
    }
    m_filtroTema->blockSignals(true);
    m_filtroTema->clear();
    m_filtroTema->addItem(tr("Todos los temas"), 0);
    for (const Tema &t : m_temas)
        m_filtroTema->addItem(t.nombre(), t.numero);
    m_filtroTema->setCurrentIndex(qMax(0, m_filtroTema->findData(temaFiltro)));
    m_filtroTema->blockSignals(false);

    m_modelo->establecer(m_preguntas);
    filtrar();
    reconstruirTemas();
    reconstruirRepartos();

    if (seleccionarId)
        seleccionarPregunta(seleccionarId);
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

void VentanaPrincipal::seleccionarPregunta(int id)
{
    const int fila = m_modelo->filaDeId(id);
    const QModelIndex i = m_filtro->mapFromSource(m_modelo->index(fila, 0));
    if (i.isValid()) {
        m_tabla->setCurrentIndex(i);
        m_tabla->scrollTo(i);
    }
}

// Rejilla de "preguntas por tema" de la pestaña de examen.
void VentanaPrincipal::reconstruirTemas()
{
    // Lo pedido de cada tema, salvo si está en el máximo porque no había más (al pedir solo las verificadas, o sin las
    // de revisar): se queda lo que se había pedido, que vuelve cuando vuelve a haberlas.
    for (auto it = m_cantidades.cbegin(); it != m_cantidades.cend(); ++it) {
        const QSpinBox *s = it.value();
        if (!(s->value() == s->maximum() && m_cantidadesGuardadas.value(it.key()) > s->value()))
            m_cantidadesGuardadas[it.key()] = s->value();
    }
    QLayoutItem *item;
    while ((item = m_rejillaTemas->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    m_cantidades.clear();

    const QMap<int, int> disponibles = Generador::gruposDisponibles(m_preguntas, m_incluirRevisar->isChecked(),
                                                                    m_soloVerificadas->isChecked());
    int fila = 0;
    for (const Tema &t : m_temas) {
        auto *nombre = new QLabel(QStringLiteral("T%1  %2").arg(t.numero).arg(t.titulo));
        nombre->setToolTip(t.nombre() + (t.autor.isEmpty() ? QString() : QStringLiteral(" (%1)").arg(t.autor)));
        auto *spin = new QSpinBox;
        spin->setRange(0, qMax(0, disponibles.value(t.numero)));
        spin->setValue(m_cantidadesGuardadas.value(t.numero, 0));
        //: Junto al número de preguntas que se piden de un tema: «[3] de 12» (12 preguntas disponibles)
        auto *max = new QLabel(tr("de %1").arg(disponibles.value(t.numero)));
        max->setStyleSheet(QStringLiteral("color: gray"));
        max->setToolTip(tr("Preguntas distintas disponibles (las variantes a, b, c... cuentan como una)"));
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

// La configuración guarda las opciones del último banco abierto; al abrir uno, se cambian por las suyas (ver
// aplicarOpcionesDelBanco()). Un banco que aún no tiene se queda con las que haya.
void VentanaPrincipal::leerConfiguracion()
{
    QSettings s;
    restoreGeometry(s.value(QStringLiteral("ventana/geometria")).toByteArray());
    for (const Ajuste &a : ajustes())
        if (s.contains(a.clave))
            ponerAjuste(a.control, s.value(a.clave));
    if (!s.contains(kClavePuntuacion) && s.contains(kClavePuntuacionAntigua))
        ponerAjuste(m_puntuacion, puntuacionAntigua(s.value(kClavePuntuacionAntigua)));
    const QStringList examenes = s.value(QStringLiteral("examenes/nombres"), kExamenesPorDefecto).toStringList();
    const QStringList marcados = s.value(QStringLiteral("examenes/marcados"), examenes).toStringList();
    for (const QString &nombre : examenes)
        anadirExamen(nombre, marcados.contains(nombre));
    connect(m_incluirRevisar, &QCheckBox::toggled, this, &VentanaPrincipal::reconstruirTemas);
    connect(m_soloVerificadas, &QCheckBox::toggled, this, &VentanaPrincipal::reconstruirTemas);
    actualizarPuntuacion();
}

void VentanaPrincipal::guardarConfiguracion()
{
    QSettings s;
    s.setValue(QStringLiteral("ventana/geometria"), saveGeometry());
    for (const Ajuste &a : ajustes())
        s.setValue(a.clave, valorAjuste(a.control));
    QStringList marcados;
    s.setValue(QStringLiteral("examenes/nombres"), listaExamenes(&marcados));
    s.setValue(QStringLiteral("examenes/marcados"), marcados);
    s.remove(QStringLiteral("examen/asignatura"));   // de las versiones anteriores: ahora va en el banco
    s.remove(kClavePuntuacionAntigua);
    if (m_bd.abierta())
        m_bd.guardarOpciones(opcionesDelBanco());
}

QStringList VentanaPrincipal::listaExamenes(QStringList *marcados) const
{
    QStringList examenes;
    for (int i = 0; i < m_listaExamenes->count(); ++i) {
        const QListWidgetItem *item = m_listaExamenes->item(i);
        const QString nombre = item->text().trimmed();
        if (nombre.isEmpty())
            continue;
        examenes << nombre;
        if (marcados && item->checkState() == Qt::Checked)
            *marcados << nombre;
    }
    return examenes;
}

QVariantMap VentanaPrincipal::opcionesDelBanco() const
{
    QVariantMap opciones;
    for (const Ajuste &a : ajustes())
        opciones.insert(a.clave, valorAjuste(a.control));
    QStringList marcados;
    opciones.insert(QStringLiteral("examenes/nombres"), listaExamenes(&marcados));
    opciones.insert(QStringLiteral("examenes/marcados"), marcados);
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
    // En el orden de ajustes(): la forma de puntuar antes que los puntos, para que se queden los guardados.
    for (const Ajuste &a : ajustes())
        if (opciones.contains(a.clave))
            ponerAjuste(a.control, opciones.value(a.clave));
    if (!opciones.contains(kClavePuntuacion) && opciones.contains(kClavePuntuacionAntigua))
        ponerAjuste(m_puntuacion, puntuacionAntigua(opciones.value(kClavePuntuacionAntigua)));
    if (opciones.contains(QStringLiteral("examenes/nombres"))) {
        const QStringList marcados = opciones.value(QStringLiteral("examenes/marcados")).toStringList();
        m_listaExamenes->clear();
        for (const QString &nombre : opciones.value(QStringLiteral("examenes/nombres")).toStringList())
            anadirExamen(nombre, marcados.contains(nombre));
    }
    actualizarPuntuacion();
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

void VentanaPrincipal::guardarEstadoBanco()
{
    guardarAsignatura();
    QMap<int, int> reparto;
    for (auto it = m_cantidades.cbegin(); it != m_cantidades.cend(); ++it)
        reparto.insert(it.key(), it.value()->value());
    if (!m_cantidades.isEmpty())
        m_bd.guardarUltimoReparto(reparto);
}

QVector<VentanaPrincipal::Ajuste> VentanaPrincipal::ajustes() const
{
    // En este orden se leen: la forma de puntuar antes que los puntos, para que se queden los guardados.
    return {
        {QStringLiteral("examen/instrucciones"), m_instrucciones},
        {QStringLiteral("examen/asignaturaEnCabecera"), m_asignaturaEnCabecera},
        {QStringLiteral("examen/convocatoriaEnCabecera"), m_convocatoriaEnCabecera},
        {QStringLiteral("examen/asignaturaEnPie"), m_asignaturaEnPie},
        {QStringLiteral("examen/convocatoriaEnPie"), m_convocatoriaEnPie},
        {QStringLiteral("examen/noRepetir"), m_noRepetir},
        {QStringLiteral("examen/evitarRecientes"), m_evitarRecientes},
        {QStringLiteral("examen/numRecientes"), m_numRecientes},
        {QStringLiteral("examen/evitarOld"), m_evitarOld},
        {QStringLiteral("examen/incluirRevisar"), m_incluirRevisar},
        {QStringLiteral("examen/soloVerificadas"), m_soloVerificadas},
        {QStringLiteral("examen/ordenAleatorio"), m_ordenAleatorio},
        {QStringLiteral("examen/barajarOpciones"), m_barajarOpciones},
        {QStringLiteral("examen/limitarPaginas"), m_limitarPaginas},
        {kClavePuntuacion, m_puntuacion},
        {QStringLiteral("examen/acierto"), m_acierto},
        {QStringLiteral("examen/fallo"), m_fallo},
    };
}

void VentanaPrincipal::closeEvent(QCloseEvent *evento)
{
    if (!puedenDescartarse()) {
        evento->ignore();
        return;
    }
    guardarConfiguracion();
    guardarEstadoBanco();
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

void VentanaPrincipal::guardarAsignatura()
{
    if (!m_asignatura->isModified() || !m_bd.abierta())
        return;
    m_asignatura->setModified(false);
    if (m_asignatura->text().trimmed() == m_bd.asignatura())
        return;
    if (!m_bd.guardarAsignatura(m_asignatura->text())) {
        QMessageBox::warning(this, windowTitle(), tr("No se pudo guardar el nombre de la asignatura en el banco:\n%1")
                                                      .arg(m_bd.ultimoError()));
        return;
    }
    actualizarTitulo();
    bancoEditado();   // el JSON del banco lleva el nombre
}

void VentanaPrincipal::cambiarIdiomaExamen()
{
    const QString antes = m_bd.idioma();
    if (!m_bd.abierta() || idiomaExamen() == antes)
        return;
    if (!m_bd.guardarIdioma(idiomaExamen())) {
        QMessageBox::warning(this, windowTitle(), tr("No se pudo guardar el idioma de los exámenes en el banco:\n%1")
                                                      .arg(m_bd.ultimoError()));
        const QSignalBlocker bloqueo(m_idiomaExamen);
        m_idiomaExamen->setCurrentIndex(qMax(0, m_idiomaExamen->findData(antes)));
        return;
    }
    adaptarAlIdiomaExamen(antes);
    bancoEditado();   // el JSON del banco lleva el idioma
}

QString VentanaPrincipal::idiomaExamen() const
{
    return m_idiomaExamen->currentData().toString();
}

void VentanaPrincipal::adaptarAlIdiomaExamen(const QString &antes)
{
    const QString ahora = idiomaExamen();
    if (ahora == antes)
        return;
    const QString convocatoria = m_convocatoria->currentText();
    const int mes = DatosConvocatoria::mesDe(convocatoria);
    m_convocatoria->clear();
    m_convocatoria->addItems(DatosConvocatoria::ordinarias(ahora));
    m_convocatoria->setCurrentText(mes > 0 && convocatoria == DatosConvocatoria::nombreDeMes(mes, antes)
                                       ? DatosConvocatoria::nombreDeMes(mes, ahora)
                                       : convocatoria);
    if (m_instrucciones->toPlainText().trimmed() == DatosConvocatoria::instruccionesPorDefecto(antes).trimmed())
        m_instrucciones->setPlainText(DatosConvocatoria::instruccionesPorDefecto(ahora));
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

void VentanaPrincipal::reconstruirRepartos()
{
    QLayoutItem *item;
    while ((item = m_rejillaRepartos->takeAt(0)) != nullptr) {
        // Más tarde: puede ser el botón cuyo menú ha pedido el cambio.
        if (QWidget *w = item->widget()) {
            w->setParent(nullptr);
            w->deleteLater();
        }
        delete item;
    }
    int n = 0;
    auto anadir = [this, &n](QPushButton *b) {
        m_rejillaRepartos->addWidget(b, n / kRepartosPorFila, n % kRepartosPorFila);
        ++n;
    };
    for (const Reparto &r : m_bd.repartos()) {
        auto *b = new QPushButton(r.nombre);
        //: %1 es el reparto: «T1: 3, T2: 2 (5 preguntas)»
        b->setToolTip(tr("%1\nBotón derecho: sustituirlo o quitarlo").arg(describirReparto(r.preguntas)));
        connect(b, &QPushButton::clicked, this, [this, preguntas = r.preguntas]() { aplicarReparto(preguntas); });
        b->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(b, &QPushButton::customContextMenuRequested, this,
                [this, b, nombre = r.nombre](const QPoint &donde) { menuReparto(nombre, b->mapToGlobal(donde)); });
        anadir(b);
    }
    //: Botón de reparto: ninguna pregunta de ningún tema (pone todos los temas a 0)
    auto *ninguna = new QPushButton(tr("Ninguna"));
    ninguna->setToolTip(tr("Poner todos los temas a 0"));
    connect(ninguna, &QPushButton::clicked, this, [this]() { aplicarReparto({}); });
    anadir(ninguna);
}

void VentanaPrincipal::menuReparto(const QString &nombre, const QPoint &donde)
{
    QMenu menu;
    QAction *sustituir = menu.addAction(tr("Sustituir «%1» por el reparto de ahora").arg(nombre));
    sustituir->setEnabled(!repartoActual().isEmpty());
    QAction *quitar = menu.addAction(tr("Quitar «%1»").arg(nombre));
    QAction *elegida = menu.exec(donde);
    if (!elegida || (elegida == quitar && !confirmar(this, windowTitle(), tr("¿Quitar el reparto «%1»?").arg(nombre))))
        return;
    QVector<Reparto> repartos = m_bd.repartos();
    for (int i = 0; i < repartos.size(); ++i) {
        if (repartos.at(i).nombre != nombre)
            continue;
        if (elegida == quitar)
            repartos.removeAt(i);
        else
            repartos[i].preguntas = repartoActual();
        break;
    }
    if (!m_bd.guardarRepartos(repartos))
        QMessageBox::warning(this, windowTitle(), tr("No se pudo guardar el cambio:\n%1").arg(m_bd.ultimoError()));
    reconstruirRepartos();
}

QMap<int, int> VentanaPrincipal::repartoActual() const
{
    QMap<int, int> reparto;
    for (auto it = m_cantidades.cbegin(); it != m_cantidades.cend(); ++it)
        if (it.value()->value() > 0)
            reparto.insert(it.key(), it.value()->value());
    return reparto;
}

void VentanaPrincipal::aplicarReparto(const QMap<int, int> &reparto)
{
    for (auto it = m_cantidades.begin(); it != m_cantidades.end(); ++it)
        it.value()->setValue(reparto.value(it.key(), 0));
}

void VentanaPrincipal::guardarReparto()
{
    const QMap<int, int> reparto = repartoActual();
    if (reparto.isEmpty()) {
        QMessageBox::information(this, windowTitle(), tr("Indique antes cuántas preguntas quiere de cada tema."));
        return;
    }
    QString nombre = nombreDeReparto(reparto);
    if (!pedirTexto(this, tr("Guardar reparto"),
                    tr("%1\n\nNombre del reparto (el texto de su botón):").arg(describirReparto(reparto)), &nombre)
        || nombre.isEmpty())
        return;
    QVector<Reparto> repartos = m_bd.repartos();
    auto mismo = std::find_if(repartos.begin(), repartos.end(), [&nombre](const Reparto &r) { return r.nombre == nombre; });
    if (mismo == repartos.end()) {
        repartos.append({nombre, reparto});
    } else {
        if (!confirmar(this, windowTitle(), tr("Ya hay un reparto «%1»: %2.\n\n¿Sustituirlo por este?")
                                                .arg(nombre, describirReparto(mismo->preguntas))))
            return;
        mismo->preguntas = reparto;
    }
    if (!m_bd.guardarRepartos(repartos))
        QMessageBox::warning(this, windowTitle(), tr("No se pudo guardar el reparto:\n%1").arg(m_bd.ultimoError()));
    reconstruirRepartos();
}

void VentanaPrincipal::actualizarTotal()
{
    int total = 0;
    for (QSpinBox *s : std::as_const(m_cantidades))
        total += s->value();
    m_total->setText(tr("<b>Total: %1 preguntas</b>").arg(total));
    actualizarPuntuacion();
}

void VentanaPrincipal::actualizarPuntuacion()
{
    const int forma = m_formasPuntuacion->checkedId();
    const auto puntuacion = forma < 0 ? DatosConvocatoria::Personalizada : DatosConvocatoria::Puntuacion(forma);
    m_acierto->setEnabled(puntuacion == DatosConvocatoria::Personalizada);
    m_fallo->setEnabled(puntuacion == DatosConvocatoria::Personalizada);
    int total = 0;
    for (QSpinBox *s : std::as_const(m_cantidades))
        total += s->value();
    DatosConvocatoria d;
    d.puntosAcierto = m_acierto->value();
    d.puntosFallo = m_fallo->value();
    d.puntuar(puntuacion, total);
    m_acierto->setValue(d.puntosAcierto);
    m_fallo->setValue(d.puntosFallo);
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
    //: Nombre que se propone para un examen nuevo de la lista (se cambia en el momento)
    QListWidgetItem *item = anadirExamen(tr("Nuevo examen"), true);
    m_listaExamenes->setCurrentItem(item);
    m_listaExamenes->editItem(item);
}

void VentanaPrincipal::quitarExamen()
{
    QListWidgetItem *item = m_listaExamenes->currentItem();
    if (item && confirmar(this, windowTitle(), tr("¿Quitar «%1» de la lista de exámenes?").arg(item->text())))
        delete item;
}

DatosConvocatoria VentanaPrincipal::datosConvocatoria() const
{
    DatosConvocatoria d;
    d.asignatura = m_asignatura->text().trimmed();
    d.idioma = idiomaExamen();
    d.convocatoria = m_convocatoria->currentText().trimmed();
    d.anio = m_anio->value();
    d.instrucciones = m_instrucciones->toPlainText().trimmed();
    d.puntosAcierto = m_acierto->value();
    d.puntosFallo = m_fallo->value();
    d.asignaturaEnCabecera = m_asignaturaEnCabecera->isChecked();
    d.convocatoriaEnCabecera = m_convocatoriaEnCabecera->isChecked();
    d.asignaturaEnPie = m_asignaturaEnPie->isChecked();
    d.convocatoriaEnPie = m_convocatoriaEnPie->isChecked();
    // La de los exámenes que se ven, para las soluciones y las propiedades de los ficheros.
    if (hayExamen()) {
        d.semilla = m_resultado.semilla;
        d.cambiadosAMano = !m_cambios.isEmpty();
    }
    return d;
}

ConfigExamen VentanaPrincipal::configuracionExamen() const
{
    ConfigExamen c;
    c.preguntasPorTema = repartoActual();
    for (int i = 0; i < m_listaExamenes->count(); ++i)
        if (m_listaExamenes->item(i)->checkState() == Qt::Checked)
            c.nombresModelos << m_listaExamenes->item(i)->text().trimmed();
    c.noRepetirEntreModelos = m_noRepetir->isChecked();
    c.incluirParaRevisar = m_incluirRevisar->isChecked();
    c.soloVerificadas = m_soloVerificadas->isChecked();
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
        QMessageBox::information(this, windowTitle(), tr("Indique cuántas preguntas quiere de cada tema."));
        return;
    }
    QString error;
    if (config.nombresModelos.isEmpty())
        error = tr("Marque en la lista los exámenes que quiere generar.");
    else if (config.nombresModelos.contains(QString()))
        error = tr("Hay un examen marcado sin nombre.");
    else if (QSet<QString>(config.nombresModelos.cbegin(), config.nombresModelos.cend()).size() != config.nombresModelos.size())
        error = tr("Hay dos exámenes marcados con el mismo nombre.");
    if (!error.isEmpty()) {
        QMessageBox::information(this, windowTitle(), error);
        return;
    }
    if (!puedenDescartarse())
        return;
    config.usoReciente = usoReciente(config.nombresModelos);
    m_resultado = Generador::generar(m_preguntas, config);
    m_semillaFija = config.semilla != 0;
    if (m_semillaFija)
        statusBar()->showMessage(tr("Generados con la semilla fija %1: cada vez salen los mismos exámenes. "
                                    "Borre el campo «Semilla» para que salgan otros.").arg(config.semilla),
                                 10000);
    if (m_evitarRecientes->isChecked() && m_bd.examenesRegistrados().isEmpty())
        m_resultado.avisos << tr("No hay ninguna convocatoria registrada en el historial, así que no se evitan las "
                                 "preguntas de las anteriores. Si se hicieron fuera del programa, regístrelas con "
                                 "Archivo → Registrar exámenes anteriores…");
    m_configResultado = config;
    guardarConfiguracion();
    presentarResultado();
}

QHash<QString, int> VentanaPrincipal::usoReciente(const QStringList &examenes)
{
    if (!m_evitarRecientes->isChecked())
        return {};
    return m_bd.usoReciente(m_numRecientes->value(), m_anio->value(), m_convocatoria->currentText().trimmed(), examenes);
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
    } else if (!m_limitarPaginas->isChecked()) {
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
    m_avisos->setText(avisos.join(QLatin1Char('\n')));
    m_avisos->setVisible(!avisos.isEmpty());
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
    const QString base = limpiarNombreFichero(QStringLiteral("%1 %2").arg(m_anio->value()).arg(m_convocatoria->currentText().trimmed()));
    return m_letraGrande->isChecked() ? base + QStringLiteral("-letra-grande") : base;
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
    if (!m_limitarPaginas->isChecked())
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
    m_botonRegistrar->setStyleSheet(pendiente ? QStringLiteral("background: #fff3cd; color: #664d03") : QString());
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
    QStringList yaRegistrados;
    for (const ExamenRegistrado &e : m_bd.examenesRegistrados())
        if (e.datos.anio == d.anio && e.datos.convocatoria == d.convocatoria && nombres.contains(e.nombre))
            yaRegistrados << e.nombre;
    QString texto = tr("Se guardarán en el historial los exámenes de %1: %2.\n\n"
                       "Al generar, se evitarán sus preguntas, y desde Archivo → Historial de convocatorias "
                       "se podrán volver a cargar tal como están.")
                        .arg(d.convocatoriaYAnio(), nombres.join(QStringLiteral(", ")));
    if (!yaRegistrados.isEmpty())
        texto += tr("\n\nYa había registrados con el mismo nombre, y se sustituirán: %1.")
                     .arg(yaRegistrados.join(QStringLiteral(", ")));
    if (!confirmar(this, tr("Registrar convocatoria"), texto))
        return;
    if (!m_bd.registrarExamenes(m_resultado.modelos, d, m_resultado.semilla)) {
        QMessageBox::warning(this, windowTitle(), tr("No se pudo registrar la convocatoria:\n%1").arg(m_bd.ultimoError()));
        return;
    }
    statusBar()->showMessage(tr("Registrados en el historial los exámenes de %1").arg(d.convocatoriaYAnio()), 8000);
    m_registrados = true;
    actualizarRegistro();
    seleccionCambiada();   // la ficha de la pregunta dice en qué exámenes ha salido
}

void VentanaPrincipal::historialConvocatorias()
{
    DialogoHistorial dialogo(&m_bd, this);
    if (dialogo.exec() == QDialog::Accepted && puedenDescartarse())
        cargarRegistrados(dialogo.examenesElegidos());
    seleccionCambiada();   // por si se ha eliminado algún examen
}

void VentanaPrincipal::registrarAnteriores()
{
    QStringList nombres;
    for (int i = 0; i < m_listaExamenes->count(); ++i)
        nombres << m_listaExamenes->item(i)->text().trimmed();
    DialogoAnteriores dialogo(&m_bd, m_preguntas, datosConvocatoria(), nombres, this);
    const bool registrados = dialogo.exec() == QDialog::Accepted;
    guardarConfiguracion();   // la carpeta de los PDF, en el banco
    if (!registrados)
        return;
    statusBar()->showMessage(tr("Registrados en el historial los exámenes de %1").arg(dialogo.convocatoriaYAnio()), 8000);
    seleccionCambiada();   // la ficha de la pregunta dice en qué exámenes ha salido
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
    m_instrucciones->setPlainText(d.instrucciones);
    if (qAbs(m_acierto->value() - d.puntosAcierto) > 0.001 || qAbs(m_fallo->value() - d.puntosFallo) > 0.001) {
        m_formasPuntuacion->button(DatosConvocatoria::Personalizada)->setChecked(true);
        m_acierto->setValue(d.puntosAcierto);
        m_fallo->setValue(d.puntosFallo);
    }
    // Para cambiar preguntas de los exámenes cargados: las opciones de la ventana.
    m_semillaFija = false;
    m_configResultado = configuracionExamen();
    m_configResultado.nombresModelos = nombresModelos();
    m_configResultado.usoReciente = usoReciente(m_configResultado.nombresModelos);
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
    m_quitadas[modelo].insert(quitada.grupo());
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
    editar(enBanco, DialogoPregunta::Cerrar, QString(), &guardada);
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
                           editar(p, DialogoPregunta::Cerrar, QString(), &guardada);
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
    auto poner = [this](ResultadoGeneracion &r) {
        for (Modelo &m : r.modelos) {
            for (PreguntaExamen &pe : m.preguntas) {
                const Pregunta enBanco = preguntaDelBanco(pe.pregunta);
                if (enBanco.id != 0) {
                    pe.pregunta.verificada = enBanco.verificada;
                    pe.pregunta.verificadores = enBanco.verificadores;
                }
            }
        }
    };
    poner(m_resultado);
    for (CambioPregunta &cambio : m_cambios)
        poner(cambio.antes);
    if (hayExamen())
        mostrarModelo();
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
    const int duracion = ajustes.value(QStringLiteral("practica/duracion"),
                                       ajustes.value(QStringLiteral("examen/duracion"), 60)).toInt();
    DialogoPractica d(m_resultado.modelos.at(i), datosConvocatoria(), duracion, this);
    d.exec();
    ajustes.setValue(QStringLiteral("practica/duracion"), d.duracionMinutos());
}

// ---------------------------------------------------------------------------------------------
// Banco de preguntas

void VentanaPrincipal::filtrar()
{
    m_filtro->setTema(m_filtroTema->currentData().toInt());
    m_filtro->setEstado(FiltroPreguntas::FiltroEstado(m_filtroEstado->currentData().toInt()));
    m_filtro->setTexto(m_filtroTexto->text());
    ponerCuenta();
    seleccionCambiada();
}

// Cuántas preguntas hay en la lista (las que cumplen los filtros) y cuántas de ellas están verificadas.
void VentanaPrincipal::ponerCuenta()
{
    const int n = m_filtro->rowCount();
    int verificadas = 0;
    for (int f = 0; f < n; ++f)
        verificadas += m_modelo->pregunta(m_filtro->mapToSource(m_filtro->index(f, 0)).row()).verificada ? 1 : 0;
    if (n == 0)
        m_cuenta->setText(tr("0 preguntas"));
    else if (n == 1)
        m_cuenta->setText(verificadas == 1 ? tr("1 pregunta, verificada") : tr("1 pregunta, sin verificar"));
    else
        m_cuenta->setText(tr("%1 preguntas: %2 verificadas, %3 sin verificar").arg(n).arg(verificadas).arg(n - verificadas));
    m_barraVerificadas->setVisible(n > 0);
    m_barraVerificadas->setRange(0, qMax(1, n));
    m_barraVerificadas->setValue(verificadas);
    //: Al dejar el ratón sobre la barra de avance del banco de preguntas; %1 es el porcentaje (40)
    m_barraVerificadas->setToolTip(n > 0 ? tr("Verificadas: el %1 % de las preguntas de la lista").arg(verificadas * 100 / n)
                                         : QString());
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
        html.replace(QStringLiteral("</body>"), tr("<p><b>Ha salido en:</b> %1</p></body>")
                                                    .arg(Maquetador::escapar(usos.join(QStringLiteral("; ")))));
    m_ficha->ponerDocumento(Maquetador::crearDocumento(html, {p}, m_ficha));
}

DialogoPregunta::Despues VentanaPrincipal::editar(Pregunta p, DialogoPregunta::Despues boton, const QString &texto,
                                                  Pregunta *guardada)
{
    if (guardada)
        *guardada = Pregunta();
    if (m_temas.isEmpty()) {
        QMessageBox::information(this, windowTitle(), tr("Primero cree algún tema (Editar → Temas)."));
        return DialogoPregunta::Cerrar;
    }
    DialogoPregunta d(m_temas, m_preguntas, p, idiomaExamen(), this);
    d.ponerVerificacion(&m_bd);
    if (boton != DialogoPregunta::Cerrar)
        d.anadirBoton(boton, texto);
    while (d.exec() == QDialog::Accepted) {
        Pregunta nueva = d.pregunta();
        if (m_bd.guardarPregunta(nueva)) {
            m_idGuardada = nueva.id;
            if (guardada)
                *guardada = nueva;
            bancoEditado(nueva.id);
            return d.despues();
        }
        QMessageBox::warning(this, windowTitle(), tr("No se pudo guardar la pregunta:\n%1").arg(m_bd.ultimoError()));
    }
    return DialogoPregunta::Cerrar;
}

void VentanaPrincipal::nuevaPregunta()
{
    Pregunta p;
    const int temaFiltro = m_filtroTema->currentData().toInt();
    for (const Tema &t : std::as_const(m_temas))
        if (t.numero == temaFiltro || (temaFiltro == 0 && p.temaId == 0))
            p.temaId = t.id, p.temaNumero = t.numero;
    // «Guardar y nueva» abre otra en el tema de la que se acaba de guardar.
    for (;;) {
        p.numero = siguienteNumero(m_preguntas, p.temaId);
        if (editar(p, DialogoPregunta::Nueva, tr("Guardar y nueva")) != DialogoPregunta::Nueva)
            return;
        for (const Pregunta &q : std::as_const(m_preguntas))
            if (q.id == m_idGuardada)
                p.temaId = q.temaId, p.temaNumero = q.temaNumero;
    }
}

void VentanaPrincipal::editarPregunta()
{
    bool ok;
    Pregunta p = preguntaSeleccionada(&ok);
    // «Guardar y siguiente» pasa a la siguiente de la tabla, con su filtro y su orden: la de detrás de la guardada o,
    // si con el cambio ha dejado de cumplir el filtro, la que ha quedado en su sitio.
    while (ok) {
        const int antes = m_tabla->currentIndex().row();
        if (editar(p, DialogoPregunta::Siguiente, tr("Guardar y siguiente")) != DialogoPregunta::Siguiente)
            return;
        const QModelIndex guardada = m_tabla->currentIndex();
        const int fila = guardada.isValid() ? guardada.row() + 1 : antes;
        if (fila >= m_filtro->rowCount()) {
            statusBar()->showMessage(tr("Era la última pregunta de la lista."), 5000);
            return;
        }
        m_tabla->setCurrentIndex(m_filtro->index(fila, 0));
        p = preguntaSeleccionada(&ok);
    }
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
        QMessageBox::information(this, windowTitle(), tr("%1 ya tiene variantes de la a a la z.").arg(p.grupo()));
        return;
    }
    p.old = false;
    // Es otra pregunta: nadie la ha revisado aún.
    p.verificada = false;
    p.verificadores.clear();
    editar(p);
}

void VentanaPrincipal::verificarPregunta()
{
    // Las de la lista, con su filtro y su orden, desde la seleccionada.
    QVector<Pregunta> preguntas;
    for (int f = 0; f < m_filtro->rowCount(); ++f)
        preguntas << m_modelo->pregunta(m_filtro->mapToSource(m_filtro->index(f, 0)).row());
    if (preguntas.isEmpty())
        return;
    DialogoVerificar d(&m_bd, preguntas, qMax(0, m_tabla->currentIndex().row()), m_temas,
                       [this](const Pregunta &p) {
                           Pregunta guardada;
                           editar(p, DialogoPregunta::Cerrar, QString(), &guardada);
                           return guardada;
                       },
                       this);
    // Cada pregunta que se verifica se ve ya en la lista, detrás del diálogo, sin esperar a cerrarlo.
    connect(&d, &DialogoVerificar::verificacionGuardada, this, [this](int id) { verificacionCambiada(id, false); });
    d.exec();
    const int actual = preguntas.value(d.actual()).id;
    if (!d.cambiadas().isEmpty())
        verificacionCambiada(actual);
    else
        seleccionarPregunta(actual);
}

void VentanaPrincipal::verificarLista()
{
    QVector<Pregunta> lista;
    for (int f = 0; f < m_filtro->rowCount(); ++f)
        lista << m_modelo->pregunta(m_filtro->mapToSource(m_filtro->index(f, 0)).row());
    if (lista.isEmpty())
        return;
    QDialog d(this);
    d.setWindowTitle(tr("Marcar como verificadas"));
    auto *l = new QVBoxLayout(&d);
    l->addWidget(new QLabel(lista.size() == 1 ? tr("Se marcará como verificada la pregunta de la lista por:")
                                              : tr("Se marcarán como verificadas las %1 preguntas de la lista por:").arg(lista.size())));
    auto *panel = new PanelVerificacion(&m_bd, false);
    panel->marcar(m_bd.ultimosVerificadores());
    l->addWidget(panel);
    auto *nota = new QLabel(tr("A las que ya estaban verificadas se les añaden estas personas."));
    nota->setStyleSheet(QStringLiteral("color: gray"));
    l->addWidget(nota);
    auto *botones = new QDialogButtonBox;
    botones->addButton(tr("Marcar como verificadas"), QDialogButtonBox::AcceptRole)->setDefault(true);
    botones->addButton(tr("Cancelar"), QDialogButtonBox::RejectRole);
    connect(botones, &QDialogButtonBox::accepted, &d, &QDialog::accept);
    connect(botones, &QDialogButtonBox::rejected, &d, &QDialog::reject);
    DialogoAyuda::ponerAyuda(&d, botones, QStringLiteral("verificar"));
    l->addWidget(botones);
    if (d.exec() != QDialog::Accepted)
        return;
    if (panel->hayVerificadores() && panel->verificadores().isEmpty()) {
        QMessageBox::information(this, windowTitle(), tr("Marque quién las ha verificado."));
        return;
    }
    QVector<Pregunta> cambiadas;
    for (Pregunta p : std::as_const(lista)) {
        QStringList nombres = p.verificadores;
        for (const QString &n : panel->verificadores())
            if (!nombres.contains(n))
                nombres << n;
        if (p.verificada && nombres == p.verificadores)
            continue;
        p.verificada = true;
        p.verificadores = nombres;
        cambiadas << p;
    }
    if (!m_bd.guardarVerificaciones(cambiadas)) {
        QMessageBox::warning(this, windowTitle(), tr("No se pudo guardar la verificación:\n%1").arg(m_bd.ultimoError()));
        return;
    }
    panel->recordar();
    bool ok;
    verificacionCambiada(preguntaSeleccionada(&ok).id);
    statusBar()->showMessage(cambiadas.size() == 1 ? tr("1 pregunta marcada como verificada.")
                                                   : tr("%1 preguntas marcadas como verificadas.").arg(cambiadas.size()),
                             8000);
}

void VentanaPrincipal::gestionarVerificadores()
{
    DialogoVerificadores d(&m_bd, this);
    d.exec();
    if (d.hayCambios()) {
        bool ok;
        verificacionCambiada(preguntaSeleccionada(&ok).id);
    }
}

void VentanaPrincipal::menuTabla(const QPoint &donde)
{
    QMenu menu;
    const QModelIndex i = m_tabla->indexAt(donde);
    if (i.isValid()) {
        m_tabla->setCurrentIndex(i);
        menu.addAction(tr("Editar…"), this, &VentanaPrincipal::editarPregunta);
        menu.addAction(tr("Verificar…"), this, &VentanaPrincipal::verificarPregunta);
        menu.addSeparator();
    }
    const int n = m_filtro->rowCount();
    if (n > 0)
        menu.addAction(n == 1 ? tr("Marcar como verificada la pregunta de la lista…")
                              : tr("Marcar como verificadas las %1 preguntas de la lista…").arg(n),
                       this, &VentanaPrincipal::verificarLista);
    if (!menu.isEmpty())
        menu.exec(m_tabla->viewport()->mapToGlobal(donde));
}

void VentanaPrincipal::eliminarPregunta()
{
    bool ok;
    const Pregunta p = preguntaSeleccionada(&ok);
    if (!ok)
        return;
    if (!confirmar(this, windowTitle(),
                   tr("¿Eliminar definitivamente la pregunta %1?\n\n%2\n\n"
                      "(Si solo quiere que deje de salir en los exámenes, edítela y márquela como anulada.)")
                       .arg(p.codigoCompleto(), p.enunciado.left(200))))
        return;
    if (!m_bd.borrarPregunta(p.id))
        QMessageBox::warning(this, windowTitle(), m_bd.ultimoError());
    bancoEditado();
}
