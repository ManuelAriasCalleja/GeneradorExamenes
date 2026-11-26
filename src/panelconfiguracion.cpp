#include "panelconfiguracion.h"
#include "paleta.h"
#include "basedatos.h"
#include "grupoplegable.h"
#include "idioma.h"
#include "utilidades.h"

#include <QAction>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDate>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QRadioButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStyledItemDelegate>
#include <QTextBlock>
#include <QTextLayout>
#include <QVBoxLayout>
#include <QtMath>
#include <algorithm>

namespace {

// Botones de reparto por fila.
constexpr int kRepartosPorFila = 3;   // detrás de su etiqueta, «Repartos guardados:»

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
const QString kClaveExamenes = QStringLiteral("examenes/nombres");        // la lista de exámenes y los marcados
const QString kClaveExamenesMarcados = QStringLiteral("examenes/marcados");
const QString kClaveExamenesVersiones = QStringLiteral("examenes/versiones");   // nombre -> versiones (si más de una)
constexpr int kRolVersiones = Qt::UserRole + 1;   // de cada examen de la lista

// Las versiones de un examen de la lista que tiene 'n', «(A–C)»; vacío si no tiene.
QString textoVersiones(int n, bool conNumeros)
{
    return n > 1 ? QStringLiteral("(%1–%2)").arg(Generador::etiquetaVersion(0, conNumeros),
                                                 Generador::etiquetaVersion(n - 1, conNumeros))
                 : QString();
}

// Campo de texto de varias líneas tan alto como su texto, de 3 a 8 líneas (con más, se desplaza), con la letra que
// tenga, también al ampliar el panel: para las instrucciones, que con un alto fijo no se veían enteras.
class CampoAjustado : public QPlainTextEdit
{
public:
    explicit CampoAjustado(const QString &texto) : QPlainTextEdit(texto)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(this, &QPlainTextEdit::textChanged, this, &CampoAjustado::ajustarAlto);
        ajustarAlto();
    }

protected:
    // Al cambiar el ancho (las líneas se parten en otros sitios) o la letra. Después, no dentro del cambio: el alto que
    // se le pone dentro del de tamaño no le llega a su vista, que se quedaría con el de antes.
    void resizeEvent(QResizeEvent *e) override
    {
        QPlainTextEdit::resizeEvent(e);
        if (e->size().width() != e->oldSize().width())
            pedirAjuste();
    }
    void changeEvent(QEvent *e) override
    {
        QPlainTextEdit::changeEvent(e);
        if (e->type() == QEvent::FontChange)
            pedirAjuste();
    }

private:
    static constexpr int kMinLineas = 3;
    static constexpr int kMaxLineas = 8;

    // Las líneas que ocupa el texto con el ancho que tiene el campo (las largas se parten).
    int lineasDelTexto() const
    {
        const qreal ancho = qMax(qreal(20), viewport()->width() - 2 * document()->documentMargin());
        int lineas = 0;
        for (QTextBlock b = document()->begin(); b.isValid(); b = b.next()) {
            QTextLayout capa(b.text(), font());
            capa.setTextOption(document()->defaultTextOption());
            int delBloque = 0;
            capa.beginLayout();
            for (QTextLine l = capa.createLine(); l.isValid(); l = capa.createLine()) {
                l.setLineWidth(ancho);
                ++delBloque;
            }
            capa.endLayout();
            lineas += qMax(1, delBloque);
        }
        return lineas;
    }

    void pedirAjuste()
    {
        if (m_ajustePedido)
            return;
        m_ajustePedido = true;
        QMetaObject::invokeMethod(this, [this]() {
            m_ajustePedido = false;
            ajustarAlto();
        }, Qt::QueuedConnection);
    }

    bool m_ajustePedido = false;

    void ajustarAlto()
    {
        // El alto del texto como lo pone el campo (cada línea, en píxeles enteros; el margen del documento, arriba y
        // abajo, y uno más, que pide para no desplazarse), más el marco.
        const int lineas = qBound(kMinLineas, lineasDelTexto(), kMaxLineas);
        const int texto = lineas * qCeil(QFontMetricsF(font()).lineSpacing()) + qCeil(2 * document()->documentMargin()) + 1;
        const QMargins vista = viewportMargins();
        setFixedHeight(texto + 2 * frameWidth() + vista.top() + vista.bottom());
    }
};

// Cada examen de la lista: su nombre y, si tiene varias versiones, cuáles («Nacional Original (A–C)»). Solo al verlo:
// al cambiarle el nombre se edita el nombre.
class DelegadoExamenes : public QStyledItemDelegate
{
public:
    DelegadoExamenes(std::function<bool()> conNumeros, QObject *parent)
        : QStyledItemDelegate(parent), m_conNumeros(std::move(conNumeros))
    {
    }

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        const QString versiones = textoVersiones(index.data(kRolVersiones).toInt(), m_conNumeros());
        if (!versiones.isEmpty())
            option->text += QStringLiteral("  ") + versiones;
    }

private:
    std::function<bool()> m_conNumeros;
};
const QString kClaveTemasAntiguos = QStringLiteral("examen/temas");       // de versiones anteriores: va en el banco

QString puntuacionAntigua(const QVariant &automatica)
{
    return automatica.toBool() ? QStringLiteral("resta30") : QStringLiteral("personalizada");
}

} // namespace

PanelConfiguracion::PanelConfiguracion(BaseDatos *bd, QWidget *parent)
    : QWidget(parent), m_bd(bd)
{
    auto *lc = new QVBoxLayout(this);
    // Todos los recuadros se pliegan, con lo elegido en la cabecera, y cómo se dejan se recuerda. La puntuación y las
    // opciones, que se eligen una vez y casi nunca se cambian, vienen plegadas; lo que se decide cada vez (la
    // convocatoria, los exámenes y las preguntas por tema), desplegado.
    auto plegable = [lc](QWidget *grupo, const char *nombre, bool plegado) {
        auto *p = new GrupoPlegable(qobject_cast<QGroupBox *>(grupo),
                                    QStringLiteral("vista/plegado%1").arg(QLatin1String(nombre)), plegado);
        p->setObjectName(QStringLiteral("plegable%1").arg(QLatin1String(nombre)));
        lc->addWidget(p);
        return p;
    };
    m_plegableConvocatoria = plegable(crearGrupoConvocatoria(), "Convocatoria", false);
    m_plegableExamenes = plegable(crearGrupoExamenes(), "Examenes", false);
    m_plegableTemas = plegable(crearGrupoTemas(), "Temas", false);
    m_plegablePuntuacion = plegable(crearGrupoPuntuacion(), "Puntuacion", true);
    m_plegableOpciones = plegable(crearGrupoOpciones(), "Opciones", true);
    resumirConvocatoria();
    actualizarCuentaExamenes();
    actualizarTotal();
    resumirPuntuacion();
    resumirOpciones();
    lc->addStretch();
}

