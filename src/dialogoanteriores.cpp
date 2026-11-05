#include "dialogoanteriores.h"
#include "basedatos.h"
#include "dialogoayuda.h"
#include "utilidades.h"

#include <QComboBox>
#include <QDate>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>

using ExamenesAnteriores::Reconocida;

namespace {

enum ColumnaExamen { ColNombre, ColNumPreguntas, ColDudosas };
enum ColumnaPregunta { ColNumero, ColCodigo, ColParecido, ColTexto };

const char *const kClaveCarpeta = "anteriores/carpeta";

// Las dudosas, en amarillo (con el texto oscuro, que se lea también con el tema oscuro).
const QColor kFondoDudosa(0xff, 0xf3, 0xcd);
const QColor kTextoDudosa(0x66, 0x4d, 0x03);

int dudosas(const QVector<Reconocida> &preguntas)
{
    return int(std::count_if(preguntas.cbegin(), preguntas.cend(), [](const Reconocida &r) { return r.dudosa; }));
}

} // namespace

DialogoAnteriores::DialogoAnteriores(BaseDatos *bd, const QVector<Pregunta> &banco, const DatosConvocatoria &datos,
                                     const QStringList &conocidos, QWidget *parent)
    : QDialog(parent), m_bd(bd), m_banco(banco), m_datos(datos), m_conocidos(conocidos)
{
    setWindowTitle(tr("Registrar exámenes anteriores"));
    resize(1100, 640);

    auto *explicacion = new QLabel(tr(
        "Para que al generar se eviten las preguntas de las convocatorias hechas fuera del programa, añada aquí sus "
        "exámenes: sus PDF o su texto copiado de Word. El programa reconoce cada pregunta en el banco; compruebe las "
        "dudosas (en amarillo) y, si hace falta, corrija su código. Se registran en el historial como las generadas."));
    explicacion->setWordWrap(true);

    // Por defecto, la última convocatoria; al añadir el primer examen, la que diga su fichero.
    const DatosConvocatoria anterior = DatosConvocatoria::anterior(QDate::currentDate(), datos.idioma);
    m_convocatoria = new QComboBox;
    m_convocatoria->setEditable(true);
    m_convocatoria->addItems(DatosConvocatoria::ordinarias(datos.idioma));
    m_convocatoria->setCurrentText(anterior.convocatoria);
    m_anio = new QSpinBox;
    m_anio->setRange(2000, 2100);
    m_anio->setValue(anterior.anio);
    auto *filaConvocatoria = new QHBoxLayout;
    filaConvocatoria->addWidget(new QLabel(tr("Convocatoria:")));
    filaConvocatoria->addWidget(m_convocatoria);
    filaConvocatoria->addWidget(new QLabel(tr("Año:")));
    filaConvocatoria->addWidget(m_anio);
    filaConvocatoria->addStretch();

    m_lista = new QTableWidget(0, 3);
    m_lista->setHorizontalHeaderLabels({tr("Examen"), tr("Preguntas"), tr("Dudosas")});
    m_lista->horizontalHeader()->setSectionResizeMode(ColNombre, QHeaderView::Stretch);
    m_lista->verticalHeader()->hide();
    m_lista->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_lista->setSelectionMode(QAbstractItemView::SingleSelection);
    m_lista->setToolTip(tr("Doble clic en el nombre para cambiarlo"));
    connect(m_lista, &QTableWidget::itemSelectionChanged, this, &DialogoAnteriores::mostrarExamen);
    connect(m_lista, &QTableWidget::itemChanged, this, &DialogoAnteriores::nombreCambiado);

    auto *bPdf = new QPushButton(tr("Añadir PDF…"));
    bPdf->setEnabled(ExamenesAnteriores::puedeLeerPdf());
    if (!bPdf->isEnabled())
        bPdf->setToolTip(tr("Esta versión del programa no lee PDF: copie el texto del examen y péguelo"));
    auto *bPegar = new QPushButton(tr("Pegar texto…"));
    auto *bQuitar = new QPushButton(tr("Quitar"));
    connect(bPdf, &QPushButton::clicked, this, &DialogoAnteriores::anadirPdf);
    connect(bPegar, &QPushButton::clicked, this, &DialogoAnteriores::pegarTexto);
    connect(bQuitar, &QPushButton::clicked, this, &DialogoAnteriores::quitarExamen);
    auto *botonesLista = new QHBoxLayout;
    botonesLista->addWidget(bPdf);
    botonesLista->addWidget(bPegar);
    botonesLista->addWidget(bQuitar);
    botonesLista->addStretch();
    auto *izquierda = new QWidget;
    auto *li = new QVBoxLayout(izquierda);
    li->setContentsMargins(0, 0, 0, 0);
    li->addWidget(m_lista, 1);
    li->addLayout(botonesLista);

    m_preguntas = new QTableWidget(0, 4);
    //: Número de la pregunta en el examen
    m_preguntas->setHorizontalHeaderLabels({tr("Nº"), tr("Código"), tr("Parecido"),
                                            tr("En el examen")});
    m_preguntas->horizontalHeader()->setSectionResizeMode(ColTexto, QHeaderView::Stretch);
    m_preguntas->verticalHeader()->hide();
    m_preguntas->setWordWrap(false);
    m_preguntas->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed
                                 | QAbstractItemView::AnyKeyPressed);
    connect(m_preguntas, &QTableWidget::itemChanged, this, &DialogoAnteriores::codigoCambiado);

    auto *divisor = new QSplitter;
    divisor->addWidget(izquierda);
    divisor->addWidget(m_preguntas);
    divisor->setStretchFactor(1, 1);
    divisor->setSizes({360, 740});

    m_resumen = new QLabel;
    m_resumen->setWordWrap(true);

    // Botones con texto propio: los estándar vuelven al inglés si no están las traducciones de Qt.
    auto *botones = new QDialogButtonBox;
    botones->addButton(tr("Registrar"), QDialogButtonBox::AcceptRole)->setDefault(true);
    botones->addButton(tr("Cancelar"), QDialogButtonBox::RejectRole);
    connect(botones, &QDialogButtonBox::accepted, this, &DialogoAnteriores::accept);
    connect(botones, &QDialogButtonBox::rejected, this, &DialogoAnteriores::reject);
    DialogoAyuda::ponerAyuda(this, botones, QStringLiteral("anteriores"));

    auto *l = new QVBoxLayout(this);
    l->addWidget(explicacion);
    l->addLayout(filaConvocatoria);
    l->addWidget(divisor, 1);
    l->addWidget(m_resumen);
    l->addWidget(botones);
    actualizarExamen(-1);
}

