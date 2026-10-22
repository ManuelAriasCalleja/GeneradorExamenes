#include "ventanaprincipal.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

#ifndef RUTA_BD_POR_DEFECTO
#define RUTA_BD_POR_DEFECTO "preguntas.db"
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("UNED-FI"));
    QApplication::setApplicationName(QStringLiteral("GeneradorExamenesFI"));
    QApplication::setApplicationDisplayName(QStringLiteral("Generador de exámenes FI"));
    QLocale::setDefault(QLocale(QLocale::Spanish, QLocale::Spain));

    QIcon icono;
    for (int lado : {16, 24, 32, 48, 64, 128, 256})
        icono.addFile(QStringLiteral(":/recursos/icono/icono_%1.png").arg(lado), QSize(lado, lado));
    QApplication::setWindowIcon(icono);

    // Diálogos estándar de Qt en castellano, si están instaladas las traducciones.
    QTranslator traductor;
    if (traductor.load(QLocale(QLocale::Spanish), QStringLiteral("qtbase"), QStringLiteral("_"),
                       QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&traductor);

    QCommandLineParser opciones;
    opciones.setApplicationDescription(QStringLiteral("Generador de exámenes tipo test de Fundamentos de Informática"));
    opciones.addHelpOption();
    opciones.addPositionalArgument(QStringLiteral("basedatos"), QStringLiteral("Fichero SQLite de preguntas (se crea si no existe)"));
    opciones.process(app);

    // Base de datos: la indicada en la línea de órdenes, la última usada o la de la carpeta datos/ del proyecto.
    QString ruta = opciones.positionalArguments().value(0);
    if (ruta.isEmpty()) {
        ruta = QSettings().value(QStringLiteral("bd/ruta")).toString();
        if (ruta.isEmpty() || !QFileInfo::exists(ruta))
            ruta = QString::fromUtf8(RUTA_BD_POR_DEFECTO);
    }

    VentanaPrincipal ventana;
    ventana.show();
    ventana.abrirBaseDatos(QFileInfo(ruta).absoluteFilePath());
    return app.exec();
}