PanelConfiguracion::~PanelConfiguracion()
{
    // Al destruirse, la lista de exámenes se vacía y lo avisa cuando su recuadro ya se está destruyendo: sin esto, se
    // pondría el título de un recuadro a medio destruir.
    m_listaExamenes->model()->disconnect(this);
}

DatosConvocatoria PanelConfiguracion::datos() const
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
    d.versionEnCabecera = m_versionEnCabecera->isChecked();
    d.asignaturaEnPie = m_asignaturaEnPie->isChecked();
    d.enunciadoEnCursiva = m_enunciadoEnCursiva->isChecked();
    d.convocatoriaEnPie = m_convocatoriaEnPie->isChecked();
    return d;
}

QString PanelConfiguracion::semilla() const
{
    return m_semilla->text();
}

void PanelConfiguracion::quitarSemilla()
{
    m_semilla->clear();
}

bool PanelConfiguracion::evitaRecientes() const
{
    return m_evitarRecientes->isChecked();
}

int PanelConfiguracion::numRecientes() const
{
    return m_numRecientes->value();
}

bool PanelConfiguracion::limitaPaginas() const
{
    return m_limitarPaginas->isChecked();
}

void PanelConfiguracion::ponerConvocatoria(const DatosConvocatoria &d)
{
    m_asignatura->setText(d.asignatura);
    m_convocatoria->setCurrentText(d.convocatoria);
    m_anio->setValue(d.anio);
    m_instrucciones->setPlainText(d.instrucciones);
    if (qAbs(m_acierto->value() - d.puntosAcierto) > 0.001 || qAbs(m_fallo->value() - d.puntosFallo) > 0.001) {
        m_formasPuntuacion->button(DatosConvocatoria::Personalizada)->setChecked(true);
        m_acierto->setValue(d.puntosAcierto);
        m_fallo->setValue(d.puntosFallo);
    }
}

void PanelConfiguracion::ponerAsignaturaDelBanco()
{
    m_asignatura->setText(m_bd->asignatura());
}

void PanelConfiguracion::ponerIdiomaDelBanco()
{
    const QString antes = idiomaExamen();
    {
        const QSignalBlocker bloqueo(m_idiomaExamen);
        m_idiomaExamen->setCurrentIndex(qMax(0, m_idiomaExamen->findData(m_bd->idioma())));
    }
    adaptarAlIdiomaExamen(antes);
}

void PanelConfiguracion::ponerUltimoReparto()
{
    // El último reparto usado con el banco. Si no hay ninguno, el que guardaban en la configuración las versiones
    // anteriores (uno para todos los bancos: se pasa al primero que se abre) o, si no, el primero de los del banco.
    m_cantidadesGuardadas = m_bd->ultimoReparto();
    if (m_cantidadesGuardadas.isEmpty()) {
        QSettings s;
        const QVariantMap antiguo = s.value(kClaveTemasAntiguos).toMap();
        s.remove(kClaveTemasAntiguos);
        for (auto it = antiguo.cbegin(); it != antiguo.cend(); ++it)
            m_cantidadesGuardadas.insert(it.key().toInt(), it.value().toInt());
        const QVector<Reparto> repartos = m_bd->repartos();
        if (m_cantidadesGuardadas.isEmpty() && !repartos.isEmpty())
            m_cantidadesGuardadas = repartos.first().preguntas;
    }
    m_cantidades.clear();   // las casillas del banco anterior, que no deben pasar sus números a este
}

void PanelConfiguracion::ponerDatos(const QVector<Tema> &temas, const QVector<Pregunta> &preguntas)
{
    m_temas = temas;
    m_preguntas = preguntas;
    reconstruirTemas();
    reconstruirRepartos();
}

// La configuración guarda las opciones del último banco abierto; al abrir uno, se cambian por las suyas (ver
// aplicarOpciones()). Un banco que aún no tiene se queda con las que haya.
void PanelConfiguracion::leerConfiguracion()
{
    QSettings s;
    for (const Ajuste &a : ajustes())
        if (s.contains(a.clave))
            ponerAjuste(a.control, s.value(a.clave));
    if (!s.contains(kClavePuntuacion) && s.contains(kClavePuntuacionAntigua))
        ponerAjuste(m_puntuacion, puntuacionAntigua(s.value(kClavePuntuacionAntigua)));
    const QStringList examenes = s.value(kClaveExamenes, kExamenesPorDefecto).toStringList();
    const QStringList marcados = s.value(kClaveExamenesMarcados, examenes).toStringList();
    const QVariantMap versiones = s.value(kClaveExamenesVersiones).toMap();
    for (const QString &nombre : examenes)
        anadirExamen(nombre, marcados.contains(nombre), versiones.value(nombre, 1).toInt());
    connect(m_incluirRevisar, &QCheckBox::toggled, this, &PanelConfiguracion::reconstruirTemas);
    connect(m_soloVerificadas, &QCheckBox::toggled, this, &PanelConfiguracion::reconstruirTemas);
    actualizarPuntuacion();
}

void PanelConfiguracion::guardarConfiguracion()
{
    QSettings s;
    for (const Ajuste &a : ajustes())
        s.setValue(a.clave, valorAjuste(a.control));
    QStringList marcados;
    s.setValue(kClaveExamenes, examenes(&marcados));
    s.setValue(kClaveExamenesMarcados, marcados);
    s.setValue(kClaveExamenesVersiones, versionesDeLaLista());
    s.remove(QStringLiteral("examen/asignatura"));   // de las versiones anteriores: ahora va en el banco
    s.remove(kClavePuntuacionAntigua);
}

QVariantMap PanelConfiguracion::opciones() const
{
    QVariantMap opciones;
    for (const Ajuste &a : ajustes())
        opciones.insert(a.clave, valorAjuste(a.control));
    QStringList marcados;
    opciones.insert(kClaveExamenes, examenes(&marcados));
    opciones.insert(kClaveExamenesMarcados, marcados);
    opciones.insert(kClaveExamenesVersiones, versionesDeLaLista());
    return opciones;
}

