#include "dialogoverificar.h"
#include "paleta.h"
#include "basedatos.h"
#include "dialogoayuda.h"
#include "maquetador.h"
#include "panelverificacion.h"
#include "utilidades.h"
#include "visorampliable.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QTextDocument>
#include <QVBoxLayout>

namespace {

// Claves de la configuración de cada usuario (QSettings) del diálogo de verificar.
const QString kClaveEscala = QStringLiteral("verificar/escala");       // el tamaño de la ficha
const QString kClaveAvisos = QStringLiteral("verificar/avisos");       // los avisos de atención (prisa y pausas)
const QString kClaveResolver = QStringLiteral("verificar/resolver");   // resolverla antes de ver la solución

} // namespace

DialogoVerificar::DialogoVerificar(BaseDatos *bd, const QVector<Pregunta> &preguntas, int inicial, const QVector<Tema> &temas,
                                   std::function<Pregunta(const Pregunta &)> editar, QWidget *parent)
    : QDialog(parent), m_bd(bd), m_preguntas(preguntas), m_temas(temas), m_editar(std::move(editar))
{
    setWindowTitle(tr("Verificar preguntas"));
    resize(900, 720);
    auto *l = new QVBoxLayout(this);
    m_posicion = new QLabel;
    QFont negrita = m_posicion->font();
    negrita.setBold(true);
    m_posicion->setFont(negrita);
    // Arriba, a la derecha, el avance: cuántas de las que se recorren están ya verificadas.
    auto *arriba = new QHBoxLayout;
    arriba->addWidget(m_posicion, 1);
    m_progreso = new QLabel;
    m_progreso->setObjectName(QStringLiteral("progresoVerificar"));
    arriba->addWidget(m_progreso);
    m_barra = new QProgressBar;
    m_barra->setTextVisible(false);
    m_barra->setFixedWidth(160);
    m_barra->setMaximumHeight(12);
    arriba->addWidget(m_barra);
    l->addLayout(arriba);
    // El tamaño de la letra (Ctrl + rueda, Ctrl++, Ctrl+-, Ctrl+0) es el mismo en todas las preguntas, y se recuerda
    // de una vez a otra.
    m_ficha = new VisorAmpliable;
    m_ficha->recordarEscalaEn(kClaveEscala);
    addActions(m_ficha->accionesEscala());
    l->addWidget(m_ficha, 1);

    auto *grupo = new QGroupBox(tr("Verificación"));
    auto *lg = new QVBoxLayout(grupo);
    m_panel = new PanelVerificacion(bd);
    lg->addWidget(m_panel);
    m_aviso = new QLabel;
    m_aviso->setStyleSheet(Paleta::css(Paleta::Atencion));
    m_aviso->setObjectName(QStringLiteral("avisoVerificar"));
    m_aviso->hide();
    lg->addWidget(m_aviso);
    m_avisos = new QCheckBox(tr("Avisar si voy muy deprisa o llevo mucho rato sin parar"));
    m_avisos->setObjectName(QStringLiteral("avisosVerificar"));
    m_avisos->setToolTip(tr("Avisa si se marca una pregunta como verificada sin que haya dado tiempo a leerla (hay que "
                            "pulsar otra vez) y propone una pausa cada 25 minutos seguidos verificando."));
    m_avisos->setChecked(QSettings().value(kClaveAvisos, true).toBool());
    connect(m_avisos, &QCheckBox::toggled, this,
            [](bool marcada) { QSettings().setValue(kClaveAvisos, marcada); });
    lg->addWidget(m_avisos);

    m_resolver = new QCheckBox(tr("Resolverla antes de ver la solución"));
    m_resolver->setObjectName(QStringLiteral("resolverVerificar"));
    m_resolver->setToolTip(tr("Las preguntas sin verificar se muestran sin la respuesta correcta ni la explicación: se "
                              "resuelve la pregunta y, al mostrar la solución, se compara con la del banco. Viéndola "
                              "desde el principio es más fácil dar por buena una respuesta equivocada."));
    m_resolver->setChecked(QSettings().value(kClaveResolver, false).toBool());
    lg->addWidget(m_resolver);
    m_filaRespuesta = new QWidget;
    auto *lr = new QHBoxLayout(m_filaRespuesta);
    lr->setContentsMargins(0, 0, 0, 0);
    //: En el diálogo de verificar, delante de las casillas a), b), c) y d) para decir cuál se cree que es la correcta
    lr->addWidget(new QLabel(tr("Su respuesta:")));
    m_respuesta = new QButtonGroup(this);
    for (int i = 0; i < 4; ++i) {
        auto *r = new QRadioButton(QStringLiteral("%1)").arg(Pregunta::letra(i)));
        m_respuesta->addButton(r, i);
        lr->addWidget(r);
    }
    lr->addStretch();
    lg->addWidget(m_filaRespuesta);
    connect(m_resolver, &QCheckBox::toggled, this, [this](bool marcada) {
        QSettings().setValue(kClaveResolver, marcada);
        // En la pregunta que se ve, si aún no se ha tocado su verificación.
        if (m_actual >= 0 && (!marcada || (!m_preguntas.at(m_actual).verificada && m_panel->sinCambios())))
            ponerOculta(marcada);
    });
    l->addWidget(grupo);
    m_atencion.empezar();
    connect(m_panel, &PanelVerificacion::cambiado, m_aviso, &QWidget::hide);

    auto *fila = new QHBoxLayout;
    //: Botón: pasa a la pregunta anterior de la lista (guarda lo que se haya marcado)
    m_anterior = new QPushButton(tr("&Anterior"));
    //: Botón: pasa a la pregunta siguiente de la lista sin marcarla (guarda lo que se haya marcado)
    m_siguiente = new QPushButton(tr("&Siguiente"));
    m_botonEditar = new QPushButton(tr("&Editar…"));
    m_botonEditar->setToolTip(tr("Abre la pregunta en el editor para corregirla"));
    m_botonEditar->setVisible(bool(m_editar));
    m_verificarYSeguir = new QPushButton;
    m_verificarYSeguir->setToolTip(tr("La marca como verificada (por los mismos que la anterior, si no se marca a nadie) y "
                                      "pasa a la siguiente (Intro)"));
    for (QPushButton *b : {m_anterior, m_siguiente, m_botonEditar}) {
        b->setAutoDefault(false);
        fila->addWidget(b);
    }
    fila->addStretch();
    fila->addWidget(m_verificarYSeguir);
    m_verificarYSeguir->setDefault(true);
    auto *botones = new QDialogButtonBox;
    botones->addButton(tr("Cerrar"), QDialogButtonBox::RejectRole)->setAutoDefault(false);
    connect(botones, &QDialogButtonBox::rejected, this, &QDialog::reject);
    DialogoAyuda::ponerAyuda(this, botones, QStringLiteral("verificar"));
    fila->addWidget(botones);
    l->addLayout(fila);

    connect(m_anterior, &QPushButton::clicked, this, [this]() { ir(m_actual - 1); });
    connect(m_siguiente, &QPushButton::clicked, this, [this]() { ir(m_actual + 1); });
    connect(m_verificarYSeguir, &QPushButton::clicked, this, &DialogoVerificar::verificarYSeguir);
    connect(m_botonEditar, &QPushButton::clicked, this, &DialogoVerificar::editarActual);

    if (!m_preguntas.isEmpty())
        mostrar(qBound(0, inicial, int(m_preguntas.size()) - 1));
    m_verificarYSeguir->setFocus();
}

