#pragma once

#include <QDialog>
#include <QVector>

class BaseDatos;
class QTreeWidget;
class QTreeWidgetItem;

// Historial de convocatorias: los exámenes registrados, agrupados por convocatoria. Se puede cargar una
// convocatoria (o un solo examen) en la ventana principal, para verla, exportarla o imprimirla tal como salió, o
// eliminarla del historial.
class DialogoHistorial : public QDialog
{
    Q_OBJECT
public:
    explicit DialogoHistorial(BaseDatos *bd, QWidget *parent = nullptr);

    // Exámenes que hay que cargar, en orden (los de la convocatoria o el examen elegidos al aceptar).
    QVector<int> examenesElegidos() const { return m_elegidos; }

public slots:
    void accept() override;

private slots:
    void eliminar();

private:
    void rellenar();
    // Exámenes de un elemento del árbol: el suyo, o todos los de la convocatoria.
    QVector<int> examenesDe(const QTreeWidgetItem *item) const;

    BaseDatos *m_bd;
    QTreeWidget *m_arbol;
    QVector<int> m_elegidos;
};
