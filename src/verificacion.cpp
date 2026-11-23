#include "verificacion.h"
#include "basedatos.h"
#include "dialogoayuda.h"
#include "maquetador.h"
#include "utilidades.h"
#include "visorampliable.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QTextDocument>
#include <QVBoxLayout>

namespace {

constexpr int kColumnas = 3;   // casillas de verificadores por fila

// Claves de la configuración de cada usuario (QSettings) del diálogo de verificar.
const QString kClaveEscala = QStringLiteral("verificar/escala");       // el tamaño de la ficha
const QString kClaveAvisos = QStringLiteral("verificar/avisos");       // los avisos de atención (prisa y pausas)
const QString kClaveResolver = QStringLiteral("verificar/resolver");   // resolverla antes de ver la solución

QStringList ordenados(QStringList nombres)
{
    nombres.sort(Qt::CaseInsensitive);
    return nombres;
}

} // namespace

// ---------------------------------------------------------------------------------------------
// PanelVerificacion

PanelVerificacion::PanelVerificacion(BaseDatos *bd, bool conCasilla, QWidget *parent)
    : QWidget(parent), m_bd(bd)
{
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    if (conCasilla) {
        m_verificada = new QCheckBox(tr("Verificada por humanos"));
        m_verificada->setToolTip(tr("Una persona (al menos) ha revisado la pregunta. Al marcarla se marcan también los que "
                                    "verificaron la anterior, que suelen ser los mismos."));
        connect(m_verificada, &QCheckBox::toggled, this, &PanelVerificacion::verificadaCambiada);
        l->addWidget(m_verificada);
    }
    auto *fila = new QHBoxLayout;
    if (conCasilla)
        fila->setContentsMargins(20, 0, 0, 0);   // bajo la casilla, como lo que depende de ella
    //: Delante de las casillas de las personas que han verificado la pregunta: «Por: [ ] Ana  [ ] Luis»
    fila->addWidget(new QLabel(tr("Por:")), 0, Qt::AlignTop);
    m_rejilla = new QGridLayout;
    m_rejilla->setContentsMargins(0, 0, 0, 0);
    fila->addLayout(m_rejilla, 1);
    m_ninguno = new QLabel(tr("Aún no hay verificadores en este banco."));
    m_ninguno->setStyleSheet(QStringLiteral("color: gray"));
    fila->addWidget(m_ninguno, 1, Qt::AlignTop);
    //: Botón: abre la lista de verificadores del banco para añadir o quitar personas
    auto *gestionar = new QPushButton(tr("Añadir o quitar…"));
    gestionar->setAutoDefault(false);
    gestionar->setToolTip(tr("Añade verificadores al banco, les cambia el nombre o los quita"));
    connect(gestionar, &QPushButton::clicked, this, &PanelVerificacion::gestionar);
    fila->addWidget(gestionar, 0, Qt::AlignTop);
    l->addLayout(fila);
    construir({});
}

void PanelVerificacion::construir(const QStringList &marcados)
{
    qDeleteAll(m_casillas);
    m_casillas.clear();
    const QStringList nombres = m_bd->verificadores();
    for (int i = 0; i < nombres.size(); ++i) {
        auto *c = new QCheckBox(nombres.at(i));
        c->setChecked(marcados.contains(nombres.at(i)));
        connect(c, &QCheckBox::toggled, this, &PanelVerificacion::personaCambiada);
        m_rejilla->addWidget(c, i / kColumnas, i % kColumnas);
        m_casillas << c;
    }
    m_ninguno->setVisible(nombres.isEmpty());
}

void PanelVerificacion::ponerPregunta(const Pregunta &p)
{
    m_cambiando = true;
    if (m_verificada)
        m_verificada->setChecked(p.verificada);
    for (QCheckBox *c : std::as_const(m_casillas))
        c->setChecked(p.verificada && p.verificadores.contains(c->text()));
    m_cambiando = false;
    m_antes.clear();
    m_inicialVerificada = p.verificada;
    m_inicialVerificadores = p.verificada ? verificadores() : QStringList();
}

