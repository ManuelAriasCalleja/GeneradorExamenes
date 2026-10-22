#pragma once

#include "pregunta.h"

#include <QDialog>
#include <functional>

class QCheckBox;
class QComboBox;
class QLabel;
class QTreeWidget;

// Informe de estilo de todo el banco abierto: los avisos de cada pregunta, por partes (enunciado, opciones,
// referencia). Desde aquí se abre cada pregunta para corregirla, y el informe se puede guardar en un fichero de
// texto (solo con los códigos de las preguntas y los avisos, sin su texto).
class DialogoEstilo : public QDialog
{
    Q_OBJECT
public:
    // 'preguntas' da las del banco abierto (se vuelve a llamar tras editar una); 'editar' abre el editor de la
    // pregunta con ese id; 'banco' es la ruta del banco, para el informe guardado.
    DialogoEstilo(std::function<QVector<Pregunta>()> preguntas, std::function<void(int)> editar, const QString &banco,
                  QWidget *parent = nullptr);

private slots:
    void rellenar();
    void editarSeleccionada();
    void guardar();

private:
    std::function<QVector<Pregunta>()> m_preguntas;
    std::function<void(int)> m_editar;
    QString m_banco;
    QComboBox *m_parte;
    QCheckBox *m_todas;
    QLabel *m_resumen;
    QTreeWidget *m_arbol;
};
