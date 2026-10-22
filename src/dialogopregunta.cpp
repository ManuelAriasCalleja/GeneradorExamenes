#include "dialogopregunta.h"
#include "maquetador.h"

#include <QBuffer>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpressionValidator>
#include <QSplitter>
#include <QSpinBox>
#include <QTextBrowser>
#include <QTextDocument>
#include <QVBoxLayout>

DialogoPregunta::DialogoPregunta(const QVector<Tema> &temas, const Pregunta &pregunta, QWidget *parent)
    : QDialog(parent), m_temas(temas), m_original(pregunta), m_imagen(pregunta.imagen)
{
    setWindowTitle(pregunta.id ? QStringLiteral("Editar pregunta %1").arg(pregunta.codigoCompleto())
                               : QStringLiteral("Nueva pregunta"));
    resize(1150, 760);

    auto *formulario = new QWidget;
    auto *lf = new QVBoxLayout(formulario);
    lf->setContentsMargins(0, 0, 0, 0);

    // Identificación
    auto *gId = new QGroupBox(QStringLiteral("Identificación"));
    auto *fid = new QGridLayout(gId);
    m_tema = new QComboBox;
    for (const Tema &t : temas)
        m_tema->addItem(t.nombre(), t.id);
    m_tema->setCurrentIndex(qMax(0, m_tema->findData(pregunta.temaId)));
    m_numero = new QSpinBox;
    m_numero->setRange(1, 999);
    m_numero->setValue(qMax(1, pregunta.numero));
    m_numero->setToolTip(QStringLiteral("Las preguntas con el mismo número dentro de un tema son variantes "
                                        "(pseudorrepetidas) y nunca salen juntas en un examen."));
    m_variante = new QLineEdit(pregunta.variante);
    m_variante->setMaxLength(1);
    m_variante->setMaximumWidth(40);
    m_variante->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("[a-z]?")), m_variante));
    m_variante->setToolTip(QStringLiteral("Letra de la variante: a, b, c... (vacío si no tiene variantes)"));
    m_estado = new QComboBox;
    m_estado->addItem(QStringLiteral("Activa"), QStringLiteral("activa"));
    m_estado->addItem(QStringLiteral("Anulada"), QStringLiteral("anulada"));
    m_estado->addItem(QStringLiteral("Obsoleta"), QStringLiteral("obsoleta"));
    m_estado->setCurrentIndex(qMax(0, m_estado->findData(pregunta.estado)));
    m_old = new QCheckBox(QStringLiteral("Ya ha salido en exámenes (old)"));
    m_old->setChecked(pregunta.old);
    m_revisar = new QCheckBox(QStringLiteral("Marcar para revisar"));
    m_revisar->setChecked(pregunta.revisar);
    m_pseudo = new QLineEdit(pregunta.pseudoCon);
    m_pseudo->setPlaceholderText(QStringLiteral("p. ej. T1-7"));
    m_pseudo->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("(T\\d+-\\d+)?")), m_pseudo));
    m_pseudo->setToolTip(QStringLiteral("Pregunta equivalente de otro tema (tema-número). No saldrán juntas."));

    fid->addWidget(new QLabel(QStringLiteral("Tema:")), 0, 0);
    fid->addWidget(m_tema, 0, 1, 1, 5);
    fid->addWidget(new QLabel(QStringLiteral("Número:")), 1, 0);
    fid->addWidget(m_numero, 1, 1);
    fid->addWidget(new QLabel(QStringLiteral("Variante:")), 1, 2);
    fid->addWidget(m_variante, 1, 3);
    fid->addWidget(new QLabel(QStringLiteral("Estado:")), 1, 4);
    fid->addWidget(m_estado, 1, 5);
    fid->addWidget(m_old, 2, 0, 1, 3);
    fid->addWidget(m_revisar, 2, 3, 1, 3);
    fid->addWidget(new QLabel(QStringLiteral("Pseudorrepetida con:")), 3, 0, 1, 2);
    fid->addWidget(m_pseudo, 3, 2, 1, 4);
    lf->addWidget(gId);

    // Enunciado y opciones
    auto *gTexto = new QGroupBox(QStringLiteral("Enunciado y respuestas"));
    auto *ft = new QGridLayout(gTexto);
    m_enunciado = new QPlainTextEdit(pregunta.enunciado);
    m_enunciado->setTabChangesFocus(true);
    m_enunciado->setMinimumHeight(90);
    auto *ayuda = new QLabel(QStringLiteral("Código entre líneas <tt>```</tt>; filas de tabla como <tt>| celda | celda |</tt>."));
    ayuda->setStyleSheet(QStringLiteral("color: gray"));
    ft->addWidget(m_enunciado, 0, 0, 1, 3);
    ft->addWidget(ayuda, 1, 0, 1, 3);
    auto *grupoCorrecta = new QButtonGroup(this);
    for (int i = 0; i < 4; ++i) {
        m_correcta[i] = new QRadioButton(QStringLiteral("%1)").arg(Pregunta::letra(i)));
        m_correcta[i]->setToolTip(QStringLiteral("Marque la respuesta correcta"));
        grupoCorrecta->addButton(m_correcta[i], i);
        m_opciones[i] = new QLineEdit(pregunta.opciones.value(i));
        ft->addWidget(m_correcta[i], 2 + i, 0);
        ft->addWidget(m_opciones[i], 2 + i, 1, 1, 2);
    }
    m_correcta[qBound(0, pregunta.correcta, 3)]->setChecked(true);
    auto *nota = new QLabel(QStringLiteral("El botón marcado indica la respuesta correcta."));
    nota->setStyleSheet(QStringLiteral("color: gray"));
    ft->addWidget(nota, 6, 1, 1, 2);
    lf->addWidget(gTexto, 1);

    // Imagen
    auto *gImg = new QGroupBox(QStringLiteral("Imagen (opcional, se muestra bajo el enunciado)"));
    auto *li = new QHBoxLayout(gImg);
    m_vistaImagen = new QLabel;
    m_vistaImagen->setMinimumHeight(60);
    auto *bCargar = new QPushButton(QStringLiteral("Cargar…"));
    auto *bQuitar = new QPushButton(QStringLiteral("Quitar"));
    connect(bCargar, &QPushButton::clicked, this, &DialogoPregunta::cargarImagen);
    connect(bQuitar, &QPushButton::clicked, this, &DialogoPregunta::quitarImagen);
    li->addWidget(m_vistaImagen, 1);
    li->addWidget(bCargar);
    li->addWidget(bQuitar);
    lf->addWidget(gImg);

    // Otros datos
    auto *gOtros = new QGroupBox(QStringLiteral("Referencia y notas"));
    auto *fo = new QFormLayout(gOtros);
    m_referencia = new QLineEdit(pregunta.referencia);
    m_referencia->setPlaceholderText(QStringLiteral("Véase la página ... del libro de teoría."));
    m_explicacion = new QPlainTextEdit(pregunta.explicacion);
    m_explicacion->setMaximumHeight(60);
    m_explicacion->setTabChangesFocus(true);
    m_notas = new QLineEdit(pregunta.notas);
    fo->addRow(QStringLiteral("Referencia:"), m_referencia);
    fo->addRow(QStringLiteral("Explicación:"), m_explicacion);
    fo->addRow(QStringLiteral("Notas internas:"), m_notas);
    lf->addWidget(gOtros);

    // Vista previa
    auto *gPrevia = new QGroupBox(QStringLiteral("Vista previa"));
    auto *lp = new QVBoxLayout(gPrevia);
    m_vistaPrevia = new QTextBrowser;
    lp->addWidget(m_vistaPrevia);

    auto *divisor = new QSplitter;
    divisor->addWidget(formulario);
    divisor->addWidget(gPrevia);
    divisor->setStretchFactor(0, 3);
    divisor->setStretchFactor(1, 2);

    auto *botones = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    botones->button(QDialogButtonBox::Save)->setText(QStringLiteral("Guardar"));
    botones->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Cancelar"));
    connect(botones, &QDialogButtonBox::accepted, this, &DialogoPregunta::accept);
    connect(botones, &QDialogButtonBox::rejected, this, &DialogoPregunta::reject);

    auto *l = new QVBoxLayout(this);
    l->addWidget(divisor, 1);
    l->addWidget(botones);

    for (int i = 0; i < 4; ++i) {
        connect(m_opciones[i], &QLineEdit::textChanged, this, &DialogoPregunta::actualizarVistaPrevia);
        connect(m_correcta[i], &QRadioButton::toggled, this, &DialogoPregunta::actualizarVistaPrevia);
    }
    connect(m_enunciado, &QPlainTextEdit::textChanged, this, &DialogoPregunta::actualizarVistaPrevia);
    connect(m_referencia, &QLineEdit::textChanged, this, &DialogoPregunta::actualizarVistaPrevia);
    connect(m_explicacion, &QPlainTextEdit::textChanged, this, &DialogoPregunta::actualizarVistaPrevia);
    connect(m_numero, &QSpinBox::valueChanged, this, &DialogoPregunta::actualizarVistaPrevia);
    connect(m_variante, &QLineEdit::textChanged, this, &DialogoPregunta::actualizarVistaPrevia);
    connect(m_tema, &QComboBox::currentIndexChanged, this, &DialogoPregunta::actualizarVistaPrevia);

    mostrarImagen();
    actualizarVistaPrevia();
}

