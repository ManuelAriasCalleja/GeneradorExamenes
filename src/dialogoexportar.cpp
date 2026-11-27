#include "dialogoexportar.h"
#include "dialogoayuda.h"
#include "utilidades.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
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

// La carpeta de los enunciados es también la de Moodle (DialogoMoodle). Las dos van con cada banco (VentanaPrincipal).
const char *const kClaveCarpeta = "exportar/carpeta";
const char *const kClaveCarpetaSoluciones = "exportar/carpetaSoluciones";
const char *const kClaveFormatos = "exportar/formatos";   // sus extensiones
const char *const kClaveEnunciados = "exportar/enunciados";
const char *const kClaveSoluciones = "exportar/soluciones";
enum Columna { ColExamen = 0, ColFichero = 1 };

} // namespace

QString DialogoExportar::extension(Formato formato)
{
    switch (formato) {
    case Word:
        return QStringLiteral("docx");
    case Html:
        return QStringLiteral("html");
    case Latex:
        return QStringLiteral("tex");
    case Pdf:
        break;
    }
    return QStringLiteral("pdf");
}

DialogoExportar::DialogoExportar(const QStringList &nombres, const QString &nombreBase, QWidget *parent)
    : QDialog(parent), m_numModelos(int(nombres.size()))
{
    setWindowTitle(tr("Exportar exámenes"));
    resize(700, 520);
    const QSettings ajustes;
    auto *l = new QVBoxLayout(this);

    auto *filaFormatos = new QHBoxLayout;
    //: Delante de las casillas de los formatos en que se exportan los exámenes (PDF, Word, HTML, LaTeX)
    filaFormatos->addWidget(new QLabel(tr("Formatos:")));
    //: Formato de exportación: documento de Word
    const QStringList textos{QStringLiteral("PDF"), tr("Word (.docx)"), QStringLiteral("HTML"), QStringLiteral("LaTeX")};
    const QStringList avisos{QString(), tr("Se puede abrir y modificar con Word o LibreOffice."),
                             tr("Se puede abrir y modificar con Word o LibreOffice."),
                             tr("Para compilarlo con pdflatex (dos veces, para que salga el total de páginas del pie).")};
    const QStringList marcados = ajustes.value(QLatin1String(kClaveFormatos), QStringList{extension(Pdf)}).toStringList();
    for (int f = 0; f < kNumFormatos; ++f) {
        m_formatos[f] = new QCheckBox(textos.at(f));
        m_formatos[f]->setObjectName(QStringLiteral("formato-") + extension(Formato(f)));
        m_formatos[f]->setToolTip(avisos.at(f));
        m_formatos[f]->setChecked(marcados.contains(extension(Formato(f))));
        connect(m_formatos[f], &QCheckBox::toggled, this, &DialogoExportar::actualizar);
        filaFormatos->addWidget(m_formatos[f]);
    }
    filaFormatos->addStretch();
    l->addLayout(filaFormatos);

    // Enunciados y soluciones, cada uno con su carpeta. La de las soluciones, si no hay, la de los enunciados.
    auto *carpetas = new QGridLayout;
    const QString documentos = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString deEnunciados = ajustes.value(QLatin1String(kClaveCarpeta), documentos).toString();
    auto filaCarpeta = [&](int fila, QCheckBox *casilla, QLineEdit *&campo, QPushButton *&elegir, const QString &ruta, bool soluciones) {
        campo = new QLineEdit(ruta);
        campo->setObjectName(soluciones ? QStringLiteral("carpetaSoluciones") : QStringLiteral("carpetaEnunciados"));
        elegir = new QPushButton(tr("Elegir…"));
        connect(elegir, &QPushButton::clicked, this, [this, soluciones] { elegirCarpeta(soluciones); });
        connect(casilla, &QCheckBox::toggled, this, &DialogoExportar::actualizar);
        carpetas->addWidget(casilla, fila, 0);
        //: Delante del campo de la carpeta de los enunciados o de las soluciones, en el diálogo de exportar
        carpetas->addWidget(new QLabel(tr("en la carpeta")), fila, 1);
        carpetas->addWidget(campo, fila, 2);
        carpetas->addWidget(elegir, fila, 3);
    };
    //: Casilla del diálogo de exportar: los exámenes tal como los ven los estudiantes, sin soluciones
    m_enunciados = new QCheckBox(tr("Enunciados"));
    m_enunciados->setToolTip(tr("Los exámenes tal como los ven los estudiantes, sin soluciones."));
    m_enunciados->setChecked(ajustes.value(QLatin1String(kClaveEnunciados), true).toBool());
    //: Casilla del diálogo de exportar: los exámenes con sus soluciones
    m_soluciones = new QCheckBox(tr("Soluciones"));
    m_soluciones->setToolTip(tr("La hoja de soluciones (estructura y plantilla de corrección) y los exámenes con la "
                                "respuesta correcta marcada y, en cada pregunta, la explicación y la referencia."));
    m_soluciones->setChecked(ajustes.value(QLatin1String(kClaveSoluciones), true).toBool());
    filaCarpeta(0, m_enunciados, m_carpetaEnunciados, m_elegirEnunciados, deEnunciados, false);
    filaCarpeta(1, m_soluciones, m_carpetaSoluciones, m_elegirSoluciones,
                ajustes.value(QLatin1String(kClaveCarpetaSoluciones), deEnunciados).toString(), true);
    carpetas->setColumnStretch(2, 1);
    l->addLayout(carpetas);

    l->addWidget(new QLabel(tr("Marque los exámenes que quiere exportar y cambie el nombre de sus ficheros si lo desea:")));
    m_tabla = new QTableWidget(0, 2);
    m_tabla->setHorizontalHeaderLabels({tr("Examen"), tr("Fichero")});
    m_tabla->horizontalHeader()->setSectionResizeMode(ColExamen, QHeaderView::ResizeToContents);
    m_tabla->horizontalHeader()->setSectionResizeMode(ColFichero, QHeaderView::Stretch);
    m_tabla->verticalHeader()->hide();
    m_tabla->setSelectionMode(QAbstractItemView::NoSelection);
    m_tabla->setEditTriggers(QAbstractItemView::AllEditTriggers);
    auto anadirFila = [&](const QString &examen, const QString &fichero, bool marcado) {
        const int f = m_tabla->rowCount();
        m_tabla->insertRow(f);
        auto *marca = new QTableWidgetItem(examen);
        marca->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        marca->setCheckState(marcado ? Qt::Checked : Qt::Unchecked);
        m_tabla->setItem(f, ColExamen, marca);
        auto *nombre = new QTableWidgetItem(fichero);
        nombre->setToolTip(tr("Pulse sobre el nombre para cambiarlo. Sin extensión: cada fichero lleva la de su formato "
                              "y los de las soluciones, además, «-soluciones» al final."));
        m_tabla->setItem(f, ColFichero, nombre);
    };
    for (const QString &examen : nombres)
        anadirFila(examen, nombreBase + QLatin1Char('-') + limpiarNombreFichero(examen), true);
    if (nombres.size() > 1)
        anadirFila(tr("Todos los exámenes en un fichero"), nombreBase, false);
    connect(m_tabla, &QTableWidget::itemChanged, this, &DialogoExportar::actualizar);
    l->addWidget(m_tabla, 1);
    m_resumen = new QLabel;
    m_resumen->setObjectName(QStringLiteral("resumen"));
    m_resumen->setWordWrap(true);
    l->addWidget(m_resumen);

    auto *botones = new QDialogButtonBox;
    QPushButton *exportar = botones->addButton(tr("Exportar"), QDialogButtonBox::AcceptRole);
    botones->addButton(tr("Cancelar"), QDialogButtonBox::RejectRole);
    connect(botones, &QDialogButtonBox::accepted, this, &DialogoExportar::accept);
    connect(botones, &QDialogButtonBox::rejected, this, &DialogoExportar::reject);
    DialogoAyuda::ponerAyuda(this, botones, QStringLiteral("exportar"));
    l->addWidget(botones);
    // Después de meter los botones en el diálogo: antes, Qt no se entera de cuál es el botón por defecto (el de
    // Intro) y al mostrarse se lo da al primero que encuentra.
    exportar->setDefault(true);
    actualizar();
}

