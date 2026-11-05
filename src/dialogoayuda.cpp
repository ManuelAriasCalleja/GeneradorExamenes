#include "dialogoayuda.h"
#include "idioma.h"
#include "utilidades.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

DialogoAyuda::DialogoAyuda(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Ayuda del generador de exámenes"));
    setAttribute(Qt::WA_DeleteOnClose);
    resize(1000, 720);

    m_buscar = new QLineEdit;
    m_buscar->setPlaceholderText(tr("Buscar en la ayuda…"));
    m_buscar->setClearButtonEnabled(true);
    //: «Intro» es la tecla Intro (Entrar)
    m_buscar->setToolTip(tr("Intro: siguiente vez que aparece"));
    connect(m_buscar, &QLineEdit::textEdited, this, [this]() { buscar(false); });
    connect(m_buscar, &QLineEdit::returnPressed, this, [this]() { buscar(true); });
    auto *fila = new QHBoxLayout;
    fila->addWidget(new QLabel(tr("Buscar:")));
    fila->addWidget(m_buscar, 1);

    m_texto = new QTextBrowser;
    m_texto->setOpenExternalLinks(true);
    m_texto->setSource(QUrl(Idioma::ayuda(Idioma::interfaz())));
    m_indice = new QTreeWidget;
    m_indice->setHeaderHidden(true);
    rellenarIndice();
    connect(m_indice, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *item) {
        if (item)
            m_texto->scrollToAnchor(item->data(0, Qt::UserRole).toString());
    });

    auto *divisor = new QSplitter;
    divisor->addWidget(m_indice);
    divisor->addWidget(m_texto);
    divisor->setStretchFactor(1, 1);
    divisor->setSizes({280, 720});

    // Botón con texto propio: los estándar vuelven al inglés si no están las traducciones de Qt.
    auto *botones = new QDialogButtonBox;
    botones->addButton(tr("Cerrar"), QDialogButtonBox::RejectRole);
    connect(botones, &QDialogButtonBox::rejected, this, &DialogoAyuda::reject);

    auto *l = new QVBoxLayout(this);
    l->addLayout(fila);
    l->addWidget(divisor, 1);
    l->addWidget(botones);

    connect(new QShortcut(QKeySequence::Find, this), &QShortcut::activated, this, [this]() {
        m_buscar->setFocus();
        m_buscar->selectAll();
    });
    connect(new QShortcut(QKeySequence::FindNext, this), &QShortcut::activated, this, [this]() { buscar(true); });
}

DialogoAyuda *DialogoAyuda::mostrar(QWidget *desde, const QString &seccion)
{
    static QPointer<DialogoAyuda> abierta;
    QWidget *ventana = desde ? desde->window() : nullptr;
    if (abierta && abierta->parentWidget() != ventana) {
        // Se sustituye por otra, hija de la ventana nueva, en el mismo sitio.
        const QRect sitio = abierta->geometry();
        delete abierta;
        abierta = new DialogoAyuda(ventana);
        abierta->setGeometry(sitio);
    }
    if (!abierta)
        abierta = new DialogoAyuda(ventana);
    abierta->irA(seccion);
    abierta->show();
    abierta->raise();
    abierta->activateWindow();
    return abierta;
}

void DialogoAyuda::ponerAyuda(QDialog *dialogo, QDialogButtonBox *botones, const QString &seccion)
{
    auto abrir = [dialogo, seccion]() { mostrar(dialogo, seccion); };
    connect(new QShortcut(QKeySequence::HelpContents, dialogo), &QShortcut::activated, dialogo, abrir);
    if (botones)
        connect(botones->addButton(tr("Ayuda"), QDialogButtonBox::HelpRole), &QPushButton::clicked, dialogo, abrir);
}

// El índice sale de los títulos del manual: los capítulos (h2) y, dentro, sus apartados (h3). Cada título lleva el
// ancla a la que se salta.
void DialogoAyuda::rellenarIndice()
{
    QTreeWidgetItem *capitulo = nullptr;
    for (QTextBlock b = m_texto->document()->begin(); b.isValid(); b = b.next()) {
        const int nivel = b.blockFormat().headingLevel();
        if (nivel != 2 && nivel != 3)
            continue;
        QString ancla;
        for (auto it = b.begin(); !it.atEnd() && ancla.isEmpty(); ++it)
            ancla = it.fragment().charFormat().anchorNames().value(0);
        QTreeWidgetItem *item = nivel == 2 || !capitulo ? new QTreeWidgetItem(m_indice) : new QTreeWidgetItem(capitulo);
        if (nivel == 2)
            capitulo = item;
        item->setText(0, b.text().trimmed());
        item->setData(0, Qt::UserRole, ancla);
    }
}

bool DialogoAyuda::irA(const QString &seccion)
{
    for (QTreeWidgetItemIterator it(m_indice); *it; ++it) {
        if ((*it)->data(0, Qt::UserRole).toString() == seccion) {
            m_indice->setCurrentItem(*it);
            m_indice->scrollToItem(*it);
            // Por si ya estaba elegido: el texto puede haberse movido después.
            m_texto->scrollToAnchor(seccion);
            return true;
        }
    }
    return false;
}

QString DialogoAyuda::seccionActual() const
{
    const QTreeWidgetItem *item = m_indice->currentItem();
    return item ? item->data(0, Qt::UserRole).toString() : QString();
}

QStringList DialogoAyuda::secciones() const
{
    QStringList res;
    for (QTreeWidgetItemIterator it(m_indice); *it; ++it)
        res << (*it)->data(0, Qt::UserRole).toString();
    return res;
}

void DialogoAyuda::buscar(bool siguiente)
{
    // Sin distinguir tildes ni mayúsculas: se busca en el texto del manual tal como lo da paraBuscar(), que tiene las
    // mismas posiciones que el documento.
    const QString texto = paraBuscar(m_buscar->text());
    const QTextCursor cursor = m_texto->textCursor();
    qsizetype encontrado = 0;
    QTextCursor sinMarca = cursor;
    sinMarca.setPosition(cursor.selectionStart());
    if (!texto.isEmpty()) {
        if (m_manual.isEmpty())
            m_manual = paraBuscar(m_texto->document()->toRawText());
        const qsizetype desde = siguiente ? cursor.selectionEnd() : cursor.selectionStart();
        encontrado = m_manual.indexOf(texto, desde);
        if (encontrado < 0)   // desde el principio
            encontrado = m_manual.indexOf(texto);
        if (encontrado >= 0) {
            QTextCursor marca(m_texto->document());
            marca.setPosition(int(encontrado));
            marca.setPosition(int(encontrado + texto.size()), QTextCursor::KeepAnchor);
            m_texto->setTextCursor(marca);
            m_texto->ensureCursorVisible();
        } else if (!siguiente) {   // lo escrito no está: no queda marcado lo de antes
            m_texto->setTextCursor(sinMarca);
        }
    } else {
        m_texto->setTextCursor(sinMarca);
    }
    m_buscar->setStyleSheet(encontrado >= 0 ? QString() : QStringLiteral("background: #f8d7da; color: #58151c"));
}