Pregunta DialogoPregunta::pregunta() const
{
    Pregunta p = m_original;
    p.temaId = m_tema->currentData().toInt();
    for (const Tema &t : m_temas)
        if (t.id == p.temaId)
            p.temaNumero = t.numero;
    p.numero = m_numero->value();
    p.variante = m_variante->text().trimmed().toLower();
    p.estado = m_estado->currentData().toString();
    p.old = m_old->isChecked();
    p.revisar = m_revisar->isChecked();
    p.pseudoCon = m_pseudo->text().trimmed().toUpper();
    p.enunciado = m_enunciado->toPlainText().trimmed();
    for (int i = 0; i < 4; ++i) {
        p.opciones[i] = m_opciones[i]->text().trimmed();
        if (m_correcta[i]->isChecked())
            p.correcta = i;
    }
    p.referencia = m_referencia->text().trimmed();
    p.explicacion = m_explicacion->toPlainText().trimmed();
    p.notas = m_notas->text().trimmed();
    p.imagen = m_imagen;
    return p;
}

void DialogoPregunta::accept()
{
    const Pregunta p = pregunta();
    QString error;
    if (p.temaId == 0)
        error = QStringLiteral("Elija un tema.");
    else if (p.enunciado.isEmpty())
        error = QStringLiteral("El enunciado está vacío.");
    for (int i = 0; i < 4 && error.isEmpty(); ++i)
        if (p.opciones.value(i).isEmpty())
            error = QStringLiteral("Falta el texto de la opción %1).").arg(Pregunta::letra(i));
    if (!error.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), error);
        return;
    }
    QDialog::accept();
}

