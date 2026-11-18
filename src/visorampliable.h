#pragma once

#include <QTextBrowser>

class QAction;
class QTextDocument;

// Visor de texto que se amplía y se reduce con los controles habituales: Ctrl + rueda del ratón, Ctrl + «+»,
// Ctrl + «-» y Ctrl + 0 (tamaño normal). Solo cambia la vista: el documento se imprime y se exporta igual.
//
// QTextEdit::zoomIn() no sirve para los exámenes: cambia la letra por defecto del documento, y la de los exámenes
// lleva su tamaño en puntos (Maquetador). Aquí se cambia el tamaño de la letra de cada trozo del documento, el de sus
// imágenes y el ancho fijo de las columnas de sus tablas; los márgenes y las sangrías se quedan como están.
class VisorAmpliable : public QTextBrowser
{
    Q_OBJECT

public:
    explicit VisorAmpliable(QWidget *parent = nullptr);

    // 1 es el tamaño normal; va de 0,5 a 3.
    qreal escala() const { return m_escala; }
    void ponerEscala(qreal escala);

    // Pone un documento (como ponerDocumento() de utilidades.h) con la escala de la vista.
    void ponerDocumento(QTextDocument *doc);

    // Ampliar, reducir y tamaño normal, con sus atajos. Funcionan cuando el foco está en el visor o en cualquier otro
    // control de los widgets a los que se añadan (QWidget::addActions()).
    QList<QAction *> accionesEscala() const;

    // Pone el documento a 'escala' veces su tamaño (el de la letra, las imágenes y las columnas). Los tamaños
    // originales se guardan en sus formatos la primera vez, así que se puede llamar varias veces seguidas y con 1
    // vuelve a ser el que era.
    static void escalarDocumento(QTextDocument *doc, qreal escala);

public slots:
    void ampliar();
    void reducir();
    void tamanoNormal();

signals:
    void escalaCambiada(qreal escala);

protected:
    void wheelEvent(QWheelEvent *e) override;

private:
    void actualizarAcciones();

    qreal m_escala = 1;
    int m_giro = 0;   // lo que ha girado la rueda con Ctrl y aún no llega a un paso (120)
    QAction *m_ampliar;
    QAction *m_reducir;
    QAction *m_normal;
};
