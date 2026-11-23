#pragma once

#include <QFont>
#include <QList>
#include <QObject>
#include <QPointer>

class QWidget;
class VisorAmpliable;

// Zona de la ventana (un panel con sus controles) cuya letra se amplía y se reduce con Ctrl + rueda del ratón sobre
// ella, por los mismos pasos que un VisorAmpliable. Cambia la letra del panel, que heredan sus controles; lo que no
// depende de la letra (el ancho que se haya dado a las columnas de una tabla) lo ajusta quien la usa, con
// escalaCambiada().
class ZonaAmpliable : public QObject
{
    Q_OBJECT

public:
    explicit ZonaAmpliable(QWidget *raiz, QObject *parent = nullptr);

    // 1 es el tamaño normal; va de 0,5 a 3.
    qreal escala() const { return m_escala; }
    void ponerEscala(qreal escala);

    // Un visor de la zona (una ficha): se amplía con ella, a su manera (VisorAmpliable::ponerEscala()).
    void anadirVisor(VisorAmpliable *visor);

    // Si 'w' es de la zona.
    bool contiene(const QWidget *w) const;

public slots:
    void ampliar();
    void reducir();
    void tamanoNormal();

signals:
    void escalaCambiada(qreal escala);

protected:
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    void aplicar();
    QFont letra() const;   // la del panel con la escala

    QPointer<QWidget> m_raiz;
    QList<QPointer<VisorAmpliable>> m_visores;
    QFont m_letra;      // la del panel a tamaño normal
    qreal m_escala = 1;
    int m_giro = 0;     // lo que ha girado la rueda con Ctrl y aún no llega a un paso (120)
};