void PanelConfiguracion::aplicarOpciones(const QVariantMap &opciones)
{
    // En el orden de ajustes(): la forma de puntuar antes que los puntos, para que se queden los guardados.
    for (const Ajuste &a : ajustes())
        if (opciones.contains(a.clave))
            ponerAjuste(a.control, opciones.value(a.clave));
    if (!opciones.contains(kClavePuntuacion) && opciones.contains(kClavePuntuacionAntigua))
        ponerAjuste(m_puntuacion, puntuacionAntigua(opciones.value(kClavePuntuacionAntigua)));
    if (opciones.contains(kClaveExamenes)) {
        const QStringList marcados = opciones.value(kClaveExamenesMarcados).toStringList();
        const QVariantMap versiones = opciones.value(kClaveExamenesVersiones).toMap();
        m_listaExamenes->clear();
        for (const QString &nombre : opciones.value(kClaveExamenes).toStringList())
            anadirExamen(nombre, marcados.contains(nombre), versiones.value(nombre, 1).toInt());
    }
    actualizarPuntuacion();
}

void PanelConfiguracion::guardarEstadoBanco()
{
    guardarAsignatura();
    QMap<int, int> reparto;
    for (auto it = m_cantidades.cbegin(); it != m_cantidades.cend(); ++it)
        reparto.insert(it.key(), it.value()->value());
    if (!m_cantidades.isEmpty())
        m_bd->guardarUltimoReparto(reparto);
}

QWidget *PanelConfiguracion::crearGrupoConvocatoria()
{
    //: Convocatoria de examen (Febrero, Junio, Septiembre…): datos de la cabecera de los exámenes
    auto *gConv = new QGroupBox(tr("Convocatoria"));
    auto *fConv = new QFormLayout(gConv);
    m_asignatura = new QLineEdit;
    m_asignatura->setPlaceholderText(tr("Nombre de la asignatura"));
    m_asignatura->setToolTip(tr("Va en la cabecera de los exámenes. Se guarda en el banco de preguntas: "
                                "cada banco tiene el de su asignatura."));
    connect(m_asignatura, &QLineEdit::editingFinished, this, &PanelConfiguracion::guardarAsignatura);
    m_idiomaExamen = new QComboBox;
    for (const Idioma::Disponible &d : Idioma::disponibles())
        m_idiomaExamen->addItem(d.nombre, d.codigo);
    m_idiomaExamen->setToolTip(tr("Idioma de lo que pone el programa en los exámenes (instrucciones propuestas, puntuación, "
                                  "soluciones, meses de las convocatorias) y en Moodle, y de las pautas de estilo que se "
                                  "comprueban. Se guarda en el banco de preguntas: cada banco tiene el de su asignatura."));
    connect(m_idiomaExamen, &QComboBox::currentIndexChanged, this, &PanelConfiguracion::cambiarIdiomaExamen);
    // La convocatoria y el año no se guardan en la configuración: al abrir se propone la próxima.
    const DatosConvocatoria proxima = DatosConvocatoria::proxima(QDate::currentDate());
    m_convocatoria = new QComboBox;
    m_convocatoria->setEditable(true);
    m_convocatoria->addItems(DatosConvocatoria::ordinarias());
    m_convocatoria->setCurrentText(proxima.convocatoria);
    m_anio = new QSpinBox;
    m_anio->setRange(2000, 2100);
    m_anio->setValue(proxima.anio);
    m_instrucciones = new CampoAjustado(DatosConvocatoria().instrucciones);
    m_instrucciones->setObjectName(QStringLiteral("instrucciones"));
    m_instrucciones->setTabChangesFocus(true);
    connect(m_asignatura, &QLineEdit::textChanged, this, &PanelConfiguracion::resumirConvocatoria);
    connect(m_convocatoria, &QComboBox::currentTextChanged, this, &PanelConfiguracion::resumirConvocatoria);
    connect(m_anio, &QSpinBox::valueChanged, this, &PanelConfiguracion::resumirConvocatoria);
    // Qué va arriba (en la cabecera de cada examen y de la hoja de soluciones) y abajo (en el pie de cada página).
    auto casilla = [this](const QString &texto, const QString &descripcion) {
        auto *c = new QCheckBox(texto);
        c->setToolTip(descripcion);
        c->setChecked(true);
        connect(c, &QCheckBox::toggled, this, &PanelConfiguracion::maquetaCambiada);
        return c;
    };
    //: Casilla: el nombre de la asignatura va en la cabecera (o en el pie) de los exámenes
    m_asignaturaEnCabecera = casilla(tr("Asignatura"), tr("Pone el nombre de la asignatura arriba, en la cabecera de los "
                                                          "exámenes y de la hoja de soluciones."));
    //: Casilla: la convocatoria y el año («Junio 2026») van en la cabecera (o en el pie) de los exámenes
    m_convocatoriaEnCabecera = casilla(tr("Convocatoria y año"), tr("Pone la convocatoria y el año arriba, en la cabecera "
                                                                    "de los exámenes y de la hoja de soluciones."));
    //: Casilla: la versión del examen («Versión B») va en la cabecera, en los exámenes con varias versiones
    m_versionEnCabecera = casilla(tr("Versión"), tr("En los exámenes con varias versiones, pone la versión («Versión B») "
                                                    "arriba a la derecha de la primera página."));
    m_versionEnCabecera->setChecked(false);
    m_asignaturaEnPie = casilla(tr("Asignatura"), tr("Pone el nombre de la asignatura abajo, en el pie de cada página "
                                                     "(en el PDF, el LaTeX y el Word)."));
    m_convocatoriaEnPie = casilla(tr("Convocatoria y año"), tr("Pone la convocatoria y el año abajo, en el pie de cada "
                                                               "página (en el PDF, el LaTeX y el Word)."));
    auto fila = [](std::initializer_list<QCheckBox *> casillas) {
        auto *l = new QHBoxLayout;
        for (QCheckBox *c : casillas)
            l->addWidget(c);
        l->addStretch();
        return l;
    };
    fConv->addRow(tr("Asignatura:"), m_asignatura);
    fConv->addRow(tr("Idioma de los exámenes:"), m_idiomaExamen);
    fConv->addRow(tr("Convocatoria:"), m_convocatoria);
    fConv->addRow(tr("Año:"), m_anio);
    fConv->addRow(tr("Instrucciones:"), m_instrucciones);
    //: Delante de las casillas de lo que va en la cabecera de los exámenes (arriba de la primera página)
    fConv->addRow(tr("Cabecera:"), fila({m_asignaturaEnCabecera, m_convocatoriaEnCabecera, m_versionEnCabecera}));
    //: Delante de las casillas de lo que va en el pie de cada página de los exámenes
    fConv->addRow(tr("Pie de página:"), fila({m_asignaturaEnPie, m_convocatoriaEnPie}));
    return gConv;
}