QString DialogoExportar::carpeta(bool soluciones) const
{
    return (soluciones ? m_carpetaSoluciones : m_carpetaEnunciados)->text().trimmed();
}

QString DialogoExportar::nombre(int fila) const
{
    QString nombre = m_tabla->item(fila, ColFichero)->text().trimmed();
    for (int f = 0; f < kNumFormatos; ++f) {
        const QString sufijo = QLatin1Char('.') + extension(Formato(f));
        if (nombre.endsWith(sufijo, Qt::CaseInsensitive))
            return nombre.chopped(sufijo.size()).trimmed();
    }
    return nombre;
}

QVector<DialogoExportar::Fichero> DialogoExportar::ficheros() const
{
    QVector<Fichero> res;
    for (int fila = 0; fila < m_tabla->rowCount(); ++fila) {
        if (m_tabla->item(fila, ColExamen)->checkState() != Qt::Checked)
            continue;
        QVector<int> modelos;
        if (fila < m_numModelos) {
            modelos << fila;
        } else {
            for (int i = 0; i < m_numModelos; ++i)
                modelos << i;
        }
        for (bool soluciones : {false, true}) {
            if (!(soluciones ? m_soluciones : m_enunciados)->isChecked())
                continue;
            for (int f = 0; f < kNumFormatos; ++f) {
                if (!m_formatos[f]->isChecked())
                    continue;
                Fichero fichero;
                fichero.modelos = modelos;
                fichero.formato = Formato(f);
                fichero.soluciones = soluciones;
                fichero.ruta = QDir(carpeta(soluciones)).filePath(nombre(fila) + (soluciones ? QStringLiteral("-soluciones") : QString())
                                                                  + QLatin1Char('.') + extension(Formato(f)));
                res << fichero;
            }
        }
    }
    return res;
}

