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

void GrupoPlegable::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    ponerTexto();
}

void GrupoPlegable::ponerTexto()
{
    //: Cabecera de un grupo plegado: %1 es su título («Puntuación») y %2, el resumen de lo elegido dentro
    const QString texto = m_plegado && !m_resumen.isEmpty() ? tr("%1: %2").arg(m_titulo, m_resumen) : m_titulo;
    // Sitio para la flecha y los márgenes del botón.
    const int ancho = qMax(40, m_cabecera->width() - m_cabecera->iconSize().width() - 16);
    m_cabecera->setText(m_cabecera->fontMetrics().elidedText(texto, Qt::ElideRight, ancho));
    m_cabecera->setToolTip(texto);
}