bool PanelVerificacion::verificada() const
{
    return m_verificada ? m_verificada->isChecked() : true;
}

QStringList PanelVerificacion::verificadores() const
{
    QStringList res;
    for (const QCheckBox *c : m_casillas)
        if (c->isChecked())
            res << c->text();
    return res;
}

bool PanelVerificacion::sinCambios() const
{
    if (verificada() != m_inicialVerificada)
        return false;
    return !verificada() || ordenados(verificadores()) == ordenados(m_inicialVerificadores);
}

void PanelVerificacion::marcar(const QStringList &nombres)
{
    for (QCheckBox *c : std::as_const(m_casillas))
        if (nombres.contains(c->text()))
            c->setChecked(true);
}

void PanelVerificacion::marcarVerificada()
{
    if (m_verificada && !m_verificada->isChecked())
        m_verificada->setChecked(true);   // marca a los últimos (verificadaCambiada())
    else if (verificadores().isEmpty())
        marcar(m_bd->ultimosVerificadores());
}

void PanelVerificacion::recordar()
{
    if (verificada() && !verificadores().isEmpty())
        m_bd->guardarUltimosVerificadores(verificadores());
}

void PanelVerificacion::verificadaCambiada(bool marcada)
{
    if (m_cambiando)
        return;
    m_cambiando = true;
    if (!marcada) {
        m_antes = verificadores();
        for (QCheckBox *c : std::as_const(m_casillas))
            c->setChecked(false);
    } else if (verificadores().isEmpty()) {
        // Los que había antes de desmarcarla; si no, los que verificaron la última.
        marcar(m_antes.isEmpty() ? m_bd->ultimosVerificadores() : m_antes);
    }
    m_cambiando = false;
    emit cambiado();
}

void PanelVerificacion::personaCambiada(bool marcada)
{
    if (m_cambiando)
        return;
    if (marcada && m_verificada && !m_verificada->isChecked()) {
        m_cambiando = true;
        m_verificada->setChecked(true);
        m_cambiando = false;
    }
    emit cambiado();
}

void PanelVerificacion::gestionar()
{
    DialogoVerificadores d(m_bd, this);
    d.exec();
    if (!d.hayCambios())
        return;
    // Los marcados (y los que tenía la pregunta) siguen marcados con su nombre nuevo; los quitados, no.
    auto renombrar = [&d](QStringList nombres) {
        for (QString &n : nombres)
            n = d.renombrados().value(n, n);
        return nombres;
    };
    const QStringList existentes = m_bd->verificadores();
    auto quedan = [&existentes](const QStringList &nombres) {
        QStringList res;
        for (const QString &n : nombres)
            if (existentes.contains(n))
                res << n;
        return res;
    };
    const QStringList marcados = quedan(renombrar(verificadores()));
    m_inicialVerificadores = quedan(renombrar(m_inicialVerificadores));
    m_antes = quedan(renombrar(m_antes));
    m_cambiando = true;
    construir(marcados);
    m_cambiando = false;
    emit verificadoresCambiados();
    emit cambiado();
}

// ---------------------------------------------------------------------------------------------
// DialogoVerificadores

