#pragma once

#include "examenesanteriores.h"
#include "maquetador.h"

#include <QDialog>

class BaseDatos;
class QComboBox;
class QLabel;
class QSpinBox;
class QTableWidget;
class QTableWidgetItem;

// «Registrar exámenes anteriores»: los exámenes de una convocatoria hecha fuera del programa, leídos de sus PDF o de
// su texto pegado, con sus preguntas reconocidas en el banco, para revisarlas y registrarlas en el historial. Así
// se evitan al generar los de las convocatorias siguientes.
class DialogoAnteriores : public QDialog
{
    Q_OBJECT
public:
    // 'datos': asignatura e instrucciones con que se registran; 'conocidos': los nombres de la lista de exámenes, a
    // los que se ajustan los que digan los ficheros.
    DialogoAnteriores(BaseDatos *bd, const QVector<Pregunta> &banco, const DatosConvocatoria &datos,
                      const QStringList &conocidos, QWidget *parent = nullptr);

    // Añade un examen con su texto; 'fichero', si viene de uno, para proponer su nombre y la convocatoria. Devuelve
    // false (y avisa) si no se encuentra ninguna pregunta.
    bool anadirExamen(const QString &texto, const QString &fichero = QString());
    QString convocatoriaYAnio() const;

public slots:
    void accept() override;   // registra los exámenes

private:
    void anadirPdf();
    void pegarTexto();
    void quitarExamen();
    void mostrarExamen();   // las preguntas del examen elegido
    void ponerFila(int fila);
    void codigoCambiado(QTableWidgetItem *item);
    void nombreCambiado(QTableWidgetItem *item);
    void actualizarExamen(int examen);   // su fila en la lista y el resumen

    struct Examen
    {
        QString nombre;
        QString fuente;   // fichero o «texto pegado»
        QVector<ExamenesAnteriores::Reconocida> preguntas;
    };
    QVector<Examen> m_examenes;
    BaseDatos *m_bd;
    QVector<Pregunta> m_banco;
    DatosConvocatoria m_datos;
    QStringList m_conocidos;
    bool m_rellenando = false;   // las tablas se están rellenando: sus cambios no son del usuario

    QComboBox *m_convocatoria;
    QSpinBox *m_anio;
    QTableWidget *m_lista;       // los exámenes
    QTableWidget *m_preguntas;   // las preguntas del examen elegido
    QLabel *m_resumen;
};
