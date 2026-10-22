#pragma once

#include <QDialog>
#include <QStringList>
#include <QVector>

class QLineEdit;
class QTableWidget;

// Exportación en bloque de los exámenes generados: se elige la carpeta y el nombre del fichero de cada examen
// (y, si hay varios, opcionalmente uno con todos juntos) y quien llama los escribe de una vez.
class DialogoExportar : public QDialog
{
    Q_OBJECT
public:
    struct Fichero
    {
        QVector<int> modelos;   // índices de los modelos que contiene
        QString ruta;           // ruta completa, con extensión
    };

    // 'nombres' son los de los exámenes (Nacional Original...) y 'nombreBase' el principio propuesto para el nombre de
    // los ficheros (sin extensión): «<nombreBase>_<examen>», y el de todos juntos, «<nombreBase>». 'extension' va sin punto.
    DialogoExportar(const QString &titulo, const QStringList &nombres, const QString &nombreBase,
                    const QString &extension, QWidget *parent = nullptr);

    QString carpeta() const;
    // Ficheros marcados, en el orden de la tabla.
    QVector<Fichero> ficheros() const;

public slots:
    void accept() override;

private:
    void elegirCarpeta();
    QString nombreFichero(int fila) const;

    QLineEdit *m_carpeta;
    QTableWidget *m_tabla;
    QString m_extension;
    int m_numModelos;
};
