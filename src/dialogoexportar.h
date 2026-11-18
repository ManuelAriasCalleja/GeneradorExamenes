#pragma once

#include <QDialog>
#include <QStringList>
#include <QVector>

class QCheckBox;
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
    // los ficheros (sin extensión): «<nombreBase>-<examen>», y el de todos juntos, «<nombreBase>». Con 'soluciones',
    // se propone el mismo nombre con «-soluciones» al final, para que cada fichero quede junto al de sin soluciones
    // en la misma carpeta sin pisarlo. 'extension' va sin punto.
    DialogoExportar(const QString &titulo, const QStringList &nombres, const QString &nombreBase,
                    const QString &extension, bool soluciones, QWidget *parent = nullptr);

    QString carpeta() const;
    // Ficheros marcados, en el orden de la tabla.
    QVector<Fichero> ficheros() const;

    // Pone la casilla «Con soluciones» (con el texto 'texto'), marcada si se han propuesto los nombres con soluciones.
    // Al marcarla o desmarcarla, los nombres propuestos que no se hayan cambiado pasan a llevar «-soluciones» o no.
    void ofrecerSoluciones(const QString &texto);
    bool conSoluciones() const;

public slots:
    void accept() override;

private:
    void elegirCarpeta();
    QString nombreFichero(int fila) const;
    QString nombrePropuesto(int fila, bool soluciones) const;
    void cambiarNombres(bool soluciones);

    QLineEdit *m_carpeta;
    QTableWidget *m_tabla;
    QCheckBox *m_soluciones = nullptr;
    QStringList m_nombres;   // de los exámenes
    QString m_nombreBase;
    QString m_extension;
    bool m_solucionesPropuestas;   // si los nombres que se propusieron al abrir son los de con soluciones
    int m_numModelos;
};