void PanelConfiguracion::resumirConvocatoria()
{
    if (!m_plegableConvocatoria)
        return;
    QStringList partes{QStringLiteral("%1 %2").arg(m_convocatoria->currentText().trimmed()).arg(m_anio->value()).trimmed()};
    if (!m_asignatura->text().trimmed().isEmpty())
        partes << m_asignatura->text().trimmed();
    m_plegableConvocatoria->ponerResumen(partes.join(QStringLiteral(" · ")));
}

QWidget *PanelConfiguracion::crearGrupoExamenes()
{
    //: Título del recuadro de la lista de exámenes (modelos) que se generan
    auto *gExamenes = new QGroupBox(tr("Exámenes"));
    auto *le = new QVBoxLayout(gExamenes);
    m_listaExamenes = new QListWidget;
    m_listaExamenes->setToolTip(tr("Se genera un examen por cada nombre marcado (o uno por cada versión), en este orden.\n"
                                   "Doble clic para cambiar el nombre; arrastre, o Subir y Bajar, para cambiar el orden."));
    m_listaExamenes->setDragDropMode(QAbstractItemView::InternalMove);
    m_listaExamenes->setItemDelegate(new DelegadoExamenes([this] { return versionesConNumeros(); }, m_listaExamenes));
    m_listaExamenes->setMaximumHeight(150);
    le->addWidget(m_listaExamenes);
    // Cada botón tiene su atajo, Alt y la letra subrayada (sin las de los menús), que dice su descripción emergente.
    auto botonExamenes = [this](QHBoxLayout *fila, const QString &texto, const QString &descripcion, auto accion) {
        auto *b = new QPushButton(texto);
        const QString atajo = QKeySequence::mnemonic(texto).toString(QKeySequence::NativeText);
        b->setToolTip(atajo.isEmpty() ? descripcion : QStringLiteral("%1 (%2)").arg(descripcion, atajo));
        connect(b, &QPushButton::clicked, this, accion);
        fila->addWidget(b);
    };
    auto marcarTodos = [this](bool marcar) {
        for (int i = 0; i < m_listaExamenes->count(); ++i)
            m_listaExamenes->item(i)->setCheckState(marcar ? Qt::Checked : Qt::Unchecked);
    };
    auto *botonesExamenes = new QHBoxLayout;
    //: Botón: añade un examen a la lista de exámenes que se generan. El & marca la letra del atajo (Alt+letra):
    //: no puede ser la de un menú ni la de otro de estos seis botones.
    botonExamenes(botonesExamenes, tr("Aña&dir"), tr("Añade un examen a la lista"), &PanelConfiguracion::nuevoExamen);
    //: Botón: quita de la lista el examen seleccionado. El & marca la letra del atajo (Alt+letra).
    botonExamenes(botonesExamenes, tr("&Quitar"), tr("Quita de la lista el examen seleccionado"), &PanelConfiguracion::quitarExamen);
    //: Botón: marca todos los exámenes de la lista. El & marca la letra del atajo (Alt+letra).
    botonExamenes(botonesExamenes, tr("&Todos"), tr("Marca todos los exámenes de la lista"), [marcarTodos]() { marcarTodos(true); });
    //: Botón: desmarca todos los exámenes de la lista. El & marca la letra del atajo (Alt+letra).
    botonExamenes(botonesExamenes, tr("&Ninguno"), tr("Desmarca todos los exámenes de la lista"), [marcarTodos]() { marcarTodos(false); });
    le->addLayout(botonesExamenes);

    // El orden de la lista, también sin ratón, y las versiones de cada examen.
    auto *fila = new QHBoxLayout;
    //: Botón: sube en la lista el examen seleccionado. El & marca la letra del atajo (Alt+letra): no puede ser la de un
    //: menú ni la de otro de estos seis botones.
    botonExamenes(fila, tr("&Subir"), tr("Sube en la lista el examen seleccionado"), [this]() { moverExamen(-1); });
    //: Botón: baja en la lista el examen seleccionado. El & marca la letra del atajo (Alt+letra).
    botonExamenes(fila, tr("&Bajar"), tr("Baja en la lista el examen seleccionado"), [this]() { moverExamen(1); });
    fila->addStretch();
    m_versiones = new QSpinBox;
    m_versiones->setRange(1, Generador::kMaxVersiones);
    m_versiones->setEnabled(false);
    m_versiones->setToolTip(tr("Versiones del examen seleccionado. Con varias, cada una es un examen más, con las mismas "
                               "preguntas en otro orden (siempre por temas: las del tema 1, luego las del 2…) y las "
                               "opciones barajadas.\n"
                               "Si ya está generado sin ellas, se le pueden añadir sin volver a generarlo: lo ofrecen "
                               "los avisos, junto al examen."));
    //: Delante del número de versiones del examen seleccionado en la lista
    auto *etiqueta = new QLabel(tr("Versiones:"));
    etiqueta->setBuddy(m_versiones);
    fila->addWidget(etiqueta);
    fila->addWidget(m_versiones);
    m_numeracion = new QWidget;
    auto *ln = new QHBoxLayout(m_numeracion);
    ln->setContentsMargins(0, 0, 0, 0);
    //: Numeración de las versiones de un examen: con letras
    auto *letras = new QRadioButton(tr("A, B, C…"));
    letras->setProperty("valor", QStringLiteral("letras"));
    letras->setChecked(true);
    //: Numeración de las versiones de un examen: con números
    auto *numeros = new QRadioButton(tr("1, 2, 3…"));
    numeros->setProperty("valor", QStringLiteral("numeros"));
    for (QRadioButton *r : {letras, numeros}) {
        r->setToolTip(tr("Cómo se llaman las versiones de los exámenes: «Nacional Original A», «… B»… o "
                         "«Nacional Original 1», «… 2»…"));
        ln->addWidget(r);
        connect(r, &QRadioButton::toggled, m_listaExamenes->viewport(), qOverload<>(&QWidget::update));
        connect(r, &QRadioButton::toggled, this, &PanelConfiguracion::actualizarCuentaExamenes);
    }
    fila->addWidget(m_numeracion);
    le->addLayout(fila);
    connect(m_listaExamenes, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *item) {
        const QSignalBlocker bloqueo(m_versiones);
        m_versiones->setEnabled(item != nullptr);
        m_versiones->setValue(item ? qMax(1, item->data(kRolVersiones).toInt()) : 1);
    });
    connect(m_versiones, &QSpinBox::valueChanged, this, [this](int n) {
        if (QListWidgetItem *item = m_listaExamenes->currentItem())
            item->setData(kRolVersiones, n);
    });
    // El título dice cuántos hay marcados: se pone al día al marcar o desmarcar, añadir, quitar o cargar la lista.
    const QAbstractItemModel *modelo = m_listaExamenes->model();
    connect(modelo, &QAbstractItemModel::dataChanged, this, &PanelConfiguracion::actualizarCuentaExamenes);
    connect(modelo, &QAbstractItemModel::rowsInserted, this, &PanelConfiguracion::actualizarCuentaExamenes);
    connect(modelo, &QAbstractItemModel::rowsRemoved, this, &PanelConfiguracion::actualizarCuentaExamenes);
    connect(modelo, &QAbstractItemModel::modelReset, this, &PanelConfiguracion::actualizarCuentaExamenes);
    connect(modelo, &QAbstractItemModel::dataChanged, this, &PanelConfiguracion::listaExamenesCambiada);
    connect(modelo, &QAbstractItemModel::rowsInserted, this, &PanelConfiguracion::listaExamenesCambiada);
    connect(modelo, &QAbstractItemModel::rowsRemoved, this, &PanelConfiguracion::listaExamenesCambiada);
    connect(modelo, &QAbstractItemModel::modelReset, this, &PanelConfiguracion::listaExamenesCambiada);
    return gExamenes;
}

