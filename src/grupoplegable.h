#pragma once

#include <QWidget>

class QGroupBox;
class QToolButton;

// Un grupo de controles que se pliega: plegado, solo se ve su título con un resumen de lo que hay elegido dentro
// («Puntuación: cada fallo resta el 30 %»). Para lo que se decide una vez y casi nunca se cambia: no ocupa sitio ni
// distrae, y lo elegido sigue a la vista. Pulsando en el título se despliega o se pliega; cómo está se recuerda de
// una vez a otra (en la configuración, con la clave que se le da).
class GrupoPlegable : public QWidget
{
    Q_OBJECT
public:
    // 'grupo' pasa a ser el contenido: su título se pone en la cabecera. 'plegadoDeEntrada': la primera vez.
    GrupoPlegable(QGroupBox *grupo, const QString &clave, bool plegadoDeEntrada, QWidget *parent = nullptr);

    bool plegado() const { return m_plegado; }
    void plegar(bool plegado);
    // Lo que se ve tras el título cuando está plegado.
    void ponerResumen(const QString &resumen);
    QString resumen() const { return m_resumen; }

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    void ponerTexto();

    QString m_titulo;
    QString m_resumen;
    QString m_clave;
    bool m_plegado = false;
    QToolButton *m_cabecera;
    QGroupBox *m_grupo;
};