QString DialogoAnteriores::convocatoriaYAnio() const
{
    DatosConvocatoria d;
    d.convocatoria = m_convocatoria->currentText().trimmed();
    d.anio = m_anio->value();
    return d.convocatoriaYAnio();
}

bool DialogoAnteriores::anadirExamen(const QString &texto, const QString &fichero)
{
    Examen e;
    e.fuente = fichero.isEmpty() ? tr("texto pegado") : QFileInfo(fichero).fileName();
    e.preguntas = ExamenesAnteriores::reconocer(ExamenesAnteriores::separar(texto), m_banco);
    if (e.preguntas.isEmpty()) {
        //: «1.-», «2.-» y «a)»…«d)» son lo que se busca en el texto del examen: se dejan tal cual
        QMessageBox::warning(this, windowTitle(),
                             tr("No se ha encontrado ninguna pregunta en %1. Deben ir numeradas «1.-», «2.-»… "
                                "al principio de una línea, con sus opciones «a)»…«d)».").arg(e.fuente));
        return false;
    }
    const ExamenesAnteriores::DatosFichero d =
        fichero.isEmpty() ? ExamenesAnteriores::DatosFichero() : ExamenesAnteriores::datosDeFichero(fichero, m_conocidos);
    e.nombre = d.nombre.isEmpty() ? tr("Examen %1").arg(m_examenes.size() + 1) : d.nombre;
    if (m_examenes.isEmpty()) {
        if (!d.convocatoria.isEmpty())
            m_convocatoria->setCurrentText(d.convocatoria);
        if (d.anio > 0)
            m_anio->setValue(d.anio);
    }
    m_examenes << e;

    m_rellenando = true;
    const int fila = m_lista->rowCount();
    m_lista->insertRow(fila);
    auto *nombre = new QTableWidgetItem(e.nombre);
    nombre->setToolTip(e.fuente);
    m_lista->setItem(fila, ColNombre, nombre);
    for (int c : {ColNumPreguntas, ColDudosas}) {
        auto *item = new QTableWidgetItem;
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setTextAlignment(Qt::AlignCenter);
        m_lista->setItem(fila, c, item);
    }
    m_rellenando = false;
    actualizarExamen(fila);
    m_lista->selectRow(fila);
    return true;
}

