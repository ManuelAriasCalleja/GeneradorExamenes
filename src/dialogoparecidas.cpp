#include "dialogoparecidas.h"
#include "dialogoayuda.h"
#include "maquetador.h"
#include "utilidades.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QTextBrowser>
#include <QVBoxLayout>

DialogoParecidas::DialogoParecidas(const Pregunta &nueva, const QVector<Parecidas::Parecida> &parecidas,
                                   const QVector<Tema> &temas, QWidget *parent)
    : QDialog(parent), m_nueva(nueva), m_parecidas(parecidas), m_temas(temas)
{
    setWindowTitle(tr("Preguntas parecidas"));
    resize(900, 520);

    auto *explicacion = new QLabel(tr(
        "La pregunta nueva se parece mucho a estas del banco. Si es la misma idea (otro dato, la pregunta al revés u "
        "otro aspecto del mismo concepto), guárdela como variante de la existente: así nunca saldrán las dos en el "
        "mismo examen. Si es de otro tema, enlácelas como pseudorrepetidas."));
    explicacion->setWordWrap(true);

    m_lista = new QListWidget;
    for (const Parecidas::Parecida &p : parecidas)
        m_lista->addItem(QStringLiteral("%1 · %2 %").arg(p.pregunta.codigoCompleto()).arg(qRound(p.parecido * 100)));
    m_ficha = new QTextBrowser;
    auto *divisor = new QSplitter;
    divisor->addWidget(m_lista);
    divisor->addWidget(m_ficha);
    divisor->setStretchFactor(1, 1);
    divisor->setSizes({180, 700});
    connect(m_lista, &QListWidget::currentRowChanged, this, &DialogoParecidas::mostrarElegida);

    m_variante = new QPushButton;
    m_pseudorrepetida = new QPushButton;
    auto *nuevaBoton = new QPushButton(tr("Guardar como pregunta nueva"));
    auto *seguir = new QPushButton(tr("Seguir editando"));
    connect(m_variante, &QPushButton::clicked, this, [this]() { decidir(ComoVariante); });
    connect(m_pseudorrepetida, &QPushButton::clicked, this, [this]() { decidir(ComoPseudorrepetida); });
    connect(nuevaBoton, &QPushButton::clicked, this, [this]() { decidir(ComoNueva); });
    connect(seguir, &QPushButton::clicked, this, &DialogoParecidas::reject);
    DialogoAyuda::ponerAyuda(this, nullptr, QStringLiteral("parecidas"));
    auto *botones = new QHBoxLayout;
    botones->addWidget(m_variante);
    botones->addWidget(m_pseudorrepetida);
    botones->addStretch();
    botones->addWidget(nuevaBoton);
    botones->addWidget(seguir);

    auto *l = new QVBoxLayout(this);
    l->addWidget(explicacion);
    l->addWidget(divisor, 1);
    l->addLayout(botones);
    m_lista->setCurrentRow(0);
}

Pregunta DialogoParecidas::elegida() const
{
    const int i = m_lista->currentRow();
    return i >= 0 && i < m_parecidas.size() ? m_parecidas.at(i).pregunta : Pregunta();
}

void DialogoParecidas::mostrarElegida()
{
    const Pregunta p = elegida();
    ponerDocumento(m_ficha, Maquetador::crearDocumento(Maquetador::fichaPreguntaHtml(p, nombreTema(m_temas, p.temaId)), {p}, m_ficha));

    // Una variante es del mismo tema; con una pregunta de otro tema, se enlazan como pseudorrepetidas.
    const bool mismoTema = p.temaNumero == m_nueva.temaNumero;
    m_variante->setText(tr("Guardar como variante de %1").arg(p.grupo()));
    m_variante->setEnabled(mismoTema);
    m_pseudorrepetida->setText(tr("Enlazar con %1 (pseudorrepetida)").arg(p.grupo()));
    m_pseudorrepetida->setEnabled(!mismoTema);
}

void DialogoParecidas::decidir(Decision decision)
{
    m_decision = decision;
    accept();
}
