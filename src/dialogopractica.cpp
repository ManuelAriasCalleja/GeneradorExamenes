#include "dialogopractica.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

DialogoPractica::DialogoPractica(const Modelo &modelo, const DatosConvocatoria &datos, QWidget *parent)
    : QDialog(parent), m_modelo(modelo), m_datos(datos), m_segundosRestantes(datos.duracionMinutos * 60)
{
    setWindowTitle(QStringLiteral("Practicar: %1 - %2").arg(datos.asignatura, modelo.nombre));
    resize(900, 800);

    auto *titulo = new QLabel(QStringLiteral("<b>%1</b> &mdash; %2 &mdash; %3 preguntas<br>"
                                             "<small>Acierto: +%4 &nbsp; Fallo: −%5 &nbsp; En blanco: 0</small>")
                                  .arg(Maquetador::escapar(datos.asignatura), Maquetador::escapar(modelo.nombre))
                                  .arg(modelo.preguntas.size())
                                  .arg(Maquetador::formatoPuntos(datos.puntosAcierto), Maquetador::formatoPuntos(datos.puntosFallo)));
    m_reloj = new QLabel;
    m_reloj->setStyleSheet(QStringLiteral("font-size: 16pt; font-weight: bold"));
    m_nota = new QLabel;
    m_nota->setStyleSheet(QStringLiteral("font-size: 14pt; font-weight: bold; color: #0a5d9c"));
    auto *cabecera = new QHBoxLayout;
    cabecera->addWidget(titulo, 1);
    cabecera->addWidget(m_nota);
    cabecera->addSpacing(20);
    cabecera->addWidget(m_reloj);

    auto *contenido = new QWidget;
    auto *lc = new QVBoxLayout(contenido);
    for (int i = 0; i < modelo.preguntas.size(); ++i) {
        const PreguntaExamen &pe = modelo.preguntas.at(i);
        auto *marco = new QFrame;
        marco->setFrameShape(QFrame::StyledPanel);
        auto *g = new QGridLayout(marco);
        g->setColumnStretch(1, 1);

        auto *enunciado = new QLabel(QStringLiteral("<div style=\"font-weight:bold; font-style:italic\">%1</div>")
                                         .arg(Maquetador::enunciadoHtml(pe.pregunta.enunciado, QStringLiteral("%1.- ").arg(i + 1))));
        enunciado->setWordWrap(true);
        enunciado->setTextFormat(Qt::RichText);
        g->addWidget(enunciado, 0, 0, 1, 3);
        int fila = 1;
        if (!pe.pregunta.imagen.isEmpty()) {
            QImage img;
            if (img.loadFromData(pe.pregunta.imagen)) {
                auto *dibujo = new QLabel;
                dibujo->setPixmap(QPixmap::fromImage(img.scaledToHeight(qMin(img.height(), 150), Qt::SmoothTransformation)));
                g->addWidget(dibujo, fila++, 1);
            }
        }

        Fila f;
        f.grupo = new QButtonGroup(this);
        for (int k = 0; k < 4; ++k) {
            auto *radio = new QRadioButton(QStringLiteral("%1)").arg(Pregunta::letra(k)));
            f.grupo->addButton(radio, k);
            f.opciones[k] = new QLabel(Maquetador::escapar(pe.opcion(k)));
            f.opciones[k]->setWordWrap(true);
            f.opciones[k]->setProperty("radio", QVariant::fromValue<QObject *>(radio));
            f.opciones[k]->installEventFilter(this);
            g->addWidget(radio, fila, 0, Qt::AlignTop);
            g->addWidget(f.opciones[k], fila, 1);
            ++fila;
        }
        auto *blanco = new QToolButton;
        blanco->setText(QStringLiteral("Dejar en blanco"));
        blanco->setAutoRaise(true);
        QButtonGroup *grupo = f.grupo;
        connect(blanco, &QToolButton::clicked, this, [grupo, this]() {
            if (m_corregido || !grupo->checkedButton())
                return;
            grupo->setExclusive(false);
            grupo->checkedButton()->setChecked(false);
            grupo->setExclusive(true);
        });
        g->addWidget(blanco, 1, 2, Qt::AlignTop | Qt::AlignRight);
        f.blanco = blanco;
        f.resultado = new QLabel;
        f.resultado->setWordWrap(true);
        f.resultado->hide();
        g->addWidget(f.resultado, fila, 0, 1, 3);
        m_filas << f;
        lc->addWidget(marco);
    }
    lc->addStretch();

    auto *scroll = new QScrollArea;
    scroll->setWidget(contenido);
    scroll->setWidgetResizable(true);

    m_corregir = new QPushButton(QStringLiteral("Terminar y corregir"));
    m_corregir->setDefault(true);
    connect(m_corregir, &QPushButton::clicked, this, &DialogoPractica::corregir);
    auto *botones = new QDialogButtonBox;
    botones->addButton(QStringLiteral("Cerrar"), QDialogButtonBox::RejectRole);
    botones->addButton(m_corregir, QDialogButtonBox::ActionRole);
    connect(botones, &QDialogButtonBox::rejected, this, &DialogoPractica::reject);

    auto *l = new QVBoxLayout(this);
    l->addLayout(cabecera);
    l->addWidget(scroll, 1);
    l->addWidget(botones);

    m_temporizador = new QTimer(this);
    connect(m_temporizador, &QTimer::timeout, this, &DialogoPractica::tic);
    m_temporizador->start(1000);
    tic();
}