void PanelConfiguracion::actualizarCuentaExamenes()
{
    if (!m_plegableExamenes)
        return;
    const int total = m_listaExamenes->count();
    int marcados = 0;
    for (int i = 0; i < total; ++i)
        marcados += m_listaExamenes->item(i)->checkState() == Qt::Checked ? 1 : 0;
    //: Título del recuadro de la lista de exámenes (modelos): cuántos están marcados, los que se generan, de cuántos hay
    QString titulo = marcados == 1 ? tr("Exámenes: 1 modelo de %1 seleccionado.").arg(total)
                                   //: Título del recuadro de la lista de exámenes (modelos): cuántos están
                                   //: marcados, los que se generan, de cuántos hay
                                   : tr("Exámenes: %1 modelos de %2 seleccionados.").arg(marcados).arg(total);
    // Con versiones, cuántos exámenes salen.
    const int examenes = int(Generador::nombresExamenes(configuracionExamenes()).size());
    if (examenes != marcados)
        //: Se añade al título del recuadro de la lista de exámenes (con el espacio delante) si alguno tiene versiones
        titulo += tr(" Con sus versiones, %1 exámenes.").arg(examenes);
    m_plegableExamenes->ponerTituloDesplegado(titulo);
    QStringList generados;
    for (int i = 0; i < m_listaExamenes->count(); ++i) {
        const QListWidgetItem *item = m_listaExamenes->item(i);
        if (item->checkState() != Qt::Checked)
            continue;
        const QString versiones = textoVersiones(item->data(kRolVersiones).toInt(), versionesConNumeros());
        generados << (versiones.isEmpty() ? item->text().trimmed() : item->text().trimmed() + QLatin1Char(' ') + versiones);
    }
    //: Resumen del recuadro plegado de la lista de exámenes, si no hay ninguno marcado
    m_plegableExamenes->ponerResumen(generados.isEmpty() ? tr("ninguno marcado") : generados.join(QStringLiteral(", ")));
}

QWidget *PanelConfiguracion::crearGrupoTemas()
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
    //: Botón: pone a 0 el número de preguntas de todos los temas
    auto *aCero = new QPushButton(tr("Poner a 0"));
    aCero->setToolTip(tr("Pone todos los temas a 0"));
    connect(aCero, &QPushButton::clicked, this, [this]() { aplicarReparto({}); });
    pie->addWidget(aCero);
    auto *guardar = new QPushButton(tr("Guardar reparto…"));
    guardar->setToolTip(tr("Guarda en el banco el número de preguntas de cada tema que hay ahora, con un nombre: "
                           "sale un botón que lo pone"));
    connect(guardar, &QPushButton::clicked, this, &PanelConfiguracion::guardarReparto);
    pie->addWidget(guardar);
    lt->addLayout(pie);
    return gTemas;
}

QWidget *PanelConfiguracion::crearGrupoPuntuacion()
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
    connect(m_formasPuntuacion, &QButtonGroup::idToggled, this, &PanelConfiguracion::resumirPuntuacion);
    connect(m_acierto, &QDoubleSpinBox::valueChanged, this, &PanelConfiguracion::resumirPuntuacion);
    connect(m_fallo, &QDoubleSpinBox::valueChanged, this, &PanelConfiguracion::resumirPuntuacion);
    return gPunt;
}

void PanelConfiguracion::resumirPuntuacion()
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

// Lo que se aparta de lo habitual (ver m_opciones), que es lo que conviene tener presente: sobre todo la semilla, con
// la que todas las generaciones salen iguales.
void PanelConfiguracion::resumirOpciones()
{
    if (!m_plegableOpciones)
        return;
    QStringList cambios;
    for (const Opcion &o : m_opciones) {
        const QString cambio = o.cambio ? o.cambio() : QString();
        if (!cambio.isEmpty())
            cambios << cambio;
    }
    //: Resumen de las opciones cuando están todas como vienen de entrada
    m_plegableOpciones->ponerResumen(cambios.isEmpty() ? tr("las habituales") : cambios.join(QStringLiteral(", ")));
}