void DialogoAnteriores::anadirPdf()
{
    QSettings ajustes;
    const QStringList rutas = QFileDialog::getOpenFileNames(this, tr("Exámenes anteriores (PDF)"),
                                                            ajustes.value(QLatin1String(kClaveCarpeta)).toString(),
                                                            tr("PDF (*.pdf)"));
    if (rutas.isEmpty())
        return;
    ajustes.setValue(QLatin1String(kClaveCarpeta), QFileInfo(rutas.first()).absolutePath());
    for (const QString &ruta : rutas) {
        QString error;
        const QString texto = ExamenesAnteriores::textoDePdf(ruta, &error);
        if (texto.isEmpty())
            QMessageBox::warning(this, windowTitle(), QStringLiteral("%1\n\n%2").arg(QFileInfo(ruta).fileName(), error));
        else
            anadirExamen(texto, ruta);
    }
}

void DialogoAnteriores::pegarTexto()
{
    QInputDialog d(this);
    d.setWindowTitle(tr("Pegar el texto de un examen"));
    d.setLabelText(tr("Abra el examen (en Word o en un visor de PDF), seleccione todo su texto, cópielo y péguelo aquí:"));
    d.setOption(QInputDialog::UsePlainTextEditForTextInput);
    d.setOkButtonText(tr("Añadir"));
    d.setCancelButtonText(tr("Cancelar"));
    d.resize(720, 520);
    if (d.exec() == QDialog::Accepted && !d.textValue().trimmed().isEmpty())
        anadirExamen(d.textValue());
}

void DialogoAnteriores::quitarExamen()
{
    const int fila = m_lista->currentRow();
    if (fila < 0 || fila >= m_examenes.size())
        return;
    m_examenes.removeAt(fila);
    m_lista->removeRow(fila);
    mostrarExamen();
    actualizarExamen(-1);
}

void DialogoAnteriores::mostrarExamen()
{
    const int e = m_lista->currentRow();
    m_rellenando = true;
    m_preguntas->setRowCount(e >= 0 && e < m_examenes.size() ? int(m_examenes.at(e).preguntas.size()) : 0);
    m_rellenando = false;
    for (int i = 0; i < m_preguntas->rowCount(); ++i)
        ponerFila(i);
    m_preguntas->resizeColumnToContents(ColNumero);
    m_preguntas->resizeColumnToContents(ColParecido);
    m_preguntas->setColumnWidth(ColCodigo, qMax(90, m_preguntas->columnWidth(ColCodigo)));
}

