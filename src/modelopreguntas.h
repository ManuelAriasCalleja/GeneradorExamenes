#pragma once

#include "pregunta.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QVector>

// Tabla de preguntas para la vista del banco.
class ModeloPreguntas : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Columna { ColTema, ColCodigo, ColEstado, ColEnunciado, ColCorrecta, NumColumnas };

    using QAbstractTableModel::QAbstractTableModel;

    void establecer(const QVector<Pregunta> &preguntas);
    const Pregunta &pregunta(int fila) const { return m_preguntas.at(fila); }
    // Enunciado, opciones y notas de la pregunta, como los da paraBuscar(), para filtrar.
    const QString &textoParaBuscar(int fila) const { return m_paraBuscar.at(fila); }
    int filaDeId(int id) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    QVector<Pregunta> m_preguntas;
    QStringList m_paraBuscar;
};

class FiltroPreguntas : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    enum FiltroEstado { Todas, Activas, Anuladas, Obsoletas, ParaRevisar, ConImagen };

    using QSortFilterProxyModel::QSortFilterProxyModel;

    void setTema(int numeroTema);        // 0 = todos
    void setEstado(FiltroEstado estado);
    void setTexto(const QString &texto);

protected:
    bool filterAcceptsRow(int fila, const QModelIndex &padre) const override;
    bool lessThan(const QModelIndex &a, const QModelIndex &b) const override;

private:
    int m_tema = 0;
    FiltroEstado m_estado = Todas;
    QString m_texto;   // como lo da paraBuscar()
};