// Cada opción se apunta en m_opciones al crearla: de ahí salen su clave en la configuración (ajustes()) y lo que dice
// de ella el resumen del recuadro plegado (resumirOpciones()).
QWidget *PanelConfiguracion::crearGrupoOpciones()
{
    auto *gOpc = new QGroupBox(tr("Opciones"));
    auto *fOpc = new QFormLayout(gOpc);
    // Una casilla: su clave, su texto, si viene marcada y lo que dice el resumen cuando no está como viene.
    auto casilla = [this](const char *clave, const QString &texto, bool habitual, const QString &cambio) {
        auto *c = new QCheckBox(texto);
        c->setChecked(habitual);
        connect(c, &QCheckBox::toggled, this, &PanelConfiguracion::resumirOpciones);
        m_opciones.append({QString::fromLatin1(clave), c, [c, habitual, cambio] { return c->isChecked() == habitual ? QString() : cambio; }});
        return c;
    };
    m_noRepetir = casilla("examen/noRepetir", tr("No repetir preguntas entre exámenes"), true, tr("se repiten entre exámenes"));
    // Las convocatorias recientes: la casilla y, a su lado, cuántas. El resumen las cuenta juntas.
    constexpr int kRecientesHabituales = 3;
    //: Casilla seguida de un número y del sufijo « convocatorias»: «Evitar las preguntas de las últimas [3] convocatorias»
    m_evitarRecientes = new QCheckBox(tr("Evitar las preguntas de las últimas"));
    m_evitarRecientes->setToolTip(tr("Deja para el final las preguntas (y sus variantes) que salieron en las últimas "
                                     "convocatorias registradas, empezando por las más recientes"));
    m_numRecientes = new QSpinBox;
    m_numRecientes->setRange(1, 99);
    m_numRecientes->setValue(kRecientesHabituales);
    //: Sufijo del número: «Evitar las preguntas de las últimas [3] convocatorias» (con el espacio delante)
    m_numRecientes->setSuffix(tr(" convocatorias"));
    connect(m_evitarRecientes, &QCheckBox::toggled, m_numRecientes, &QWidget::setEnabled);
    m_evitarRecientes->setChecked(true);
    connect(m_evitarRecientes, &QCheckBox::toggled, this, &PanelConfiguracion::resumirOpciones);
    connect(m_numRecientes, &QSpinBox::valueChanged, this, &PanelConfiguracion::resumirOpciones);
    m_opciones.append({QStringLiteral("examen/evitarRecientes"), m_evitarRecientes, [this]() {
        if (!m_evitarRecientes->isChecked())
            return tr("sin evitar las recientes");
        if (m_numRecientes->value() != kRecientesHabituales)
            //: Resumen de las opciones: %1 es el número de convocatorias
            return tr("evita las de las últimas %1").arg(m_numRecientes->value());
        return QString();
    }});
    m_opciones.append({QStringLiteral("examen/numRecientes"), m_numRecientes, nullptr});
    auto *filaRecientes = new QHBoxLayout;
    filaRecientes->addWidget(m_evitarRecientes);
    filaRecientes->addWidget(m_numRecientes);
    filaRecientes->addStretch();
    //: «old» es el nombre de la marca de las preguntas que ya salieron en exámenes anteriores al historial
    m_evitarOld = casilla("examen/evitarOld", tr("Evitar también las marcadas «old»"), true, tr("sin evitar las «old»"));
    m_evitarOld->setToolTip(tr("Deja para el final (aunque antes que las de las convocatorias registradas) las "
                               "preguntas que ya salieron en exámenes anteriores al historial"));
    m_incluirRevisar = casilla("examen/incluirRevisar", tr("Incluir preguntas marcadas para revisar"), true, tr("sin las de revisar"));
    m_soloVerificadas = casilla("examen/soloVerificadas", tr("Solo preguntas verificadas por humanos"), false, tr("solo verificadas"));
    m_soloVerificadas->setToolTip(tr("Sin marcar, entran todas las preguntas; marcada, solo las que ha revisado alguien "
                                     "(«Verificada por humanos»). En la vista del examen, cada pregunta dice si está "
                                     "verificada."));
    m_ordenAleatorio = casilla("examen/ordenAleatorio", tr("Orden aleatorio (si no, por tema y número)"), false, tr("orden aleatorio"));
    m_barajarOpciones = casilla("examen/barajarOpciones", tr("Barajar las opciones (la correcta, repartida entre a)–d))"), false,
                                tr("opciones barajadas"));
    m_barajarOpciones->setToolTip(tr("En cada examen, la correcta sale en a), b), c) y d) por igual y nunca tres veces "
                                     "seguidas en la misma letra (si se puede).\nNo se barajan las preguntas con "
                                     "opciones del tipo «Todas las anteriores»."));
    m_limitarPaginas = casilla("examen/limitarPaginas", tr("Ajustar a una página por cada 10 preguntas"), true,
                               tr("sin ajustar las páginas"));
    m_limitarPaginas->setToolTip(tr("Compacta la maqueta (opciones en dos columnas, menos separación, letra menor) hasta "
                                    "que cada examen quepa en una página por cada 10 preguntas, y avisa si no se consigue.\n"
                                    "Sin marcar, los exámenes van con la maqueta normal y ocupan las páginas que necesiten."));
    connect(m_limitarPaginas, &QCheckBox::toggled, this, &PanelConfiguracion::maquetaCambiada);
    m_enunciadoEnCursiva = casilla("examen/enunciadoEnCursiva", tr("Enunciados en cursiva"), true, tr("enunciados sin cursiva"));
    m_enunciadoEnCursiva->setToolTip(tr("Marcada, los enunciados van en negrita cursiva, como siempre. Sin marcar, solo en "
                                        "negrita, que se lee algo mejor en los enunciados largos.\n"
                                        "Con «Letra grande» nunca van en cursiva."));
    connect(m_enunciadoEnCursiva, &QCheckBox::toggled, this, &PanelConfiguracion::maquetaCambiada);
    m_semilla = new QLineEdit;
    m_semilla->setValidator(new QIntValidator(0, 2147483647, m_semilla));
    //: Lo que pone el campo «Semilla» cuando está vacío (la semilla será aleatoria)
    m_semilla->setPlaceholderText(tr("aleatoria"));
    m_semilla->setClearButtonEnabled(true);
    m_semilla->setToolTip(tr("Con la misma semilla y la misma configuración se obtienen los mismos exámenes.\n"
                             "Vacía, cada vez salen otros."));
    // Con una semilla escrita, todas las generaciones salen iguales. Lo dicen el resumen del recuadro plegado y, al
    // generar, el aviso junto al examen; el campo no lleva color, para que el de atención no esté en tres sitios a la vez.
    connect(m_semilla, &QLineEdit::textChanged, this, &PanelConfiguracion::resumirOpciones);
    connect(m_semilla, &QLineEdit::textChanged, this, &PanelConfiguracion::semillaCambiada);
    // La semilla no se guarda (sin clave): es de cada generación.
    m_opciones.append({QString(), m_semilla, [this]() {
        //: Resumen de las opciones: %1 es la semilla (un número)
        return m_semilla->text().isEmpty() ? QString() : tr("semilla %1").arg(m_semilla->text());
    }});
    fOpc->addRow(m_noRepetir);
    fOpc->addRow(filaRecientes);
    fOpc->addRow(m_evitarOld);
    fOpc->addRow(m_incluirRevisar);
    fOpc->addRow(m_soloVerificadas);
    fOpc->addRow(m_ordenAleatorio);
    fOpc->addRow(m_barajarOpciones);
    fOpc->addRow(m_limitarPaginas);
    fOpc->addRow(m_enunciadoEnCursiva);
    //: Semilla del generador de números aleatorios
    fOpc->addRow(tr("Semilla:"), m_semilla);
    return gOpc;
}