void DialogoAnteriores::ponerFila(int fila)
{
    const Reconocida &r = m_examenes.at(m_lista->currentRow()).preguntas.at(fila);
    const Pregunta &p = r.pregunta.pregunta;
    QString parecido;
    if (p.id == 0)
        parecido = tr("no está en el banco");
    else if (r.parecido < 0)
        //: En la columna «Parecido»: el código de la pregunta se ha escrito a mano
        parecido = tr("a mano");
    else
        parecido = QStringLiteral("%1 %").arg(qRound(r.parecido * 100));
    QStringList enElExamen{r.leida.enunciado};
    for (int i = 0; i < 4; ++i)
        enElExamen << QStringLiteral("%1) %2").arg(Pregunta::letra(i)).arg(r.leida.opciones.value(i));

    m_rellenando = true;
    const QStringList textos{QString::number(fila + 1), p.id ? p.codigoCompleto() : QString(), parecido, r.leida.enunciado};
    for (int c = 0; c < textos.size(); ++c) {
        auto *item = m_preguntas->item(fila, c);
        if (!item) {
            item = new QTableWidgetItem;
            if (c != ColCodigo)
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            m_preguntas->setItem(fila, c, item);
        }
        item->setText(textos.at(c));
        item->setBackground(r.dudosa ? QBrush(kFondoDudosa) : QBrush());
        item->setForeground(r.dudosa ? QBrush(kTextoDudosa) : QBrush());
    }
    //: T1-7a es un ejemplo de código de pregunta: se escribe igual en cualquier idioma
    m_preguntas->item(fila, ColCodigo)->setToolTip(
        p.id ? tr("En el banco:\n%1\n\nDoble clic para escribir otro código (T1-7a)").arg(p.enunciado)
             : tr("Doble clic para escribir el código de la pregunta (T1-7a)"));
    m_preguntas->item(fila, ColTexto)->setToolTip(enElExamen.join(QLatin1Char('\n')));
    QString porQue;
    if (r.dudosa && p.id != 0)
        porQue = r.parecido < ExamenesAnteriores::kMinimoReconocida
                     ? tr("Se parece poco: compruebe que es esta pregunta.")
                     : tr("Se parece casi lo mismo a %1 (%2 %): compruebe cuál es.").arg(r.otra).arg(qRound(r.parecidoOtra * 100));
    m_preguntas->item(fila, ColParecido)->setToolTip(porQue);
    m_rellenando = false;
}

void DialogoAnteriores::codigoCambiado(QTableWidgetItem *item)
{
    const int e = m_lista->currentRow();
    if (m_rellenando || item->column() != ColCodigo || e < 0 || e >= m_examenes.size())
        return;
    Reconocida &r = m_examenes[e].preguntas[item->row()];
    const QString codigo = item->text().trimmed();
    r.pregunta = PreguntaExamen();
    r.parecido = 0;
    r.dudosa = true;
    for (const Pregunta &p : std::as_const(m_banco)) {
        if (p.codigoCompleto().compare(codigo, Qt::CaseInsensitive) == 0) {
            r.pregunta.pregunta = p;
            r.pregunta.orden = ExamenesAnteriores::ordenDeOpciones(r.leida, p);
            r.parecido = -1;   // puesta a mano
            r.dudosa = false;
            break;
        }
    }
    ponerFila(item->row());
    actualizarExamen(e);
}

void DialogoAnteriores::nombreCambiado(QTableWidgetItem *item)
{
    if (!m_rellenando && item->column() == ColNombre && item->row() < m_examenes.size())
        m_examenes[item->row()].nombre = item->text().trimmed();
}

void DialogoAnteriores::actualizarExamen(int examen)
{
    if (examen >= 0 && examen < m_examenes.size()) {
        m_rellenando = true;
        const QVector<Reconocida> &preguntas = m_examenes.at(examen).preguntas;
        m_lista->item(examen, ColNumPreguntas)->setText(QString::number(preguntas.size()));
        const int n = dudosas(preguntas);
        m_lista->item(examen, ColDudosas)->setText(n ? QString::number(n) : QString());
        for (int c = 0; c < m_lista->columnCount(); ++c) {
            m_lista->item(examen, c)->setBackground(n ? QBrush(kFondoDudosa) : QBrush());
            m_lista->item(examen, c)->setForeground(n ? QBrush(kTextoDudosa) : QBrush());
        }
        m_rellenando = false;
    }
    int preguntas = 0, total = 0;
    for (const Examen &e : std::as_const(m_examenes)) {
        preguntas += int(e.preguntas.size());
        total += dudosas(e.preguntas);
    }
    //: «Añadir PDF…» y «Pegar texto…» son botones de este diálogo: tradúzcanse igual
    if (m_examenes.isEmpty())
        m_resumen->setText(tr("Añada los exámenes de la convocatoria con «Añadir PDF…» (varios a la vez) o «Pegar texto…»."));
    else
        m_resumen->setText(tr("%1 exámenes, %2 preguntas. %3")
                               .arg(m_examenes.size()).arg(preguntas)
                               .arg(total == 0 ? tr("Todas reconocidas sin dudas.")
                                               : tr("Dudosas: %1 (en amarillo); compruébelas y, si hace falta, "
                                                    "escriba su código.").arg(total)));
}