DialogoVerificadores::DialogoVerificadores(BaseDatos *bd, QWidget *parent)
    : QDialog(parent), m_bd(bd)
{
    setWindowTitle(tr("Verificadores"));
    resize(460, 360);
    auto *l = new QVBoxLayout(this);
    auto *explicacion = new QLabel(tr("Las personas que revisan las preguntas de este banco. En cada pregunta se marca "
                                      "quiénes la han verificado."));
    explicacion->setWordWrap(true);
    l->addWidget(explicacion);
    auto *fila = new QHBoxLayout;
    m_lista = new QListWidget;
    fila->addWidget(m_lista, 1);
    auto *botones = new QVBoxLayout;
    //: Botón: añade una persona a la lista de verificadores
    auto *anadir = new QPushButton(tr("Añadir…"));
    m_cambiarNombre = new QPushButton(tr("Cambiar el nombre…"));
    //: Botón: quita a la persona seleccionada de la lista de verificadores
    m_quitar = new QPushButton(tr("Quitar"));
    for (QPushButton *b : {anadir, m_cambiarNombre, m_quitar}) {
        b->setAutoDefault(false);
        botones->addWidget(b);
    }
    botones->addStretch();
    fila->addLayout(botones);
    l->addLayout(fila, 1);
    auto *cerrar = new QDialogButtonBox;
    cerrar->addButton(tr("Cerrar"), QDialogButtonBox::AcceptRole);
    connect(cerrar, &QDialogButtonBox::accepted, this, &QDialog::accept);
    DialogoAyuda::ponerAyuda(this, cerrar, QStringLiteral("verificadores"));
    l->addWidget(cerrar);

    connect(anadir, &QPushButton::clicked, this, &DialogoVerificadores::anadir);
    connect(m_cambiarNombre, &QPushButton::clicked, this, &DialogoVerificadores::cambiarNombre);
    connect(m_quitar, &QPushButton::clicked, this, &DialogoVerificadores::quitar);
    connect(m_lista, &QListWidget::itemDoubleClicked, this, &DialogoVerificadores::cambiarNombre);
    connect(m_lista, &QListWidget::currentRowChanged, this, [this]() {
        m_cambiarNombre->setEnabled(!seleccionado().isEmpty());
        m_quitar->setEnabled(!seleccionado().isEmpty());
    });
    actualizar();
}

void DialogoVerificadores::actualizar(const QString &seleccionar)
{
    m_lista->clear();
    for (const QString &nombre : m_bd->verificadores()) {
        const int n = m_bd->preguntasVerificadasPor(nombre);
        //: Un verificador en la lista, con cuántas preguntas ha verificado
        auto *item = new QListWidgetItem(n == 0 ? nombre : n == 1 ? tr("%1 (1 pregunta)").arg(nombre)
                                                                  : tr("%1 (%2 preguntas)").arg(nombre).arg(n));
        item->setData(Qt::UserRole, nombre);
        m_lista->addItem(item);
        if (nombre == seleccionar)
            m_lista->setCurrentItem(item);
    }
    m_cambiarNombre->setEnabled(!seleccionado().isEmpty());
    m_quitar->setEnabled(!seleccionado().isEmpty());
}

