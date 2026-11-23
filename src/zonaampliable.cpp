#include "zonaampliable.h"

#include "visorampliable.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QWheelEvent>
#include <QWidget>

namespace {

// Propiedad de los controles a los que se ha puesto la letra de la zona uno a uno.
constexpr const char *kConLetraDeLaZona = "conLetraDeLaZona";

void ponerLetra(QWidget *w, const QFont &letra)
{
    // Con lo suyo (negrita, cursiva) y el tamaño de la zona.
    QFont f = w->font();
    if (letra.pointSizeF() > 0)
        f.setPointSizeF(letra.pointSizeF());
    else
        f.setPixelSize(letra.pixelSize());
    if (f != w->font() || !w->property(kConLetraDeLaZona).toBool()) {
        w->setProperty(kConLetraDeLaZona, true);
        w->setFont(f);
    }
}

} // namespace

ZonaAmpliable::ZonaAmpliable(QWidget *raiz, QObject *parent)
    : QObject(parent), m_raiz(raiz), m_letra(raiz->font())
{
    // La rueda le llega al control que está bajo el puntero (y con Ctrl, una casilla numérica cambiaría de valor):
    // se mira antes, en la aplicación.
    qApp->installEventFilter(this);
}

void ZonaAmpliable::anadirVisor(VisorAmpliable *visor)
{
    m_visores.append(visor);
    visor->ponerEscala(m_escala);
}

bool ZonaAmpliable::contiene(const QWidget *w) const
{
    return m_raiz && w && (w == m_raiz || m_raiz->isAncestorOf(w));
}

void ZonaAmpliable::ponerEscala(qreal escala)
{
    escala = VisorAmpliable::escalaValida(escala);
    if (qAbs(escala - m_escala) < 0.001)
        return;
    m_escala = escala;
    aplicar();
    emit escalaCambiada(m_escala);
}

void ZonaAmpliable::ampliar()
{
    ponerEscala(VisorAmpliable::escalaSiguiente(m_escala));
}

void ZonaAmpliable::reducir()
{
    ponerEscala(VisorAmpliable::escalaAnterior(m_escala));
}

void ZonaAmpliable::tamanoNormal()
{
    ponerEscala(1);
}

QFont ZonaAmpliable::letra() const
{
    QFont letra = m_letra;
    if (m_letra.pointSizeF() > 0)
        letra.setPointSizeF(m_letra.pointSizeF() * m_escala);
    else if (m_letra.pixelSize() > 0)
        letra.setPixelSize(qMax(1, qRound(m_letra.pixelSize() * m_escala)));
    return letra;
}

void ZonaAmpliable::aplicar()
{
    if (!m_raiz)
        return;
    const QFont letra = this->letra();
    m_raiz->setFont(letra);
    // Los controles con hoja de estilo propia (un color) no heredan la letra del panel: se les pone (y se apunta, por
    // si luego se quedan sin hoja de estilo). Los que se crean o la reciben después, en eventFilter().
    const QList<QWidget *> controles = m_raiz->findChildren<QWidget *>();
    for (QWidget *w : controles)
        if (!w->styleSheet().isEmpty() || w->property(kConLetraDeLaZona).toBool())
            ponerLetra(w, letra);
    // Las listas de los desplegables son ventanas aparte y no heredan la letra.
    const QList<QComboBox *> desplegables = m_raiz->findChildren<QComboBox *>();
    for (QComboBox *c : desplegables)
        c->view()->setFont(letra);
    // Los visores amplían su documento ellos mismos (la letra del panel no les cambia la del documento).
    for (const QPointer<VisorAmpliable> &v : std::as_const(m_visores))
        if (v)
            v->ponerEscala(m_escala);
}

bool ZonaAmpliable::eventFilter(QObject *o, QEvent *e)
{
    // Un control de la zona con hoja de estilo, recién puesto o que acaba de recibirla: no hereda la letra ampliada.
    if ((e->type() == QEvent::Polish || e->type() == QEvent::StyleChange) && m_escala != 1 && o->isWidgetType()) {
        auto *w = static_cast<QWidget *>(o);
        if (!w->styleSheet().isEmpty() && w != m_raiz && contiene(w))
            ponerLetra(w, letra());
        return false;
    }
    if (e->type() != QEvent::Wheel || !o->isWidgetType())
        return false;
    auto *rueda = static_cast<QWheelEvent *>(e);
    if (!(rueda->modifiers() & Qt::ControlModifier) || !contiene(static_cast<QWidget *>(o))) {
        if (contiene(static_cast<QWidget *>(o)))
            m_giro = 0;
        return false;
    }
    // Un paso de la rueda son 120; las ruedas finas y los paneles táctiles dan pasos más cortos, que se van sumando.
    m_giro += rueda->angleDelta().y();
    for (; m_giro >= 120; m_giro -= 120)
        ampliar();
    for (; m_giro <= -120; m_giro += 120)
        reducir();
    e->accept();
    return true;
}