void DialogoPregunta::actualizarVistaPrevia()
{
    const Pregunta p = pregunta();
    QString nombreTema = m_tema->currentText();
    // El documento inicial lo borra el propio visor al sustituirlo; los creados aquí (hijos del visor), no.
    QTextDocument *anterior = m_vistaPrevia->document();
    const bool propio = anterior && anterior->parent() == m_vistaPrevia;
    m_vistaPrevia->setDocument(Maquetador::crearDocumento(Maquetador::fichaPreguntaHtml(p, nombreTema), {p}, m_vistaPrevia));
    if (propio)
        anterior->deleteLater();
}

void DialogoPregunta::mostrarImagen()
{
    QImage img;
    if (!m_imagen.isEmpty() && img.loadFromData(m_imagen)) {
        m_vistaImagen->setPixmap(QPixmap::fromImage(img.scaledToHeight(qMin(img.height(), 80), Qt::SmoothTransformation)));
    } else {
        m_vistaImagen->setPixmap(QPixmap());
        m_vistaImagen->setText(QStringLiteral("(sin imagen)"));
    }
}

void DialogoPregunta::cargarImagen()
{
    const QString ruta = QFileDialog::getOpenFileName(this, QStringLiteral("Cargar imagen"), QString(),
                                                      QStringLiteral("Imágenes (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (ruta.isEmpty())
        return;
    QImage img(ruta);
    if (img.isNull()) {
        QMessageBox::warning(this, windowTitle(), QStringLiteral("No se pudo leer la imagen."));
        return;
    }
    QBuffer buf(&m_imagen);
    m_imagen.clear();
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    mostrarImagen();
    actualizarVistaPrevia();
}

void DialogoPregunta::quitarImagen()
{
    m_imagen.clear();
    mostrarImagen();
    actualizarVistaPrevia();
}
