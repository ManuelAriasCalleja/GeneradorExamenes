#include "pestanabanco.h"
#include "paleta.h"
#include "basedatos.h"
#include "dialogoayuda.h"
#include "dialogoverificadores.h"
#include "dialogoverificar.h"
#include "maquetador.h"
#include "modelopreguntas.h"
#include "panelverificacion.h"
#include "utilidades.h"
#include "visorampliable.h"
#include "zonaampliable.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QTableView>
#include <QTextDocument>
#include <QVBoxLayout>
#include <algorithm>

namespace {

// Los filtros, en la configuración de cada usuario (QSettings).
const QString kClaveFiltroTema = QStringLiteral("banco/filtroTema");
const QString kClaveFiltroEstado = QStringLiteral("banco/filtroEstado");

} // namespace

PestanaBanco::PestanaBanco(BaseDatos *bd, QWidget *parent)
    : QWidget(parent), m_bd(bd)
{
    auto *l = new QVBoxLayout(this);

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
    connect(m_tabla, &QWidget::customContextMenuRequested, this, &PestanaBanco::menuTabla);
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
    auto boton = [&](const QString &texto, void (PestanaBanco::*slot)()) {
        auto *b = new QPushButton(texto);
        connect(b, &QPushButton::clicked, this, slot);
        botones->addWidget(b);
    };
    //: Botón: nueva pregunta
    boton(tr("Nueva…"), &PestanaBanco::nuevaPregunta);
    //: Botón: editar la pregunta seleccionada
    boton(tr("Editar…"), &PestanaBanco::editarPregunta);
    //: Botón: recorre las preguntas de la lista, desde la seleccionada, para marcarlas como verificadas por personas
    boton(tr("Verificar…"), &PestanaBanco::verificarPregunta);
    boton(tr("Duplicar como variante…"), &PestanaBanco::duplicarPregunta);
    //: Botón: eliminar la pregunta seleccionada
    boton(tr("Eliminar"), &PestanaBanco::eliminarPregunta);
    botones->addStretch();
    l->addLayout(botones);

    // Los filtros de tema y de estado se recuerdan de una vez a otra, para seguir donde se dejó («Tema 4, sin
    // verificar»): los que elige el usuario, no los que pone el programa. El texto que se busca no se recuerda.
    m_filtroEstado->setCurrentIndex(qMax(0, m_filtroEstado->findData(QSettings().value(kClaveFiltroEstado, 0))));
    connect(m_filtroTema, &QComboBox::activated, this,
            [this]() { QSettings().setValue(kClaveFiltroTema, m_filtroTema->currentData()); });
    connect(m_filtroEstado, &QComboBox::activated, this,
            [this]() { QSettings().setValue(kClaveFiltroEstado, m_filtroEstado->currentData()); });
    connect(m_filtroTema, &QComboBox::currentIndexChanged, this, &PestanaBanco::filtrar);
    connect(m_filtroEstado, &QComboBox::currentIndexChanged, this, &PestanaBanco::filtrar);
    connect(m_filtroTexto, &QLineEdit::textChanged, this, &PestanaBanco::filtrar);
    connect(m_tabla->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &PestanaBanco::actualizarFicha);
    connect(m_tabla, &QTableView::doubleClicked, this, &PestanaBanco::editarPregunta);
}

void PestanaBanco::ponerZona(ZonaAmpliable *zona)
{
    // La lista y la ficha se amplían juntas. El ancho de las columnas y el alto de las filas no cambian solos con la
    // letra: se ajustan aquí.
    zona->anadirVisor(m_ficha);
    auto ajustarColumnas = [this, anterior = 1.0, alto = m_tabla->verticalHeader()->defaultSectionSize()](qreal escala) mutable {
        m_tabla->verticalHeader()->setDefaultSectionSize(qRound(alto * escala));
        for (int c = 0; c < m_tabla->model()->columnCount(); ++c)
            if (m_tabla->horizontalHeader()->sectionResizeMode(c) == QHeaderView::Interactive)
                m_tabla->setColumnWidth(c, qRound(m_tabla->columnWidth(c) * escala / anterior));
        anterior = escala;
    };
    ajustarColumnas(zona->escala());
    connect(zona, &ZonaAmpliable::escalaCambiada, this, ajustarColumnas);
}

