#pragma once

#include <QDialog>

class QDialogButtonBox;
class QLineEdit;
class QTextBrowser;
class QTreeWidget;

// Manual de uso del programa (recursos/ayuda.html): el texto, con el índice de sus apartados a la izquierda y un
// buscador encima. Cada parte del programa lo abre por su apartado, con F1 o con el botón «Ayuda».
class DialogoAyuda : public QDialog
{
    Q_OBJECT
public:
    explicit DialogoAyuda(QWidget *parent = nullptr);

    // Abre la ayuda por el apartado 'seccion' (el ancla de su título en el manual). Hay una sola ventana de ayuda,
    // hija de la ventana desde la que se pide: si fuera hija de otra, un diálogo modal la dejaría sin poder usarse.
    static DialogoAyuda *mostrar(QWidget *desde, const QString &seccion);
    // Pone en 'dialogo' la tecla F1 y, si se da 'botones', el botón «Ayuda», que abren la ayuda por 'seccion'.
    static void ponerAyuda(QDialog *dialogo, QDialogButtonBox *botones, const QString &seccion);

    // Lleva el texto al apartado y lo marca en el índice; false si el manual no lo tiene.
    bool irA(const QString &seccion);
    QString seccionActual() const;
    // Anclas de los apartados del índice, en orden.
    QStringList secciones() const;

private:
    void rellenarIndice();
    // Busca el texto del recuadro de búsqueda, sin distinguir tildes ni mayúsculas. Al escribir, desde donde empieza
    // lo encontrado (para que se alargue); con 'siguiente', a continuación. Al llegar al final sigue por el principio.
    void buscar(bool siguiente);

    QTextBrowser *m_texto;
    QTreeWidget *m_indice;
    QLineEdit *m_buscar;
    QString m_manual;   // el texto del manual como lo da paraBuscar(), para buscar en él
};
