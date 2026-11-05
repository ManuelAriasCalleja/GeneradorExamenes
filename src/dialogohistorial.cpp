#include "dialogohistorial.h"
#include "basedatos.h"
#include "dialogoayuda.h"
#include "utilidades.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {
enum Columna { ColNombre, ColPreguntas, ColRegistrado };
}

DialogoHistorial::DialogoHistorial(BaseDatos *bd, QWidget *parent)
    : QDialog(parent), m_bd(bd)
{
    setWindowTitle(tr("Historial de convocatorias"));
    resize(620, 440);

    m_arbol = new QTreeWidget;
    m_arbol->setHeaderLabels({tr("Convocatoria / examen"), tr("Preguntas"), tr("Registrado")});
    m_arbol->header()->setSectionResizeMode(ColNombre, QHeaderView::Stretch);
    m_arbol->header()->setStretchLastSection(false);
    connect(m_arbol, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item) {
        if (item->parent())   // en una convocatoria, el doble clic solo la abre o la cierra
            accept();
    });

    auto *bEliminar = new QPushButton(tr("Eliminar del historial…"));
    connect(bEliminar, &QPushButton::clicked, this, &DialogoHistorial::eliminar);
    auto *botones = new QDialogButtonBox;
    QPushButton *cargar = botones->addButton(tr("Cargar"), QDialogButtonBox::AcceptRole);
    cargar->setDefault(true);
    botones->addButton(tr("Cerrar"), QDialogButtonBox::RejectRole);
    connect(botones, &QDialogButtonBox::accepted, this, &DialogoHistorial::accept);
    connect(botones, &QDialogButtonBox::rejected, this, &DialogoHistorial::reject);
    DialogoAyuda::ponerAyuda(this, botones, QStringLiteral("historial"));

    //: «Registrar convocatoria…» es una orden del menú Archivo de la ventana principal: tradúzcase igual
    auto *explicacion = new QLabel(tr(
        "Exámenes guardados con «Registrar convocatoria…». Cargue una convocatoria o un examen para verlo, "
        "exportarlo o imprimirlo tal como salió, aunque después se hayan corregido sus preguntas."));
    explicacion->setWordWrap(true);
    auto *fila = new QHBoxLayout;
    fila->addWidget(bEliminar);
    fila->addStretch();
    fila->addWidget(botones);
    auto *l = new QVBoxLayout(this);
    l->addWidget(explicacion);
    l->addWidget(m_arbol, 1);
    l->addLayout(fila);
    rellenar();
}

void DialogoHistorial::rellenar()
{
    m_arbol->clear();
    QTreeWidgetItem *convocatoria = nullptr;
    QString actual;
    for (const ExamenRegistrado &e : m_bd->examenesRegistrados()) {
        const QString nombre = e.datos.convocatoriaYAnio();
        if (!convocatoria || nombre != actual) {
            convocatoria = new QTreeWidgetItem(m_arbol, {nombre});
            QFont f = convocatoria->font(ColNombre);
            f.setBold(true);
            convocatoria->setFont(ColNombre, f);
            actual = nombre;
        }
        auto *examen = new QTreeWidgetItem(convocatoria, {e.nombre, QString::number(e.numPreguntas),
                                                          QLocale().toString(e.registrado, QLocale::ShortFormat)});
        examen->setData(ColNombre, Qt::UserRole, e.id);
        examen->setTextAlignment(ColPreguntas, Qt::AlignRight | Qt::AlignVCenter);
    }
    m_arbol->expandAll();
    m_arbol->resizeColumnToContents(ColPreguntas);
    m_arbol->resizeColumnToContents(ColRegistrado);
    if (m_arbol->topLevelItemCount() == 0) {
        //: «Archivo → Registrar exámenes anteriores…» es un menú de la ventana principal: tradúzcase igual
        auto *vacio = new QTreeWidgetItem(m_arbol, {tr("Todavía no hay ninguna convocatoria registrada. Las "
                                                       "anteriores al programa se registran con Archivo → "
                                                       "Registrar exámenes anteriores…")});
        vacio->setFlags(Qt::NoItemFlags);
    }
}

QVector<int> DialogoHistorial::examenesDe(const QTreeWidgetItem *item) const
{
    QVector<int> ids;
    if (!item)
        return ids;
    if (item->parent()) {
        ids << item->data(ColNombre, Qt::UserRole).toInt();
    } else {
        for (int i = 0; i < item->childCount(); ++i)
            ids << item->child(i)->data(ColNombre, Qt::UserRole).toInt();
    }
    return ids;
}

void DialogoHistorial::accept()
{
    m_elegidos = examenesDe(m_arbol->currentItem());
    if (m_elegidos.isEmpty()) {
        QMessageBox::information(this, windowTitle(), tr("Elija una convocatoria o un examen."));
        return;
    }
    QDialog::accept();
}

void DialogoHistorial::eliminar()
{
    const QTreeWidgetItem *item = m_arbol->currentItem();
    const QVector<int> ids = examenesDe(item);
    if (ids.isEmpty())
        return;
    const QString que = item->parent()
                            ? tr("el examen «%1» de %2").arg(item->text(ColNombre), item->parent()->text(ColNombre))
                            : ids.size() == 1 ? tr("la convocatoria %1 (un examen)").arg(item->text(ColNombre))
                            : tr("la convocatoria %1 (%2 exámenes)").arg(item->text(ColNombre)).arg(ids.size());
    if (!confirmar(this, windowTitle(),
                   tr("¿Eliminar del historial %1?\n\nSus preguntas dejarán de contar como usadas al generar "
                      "exámenes y no se podrá volver a cargarlo.").arg(que)))
        return;
    for (int id : ids) {
        if (!m_bd->borrarExamenRegistrado(id)) {
            QMessageBox::warning(this, windowTitle(), tr("No se pudo eliminar: %1").arg(m_bd->ultimoError()));
            break;
        }
    }
    rellenar();
}