void DialogoVerificar::mostrar(int indice)
{
    m_actual = indice;
    m_atencion.preguntaMostrada();
    const Pregunta &p = m_preguntas.at(indice);
    //: Arriba del diálogo de verificar: «Pregunta 3 de 20: T1-7a»
    m_posicion->setText(tr("Pregunta %1 de %2: %3").arg(indice + 1).arg(m_preguntas.size()).arg(p.codigoCompleto()));
    m_panel->ponerPregunta(p);
    ponerProgreso();
    m_aviso->hide();
    m_anterior->setEnabled(indice > 0);
    m_siguiente->setEnabled(indice + 1 < m_preguntas.size());
    m_respuesta->setExclusive(false);
    for (QAbstractButton *b : m_respuesta->buttons())
        b->setChecked(false);
    m_respuesta->setExclusive(true);
    // Las que ya están verificadas se ven enteras: ya se revisaron.
    ponerOculta(m_resolver->isChecked() && !p.verificada);
}

void DialogoVerificar::ponerOculta(bool oculta)
{
    m_oculta = oculta;
    const Pregunta &p = m_preguntas.at(m_actual);
    m_ficha->ponerDocumento(Maquetador::crearDocumento(Maquetador::fichaPreguntaHtml(p, nombreTema(m_temas, p.temaId), !oculta),
                                                       {p}, m_ficha));
    // Sin ver la solución no se puede dar por verificada.
    m_panel->setEnabled(!oculta);
    m_filaRespuesta->setVisible(oculta);
    if (oculta) {
        //: Botón (Intro) del diálogo de verificar cuando la solución está oculta: la enseña
        m_verificarYSeguir->setText(tr("&Mostrar la solución"));
        return;
    }
    //: Botón (Intro): marca la pregunta como verificada y pasa a la siguiente
    m_verificarYSeguir->setText(m_actual + 1 < m_preguntas.size() ? tr("&Verificada, siguiente")
                                                                   //: Botón (Intro) en la última pregunta: la marca como verificada
                                                                   : tr("&Verificada"));
}

