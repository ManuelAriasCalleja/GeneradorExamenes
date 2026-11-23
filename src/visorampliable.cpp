#include "visorampliable.h"

#include "utilidades.h"

#include <QAbstractTextDocumentLayout>
#include <QAction>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextTable>
#include <QWheelEvent>
#include <array>
#include <cmath>

namespace {

// Escalas por las que se pasa al ampliar y al reducir, como en los navegadores.
constexpr std::array<qreal, 13> kNiveles = {0.5, 0.67, 0.75, 0.8, 0.9, 1.0, 1.1, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0};

// Propiedades en las que se guardan los tamaños originales de un formato (0: no tenía).
enum {
    kPuntosOriginales = QTextFormat::UserProperty + 0x7a00,
    kPixelesOriginales,
    kAnchoOriginal,
    kAltoOriginal,
    kColumnasOriginales,
    kAnchoTablaOriginal,
};

// Propiedad del documento en la que se guarda su letra por defecto original.
constexpr const char *kLetraOriginal = "letraOriginal";

// 'f' con sus tamaños a 'escala' veces los originales. Los trozos sin tamaño propio no se tocan: llevan el de la letra
// por defecto del documento, que se escala aparte (y también los títulos, cuyo tamaño Qt calcula a partir de ella).
QTextCharFormat escalado(QTextCharFormat f, qreal escala)
{
    if (!f.hasProperty(kPuntosOriginales)) {
        if (escala == 1)
            return f;
        f.setProperty(kPuntosOriginales, f.doubleProperty(QTextFormat::FontPointSize));
        f.setProperty(kPixelesOriginales, f.intProperty(QTextFormat::FontPixelSize));
        if (f.isImageFormat()) {
            f.setProperty(kAnchoOriginal, f.doubleProperty(QTextFormat::ImageWidth));
            f.setProperty(kAltoOriginal, f.doubleProperty(QTextFormat::ImageHeight));
        }
    }
    const qreal puntos = f.doubleProperty(kPuntosOriginales);
    const int pixeles = f.intProperty(kPixelesOriginales);
    if (puntos > 0)
        f.setFontPointSize(puntos * escala);
    if (pixeles > 0)
        f.setProperty(QTextFormat::FontPixelSize, qMax(1, qRound(pixeles * escala)));
    if (f.isImageFormat()) {
        const qreal ancho = f.doubleProperty(kAnchoOriginal);
        const qreal alto = f.doubleProperty(kAltoOriginal);
        if (ancho > 0)
            f.setProperty(QTextFormat::ImageWidth, ancho * escala);
        if (alto > 0)
            f.setProperty(QTextFormat::ImageHeight, alto * escala);
    }
    return f;
}

// Apunta en 'tablas' las de 'marco' (y las de dentro) cuyo formato cambia con la escala: el ancho de las columnas y de
// la tabla, si es fijo, para que lo que cabía en una casilla siga cabiendo.
void tablasEscaladas(QTextFrame *marco, qreal escala, QVector<QPair<QTextTable *, QTextTableFormat>> *tablas)
{
    for (QTextFrame *hijo : marco->childFrames()) {
        tablasEscaladas(hijo, escala, tablas);
        auto *tabla = qobject_cast<QTextTable *>(hijo);
        if (!tabla)
            continue;
        QTextTableFormat f = tabla->format();
        if (!f.hasProperty(kColumnasOriginales)) {
            if (escala == 1)
                continue;
            f.setProperty(kColumnasOriginales, f.columnWidthConstraints());
            f.setProperty(kAnchoTablaOriginal, f.width());
        }
        auto escalada = [escala](const QTextLength &l) {
            return l.type() == QTextLength::FixedLength ? QTextLength(QTextLength::FixedLength, l.rawValue() * escala) : l;
        };
        QList<QTextLength> columnas = f.lengthVectorProperty(kColumnasOriginales);
        for (QTextLength &c : columnas)
            c = escalada(c);
        f.setColumnWidthConstraints(columnas);
        f.setWidth(escalada(f.lengthProperty(kAnchoTablaOriginal)));
        if (f != tabla->format())
            tablas->append({tabla, f});
    }
}

} // namespace

