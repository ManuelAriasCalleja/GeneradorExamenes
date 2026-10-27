#include "dialogoexportar.h"
#include "dialogoayuda.h"
#include "utilidades.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

const char *const kClaveCarpeta = "exportar/carpeta";
enum Columna { ColExamen = 0, ColFichero = 1 };

} // namespace

DialogoExportar::DialogoExportar(const QString &titulo, const QStringList &nombres, const QString &nombreBase,
                                 const QString &extension, QWidget *parent)
    : QDialog(parent), m_nombres(nombres), m_nombreBase(nombreBase), m_extension(extension), m_numModelos(nombres.size())
{
    setWindowTitle(titulo);
    resize(640, 320);

    auto *l = new QVBoxLayout(this);
    auto *fila = new QHBoxLayout;
    m_carpeta = new QLineEdit(QSettings().value(QLatin1String(kClaveCarpeta),
                                                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString());
    auto *elegir = new QPushButton(QStringLiteral("Elegir…"));
    connect(elegir, &QPushButton::clicked, this, &DialogoExportar::elegirCarpeta);
    fila->addWidget(new QLabel(QStringLiteral("Carpeta:")));
    fila->addWidget(m_carpeta, 1);
    fila->addWidget(elegir);
    l->addLayout(fila);

    l->addWidget(new QLabel(QStringLiteral("Marque los exámenes que quiere exportar y cambie el nombre de sus ficheros si lo desea:")));
    m_tabla = new QTableWidget(0, 2);
    m_tabla->setHorizontalHeaderLabels({QStringLiteral("Examen"), QStringLiteral("Fichero")});
    m_tabla->horizontalHeader()->setSectionResizeMode(ColExamen, QHeaderView::ResizeToContents);
    m_tabla->horizontalHeader()->setSectionResizeMode(ColFichero, QHeaderView::Stretch);
    m_tabla->verticalHeader()->hide();
    m_tabla->setSelectionMode(QAbstractItemView::NoSelection);
    m_tabla->setEditTriggers(QAbstractItemView::AllEditTriggers);
    auto anadirFila = [&](const QString &examen, bool marcado) {
        const int f = m_tabla->rowCount();
        m_tabla->insertRow(f);
        auto *marca = new QTableWidgetItem(examen);
        marca->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        marca->setCheckState(marcado ? Qt::Checked : Qt::Unchecked);
        m_tabla->setItem(f, ColExamen, marca);
        auto *fichero = new QTableWidgetItem(nombrePropuesto(f, false));
        fichero->setToolTip(QStringLiteral("Pulse sobre el nombre para cambiarlo"));
        m_tabla->setItem(f, ColFichero, fichero);
    };
    for (const QString &nombre : nombres)
        anadirFila(nombre, true);
    if (nombres.size() > 1)
        anadirFila(QStringLiteral("Todos los exámenes en un fichero"), false);
    l->addWidget(m_tabla, 1);

    auto *botones = new QDialogButtonBox;
    QPushButton *exportar = botones->addButton(QStringLiteral("Exportar"), QDialogButtonBox::AcceptRole);
    exportar->setDefault(true);
    botones->addButton(QStringLiteral("Cancelar"), QDialogButtonBox::RejectRole);
    connect(botones, &QDialogButtonBox::accepted, this, &DialogoExportar::accept);
    connect(botones, &QDialogButtonBox::rejected, this, &DialogoExportar::reject);
    DialogoAyuda::ponerAyuda(this, botones, QStringLiteral("exportar"));
    l->addWidget(botones);
}

// «<base>_<examen>.<extensión>» y, el de todos juntos, «<base>.<extensión>».
QString DialogoExportar::nombrePropuesto(int fila, bool soluciones) const
{
    const QString base = soluciones ? m_nombreBaseSoluciones : m_nombreBase;
    const QString nombre = fila < m_numModelos ? QStringLiteral("%1_%2").arg(base, limpiarNombreFichero(m_nombres.at(fila))) : base;
    return nombre + QLatin1Char('.') + m_extension;
}

void DialogoExportar::ofrecerSoluciones(const QString &texto, bool marcada, const QString &nombreBaseSoluciones)
{
    m_nombreBaseSoluciones = nombreBaseSoluciones;
    m_soluciones = new QCheckBox(texto);
    // Bajo la carpeta.
    if (auto *l = qobject_cast<QVBoxLayout *>(layout()))
        l->insertWidget(1, m_soluciones);
    m_soluciones->setChecked(marcada);
    cambiarNombres(marcada);
    connect(m_soluciones, &QCheckBox::toggled, this, &DialogoExportar::cambiarNombres);
}

bool DialogoExportar::conSoluciones() const
{
    return m_soluciones && m_soluciones->isChecked();
}

// Los nombres que siguen siendo los propuestos pasan a ser los de con (o sin) soluciones; los cambiados, no.
void DialogoExportar::cambiarNombres(bool soluciones)
{
    for (int f = 0; f < m_tabla->rowCount(); ++f) {
        QTableWidgetItem *item = m_tabla->item(f, ColFichero);
        if (item->text() == nombrePropuesto(f, !soluciones))
            item->setText(nombrePropuesto(f, soluciones));
    }
}

QString DialogoExportar::carpeta() const
{
    return m_carpeta->text().trimmed();
}

QString DialogoExportar::nombreFichero(int fila) const
{
    QString nombre = m_tabla->item(fila, ColFichero)->text().trimmed();
    const QString sufijo = QLatin1Char('.') + m_extension;
    if (!nombre.isEmpty() && !nombre.endsWith(sufijo, Qt::CaseInsensitive))
        nombre += sufijo;
    return nombre;
}

QVector<DialogoExportar::Fichero> DialogoExportar::ficheros() const
{
    QVector<Fichero> res;
    const QDir dir(carpeta());
    for (int f = 0; f < m_tabla->rowCount(); ++f) {
        if (m_tabla->item(f, ColExamen)->checkState() != Qt::Checked)
            continue;
        Fichero fichero;
        if (f < m_numModelos) {
            fichero.modelos << f;
        } else {
            for (int i = 0; i < m_numModelos; ++i)
                fichero.modelos << i;
        }
        fichero.ruta = dir.filePath(nombreFichero(f));
        res << fichero;
    }
    return res;
}

void DialogoExportar::elegirCarpeta()
{
    const QString ruta = QFileDialog::getExistingDirectory(this, QStringLiteral("Carpeta donde guardar los exámenes"), carpeta());
    if (!ruta.isEmpty())
        m_carpeta->setText(QDir::toNativeSeparators(ruta));
}

void DialogoExportar::accept()
{
    // Por si se pulsa Exportar mientras se está editando un nombre.
    if (QWidget *editor = m_tabla->focusWidget(); editor && editor != m_tabla)
        m_tabla->setFocus();

    const QString carpeta = this->carpeta();
    if (carpeta.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), QStringLiteral("Indique la carpeta donde guardar los ficheros."));
        return;
    }
    if (!QDir(carpeta).exists() && !QDir().mkpath(carpeta)) {
        QMessageBox::warning(this, windowTitle(), QStringLiteral("No se pudo crear la carpeta\n%1").arg(carpeta));
        return;
    }
    const QVector<Fichero> lista = ficheros();
    if (lista.isEmpty()) {
        QMessageBox::information(this, windowTitle(), QStringLiteral("Marque al menos un examen."));
        return;
    }
    QSet<QString> vistos;
    QStringList existentes;
    for (const Fichero &f : lista) {
        const QString nombre = QFileInfo(f.ruta).fileName();
        if (nombre.isEmpty() || nombre == QLatin1Char('.') + m_extension) {
            QMessageBox::warning(this, windowTitle(), QStringLiteral("Hay un examen marcado sin nombre de fichero."));
            return;
        }
        if (nombre.contains(QLatin1Char('/')) || nombre.contains(QLatin1Char('\\'))) {
            QMessageBox::warning(this, windowTitle(), QStringLiteral("El nombre «%1» no puede contener barras.").arg(nombre));
            return;
        }
        if (vistos.contains(f.ruta)) {
            QMessageBox::warning(this, windowTitle(), QStringLiteral("Dos exámenes tienen el mismo nombre de fichero: %1").arg(nombre));
            return;
        }
        vistos.insert(f.ruta);
        if (QFileInfo::exists(f.ruta))
            existentes << nombre;
    }
    if (!existentes.isEmpty()
        && !confirmar(this, windowTitle(),
                      QStringLiteral("Ya existen estos ficheros en la carpeta:\n%1\n\n¿Quiere sobrescribirlos?")
                          .arg(existentes.join(QLatin1Char('\n')))))
        return;
    QSettings().setValue(QLatin1String(kClaveCarpeta), carpeta);
    QDialog::accept();
}