void PestanaBanco::ponerDatos(const QVector<Tema> &temas, const QVector<Pregunta> &preguntas)
{
    m_temas = temas;
    m_preguntas = preguntas;

    int temaFiltro = m_filtroTema->currentData().toInt();
    // La primera vez, el tema que se dejó elegido la última vez que se usó el programa (si el banco lo tiene).
    if (!m_filtroTemaRecordado && !m_temas.isEmpty()) {
        temaFiltro = QSettings().value(kClaveFiltroTema, 0).toInt();
        m_filtroTemaRecordado = true;
    }
    m_filtroTema->blockSignals(true);
    m_filtroTema->clear();
    m_filtroTema->addItem(tr("Todos los temas"), 0);
    for (const Tema &t : std::as_const(m_temas))
        m_filtroTema->addItem(t.nombre(), t.numero);
    m_filtroTema->setCurrentIndex(qMax(0, m_filtroTema->findData(temaFiltro)));
    m_filtroTema->blockSignals(false);

    m_modelo->establecer(m_preguntas);
    filtrar();
}

void PestanaBanco::seleccionar(int id)
{
    const int fila = m_modelo->filaDeId(id);
    const QModelIndex i = m_filtro->mapFromSource(m_modelo->index(fila, 0));
    if (i.isValid()) {
        m_tabla->setCurrentIndex(i);
        m_tabla->scrollTo(i);
    }
}

void PestanaBanco::filtrar()
{
    m_filtro->setTema(m_filtroTema->currentData().toInt());
    m_filtro->setEstado(FiltroPreguntas::FiltroEstado(m_filtroEstado->currentData().toInt()));
    m_filtro->setTexto(m_filtroTexto->text());
    ponerCuenta();
    actualizarFicha();
}

// Cuántas preguntas hay en la lista (las que cumplen los filtros) y cuántas de ellas están verificadas.
void PestanaBanco::ponerCuenta()
{
    const QVector<Pregunta> lista = preguntasDeLaLista();
    const int n = int(lista.size());
    const int verificadas = int(std::count_if(lista.begin(), lista.end(), [](const Pregunta &p) { return p.verificada; }));
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

QVector<Pregunta> PestanaBanco::preguntasDeLaLista() const
{
    QVector<Pregunta> lista;
    lista.reserve(m_filtro->rowCount());
    for (int f = 0; f < m_filtro->rowCount(); ++f)
        lista << m_modelo->pregunta(m_filtro->mapToSource(m_filtro->index(f, 0)).row());
    return lista;
}

Pregunta PestanaBanco::preguntaSeleccionada(bool *ok)
{
    const QModelIndex i = m_filtro->mapToSource(m_tabla->currentIndex());
    *ok = i.isValid();
    return i.isValid() ? m_modelo->pregunta(i.row()) : Pregunta();
}

void PestanaBanco::actualizarFicha()
{
    bool ok;
    const Pregunta p = preguntaSeleccionada(&ok);
    if (!ok) {
        m_ficha->clear();
        return;
    }
    QString html = Maquetador::fichaPreguntaHtml(p, nombreTema(m_temas, p.temaId));
    const QStringList usos = m_bd->usosDe(p.codigoCompleto());
    if (!usos.isEmpty())
        html.replace(QStringLiteral("</body>"), tr("<p><b>Ha salido en:</b> %1</p></body>")
                                                    .arg(Maquetador::escapar(usos.join(QStringLiteral("; ")))));
    m_ficha->ponerDocumento(Maquetador::crearDocumento(html, {p}, m_ficha));
}

DialogoPregunta::Despues PestanaBanco::editar(Pregunta p, DialogoPregunta::Despues boton, const QString &texto,
                                                  Pregunta *guardada)
{
    if (guardada)
        *guardada = Pregunta();
    if (m_temas.isEmpty()) {
        QMessageBox::information(this, window()->windowTitle(), tr("Primero cree algún tema (Editar → Temas)."));
        return DialogoPregunta::Cerrar;
    }
    DialogoPregunta d(m_temas, m_preguntas, p, m_bd->idioma(), this);
    d.ponerVerificacion(m_bd);
    if (boton != DialogoPregunta::Cerrar)
        d.anadirBoton(boton, texto);
    while (d.exec() == QDialog::Accepted) {
        Pregunta nueva = d.pregunta();
        if (m_bd->guardarPregunta(nueva)) {
            m_idGuardada = nueva.id;
            if (guardada)
                *guardada = nueva;
            emit bancoEditado(nueva.id);
            return d.despues();
        }
        QMessageBox::warning(this, window()->windowTitle(), tr("No se pudo guardar la pregunta:\n%1").arg(m_bd->ultimoError()));
    }
    return DialogoPregunta::Cerrar;
}

void PestanaBanco::nuevaPregunta()
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

void PestanaBanco::editarPregunta()
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
            emit aviso(tr("Era la última pregunta de la lista."), 5000);
            return;
        }
        m_tabla->setCurrentIndex(m_filtro->index(fila, 0));
        p = preguntaSeleccionada(&ok);
    }
}