// Rejilla de "preguntas por tema" de la pestaña de examen.
void PanelConfiguracion::reconstruirTemas()
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
        max->setStyleSheet(Paleta::css(Paleta::Secundario));
        max->setToolTip(tr("Preguntas distintas disponibles (las variantes a, b, c... cuentan como una)"));
        m_rejillaTemas->addWidget(nombre, fila, 0);
        m_rejillaTemas->addWidget(spin, fila, 1);
        m_rejillaTemas->addWidget(max, fila, 2);
        connect(spin, &QSpinBox::valueChanged, this, &PanelConfiguracion::actualizarTotal);
        m_cantidades.insert(t.numero, spin);
        ++fila;
    }
    m_rejillaTemas->setColumnStretch(0, 1);
    actualizarTotal();
}

QStringList PanelConfiguracion::examenes(QStringList *marcados) const
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

QVector<PanelConfiguracion::Ajuste> PanelConfiguracion::ajustes() const
{
    // En este orden se leen: la forma de puntuar antes que los puntos, para que se queden los guardados.
    QVector<Ajuste> ajustes{
        {QStringLiteral("examen/instrucciones"), m_instrucciones},
        {QStringLiteral("examen/asignaturaEnCabecera"), m_asignaturaEnCabecera},
        {QStringLiteral("examen/convocatoriaEnCabecera"), m_convocatoriaEnCabecera},
        {QStringLiteral("examen/asignaturaEnPie"), m_asignaturaEnPie},
        {QStringLiteral("examen/convocatoriaEnPie"), m_convocatoriaEnPie},
        {QStringLiteral("examen/versionEnCabecera"), m_versionEnCabecera},
        {QStringLiteral("examenes/numeracion"), m_numeracion},
    };
    for (const Opcion &o : m_opciones)
        if (!o.clave.isEmpty())
            ajustes.append({o.clave, o.control});
    ajustes.append({kClavePuntuacion, m_puntuacion});
    ajustes.append({QStringLiteral("examen/acierto"), m_acierto});
    ajustes.append({QStringLiteral("examen/fallo"), m_fallo});
    return ajustes;
}

void PanelConfiguracion::guardarAsignatura()
{
    if (!m_asignatura->isModified() || !m_bd->abierta())
        return;
    m_asignatura->setModified(false);
    if (m_asignatura->text().trimmed() == m_bd->asignatura())
        return;
    if (!m_bd->guardarAsignatura(m_asignatura->text())) {
        QMessageBox::warning(this, window()->windowTitle(), tr("No se pudo guardar el nombre de la asignatura en el banco:\n%1")
                                                      .arg(m_bd->ultimoError()));
        return;
    }
    emit asignaturaGuardada();
    emit bancoEditado();   // el JSON del banco lleva el nombre
}

void PanelConfiguracion::cambiarIdiomaExamen()
{
    const QString antes = m_bd->idioma();
    if (!m_bd->abierta() || idiomaExamen() == antes)
        return;
    if (!m_bd->guardarIdioma(idiomaExamen())) {
        QMessageBox::warning(this, window()->windowTitle(), tr("No se pudo guardar el idioma de los exámenes en el banco:\n%1")
                                                      .arg(m_bd->ultimoError()));
        const QSignalBlocker bloqueo(m_idiomaExamen);
        m_idiomaExamen->setCurrentIndex(qMax(0, m_idiomaExamen->findData(antes)));
        return;
    }
    adaptarAlIdiomaExamen(antes);
    emit bancoEditado();   // el JSON del banco lleva el idioma
}

QString PanelConfiguracion::idiomaExamen() const
{
    return m_idiomaExamen->currentData().toString();
}

void PanelConfiguracion::adaptarAlIdiomaExamen(const QString &antes)
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

void PanelConfiguracion::reconstruirRepartos()
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
    m_botonesReparto.clear();
    const QVector<Reparto> repartos = m_bd->repartos();
    if (repartos.isEmpty())
        return;
    // Delante, qué son: sin ella, un botón «20 preguntas» no dice que pone las preguntas de cada tema.
    auto *etiqueta = new QLabel(tr("Repartos guardados:"));
    etiqueta->setToolTip(tr("Repartos guardados en el banco: cada botón pone en las casillas de los temas su número de "
                            "preguntas, y el del reparto que hay puesto se ve pulsado.\n"
                            "Se crean con «Guardar reparto…»; con el botón derecho sobre uno, se sustituye o se quita."));
    m_rejillaRepartos->addWidget(etiqueta, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
    int n = 0;
    for (const Reparto &r : repartos) {
        auto *b = new QPushButton(r.nombre);
        b->setCheckable(true);
        //: %1 es el reparto: «T1: 3, T2: 2 (5 preguntas)»
        b->setToolTip(tr("Pone en las casillas de los temas: %1\nBotón derecho: sustituirlo o quitarlo")
                          .arg(describirReparto(r.preguntas)));
        connect(b, &QPushButton::clicked, this, [this, preguntas = r.preguntas]() {
            aplicarReparto(preguntas);
            marcarRepartoPuesto();   // si ya estaba puesto, las casillas no cambian, pero el botón se ha soltado
        });
        b->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(b, &QPushButton::customContextMenuRequested, this,
                [this, b, nombre = r.nombre](const QPoint &donde) { menuReparto(nombre, b->mapToGlobal(donde)); });
        m_rejillaRepartos->addWidget(b, n / kRepartosPorFila, 1 + n % kRepartosPorFila);
        m_botonesReparto.append({b, r.preguntas});
        ++n;
    }
    marcarRepartoPuesto();
}

void PanelConfiguracion::marcarRepartoPuesto()
{
    const QMap<int, int> puesto = repartoActual();
    for (const auto &[boton, preguntas] : std::as_const(m_botonesReparto)) {
        QMap<int, int> sinCeros = preguntas;
        sinCeros.removeIf([](auto it) { return it.value() <= 0; });
        boton->setChecked(!puesto.isEmpty() && sinCeros == puesto);
    }
}

void PanelConfiguracion::menuReparto(const QString &nombre, const QPoint &donde)
{
    QMenu menu;
    QAction *sustituir = menu.addAction(tr("Sustituir «%1» por el reparto de ahora").arg(nombre));
    sustituir->setEnabled(!repartoActual().isEmpty());
    QAction *quitar = menu.addAction(tr("Quitar «%1»").arg(nombre));
    QAction *elegida = menu.exec(donde);
    if (!elegida || (elegida == quitar && !confirmar(this, window()->windowTitle(), tr("¿Quitar el reparto «%1»?").arg(nombre))))
        return;
    QVector<Reparto> repartos = m_bd->repartos();
    for (int i = 0; i < repartos.size(); ++i) {
        if (repartos.at(i).nombre != nombre)
            continue;
        if (elegida == quitar)
            repartos.removeAt(i);
        else
            repartos[i].preguntas = repartoActual();
        break;
    }
    if (!m_bd->guardarRepartos(repartos))
        QMessageBox::warning(this, window()->windowTitle(), tr("No se pudo guardar el cambio:\n%1").arg(m_bd->ultimoError()));
    reconstruirRepartos();
}

