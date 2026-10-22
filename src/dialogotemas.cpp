#include "dialogotemas.h"
#include "basedatos.h"
#include "utilidades.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
enum { ColNumero, ColTitulo, ColAutor, ColPreguntas };
}

DialogoTemas::DialogoTemas(BaseDatos *bd, QWidget *parent)
    : QDialog(parent), m_bd(bd)
{
    setWindowTitle(QStringLiteral("Temas"));
    resize(640, 420);

    m_tabla = new QTableWidget(0, 4);
    m_tabla->setHorizontalHeaderLabels({QStringLiteral("Nº"), QStringLiteral("Título"), QStringLiteral("Autor"), QStringLiteral("Preguntas")});
    m_tabla->horizontalHeader()->setSectionResizeMode(ColTitulo, QHeaderView::Stretch);
    m_tabla->verticalHeader()->hide();
    m_tabla->setSelectionBehavior(QAbstractItemView::SelectRows);
    for (const Tema &t : bd->temas())
        anadirFila(t, bd->contarPreguntas(t.id));

    auto *bAnadir = new QPushButton(QStringLiteral("Añadir tema"));
    auto *bEliminar = new QPushButton(QStringLiteral("Eliminar tema"));
    connect(bAnadir, &QPushButton::clicked, this, &DialogoTemas::anadir);
    connect(bEliminar, &QPushButton::clicked, this, &DialogoTemas::eliminar);

    auto *botones = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    botones->button(QDialogButtonBox::Save)->setText(QStringLiteral("Guardar"));
    botones->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Cancelar"));
    connect(botones, &QDialogButtonBox::accepted, this, &DialogoTemas::accept);
    connect(botones, &QDialogButtonBox::rejected, this, &DialogoTemas::reject);

    auto *fila = new QHBoxLayout;
    fila->addWidget(bAnadir);
    fila->addWidget(bEliminar);
    fila->addStretch();
    auto *l = new QVBoxLayout(this);
    l->addWidget(new QLabel(QStringLiteral("Haga doble clic en una celda para modificarla.")));
    l->addWidget(m_tabla);
    l->addLayout(fila);
    l->addWidget(botones);
}

void DialogoTemas::anadirFila(const Tema &t, int numPreguntas)
{
    const int f = m_tabla->rowCount();
    m_tabla->insertRow(f);
    auto *num = new QTableWidgetItem;
    num->setData(Qt::EditRole, t.numero);
    num->setData(Qt::UserRole, t.id);
    m_tabla->setItem(f, ColNumero, num);
    m_tabla->setItem(f, ColTitulo, new QTableWidgetItem(t.titulo));
    m_tabla->setItem(f, ColAutor, new QTableWidgetItem(t.autor));
    auto *np = new QTableWidgetItem(QString::number(numPreguntas));
    np->setFlags(np->flags() & ~Qt::ItemIsEditable);
    m_tabla->setItem(f, ColPreguntas, np);
}

void DialogoTemas::anadir()
{
    int maximo = 0;
    for (int f = 0; f < m_tabla->rowCount(); ++f)
        maximo = qMax(maximo, m_tabla->item(f, ColNumero)->data(Qt::EditRole).toInt());
    Tema t;
    t.numero = maximo + 1;
    t.titulo = QStringLiteral("Nuevo tema");
    anadirFila(t, 0);
    m_tabla->editItem(m_tabla->item(m_tabla->rowCount() - 1, ColTitulo));
}

void DialogoTemas::eliminar()
{
    const int f = m_tabla->currentRow();
    if (f < 0)
        return;
    const int n = m_tabla->item(f, ColPreguntas)->text().toInt();
    if (n > 0 && !confirmar(this, windowTitle(),
                            QStringLiteral("El tema tiene %1 preguntas, que también se borrarán. ¿Continuar?").arg(n)))
        return;
    const int id = m_tabla->item(f, ColNumero)->data(Qt::UserRole).toInt();
    if (id)
        m_borrados << id;
    m_tabla->removeRow(f);
}

void DialogoTemas::accept()
{
    QSet<int> numeros;
    for (int f = 0; f < m_tabla->rowCount(); ++f) {
        const int n = m_tabla->item(f, ColNumero)->data(Qt::EditRole).toInt();
        if (n <= 0 || numeros.contains(n)) {
            QMessageBox::warning(this, windowTitle(), QStringLiteral("Los números de tema deben ser positivos y distintos."));
            return;
        }
        numeros.insert(n);
    }
    for (int id : m_borrados)
        m_bd->borrarTema(id);
    // Se renumeran en dos pasos para no chocar con la restricción UNIQUE si se intercambian números.
    QVector<Tema> temas;
    for (int f = 0; f < m_tabla->rowCount(); ++f) {
        Tema t;
        t.id = m_tabla->item(f, ColNumero)->data(Qt::UserRole).toInt();
        t.numero = m_tabla->item(f, ColNumero)->data(Qt::EditRole).toInt();
        t.titulo = m_tabla->item(f, ColTitulo)->text().trimmed();
        t.autor = m_tabla->item(f, ColAutor) ? m_tabla->item(f, ColAutor)->text().trimmed() : QString();
        temas << t;
    }
    for (Tema t : temas) {
        if (t.id) {
            t.numero = -t.numero;
            m_bd->guardarTema(t);
        }
    }
    for (Tema &t : temas) {
        if (!m_bd->guardarTema(t)) {
            QMessageBox::warning(this, windowTitle(), m_bd->ultimoError());
            return;
        }
    }
    QDialog::accept();
}
