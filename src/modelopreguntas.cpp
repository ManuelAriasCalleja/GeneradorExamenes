#include "modelopreguntas.h"

#include <QBrush>
#include <QColor>
#include <QFont>

void ModeloPreguntas::establecer(const QVector<Pregunta> &preguntas)
{
    beginResetModel();
    m_preguntas = preguntas;
    endResetModel();
}

int ModeloPreguntas::filaDeId(int id) const
{
    for (int i = 0; i < m_preguntas.size(); ++i)
        if (m_preguntas.at(i).id == id)
            return i;
    return -1;
}

int ModeloPreguntas::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_preguntas.size());
}

int ModeloPreguntas::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NumColumnas;
}

QVariant ModeloPreguntas::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    const Pregunta &p = m_preguntas.at(index.row());
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColTema:
            return QStringLiteral("T%1").arg(p.temaNumero);
        case ColCodigo:
            return p.old ? p.codigo() + QStringLiteral(" (old)") : p.codigo();
        case ColEstado: {
            QString e = p.estado;
            if (p.revisar)
                e += QStringLiteral(" · revisar");
            if (!p.imagen.isEmpty())
                e += QStringLiteral(" · imagen");
            return e;
        }
        case ColEnunciado: {
            QString t = p.enunciado;
            t.replace(QLatin1String("```"), QString());
            return t.simplified();
        }
        case ColCorrecta:
            return QString(QChar('A' + p.correcta));
        }
    } else if (role == Qt::ToolTipRole && index.column() == ColEnunciado) {
        QString t = p.enunciado;
        for (int i = 0; i < 4; ++i)
            t += QStringLiteral("\n%1) %2").arg(Pregunta::letra(i)).arg(p.opciones.value(i));
        return t;
    } else if (role == Qt::ForegroundRole && !p.activa()) {
        return QBrush(QColor(150, 150, 150));
    } else if (role == Qt::FontRole && !p.activa()) {
        QFont f;
        f.setStrikeOut(true);
        return f;
    } else if (role == Qt::TextAlignmentRole && index.column() != ColEnunciado) {
        return int(Qt::AlignCenter);
    }
    return {};
}

QVariant ModeloPreguntas::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case ColTema: return QStringLiteral("Tema");
    case ColCodigo: return QStringLiteral("Código");
    case ColEstado: return QStringLiteral("Estado");
    case ColEnunciado: return QStringLiteral("Enunciado");
    case ColCorrecta: return QStringLiteral("Resp.");
    }
    return {};
}

void FiltroPreguntas::setTema(int numeroTema)
{
    m_tema = numeroTema;
    invalidateFilter();
}

void FiltroPreguntas::setEstado(FiltroEstado estado)
{
    m_estado = estado;
    invalidateFilter();
}

void FiltroPreguntas::setTexto(const QString &texto)
{
    m_texto = texto.trimmed();
    invalidateFilter();
}

bool FiltroPreguntas::filterAcceptsRow(int fila, const QModelIndex &) const
{
    const auto *m = static_cast<const ModeloPreguntas *>(sourceModel());
    const Pregunta &p = m->pregunta(fila);
    if (m_tema != 0 && p.temaNumero != m_tema)
        return false;
    switch (m_estado) {
    case Todas: break;
    case Activas: if (!p.activa()) return false; break;
    case Anuladas: if (p.estado != QLatin1String("anulada")) return false; break;
    case Obsoletas: if (p.estado != QLatin1String("obsoleta")) return false; break;
    case ParaRevisar: if (!p.revisar) return false; break;
    case ConImagen: if (p.imagen.isEmpty()) return false; break;
    }
    if (m_texto.isEmpty())
        return true;
    if (p.codigoCompleto().compare(m_texto, Qt::CaseInsensitive) == 0 || p.codigo() == m_texto)
        return true;
    if (p.enunciado.contains(m_texto, Qt::CaseInsensitive) || p.notas.contains(m_texto, Qt::CaseInsensitive))
        return true;
    for (const QString &o : p.opciones)
        if (o.contains(m_texto, Qt::CaseInsensitive))
            return true;
    return false;
}

bool FiltroPreguntas::lessThan(const QModelIndex &a, const QModelIndex &b) const
{
    const auto *m = static_cast<const ModeloPreguntas *>(sourceModel());
    const Pregunta &x = m->pregunta(a.row());
    const Pregunta &y = m->pregunta(b.row());
    if (a.column() == ModeloPreguntas::ColTema || a.column() == ModeloPreguntas::ColCodigo) {
        if (x.temaNumero != y.temaNumero)
            return x.temaNumero < y.temaNumero;
        if (x.numero != y.numero)
            return x.numero < y.numero;
        return x.variante < y.variante;
    }
    return QSortFilterProxyModel::lessThan(a, b);
}