void DialogoVerificar::mostrarSolucion()
{
    const int respuesta = m_respuesta->checkedId();
    const int correcta = m_preguntas.at(m_actual).correcta;
    ponerOculta(false);
    if (respuesta < 0)
        return;
    if (respuesta == correcta)
        //: Al mostrar la solución en el diálogo de verificar; %1 es la letra de la opción, «c)»
        m_aviso->setText(tr("Su respuesta, la %1, coincide con la del banco.").arg(QStringLiteral("%1)").arg(Pregunta::letra(respuesta))));
    else
        //: Al mostrar la solución en el diálogo de verificar; %1 y %2 son letras de opciones, «b)» y «c)»
        m_aviso->setText(tr("Ha respondido la %1 y la respuesta del banco es la %2: revísela con cuidado.")
                             .arg(QStringLiteral("%1)").arg(Pregunta::letra(respuesta)), QStringLiteral("%1)").arg(Pregunta::letra(correcta))));
    m_aviso->show();
}

bool DialogoVerificar::conAvisos() const
{
    return m_avisos->isChecked();
}

void DialogoVerificar::ponerProgreso()
{
    int verificadas = 0;
    for (const Pregunta &p : m_preguntas)
        verificadas += p.verificada ? 1 : 0;
    //: Arriba del diálogo de verificar, junto a la barra de avance: «Verificadas: 12 de 20»
    m_progreso->setText(tr("Verificadas: %1 de %2").arg(verificadas).arg(m_preguntas.size()));
    m_barra->setRange(0, qMax(1, int(m_preguntas.size())));
    m_barra->setValue(verificadas);
}

bool DialogoVerificar::guardar()
{
    if (m_actual < 0 || m_panel->sinCambios())
        return true;
    Pregunta &p = m_preguntas[m_actual];
    if (!m_bd->guardarVerificacion(p.id, m_panel->verificada(), m_panel->verificadores())) {
        QMessageBox::warning(this, windowTitle(), tr("No se pudo guardar la verificación:\n%1").arg(m_bd->ultimoError()));
        return false;
    }
    if (!p.verificada && m_panel->verificada())
        m_atencion.contarVerificada();
    p.verificada = m_panel->verificada();
    p.verificadores = p.verificada ? m_panel->verificadores() : QStringList();
    m_cambiadas.insert(p.id, m_actual);
    m_panel->recordar();
    m_panel->ponerPregunta(p);
    ponerProgreso();
    emit verificacionGuardada(p.id);
    return true;
}

void DialogoVerificar::ir(int indice)
{
    if (indice < 0 || indice >= m_preguntas.size() || !guardar())
        return;
    mostrar(indice);
}

void DialogoVerificar::verificarYSeguir()
{
    if (m_actual < 0)
        return;
    if (m_oculta) {
        mostrarSolucion();
        return;
    }
    // Marcada antes de que haya dado tiempo a leerla: se avisa una vez, y hay que pulsar de nuevo.
    if (conAvisos() && !m_preguntas.at(m_actual).verificada && m_atencion.demasiadoDeprisa(m_actual, m_preguntas.at(m_actual))) {
        m_aviso->setText(tr("Apenas ha habido tiempo para leerla. Si ya la ha revisado, pulse otra vez."));
        m_aviso->show();
        return;
    }
    m_atencion.contarDescanso();
    m_panel->marcarVerificada();
    // Si el banco tiene verificadores, tiene que decir quién (la primera vez no hay «últimos» que proponer).
    if (m_panel->hayVerificadores() && m_panel->verificadores().isEmpty()) {
        m_aviso->setText(tr("Marque quién la ha verificado."));
        m_aviso->show();
        return;
    }
    if (!guardar())
        return;
    if (m_actual + 1 < m_preguntas.size()) {
        mostrar(m_actual + 1);
        // La atención baja al cabo de un rato de una tarea repetitiva: se propone una pausa.
        int minutos = 0;
        int verificadas = 0;
        if (conAvisos() && m_atencion.tocaPausa(&minutos, &verificadas)) {
            //: En el diálogo de verificar; %1 son minutos y %2, preguntas (más de una casi siempre)
            m_aviso->setText(tr("Lleva %1 min verificando sin parar (preguntas verificadas: %2). Una pausa breve ayuda a "
                                "mantener la atención.").arg(minutos).arg(verificadas));
            m_aviso->show();
        }
    } else {
        m_aviso->setText(tr("Era la última de la lista."));
        m_aviso->show();
    }
}

void DialogoVerificar::editarActual()
{
    if (!m_editar || m_actual < 0 || !guardar())
        return;
    const Pregunta nueva = m_editar(m_preguntas.at(m_actual));
    if (nueva.id == 0)
        return;
    m_preguntas[m_actual] = nueva;
    m_cambiadas.insert(nueva.id, m_actual);
    m_editadas = true;
    mostrar(m_actual);
}

void DialogoVerificar::done(int resultado)
{
    guardar();
    QDialog::done(resultado);
}

QVector<Pregunta> DialogoVerificar::cambiadas() const
{
    QVector<Pregunta> res;
    for (int indice : m_cambiadas)
        res << m_preguntas.at(indice);
    return res;
}
