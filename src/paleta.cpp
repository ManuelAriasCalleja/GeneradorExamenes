#include "paleta.h"

#include <QGuiApplication>
#include <QPalette>

namespace {

struct Tonos
{
    const char *sobreClaro;
    const char *sobreOscuro;
};

// En el orden de Paleta::Tinta.
constexpr Tonos kTintas[] = {
    {"#595959", "#b4b4b4"},   // Secundario
    {"#969696", "#8c8c8c"},   // Inactiva
    {"#a04a00", "#fbbf24"},   // Atencion
    {"#0a5d9c", "#8ab4f8"},   // Informacion
    {"#1d4ed8", "#8ab4f8"},   // Verificada
    {"#0a7d32", "#6fcf8a"},   // Correcta
    {"#c0392b", "#ff8a80"},   // Error
};

constexpr const char *kFondoAtencion = "#fff3cd";
constexpr const char *kTintaAtencion = "#664d03";
constexpr const char *kBordeAtencion = "#ffe69c";
constexpr const char *kFondoInformacion = "#e7f1fb";
constexpr const char *kTintaInformacion = "#0b3c6b";
constexpr const char *kBordeInformacion = "#b6d4f2";
constexpr const char *kFondoError = "#f8d7da";
constexpr const char *kTintaError = "#58151c";

QString fondo(const char *colorFondo, const char *colorTinta)
{
    return QStringLiteral("background: %1; color: %2").arg(QLatin1String(colorFondo), QLatin1String(colorTinta));
}

QString recuadro(const char *colorFondo, const char *colorTinta, const char *colorBorde)
{
    return fondo(colorFondo, colorTinta) + QStringLiteral("; border: 1px solid %1; padding: 6px").arg(QLatin1String(colorBorde));
}

} // namespace

bool Paleta::oscura()
{
    return QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
}

QColor Paleta::tinta(Tinta t, bool sobreOscuro)
{
    return QColor(QLatin1String(sobreOscuro ? kTintas[t].sobreOscuro : kTintas[t].sobreClaro));
}

QColor Paleta::tinta(Tinta t)
{
    return tinta(t, oscura());
}

QString Paleta::css(Tinta t)
{
    return QStringLiteral("color: %1").arg(tinta(t).name());
}

QString Paleta::fondoAtencion()
{
    return fondo(kFondoAtencion, kTintaAtencion);
}

QString Paleta::fondoError()
{
    return fondo(kFondoError, kTintaError);
}

QString Paleta::recuadroAtencion()
{
    return recuadro(kFondoAtencion, kTintaAtencion, kBordeAtencion);
}

QString Paleta::recuadroInformacion()
{
    return recuadro(kFondoInformacion, kTintaInformacion, kBordeInformacion);
}

QColor Paleta::colorFondoAtencion()
{
    return QColor(QLatin1String(kFondoAtencion));
}

QColor Paleta::colorTintaAtencion()
{
    return QColor(QLatin1String(kTintaAtencion));
}

QPalette Paleta::papel(const QPalette &base)
{
    QPalette p = base;
    for (QPalette::ColorGroup grupo : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        p.setColor(grupo, QPalette::Base, Qt::white);
        p.setColor(grupo, QPalette::Text, Qt::black);
        p.setColor(grupo, QPalette::Link, QColor(QLatin1String(kPapelVerificada)));
        p.setColor(grupo, QPalette::LinkVisited, QColor(QLatin1String(kPapelVerificada)));
    }
    return p;
}
