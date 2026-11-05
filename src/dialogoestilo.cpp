#include "dialogoestilo.h"
#include "dialogoayuda.h"
#include "estilo.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

DialogoEstilo::DialogoEstilo(std::function<QVector<Pregunta>()> preguntas, std::function<bool(int)> editar,
                             const QString &banco, const QVector<Tema> &temas, const QString &idioma, QWidget *parent)
    : QDialog(parent), m_preguntas(std::move(preguntas)), m_editar(std::move(editar)), m_banco(banco), m_temas(temas),
      m_idioma(idioma)
{
    setWindowTitle(tr("Revisión de estilo del banco"));
    resize(760, 560);

    m_parte = new QComboBox;
    m_parte->addItem(tr("Todos los avisos"), -1);
    for (Estilo::Aviso::Parte parte : {Estilo::Aviso::Enunciado, Estilo::Aviso::Opciones, Estilo::Aviso::Referencia})
        m_parte->addItem(Estilo::nombreParte(parte), int(parte));
    m_todas = new QCheckBox(tr("Incluir las anuladas y obsoletas"));
    connect(m_parte, &QComboBox::currentIndexChanged, this, &DialogoEstilo::rellenar);
    connect(m_todas, &QCheckBox::toggled, this, &DialogoEstilo::rellenar);
    auto *filtros = new QHBoxLayout;
    filtros->addWidget(new QLabel(tr("Mostrar:")));
    filtros->addWidget(m_parte);
    filtros->addSpacing(12);
    filtros->addWidget(m_todas);
    filtros->addStretch();

    m_resumen = new QLabel;
    m_arbol = new QTreeWidget;
    m_arbol->setHeaderLabels({tr("Pregunta"), tr("Aviso")});
    m_arbol->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_arbol->setWordWrap(true);
    connect(m_arbol, &QTreeWidget::itemDoubleClicked, this, &DialogoEstilo::editarSeleccionada);

    auto *bEditar = new QPushButton(tr("Editar la pregunta…"));
    auto *bGuardar = new QPushButton(tr("Guardar el informe…"));
    connect(bEditar, &QPushButton::clicked, this, &DialogoEstilo::editarSeleccionada);
    connect(bGuardar, &QPushButton::clicked, this, &DialogoEstilo::guardar);
    auto *cerrar = new QDialogButtonBox;
    cerrar->addButton(tr("Cerrar"), QDialogButtonBox::RejectRole);
    connect(cerrar, &QDialogButtonBox::rejected, this, &DialogoEstilo::reject);
    DialogoAyuda::ponerAyuda(this, cerrar, QStringLiteral("estilo"));
    auto *botones = new QHBoxLayout;
    botones->addWidget(bEditar);
    botones->addWidget(bGuardar);
    botones->addStretch();
    botones->addWidget(cerrar);

    auto *l = new QVBoxLayout(this);
    l->addLayout(filtros);
    l->addWidget(m_resumen);
    l->addWidget(m_arbol, 1);
    l->addLayout(botones);
    rellenar();
}

void DialogoEstilo::rellenar()
{
    // Se mantiene la pregunta elegida al volver de editarla.
    const QTreeWidgetItem *actual = m_arbol->currentItem();
    const int idActual = actual ? (actual->parent() ? actual->parent() : actual)->data(0, Qt::UserRole).toInt() : 0;

    m_arbol->clear();
    const int parte = m_parte->currentData().toInt();
    int revisadas = 0, conAvisos = 0, total = 0;
    for (const Pregunta &p : m_preguntas()) {
        if (!m_todas->isChecked() && !p.activa())
            continue;
        ++revisadas;
        QStringList avisos;
        for (const Estilo::Aviso &a : Estilo::revisar(p, m_temas, m_idioma))
            if (parte < 0 || int(a.parte) == parte)
                avisos << a.texto;
        if (avisos.isEmpty())
            continue;
        ++conAvisos;
        total += avisos.size();
        auto *pregunta = new QTreeWidgetItem(m_arbol, {p.codigoCompleto(), avisos.size() == 1 ? avisos.first()
                                                                                                : tr("%1 avisos").arg(avisos.size())});
        pregunta->setData(0, Qt::UserRole, p.id);
        QFont negrita = pregunta->font(0);
        negrita.setBold(true);
        pregunta->setFont(0, negrita);
        if (avisos.size() > 1) {
            for (const QString &a : avisos)
                new QTreeWidgetItem(pregunta, {QString(), a});
            pregunta->setExpanded(true);
        }
        if (p.id == idActual)
            m_arbol->setCurrentItem(pregunta);
    }
    m_resumen->setText(conAvisos == 0 ? tr("Ninguna de las %1 preguntas tiene avisos.").arg(revisadas)
                                      : tr("Con avisos: %1 de %2 preguntas (%3 avisos). Doble clic para editar una.")
                                            .arg(conAvisos).arg(revisadas).arg(total));
}

void DialogoEstilo::editarSeleccionada()
{
    const QTreeWidgetItem *item = m_arbol->currentItem();
    if (!item)
        return;
    if (item->parent())
        item = item->parent();
    int posicion = m_arbol->indexOfTopLevelItem(const_cast<QTreeWidgetItem *>(item));
    int id = item->data(0, Qt::UserRole).toInt();
    // Con «Guardar y siguiente con avisos», la siguiente de la lista: detrás de la guardada, si sigue con avisos, o
    // en su sitio, si ya no los tiene y ha salido de la lista.
    while (m_editar(id)) {
        rellenar();
        const QTreeWidgetItem *misma = m_arbol->topLevelItem(posicion);
        if (misma && misma->data(0, Qt::UserRole).toInt() == id)
            ++posicion;
        QTreeWidgetItem *siguiente = m_arbol->topLevelItem(posicion);
        if (!siguiente || siguiente->data(0, Qt::UserRole).toInt() == 0) {
            QMessageBox::information(this, windowTitle(), tr("No hay más preguntas con avisos en la lista."));
            break;
        }
        m_arbol->setCurrentItem(siguiente);
        id = siguiente->data(0, Qt::UserRole).toInt();
    }
    rellenar();
}

void DialogoEstilo::guardar()
{
    const QString ruta = QFileDialog::getSaveFileName(
        this, tr("Guardar el informe de estilo"),
        QFileInfo(m_banco).absolutePath() + QStringLiteral("/estilo_%1.txt").arg(QFileInfo(m_banco).completeBaseName()),
        tr("Texto (*.txt)"));
    if (ruta.isEmpty())
        return;
    QString texto = tr("Revisión de estilo del banco %1 (%2)\n%3\n\n")
                        .arg(QDir::toNativeSeparators(m_banco), QDate::currentDate().toString(Qt::ISODate), m_resumen->text());
    for (int i = 0; i < m_arbol->topLevelItemCount(); ++i) {
        const QTreeWidgetItem *pregunta = m_arbol->topLevelItem(i);
        texto += pregunta->text(0) + QLatin1Char('\n');
        if (pregunta->childCount() == 0)
            texto += QStringLiteral("  - ") + pregunta->text(1) + QLatin1Char('\n');
        for (int j = 0; j < pregunta->childCount(); ++j)
            texto += QStringLiteral("  - ") + pregunta->child(j)->text(1) + QLatin1Char('\n');
    }
    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::warning(this, windowTitle(), tr("No se pudo escribir %1").arg(QDir::toNativeSeparators(ruta)));
        return;
    }
    f.write(texto.toUtf8());
}