QString DialogoVerificadores::seleccionado() const
{
    const QListWidgetItem *item = m_lista->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

void DialogoVerificadores::anadir()
{
    bool ok = false;
    //: Etiqueta del campo para escribir el nombre de un verificador
    const QString nombre = QInputDialog::getText(this, tr("Añadir verificador"), tr("Nombre:"), QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || nombre.isEmpty())
        return;
    if (!m_bd->anadirVerificador(nombre)) {
        QMessageBox::warning(this, windowTitle(), m_bd->ultimoError());
        return;
    }
    m_cambios = true;
    actualizar(nombre);
}

void DialogoVerificadores::cambiarNombre()
{
    const QString antes = seleccionado();
    if (antes.isEmpty())
        return;
    bool ok = false;
    const QString despues = QInputDialog::getText(this, tr("Cambiar el nombre"), tr("Nombre:"), QLineEdit::Normal, antes, &ok).trimmed();
    if (!ok || despues.isEmpty() || despues == antes)
        return;
    if (!m_bd->cambiarNombreVerificador(antes, despues)) {
        QMessageBox::warning(this, windowTitle(), m_bd->ultimoError());
        return;
    }
    // Si ya se había cambiado antes, el nombre original pasa directamente al nuevo.
    bool encadenado = false;
    for (auto it = m_renombrados.begin(); it != m_renombrados.end(); ++it) {
        if (it.value() == antes) {
            it.value() = despues;
            encadenado = true;
        }
    }
    if (!encadenado)
        m_renombrados.insert(antes, despues);
    m_cambios = true;
    actualizar(despues);
}

void DialogoVerificadores::quitar()
{
    const QString nombre = seleccionado();
    if (nombre.isEmpty())
        return;
    const int n = m_bd->preguntasVerificadasPor(nombre);
    QString texto = tr("¿Quitar a %1 de la lista de verificadores?").arg(nombre);
    if (n > 0)
        texto += QStringLiteral("\n\n") + (n == 1 ? tr("Ha verificado 1 pregunta: seguirá verificada, pero ya no dirá que la verificó.")
                                                  : tr("Ha verificado %1 preguntas: seguirán verificadas, pero ya no dirán que las verificó.").arg(n));
    if (!confirmar(this, windowTitle(), texto))
        return;
    if (!m_bd->borrarVerificador(nombre)) {
        QMessageBox::warning(this, windowTitle(), m_bd->ultimoError());
        return;
    }
    m_cambios = true;
    actualizar();
}

// ---------------------------------------------------------------------------------------------
// DialogoVerificar

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
    m_ficha->ponerEscala(QSettings().value(kClaveEscala, 1.0).toDouble());
    connect(m_ficha, &VisorAmpliable::escalaCambiada, this,
            [](qreal escala) { QSettings().setValue(kClaveEscala, escala); });
    addActions(m_ficha->accionesEscala());
    l->addWidget(m_ficha, 1);

    auto *grupo = new QGroupBox(tr("Verificación"));
    auto *lg = new QVBoxLayout(grupo);
    m_panel = new PanelVerificacion(bd);
    lg->addWidget(m_panel);
    m_aviso = new QLabel;
    m_aviso->setStyleSheet(QStringLiteral("color: #b45309"));
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
    m_sinParar.start();
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
    m_vista.start();
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

int DialogoVerificar::tiempoParaLeer(const Pregunta &p)
{
    // Lo que se lee al verificar: el enunciado, las opciones y la explicación. A unas 400 palabras por minuto, que es
    // leer muy deprisa: por debajo no se ha leído. Entre 2 y 20 segundos.
    static const QRegularExpression espacios(QStringLiteral("\\s+"));
    const QString texto = p.enunciado + QLatin1Char(' ') + p.opciones.join(QLatin1Char(' ')) + QLatin1Char(' ') + p.explicacion;
    const int palabras = int(texto.split(espacios, Qt::SkipEmptyParts).size());
    return qBound(2000, palabras * 150, 20000);
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
    m_desdeLaPausa += !p.verificada && m_panel->verificada() ? 1 : 0;
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
    if (conAvisos() && !m_preguntas.at(m_actual).verificada && m_avisadaDePrisa != m_actual
        && m_vista.elapsed() < tiempoParaLeer(m_preguntas.at(m_actual))) {
        m_avisadaDePrisa = m_actual;
        m_aviso->setText(tr("Apenas ha habido tiempo para leerla. Si ya la ha revisado, pulse otra vez."));
        m_aviso->show();
        return;
    }
    // Si se ha estado un rato sin tocar nada, eso ya ha sido una pausa.
    if (m_vista.elapsed() > 3 * 60 * 1000) {
        m_sinParar.start();
        m_desdeLaPausa = 0;
    }
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
        if (conAvisos() && m_desdeLaPausa > 0 && m_sinParar.elapsed() >= m_pausaCada) {
            //: En el diálogo de verificar; %1 son minutos y %2, preguntas (más de una casi siempre)
            m_aviso->setText(tr("Lleva %1 min verificando sin parar (preguntas verificadas: %2). Una pausa breve ayuda a "
                                "mantener la atención.").arg(m_sinParar.elapsed() / 60000).arg(m_desdeLaPausa));
            m_aviso->show();
            m_sinParar.start();
            m_desdeLaPausa = 0;
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