QMap<int, int> PanelConfiguracion::repartoActual() const
{
    QMap<int, int> reparto;
    for (auto it = m_cantidades.cbegin(); it != m_cantidades.cend(); ++it)
        if (it.value()->value() > 0)
            reparto.insert(it.key(), it.value()->value());
    return reparto;
}

void PanelConfiguracion::aplicarReparto(const QMap<int, int> &reparto)
{
    for (auto it = m_cantidades.begin(); it != m_cantidades.end(); ++it)
        it.value()->setValue(reparto.value(it.key(), 0));
}

void PanelConfiguracion::guardarReparto()
{
    const QMap<int, int> reparto = repartoActual();
    if (reparto.isEmpty()) {
        QMessageBox::information(this, window()->windowTitle(), tr("Indique antes cuántas preguntas quiere de cada tema."));
        return;
    }
    QString nombre = nombreDeReparto(reparto);
    if (!pedirTexto(this, tr("Guardar reparto"),
                    tr("%1\n\nNombre del reparto (el texto de su botón):").arg(describirReparto(reparto)), &nombre)
        || nombre.isEmpty())
        return;
    QVector<Reparto> repartos = m_bd->repartos();
    auto mismo = std::find_if(repartos.begin(), repartos.end(), [&nombre](const Reparto &r) { return r.nombre == nombre; });
    if (mismo == repartos.end()) {
        repartos.append({nombre, reparto});
    } else {
        if (!confirmar(this, window()->windowTitle(), tr("Ya hay un reparto «%1»: %2.\n\n¿Sustituirlo por este?")
                                                .arg(nombre, describirReparto(mismo->preguntas))))
            return;
        mismo->preguntas = reparto;
    }
    if (!m_bd->guardarRepartos(repartos))
        QMessageBox::warning(this, window()->windowTitle(), tr("No se pudo guardar el reparto:\n%1").arg(m_bd->ultimoError()));
    reconstruirRepartos();
}

void PanelConfiguracion::actualizarTotal()
{
    int total = 0;
    QStringList temas;
    for (auto it = m_cantidades.cbegin(); it != m_cantidades.cend(); ++it) {
        total += it.value()->value();
        if (it.value()->value() > 0)
            temas << QStringLiteral("T%1: %2").arg(it.key()).arg(it.value()->value());
    }
    m_total->setText(tr("<b>Total: %1 preguntas</b>").arg(total));
    if (m_plegableTemas)
        //: Resumen del recuadro plegado de las preguntas por tema: %1 es el total y %2, las de cada tema («T1: 3, T2: 2»)
        m_plegableTemas->ponerResumen(total == 0 ? tr("ninguna pregunta") : tr("%1 preguntas (%2)").arg(total).arg(temas.join(QStringLiteral(", "))));
    marcarRepartoPuesto();
    actualizarPuntuacion();
}

void PanelConfiguracion::actualizarPuntuacion()
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

QListWidgetItem *PanelConfiguracion::anadirExamen(const QString &nombre, bool marcado, int versiones)
{
    auto *item = new QListWidgetItem(nombre, m_listaExamenes);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable
                   | Qt::ItemIsDragEnabled);
    item->setCheckState(marcado ? Qt::Checked : Qt::Unchecked);
    item->setData(kRolVersiones, qBound(1, versiones, Generador::kMaxVersiones));
    return item;
}

void PanelConfiguracion::moverExamen(int desplazamiento)
{
    const int fila = m_listaExamenes->currentRow();
    const int destino = fila + desplazamiento;
    if (fila < 0 || destino < 0 || destino >= m_listaExamenes->count())
        return;
    QListWidgetItem *item = m_listaExamenes->takeItem(fila);
    m_listaExamenes->insertItem(destino, item);
    m_listaExamenes->setCurrentItem(item);
}

QVariantMap PanelConfiguracion::versionesDeLaLista() const
{
    QVariantMap versiones;
    for (int i = 0; i < m_listaExamenes->count(); ++i) {
        const QListWidgetItem *item = m_listaExamenes->item(i);
        const int n = item->data(kRolVersiones).toInt();
        if (n > 1 && !item->text().trimmed().isEmpty())
            versiones.insert(item->text().trimmed(), n);
    }
    return versiones;
}

bool PanelConfiguracion::versionesConNumeros() const
{
    return valorAjuste(m_numeracion).toString() == QLatin1String("numeros");
}

void PanelConfiguracion::nuevoExamen()
{
    //: Nombre que se propone para un examen nuevo de la lista (se cambia en el momento)
    QListWidgetItem *item = anadirExamen(tr("Nuevo examen"), true);
    m_listaExamenes->setCurrentItem(item);
    m_listaExamenes->editItem(item);
}

void PanelConfiguracion::quitarExamen()
{
    QListWidgetItem *item = m_listaExamenes->currentItem();
    if (item && confirmar(this, window()->windowTitle(), tr("¿Quitar «%1» de la lista de exámenes?").arg(item->text())))
        delete item;
}

ConfigExamen PanelConfiguracion::configuracionExamenes() const
{
    ConfigExamen c;
    for (int i = 0; i < m_listaExamenes->count(); ++i)
        if (m_listaExamenes->item(i)->checkState() == Qt::Checked) {
            c.nombresModelos << m_listaExamenes->item(i)->text().trimmed();
            c.versiones << qMax(1, m_listaExamenes->item(i)->data(kRolVersiones).toInt());
        }
    c.versionesConNumeros = versionesConNumeros();
    return c;
}

ConfigExamen PanelConfiguracion::configuracion() const
{
    ConfigExamen c;
    c.preguntasPorTema = repartoActual();
    const ConfigExamen examenes = configuracionExamenes();
    c.nombresModelos = examenes.nombresModelos;
    c.versiones = examenes.versiones;
    c.versionesConNumeros = examenes.versionesConNumeros;
    c.noRepetirEntreModelos = m_noRepetir->isChecked();
    c.incluirParaRevisar = m_incluirRevisar->isChecked();
    c.soloVerificadas = m_soloVerificadas->isChecked();
    c.ordenAleatorio = m_ordenAleatorio->isChecked();
    c.barajarOpciones = m_barajarOpciones->isChecked();
    c.evitarOld = m_evitarOld->isChecked();
    c.semilla = m_semilla->text().toUInt();
    return c;
}
