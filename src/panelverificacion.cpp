#include "panelverificacion.h"
#include "basedatos.h"
#include "dialogoverificadores.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

constexpr int kColumnas = 3;   // casillas de verificadores por fila

QStringList ordenados(QStringList nombres)
{
    nombres.sort(Qt::CaseInsensitive);
    return nombres;
}

} // namespace

PanelVerificacion::PanelVerificacion(BaseDatos *bd, bool conCasilla, QWidget *parent)
    : QWidget(parent), m_bd(bd)
{
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    if (conCasilla) {
        m_verificada = new QCheckBox(tr("Verificada por humanos"));
        m_verificada->setToolTip(tr("Una persona (al menos) ha revisado la pregunta. Al marcarla se marcan también los que "
                                    "verificaron la anterior, que suelen ser los mismos."));
        connect(m_verificada, &QCheckBox::toggled, this, &PanelVerificacion::verificadaCambiada);
        l->addWidget(m_verificada);
    }
    auto *fila = new QHBoxLayout;
    if (conCasilla)
        fila->setContentsMargins(20, 0, 0, 0);   // bajo la casilla, como lo que depende de ella
    //: Delante de las casillas de las personas que han verificado la pregunta: «Por: [ ] Ana  [ ] Luis»
    fila->addWidget(new QLabel(tr("Por:")), 0, Qt::AlignTop);
    m_rejilla = new QGridLayout;
    m_rejilla->setContentsMargins(0, 0, 0, 0);
    fila->addLayout(m_rejilla, 1);
    m_ninguno = new QLabel(tr("Aún no hay verificadores en este banco."));
    m_ninguno->setStyleSheet(QStringLiteral("color: gray"));
    fila->addWidget(m_ninguno, 1, Qt::AlignTop);
    //: Botón: abre la lista de verificadores del banco para añadir o quitar personas
    auto *gestionar = new QPushButton(tr("Añadir o quitar…"));
    gestionar->setAutoDefault(false);
    gestionar->setToolTip(tr("Añade verificadores al banco, les cambia el nombre o los quita"));
    connect(gestionar, &QPushButton::clicked, this, &PanelVerificacion::gestionar);
    fila->addWidget(gestionar, 0, Qt::AlignTop);
    l->addLayout(fila);
    construir({});
}

void PanelVerificacion::construir(const QStringList &marcados)
{
    qDeleteAll(m_casillas);
    m_casillas.clear();
    const QStringList nombres = m_bd->verificadores();
    for (int i = 0; i < nombres.size(); ++i) {
        auto *c = new QCheckBox(nombres.at(i));
        c->setChecked(marcados.contains(nombres.at(i)));
        connect(c, &QCheckBox::toggled, this, &PanelVerificacion::personaCambiada);
        m_rejilla->addWidget(c, i / kColumnas, i % kColumnas);
        m_casillas << c;
    }
    m_ninguno->setVisible(nombres.isEmpty());
}

void PanelVerificacion::ponerPregunta(const Pregunta &p)
{
    m_cambiando = true;
    if (m_verificada)
        m_verificada->setChecked(p.verificada);
    for (QCheckBox *c : std::as_const(m_casillas))
        c->setChecked(p.verificada && p.verificadores.contains(c->text()));
    m_cambiando = false;
    m_antes.clear();
    m_inicialVerificada = p.verificada;
    m_inicialVerificadores = p.verificada ? verificadores() : QStringList();
}

bool PanelVerificacion::verificada() const
{
    return m_verificada ? m_verificada->isChecked() : true;
}

QStringList PanelVerificacion::verificadores() const
{
    QStringList res;
    for (const QCheckBox *c : m_casillas)
        if (c->isChecked())
            res << c->text();
    return res;
}

bool PanelVerificacion::sinCambios() const
{
    if (verificada() != m_inicialVerificada)
        return false;
    return !verificada() || ordenados(verificadores()) == ordenados(m_inicialVerificadores);
}

void PanelVerificacion::marcar(const QStringList &nombres)
{
    for (QCheckBox *c : std::as_const(m_casillas))
        if (nombres.contains(c->text()))
            c->setChecked(true);
}

void PanelVerificacion::marcarVerificada()
{
    if (m_verificada && !m_verificada->isChecked())
        m_verificada->setChecked(true);   // marca a los últimos (verificadaCambiada())
    else if (verificadores().isEmpty())
        marcar(m_bd->ultimosVerificadores());
}

void PanelVerificacion::recordar()
{
    if (verificada() && !verificadores().isEmpty())
        m_bd->guardarUltimosVerificadores(verificadores());
}

void PanelVerificacion::verificadaCambiada(bool marcada)
{
    if (m_cambiando)
        return;
    m_cambiando = true;
    if (!marcada) {
        m_antes = verificadores();
        for (QCheckBox *c : std::as_const(m_casillas))
            c->setChecked(false);
    } else if (verificadores().isEmpty()) {
        // Los que había antes de desmarcarla; si no, los que verificaron la última.
        marcar(m_antes.isEmpty() ? m_bd->ultimosVerificadores() : m_antes);
    }
    m_cambiando = false;
    emit cambiado();
}

void PanelVerificacion::personaCambiada(bool marcada)
{
    if (m_cambiando)
        return;
    if (marcada && m_verificada && !m_verificada->isChecked()) {
        m_cambiando = true;
        m_verificada->setChecked(true);
        m_cambiando = false;
    }
    emit cambiado();
}

void PanelVerificacion::gestionar()
{
    DialogoVerificadores d(m_bd, this);
    d.exec();
    if (!d.hayCambios())
        return;
    // Los marcados (y los que tenía la pregunta) siguen marcados con su nombre nuevo; los quitados, no.
    auto renombrar = [&d](QStringList nombres) {
        for (QString &n : nombres)
            n = d.renombrados().value(n, n);
        return nombres;
    };
    const QStringList existentes = m_bd->verificadores();
    auto quedan = [&existentes](const QStringList &nombres) {
        QStringList res;
        for (const QString &n : nombres)
            if (existentes.contains(n))
                res << n;
        return res;
    };
    const QStringList marcados = quedan(renombrar(verificadores()));
    m_inicialVerificadores = quedan(renombrar(m_inicialVerificadores));
    m_antes = quedan(renombrar(m_antes));
    m_cambiando = true;
    construir(marcados);
    m_cambiando = false;
    emit verificadoresCambiados();
    emit cambiado();
}