bool DialogoPractica::eventFilter(QObject *objeto, QEvent *evento)
{
    if (evento->type() == QEvent::MouseButtonPress && !m_corregido) {
        if (auto *radio = qobject_cast<QRadioButton *>(objeto->property("radio").value<QObject *>()))
            radio->setChecked(true);
    }
    return QDialog::eventFilter(objeto, evento);
}

void DialogoPractica::tic()
{
    if (m_corregido)
        return;
    if (m_segundosRestantes <= 0) {
        m_reloj->setText(QStringLiteral("0:00"));
        m_temporizador->stop();
        QMessageBox::information(this, windowTitle(), QStringLiteral("Se ha acabado el tiempo."));
        corregir();
        return;
    }
    m_reloj->setText(QStringLiteral("%1:%2").arg(m_segundosRestantes / 60).arg(m_segundosRestantes % 60, 2, 10, QLatin1Char('0')));
    m_reloj->setStyleSheet(m_segundosRestantes < 300 ? QStringLiteral("font-size: 16pt; font-weight: bold; color: #c0392b")
                                                     : QStringLiteral("font-size: 16pt; font-weight: bold"));
    --m_segundosRestantes;
}

void DialogoPractica::corregir()
{
    if (m_corregido)
        return;
    m_corregido = true;
    m_temporizador->stop();
    m_corregir->setEnabled(false);

    int aciertos = 0, fallos = 0, blancos = 0;
    for (int i = 0; i < m_filas.size(); ++i) {
        Fila &f = m_filas[i];
        const PreguntaExamen &pe = m_modelo.preguntas.at(i);
        const int correcta = pe.correcta();
        const int marcada = f.grupo->checkedId();
        for (QAbstractButton *b : f.grupo->buttons())
            b->setEnabled(false);
        f.blanco->hide();
        f.opciones[correcta]->setStyleSheet(QStringLiteral("color: %1; font-weight: bold").arg(QLatin1String(Maquetador::kColorCorrecta)));
        QString texto;
        if (marcada < 0) {
            ++blancos;
            texto = QStringLiteral("<span style=\"color:#7f8c8d\"><b>En blanco.</b></span>");
        } else if (marcada == correcta) {
            ++aciertos;
            texto = QStringLiteral("<span style=\"color:%1\"><b>¡Correcto!</b></span>").arg(QLatin1String(Maquetador::kColorCorrecta));
        } else {
            ++fallos;
            f.opciones[marcada]->setStyleSheet(QStringLiteral("color: #c0392b; text-decoration: line-through"));
            texto = QStringLiteral("<span style=\"color:#c0392b\"><b>Incorrecto.</b></span> La respuesta correcta es la %1).")
                        .arg(Pregunta::letra(correcta));
        }
        texto += QStringLiteral(" <small>[%1] %2</small>").arg(Maquetador::escapar(pe.pregunta.codigoCompleto()),
                                                                  Maquetador::escapar(pe.pregunta.referencia));
        if (!pe.pregunta.explicacion.isEmpty())
            texto += QStringLiteral("<br><small>%1</small>").arg(Maquetador::escapar(pe.pregunta.explicacion));
        f.resultado->setText(texto);
        f.resultado->show();
    }
    const double nota = aciertos * m_datos.puntosAcierto - fallos * m_datos.puntosFallo;
    const double maximo = m_filas.size() * m_datos.puntosAcierto;
    m_nota->setText(QStringLiteral("Nota: %1 / %2").arg(Maquetador::formatoPuntos(nota), Maquetador::formatoPuntos(maximo)));
    QMessageBox::information(this, windowTitle(),
                             QStringLiteral("Aciertos: %1\nFallos: %2\nEn blanco: %3\n\nNota: %4 sobre %5")
                                 .arg(aciertos).arg(fallos).arg(blancos)
                                 .arg(Maquetador::formatoPuntos(nota), Maquetador::formatoPuntos(maximo)));
}
