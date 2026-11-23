#include "dialogoverificadores.h"
#include "basedatos.h"
#include "dialogoayuda.h"
#include "utilidades.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

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