void DialogoAnteriores::accept()
{
    QString error;
    const QString convocatoria = m_convocatoria->currentText().trimmed();
    QSet<QString> nombres;
    QStringList sinCodigo;
    for (const Examen &e : std::as_const(m_examenes)) {
        if (e.nombre.isEmpty() && error.isEmpty())
            error = tr("Hay un examen sin nombre.");
        else if (nombres.contains(e.nombre) && error.isEmpty())
            error = tr("Hay dos exámenes con el mismo nombre: %1.").arg(e.nombre);
        nombres.insert(e.nombre);
        for (int i = 0; i < e.preguntas.size(); ++i)
            if (e.preguntas.at(i).pregunta.pregunta.id == 0)
                sinCodigo << tr("«%1», pregunta %2").arg(e.nombre).arg(i + 1);
    }
    if (convocatoria.isEmpty())
        error = tr("Indique la convocatoria (Febrero, Junio…).");
    else if (m_examenes.isEmpty())
        error = tr("Añada los exámenes de la convocatoria.");
    else if (error.isEmpty() && !sinCodigo.isEmpty())
        error = tr("Escriba el código de estas preguntas, que no se han reconocido:\n%1").arg(sinCodigo.join(QLatin1Char('\n')));
    if (!error.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), error);
        return;
    }

    QStringList yaRegistrados;
    for (const ExamenRegistrado &e : m_bd->examenesRegistrados())
        if (e.datos.anio == m_anio->value() && e.datos.convocatoria == convocatoria && nombres.contains(e.nombre))
            yaRegistrados << e.nombre;
    int numDudosas = 0;
    for (const Examen &e : std::as_const(m_examenes))
        numDudosas += dudosas(e.preguntas);
    QString texto = tr("Se registrarán en el historial %1 exámenes de %2. Las preguntas se guardan tal como "
                       "están ahora en el banco.").arg(m_examenes.size()).arg(convocatoriaYAnio());
    if (numDudosas > 0)
        texto += tr("\n\nQuedan %1 preguntas dudosas, en amarillo.").arg(numDudosas);
    if (!yaRegistrados.isEmpty())
        texto += tr("\n\nYa había registrados con el mismo nombre, y se sustituirán: %1.").arg(yaRegistrados.join(QStringLiteral(", ")));
    if (!confirmar(this, windowTitle(), texto + tr("\n\n¿Registrarlos?")))
        return;

    QVector<Modelo> modelos;
    for (const Examen &e : std::as_const(m_examenes)) {
        Modelo m;
        m.nombre = e.nombre;
        for (const Reconocida &r : e.preguntas)
            m.preguntas << r.pregunta;
        modelos << m;
    }
    // La puntuación de siempre: 10 puntos en total y cada fallo resta el 30 %.
    DatosConvocatoria datos = m_datos;
    datos.convocatoria = convocatoria;
    datos.anio = m_anio->value();
    datos.puntosAcierto = 10.0 / modelos.first().preguntas.size();
    datos.puntosFallo = qRound(datos.puntosAcierto * 0.3 * 100) / 100.0;
    if (!m_bd->registrarExamenes(modelos, datos, 0)) {
        QMessageBox::warning(this, windowTitle(), tr("No se pudieron registrar:\n%1").arg(m_bd->ultimoError()));
        return;
    }
    QDialog::accept();
}