void DialogoExportar::actualizar()
{
    m_carpetaEnunciados->setEnabled(m_enunciados->isChecked());
    m_elegirEnunciados->setEnabled(m_enunciados->isChecked());
    m_carpetaSoluciones->setEnabled(m_soluciones->isChecked());
    m_elegirSoluciones->setEnabled(m_soluciones->isChecked());
    const int cuantos = int(ficheros().size());
    QString resumen = cuantos == 0 ? tr("No se guardará ningún fichero: marque al menos un formato, «Enunciados» o "
                                        "«Soluciones» y un examen.")
                    : cuantos == 1 ? tr("Se guardará un fichero.")
                                   : tr("Se guardarán %1 ficheros.").arg(cuantos);
    if (cuantos > 0 && m_soluciones->isChecked())
        resumen += QLatin1Char(' ') + tr("Los de las soluciones acaban en «-soluciones».");
    m_resumen->setText(resumen);
}

void DialogoExportar::elegirCarpeta(bool soluciones)
{
    QLineEdit *campo = soluciones ? m_carpetaSoluciones : m_carpetaEnunciados;
    // El foco vuelve al campo de la carpeta: si se quedara en «Elegir…», Intro la pediría otra vez en vez de exportar.
    campo->setFocus();
    const QString titulo = soluciones ? tr("Carpeta donde guardar las soluciones") : tr("Carpeta donde guardar los exámenes");
    const QString ruta = QFileDialog::getExistingDirectory(this, titulo, carpeta(soluciones));
    if (!ruta.isEmpty())
        campo->setText(QDir::toNativeSeparators(ruta));
}