void PestanaBanco::duplicarPregunta()
{
    bool ok;
    Pregunta p = preguntaSeleccionada(&ok);
    if (!ok)
        return;
    // Nueva variante del mismo número: siguiente letra libre.
    p.id = 0;
    p.variante = siguienteVariante(m_preguntas, p.temaId, p.numero);
    if (p.variante.isEmpty()) {
        QMessageBox::information(this, window()->windowTitle(), tr("%1 ya tiene variantes de la a a la z.").arg(p.grupo()));
        return;
    }
    p.old = false;
    // Es otra pregunta: nadie la ha revisado aún.
    p.verificada = false;
    p.verificadores.clear();
    editar(p);
}

void PestanaBanco::verificarPregunta()
{
    // Las de la lista, con su filtro y su orden, desde la seleccionada.
    const QVector<Pregunta> preguntas = preguntasDeLaLista();
    if (preguntas.isEmpty())
        return;
    DialogoVerificar d(m_bd, preguntas, qMax(0, m_tabla->currentIndex().row()), m_temas,
                       [this](const Pregunta &p) {
                           Pregunta guardada;
                           editar(p, DialogoPregunta::Cerrar, QString(), &guardada);
                           return guardada;
                       },
                       this);
    // Cada pregunta que se verifica se ve ya en la lista, detrás del diálogo, sin esperar a cerrarlo.
    connect(&d, &DialogoVerificar::verificacionGuardada, this, [this](int id) { emit verificacionCambiada(id, false); });
    d.exec();
    const int actual = preguntas.value(d.actual()).id;
    if (!d.cambiadas().isEmpty())
        emit verificacionCambiada(actual);
    else
        seleccionar(actual);
}

void PestanaBanco::verificarLista()
{
    QVector<Pregunta> lista = preguntasDeLaLista();
    if (lista.isEmpty())
        return;
    QDialog d(this);
    d.setWindowTitle(tr("Marcar como verificadas"));
    auto *l = new QVBoxLayout(&d);
    l->addWidget(new QLabel(lista.size() == 1 ? tr("Se marcará como verificada la pregunta de la lista por:")
                                              : tr("Se marcarán como verificadas las %1 preguntas de la lista por:").arg(lista.size())));
    auto *panel = new PanelVerificacion(m_bd, false);
    panel->marcar(m_bd->ultimosVerificadores());
    l->addWidget(panel);
    auto *nota = new QLabel(tr("A las que ya estaban verificadas se les añaden estas personas."));
    nota->setStyleSheet(Paleta::css(Paleta::Secundario));
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
        QMessageBox::information(this, window()->windowTitle(), tr("Marque quién las ha verificado."));
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
    if (!m_bd->guardarVerificaciones(cambiadas)) {
        QMessageBox::warning(this, window()->windowTitle(), tr("No se pudo guardar la verificación:\n%1").arg(m_bd->ultimoError()));
        return;
    }
    panel->recordar();
    bool ok;
    emit verificacionCambiada(preguntaSeleccionada(&ok).id);
    emit aviso(cambiadas.size() == 1 ? tr("1 pregunta marcada como verificada.")
                                                   : tr("%1 preguntas marcadas como verificadas.").arg(cambiadas.size()),
                             8000);
}

void PestanaBanco::gestionarVerificadores()
{
    DialogoVerificadores d(m_bd, this);
    d.exec();
    if (d.hayCambios()) {
        bool ok;
        emit verificacionCambiada(preguntaSeleccionada(&ok).id);
    }
}

void PestanaBanco::menuTabla(const QPoint &donde)
{
    QMenu menu;
    const QModelIndex i = m_tabla->indexAt(donde);
    if (i.isValid()) {
        m_tabla->setCurrentIndex(i);
        menu.addAction(tr("Editar…"), this, &PestanaBanco::editarPregunta);
        menu.addAction(tr("Verificar…"), this, &PestanaBanco::verificarPregunta);
        menu.addSeparator();
    }
    const int n = m_filtro->rowCount();
    if (n > 0)
        menu.addAction(n == 1 ? tr("Marcar como verificada la pregunta de la lista…")
                              : tr("Marcar como verificadas las %1 preguntas de la lista…").arg(n),
                       this, &PestanaBanco::verificarLista);
    if (!menu.isEmpty())
        menu.exec(m_tabla->viewport()->mapToGlobal(donde));
}

void PestanaBanco::eliminarPregunta()
{
    bool ok;
    const Pregunta p = preguntaSeleccionada(&ok);
    if (!ok)
        return;
    if (!confirmar(this, window()->windowTitle(),
                   tr("¿Eliminar definitivamente la pregunta %1?\n\n%2\n\n"
                      "(Si solo quiere que deje de salir en los exámenes, edítela y márquela como anulada.)")
                       .arg(p.codigoCompleto(), p.enunciado.left(200))))
        return;
    if (!m_bd->borrarPregunta(p.id))
        QMessageBox::warning(this, window()->windowTitle(), m_bd->ultimoError());
    emit bancoEditado(0);
}
