#include "atencionalverificar.h"
#include "pregunta.h"

#include <QRegularExpression>

namespace {

// Sin tocar nada este tiempo (milisegundos), ya se ha descansado.
constexpr int kDescanso = 3 * 60 * 1000;

} // namespace

int AtencionAlVerificar::tiempoParaLeer(const Pregunta &p)
{
    // Lo que se lee al verificar: el enunciado, las opciones y la explicación. A unas 400 palabras por minuto, que es
    // leer muy deprisa: por debajo no se ha leído. Entre 2 y 20 segundos.
    static const QRegularExpression espacios(QStringLiteral("\\s+"));
    const QString texto = p.enunciado + QLatin1Char(' ') + p.opciones.join(QLatin1Char(' ')) + QLatin1Char(' ') + p.explicacion;
    const int palabras = int(texto.split(espacios, Qt::SkipEmptyParts).size());
    return qBound(2000, palabras * 150, 20000);
}

void AtencionAlVerificar::empezar()
{
    m_sinParar.start();
    m_desdeLaPausa = 0;
}

void AtencionAlVerificar::preguntaMostrada()
{
    m_vista.start();
}

bool AtencionAlVerificar::demasiadoDeprisa(int indice, const Pregunta &p)
{
    if (m_avisadaDePrisa == indice || m_vista.elapsed() >= tiempoParaLeer(p))
        return false;
    m_avisadaDePrisa = indice;
    return true;
}

void AtencionAlVerificar::contarDescanso()
{
    if (m_vista.elapsed() > kDescanso)
        empezar();
}

bool AtencionAlVerificar::tocaPausa(int *minutos, int *verificadas)
{
    if (m_desdeLaPausa == 0 || m_sinParar.elapsed() < m_pausaCada)
        return false;
    *minutos = int(m_sinParar.elapsed() / 60000);
    *verificadas = m_desdeLaPausa;
    empezar();
    return true;
}
