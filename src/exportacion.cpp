#include "exportacion.h"
#include "utilidades.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPdfWriter>

namespace Exportacion {

namespace {

// Guarda las imágenes de las preguntas en la carpeta de al lado de 'ruta' y devuelve su nombre (relativo).
QString guardarImagenes(const QString &ruta, const QVector<Modelo> &modelos)
{
    const QFileInfo info(ruta);
    const QString carpeta = limpiarNombreFichero(info.completeBaseName()) + QStringLiteral("_imagenes");
    for (const Pregunta &p : Maquetador::preguntasDe(modelos)) {
        if (p.imagen.isEmpty())
            continue;
        QDir(info.absolutePath()).mkpath(carpeta);
        QFile img(info.absolutePath() + QLatin1Char('/') + carpeta + QLatin1Char('/') + Maquetador::nombreImagen(p));
        if (img.open(QIODevice::WriteOnly))
            img.write(p.imagen);
    }
    return carpeta;
}

bool escribir(const QString &ruta, const QString &texto)
{
    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    f.write(texto.toUtf8());
    return true;
}

} // namespace

bool pdf(const QString &ruta, const QVector<Modelo> &modelos, const DatosConvocatoria &datos, bool soluciones, int nivel)
{
    QPdfWriter pdf(ruta);
    Maquetador::configurarPdf(pdf);
    return Maquetador::imprimirExamenes(&pdf, modelos, datos, soluciones, nivel);
}

bool html(const QString &ruta, const QVector<Modelo> &modelos, const DatosConvocatoria &datos, bool soluciones, int nivel)
{
    QString html = Maquetador::examenesHtml(modelos, datos, soluciones, nivel);
    html.replace(QStringLiteral("<html>"),
                 QStringLiteral("<html><head><meta charset=\"utf-8\"><title>%1</title>"
                                "<style>p { margin-top: 0; margin-bottom: 2px; } td { vertical-align: top; }</style></head>")
                     .arg(Maquetador::escapar(datos.asignatura + QStringLiteral(" - ") + datos.convocatoriaYAnio())));
    // Las imágenes, que la maqueta sirve como recursos «img:<id>», pasan a los ficheros de la carpeta.
    const QString carpeta = guardarImagenes(ruta, modelos);
    for (const Pregunta &p : Maquetador::preguntasDe(modelos))
        html.replace(QStringLiteral("src=\"img:%1\"").arg(p.id), QStringLiteral("src=\"%1/%2\"").arg(carpeta, Maquetador::nombreImagen(p)));
    return escribir(ruta, html);
}

bool latex(const QString &ruta, const QVector<Modelo> &modelos, const DatosConvocatoria &datos, bool soluciones, int nivel)
{
    const QString carpeta = guardarImagenes(ruta, modelos);
    return escribir(ruta, Maquetador::examenesLatex(modelos, datos, soluciones, carpeta, nivel));
}

} // namespace Exportacion
