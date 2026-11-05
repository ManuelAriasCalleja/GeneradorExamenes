#pragma once

#include "comprobadorjson.h"

#include <QDialog>

class QDialogButtonBox;
class QLabel;
class QPushButton;
class QTreeWidget;

// Problemas de un JSON de preguntas (ComprobadorJson): Archivo → Comprobar un JSON… y, antes de importar, cuando el
// JSON tiene alguno. Cada problema con su línea, la pregunta en la que está y qué pasa. Se puede corregir el fichero
// en un editor de textos y volver a comprobarlo sin cerrar el diálogo.
class DialogoComprobacion : public QDialog
{
    Q_OBJECT
public:
    enum Modo {
        Informe,            // solo enseña los problemas
        AntesDeImportar,    // con errores no deja importar; con avisos, se puede «Importar de todas formas»
    };
    // 'grupos', los del banco abierto (ver ComprobadorJson::comprobar()).
    DialogoComprobacion(const QString &ruta, const QSet<QString> &grupos, Modo modo, QWidget *parent = nullptr);

    const ComprobadorJson::Resultado &resultado() const { return m_resultado; }

private slots:
    void comprobar();
    void copiar();
    void abrirFichero();

private:
    QString m_ruta;
    QSet<QString> m_grupos;
    Modo m_modo;
    ComprobadorJson::Resultado m_resultado;
    QLabel *m_resumen;
    QTreeWidget *m_lista;
    QPushButton *m_importar = nullptr;
    QDialogButtonBox *m_botones;
};