VisorAmpliable::VisorAmpliable(QWidget *parent)
    : QTextBrowser(parent)
{
    // Atajos sin repetir: en algunos sistemas «Ctrl+=» ya es uno de los de ampliar, y una tecla repetida en la misma
    // acción la haría ambigua.
    auto atajos = [](QKeySequence::StandardKey estandar, std::initializer_list<QKeySequence> otros) {
        QList<QKeySequence> lista = QKeySequence::keyBindings(estandar);
        for (const QKeySequence &k : otros)
            if (!lista.contains(k))
                lista << k;
        return lista;
    };
    auto accion = [this](const QString &texto, const QList<QKeySequence> &teclas, void (VisorAmpliable::*slot)()) {
        auto *a = new QAction(texto, this);
        a->setShortcuts(teclas);
        a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(a, &QAction::triggered, this, slot);
        addAction(a);
        return a;
    };
    //: Acción de la vista del examen (Ctrl++): letra más grande, solo en pantalla
    m_ampliar = accion(tr("Ampliar la vista"), atajos(QKeySequence::ZoomIn, {QKeySequence(Qt::CTRL | Qt::Key_Equal)}),
                       &VisorAmpliable::ampliar);
    //: Acción de la vista del examen (Ctrl+-): letra más pequeña, solo en pantalla
    m_reducir = accion(tr("Reducir la vista"), atajos(QKeySequence::ZoomOut, {}), &VisorAmpliable::reducir);
    //: Acción de la vista del examen (Ctrl+0): vuelve al tamaño normal
    m_normal = accion(tr("Tamaño normal"), {QKeySequence(Qt::CTRL | Qt::Key_0)}, &VisorAmpliable::tamanoNormal);
    actualizarAcciones();
}

QList<QAction *> VisorAmpliable::accionesEscala() const
{
    return {m_ampliar, m_reducir, m_normal};
}

void VisorAmpliable::ponerEscala(qreal escala)
{
    escala = escalaValida(escala);
    if (qAbs(escala - m_escala) < 0.001)
        return;
    // Lo que se ve arriba de la vista sigue arriba.
    const QTextCursor arriba = cursorForPosition(QPoint(0, 0));
    const int oculto = cursorRect(arriba).top();   // lo que su línea tiene por encima de la vista (0 o menos)
    const bool alPrincipio = verticalScrollBar()->value() == 0;
    m_escala = escala;
    escalarDocumento(document(), m_escala);
    // Las barras de desplazamiento se ponen al día solas, pero más tarde: aquí, ya, para poder ir a su sitio.
    const QSizeF tamano = document()->documentLayout()->documentSize();   // maqueta todo el documento
    QScrollBar *barra = verticalScrollBar();
    barra->setRange(0, qMax(0, int(std::ceil(tamano.height())) - viewport()->height()));
    if (!alPrincipio)
        barra->setValue(barra->value() + cursorRect(arriba).top() - oculto);
    actualizarAcciones();
    emit escalaCambiada(m_escala);
}

void VisorAmpliable::ponerDocumento(QTextDocument *doc)
{
    ::ponerDocumento(this, doc);
    if (m_escala != 1)
        escalarDocumento(doc, m_escala);
}

qreal VisorAmpliable::escalaSiguiente(qreal escala)
{
    for (qreal n : kNiveles)
        if (n > escala + 0.001)
            return n;
    return escala;
}

qreal VisorAmpliable::escalaAnterior(qreal escala)
{
    for (auto it = kNiveles.rbegin(); it != kNiveles.rend(); ++it)
        if (*it < escala - 0.001)
            return *it;
    return escala;
}

qreal VisorAmpliable::escalaValida(qreal escala)
{
    return qBound(kNiveles.front(), escala, kNiveles.back());
}

void VisorAmpliable::quitarAtajos()
{
    for (QAction *a : accionesEscala()) {
        if (a->shortcuts().isEmpty())
            continue;
        a->setText(a->text() + QLatin1Char('\t') + a->shortcut().toString(QKeySequence::NativeText));
        a->setShortcuts(QList<QKeySequence>());
    }
}

void VisorAmpliable::ampliar()
{
    ponerEscala(escalaSiguiente(m_escala));
}

void VisorAmpliable::reducir()
{
    ponerEscala(escalaAnterior(m_escala));
}

void VisorAmpliable::tamanoNormal()
{
    ponerEscala(1);
}

