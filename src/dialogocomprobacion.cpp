#include "dialogocomprobacion.h"
#include "dialogoayuda.h"
#include "utilidades.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

DialogoComprobacion::DialogoComprobacion(const QString &ruta, const QSet<QString> &grupos, Modo modo, QWidget *parent)
    : QDialog(parent), m_ruta(ruta), m_grupos(grupos), m_modo(modo)
{
    setWindowTitle(tr("Comprobar %1").arg(QFileInfo(ruta).fileName()));
    resize(900, 480);
    auto *l = new QVBoxLayout(this);

    m_resumen = new QLabel;
    m_resumen->setWordWrap(true);
    m_resumen->setTextInteractionFlags(Qt::TextSelectableByMouse);
    l->addWidget(m_resumen);

    m_lista = new QTreeWidget;
    //: Columnas de la lista de problemas de un JSON: gravedad (error o aviso), línea del fichero, pregunta y qué pasa
    m_lista->setHeaderLabels({tr("Tipo"), tr("Línea"), tr("Dónde"), tr("Problema")});
    m_lista->setRootIsDecorated(false);
    m_lista->setWordWrap(true);
    m_lista->setUniformRowHeights(false);
    m_lista->setAlternatingRowColors(true);
    m_lista->header()->setStretchLastSection(true);
    l->addWidget(m_lista, 1);

    m_botones = new QDialogButtonBox;
    auto *volver = m_botones->addButton(tr("Volver a comprobar"), QDialogButtonBox::ActionRole);
    volver->setToolTip(tr("Después de corregir el fichero en un editor de textos (y guardarlo)"));
    connect(volver, &QPushButton::clicked, this, &DialogoComprobacion::comprobar);
    connect(m_botones->addButton(tr("Abrir el fichero"), QDialogButtonBox::ActionRole), &QPushButton::clicked, this,
            &DialogoComprobacion::abrirFichero);
    connect(m_botones->addButton(tr("Copiar el informe"), QDialogButtonBox::ActionRole), &QPushButton::clicked, this,
            &DialogoComprobacion::copiar);
    if (m_modo == AntesDeImportar) {
        m_importar = m_botones->addButton(tr("Importar de todas formas"), QDialogButtonBox::AcceptRole);
        m_botones->addButton(tr("Cancelar"), QDialogButtonBox::RejectRole);
    } else {
        m_botones->addButton(tr("Cerrar"), QDialogButtonBox::RejectRole);
    }
    connect(m_botones, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_botones, &QDialogButtonBox::rejected, this, &QDialog::reject);
    DialogoAyuda::ponerAyuda(this, m_botones, QStringLiteral("comprobarjson"));
    l->addWidget(m_botones);

    comprobar();
}

void DialogoComprobacion::comprobar()
{
    m_resultado = ComprobadorJson::comprobarFichero(m_ruta, m_grupos);
    const int errores = m_resultado.errores();
    QString texto = QStringLiteral("<b>%1</b><br>%2").arg(QDir::toNativeSeparators(m_ruta).toHtmlEscaped(),
                                                         ComprobadorJson::resumen(m_resultado).toHtmlEscaped());
    if (m_modo == AntesDeImportar)
        texto += QStringLiteral("<br>")
                 + (errores > 0 ? tr("Con errores no se puede importar: corríjalos en el fichero y pulse «Volver a comprobar».")
                    : m_resultado.problemas.isEmpty() ? tr("Ya no tiene problemas: se puede importar.")
                                                      : tr("Los avisos no impiden importarlo."));
    m_resumen->setText(texto);
    m_resumen->setStyleSheet(QLatin1String(errores > 0 || !m_resultado.problemas.isEmpty() ? kEstiloAviso : kEstiloInformacion));

    m_lista->clear();
    const QIcon iconoError = style()->standardIcon(QStyle::SP_MessageBoxCritical);
    const QIcon iconoAviso = style()->standardIcon(QStyle::SP_MessageBoxWarning);
    for (const ComprobadorJson::Problema &p : std::as_const(m_resultado.problemas)) {
        auto *item = new QTreeWidgetItem(m_lista);
        const bool error = p.gravedad == ComprobadorJson::Problema::Error;
        item->setIcon(0, error ? iconoError : iconoAviso);
        item->setText(0, ComprobadorJson::nombreGravedad(p.gravedad));
        item->setText(1, p.linea > 0 ? QString::number(p.linea) : QString());
        item->setTextAlignment(1, Qt::AlignRight | Qt::AlignTop);
        item->setText(2, p.donde);
        item->setText(3, p.texto);
        item->setToolTip(3, p.texto);
    }
    for (int c = 0; c < 3; ++c)
        m_lista->resizeColumnToContents(c);
    if (m_importar) {
        m_importar->setEnabled(errores == 0);
        m_importar->setText(m_resultado.problemas.isEmpty() ? tr("Importar") : tr("Importar de todas formas"));
    }
}

void DialogoComprobacion::copiar()
{
    QApplication::clipboard()->setText(ComprobadorJson::informe(m_resultado, QDir::toNativeSeparators(m_ruta)));
}

void DialogoComprobacion::abrirFichero()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_ruta));
}