void DialogoExportar::accept()
{
    // Por si se pulsa Exportar mientras se está editando un nombre.
    if (QWidget *editor = m_tabla->focusWidget(); editor && editor != m_tabla)
        m_tabla->setFocus();

    bool algunFormato = false;
    for (QCheckBox *f : m_formatos)
        algunFormato = algunFormato || f->isChecked();
    if (!algunFormato) {
        QMessageBox::information(this, windowTitle(), tr("Marque al menos un formato."));
        return;
    }
    if (!m_enunciados->isChecked() && !m_soluciones->isChecked()) {
        QMessageBox::information(this, windowTitle(), tr("Marque «Enunciados», «Soluciones» o los dos."));
        return;
    }
    for (bool soluciones : {false, true}) {
        if (!(soluciones ? m_soluciones : m_enunciados)->isChecked())
            continue;
        const QString carpeta = this->carpeta(soluciones);
        if (carpeta.isEmpty()) {
            QMessageBox::warning(this, windowTitle(), soluciones ? tr("Indique la carpeta donde guardar las soluciones.")
                                                                 : tr("Indique la carpeta donde guardar los enunciados."));
            return;
        }
        if (!QDir(carpeta).exists() && !QDir().mkpath(carpeta)) {
            QMessageBox::warning(this, windowTitle(), tr("No se pudo crear la carpeta\n%1").arg(carpeta));
            return;
        }
    }
    for (int fila = 0; fila < m_tabla->rowCount(); ++fila) {
        if (m_tabla->item(fila, ColExamen)->checkState() != Qt::Checked)
            continue;
        const QString nombre = this->nombre(fila);
        if (nombre.isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("Hay un examen marcado sin nombre de fichero."));
            return;
        }
        if (nombre.contains(QLatin1Char('/')) || nombre.contains(QLatin1Char('\\'))) {
            QMessageBox::warning(this, windowTitle(), tr("El nombre «%1» no puede contener barras.").arg(nombre));
            return;
        }
    }
    const QVector<Fichero> lista = ficheros();
    if (lista.isEmpty()) {
        QMessageBox::information(this, windowTitle(), tr("Marque al menos un examen."));
        return;
    }
    QSet<QString> vistos;
    QStringList existentes;
    for (const Fichero &f : lista) {
        const QString nombre = QFileInfo(f.ruta).fileName();
        if (vistos.contains(f.ruta)) {
            QMessageBox::warning(this, windowTitle(), tr("Dos exámenes tienen el mismo nombre de fichero: %1").arg(nombre));
            return;
        }
        vistos.insert(f.ruta);
        if (QFileInfo::exists(f.ruta))
            existentes << nombre;
    }
    if (!existentes.isEmpty()) {
        // Como mucho, doce: con varios formatos pueden ser muchos.
        constexpr int kMaxNombres = 12;
        QString lista = existentes.mid(0, kMaxNombres).join(QLatin1Char('\n'));
        if (existentes.size() > kMaxNombres)
            //: Tras la lista de ficheros que ya existen, cuando son más de los que se ven
            lista += QLatin1Char('\n') + tr("… y %1 más").arg(existentes.size() - kMaxNombres);
        if (!confirmar(this, windowTitle(), tr("Ya existen estos ficheros en la carpeta:\n%1\n\n¿Quiere sobrescribirlos?").arg(lista)))
            return;
    }
    QSettings ajustes;
    if (m_enunciados->isChecked())
        ajustes.setValue(QLatin1String(kClaveCarpeta), carpeta(false));
    if (m_soluciones->isChecked())
        ajustes.setValue(QLatin1String(kClaveCarpetaSoluciones), carpeta(true));
    QStringList formatos;
    for (int f = 0; f < kNumFormatos; ++f)
        if (m_formatos[f]->isChecked())
            formatos << extension(Formato(f));
    ajustes.setValue(QLatin1String(kClaveFormatos), formatos);
    ajustes.setValue(QLatin1String(kClaveEnunciados), m_enunciados->isChecked());
    ajustes.setValue(QLatin1String(kClaveSoluciones), m_soluciones->isChecked());
    QDialog::accept();
}