void VisorAmpliable::wheelEvent(QWheelEvent *e)
{
    if (!(e->modifiers() & Qt::ControlModifier)) {
        m_giro = 0;
        QTextBrowser::wheelEvent(e);
        return;
    }
    // Un paso de la rueda son 120; las ruedas finas y los paneles táctiles dan pasos más cortos, que se van sumando.
    m_giro += e->angleDelta().y();
    for (; m_giro >= 120; m_giro -= 120)
        ampliar();
    for (; m_giro <= -120; m_giro += 120)
        reducir();
    e->accept();
}

void VisorAmpliable::changeEvent(QEvent *e)
{
    // Cuando cambia la letra del visor (la de la ventana, o la del panel en el que está: ZonaAmpliable), QTextEdit se
    // la pone al documento como letra por defecto. La de los documentos que se ven aquí es la suya (Maquetador), con
    // la escala de la vista: se deja como estaba.
    if (e->type() != QEvent::FontChange || !document()) {
        QTextBrowser::changeEvent(e);
        return;
    }
    const QFont letra = document()->defaultFont();
    QTextBrowser::changeEvent(e);
    if (document()->defaultFont() != letra)
        document()->setDefaultFont(letra);
}

void VisorAmpliable::actualizarAcciones()
{
    m_ampliar->setEnabled(m_escala < kNiveles.back() - 0.001);
    m_reducir->setEnabled(m_escala > kNiveles.front() + 0.001);
    m_normal->setEnabled(qAbs(m_escala - 1) >= 0.001);
}

void VisorAmpliable::escalarDocumento(QTextDocument *doc, qreal escala)
{
    // Primero se apuntan los cambios y luego se hacen, porque al cambiar el formato de un trozo el documento puede
    // juntarlo con el siguiente. El formato de un bloque (el de su separador) es el de su línea si está vacía.
    struct Cambio
    {
        int inicio;
        int fin;   // -1: es el formato del bloque
        QTextCharFormat formato;
    };
    QVector<Cambio> cambios;
    for (QTextBlock b = doc->begin(); b.isValid(); b = b.next()) {
        const QTextCharFormat delBloque = escalado(b.charFormat(), escala);
        if (delBloque != b.charFormat())
            cambios.append({b.position(), -1, delBloque});
        for (auto it = b.begin(); !it.atEnd(); ++it) {
            const QTextFragment trozo = it.fragment();
            const QTextCharFormat f = escalado(trozo.charFormat(), escala);
            if (f != trozo.charFormat())
                cambios.append({trozo.position(), trozo.position() + trozo.length(), f});
        }
    }
    // La letra por defecto.
    if (!doc->property(kLetraOriginal).isValid() && escala != 1)
        doc->setProperty(kLetraOriginal, doc->defaultFont());
    if (doc->property(kLetraOriginal).isValid()) {
        const QFont original = doc->property(kLetraOriginal).value<QFont>();
        QFont letra = original;
        if (original.pointSizeF() > 0)
            letra.setPointSizeF(original.pointSizeF() * escala);
        else if (original.pixelSize() > 0)
            letra.setPixelSize(qMax(1, qRound(original.pixelSize() * escala)));
        if (letra != doc->defaultFont())
            doc->setDefaultFont(letra);
    }
    QVector<QPair<QTextTable *, QTextTableFormat>> tablas;
    tablasEscaladas(doc->rootFrame(), escala, &tablas);
    if (cambios.isEmpty() && tablas.isEmpty())
        return;
    // Sin guardar cada cambio para poder deshacerlo: no hace falta y es más rápido.
    const bool deshacer = doc->isUndoRedoEnabled();
    doc->setUndoRedoEnabled(false);
    QTextCursor c(doc);
    c.beginEditBlock();
    for (const Cambio &cambio : std::as_const(cambios)) {
        c.setPosition(cambio.inicio);
        if (cambio.fin < 0) {
            c.setBlockCharFormat(cambio.formato);
        } else {
            c.setPosition(cambio.fin, QTextCursor::KeepAnchor);
            c.setCharFormat(cambio.formato);
        }
    }
    for (const auto &[tabla, formato] : std::as_const(tablas))
        tabla->setFormat(formato);
    c.endEditBlock();
    doc->setUndoRedoEnabled(deshacer);
}
