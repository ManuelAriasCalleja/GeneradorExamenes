#include "dialogomoodle.h"
#include "dialogoayuda.h"
#include "utilidades.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace {
const char *const kClaveCarpeta = "exportar/carpeta";   // la misma que el diálogo de exportar exámenes
}

DialogoMoodle::DialogoMoodle(const QVector<Pregunta> &preguntas, const QString &asignatura, QWidget *parent)
    : QDialog(parent), m_preguntas(preguntas), m_asignatura(asignatura)
{
    setWindowTitle(QStringLiteral("Exportar el banco a Moodle"));
    resize(620, 0);
    auto *l = new QVBoxLayout(this);

    auto *explicacion = new QLabel(QStringLiteral(
        "Se guarda un cuestionario en formato «Moodle XML» con las preguntas activas del banco (las anuladas y obsoletas no van): "
        "una categoría por tema bajo «%1», cada pregunta de opción múltiple con una sola respuesta, la referencia al libro y la "
        "explicación como realimentación, y las imágenes incrustadas. En Moodle se importa desde "
        "<i>Banco de preguntas → Importar</i>.").arg(asignatura.toHtmlEscaped()));
    explicacion->setWordWrap(true);
    l->addWidget(explicacion);

    auto *fila = new QHBoxLayout;
    const QString carpeta = QSettings().value(QLatin1String(kClaveCarpeta),
                                              QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    m_ruta = new QLineEdit(carpeta + QLatin1Char('/') + limpiarNombreFichero(asignatura) + QStringLiteral("_moodle.xml"));
    auto *elegir = new QPushButton(QStringLiteral("Elegir…"));
    connect(elegir, &QPushButton::clicked, this, &DialogoMoodle::elegirFichero);
    fila->addWidget(new QLabel(QStringLiteral("Fichero:")));
    fila->addWidget(m_ruta, 1);
    fila->addWidget(elegir);
    l->addLayout(fila);

    m_incluirRevisar = new QCheckBox(QStringLiteral("Incluir las preguntas marcadas para revisar"));
    m_incluirRevisar->setChecked(true);
    connect(m_incluirRevisar, &QCheckBox::toggled, this, &DialogoMoodle::actualizarCuenta);
    m_penalizar = new QCheckBox(QStringLiteral("Las respuestas erróneas restan el 30 % de la pregunta (como en el examen)"));
    m_penalizar->setChecked(true);
    l->addWidget(m_incluirRevisar);
    l->addWidget(m_penalizar);
    m_cuenta = new QLabel;
    l->addWidget(m_cuenta);
    actualizarCuenta();

    auto *botones = new QDialogButtonBox;
    QPushButton *exportar = botones->addButton(QStringLiteral("Exportar"), QDialogButtonBox::AcceptRole);
    exportar->setDefault(true);
    botones->addButton(QStringLiteral("Cancelar"), QDialogButtonBox::RejectRole);
    connect(botones, &QDialogButtonBox::accepted, this, &DialogoMoodle::accept);
    connect(botones, &QDialogButtonBox::rejected, this, &DialogoMoodle::reject);
    DialogoAyuda::ponerAyuda(this, botones, QStringLiteral("moodle"));
    l->addWidget(botones);
}

QString DialogoMoodle::ruta() const
{
    return m_ruta->text().trimmed();
}

Moodle::Opciones DialogoMoodle::opciones() const
{
    Moodle::Opciones o;
    o.asignatura = m_asignatura;
    o.incluirParaRevisar = m_incluirRevisar->isChecked();
    o.penalizarFallos = m_penalizar->isChecked();
    return o;
}

void DialogoMoodle::elegirFichero()
{
    const QString ruta = QFileDialog::getSaveFileName(this, windowTitle(), m_ruta->text(), QStringLiteral("Moodle XML (*.xml)"));
    if (!ruta.isEmpty())
        m_ruta->setText(ruta);
}

void DialogoMoodle::actualizarCuenta()
{
    const QVector<Pregunta> elegidas = Moodle::seleccion(m_preguntas, opciones());
    QSet<int> temas;
    for (const Pregunta &p : elegidas)
        temas.insert(p.temaId);
    m_cuenta->setText(QStringLiteral("Se exportarán %1 preguntas de %2 temas.").arg(elegidas.size()).arg(temas.size()));
}

void DialogoMoodle::accept()
{
    if (ruta().isEmpty()) {
        QMessageBox::warning(this, windowTitle(), QStringLiteral("Indique el fichero."));
        return;
    }
    QSettings().setValue(QLatin1String(kClaveCarpeta), QFileInfo(ruta()).absolutePath());
    QDialog::accept();
}
