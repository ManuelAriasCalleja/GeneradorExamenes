#include "grupoplegable.h"

#include <QGroupBox>
#include <QSettings>
#include <QToolButton>
#include <QVBoxLayout>

GrupoPlegable::GrupoPlegable(QGroupBox *grupo, const QString &clave, bool plegadoDeEntrada, QWidget *parent)
    : QWidget(parent), m_titulo(grupo->title()), m_clave(clave), m_grupo(grupo)
{
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(2);
    m_cabecera = new QToolButton;
    m_cabecera->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_cabecera->setAutoRaise(true);
    // El resumen no ensancha el panel: si no cabe, se corta (y se ve entero al dejar el ratón encima).
    m_cabecera->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    connect(m_cabecera, &QToolButton::clicked, this, [this]() { plegar(!m_plegado); });
    l->addWidget(m_cabecera);
    grupo->setTitle(QString());
    l->addWidget(grupo);
    m_plegado = !QSettings().value(m_clave, plegadoDeEntrada).toBool();   // para que plegar() lo aplique
    plegar(!m_plegado);
}

void GrupoPlegable::plegar(bool plegado)
{
    if (plegado != m_plegado) {
        m_plegado = plegado;
        QSettings().setValue(m_clave, plegado);
    }
    m_grupo->setVisible(!plegado);
    m_cabecera->setArrowType(plegado ? Qt::RightArrow : Qt::DownArrow);
    ponerTexto();
}

void GrupoPlegable::ponerResumen(const QString &resumen)
{
    m_resumen = resumen;
    ponerTexto();
}

void GrupoPlegable::ponerTituloDesplegado(const QString &titulo)
{
    m_tituloDesplegado = titulo;
    ponerTexto();
}

void GrupoPlegable::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    ponerTexto();
}

void GrupoPlegable::ponerTexto()
{
    //: Cabecera de un grupo plegado: %1 es su título («Puntuación») y %2, el resumen de lo elegido dentro
    if (m_plegado)
        m_texto = m_resumen.isEmpty() ? m_titulo : tr("%1: %2").arg(m_titulo, m_resumen);
    else
        m_texto = m_tituloDesplegado.isEmpty() ? m_titulo : m_tituloDesplegado;
    // Sitio para la flecha y los márgenes del botón.
    const int ancho = qMax(40, m_cabecera->width() - m_cabecera->iconSize().width() - 16);
    m_cabecera->setText(m_cabecera->fontMetrics().elidedText(m_texto, Qt::ElideRight, ancho));
    m_cabecera->setToolTip(m_texto);
}
